/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    dm_ble_privacy_mode.h

DESCRIPTION
   
    This file contains the prototypes for BLE DM Privacy Mode functions.

NOTES

*/

#include <app/bluestack/dm_prim.h>

/****************************************************************************
NAME    
    connectionHandleDmUlpSetPrivacyModeCfm

DESCRIPTION
    Handler for Privacy Mode preferences messages

RETURNS
    void
*/
void connectionHandleDmUlpSetPrivacyModeCfm(const DM_HCI_ULP_SET_PRIVACY_MODE_CFM_T *ind);




