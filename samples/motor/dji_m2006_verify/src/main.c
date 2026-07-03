/*
 * DJI M2006 verification app for DM_MC02 FDCAN1.
 *
 * Copyright (c) 2026 ttwards
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/motor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dji_m2006_verify, LOG_LEVEL_INF);

#define M2006_NODE DT_NODELABEL(m2006_motor)
#define CAN1_NODE  DT_NODELABEL(can1)

static const struct device *const motor = DEVICE_DT_GET(M2006_NODE);
static const struct device *const can1 = DEVICE_DT_GET(CAN1_NODE);

static const float test_rpm[] = {
	0.0f, 30.0f, 60.0f, -30.0f, 0.0f,
};

static const float test_torque[] = {
	0.0f,
	0.15f,
	-0.15f,
	0.0f,
};

static const float test_angle[] = {
	0.0f, 90.0f, 0.0f, -90.0f, 0.0f,
};

static int32_t scale_100(float value)
{
	return (int32_t)(value * 100.0f);
}

static int32_t scale_1000(float value)
{
	return (int32_t)(value * 1000.0f);
}

static void log_status(const char *tag)
{
	motor_status_t status = {0};
	int ret = motor_get(motor, &status);

	if (ret != 0) {
		LOG_ERR("%s motor_get failed: %d", tag, ret);
		return;
	}

	LOG_INF("%s online=%d enabled=%d mode=%d target=%d err=%d rpm_x100=%d angle_x100=%d "
		"sum_x100=%d torque_x1000=%d temp_x100=%d",
		tag, status.online, status.enabled, status.mode, status.target, status.error,
		scale_100(status.rpm), scale_100(status.angle), scale_100(status.sum_angle),
		scale_1000(status.torque), scale_100(status.temperature));
}

int main(void)
{
	int ret;

	LOG_INF("DJI M2006 verify start: board=dm_mc02 can=FDCAN1 tx=0x200 rx=0x201");

	if (!device_is_ready(can1)) {
		LOG_ERR("CAN1 device is not ready");
		return -ENODEV;
	}

	if (!device_is_ready(motor)) {
		LOG_ERR("M2006 motor device is not ready");
		return -ENODEV;
	}

	ret = can_start(can1);
	LOG_INF("can_start(can1) ret=%d", ret);

	for (int i = 0; i < 10; i++) {
		log_status("listen");
		k_msleep(200);
	}

	motor_control(motor, ENABLE_MOTOR);
	LOG_INF("motor enable requested");
	motor_control(motor, SET_ZERO);
	LOG_INF("motor zero requested");
	k_msleep(500);

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(test_angle); i++) {
			float angle = test_angle[i];

			LOG_INF("set angle angle_x100=%d", scale_100(angle));
			ret = motor_set_angle(motor, angle);
			if (ret != 0) {
				LOG_ERR("motor_set_angle failed: %d", ret);
			}

			for (int j = 0; j < 20; j++) {
				log_status("angle");
				k_msleep(200);
			}
		}

		ret = motor_set_torque(motor, 0.0f);
		LOG_INF("angle cycle done, set torque 0 ret=%d", ret);
		for (int i = 0; i < 10; i++) {
			log_status("angle-stop");
			k_msleep(200);
		}

		for (size_t i = 0; i < ARRAY_SIZE(test_torque); i++) {
			float torque = test_torque[i];

			LOG_INF("set torque torque_x1000=%d", scale_1000(torque));
			ret = motor_set_torque(motor, torque);
			if (ret != 0) {
				LOG_ERR("motor_set_torque failed: %d", ret);
			}

			for (int j = 0; j < 10; j++) {
				log_status("torque");
				k_msleep(200);
			}
		}

		for (size_t i = 0; i < ARRAY_SIZE(test_rpm); i++) {
			float rpm = test_rpm[i];

			LOG_INF("set speed rpm_x100=%d", scale_100(rpm));
			ret = motor_set_speed(motor, rpm);
			if (ret != 0) {
				LOG_ERR("motor_set_speed failed: %d", ret);
			}

			for (int j = 0; j < 10; j++) {
				log_status("run");
				k_msleep(200);
			}
		}

		ret = motor_set_torque(motor, 0.0f);
		LOG_INF("set torque 0 ret=%d", ret);
		for (int i = 0; i < 10; i++) {
			log_status("stop");
			k_msleep(200);
		}
	}
}
