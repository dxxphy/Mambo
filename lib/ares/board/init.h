#ifndef ARES_BOARD_INIT_H_
#define ARES_BOARD_INIT_H_

#include <stdbool.h>
#include <stdint.h>

struct ares_led_rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

void ares_board_power_init(void);

#ifdef CONFIG_BOARD_DM_MC02
#include <zephyr/sys/util.h>

enum ares_board_xt30 {
	ARES_BOARD_XT30_1 = BIT(0),
	ARES_BOARD_XT30_2 = BIT(1),
	ARES_BOARD_XT30_ALL = ARES_BOARD_XT30_1 | ARES_BOARD_XT30_2,
};

int ares_board_xt30_power_set(enum ares_board_xt30 xt30, bool enable);
__unused static inline int ares_board_xt30_power_on(enum ares_board_xt30 xt30)
{
	return ares_board_xt30_power_set(xt30, true);
}

__unused static inline int ares_board_xt30_power_off(enum ares_board_xt30 xt30)
{
	return ares_board_xt30_power_set(xt30, false);
}
#endif /* CONFIG_BOARD_DM_MC02 */

void ares_board_status_led_init(void);
void ares_board_status_led_service_start(void);
int ares_board_status_led_set_rgb(const struct ares_led_rgb *color);
uint8_t ares_board_status_led_max_channel(void);

#endif /* ARES_BOARD_INIT_H_ */
