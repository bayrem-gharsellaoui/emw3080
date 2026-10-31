# Copyright (c) 2026 Bayrem Gharsellaoui
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    emw3080.c
    emw3080_mipc.c
    emw3080_spi.c
    emw3080_wifi.c
)
