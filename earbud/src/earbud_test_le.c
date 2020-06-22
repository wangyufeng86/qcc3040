/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Implementation of application testing functions for LE functionality.
*/


#include "adk_log.h"
#include <connection.h>

#include "earbud_test_le.h"

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

void appTestSetAdvertEnable(bool enable)
{
    ConnectionDmBleSetAdvertiseEnable(enable);
}


void appTestSetAdvertisingParamsReq(void)
{
    ble_adv_params_t parm;
    parm.undirect_adv.adv_interval_min = 0x800;
    parm.undirect_adv.adv_interval_max = 0x2000;
    parm.undirect_adv.filter_policy    = ble_filter_none;

    ConnectionDmBleSetAdvertisingParamsReq(ble_adv_nonconn_ind,FALSE,BLE_ADV_CHANNEL_ALL,&parm);
}

