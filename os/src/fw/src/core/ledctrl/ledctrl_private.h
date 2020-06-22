/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
#ifndef __LEDCTRL_PRIVATE_H__
#define __LEDCTRL_PRIVATE_H__

#include "ledctrl/ledctrl.h"
#include "hydra_log/hydra_log.h"
#define IO_DEFS_MODULE_LED_CTRL
#define IO_DEFS_MODULE_APPS_SYS_CLKGEN
#include "io/io_map.h"
#include "hal/hal_macros.h"
#include "io/io_defs.h"

/* Defines for fields inside LED_LOGARITHMIC_STATUS */
#define LOGSTAT_EN_OFFSET   0x08
#define LOGSTAT_EN_MASK     0x01
#define LOGSTAT_OFFHI_OFFSET 0x04
#define LOGSTAT_OFFHI_MASK  0x0f
#define LOGSTAT_OFFLO_OFFSET 0x00
#define LOGSTAT_OFFLO_MASK  0x0f

/* Define the maximum values for config parameters */
#define LEDCTRL_MAX_PERIOD          0x0f
#define LEDCTRL_INVALID_PERIOD      0xff

#define LEDCTRL_MAX_DUTY_CYCLE      0x0fff
#define LEDCTRL_INVALID_DUTY_CYCLE  0x1000

#define LEDCTRL_MAX_LOG_OFFSET      0x0f

#define LEDCTRL_MAX_FLASH_RATE      0x0f

#ifdef DESKTOP_TEST_BUILD
/* The real defs use hw enums that aren't present on desktop builds */
#define LEDCTRL_MAX_INITIAL_STATE (7)
#define LEDCTRL_MAX_PIN_DRIVE_CONFIG (3)
#else
#define LEDCTRL_MAX_INITIAL_STATE   (LED_START_UP_STATE_COUNT_HIGH_RAMP_DOWN)

#define LEDCTRL_MAX_PIN_DRIVE_CONFIG (LED_PIN_CONFIG_PUSH_PULL_INVERTED)
#endif /* DESKTOP_TEST_BUILD */

#define LEDCTRL_MAX_GROUPS          31

/* Internal data structures */

typedef struct
{
    uint32      groups;
    uint16      init_count;
    unsigned int init_state:3;
} led_config;

#endif /* __LEDCTRL_PRIVATE_H__ */
