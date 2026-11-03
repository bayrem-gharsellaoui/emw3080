/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EMW3080_WIFI_H_
#define EMW3080_WIFI_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/atomic.h>

#define EMW3080_SCAN_STACK_SIZE 2048

struct emw3080_wifi_context {
	struct k_work_q work_queue;
	struct k_work scan_work;
	K_KERNEL_STACK_MEMBER(work_stack, EMW3080_SCAN_STACK_SIZE);

	const struct device *dev;
	struct net_if *scan_iface;
	scan_result_cb_t scan_cb;
	atomic_t scan_busy;

	uint8_t mac[WIFI_MAC_ADDR_LEN];

	struct k_work connect_work;
	atomic_t connect_busy;
	struct net_if *connect_iface;

	char ssid[33];
	char password[65];
};

struct emw3080_wifi_context *emw3080_wifi_context_get(const struct device *dev);

int emw3080_wifi_init(const struct device *dev);

extern const struct net_wifi_mgmt_offload emw3080_wifi_api;

#endif /* EMW3080_WIFI_H_ */
