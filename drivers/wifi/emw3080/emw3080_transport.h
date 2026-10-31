/*
 * Copyright (c) 2026 Bayrem Gharsellaoui
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EMW3080_TRANSPORT_H_
#define EMW3080_TRANSPORT_H_

#include <zephyr/device.h>

int emw3080_transport_send(const struct device *dev, const uint8_t *buf, size_t len);
int emw3080_transport_receive(const struct device *dev, uint8_t *buf, size_t size, size_t *len,
			      uint32_t timeout_ms);
int emw3080_transport_lock(const struct device *dev);
void emw3080_transport_unlock(const struct device *dev);

#endif /* EMW3080_TRANSPORT_H_ */
