/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Configuration / parameters used by the topology.
*/

#include "headset_topology_config.h"

static const bredr_scan_manager_scan_parameters_set_t hs_inquiry_scan_params_set[] =
{
    {
        {
            [SCAN_MAN_PARAMS_TYPE_SLOW] = { .interval = 0x800, .window = 0x24 },
            [SCAN_MAN_PARAMS_TYPE_FAST] = { .interval = 0x200, .window = 0x12 },
        },
    },
};

static const bredr_scan_manager_scan_parameters_set_t hs_page_scan_params_set[] =
{
    {
        {
            [SCAN_MAN_PARAMS_TYPE_SLOW] = { .interval = 0x800, .window = 0x24 },
            [SCAN_MAN_PARAMS_TYPE_FAST] = { .interval = 0x200, .window = 0x12 },
        },
    },
};

const bredr_scan_manager_parameters_t hs_inquiry_scan_params =
{
    hs_inquiry_scan_params_set, ARRAY_DIM(hs_inquiry_scan_params_set)
};

const bredr_scan_manager_parameters_t hs_page_scan_params =
{
    hs_page_scan_params_set, ARRAY_DIM(hs_page_scan_params_set)
};
