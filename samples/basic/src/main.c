/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/wifi/emw3080.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static const struct device *wifi = DEVICE_DT_GET(DT_NODELABEL(emw3080));

static void scan_callback(const struct emw3080_ap_info *ap, void *user_data)
{
	LOG_INF("SSID: %-32s | RSSI: %ld dBm | Channel: %ld | "
		"BSSID: %02x:%02x:%02x:%02x:%02x:%02x | Security: %u",
		ap->ssid, (long)ap->rssi, (long)ap->channel, ap->bssid[0], ap->bssid[1],
		ap->bssid[2], ap->bssid[3], ap->bssid[4], ap->bssid[5], ap->security);
}

int main(void)
{
	LOG_INF("Started main...");

	char version[32] = {0};

	int ret = emw3080_get_firmware_version(wifi, version, sizeof(version));

	if (ret == 0) {
		LOG_INF("Firmware version: %s", version);
	} else {
		LOG_ERR("Failed to read firmware version: %d", ret);
	}

	uint8_t mac[6] = {0};

	ret = emw3080_get_mac_address(wifi, mac);

	if (ret == 0) {
		LOG_INF("MAC address: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2],
			mac[3], mac[4], mac[5]);
	} else {
		LOG_ERR("Failed to read MAC address: %d", ret);
	}

	uint8_t ap_count = 0;

	LOG_INF("Scanning for Wi-Fi networks...");

	ret = emw3080_scan(wifi, &ap_count, scan_callback, NULL);

	if (ret < 0) {
		LOG_ERR("Wi-Fi scan failed: %d", ret);
	} else {
		LOG_INF("Found %u access points", ap_count);
	}

	while (true) {
		k_msleep(1000);
	}

	return 0;
}
