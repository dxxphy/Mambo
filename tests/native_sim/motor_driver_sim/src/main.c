/*
 * Copyright (c) 2026 ttwards <12411711@mail.sustech.edu.cn>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/motor.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define DM_NODE  DT_NODELABEL(dm0)
#define MI_NODE  DT_NODELABEL(mi0)
#define RS_NODE  DT_NODELABEL(rs0)
#define LK_NODE  DT_NODELABEL(lk0)
#define DJI_NODE DT_NODELABEL(dji0)

#define DM_DEV  DEVICE_DT_GET(DM_NODE)
#define MI_DEV  DEVICE_DT_GET(MI_NODE)
#define RS_DEV  DEVICE_DT_GET(RS_NODE)
#define LK_DEV  DEVICE_DT_GET(LK_NODE)
#define DJI_DEV DEVICE_DT_GET(DJI_NODE)

#define DM_TX_ID  DT_PROP(DM_NODE, tx_id)
#define DM_RX_ID  DT_PROP(DM_NODE, rx_id)
#define MI_ID     DT_PROP(MI_NODE, id)
#define RS_TX_ID  DT_PROP(RS_NODE, tx_id)
#define LK_ID     DT_PROP(LK_NODE, id)
#define DJI_RX_ID DT_PROP(DJI_NODE, rx_id)

#define MI_MODE_FEEDBACK 0x02U
#define RS_MODE_FEEDBACK 0x02U

#define SIM_FILTER_MAX 16
#define SIM_TX_MAX     128

struct sim_filter {
	bool used;
	can_rx_callback_t cb;
	void *user_data;
	struct can_filter filter;
};

static struct sim_filter sim_filters[SIM_FILTER_MAX];
static struct can_frame sim_tx_history[SIM_TX_MAX];
static uint32_t sim_tx_count;
static struct k_spinlock sim_lock;

struct test_can_config {
	struct can_driver_config common;
};

struct test_can_data {
	struct can_driver_data common;
	bool started;
};

static bool frame_matches_filter(const struct can_frame *frame, const struct can_filter *filter)
{
	bool frame_ide = (frame->flags & CAN_FRAME_IDE) != 0U;
	bool filter_ide = (filter->flags & CAN_FILTER_IDE) != 0U;

	if (frame_ide != filter_ide) {
		return false;
	}

	return (frame->id & filter->mask) == (filter->id & filter->mask);
}

static int sim_can_add_rx_filter(const struct device *dev, can_rx_callback_t cb, void *user_data,
				 const struct can_filter *filter)
{
	ARG_UNUSED(dev);

	k_spinlock_key_t key = k_spin_lock(&sim_lock);

	for (int i = 0; i < ARRAY_SIZE(sim_filters); i++) {
		if (!sim_filters[i].used) {
			sim_filters[i] = (struct sim_filter){
				.used = true,
				.cb = cb,
				.user_data = user_data,
				.filter = *filter,
			};
			k_spin_unlock(&sim_lock, key);
			return i;
		}
	}

	k_spin_unlock(&sim_lock, key);
	return -ENOMEM;
}

static int sim_can_send(const struct device *dev, const struct can_frame *frame,
			k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	ARG_UNUSED(timeout);

	k_spinlock_key_t key = k_spin_lock(&sim_lock);
	sim_tx_history[sim_tx_count % ARRAY_SIZE(sim_tx_history)] = *frame;
	sim_tx_count++;
	k_spin_unlock(&sim_lock, key);

	if (callback != NULL) {
		callback(dev, 0, user_data);
	}

	return 0;
}

static int test_can_start(const struct device *dev)
{
	struct test_can_data *data = dev->data;

	data->started = true;
	return 0;
}

static int test_can_stop(const struct device *dev)
{
	struct test_can_data *data = dev->data;

	data->started = false;
	return 0;
}

static int test_can_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL;
	return 0;
}

static int test_can_set_mode(const struct device *dev, can_mode_t mode)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mode);

	return 0;
}

static int test_can_set_timing(const struct device *dev, const struct can_timing *timing)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(timing);

	return 0;
}

static int test_can_get_state(const struct device *dev, enum can_state *state,
			      struct can_bus_err_cnt *err_cnt)
{
	struct test_can_data *data = dev->data;

	if (state != NULL) {
		*state = data->started ? CAN_STATE_ERROR_ACTIVE : CAN_STATE_STOPPED;
	}
	if (err_cnt != NULL) {
		err_cnt->tx_err_cnt = 0U;
		err_cnt->rx_err_cnt = 0U;
	}
	return 0;
}

static int test_can_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);

	*rate = MHZ(80);
	return 0;
}

static int test_can_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return SIM_FILTER_MAX;
}

static int test_can_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static DEVICE_API(can, test_can_api) = {
	.start = test_can_start,
	.stop = test_can_stop,
	.get_capabilities = test_can_get_capabilities,
	.set_mode = test_can_set_mode,
	.set_timing = test_can_set_timing,
	.send = sim_can_send,
	.add_rx_filter = sim_can_add_rx_filter,
	.get_state = test_can_get_state,
	.get_core_clock = test_can_get_core_clock,
	.get_max_filters = test_can_get_max_filters,
};

#define DT_DRV_COMPAT ares_test_can

#define TEST_CAN_INIT(inst)                                                                        \
	static const struct test_can_config test_can_config_##inst = {                             \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 0, 1000000),                         \
	};                                                                                         \
	static struct test_can_data test_can_data_##inst;                                          \
	CAN_DEVICE_DT_INST_DEFINE(inst, test_can_init, NULL, &test_can_data_##inst,                \
				  &test_can_config_##inst, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,  \
				  &test_can_api);

DT_INST_FOREACH_STATUS_OKAY(TEST_CAN_INIT)

static void sim_reset_tx_history(void)
{
	k_spinlock_key_t key = k_spin_lock(&sim_lock);

	memset(sim_tx_history, 0, sizeof(sim_tx_history));
	sim_tx_count = 0;

	k_spin_unlock(&sim_lock, key);
}

static uint32_t sim_current_tx_count(void)
{
	k_spinlock_key_t key = k_spin_lock(&sim_lock);
	uint32_t count = sim_tx_count;

	k_spin_unlock(&sim_lock, key);
	return count;
}

static bool sim_tx_seen_since(uint32_t start, bool (*match)(const struct can_frame *frame))
{
	k_spinlock_key_t key = k_spin_lock(&sim_lock);
	uint32_t end = sim_tx_count;
	bool seen = false;

	for (uint32_t i = start; i < end; i++) {
		const struct can_frame *frame = &sim_tx_history[i % ARRAY_SIZE(sim_tx_history)];

		if (match(frame)) {
			seen = true;
			break;
		}
	}

	k_spin_unlock(&sim_lock, key);
	return seen;
}

static void wait_for_tx_after(uint32_t start, bool (*match)(const struct can_frame *frame),
			      const char *name)
{
	int64_t deadline = k_uptime_get() + 500;

	while (k_uptime_get() < deadline) {
		if (sim_tx_seen_since(start, match)) {
			return;
		}
		k_sleep(K_MSEC(1));
	}

	zassert_true(false, "%s did not transmit the expected CAN frame (tx count %u -> %u)", name,
		     start, sim_current_tx_count());
}

static void sim_emit_frame(const struct device *can_dev, const struct can_frame *frame)
{
	struct sim_filter filters[SIM_FILTER_MAX];

	k_spinlock_key_t key = k_spin_lock(&sim_lock);
	memcpy(filters, sim_filters, sizeof(filters));
	k_spin_unlock(&sim_lock, key);

	for (int i = 0; i < ARRAY_SIZE(filters); i++) {
		if (filters[i].used && frame_matches_filter(frame, &filters[i].filter)) {
			struct can_frame copy = *frame;

			filters[i].cb(can_dev, &copy, filters[i].user_data);
		}
	}
}

static bool motor_online(const struct device *dev)
{
	motor_status_t status = {0};
	const struct motor_driver_api *api = dev->api;
	int ret = api->motor_get(dev, &status);

	zassert_true(ret == 0 || ret == -ENODEV, "motor_get returned unexpected error %d", ret);
	return status.online;
}

static void driver_motor_control(const struct device *dev, enum motor_cmd cmd)
{
	const struct motor_driver_api *api = dev->api;

	api->motor_control(dev, cmd);
}

static void expect_online(const struct device *dev, bool expected, const char *name)
{
	bool online = motor_online(dev);

	zassert_equal(online, expected, "%s online state mismatch", name);
}

static void wait_for_online_state(const struct device *dev, bool expected, int timeout_ms,
				  const char *name)
{
	int64_t deadline = k_uptime_get() + timeout_ms;

	while (k_uptime_get() < deadline) {
		if (motor_online(dev) == expected) {
			return;
		}
		k_sleep(K_MSEC(1));
	}

	zassert_true(false, "%s did not reach expected online state %d", name, expected);
}

static bool match_dm_tx(const struct can_frame *frame)
{
	return ((frame->flags & CAN_FRAME_IDE) == 0U) && frame->id == DM_TX_ID;
}

static bool match_mi_tx(const struct can_frame *frame)
{
	return ((frame->flags & CAN_FRAME_IDE) != 0U) && ((frame->id & 0xFFU) == MI_ID);
}

static bool match_rs_tx(const struct can_frame *frame)
{
	return ((frame->flags & CAN_FRAME_IDE) != 0U) && ((frame->id & 0xFFU) == RS_TX_ID);
}

static bool match_lk_tx(const struct can_frame *frame)
{
	return ((frame->flags & CAN_FRAME_IDE) == 0U) && frame->id == (0x140U + LK_ID);
}

static void emit_dm_feedback(void)
{
	struct can_frame frame = {
		.id = DM_RX_ID & CAN_STD_ID_MASK,
		.dlc = 8,
		.data = {0x11, 0, 0, 0, 0, 0, 0, 0},
	};

	sim_emit_frame(DEVICE_DT_GET(DT_NODELABEL(fake_can)), &frame);
}

static void emit_mi_feedback(void)
{
	struct can_frame frame = {
		.id = (MI_MODE_FEEDBACK << 24) | (MI_ID << 8),
		.flags = CAN_FRAME_IDE,
		.dlc = 8,
	};

	sim_emit_frame(DEVICE_DT_GET(DT_NODELABEL(fake_can)), &frame);
}

static void emit_rs_feedback(void)
{
	struct can_frame frame = {
		.id = (RS_MODE_FEEDBACK << 24) | (RS_TX_ID << 8),
		.flags = CAN_FRAME_IDE,
		.dlc = 8,
	};

	sim_emit_frame(DEVICE_DT_GET(DT_NODELABEL(fake_can)), &frame);
}

static void emit_lk_feedback(void)
{
	struct can_frame frame = {
		.id = 0x140U + LK_ID,
		.dlc = 8,
		.data = {0x9C, 25, 0, 0, 0, 0, 0, 0},
	};

	sim_emit_frame(DEVICE_DT_GET(DT_NODELABEL(fake_can)), &frame);
}

static void emit_dji_report(void)
{
	struct can_frame frame = {
		.id = DJI_RX_ID,
		.dlc = 8,
		.data = {0x20, 0, 0, 0, 0, 0, 25, 0},
	};

	sim_emit_frame(DEVICE_DT_GET(DT_NODELABEL(fake_can)), &frame);
}

static void verify_request_reply_motor(const struct device *dev, const char *name,
				       bool (*match_tx)(const struct can_frame *frame),
				       void (*emit_feedback)(void), int offline_timeout_ms)
{
	uint32_t tx_start;

	zassert_true(device_is_ready(dev), "%s device is not ready", name);
	zassert_not_null(dev->api, "%s device has no driver API", name);
	expect_online(dev, false, name);

	tx_start = sim_current_tx_count();
	driver_motor_control(dev, ENABLE_MOTOR);
	wait_for_tx_after(tx_start, match_tx, name);
	expect_online(dev, false, name);

	emit_feedback();
	k_sleep(K_MSEC(10));
	expect_online(dev, true, name);

	wait_for_online_state(dev, false, offline_timeout_ms, name);

	tx_start = sim_current_tx_count();
	wait_for_tx_after(tx_start, match_tx, name);
	emit_feedback();
	k_sleep(K_MSEC(10));
	expect_online(dev, true, name);

	driver_motor_control(dev, DISABLE_MOTOR);
	k_sleep(K_MSEC(10));
	expect_online(dev, false, name);
}

static void *motor_driver_sim_setup(void)
{
	k_sleep(K_MSEC(1300));
	sim_reset_tx_history();
	return NULL;
}

ZTEST(motor_driver_sim, test_request_reply_motor_drivers)
{
	verify_request_reply_motor(DM_DEV, "DM", match_dm_tx, emit_dm_feedback, 500);
	verify_request_reply_motor(MI_DEV, "MI", match_mi_tx, emit_mi_feedback, 6000);
	verify_request_reply_motor(RS_DEV, "RS", match_rs_tx, emit_rs_feedback, 2000);
	verify_request_reply_motor(LK_DEV, "LK", match_lk_tx, emit_lk_feedback, 3000);
}

ZTEST(motor_driver_sim, test_dji_periodic_report_driver)
{
	expect_online(DJI_DEV, false, "DJI");

	emit_dji_report();
	k_sleep(K_MSEC(10));
	expect_online(DJI_DEV, true, "DJI");

	k_sleep(K_MSEC(30));
	expect_online(DJI_DEV, false, "DJI");

	emit_dji_report();
	k_sleep(K_MSEC(10));
	expect_online(DJI_DEV, true, "DJI");
}

ZTEST_SUITE(motor_driver_sim, NULL, motor_driver_sim_setup, NULL, NULL, NULL);
