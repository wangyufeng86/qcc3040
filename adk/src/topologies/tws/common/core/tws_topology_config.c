/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Configuration / parameters used by the topology.
*/

#include "tws_topology_config.h"

static const bredr_scan_manager_scan_parameters_set_t inquiry_scan_params_set[] =
{
    {
        {
            [SCAN_MAN_PARAMS_TYPE_SLOW] = { .interval = 0x800, .window = 0x24 },
            [SCAN_MAN_PARAMS_TYPE_FAST] = { .interval = 0x200, .window = 0x12 },
        },
    },
};

static const bredr_scan_manager_scan_parameters_set_t page_scan_params_set[] =
{
    {
        {
            [SCAN_MAN_PARAMS_TYPE_SLOW] = { .interval = 0x800, .window = 0x24 },
            [SCAN_MAN_PARAMS_TYPE_FAST] = { .interval = 0x200, .window = 0x12 },
        },
    },
};

const bredr_scan_manager_parameters_t inquiry_scan_params =
{
    inquiry_scan_params_set, ARRAY_DIM(inquiry_scan_params_set)
};

const bredr_scan_manager_parameters_t page_scan_params =
{
    page_scan_params_set, ARRAY_DIM(page_scan_params_set)
};

#define MSEC_TO_LE_TIMESLOT(x)	((x)*1000/625)
#define FAST_ADVERTISING_INTERVAL_MIN_SLOTS MSEC_TO_LE_TIMESLOT(90)
#define FAST_ADVERTISING_INTERVAL_MAX_SLOTS MSEC_TO_LE_TIMESLOT(100)
#define SLOW_ADVERTISING_INTERVAL_MIN_SLOTS MSEC_TO_LE_TIMESLOT(225)
#define SLOW_ADVERTISING_INTERVAL_MAX_SLOTS MSEC_TO_LE_TIMESLOT(250)

static const le_adv_parameters_set_t params_set =
{
    {
        /* This is an ordered list, do not make any assumptions when changing the order of the list items */
        {SLOW_ADVERTISING_INTERVAL_MIN_SLOTS, SLOW_ADVERTISING_INTERVAL_MAX_SLOTS},
        {FAST_ADVERTISING_INTERVAL_MIN_SLOTS, FAST_ADVERTISING_INTERVAL_MAX_SLOTS}
    }
};

#define TIMEOUT_FALLBACK_IN_SECONDS 10


static const le_adv_parameters_config_table_t config_table =
{
    {
        /* This is an ordered list, do not make any assumptions when changing the order of the list items */
        {le_adv_preset_advertising_interval_fast, 0},
        {le_adv_preset_advertising_interval_fast, TIMEOUT_FALLBACK_IN_SECONDS},
        {le_adv_preset_advertising_interval_slow, 0}
    }
};

const le_adv_parameters_t le_adv_params = {.sets = &params_set, .table = &config_table };
