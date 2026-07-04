/*
 * Copyright (c) 2026 ttwards <12411711@mail.sustech.edu.cn>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOTOR_LINK_H_
#define MOTOR_LINK_H_

#include <zephyr/device.h>
#include <zephyr/drivers/motor.h>

#include "motor_telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

void motor_link_request_enable(struct motor_link_state *link);
void motor_link_request_disable(struct motor_link_state *link);
bool motor_link_mark_online(const struct device *motor, struct motor_link_state *link,
			    enum motor_telemetry_reason reason);
bool motor_link_mark_offline(const struct device *motor, struct motor_link_state *link,
			     enum motor_telemetry_reason reason);
bool motor_link_observe_reply(const struct device *motor, struct motor_link_state *link);
bool motor_link_note_missed_reply(const struct device *motor, struct motor_link_state *link,
				  int16_t offline_after_misses);
bool motor_link_observe_periodic_report(const struct device *motor, struct motor_link_state *link);
bool motor_link_note_periodic_timeout(const struct device *motor, struct motor_link_state *link,
				      int16_t offline_after_misses);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_LINK_H_ */
