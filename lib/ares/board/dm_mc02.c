/*
 * Copyright (c) 2025 ttwards <12411711@mail.sustech.edu.cn>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "init.h"

LOG_MODULE_DECLARE(board_init, CONFIG_BOARD_LOG_LEVEL);

#define XT30_1_NODE DT_NODELABEL(power1)
#define XT30_2_NODE DT_NODELABEL(power2)
#define STRIP_NODE  DT_ALIAS(led_strip)

static const struct device *const pwr1 = DEVICE_DT_GET(XT30_1_NODE);
static const struct device *const pwr2 = DEVICE_DT_GET(XT30_2_NODE);
static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

static int xt30_regulator_set(const struct device *dev, bool enable)
{
	if (!device_is_ready(dev)) {
		LOG_ERR("%s is not ready", dev->name);
		return -ENODEV;
	}

	if (regulator_is_enabled(dev) == enable) {
		return 0;
	}

	return enable ? regulator_enable(dev) : regulator_disable(dev);
}

int ares_board_xt30_power_set(enum ares_board_xt30 xt30, bool enable)
{
	int ret = 0;
	int err;

	if ((xt30 & ARES_BOARD_XT30_ALL) == 0) {
		return -EINVAL;
	}

	if ((xt30 & ARES_BOARD_XT30_1) != 0) {
		err = xt30_regulator_set(pwr1, enable);
		if (err < 0) {
			ret = err;
		}
	}

	if ((xt30 & ARES_BOARD_XT30_2) != 0) {
		err = xt30_regulator_set(pwr2, enable);
		if (err < 0 && ret == 0) {
			ret = err;
		}
	}

	return ret;
}

void ares_board_power_init(void)
{
	LOG_INF("XT30 power left off for manual control.");
}

int ares_board_status_led_set_rgb(const struct ares_led_rgb *color)
{
	struct led_rgb led_color = {
		.r = color->r,
		.g = color->g,
		.b = color->b,
	};

	return led_strip_update_rgb(strip, &led_color, 1);
}

uint8_t ares_board_status_led_max_channel(void)
{
	return 0x7e;
}

void ares_board_status_led_init(void)
{
	struct ares_led_rgb color = {
		.r = 0x4f,
		.g = 0x4f,
		.b = 0x4f,
	};

	if (!device_is_ready(strip)) {
		LOG_ERR("WS2812 LED strip device is not ready");
		return;
	}

	ares_board_status_led_set_rgb(&color);
	k_sleep(K_MSEC(300));
	ares_board_status_led_service_start();
}
