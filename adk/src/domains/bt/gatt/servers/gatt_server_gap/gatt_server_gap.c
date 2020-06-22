/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Implementation of the GAP Server module.
*/

#include "gatt_server_gap.h"

#include "gatt_handler_db_if.h"
#include "le_advertising_manager.h"
#include "local_name.h"

#include <gatt_gap_server.h>
#include <logging.h>

#include <panic.h>
#include <stdlib.h>

#define GAP_ADVERT_FLAGS        (BLE_FLAGS_GENERAL_DISCOVERABLE_MODE | BLE_FLAGS_DUAL_CONTROLLER | BLE_FLAGS_DUAL_HOST)
#define GAP_ADVERT_FLAGS_LENGTH 3

static const uint8 gap_adv_flags_data[GAP_ADVERT_FLAGS_LENGTH] =
{
    GAP_ADVERT_FLAGS_LENGTH - 1,
    ble_ad_type_flags,
    GAP_ADVERT_FLAGS
};

static le_adv_data_item_t gap_name_item = {0};
static const le_adv_data_item_t gap_flags_item = {.data = gap_adv_flags_data, .size = sizeof(gap_adv_flags_data)};
static const le_adv_data_item_t gap_empty_item = {0};

/*! Structure holding information for the application handling of GAP Server */
typedef struct
{
    /*! Task for handling GAP related messages */
    TaskData    gap_task;
    /*! GAP server library data */
    GGAPS       gap_server;
    /*! Complete local name flag */
    bool        complete_local_name;
} gattServerGapData;

static gattServerGapData gatt_server_gap = {0};

/*! Get pointer to the main GAP server Task */
#define GetGattServerGapTask() (&gatt_server_gap.gap_task)

/*! Get pointer to the GAP server data passed to the library */
#define GetGattServerGapGgaps() (&gatt_server_gap.gap_server)

/*! Get the complete local name flag */
#define GetGattServerGapCompleteName() (gatt_server_gap.complete_local_name)


static bool gattServerGap_IsNameReturned(const le_adv_data_params_t * params)
{
    if((params->data_set != le_adv_data_set_handset_identifiable) &&
       (params->data_set != le_adv_data_set_handset_unidentifiable))
    {
        return FALSE;
    }

    if(params->placement != le_adv_data_placement_dont_care)
    {
        return FALSE;
    }

    if(GetGattServerGapCompleteName())
    {
        if(params->completeness != le_adv_data_completeness_full)
        {
            return FALSE;
        }
    }
    else
    {
        if(params->completeness != le_adv_data_completeness_can_be_shortened)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static bool gattServerGap_IsFlagsReturned(const le_adv_data_params_t * params)
{
    if(params->data_set == le_adv_data_set_peer)
    {
        return FALSE;
    }

    if(params->placement != le_adv_data_placement_advert)
    {
        return FALSE;
    }

    if(params->completeness != le_adv_data_completeness_full)
    {
        return FALSE;
    }

    return TRUE;
}

static unsigned gattServerGap_NumberOfAdvItems(const le_adv_data_params_t * params)
{
    unsigned count = 0;

    if(gattServerGap_IsNameReturned(params))
    {
        count++;
    }

    if(gattServerGap_IsFlagsReturned(params))
    {
        count++;
    }

    return count;
}

static le_adv_data_item_t gattServerGap_GetAdvNameItem(void)
{
    DEBUG_LOG("gattServerGap_GetAdvNameItem gap_name_item.data:%d", gap_name_item.data);

    if(gap_name_item.data == NULL)
    {
        uint16 name_len;
        const char* name = (const char*)LocalName_GetPrefixedName(&name_len);
        PanicNull((void*)name);

        uint16 data_len = name_len + AD_DATA_HEADER_SIZE;
        uint8* data = PanicUnlessMalloc(data_len);

        data[AD_DATA_LENGTH_OFFSET] = name_len + 1;
        data[AD_DATA_TYPE_OFFSET] = ble_ad_type_complete_local_name;
        memcpy(&data[AD_DATA_HEADER_SIZE], name, name_len);

        gap_name_item.size = data_len;
        gap_name_item.data = data;
    }

    return gap_name_item;
}

static le_adv_data_item_t gattServerGap_GetAdvFlagsItem(void)
{
    return gap_flags_item;
}

static le_adv_data_item_t gattServerGap_GetAdvDataItems(const le_adv_data_params_t * params, unsigned index)
{
    PanicFalse(index == 0);

    DEBUG_LOG("gattServerGap_GetAdvDataItems data_set: %d, completeness: %d,  placement:%d",
               params->data_set, params->completeness, params->placement);

    if(gattServerGap_IsNameReturned(params))
    {
        return gattServerGap_GetAdvNameItem();
    }

    if(gattServerGap_IsFlagsReturned(params))
    {
        return gattServerGap_GetAdvFlagsItem();
    }

    Panic();
    return gap_empty_item;
}

static void gattServerGap_ReleaseAdvDataItems(const le_adv_data_params_t * params)
{
    DEBUG_LOG("gattServerGap_ReleaseAdvDataItems data_set: %d, completeness: %d,  placement:%d",
               params->data_set, params->completeness, params->placement);

    if(gattServerGap_IsNameReturned(params) && gap_name_item.data)
    {
        free((void*)gap_name_item.data);
        gap_name_item.data = NULL;
        gap_name_item.size = 0;
    }
}

static const le_adv_data_callback_t gatt_gap_le_advert_callback =
{
    .GetNumberOfItems = gattServerGap_NumberOfAdvItems,
    .GetItem = gattServerGap_GetAdvDataItems,
    .ReleaseItems = gattServerGap_ReleaseAdvDataItems
};


static void gattServerGap_HandleReadDeviceNameInd(const GATT_GAP_SERVER_READ_DEVICE_NAME_IND_T *ind)
{
    uint16 name_len;
    const uint8 *name = LocalName_GetPrefixedName(&name_len);
    PanicNull((void*)name);

    /* The indication can request a portion of our name by specifying the start offset */
    if (ind->name_offset)
    {
        /* Check that we haven't been asked for an entry off the end of the name */

        if (ind->name_offset >= name_len)
        {
            name_len = 0;
            name = NULL;
        /*  \todo return gatt_status_invalid_offset  */
        }
        else
        {
            name_len -= ind->name_offset;
            name += ind->name_offset;
        /*  \todo return gatt_status_success  */
        }
    }

    PanicFalse(GetGattServerGapGgaps() == ind->gap_server);

    GattGapServerReadDeviceNameResponse(GetGattServerGapGgaps(), ind->cid,
                                        name_len, (uint8*)name);
}

static void gattServerGap_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG("gattServerGap_MessageHandler id:%d 0x%x", id, id);

    switch (id)
    {
        case GATT_GAP_SERVER_READ_DEVICE_NAME_IND:
            gattServerGap_HandleReadDeviceNameInd((const GATT_GAP_SERVER_READ_DEVICE_NAME_IND_T*)message);
            break;

        default:
            DEBUG_LOG("gattServerGap_MessageHandler. Unhandled message id:0x%x", id);
            break;
    }
}

static void gattServerGap_SetupAdvertising(void)
{
    LeAdvertisingManager_Register(NULL, &gatt_gap_le_advert_callback);
}

static void gattServerGap_init(void)
{
    memset(&gatt_server_gap, 0, sizeof(gatt_server_gap));

    gatt_server_gap.gap_task.handler = gattServerGap_MessageHandler;

    if (GattGapServerInit(GetGattServerGapGgaps(), GetGattServerGapTask(),
                                    HANDLE_GAP_SERVICE, HANDLE_GAP_SERVICE_END)
                        != gatt_gap_server_status_success)
    {
        DEBUG_LOG("gattServerGap_init Server failed");
        Panic();
    }

    gattServerGap_SetupAdvertising();
}


/*****************************************************************************/
bool GattServerGap_Init(Task init_task)
{
    UNUSED(init_task);

    gattServerGap_init();

    DEBUG_LOG("GattServerGap_Init. Server initialised");

    return TRUE;
}

void GattServerGap_UseCompleteLocalName(bool complete)
{
    gatt_server_gap.complete_local_name = complete;
}
