/*
 * Copyright (c) 2026 ttwards <12411711@mail.sustech.edu.cn>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "motor_telemetry.h"

#include "motor_can_sched.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(motor_telemetry, CONFIG_MOTOR_LOG_LEVEL);

#ifndef CONFIG_MOTOR_TELEMETRY_CAN_MAX_BUS
#define CONFIG_MOTOR_TELEMETRY_CAN_MAX_BUS 4
#endif

#ifndef CONFIG_MOTOR_TELEMETRY_CAN_LOG_INTERVAL_MS
#define CONFIG_MOTOR_TELEMETRY_CAN_LOG_INTERVAL_MS 2000
#endif

struct can_sched_snapshot {
	const struct device *can_dev;
	uint32_t tx_busy;
	uint32_t dropped_frames;
	uint32_t ack_timeouts;
	uint32_t pending_full;
	uint32_t giveups;
	uint32_t dropped_by_prio[MOTOR_CAN_SCHED_PRIO_COUNT];
};

static struct can_sched_snapshot can_snapshots[CONFIG_MOTOR_TELEMETRY_CAN_MAX_BUS];
static struct k_spinlock telemetry_lock;

static const char *reason_str(enum motor_telemetry_reason reason)
{
	switch (reason) {
	case MOTOR_TELEMETRY_REASON_REPLY:
		return "reply";
	case MOTOR_TELEMETRY_REASON_MISSED_REPLY:
		return "missed reply";
	case MOTOR_TELEMETRY_REASON_PERIODIC_REPORT:
		return "periodic report";
	case MOTOR_TELEMETRY_REASON_PERIODIC_TIMEOUT:
		return "periodic timeout";
	case MOTOR_TELEMETRY_REASON_RX_TIMEOUT:
		return "rx timeout";
	default:
		return "unknown";
	}
}

void motor_telemetry_motor_online(const struct device *motor, enum motor_telemetry_reason reason)
{
	if (motor == NULL) {
		return;
	}

	LOG_INF("Motor %s is online (%s).", motor->name, reason_str(reason));
}

void motor_telemetry_motor_offline(const struct device *motor, enum motor_telemetry_reason reason,
				   int16_t missed)
{
	if (motor == NULL) {
		return;
	}

	if (missed > 0) {
		LOG_ERR("Motor %s is offline (%s, missed=%d).", motor->name, reason_str(reason),
			missed);
		return;
	}

	LOG_ERR("Motor %s is offline (%s).", motor->name, reason_str(reason));
}

static struct can_sched_snapshot *can_snapshot_for(const struct device *can_dev)
{
	struct can_sched_snapshot *free_slot = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(can_snapshots); i++) {
		if (can_snapshots[i].can_dev == can_dev) {
			return &can_snapshots[i];
		}
		if ((can_snapshots[i].can_dev == NULL) && (free_slot == NULL)) {
			free_slot = &can_snapshots[i];
		}
	}

	if (free_slot != NULL) {
		free_slot->can_dev = can_dev;
	}
	return free_slot;
}

void motor_telemetry_can_scheduler_health(const struct device *can_dev,
					  const struct motor_can_sched_stats *stats)
{
	struct can_sched_snapshot previous;
	struct can_sched_snapshot *snapshot;
	uint32_t busy_window_us;
	uint32_t tx_busy_delta;
	uint32_t dropped_delta;
	uint32_t ack_timeout_delta;
	uint32_t pending_full_delta;
	uint32_t giveup_delta;
	uint32_t dropped_by_prio_delta[MOTOR_CAN_SCHED_PRIO_COUNT];
	bool pressure;

	if ((can_dev == NULL) || (stats == NULL)) {
		return;
	}

	busy_window_us =
		stats->window_tx_busy_us + stats->window_rx_busy_us + stats->window_reserved_us;

	k_spinlock_key_t key = k_spin_lock(&telemetry_lock);
	snapshot = can_snapshot_for(can_dev);
	if (snapshot == NULL) {
		k_spin_unlock(&telemetry_lock, key);
		return;
	}

	previous = *snapshot;
	snapshot->tx_busy = stats->tx_busy;
	snapshot->dropped_frames = stats->dropped_frames;
	snapshot->ack_timeouts = stats->ack_timeouts;
	snapshot->pending_full = stats->pending_full;
	snapshot->giveups = stats->giveups;
	memcpy(snapshot->dropped_by_prio, stats->dropped_by_prio,
	       sizeof(snapshot->dropped_by_prio));
	k_spin_unlock(&telemetry_lock, key);

	tx_busy_delta = stats->tx_busy - previous.tx_busy;
	dropped_delta = stats->dropped_frames - previous.dropped_frames;
	ack_timeout_delta = stats->ack_timeouts - previous.ack_timeouts;
	pending_full_delta = stats->pending_full - previous.pending_full;
	giveup_delta = stats->giveups - previous.giveups;
	for (size_t i = 0; i < ARRAY_SIZE(dropped_by_prio_delta); i++) {
		dropped_by_prio_delta[i] = stats->dropped_by_prio[i] - previous.dropped_by_prio[i];
	}

	pressure = (tx_busy_delta != 0U) || (dropped_delta != 0U) || (ack_timeout_delta != 0U) ||
		   (pending_full_delta != 0U) || (giveup_delta != 0U);

	if ((CONFIG_MOTOR_LOG_LEVEL >= LOG_LEVEL_DBG) && (stats->window_tx_latency_samples != 0U)) {
		uint32_t avg_latency_us =
			stats->window_tx_latency_sum_us / stats->window_tx_latency_samples;

		LOG_DBG("CAN %s scheduler tx latency: samples=%u avg=%uus max=%uus", can_dev->name,
			stats->window_tx_latency_samples, avg_latency_us,
			stats->window_tx_latency_max_us);
	}

	if (!pressure) {
		return;
	}

	LOG_WRN_RATELIMIT_RATE(
		CONFIG_MOTOR_TELEMETRY_CAN_LOG_INTERVAL_MS,
		"CAN %s scheduler pressure: tx_busy+%u dropped+%u ack_timeout+%u "
		"pending_full+%u giveup+%u drop=[%u,%u,%u,%u] "
		"qpeak=[%u,%u,%u,%u] window=%uus(tx=%u rx=%u reserved=%u)",
		can_dev->name, tx_busy_delta, dropped_delta, ack_timeout_delta, pending_full_delta,
		giveup_delta, dropped_by_prio_delta[0], dropped_by_prio_delta[1],
		dropped_by_prio_delta[2], dropped_by_prio_delta[3], stats->queue_peak[0],
		stats->queue_peak[1], stats->queue_peak[2], stats->queue_peak[3], busy_window_us,
		stats->window_tx_busy_us, stats->window_rx_busy_us, stats->window_reserved_us);
}
