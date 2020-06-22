/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
#ifndef __LEDCTRL_H__
#define __LEDCTRL_H__
#include "hydra/hydra_types.h"
#include "hydra/hydra_macros.h"

#include <app/led/led_if.h>

extern void ledctrl_init(void);
extern bool ledctrl_config_led(led_id led, led_config_key key, uint16 value);
extern bool ledctrl_led_has_pio(led_id led);
extern void ledctrl_set_pio_mask(uint16 bank, uint32 mask, bool pin_is_led);
#endif /* __LEDCTRL_H__ */
