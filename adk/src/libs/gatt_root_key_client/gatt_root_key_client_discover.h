/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROOT_KEY_CLIENT_DISCOVER_H_
#define GATT_ROOT_KEY_CLIENT_DISCOVER_H_

#include "gatt_root_key_client_private.h"

/*!
    Discover all characteristics for the service.
*/
void rootKeyDiscoverAllCharacteristics(GATT_ROOT_KEY_CLIENT *instance);


/*!
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message for 'Discover All Characteristics' response.
*/
void rootKeyHandleDiscoverAllCharacteristicsResp(GATT_ROOT_KEY_CLIENT *instance, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm);


/*!
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
void rootKeyDiscoverAllCharacteristicDescriptors(GATT_ROOT_KEY_CLIENT *instance, uint16 start_handle, uint16 end_handle);


/*!
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM message  for 'Discover All Characteristic Descriptors' response..
*/
void rootKeyHandleDiscoverAllCharacteristicDescriptorsResp(GATT_ROOT_KEY_CLIENT *instance, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm);

#endif /* GATT_ROOT_KEY_CLIENT_DISCOVER_H_ */
