# Copyright (c) 2026 Bayrem Gharsellaoui
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(../../include/zephyr/drivers/wifi/emw3080)

zephyr_library_sources(
  emw3080.c
  emw3080_spi.c
)
