/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_EMW3080_H_
#define ZEPHYR_DRIVERS_WIFI_EMW3080_H_

#include <stddef.h>
#include <zephyr/device.h>

int emw3080_get_firmware_version(const struct device *dev, char *version, size_t size);

#endif /* ZEPHYR_DRIVERS_WIFI_EMW3080_H_ */
