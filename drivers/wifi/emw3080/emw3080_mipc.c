/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
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

int emw3080_mipc_get_firmware_version(const struct device *dev, char *version, size_t size)
{
	/* Request ID = 1, command ID = 0x0003 */
	static const uint8_t request[] = {0x01, 0x00, 0x00, 0x00, 0x03, 0x00};

	uint8_t response[VERSION_RESPONSE_MAX];
	size_t response_len;
	size_t version_len;
	int ret;

	ret = emw3080_transport_send(dev, request, sizeof(request));
	if (ret < 0) {
		return ret;
	}

	ret = emw3080_transport_receive(dev, response, sizeof(response), &response_len, 2000);
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

int emw3080_mipc_scan(const struct device *dev, uint8_t *ap_count)
{
	/* Request ID = 1, command ID = 0x0102, empty SSID. */
	const uint8_t request[MIPC_HEADER_SIZE + 33] = {0x01, 0x00, 0x00, 0x00, 0x02, 0x01};

	/* Temporary shared buffer for our single-device bring-up. */
	static uint8_t response[MIPC_SCAN_BUFFER_SIZE] = {0};
	size_t response_len;
	int ret;

	ret = emw3080_transport_send(dev, request, sizeof(request));
	if (ret < 0) {
		return ret;
	}

	ret = emw3080_transport_receive(dev, response, sizeof(response), &response_len,
					MIPC_SCAN_TIMEOUT_MS);
	LOG_INF("Scan response length: %zu", response_len);
	LOG_HEXDUMP_INF(response, MIN(response_len, 64), "Scan response:");
	if (ret < 0) {
		return ret;
	}

	/* MIPC header + one byte for AP count. */
	if (response_len < MIPC_HEADER_SIZE + 1 ||
	    memcmp(response, request, MIPC_HEADER_SIZE) != 0) {
		return -EBADMSG;
	}

	uint8_t count = response[MIPC_HEADER_SIZE];
	size_t records_len = response_len - MIPC_HEADER_SIZE - 1;

	/* Confirm the response contains all reported AP records. */
	if (records_len != (size_t)count * MIPC_SCAN_AP_SIZE) {
		return -EBADMSG;
	}

	*ap_count = count;

	return 0;
}
