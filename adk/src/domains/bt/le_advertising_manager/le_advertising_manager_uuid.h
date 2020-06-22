/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Track UUID lists
*/

#ifndef LE_ADVERTSING_MANAGER_UUID_H_
#define LE_ADVERTSING_MANAGER_UUID_H_

#include "le_advertising_manager.h"
#include "le_advertising_manager_private.h"


/*! Reset all UUID lists to be empty and free any associated memory
*/
void LeAdvertisingManager_UuidReset(void);

/*! Add a data item containting a list of UUID16s to the final UUID16 list

    \param[in] item     The data item containing a UUID16 list
    \param[in] params   The placement and completeness requirements for the data item
*/
void LeAdvertisingManager_Uuid16(const le_adv_data_item_t* item, const le_adv_data_params_t* params);

/*! Get a data item containting the final UUID16 list containing the UUID16s added with 
    LeAdvertisingManager_Uuid16

    \param[out] item    Data item to be populated with the final UUID16 list
    \param[in]  params  The placement and completeness requirements for the data item
    
    \returns TRUE if the item was populated with the final UUID16 list, FALSE if there
             were no UUID16s added to the final list.
*/
bool LeAdvertisingManager_Uuid16List(le_adv_data_item_t* item, const le_adv_data_params_t* params);

/*! Add a data item containting a list of UUID32s to the final UUID32 list

    \param[in] item     The data item containing a UUID32 list
    \param[in] params   The placement and completeness requirements for the data item
*/
void LeAdvertisingManager_Uuid32(const le_adv_data_item_t* item, const le_adv_data_params_t* params);

/*! Get a data item containting the final UUID32s list containing the UUID32s added with 
    LeAdvertisingManager_Uuid32

    \param[out] item    Data item to be populated with the final UUID32s list
    \param[in]  params  The placement and completeness requirements for the data item
    
    \returns TRUE if the item was populated with the final UUID32s list, FALSE if there
             were no UUID32s added to the final list.
*/
bool LeAdvertisingManager_Uuid32List(le_adv_data_item_t* item, const le_adv_data_params_t* params);

/*! Add a data item containting a list of UUID128s to the final UUID128 list

    \param[in] item     The data item containing a UUID128 list
    \param[in] params   The placement and completeness requirements for the data item
*/
void LeAdvertisingManager_Uuid128(const le_adv_data_item_t* item, const le_adv_data_params_t* params);

/*! Get a data item containting the final UUID128 list containing the UUID128s added with 
    LeAdvertisingManager_Uuid128

    \param[out] item    Data item to be populated with the final UUID128 list
    \param[in]  params  The placement and completeness requirements for the data item
    
    \returns TRUE if the item was populated with the final UUID128 list, FALSE if there
             were no UUID128s added to the final list.
*/
bool LeAdvertisingManager_Uuid128List(le_adv_data_item_t* item, const le_adv_data_params_t* params);

#endif /* LE_ADVERTSING_MANAGER_UUID_H_ */
