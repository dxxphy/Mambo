#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/motor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

LOG_MODULE_REGISTER(motor_common, CONFIG_MOTOR_LOG_LEVEL);

#include "common.h"
#include "motor_can_sched.h"

static atomic_t runtime_stats[MOTOR_STAT_COUNT];

int can_work_init(void)
{
	motor_can_sched_init();
	return 0;
}

void motor_stats_inc(enum motor_runtime_stat stat)
{
	if ((unsigned int)stat < MOTOR_STAT_COUNT) {
		atomic_inc(&runtime_stats[stat]);
	}
}

void motor_stats_get(struct motor_runtime_stats *stats)
{
	if (stats == NULL) {
		return;
	}

	for (size_t i = 0; i < MOTOR_STAT_COUNT; i++) {
		stats->counters[i] = (uint32_t)atomic_get(&runtime_stats[i]);
	}
}

SYS_INIT(can_work_init, APPLICATION, CONFIG_MOTOR_INIT_PRIORITY);
