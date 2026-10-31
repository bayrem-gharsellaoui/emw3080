/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/wifi/emw3080.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emw3080, CONFIG_EMW3080_LOG_LEVEL);

#include "emw3080_mipc.h"
#include "emw3080_transport.h"

int emw3080_get_firmware_version(const struct device *dev, char *version, size_t size)
{
	int ret;

	if (dev == NULL || version == NULL || size == 0) {
		return -EINVAL;
	}

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	ret = emw3080_transport_lock(dev);
	if (ret < 0) {
		return ret;
	}

	ret = emw3080_mipc_get_firmware_version(dev, version, size);

	emw3080_transport_unlock(dev);

	return ret;
}

int emw3080_scan(const struct device *dev, uint8_t *ap_count, emw3080_scan_cb_t callback,
		 void *user_data)
{
	int ret;

	if (dev == NULL || ap_count == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	ret = emw3080_transport_lock(dev);
	if (ret < 0) {
		return ret;
	}

	ret = emw3080_mipc_scan(dev, ap_count, callback, user_data);

	emw3080_transport_unlock(dev);

	return ret;
}

int emw3080_get_mac_address(const struct device *dev, uint8_t mac[6])
{
	int ret;

	if (dev == NULL || mac == NULL) {
		return -EINVAL;
	}

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	ret = emw3080_transport_lock(dev);
	if (ret < 0) {
		return ret;
	}

	ret = emw3080_mipc_get_mac_address(dev, mac);

	emw3080_transport_unlock(dev);

	return ret;
}
