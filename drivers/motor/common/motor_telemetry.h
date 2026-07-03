/*
 * Copyright (c) 2026 ttwards <12411711@mail.sustech.edu.cn>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOTOR_TELEMETRY_H_
#define MOTOR_TELEMETRY_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

struct motor_can_sched_stats;

enum motor_telemetry_reason {
	MOTOR_TELEMETRY_REASON_REPLY,
	MOTOR_TELEMETRY_REASON_MISSED_REPLY,
	MOTOR_TELEMETRY_REASON_PERIODIC_REPORT,
	MOTOR_TELEMETRY_REASON_PERIODIC_TIMEOUT,
	MOTOR_TELEMETRY_REASON_RX_TIMEOUT,
};

#if defined(CONFIG_MOTOR_TELEMETRY)
void motor_telemetry_motor_online(const struct device *motor, enum motor_telemetry_reason reason);
void motor_telemetry_motor_offline(const struct device *motor, enum motor_telemetry_reason reason,
				   int16_t missed);
void motor_telemetry_can_scheduler_health(const struct device *can_dev,
					  const struct motor_can_sched_stats *stats);
#else
static inline void motor_telemetry_motor_online(const struct device *motor,
						enum motor_telemetry_reason reason)
{
	ARG_UNUSED(motor);
	ARG_UNUSED(reason);
}

static inline void motor_telemetry_motor_offline(const struct device *motor,
						 enum motor_telemetry_reason reason, int16_t missed)
{
	ARG_UNUSED(motor);
	ARG_UNUSED(reason);
	ARG_UNUSED(missed);
}

static inline void motor_telemetry_can_scheduler_health(const struct device *can_dev,
							const struct motor_can_sched_stats *stats)
{
	ARG_UNUSED(can_dev);
	ARG_UNUSED(stats);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_TELEMETRY_H_ */
