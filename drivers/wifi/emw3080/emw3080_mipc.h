/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EMW3080_MIPC_H_
#define EMW3080_MIPC_H_

#include <stddef.h>
#include <zephyr/device.h>

int emw3080_mipc_get_firmware_version(const struct device *dev, char *version, size_t size);

#endif /* EMW3080_MIPC_H_ */
