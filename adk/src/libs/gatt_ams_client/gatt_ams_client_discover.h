/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_AMS_CLIENT_DISCOVER_H_
#define GATT_AMS_CLIENT_DISCOVER_H_

#include "gatt_ams_client.h"
#include <gatt_manager.h>

/*
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message.
*/
void handleAmsDiscoverAllCharacteristicsResp(GAMS *ams, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *msg);

/*
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
bool gattAmsDiscoverAllCharacteristicDescriptors(GAMS *ams, uint16 start_handle, uint16 end_handle);

/*
   Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM message.
*/
void handleAmsDiscoverAllCharacteristicDescriptorsResp(GAMS *ams, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *msg);

/*
   Helper function to get the endhandle for discovering characteristic descriptor of NS
*/
uint16 gattAmsfindEndHandleForCharDesc(GAMS *ams, uint16 startHandle, uint16 endHandle, uint8 characteristic);

#endif /* GATT_AMS_CLIENT_DISCOVER_H_ */
