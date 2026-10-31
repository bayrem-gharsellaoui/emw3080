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
	if (dev == NULL || version == NULL || size == 0) {
		return -EINVAL;
	}

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	return emw3080_mipc_get_firmware_version(dev, version, size);
}
