#!/bin/bash

# Copyright (c) 2026 Bayrem Gharsellaoui
# SPDX-License-Identifier: Apache-2.0

set -e

west init -l emw3080
west update
west zephyr-export

sudo /opt/python/venv/bin/pip install -r /workdir/zephyr/scripts/requirements.txt

west sdk install --gnu-toolchains arm-zephyr-eabi
