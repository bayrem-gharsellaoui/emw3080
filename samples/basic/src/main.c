/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/wifi/emw3080.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static const struct device *wifi = DEVICE_DT_GET(DT_NODELABEL(emw3080));

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

	LOG_INF("Scanning for Wi-Fi networks...");

	uint8_t ap_count = 0;

	ret = emw3080_scan(wifi, &ap_count);

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
