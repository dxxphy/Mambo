/*
 * Copyright (c) 2026 ttwards <12411711@mail.sustech.edu.cn>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOTOR_LINK_H_
#define MOTOR_LINK_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void motor_link_request_enable(bool *enabled, int16_t *missed);
void motor_link_request_disable(bool *online, bool *enabled, int16_t *missed);
bool motor_link_observe_reply(bool *online, int16_t *missed);
bool motor_link_note_missed_reply(bool *online, bool enabled, int16_t *missed,
				  int16_t offline_after_misses);
bool motor_link_observe_periodic_report(bool *online, int16_t *missed);
bool motor_link_note_periodic_timeout(bool *online, int16_t *missed,
				      int16_t offline_after_timeouts);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_LINK_H_ */
