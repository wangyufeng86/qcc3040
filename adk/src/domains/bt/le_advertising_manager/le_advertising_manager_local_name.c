/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Track Usage of Local Name Token
*/

#include "le_advertising_manager_local_name.h"
#include "le_advertising_manager_utils.h"

#include <stdlib.h>
#include <panic.h>

static le_adv_data_item_t local_name;
static le_adv_data_params_t data_params;

/*! Local function to check if the requested local name data item matches the existing one */
static bool leAdvertisingManager_LocalNameDataIsMatched(const le_adv_data_item_t* item)
{   
    DEBUG_LOG("leAdvertisingManager_LocalNameDataIsMatched");
    
    PanicNull((void*)local_name.data);
    PanicZero(local_name.size);
    
    if(item->size != local_name.size)
    {
        DEBUG_LOG_LEVEL_2("leAdvertisingManager_LocalNameDataIsMatched Info, Local name size is different");
        
        return FALSE;
    }
    
    int result = memcmp(item->data, local_name.data, local_name.size);

    if(result)
    {
        DEBUG_LOG_LEVEL_2("leAdvertisingManager_LocalNameDataIsMatched Info, Local name data is different, data compare result is %x", result);
        
        return FALSE;
    }
    
    return TRUE;
}

/*! Reset the flags indicating the usage of local name token
*/
void LeAdvertisingManager_LocalNameReset(void)
{
    DEBUG_LOG_VERBOSE("LeAdvertisingManager_LocalNameReset");
    
    local_name.size = 0;
    local_name.data = NULL;
}

/*! Get the data item containting the local name

    \param[in/out] item     The data item containing the local name
    \param[in] params   The data set, placement and completeness associated with the local name data item
    
    \return TRUE if there is a data item containing the local name with the matching data parameters
            FALSE if there is no data item containing the local name with the matching data parameters
*/
bool LeAdvertisingManager_LocalNameGet(le_adv_data_item_t* item, const le_adv_data_params_t* params)
{    
    PanicNull((void*)item);
    PanicNull((void*)params);
    
    DEBUG_LOG_V_VERBOSE("leAdvertisingManager_LocalNameGet, params is %x", item, params);
        
    if((local_name.size) && (local_name.data) )
    {
        if(LeAdvertisingManager_ParametersMatch(&data_params, params) )
        {
            DEBUG_LOG_LEVEL_2("leAdvertisingManager_LocalNameGet Info, There is an existing data item with local name, size is %x, data is %x", local_name.size, local_name.data);

            *item = local_name;
        }
        else
        {
            DEBUG_LOG_LEVEL_2("leAdvertisingManager_LocalNameGet Info, Parameters mismatch");
            
            return FALSE;
        }
    }
    else
    {
        DEBUG_LOG_LEVEL_2("leAdvertisingManager_LocalNameGet Info, No existing data item with local name");
        
        return FALSE;
    }
    
    return TRUE;
}

/*! Register the data item containting the local name

    \param[in] item     The data item containing the local name
    \param[in] params   The data set, placement and completeness associated with the local name data item
    
*/
void LeAdvertisingManager_LocalNameRegister(const le_adv_data_item_t* item, const le_adv_data_params_t* params)
{
    PanicNull((void*)item);
    PanicNull((void*)params);
    
    DEBUG_LOG("leAdvertisingManager_LocalNameRegister, item is %x, params is %x", item, params);
        
    if( (local_name.size) && (local_name.data) )
    {
        DEBUG_LOG_LEVEL_2("leAdvertisingManager_LocalNameRegister Info, Local name already registered");
        
        if(leAdvertisingManager_LocalNameDataIsMatched(item))
        {            
            DEBUG_LOG_LEVEL_2("leAdvertisingManager_LocalNameRegister Info, Requested local name matched with the existing one");
        }
        else
        {
            DEBUG_LOG_LEVEL_1("leAdvertisingManager_LocalNameRegister Failure, Requested local name not matched with the existing one");
            
            Panic();
            
        }        
    }
    else
    {
        DEBUG_LOG_LEVEL_2("leAdvertisingManager_LocalNameRegister Info, No existing data item with local name, registering local name with size %x, data %x", item->size, item->data);
        
        local_name.size = item->size;
        local_name.data = item->data;
        
        data_params.data_set = params->data_set;
        data_params.completeness = params->completeness;
        data_params.placement = params->placement;
        
    }    
}
