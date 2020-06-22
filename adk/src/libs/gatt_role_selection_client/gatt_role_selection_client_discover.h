/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROLE_SELECTION_CLIENT_DISCOVER_H_
#define GATT_ROLE_SELECTION_CLIENT_DISCOVER_H_

#include "gatt_role_selection_client_private.h"

/*!
    Discover all characteristics for the service.
*/
void roleSelectionDiscoverAllCharacteristics(GATT_ROLE_SELECTION_CLIENT *instance);


/*!
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message for 'Discover All Characteristics' response.
*/
void roleSelectionHandleDiscoverAllCharacteristicsResp(GATT_ROLE_SELECTION_CLIENT *instance, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm);


/*!
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
void roleSelectionDiscoverAllCharacteristicDescriptors(GATT_ROLE_SELECTION_CLIENT *instance, uint16 start_handle, uint16 end_handle);


/*!
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM message  for 'Discover All Characteristic Descriptors' response..
*/
void roleSelectionHandleDiscoverAllCharacteristicDescriptorsResp(GATT_ROLE_SELECTION_CLIENT *instance, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm);

#endif /* GATT_ROLE_SELECTION_CLIENT_DISCOVER_H_ */
