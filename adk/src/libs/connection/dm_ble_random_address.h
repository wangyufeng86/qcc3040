/****************************************************************************
Copyright (c) 2011 - 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    dm_ble_random_address.h      

DESCRIPTION
    This file contains the prototypes for BLE DM Random Address functions.

NOTES

*/

#include "connection.h"
#include "connection_private.h"

#include <app/bluestack/dm_prim.h>
/****************************************************************************
NAME    
    connectionHandleDmSmAutoConfigureLocalAddressCfm

DESCRIPTION
    Handle the DM_SM_AUTO_CONFIGURE_LOCAL_ADDRESS_CFM from Bluestack.

RETURNS
   void
*/
void connectionHandleDmSmAutoConfigureLocalAddressCfm(
        const DM_SM_AUTO_CONFIGURE_LOCAL_ADDRESS_CFM_T* cfm
        );

#ifndef DISABLE_BLE
/****************************************************************************
NAME
    connectionHandleDmBleReadRandomAddress

DESCRIPTION
    This function is called on receipt of an
    CL_INTERNAL_DM_BLE_READ_RANDOM_ADDRESS_REQ message.

RETURNS
   void
*/
void connectionHandleDmBleReadRandomAddress(
        connectionBleReadRndAddrState *state,
        const CL_INTERNAL_DM_BLE_READ_RANDOM_ADDRESS_REQ_T* req
        );
#endif

/****************************************************************************
NAME    
    connectionHandleSmReadRandomAddressCfm

DESCRIPTION
    Handle the DM_SM_READ_RANDOM_ADDRESS_CFM_T from Bluestack.

RETURNS
   void
*/
void connectionHandleDmSmReadRandomAddressCfm(
        const DM_SM_READ_RANDOM_ADDRESS_CFM_T* cfm
        );
