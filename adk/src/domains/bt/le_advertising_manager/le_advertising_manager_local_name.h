/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief       Track Usage of Local Name Token
*/

#ifndef LE_ADVERTSING_MANAGER_LOCAL_NAME_H_
#define LE_ADVERTSING_MANAGER_LOCAL_NAME_H_

#include "le_advertising_manager.h"
#include "le_advertising_manager_private.h"

/*! Reset the flags indicating the usage of local name token
*/
void LeAdvertisingManager_LocalNameReset(void);

/*! Get the data item containting the local name

    \param[in] item     The data item containing the local name
    \param[in] params   The data set, placement and completeness associated with the local name data item
    
    \return TRUE if the data item contains a local name to be used the first time
            FALSE if the data item contains a local name which was already used
*/
bool LeAdvertisingManager_LocalNameGet(le_adv_data_item_t* item, const le_adv_data_params_t* params);

/*! Register the data item containting the local name

    \param[in] item     The data item containing the local name
    \param[in] params   The data set, placement and completeness associated with the local name data item
    
*/
void LeAdvertisingManager_LocalNameRegister(const le_adv_data_item_t* item, const le_adv_data_params_t* params);

#endif /* LE_ADVERTSING_MANAGER_LOCAL_NAME_H_ */
