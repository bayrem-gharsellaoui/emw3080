/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EMW3080_MIPC_H_
#define EMW3080_MIPC_H_

#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/drivers/wifi/emw3080.h>

struct emw3080_link_info {
	bool connected;
	char ssid[33];
	uint8_t bssid[6];
	int32_t rssi;
	int32_t channel;
	uint8_t security;
};

enum emw3080_security {
	EMW3080_SECURITY_NONE = 0,
	EMW3080_SECURITY_WEP = 1,
	EMW3080_SECURITY_WPA_TKIP = 2,
	EMW3080_SECURITY_WPA_AES = 3,
	EMW3080_SECURITY_WPA2_TKIP = 4,
	EMW3080_SECURITY_WPA2_AES = 5,
	EMW3080_SECURITY_WPA2_MIXED = 6,
	EMW3080_SECURITY_AUTO = 7,
};

struct emw3080_ap_info {
	char ssid[34];
	uint8_t bssid[6];
	int32_t rssi;
	int32_t channel;
	uint8_t security;
};

typedef void (*emw3080_scan_cb_t)(const struct emw3080_ap_info *ap, void *user_data);

int emw3080_mipc_get_firmware_version(const struct device *dev, char *version, size_t size);
int emw3080_mipc_get_mac_address(const struct device *dev, uint8_t mac[6]);
int emw3080_mipc_scan(const struct device *dev, uint8_t *ap_count, emw3080_scan_cb_t callback,
		      void *user_data);
int emw3080_mipc_connect(const struct device *dev, const char *ssid, const char *password);
int emw3080_mipc_get_link_info(const struct device *dev, struct emw3080_link_info *info);

#endif /* EMW3080_MIPC_H_ */
