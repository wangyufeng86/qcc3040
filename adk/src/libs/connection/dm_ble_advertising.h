/****************************************************************************
Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    dm_ble_advetising.h      

DESCRIPTION
    This file contains the prototypes for BLE DM advertising msgs from Bluestack .

NOTES

*/

#ifndef DISABLE_BLE
#ifndef DM_BLE_ADVERTISING_H
#define DM_BLE_ADVERTISING_H

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
        );


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
        );


/****************************************************************************
NAME
    ConnectionDmBleSetAdvertisingParametersReq

DESCRIPTION
    Sets BLE Advertising parameters

RETURNS
   void
*/
void connectionHandleDmBleSetAdvertiseEnableCfm(
        connectionBleScanAdState *state,
        const DM_HCI_ULP_SET_ADVERTISE_ENABLE_CFM_T *cfm
        );

#endif /* DM_BLE_ADVERTISING_H */
#endif /* DISABLE_BLE */
