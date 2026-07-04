/*
 * Robstride RS00 PV verification app for DM_MC02 CAN3.
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

LOG_MODULE_REGISTER(rs_pv_demo, LOG_LEVEL_INF);

#define RS00_NODE    DT_NODELABEL(rs00_motor)
#define CAN_BUS_NODE DT_NODELABEL(can3)

static const struct device *const motor = DEVICE_DT_GET(RS00_NODE);
static const struct device *const can_bus = DEVICE_DT_GET(CAN_BUS_NODE);

#define RS00_TEST_ANGLE_LIMIT_DEG 20.0f
#define RS00_TEST_SPEED_LIMIT_RPM 20.0f

static const float test_angle_deg[] = {
	0.0f, 20.0f, 0.0f, -20.0f, 0.0f,
};

static int32_t scale_100(float value)
{
	return (int32_t)(value * 100.0f);
}

static int32_t scale_1000(float value)
{
	return (int32_t)(value * 1000.0f);
}

static float clamp_float(float value, float min, float max)
{
	if (value < min) {
		return min;
	}
	if (value > max) {
		return max;
	}
	return value;
}

static void log_status(const char *tag)
{
	motor_status_t status = {0};
	int ret = motor_get(motor, &status);

	if (ret != 0) {
		LOG_ERR("%s motor_get failed: %d", tag, ret);
		return;
	}

	LOG_INF("%s online=%d enabled=%d mode=%d target=%d ctrl=%u err=%d angle_x100=%d "
		"rpm_x100=%d torque_x1000=%d temp_x100=%d",
		tag, status.online, status.enabled, status.mode, status.target,
		status.controller_id, status.error, scale_100(status.angle), scale_100(status.rpm),
		scale_1000(status.torque), scale_100(status.temperature));
}

static int set_rs00_pv_angle(float angle_deg, float speed_rpm)
{
	float angle = clamp_float(angle_deg, -RS00_TEST_ANGLE_LIMIT_DEG, RS00_TEST_ANGLE_LIMIT_DEG);
	float speed = clamp_float(speed_rpm, 0.0f, RS00_TEST_SPEED_LIMIT_RPM);
	motor_setpoint_t setpoint = {
		.angle = angle,
		.rpm = speed,
		.mode = PV,
		.target = MOTOR_TARGET_POSITION,
	};

	LOG_INF("set PV angle_x100=%d speed_x100=%d", scale_100(angle), scale_100(speed));
	return motor_set(motor, &setpoint);
}

int main(void)
{
	int ret;

	LOG_INF("RS00 PV verify start: board=dm_mc02 can=CAN3 motor_id=0x01 master=0xFD");

	if (!device_is_ready(can_bus)) {
		LOG_ERR("CAN3 device is not ready");
		return -ENODEV;
	}

	if (!device_is_ready(motor)) {
		LOG_ERR("RS00 motor device is not ready");
		return -ENODEV;
	}

	ret = can_start(can_bus);
	LOG_INF("can_start(can3) ret=%d", ret);
	if (ret < 0 && ret != -EALREADY) {
		return ret;
	}

	for (int i = 0; i < 10; i++) {
		log_status("listen");
		k_msleep(200);
	}

	motor_control(motor, CLEAR_ERROR);
	k_msleep(100);

	ret = set_rs00_pv_angle(0.0f, RS00_TEST_SPEED_LIMIT_RPM);
	LOG_INF("prime PV mode ret=%d", ret);
	if (ret < 0) {
		return ret;
	}

	motor_control(motor, ENABLE_MOTOR);
	LOG_INF("motor enable requested");
	k_msleep(500);

	motor_control(motor, SET_ZERO);
	LOG_INF("motor zero requested");
	k_msleep(200);

	ret = set_rs00_pv_angle(0.0f, RS00_TEST_SPEED_LIMIT_RPM);
	LOG_INF("hold zero ret=%d", ret);
	if (ret < 0) {
		return ret;
	}

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(test_angle_deg); i++) {
			float angle = test_angle_deg[i];

			ret = set_rs00_pv_angle(angle, RS00_TEST_SPEED_LIMIT_RPM);
			if (ret != 0) {
				LOG_ERR("set_rs00_pv_angle failed: %d", ret);
			}

			for (int j = 0; j < 15; j++) {
				log_status("pv");
				k_msleep(200);
			}
		}
	}
}
