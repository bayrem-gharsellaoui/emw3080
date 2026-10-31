/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EMW3080_MIPC_H_
#define EMW3080_MIPC_H_

#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/drivers/wifi/emw3080.h>

int emw3080_mipc_get_firmware_version(const struct device *dev, char *version, size_t size);
int emw3080_mipc_scan(const struct device *dev, uint8_t *ap_count, emw3080_scan_cb_t callback,
		      void *user_data);
int emw3080_mipc_get_mac_address(const struct device *dev, uint8_t mac[6]);

#endif /* EMW3080_MIPC_H_ */
