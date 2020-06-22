/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Manage providers of advertising/scan response data to LE advertising manager
*/

#include "le_advertising_manager_clients.h"

#include <panic.h>

/* Local database to store the callback information clients register */
static struct _le_adv_mgr_register database[MAX_NUMBER_OF_CLIENTS];

/******************************************************************************/
bool leAdvertisingManager_ClientHandleIsValid(const struct _le_adv_mgr_register * handle)
{
    for(int i = 0; i < MAX_NUMBER_OF_CLIENTS; i++)
    {
        if(&database[i] == handle)
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

/******************************************************************************/
void leAdvertisingManager_ClientsInit(void)
{
    for(int i=0; i<MAX_NUMBER_OF_CLIENTS; i++)
    {
        database[i].task = NULL;
        database[i].callback = NULL;
    }
}

/******************************************************************************/
le_adv_mgr_register_handle LeAdvertisingManager_NewClient(Task task, const le_adv_data_callback_t * const callback)
{
    int i;
    
    if( (NULL == callback) || (NULL == callback->GetNumberOfItems) || (NULL == callback->GetItem) )
        return NULL;
    
    for(i = 0; i < MAX_NUMBER_OF_CLIENTS; i++)
    {
        if(database[i].callback)
        {
            if((callback->GetNumberOfItems == database[i].callback->GetNumberOfItems) ||
               (callback->GetItem == database[i].callback->GetItem))
                return NULL;
            else
                continue;
        }
        database[i].callback = callback;
        database[i].task = task;
        break;
    }

    if(MAX_NUMBER_OF_CLIENTS == i)
    {
        DEBUG_LOG_WARN("LeAdvertisingManager_Register Failure, Reached Maximum Number of Clients");
        return NULL;
    }
    
    return &database[i];
}

/******************************************************************************/
le_adv_mgr_register_handle leAdvertisingManager_HeadClient(le_adv_mgr_client_iterator_t* iterator)
{
    if(iterator)
    {
        iterator->client_handle = &database[0];
    }
    
    return &database[0];
}

/******************************************************************************/
le_adv_mgr_register_handle leAdvertisingManager_NextClient(le_adv_mgr_client_iterator_t* iterator)
{
    le_adv_mgr_register_handle client_handle = NULL;
    
    PanicNull(iterator);
    
    iterator->client_handle++;
    
    if(iterator->client_handle < &database[MAX_NUMBER_OF_CLIENTS])
    {
        client_handle = iterator->client_handle;
    }
    
    return client_handle;
}

/******************************************************************************/
bool leAdvertisingManager_ClientListIsEmpty(void)
{
    if(NULL == database[0].callback)
    {
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************/
size_t leAdvertisingManager_ClientNumItems(le_adv_mgr_register_handle client_handle, const le_adv_data_params_t* params)
{
    if(!client_handle->callback)
    {
        return 0;
    }
    
    return client_handle->callback->GetNumberOfItems(params);
}
