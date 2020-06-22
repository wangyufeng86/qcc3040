/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Manage execution of callbacks to construct adverts and scan response
*/

#ifndef LE_ADVERTSING_MANAGER_DATA_H_
#define LE_ADVERTSING_MANAGER_DATA_H_

#include "le_advertising_manager.h"
#include "le_advertising_manager_private.h"

/*!
    Build advertising and scan response data packets
    
    Must be called before use of 
    - leAdvertisingManager_SetupScanResponseData
    - leAdvertisingManager_SetupAdvertData
    - leAdvertisingManager_ClearData
    
    \param set Mask of the data set(s) to build
    \returns TRUE if successful, FALSE if there is no data
 */
bool leAdvertisingManager_BuildData(le_adv_data_set_t set);

/*!
    Get scan response data from clients and add it to our scan response
    
    \returns TRUE if successful, FALSE otherwise
 */
void leAdvertisingManager_SetupScanResponseData(void);

/*!
    Register advertising packet built with leAdvertisingManager_BuildData
 */
void leAdvertisingManager_SetupAdvertData(void);

/*!
    Clear advertising and scan response data packets
    Must be called to clear data created with
    leAdvertisingManager_BuildData
    
    \param set Mask of the data set(s) to clear
 */
void leAdvertisingManager_ClearData(le_adv_data_set_t set);

#endif /* LE_ADVERTSING_MANAGER_DATA_H_ */
