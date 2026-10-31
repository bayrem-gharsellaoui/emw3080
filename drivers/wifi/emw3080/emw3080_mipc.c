/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(emw3080, CONFIG_EMW3080_LOG_LEVEL);

#include "emw3080_mipc.h"
#include "emw3080_transport.h"

#define MIPC_HEADER_SIZE     6
#define VERSION_RESPONSE_MAX 64

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

	ret = emw3080_transport_receive(dev, response, sizeof(response), &response_len);
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
