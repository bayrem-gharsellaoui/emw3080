/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_EMW3080_H_
#define ZEPHYR_DRIVERS_WIFI_EMW3080_H_

#include <zephyr/device.h>

int emw3080_get_firmware_version(const struct device *dev, char *version, size_t size);

struct emw3080_ap_info {
	char ssid[34];
	uint8_t bssid[6];
	int32_t rssi;
	int32_t channel;
	uint8_t security;
};

typedef void (*emw3080_scan_cb_t)(const struct emw3080_ap_info *ap, void *user_data);

int emw3080_scan(const struct device *dev, uint8_t *ap_count, emw3080_scan_cb_t callback,
		 void *user_data);

int emw3080_get_mac_address(const struct device *dev, uint8_t mac[6]);

#endif /* ZEPHYR_DRIVERS_WIFI_EMW3080_H_ */
