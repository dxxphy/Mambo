/*
 * Copyright (c) 2026 ttwards
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <drivers/transfer/ares_sbus.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sbus.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define DT_DRV_COMPAT ares_test_uart

#define TEST_UART_NODE DT_NODELABEL(test_uart)
#define SBUS_NODE      DT_NODELABEL(sbus0)

#define SBUS_FRAME_SIZE      25
#define SBUS_TEST_CHANNELS   16
#define SBUS_OFFLINE_WAIT_MS 520
#define SBUS_FLOAT_TOLERANCE 0.0001f

struct test_uart_data {
	uart_callback_t callback;
	void *user_data;
	struct uart_config config;
	bool rx_enabled;
};

static int test_uart_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int test_uart_callback_set(const struct device *dev, uart_callback_t callback,
				  void *user_data)
{
	struct test_uart_data *data = dev->data;

	data->callback = callback;
	data->user_data = user_data;

	return 0;
}

static int test_uart_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	struct test_uart_data *data = dev->data;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(timeout);

	data->rx_enabled = true;

	return 0;
}

static int test_uart_rx_disable(const struct device *dev)
{
	struct test_uart_data *data = dev->data;

	data->rx_enabled = false;

	return 0;
}

static int test_uart_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	return 0;
}

static int test_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct test_uart_data *data = dev->data;

	data->config = *cfg;

	return 0;
}

static int test_uart_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct test_uart_data *data = dev->data;

	*cfg = data->config;

	return 0;
}

static int test_uart_poll_in(const struct device *dev, unsigned char *p_char)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(p_char);

	return -ENOSYS;
}

static void test_uart_poll_out(const struct device *dev, unsigned char out_char)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(out_char);
}

static int test_uart_err_check(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static DEVICE_API(uart, test_uart_api) = {
	.callback_set = test_uart_callback_set,
	.rx_enable = test_uart_rx_enable,
	.rx_buf_rsp = test_uart_rx_buf_rsp,
	.rx_disable = test_uart_rx_disable,
	.poll_in = test_uart_poll_in,
	.poll_out = test_uart_poll_out,
	.err_check = test_uart_err_check,
	.configure = test_uart_configure,
	.config_get = test_uart_config_get,
};

#define TEST_UART_DEFINE(inst)                                                                     \
	static struct test_uart_data test_uart_data_##inst;                                        \
	DEVICE_DT_INST_DEFINE(inst, test_uart_init, NULL, &test_uart_data_##inst, NULL,            \
			      POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY, &test_uart_api);

DT_INST_FOREACH_STATUS_OKAY(TEST_UART_DEFINE)

static const struct device *const test_uart = DEVICE_DT_GET(TEST_UART_NODE);
static const struct device *const sbus = DEVICE_DT_GET(SBUS_NODE);

static void sbus_pack_frame(uint8_t frame[SBUS_FRAME_SIZE],
			    const uint16_t channels[SBUS_TEST_CHANNELS])
{
	memset(frame, 0, SBUS_FRAME_SIZE);
	frame[0] = 0x0f;
	frame[24] = 0x00;

	for (uint8_t channel = 0; channel < SBUS_TEST_CHANNELS; channel++) {
		uint16_t value = channels[channel] & 0x07ff;
		uint16_t bit_offset = channel * 11U;

		for (uint8_t bit = 0; bit < 11; bit++) {
			if ((value & BIT(bit)) != 0U) {
				frame[1 + ((bit_offset + bit) / 8U)] |=
					BIT((bit_offset + bit) % 8U);
			}
		}
	}
}

static void test_uart_emit_rx(const uint8_t *buf, size_t len)
{
	struct test_uart_data *data = test_uart->data;
	struct uart_event event = {
		.type = UART_RX_RDY,
		.data.rx.buf = (uint8_t *)buf,
		.data.rx.len = len,
		.data.rx.offset = 0,
	};

	zassert_true(data->rx_enabled, "SBUS driver did not enable UART RX");
	zassert_not_null(data->callback, "SBUS driver did not install UART callback");

	data->callback(test_uart, &event, data->user_data);
}

static void assert_sbus_cache_zeroed(void)
{
	struct sbus_driver_data *data = sbus->data;

	for (uint8_t i = 0; i < ARRAY_SIZE(data->data); i++) {
		zassert_equal(data->data[i], 0, "raw frame byte %u was not cleared", i);
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(data->channels); i++) {
		zassert_equal(data->channels[i], 0, "channel %u was not cleared", i);
	}

	zassert_false(data->frameLost, "frameLost was not cleared");
	zassert_false(data->failSafe, "failSafe was not cleared");
	zassert_false(data->digitalChannels[0], "digital channel 0 was not cleared");
	zassert_false(data->digitalChannels[1], "digital channel 1 was not cleared");
}

static void assert_float_close(float actual, float expected)
{
	zassert_true(fabsf(actual - expected) < SBUS_FLOAT_TOLERANCE, "actual=%f expected=%f",
		     (double)actual, (double)expected);
}

ZTEST(ares_native_sim_sbus, test_get_reports_offline_after_frame_timeout)
{
	uint16_t channels[SBUS_TEST_CHANNELS] = {
		[0] = 1024,
		[1] = SBUS_MAX,
		[2] = SBUS_MIN,
		[3] = 1200,
	};
	uint8_t frame[SBUS_FRAME_SIZE];

	zassert_true(device_is_ready(test_uart), "test UART is not ready");
	zassert_true(device_is_ready(sbus), "SBUS device is not ready");

	sbus_pack_frame(frame, channels);
	test_uart_emit_rx(frame, sizeof(frame));

	zassert_equal(sbus_get_digit(sbus, 0), 1024, "channel 0 was not parsed");
	zassert_equal(sbus_get_digit(sbus, 1), SBUS_MAX, "channel 1 was not parsed");
	zassert_equal(sbus_get_digit(sbus, 16), -EINVAL, "invalid channel did not fail");
	assert_float_close(sbus_get_percent(sbus, 1), 1.0f);

	k_sleep(K_MSEC(SBUS_OFFLINE_WAIT_MS));

	zassert_equal(sbus_get_digit(sbus, 0), -ENETDOWN, "offline digit get did not fail");
	assert_float_close(sbus_get_percent(sbus, 0), 0.0f);
	assert_sbus_cache_zeroed();
}

ZTEST_SUITE(ares_native_sim_sbus, NULL, NULL, NULL, NULL, NULL);
