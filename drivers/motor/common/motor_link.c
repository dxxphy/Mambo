/*
 * Copyright (c) 2026 ttwards <12411711@mail.sustech.edu.cn>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "motor_link.h"

static void reset_missed(struct motor_link_state *link)
{
	if (link != NULL) {
		link->missed = 0;
	}
}

static void increment_missed(struct motor_link_state *link)
{
	if ((link != NULL) && (link->missed < INT16_MAX)) {
		link->missed++;
	}
}

void motor_link_request_enable(struct motor_link_state *link)
{
	if (link != NULL) {
		link->requested_enabled = true;
	}
	reset_missed(link);
}

void motor_link_request_disable(struct motor_link_state *link)
{
	if (link != NULL) {
		link->online = false;
		link->requested_enabled = false;
	}
	reset_missed(link);
}

bool motor_link_mark_online(const struct device *motor, struct motor_link_state *link,
			    enum motor_telemetry_reason reason)
{
	bool was_offline = (link != NULL) && !link->online;

	if (link != NULL) {
		link->online = true;
	}
	reset_missed(link);
	if (was_offline) {
		motor_telemetry_motor_online(motor, reason);
	}
	return was_offline;
}

bool motor_link_mark_offline(const struct device *motor, struct motor_link_state *link,
			     enum motor_telemetry_reason reason)
{
	int16_t missed_count = link != NULL ? link->missed : 0;

	if ((link == NULL) || !link->online) {
		return false;
	}

	link->online = false;
	motor_telemetry_motor_offline(motor, reason, missed_count);
	return true;
}

bool motor_link_observe_reply(const struct device *motor, struct motor_link_state *link)
{
	return motor_link_mark_online(motor, link, MOTOR_TELEMETRY_REASON_REPLY);
}

bool motor_link_note_missed_reply(const struct device *motor, struct motor_link_state *link,
				  int16_t offline_after_misses)
{
	if ((link == NULL) || !link->requested_enabled) {
		return false;
	}

	increment_missed(link);
	if (link->online && (link->missed >= offline_after_misses)) {
		return motor_link_mark_offline(motor, link, MOTOR_TELEMETRY_REASON_MISSED_REPLY);
	}

	return false;
}

bool motor_link_observe_periodic_report(const struct device *motor, struct motor_link_state *link)
{
	return motor_link_mark_online(motor, link, MOTOR_TELEMETRY_REASON_PERIODIC_REPORT);
}

bool motor_link_note_periodic_timeout(const struct device *motor, struct motor_link_state *link,
				      int16_t offline_after_timeouts)
{
	if ((link == NULL) || !link->online) {
		return false;
	}

	increment_missed(link);
	if (link->missed >= offline_after_timeouts) {
		return motor_link_mark_offline(motor, link,
					       MOTOR_TELEMETRY_REASON_PERIODIC_TIMEOUT);
	}

	return false;
}
