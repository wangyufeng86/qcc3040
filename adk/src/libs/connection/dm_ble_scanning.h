/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    dm_ble_scanning.h

DESCRIPTION
    Deals with DM_HCI_ULP (BLE) prims from bluestack related to Scan
    functinality

NOTES

*/

/****************************************************************************
NAME
    connectionHandleBleSetScanCfm

DESCRIPTION
    Handles BlueStack cfm message for BLE Set Scan.

RETURNS
    void
*/

#ifndef DISABLE_BLE
#ifndef DM_BLE_SCANNING_H
#define DM_BLE_SCANNING_H


#include "connection.h"
#include "connection_private.h"
#include <app/bluestack/types.h>
#include <app/bluestack/dm_prim.h>

void connectionHandleDmBleSetScanEnableReq(
        connectionBleScanAdState *state,
        const CL_INTERNAL_DM_BLE_SET_SCAN_ENABLE_REQ_T *req
        );

void connectionHandleDmBleSetScanEnableCfm(
        connectionBleScanAdState *state,
        const DM_HCI_ULP_SET_SCAN_ENABLE_CFM_T* cfm
        );

#endif // DM_BLE_SCANNING_H
#endif /* DISABLE_BLE */
