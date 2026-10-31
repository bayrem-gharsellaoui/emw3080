
/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(emw3080, CONFIG_EMW3080_LOG_LEVEL);

#include "emw3080_mipc.h"
#include "emw3080_transport.h"

#define MIPC_HEADER_SIZE     6
#define VERSION_RESPONSE_MAX 64

#define MIPC_SCAN_CMD         0x0102
#define MIPC_SCAN_TIMEOUT_MS  5000
#define MIPC_SCAN_BUFFER_SIZE 2500
#define MIPC_SCAN_AP_SIZE     48

#define MIPC_GET_MAC_TIMEOUT_MS 2000
#define MIPC_MAC_SIZE           6

int emw3080_mipc_get_firmware_version(const struct device *dev, char *version, size_t size)
{
	/* Request ID = 1, command ID = 0x0003 */
	static const uint8_t request[] = {0x01, 0x00, 0x00, 0x00, 0x03, 0x00};

	uint8_t response[VERSION_RESPONSE_MAX];
	size_t response_len;
	size_t version_len;
	int ret;

	emw3080_transport_lock(dev);

	ret = emw3080_transport_send(dev, request, sizeof(request));
	if (ret < 0) {
		goto unlock;
	}

	ret = emw3080_transport_receive(dev, response, sizeof(response), &response_len, 2000);

unlock:
	emw3080_transport_unlock(dev);

	if (ret < 0) {
		return ret;
	}

	if (response_len <= MIPC_HEADER_SIZE || memcmp(response, request, MIPC_HEADER_SIZE) != 0) {
		return -EBADMSG;
	}

	version_len = response_len - MIPC_HEADER_SIZE;

	if (version_len >= size) {
		return -ENOSPC;
	}

	memcpy(version, response + MIPC_HEADER_SIZE, version_len);
	version[version_len] = '\0';

	return 0;
}

int emw3080_mipc_scan(const struct device *dev, uint8_t *ap_count, emw3080_scan_cb_t callback,
		      void *user_data)
{
	/* Request ID = 1, scan command = 0x0102, empty SSID. */
	static const uint8_t request[MIPC_HEADER_SIZE + 33] = {0x01, 0x00, 0x00, 0x00, 0x02, 0x01};

	static uint8_t response[MIPC_SCAN_BUFFER_SIZE];
	size_t response_len;
	int ret;

	emw3080_transport_lock(dev);

	ret = emw3080_transport_send(dev, request, sizeof(request));
	if (ret < 0) {
		goto unlock;
	}

	ret = emw3080_transport_receive(dev, response, sizeof(response), &response_len,
					MIPC_SCAN_TIMEOUT_MS);

unlock:
	emw3080_transport_unlock(dev);

	if (ret < 0) {
		return ret;
	}

	if (response_len < MIPC_HEADER_SIZE + 1 ||
	    memcmp(response, request, MIPC_HEADER_SIZE) != 0) {
		return -EBADMSG;
	}

	uint8_t count = response[MIPC_HEADER_SIZE];
	size_t records_len = response_len - MIPC_HEADER_SIZE - 1;

	if (records_len != (size_t)count * MIPC_SCAN_AP_SIZE) {
		return -EBADMSG;
	}

	const uint8_t *records = response + MIPC_HEADER_SIZE + 1;

	for (uint8_t i = 0; i < count; i++) {
		const uint8_t *record = records + i * MIPC_SCAN_AP_SIZE;
		struct emw3080_ap_info ap = {0};

		ap.rssi = (int32_t)sys_get_le32(record);
		memcpy(ap.ssid, record + 4, 32);
		ap.ssid[32] = '\0';
		memcpy(ap.bssid, record + 37, sizeof(ap.bssid));
		ap.channel = (int32_t)sys_get_le32(record + 43);
		ap.security = record[47];

		callback(&ap, user_data);
	}

	*ap_count = count;

	return 0;
}

int emw3080_mipc_get_mac_address(const struct device *dev, uint8_t mac[6])
{
	/* Request ID = 1, command ID = 0x0101. */
	static const uint8_t request[MIPC_HEADER_SIZE] = {0x01, 0x00, 0x00, 0x00, 0x01, 0x01};

	uint8_t response[MIPC_HEADER_SIZE + MIPC_MAC_SIZE] = {0};
	size_t response_len;
	int ret;

	emw3080_transport_lock(dev);

	ret = emw3080_transport_send(dev, request, sizeof(request));
	if (ret < 0) {
		goto unlock;
	}

	ret = emw3080_transport_receive(dev, response, sizeof(response), &response_len,
					MIPC_GET_MAC_TIMEOUT_MS);

unlock:
	emw3080_transport_unlock(dev);

	if (ret < 0) {
		return ret;
	}

	/* Verify response size, request ID and command ID. */
	if (response_len != sizeof(response) || memcmp(response, request, MIPC_HEADER_SIZE) != 0) {
		return -EBADMSG;
	}

	memcpy(mac, response + MIPC_HEADER_SIZE, MIPC_MAC_SIZE);

	return 0;
}
