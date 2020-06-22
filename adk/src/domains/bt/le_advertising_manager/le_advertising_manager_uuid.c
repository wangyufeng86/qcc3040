/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Track UUID lists
*/

#include "le_advertising_manager_uuid.h"
#include "le_advertising_manager_utils.h"

#include <stdlib.h>
#include <panic.h>

#define OCTETS_IN_UUID_16   2
#define OCTETS_IN_UUID_32   4
#define OCTETS_IN_UUID_128  16

#define for_uuids_in_list(uuid, list) for(uuid = (list)->data + AD_DATA_HEADER_SIZE; uuid < (list)->data + (list)->size; uuid += (list)->octets_in_uuid)

#define for_uuids_in_item(uuid, item, octets_in_uuid) for(uuid = (item)->data + AD_DATA_HEADER_SIZE; uuid < (item)->data + (item)->size; uuid += octets_in_uuid)

typedef struct
{
    uint8* data;
    uint8  size;
    uint8  octets_in_uuid;
    le_adv_data_params_t params;
} uuid_list_t;

static ble_ad_type leAdvertisingManager_GetListType(const uuid_list_t* list, bool shortened)
{
    switch(list->octets_in_uuid)
    {
        case OCTETS_IN_UUID_16:
            return shortened ? ble_ad_type_more_uuid16 : ble_ad_type_complete_uuid16;
        
        case OCTETS_IN_UUID_32:
            return shortened ? ble_ad_type_more_uuid32 : ble_ad_type_complete_uuid32;
        
        case OCTETS_IN_UUID_128:
            return shortened ? ble_ad_type_more_uuid128 : ble_ad_type_complete_uuid128;
        
        default:
            Panic();
            return (ble_ad_type)0;
    }
}

static void leAdvertisingManager_StartList(uuid_list_t* list, const le_adv_data_params_t* params)
{
    list->params = *params;
    list->data = PanicUnlessMalloc(AD_DATA_HEADER_SIZE);
    list->size = AD_DATA_HEADER_SIZE;
    list->data[AD_DATA_TYPE_OFFSET] = leAdvertisingManager_GetListType(list, FALSE);
}

static void leAdvertisingManager_SetListShortened(uuid_list_t* list)
{
    list->data[AD_DATA_TYPE_OFFSET] = leAdvertisingManager_GetListType(list, TRUE);
}

static bool leAdvertisingManager_ExtendList(uuid_list_t* list, const uint8* item_uuid)
{
    size_t size = list->size + list->octets_in_uuid;
    uint8* data = realloc(list->data, size);
    
    if(data)
    {
        DEBUG_LOG_V_VERBOSE("leAdvertisingManager_ExtendList");
        memcpy(data + list->size, item_uuid, list->octets_in_uuid);
        list->size = size;
        list->data = data;
        list->data[AD_DATA_LENGTH_OFFSET] = size - 1;
        
        return TRUE;
    }
    return FALSE;
}

static bool leAdvertisingManager_UuidIsInList(uuid_list_t* list, const uint8* uuid)
{
    uint8* list_uuid;

    for_uuids_in_list(list_uuid, list)
    {
        if(memcmp(uuid, list_uuid, list->octets_in_uuid) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static bool leAdvertisingManager_UpdateListPlacement(uuid_list_t* list, const le_adv_data_params_t* params)
{
    if(params->placement == list->params.placement)
    {
        return TRUE;
    }
    
    if(params->placement == le_adv_data_placement_dont_care)
    {
        return TRUE;
    }
    
    if(list->params.placement == le_adv_data_placement_dont_care)
    {
        list->params.placement = params->placement;
        return TRUE;
    }
    
    return FALSE;
}

static void leAdvertisingManager_AddItemToList(uuid_list_t* list, const le_adv_data_item_t* item, const le_adv_data_params_t* params)
{
    const uint8* item_uuid;
    
    /* If an item can be skipped, skip it */
    if(params->completeness == le_adv_data_completeness_can_be_skipped)
    {
        if(list->data)
        {
            /* If there are already UUIDs in the list, skipping one means
               the list is now incomplete. Mark it as shortened */
            leAdvertisingManager_SetListShortened(list);
        }
        return;
    }
    
    if(!list->data)
    {
        leAdvertisingManager_StartList(list, params);
    }
    
    PanicFalse(leAdvertisingManager_UpdateListPlacement(list, params));
    
    for_uuids_in_item(item_uuid, item, list->octets_in_uuid)
    {
        if(!leAdvertisingManager_UuidIsInList(list, item_uuid))
        {
            leAdvertisingManager_ExtendList(list, item_uuid);
            
            /* If the item can be shortened, stop after one UUID */
            if(params->completeness == le_adv_data_completeness_can_be_shortened)
            {
                /* The list is incomplete, mark it as shortened */
                leAdvertisingManager_SetListShortened(list);
                break;
            }
        }
    }
}

static bool leAdvertisingManager_GetListAsItem(const uuid_list_t* list, le_adv_data_item_t* item, const le_adv_data_params_t* params)
{
    if(LeAdvertisingManager_ParametersMatch(&list->params, params))
    {
        item->data = list->data;
        item->size = list->size;
        return TRUE;
    }
    return FALSE;
}

static void leAdvertisingManager_ResetList(uuid_list_t* list)
{
    free(list->data);
    list->data = NULL;
    list->size = 0;
}

static uuid_list_t uuid16_list;
static uuid_list_t uuid32_list;
static uuid_list_t uuid128_list;

void LeAdvertisingManager_UuidReset(void)
{
    leAdvertisingManager_ResetList(&uuid16_list);
    uuid16_list.octets_in_uuid = OCTETS_IN_UUID_16;
    
    leAdvertisingManager_ResetList(&uuid32_list);
    uuid32_list.octets_in_uuid = OCTETS_IN_UUID_32;
    
    leAdvertisingManager_ResetList(&uuid128_list);
    uuid128_list.octets_in_uuid = OCTETS_IN_UUID_128;
}

void LeAdvertisingManager_Uuid16(const le_adv_data_item_t* item, const le_adv_data_params_t* params)
{
    leAdvertisingManager_AddItemToList(&uuid16_list, item, params);
}

bool LeAdvertisingManager_Uuid16List(le_adv_data_item_t* item, const le_adv_data_params_t* params)
{
    return leAdvertisingManager_GetListAsItem(&uuid16_list, item, params);
}

void LeAdvertisingManager_Uuid32(const le_adv_data_item_t* item, const le_adv_data_params_t* params)
{
    leAdvertisingManager_AddItemToList(&uuid32_list, item, params);
}

bool LeAdvertisingManager_Uuid32List(le_adv_data_item_t* item, const le_adv_data_params_t* params)
{
    return leAdvertisingManager_GetListAsItem(&uuid32_list, item, params);
}

void LeAdvertisingManager_Uuid128(const le_adv_data_item_t* item, const le_adv_data_params_t* params)
{
    leAdvertisingManager_AddItemToList(&uuid128_list, item, params);
}

bool LeAdvertisingManager_Uuid128List(le_adv_data_item_t* item, const le_adv_data_params_t* params)
{
    return leAdvertisingManager_GetListAsItem(&uuid128_list, item, params);
}
