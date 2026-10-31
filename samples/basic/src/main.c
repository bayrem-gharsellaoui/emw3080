/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/wifi/emw3080.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
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

	uint8_t mac[6] = {0};

	ret = emw3080_get_mac_address(wifi, mac);

	if (ret == 0) {
		LOG_INF("MAC address: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2],
			mac[3], mac[4], mac[5]);
	} else {
		LOG_ERR("Failed to read MAC address: %d", ret);
	}

	struct net_if *iface = net_if_lookup_by_dev(wifi);

	if (iface == NULL) {
		LOG_ERR("EMW3080 network interface not found");
	} else {
		LOG_INF("EMW3080 network interface: %d", net_if_get_by_iface(iface));
	}

	if (iface != NULL) {
		LOG_INF("Is Wi-Fi interface: %d", net_if_is_wifi(iface));
		LOG_INF("Interface administratively up: %d", net_if_is_admin_up(iface));
	}

	while (true) {
		k_msleep(1000);
	}

	return 0;
}
