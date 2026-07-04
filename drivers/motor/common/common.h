#ifndef MOTOR_COMMON_H_
#define MOTOR_COMMON_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/motor.h>

enum motor_runtime_stat {
	MOTOR_STAT_UNSUPPORTED_MODE,
	MOTOR_STAT_UNSUPPORTED_CMD,
	MOTOR_STAT_CONFIG_ERROR,
	MOTOR_STAT_CAN_FILTER_ERROR,
	MOTOR_STAT_UNKNOWN_RX,
	MOTOR_STAT_DRIVER_ERROR,
	MOTOR_STAT_LIMIT_CLAMP,
	MOTOR_STAT_TX_ERROR,
	MOTOR_STAT_COUNT,
};

struct motor_runtime_stats {
	uint32_t counters[MOTOR_STAT_COUNT];
};

void motor_stats_inc(enum motor_runtime_stat stat);
void motor_stats_get(struct motor_runtime_stats *stats);

// Macros
#define HIGH_BYTE(x)           ((x) >> 8)
#define LOW_BYTE(x)            ((x) & 0xFF)
#define COMBINE_HL8(HIGH, LOW) ((HIGH << 8) + LOW)

#endif // MOTOR_COMMON_H_
