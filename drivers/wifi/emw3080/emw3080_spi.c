/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mxchip_emw3080

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(emw3080, CONFIG_EMW3080_LOG_LEVEL);

#include "emw3080_transport.h"

#define SPI_HEADER_SIZE 8
#define SPI_WRITE_TYPE  0x0A
#define SPI_READ_TYPE   0x0B
#define SPI_MAX_PAYLOAD 256

struct emw3080_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec cs;
	struct gpio_dt_spec enable;
	struct gpio_dt_spec flow;
	struct gpio_dt_spec notify;
};

struct emw3080_data {
	struct spi_config spi_config;
	struct gpio_callback flow_cb;
	struct gpio_callback notify_cb;
	struct k_sem flow_sem;
	struct k_sem notify_sem;
	struct k_mutex mutex;
	uint8_t tx_buffer[SPI_MAX_PAYLOAD];
	uint8_t rx_buffer[SPI_MAX_PAYLOAD];
};

static void flow_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct emw3080_data *data = CONTAINER_OF(cb, struct emw3080_data, flow_cb);

	k_sem_give(&data->flow_sem);
}

static void notify_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct emw3080_data *data = CONTAINER_OF(cb, struct emw3080_data, notify_cb);

	k_sem_give(&data->notify_sem);
}

static int wait_for_flow(const struct device *dev)
{
	const struct emw3080_config *cfg = dev->config;
	struct emw3080_data *data = dev->data;

	int ret = k_sem_take(&data->flow_sem, K_MSEC(500));
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_get_dt(&cfg->flow);
	return ret < 0 ? ret : (ret ? 0 : -EIO);
}

static int wait_for_notify(const struct device *dev)
{
	const struct emw3080_config *cfg = dev->config;
	struct emw3080_data *data = dev->data;

	int ret = gpio_pin_get_dt(&cfg->notify);
	if (ret < 0) {
		return ret;
	}

	if (ret == 0) {
		ret = k_sem_take(&data->notify_sem, K_MSEC(2000));
		if (ret < 0) {
			return ret;
		}
	}

	ret = gpio_pin_get_dt(&cfg->notify);
	return ret < 0 ? ret : (ret ? 0 : -EIO);
}

static int spi_exchange(const struct device *dev, const uint8_t *tx, uint8_t *rx, size_t len)
{
	const struct emw3080_config *cfg = dev->config;
	struct emw3080_data *data = dev->data;

	struct spi_buf tx_buf = {
		.buf = (void *)tx,
		.len = len,
	};
	struct spi_buf rx_buf = {
		.buf = rx,
		.len = len,
	};
	struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};
	struct spi_buf_set rx_set = {
		.buffers = &rx_buf,
		.count = 1,
	};

	return spi_transceive(cfg->spi.bus, &data->spi_config, tx != NULL ? &tx_set : NULL,
			      rx != NULL ? &rx_set : NULL);
}

static int spi_message(const struct device *dev, const uint8_t *tx, size_t tx_len, uint8_t *rx,
		       size_t rx_size, size_t *rx_len)
{
	const struct emw3080_config *cfg = dev->config;
	struct emw3080_data *data = dev->data;

	uint8_t header[SPI_HEADER_SIZE] = {SPI_WRITE_TYPE,
					   (uint8_t)tx_len,
					   (uint8_t)(tx_len >> 8),
					   (uint8_t)(~tx_len),
					   (uint8_t)((~tx_len) >> 8),
					   0,
					   0,
					   0};
	uint8_t reply[SPI_HEADER_SIZE] = {0};
	uint16_t received_len;
	uint16_t received_inv;
	size_t payload_len;
	int ret;

	if (tx_len > SPI_MAX_PAYLOAD || (tx_len > 0 && tx == NULL) || (rx_size > 0 && rx == NULL)) {
		return -EINVAL;
	}

	k_sem_reset(&data->flow_sem);

	ret = gpio_pin_set_dt(&cfg->cs, 1);
	if (ret < 0) {
		return ret;
	}

	ret = wait_for_flow(dev);
	if (ret < 0) {
		goto out;
	}

	ret = spi_exchange(dev, header, reply, sizeof(header));
	if (ret < 0) {
		goto out;
	}

	received_len = (uint16_t)reply[1] | ((uint16_t)reply[2] << 8);
	received_inv = (uint16_t)reply[3] | ((uint16_t)reply[4] << 8);

	if (reply[0] != SPI_READ_TYPE || (uint16_t)(received_len ^ received_inv) != UINT16_MAX) {
		ret = -EBADMSG;
		goto out;
	}

	if (received_len > SPI_MAX_PAYLOAD || received_len > rx_size) {
		ret = -EMSGSIZE;
		goto out;
	}

	payload_len = MAX(tx_len, received_len);

	if (payload_len > 0) {
		memcpy(data->tx_buffer, tx, tx_len);

		ret = wait_for_flow(dev);
		if (ret < 0) {
			goto out;
		}

		ret = spi_exchange(dev, data->tx_buffer, data->rx_buffer, payload_len);
		if (ret < 0) {
			goto out;
		}

		if (received_len > 0) {
			memcpy(rx, data->rx_buffer, received_len);
		}
	}

	if (rx_len != NULL) {
		*rx_len = received_len;
	}

out: {
	int cs_ret = gpio_pin_set_dt(&cfg->cs, 0);

	if (ret == 0) {
		ret = cs_ret;
	}
}

	return ret;
}

int emw3080_transport_send(const struct device *dev, const uint8_t *buf, size_t len)
{
	struct emw3080_data *data = dev->data;

	k_sem_reset(&data->notify_sem);

	return spi_message(dev, buf, len, NULL, 0, NULL);
}

int emw3080_transport_receive(const struct device *dev, uint8_t *buf, size_t size, size_t *len)
{
	int ret = wait_for_notify(dev);

	if (ret < 0) {
		return ret;
	}

	return spi_message(dev, NULL, 0, buf, size, len);
}

int emw3080_transport_lock(const struct device *dev)
{
	struct emw3080_data *data = dev->data;

	return k_mutex_lock(&data->mutex, K_FOREVER);
}

void emw3080_transport_unlock(const struct device *dev)
{
	struct emw3080_data *data = dev->data;

	k_mutex_unlock(&data->mutex);
}

static int emw3080_init(const struct device *dev)
{
	const struct emw3080_config *cfg = dev->config;
	struct emw3080_data *data = dev->data;
	int ret;

	if (!spi_is_ready_dt(&cfg->spi) || !gpio_is_ready_dt(&cfg->cs) ||
	    !gpio_is_ready_dt(&cfg->enable) || !gpio_is_ready_dt(&cfg->flow) ||
	    !gpio_is_ready_dt(&cfg->notify)) {
		return -ENODEV;
	}

	k_sem_init(&data->flow_sem, 0, 2);
	k_sem_init(&data->notify_sem, 0, 1);
	k_mutex_init(&data->mutex);

	/* CS must be asserted before waiting for FLOW. */
	data->spi_config = cfg->spi.config;
	data->spi_config.cs = (struct spi_cs_control){0};

	ret = gpio_pin_configure_dt(&cfg->cs, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->enable, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->flow, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->notify, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->flow_cb, flow_callback, BIT(cfg->flow.pin));
	gpio_init_callback(&data->notify_cb, notify_callback, BIT(cfg->notify.pin));

	ret = gpio_add_callback(cfg->flow.port, &data->flow_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_add_callback(cfg->notify.port, &data->notify_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->flow, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->notify, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	/* Enable the module after the GPIO interrupts are ready. */
	k_msleep(100);

	ret = gpio_pin_set_dt(&cfg->enable, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(1200);

	LOG_INF("EMW3080 initialized");

	return 0;
}

#define EMW3080_DEFINE(inst)                                                                       \
	static struct emw3080_data data_##inst;                                                    \
	static const struct emw3080_config config_##inst = {                                       \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)),                                \
		.cs = SPI_CS_GPIOS_DT_SPEC_INST_GET(inst),                                         \
		.enable = GPIO_DT_SPEC_INST_GET(inst, enable_gpios),                               \
		.flow = GPIO_DT_SPEC_INST_GET(inst, flow_gpios),                                   \
		.notify = GPIO_DT_SPEC_INST_GET(inst, notify_gpios),                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, emw3080_init, NULL, &data_##inst, &config_##inst, POST_KERNEL, \
			      CONFIG_EMW3080_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(EMW3080_DEFINE)
