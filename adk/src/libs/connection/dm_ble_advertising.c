/****************************************************************************
Copyright (c) 2011 - 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    dm_ble_scanning.c      

DESCRIPTION
    This file contains the implementation of Low Energy scan configuration.

NOTES

*/

#include "connection.h"
#include "connection_private.h"
#include "common.h"
#include "bdaddr.h"
#include "dm_ble_advertising.h"

#include <vm.h>

#ifndef DISABLE_BLE

#if HYDRACORE
#define NO_CFM_MESSAGE ((Task)0x0FFFFFFF)
#else
#define NO_CFM_MESSAGE ((Task)0x0000FFFF)
#endif

/****************************************************************************
NAME    
    ConnectionDmBleSetAdvertisingDataReq

DESCRIPTION
    Sets BLE Advertising data (0..31 octets).

RETURNS
   void
*/
void ConnectionDmBleSetAdvertisingDataReq(uint8 size_ad_data, const uint8 *ad_data)
{
    
#ifdef CONNECTION_DEBUG_LIB
        /* Check parameters. */
    if (size_ad_data == 0 || size_ad_data > BLE_AD_PDU_SIZE)
    {
        CL_DEBUG(("Pattern length is zero\n"));
    }
    if (ad_data == 0)
    {
        CL_DEBUG(("Pattern is null\n"));
    }
#endif
    {
        MAKE_PRIM_C(DM_HCI_ULP_SET_ADVERTISING_DATA_REQ);
        prim->advertising_data_len = size_ad_data;
        memmove(prim->advertising_data, ad_data, size_ad_data);
        VmSendDmPrim(prim);
    }
}

/****************************************************************************
NAME
    ConnectionDmBleSetAdvertiseEnable

DESCRIPTION
    Enable or Disable BLE Advertising.

RETURNS
   void
*/
void ConnectionDmBleSetAdvertiseEnable(bool enable)
{
    ConnectionDmBleSetAdvertiseEnableReq(NO_CFM_MESSAGE, enable);
}

/****************************************************************************
NAME
    ConnectionDmBleSetAdvertiseEnableReq

DESCRIPTION
    Enables or disables BLE Advertising. If theAppTask is anthing other than
    NULL(0) then that is treated as the task to return the CFM message to.

RETURNS
   void
*/
void ConnectionDmBleSetAdvertiseEnableReq(Task theAppTask, bool enable)
{
    MAKE_CL_MESSAGE(CL_INTERNAL_DM_BLE_SET_ADVERTISE_ENABLE_REQ);
    message->theAppTask = theAppTask;
    message->enable = enable;
    MessageSend(
                connectionGetCmTask(),
                CL_INTERNAL_DM_BLE_SET_ADVERTISE_ENABLE_REQ,
                message);
}

/****************************************************************************
NAME
    connectionHandleDmBleSetAdvertiseEnableReq.

DESCRIPTION
    This function will initiate an Advertising Enable request.

RETURNS
   void
*/
void connectionHandleDmBleSetAdvertiseEnableReq(
        connectionBleScanAdState *state,
        const CL_INTERNAL_DM_BLE_SET_ADVERTISE_ENABLE_REQ_T *req
        )
 {
    /* Check the state of the task lock before doing anything. */
    if (!state->bleScanAdLock)
    {
        MAKE_PRIM_C(DM_HCI_ULP_SET_ADVERTISE_ENABLE_REQ);

        /* One request at a time, set the ad lock. */
        state->bleScanAdLock = req->theAppTask;

        prim->advertising_enable = (req->enable) ? 1 : 0;

        VmSendDmPrim(prim);
    }
    else
    {
        /* Scan or Ad request already outstanding, queue up this request. */
        MAKE_CL_MESSAGE(CL_INTERNAL_DM_BLE_SET_ADVERTISE_ENABLE_REQ);
        COPY_CL_MESSAGE(req, message);
        MessageSendConditionallyOnTask(
                    connectionGetCmTask(),
                    CL_INTERNAL_DM_BLE_SET_ADVERTISE_ENABLE_REQ,
                    message,
                    &state->bleScanAdLock
                    );
    }
}

/****************************************************************************
NAME
    connectionHandleDmBleSetAdvertiseEnableCfm

DESCRIPTION
    Sets BLE Advertising parameters

RETURNS
   void
*/
void connectionHandleDmBleSetAdvertiseEnableCfm(
        connectionBleScanAdState *state,
        const DM_HCI_ULP_SET_ADVERTISE_ENABLE_CFM_T *cfm
        )
{
    if (state->bleScanAdLock != NO_CFM_MESSAGE)
    {
        MAKE_CL_MESSAGE(CL_DM_BLE_SET_ADVERTISE_ENABLE_CFM);
        message->status = connectionConvertHciStatus(cfm->status);
        MessageSend(
                    state->bleScanAdLock,
                    CL_DM_BLE_SET_ADVERTISE_ENABLE_CFM,
                    message
                    );
    }

    /* Reset the ad lock. */
    state->bleScanAdLock = NULL;
}


/****************************************************************************
NAME    
    ConnectionDmBleSetAdvertisingParametersReq

DESCRIPTION
    Sets BLE Advertising parameters

RETURNS
   void
*/
void ConnectionDmBleSetAdvertisingParamsReq( 
        ble_adv_type adv_type,
        uint8 own_address,
        uint8 channel_map,
        const ble_adv_params_t *adv_params 
        )
{
    MAKE_PRIM_C(DM_HCI_ULP_SET_ADVERTISING_PARAMETERS_REQ);

    /* Set defaults to avoid HCI validation failures */
    prim->direct_address_type = HCI_ULP_ADDRESS_PUBLIC;
    prim->adv_interval_max = 0x0800; /* 1.28s */
    prim->adv_interval_min = 0x0800;
    prim->advertising_filter_policy = HCI_ULP_ADV_FP_ALLOW_ANY;

    switch(adv_type)
    {
    case ble_adv_ind:
        prim->advertising_type =
                HCI_ULP_ADVERT_CONNECTABLE_UNDIRECTED;
        break;
    case ble_adv_direct_ind:
    case ble_adv_direct_ind_high_duty:
        prim->advertising_type =
                HCI_ULP_ADVERT_CONNECTABLE_DIRECTED_HIGH_DUTY;
        break;
    case ble_adv_scan_ind:
        prim->advertising_type =
                HCI_ULP_ADVERT_DISCOVERABLE;
        break;
    case ble_adv_nonconn_ind:
        prim->advertising_type =
                HCI_ULP_ADVERT_NON_CONNECTABLE;
        break;
    case ble_adv_direct_ind_low_duty:
        prim->advertising_type =
                HCI_ULP_ADVERT_CONNECTABLE_DIRECTED_LOW_DUTY;
        break;
    }

    prim->own_address_type = connectionConvertOwnAddress(own_address);

    channel_map &= BLE_ADV_CHANNEL_ALL;

    prim->advertising_channel_map = channel_map & BLE_ADV_CHANNEL_ALL;

    if (adv_type ==  ble_adv_direct_ind ||
            adv_type ==  ble_adv_direct_ind_high_duty ||
            adv_type ==  ble_adv_direct_ind_low_duty )
    {
        /* Without an address, this cannot proceed. */
        if (
                !adv_params ||
                BdaddrIsZero(&adv_params->direct_adv.direct_addr)
                )
            Panic();

        /* Use the ble_directed_adv_params_t type of the union for all
             * 'direct' advertising params, as it is same as the
             * ble_directed_low_duty_adv_params_t type for the first two
             * elements.
             */

        prim->direct_address_type =
                (adv_params->low_duty_direct_adv.random_direct_address) ?
                    HCI_ULP_ADDRESS_RANDOM : HCI_ULP_ADDRESS_PUBLIC;

        BdaddrConvertVmToBluestack(
                    &prim->direct_address,
                    &adv_params->low_duty_direct_adv.direct_addr
                    );

        if (adv_type == ble_adv_direct_ind_low_duty)
        {
            prim->adv_interval_min = adv_params->low_duty_direct_adv.adv_interval_min;
            prim->adv_interval_max = adv_params->low_duty_direct_adv.adv_interval_max;
        }
    }
    else
    {
        if (adv_params)
        {
            /* These params are validated by HCI. */
            prim->adv_interval_min
                    = adv_params->undirect_adv.adv_interval_min;
            prim->adv_interval_max
                    = adv_params->undirect_adv.adv_interval_max;

            switch (adv_params->undirect_adv.filter_policy)
            {
            case ble_filter_none:
                prim->advertising_filter_policy =
                        HCI_ULP_ADV_FP_ALLOW_ANY;
                break;
            case ble_filter_scan_only:
                prim->advertising_filter_policy =
                        HCI_ULP_ADV_FP_ALLOW_CONNECTIONS;
                break;
            case ble_filter_connect_only:
                prim->advertising_filter_policy =
                        HCI_ULP_ADV_FP_ALLOW_SCANNING;
                break;
            case ble_filter_both:
                prim->advertising_filter_policy =
                        HCI_ULP_ADV_FP_ALLOW_WHITELIST;
                break;
            }

            /* Set the direct address & type to 0, as they are not used. */
            prim->direct_address_type = 0;
            BdaddrSetZero(&prim->direct_address);
        }
        /* otherwise, if 'adv_params' is null, defaults are used. */
    }

    VmSendDmPrim(prim);
}


/****************************************************************************
NAME    
    connectionHandleDmBleAdvParamUpdateInd

DESCRIPTION
    Handle the DM_ULP_ADV_PARAM_UPDATE_IND message from Bluestack and pass it
    on to the appliction that initialised the CL.

RETURNS
    void
*/
void connectionHandleDmBleAdvParamUpdateInd( 
        const DM_ULP_ADV_PARAM_UPDATE_IND_T *ind
        ) 
{
    MAKE_CL_MESSAGE(CL_DM_BLE_ADVERTISING_PARAM_UPDATE_IND);
    
    message->adv_interval_min = ind->adv_interval_min;
    message->adv_interval_max = ind->adv_interval_max;

    switch(ind->advertising_type)
    {
        case HCI_ULP_ADVERT_CONNECTABLE_UNDIRECTED:
            message->advertising_type = ble_adv_ind;
            break;
        case HCI_ULP_ADVERT_CONNECTABLE_DIRECTED_HIGH_DUTY:
            message->advertising_type = ble_adv_direct_ind_high_duty;
            break;
        case HCI_ULP_ADVERT_DISCOVERABLE:
            message->advertising_type = ble_adv_scan_ind;
            break;
        case HCI_ULP_ADVERT_NON_CONNECTABLE:
            message->advertising_type = ble_adv_nonconn_ind;
            break;
        case HCI_ULP_ADVERT_CONNECTABLE_DIRECTED_LOW_DUTY:
            message->advertising_type = ble_adv_direct_ind_low_duty;
            break;
        default:
            CL_DEBUG(( 
                "Received unknown advertising type: %d\n",
                ind->advertising_type
                ));
            message->advertising_type = 0xFF;
            break;
    } 

    message->own_address_type = ind->own_address_type;
    message->direct_address_type = ind->direct_address_type;
    BdaddrConvertBluestackToVm(&message->direct_bd_addr, &ind->direct_address);
    message->advertising_channel_map = ind->advertising_channel_map;

    switch (ind->advertising_filter_policy)
    {
        case HCI_ULP_ADV_FP_ALLOW_ANY:
            message->advertising_filter_policy = ble_filter_none;
            break;
        case HCI_ULP_ADV_FP_ALLOW_CONNECTIONS:
            message->advertising_filter_policy = ble_filter_scan_only;
            break;
        case HCI_ULP_ADV_FP_ALLOW_SCANNING:
            message->advertising_filter_policy = ble_filter_connect_only;
            break;
        case HCI_ULP_ADV_FP_ALLOW_WHITELIST:
            message->advertising_filter_policy = ble_filter_both;
            break;    
        default:
            CL_DEBUG(( 
                "Received unknown advertising filter policy: %d\n", 
                ind->advertising_type 
                ));
            message->advertising_filter_policy = 0xFF;
            break;
    }

    MessageSend(
            connectionGetAppTask(),
            CL_DM_BLE_ADVERTISING_PARAM_UPDATE_IND,
            message);
}
#endif /* DISABLE_BLE */

