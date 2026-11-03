/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/drivers/wifi/emw3080.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(emw3080, CONFIG_EMW3080_LOG_LEVEL);

#include "emw3080_wifi.h"
#include "emw3080_mipc.h"

/* Temporary: network offload is not implemented yet. */
static struct net_offload emw3080_offload;

/* Translate the module security type to Zephyr. */

static enum wifi_security_type emw3080_security_to_zephyr(uint8_t security)
{
	switch (security) {
	case EMW3080_SECURITY_NONE:
		return WIFI_SECURITY_TYPE_NONE;

	case EMW3080_SECURITY_WEP:
		return WIFI_SECURITY_TYPE_WEP;

	case EMW3080_SECURITY_WPA_TKIP:
	case EMW3080_SECURITY_WPA_AES:
		return WIFI_SECURITY_TYPE_WPA_PSK;

	case EMW3080_SECURITY_WPA2_TKIP:
	case EMW3080_SECURITY_WPA2_AES:
	case EMW3080_SECURITY_WPA2_MIXED:
		return WIFI_SECURITY_TYPE_PSK;

	default:
		return WIFI_SECURITY_TYPE_UNKNOWN;
	}
}

/* Convert a module AP record into a Zephyr scan result. */
static void emw3080_scan_result(const struct emw3080_ap_info *ap, void *user_data)
{
	struct emw3080_wifi_context *ctx = user_data;
	struct wifi_scan_result result = {0};

	size_t ssid_len = strnlen(ap->ssid, WIFI_SSID_MAX_LEN);

	memcpy(result.ssid, ap->ssid, ssid_len);
	result.ssid_length = ssid_len;

	memcpy(result.mac, ap->bssid, WIFI_MAC_ADDR_LEN);
	result.mac_length = WIFI_MAC_ADDR_LEN;

	result.band = WIFI_FREQ_BAND_2_4_GHZ;
	result.channel = (uint8_t)ap->channel;
	result.rssi = (int8_t)CLAMP(ap->rssi, INT8_MIN, INT8_MAX);
	result.security = emw3080_security_to_zephyr(ap->security);

	ctx->scan_cb(ctx->scan_iface, 0, &result);
}

/* Execute the blocking MIPC scan outside the management caller. */
static void emw3080_scan_work_handler(struct k_work *work)
{
	struct emw3080_wifi_context *ctx =
		CONTAINER_OF(work, struct emw3080_wifi_context, scan_work);

	uint8_t ap_count = 0;
	int ret;

	ret = emw3080_mipc_scan(ctx->dev, &ap_count, emw3080_scan_result, ctx);

	ctx->scan_cb(ctx->scan_iface, ret, NULL);

	atomic_clear(&ctx->scan_busy);
}

/* Start an asynchronous scan requested by Zephyr. */
static int emw3080_wifi_scan(const struct device *dev, struct net_if *iface,
			     struct wifi_scan_params *params, scan_result_cb_t callback)
{
	LOG_INF("EMW3080 Wi-Fi scan callback invoked");

	struct emw3080_wifi_context *ctx = emw3080_wifi_context_get(dev);

	ARG_UNUSED(params);

	if (iface == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (!atomic_cas(&ctx->scan_busy, 0, 1)) {
		return -EBUSY;
	}

	ctx->scan_iface = iface;
	ctx->scan_cb = callback;

	int ret = k_work_submit_to_queue(&ctx->work_queue, &ctx->scan_work);

	if (ret <= 0) {
		atomic_clear(&ctx->scan_busy);
		return ret < 0 ? ret : -EBUSY;
	}

	return 0;
}

static void emw3080_connect_work_handler(struct k_work *work)
{
	struct emw3080_wifi_context *ctx =
		CONTAINER_OF(work, struct emw3080_wifi_context, connect_work);

	int ret = emw3080_mipc_connect(ctx->dev, ctx->ssid, ctx->password);

	memset(ctx->password, 0, sizeof(ctx->password));

	LOG_INF("Wi-Fi connection result: %d", ret);

	wifi_mgmt_raise_connect_result_event(ctx->connect_iface, ret);

	atomic_clear(&ctx->connect_busy);
}

static int emw3080_wifi_connect(const struct device *dev, struct net_if *iface,
				struct wifi_connect_req_params *params)
{
	struct emw3080_wifi_context *ctx = emw3080_wifi_context_get(dev);

	if (params == NULL || iface == NULL || params->ssid == NULL || params->ssid_length == 0 ||
	    params->ssid_length > 32) {
		return -EINVAL;
	}

	/* Initially support only open and WPA2-PSK networks. */
	if (params->security != WIFI_SECURITY_TYPE_NONE &&
	    params->security != WIFI_SECURITY_TYPE_PSK) {
		return -ENOTSUP;
	}

	if (params->security == WIFI_SECURITY_TYPE_PSK &&
	    (params->psk == NULL || params->psk_length < 8 || params->psk_length > 63)) {
		return -EINVAL;
	}

	if (!atomic_cas(&ctx->connect_busy, 0, 1)) {
		return -EBUSY;
	}

	memcpy(ctx->ssid, params->ssid, params->ssid_length);
	ctx->ssid[params->ssid_length] = '\0';

	size_t password_len = params->security == WIFI_SECURITY_TYPE_NONE ? 0 : params->psk_length;

	if (password_len > 0) {
		memcpy(ctx->password, params->psk, password_len);
	}
	ctx->password[password_len] = '\0';

	ctx->connect_iface = iface;

	int ret = k_work_submit_to_queue(&ctx->work_queue, &ctx->connect_work);

	if (ret <= 0) {
		memset(ctx->password, 0, sizeof(ctx->password));
		atomic_clear(&ctx->connect_busy);
		return ret < 0 ? ret : -EBUSY;
	}

	return 0;
}

static int emw3080_wifi_iface_status(const struct device *dev, struct net_if *iface,
				     struct wifi_iface_status *status)
{
	struct emw3080_link_info info;
	int ret;

	ARG_UNUSED(iface);

	if (status == NULL) {
		return -EINVAL;
	}

	memset(status, 0, sizeof(*status));

	ret = emw3080_mipc_get_link_info(dev, &info);
	if (ret < 0) {
		return ret;
	}

	status->iface_mode = WIFI_MODE_INFRA;

	if (!info.connected) {
		status->state = WIFI_STATE_DISCONNECTED;
		return 0;
	}

	status->state = WIFI_STATE_COMPLETED;
	status->ssid_len = strnlen(info.ssid, 32);

	memcpy(status->ssid, info.ssid, status->ssid_len);
	memcpy(status->bssid, info.bssid, sizeof(info.bssid));

	status->band = WIFI_FREQ_BAND_2_4_GHZ;
	status->channel = info.channel;
	status->rssi = info.rssi;
	status->security = emw3080_security_to_zephyr(info.security);
	status->link_mode = WIFI_LINK_MODE_UNKNOWN;

	return 0;
}

/* Configure the network interface with the real module MAC. */
static void emw3080_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct emw3080_wifi_context *ctx = emw3080_wifi_context_get(dev);

	int ret = emw3080_get_mac_address(dev, ctx->mac);

	if (ret < 0) {
		LOG_ERR("Failed to read MAC address: %d", ret);
		return;
	}

	net_if_set_link_addr(iface, ctx->mac, sizeof(ctx->mac), NET_LINK_ETHERNET);

	net_if_offload_set(iface, &emw3080_offload);

	LOG_INF("EMW3080 network interface initialized");
}

static enum offloaded_net_if_types emw3080_get_type(void)
{
	return L2_OFFLOADED_NET_IF_TYPE_WIFI;
}

static const struct wifi_mgmt_ops emw3080_mgmt_ops = {
	.scan = emw3080_wifi_scan,
	.connect = emw3080_wifi_connect,
	.iface_status = emw3080_wifi_iface_status,
};

const struct net_wifi_mgmt_offload emw3080_wifi_api = {
	.wifi_iface.iface_api.init = emw3080_iface_init,
	.wifi_iface.get_type = emw3080_get_type,
	.wifi_mgmt_api = &emw3080_mgmt_ops,
};

int emw3080_wifi_init(const struct device *dev)
{
	struct emw3080_wifi_context *ctx = emw3080_wifi_context_get(dev);

	ctx->dev = dev;

	atomic_clear(&ctx->scan_busy);
	k_work_init(&ctx->scan_work, emw3080_scan_work_handler);

	atomic_clear(&ctx->connect_busy);
	k_work_init(&ctx->connect_work, emw3080_connect_work_handler);

	k_work_queue_start(&ctx->work_queue, ctx->work_stack,
			   K_KERNEL_STACK_SIZEOF(ctx->work_stack), 5, NULL);

	return 0;
}
