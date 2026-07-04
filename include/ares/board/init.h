// init.h
#ifndef INIT_H
#define INIT_H

#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/util.h>

struct led_rgb;

#ifdef CONFIG_BOARD_DM_MC02
enum ares_board_xt30 {
	ARES_BOARD_XT30_1 = BIT(0),
	ARES_BOARD_XT30_2 = BIT(1),
	ARES_BOARD_XT30_ALL = ARES_BOARD_XT30_1 | ARES_BOARD_XT30_2,
};

int ares_board_xt30_power_set(enum ares_board_xt30 xt30, bool enable);
static inline int ares_board_xt30_power_on(enum ares_board_xt30 xt30)
{
	return ares_board_xt30_power_set(xt30, true);
}

static inline int ares_board_xt30_power_off(enum ares_board_xt30 xt30)
{
	return ares_board_xt30_power_set(xt30, false);
}
#endif /* CONFIG_BOARD_DM_MC02 */

#endif /* INIT_H */
