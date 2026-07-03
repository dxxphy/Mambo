/*
 * Copyright (c) 2026 ttwards <12411711@mail.sustech.edu.cn>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "motor_link.h"

#include <stddef.h>

static void reset_missed(int16_t *missed)
{
	if (missed != NULL) {
		*missed = 0;
	}
}

static void increment_missed(int16_t *missed)
{
	if ((missed != NULL) && (*missed < INT16_MAX)) {
		(*missed)++;
	}
}

void motor_link_request_enable(bool *enabled, int16_t *missed)
{
	if (enabled != NULL) {
		*enabled = true;
	}
	reset_missed(missed);
}

void motor_link_request_disable(bool *online, bool *enabled, int16_t *missed)
{
	if (online != NULL) {
		*online = false;
	}
	if (enabled != NULL) {
		*enabled = false;
	}
	reset_missed(missed);
}

bool motor_link_observe_reply(bool *online, int16_t *missed)
{
	bool was_offline = (online != NULL) && !*online;

	if (online != NULL) {
		*online = true;
	}
	reset_missed(missed);
	return was_offline;
}

bool motor_link_note_missed_reply(bool *online, bool enabled, int16_t *missed,
				  int16_t offline_after_misses)
{
	if (!enabled || (online == NULL) || (missed == NULL)) {
		return false;
	}

	increment_missed(missed);
	if (*online && (*missed >= offline_after_misses)) {
		*online = false;
		return true;
	}

	return false;
}

bool motor_link_observe_periodic_report(bool *online, int16_t *missed)
{
	return motor_link_observe_reply(online, missed);
}

bool motor_link_note_periodic_timeout(bool *online, int16_t *missed, int16_t offline_after_timeouts)
{
	if ((online == NULL) || (missed == NULL) || !*online) {
		return false;
	}

	increment_missed(missed);
	if (*missed >= offline_after_timeouts) {
		*online = false;
		return true;
	}

	return false;
}
