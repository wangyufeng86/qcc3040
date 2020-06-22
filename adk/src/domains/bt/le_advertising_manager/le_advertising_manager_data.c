/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Manage execution of callbacks to construct adverts and scan response
*/

#include "le_advertising_manager_data.h"
#include "le_advertising_manager_clients.h"
#include "le_advertising_manager_uuid.h"
#include "le_advertising_manager_local_name.h"

#include <stdlib.h>
#include <panic.h>

#define for_all_data_sets(params) for((params)->data_set = le_adv_data_set_handset_identifiable; (params)->data_set <= le_adv_data_set_peer; ((params)->data_set) <<= 1)

#define for_all_completeness(params) for((params)->completeness = le_adv_data_completeness_full; (params)->completeness <= le_adv_data_completeness_can_be_skipped; (params)->completeness++)

#define for_all_placements(params) for((params)->placement = le_adv_data_placement_advert; (params)->placement <= le_adv_data_placement_dont_care; (params)->placement++)

#define for_all_params_in_set(params, set) for_all_completeness(params) for_all_placements(params) for_all_data_sets(params) if(set & (params)->data_set)

typedef struct 
{
    uint8  data[MAX_AD_DATA_SIZE_IN_OCTETS];
    uint8* head;
    uint8  space;
} le_adv_data_packet_t;

static void leAdvertisingManager_DebugDataItems(const uint8 size, const uint8 * data)
{
    if(size && data)
    {
        for(int i=0;i<size;i++)
            DEBUG_LOG_V_VERBOSE("leAdvertisingManager_DebugDataItems [%d] is 0x%x", i, data[i]);
    }
}

static le_adv_data_packet_t* leAdvertisingManager_CreatePacket(void)
{
    le_adv_data_packet_t* packet = PanicUnlessMalloc(sizeof(le_adv_data_packet_t));
    packet->head = packet->data;
    packet->space = MAX_AD_DATA_SIZE_IN_OCTETS;
    return packet;
}

static bool leAdvertisingManager_AddDataItemToPacket(le_adv_data_packet_t* packet, const le_adv_data_item_t* item)
{
    PanicNull(packet);
    PanicNull((le_adv_data_item_t*)item);
    
    if(item->size > packet->space)
    {
        return FALSE;
    }
    
    if(item->size)
    {
        memcpy(packet->head, item->data, item->size);
        packet->head  += item->size;
        packet->space -= item->size;
    }
    
    return TRUE;
}

static void leAdvertisingManager_DestroyPacket(le_adv_data_packet_t* packet)
{
    free(packet);
}

static unsigned leAdvertisingManager_GetPacketSize(le_adv_data_packet_t* packet)
{
    return (packet->head - packet->data);
}

static le_adv_data_packet_t* advert;
static le_adv_data_packet_t* scan_rsp;

static bool leAdvertisingManager_AddDataItemToAdvert(const le_adv_data_item_t* item)
{
    DEBUG_LOG_VERBOSE("leAdvertisingManager_AddDataItemToAdvert");
    return leAdvertisingManager_AddDataItemToPacket(advert, item);
}

static bool leAdvertisingManager_AddDataItemToScanRsp(const le_adv_data_item_t* item)
{
    DEBUG_LOG_VERBOSE("leAdvertisingManager_AddDataItemToScanRsp");
    return leAdvertisingManager_AddDataItemToPacket(scan_rsp, item);
}

static void leAdvertisingManager_AddDataItem(const le_adv_data_item_t* item, const le_adv_data_params_t* params)
{
    bool added;
    
    switch(params->placement)
    {
        case le_adv_data_placement_advert:
        {
            added = leAdvertisingManager_AddDataItemToAdvert(item);
            break;
        }
        
        case le_adv_data_placement_scan_response:
        {
            added = leAdvertisingManager_AddDataItemToScanRsp(item);
            break;
        }
        
        case le_adv_data_placement_dont_care:
        {
            added = leAdvertisingManager_AddDataItemToAdvert(item);
            if(!added)
            {
                added = leAdvertisingManager_AddDataItemToScanRsp(item);
            }
            break;
        }
        
        default:
        {
            added = FALSE;
            DEBUG_LOG_ERROR("leAdvertisingManager_AddDataItem, Unrecognised item placement attribute %d", params->placement);
            Panic();
            break;
        }
    }
    
    if(params->completeness != le_adv_data_completeness_can_be_skipped)
    {
        DEBUG_LOG_VERBOSE("leAdvertisingManager_AddDataItem, Cannot skip the item, item ptr is 0x%x", item);

        if(item)
        {
            leAdvertisingManager_DebugDataItems(item->size, item->data);
        }

        PanicFalse(added);
    }
}

static void leAdvertisingManager_ProcessDataItem(const le_adv_data_item_t* item, const le_adv_data_params_t* params)
{
    if(item->data && item->size)
    {
        uint8 data_type = item->data[AD_DATA_TYPE_OFFSET];
        
        DEBUG_LOG_VERBOSE("leAdvertisingManager_ProcessDataItem %d", data_type);
        
        switch(data_type)
        {
            case ble_ad_type_complete_uuid16:
            {
                LeAdvertisingManager_Uuid16(item, params);
                break;
            }
            
            case ble_ad_type_complete_uuid32:
            {
                LeAdvertisingManager_Uuid32(item, params);
                break;
            }
            
            case ble_ad_type_complete_uuid128:
            {
                LeAdvertisingManager_Uuid128(item, params);
                break;
            }
            
            case ble_ad_type_complete_local_name:
            {
                LeAdvertisingManager_LocalNameRegister(item, params);
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

static void leAdvertisingManager_BuildDataItem(const le_adv_data_item_t* item, const le_adv_data_params_t* params)
{
    if(item->data && item->size)
    {
        uint8 data_type = item->data[AD_DATA_TYPE_OFFSET];
        
        switch(data_type)
        {
            case ble_ad_type_complete_uuid16:
            case ble_ad_type_complete_uuid32:
            case ble_ad_type_complete_uuid128:
            case ble_ad_type_complete_local_name:
            {
                break;
            }

            default:
            {
                DEBUG_LOG("leAdvertisingManager_BuildDataItem %d", data_type);
                leAdvertisingManager_AddDataItem(item, params);
                break;
            }
        }
    }
}

static void leAdvertisingManager_ProcessClientData(le_adv_mgr_register_handle client_handle, const le_adv_data_params_t* params)
{
    size_t num_items = leAdvertisingManager_ClientNumItems(client_handle, params);
    
    if(num_items)
    {
        DEBUG_LOG_V_VERBOSE("leAdvertisingManager_ProcessClientData num_items %d", num_items);
        
        for(unsigned i = 0; i < num_items; i++)
        {
            le_adv_data_item_t item = client_handle->callback->GetItem(params, i);
            leAdvertisingManager_ProcessDataItem(&item, params);
        }
    }
}

static void leAdvertisingManager_BuildClientData(le_adv_mgr_register_handle client_handle, const le_adv_data_params_t* params)
{
    size_t num_items = leAdvertisingManager_ClientNumItems(client_handle, params);
    
    if(num_items)
    {
        DEBUG_LOG_V_VERBOSE("leAdvertisingManager_ProcessClientData num_items %d", num_items);
        
        for(unsigned i = 0; i < num_items; i++)
        {
            le_adv_data_item_t item = client_handle->callback->GetItem(params, i);
            leAdvertisingManager_BuildDataItem(&item, params);
        }
    }
}

static void leAdvertisingManager_ClearClientData(le_adv_mgr_register_handle client_handle, const le_adv_data_params_t* params)
{
    size_t num_items = leAdvertisingManager_ClientNumItems(client_handle, params);
    
    if(num_items)
    {
        client_handle->callback->ReleaseItems(params);
    }
}

static void leAdvertisingManager_ProcessAllClientsData(const le_adv_data_params_t* params)
{
    le_adv_mgr_client_iterator_t iterator;
    le_adv_mgr_register_handle client_handle = leAdvertisingManager_HeadClient(&iterator);
    
    while(client_handle)
    {
        leAdvertisingManager_ProcessClientData(client_handle, params);
        client_handle = leAdvertisingManager_NextClient(&iterator);
    }
}

static void leAdvertisingManager_BuildAllClientsData(const le_adv_data_params_t* params)
{
    le_adv_mgr_client_iterator_t iterator;
    le_adv_mgr_register_handle client_handle = leAdvertisingManager_HeadClient(&iterator);
    
    while(client_handle)
    {
        leAdvertisingManager_BuildClientData(client_handle, params);
        client_handle = leAdvertisingManager_NextClient(&iterator);
    }
}

static void leAdvertisingManager_ClearAllClientsData(const le_adv_data_params_t* params)
{
    le_adv_mgr_client_iterator_t iterator;
    le_adv_mgr_register_handle client_handle = leAdvertisingManager_HeadClient(&iterator);
    
    while(client_handle)
    {
        leAdvertisingManager_ClearClientData(client_handle, params);
        client_handle = leAdvertisingManager_NextClient(&iterator);
    }
}

static void leAdvertisingManager_BuildLocalNameData(const le_adv_data_params_t* params)
{
    le_adv_data_item_t item;
    
    if(LeAdvertisingManager_LocalNameGet(&item, params))
    {
        leAdvertisingManager_AddDataItem(&item, params);
    }
}

static void leAdvertisingManager_BuildUuidData(const le_adv_data_params_t* params)
{
    le_adv_data_item_t item;
    
    if(LeAdvertisingManager_Uuid16List(&item, params))
    {
        leAdvertisingManager_AddDataItem(&item, params);
    }
    if(LeAdvertisingManager_Uuid32List(&item, params))
    {
        leAdvertisingManager_AddDataItem(&item, params);
    }
    if(LeAdvertisingManager_Uuid128List(&item, params))
    {
        leAdvertisingManager_AddDataItem(&item, params);
    }
}

bool leAdvertisingManager_BuildData(le_adv_data_set_t set)
{
    le_adv_data_params_t params;
    
    advert = leAdvertisingManager_CreatePacket();
    scan_rsp = leAdvertisingManager_CreatePacket();
    
    LeAdvertisingManager_UuidReset();
    LeAdvertisingManager_LocalNameReset();
    
    for_all_params_in_set(&params, set)
    {
        leAdvertisingManager_ProcessAllClientsData(&params);
    }
    
    for_all_params_in_set(&params, set)
    {
        leAdvertisingManager_BuildAllClientsData(&params);
        leAdvertisingManager_BuildLocalNameData(&params);
        leAdvertisingManager_BuildUuidData(&params);
    }
    
    if(leAdvertisingManager_GetPacketSize(advert) || leAdvertisingManager_GetPacketSize(scan_rsp))
    {
        return TRUE;
    }
    return FALSE;
}

void leAdvertisingManager_SetupScanResponseData(void)
{
    uint8 size_scan_rsp = leAdvertisingManager_GetPacketSize(scan_rsp);
    uint8* scan_rsp_start = size_scan_rsp ? scan_rsp->data : NULL;
    
    DEBUG_LOG("leAdvertisingManager_SetupScanResponseData, Size is %d", size_scan_rsp);

    leAdvertisingManager_DebugDataItems(size_scan_rsp, scan_rsp_start);

    ConnectionDmBleSetScanResponseDataReq(size_scan_rsp, scan_rsp_start);
}

void leAdvertisingManager_SetupAdvertData(void)
{
    uint8 size_advert = leAdvertisingManager_GetPacketSize(advert);
    uint8* advert_start = size_advert ? advert->data : NULL;
    
    DEBUG_LOG_VERBOSE("leAdvertisingManager_SetupAdvertData, Size is %d", size_advert);

    leAdvertisingManager_DebugDataItems(size_advert, advert_start);

    ConnectionDmBleSetAdvertisingDataReq(size_advert, advert_start);
}

void leAdvertisingManager_ClearData(le_adv_data_set_t set)
{
    le_adv_data_params_t params;
    
    leAdvertisingManager_DestroyPacket(scan_rsp);
    scan_rsp = NULL;
    
    leAdvertisingManager_DestroyPacket(advert);
    advert = NULL;
    
    LeAdvertisingManager_UuidReset();
    
    for_all_params_in_set(&params, set)
    {
        leAdvertisingManager_ClearAllClientsData(&params);
    }
}
