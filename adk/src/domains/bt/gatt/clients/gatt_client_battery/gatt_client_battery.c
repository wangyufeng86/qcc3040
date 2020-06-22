/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application support for GATT battery client
*/

#include "gatt_client.h"
#include "gatt_client_protected.h"
#include <gatt_battery_client.h>
#include <panic.h>
#include <logging.h>

#define DEBUG_GATT_CLIENT_BATTERY                   "Gatt Client Bat:"

/* Forward declaration of private functions */
static void GattBatteryClient_MsgHandler (Task task, MessageId id, Message message);
typedef struct
{
    GBASC*  lib_info;
    uint8   remote_level;
}battery_data_t;
static battery_data_t battery_data[2] = { 0 };

INIT_SIMPLE_GATT_CLIENT_INSTANCE(gatt_battery_client,
                                 GattBatteryClient_MsgHandler,
                                 gatt_uuid16, 0x180F,0,0,0,
                                 CONTINUE_SERVICE_DISCOVERY,
                                 GattBatteryClientInit,
                                 GattBatteryClientDestroy,
                                 battery_data[0].lib_info);

INIT_SIMPLE_GATT_CLIENT_INSTANCE(gatt_battery_client_2,
                                GattBatteryClient_MsgHandler,
                                gatt_uuid16, 0x180F, 0, 0, 0,
                                CONTINUE_SERVICE_DISCOVERY,
                                GattBatteryClientInit,
                                GattBatteryClientDestroy,
                                battery_data[1].lib_info);

//static uint16 gattBatteryClientFindCid(const GBASC *gbasc)
//{
//    gatt_client_t *client = NULL;

//    if(gbasc)
//    {
//        if(gbasc == (GBASC*)*gatt_battery_client.client_specific_data.lib_data_dynamic)
//        {
//            client = &gatt_battery_client;
//        }
//        else if(gbasc == (GBASC*)*gatt_battery_client_2.client_specific_data.lib_data_dynamic)
//        {
//            client = &gatt_battery_client_2;
//        }
//        else
//        {
//            /* Shouldn't be possible */
//        }
//    }

//    /* Shouldn't be possible */
//    PanicFalse(client);

//    return GattClient_Protected_GetConnectionID(client);
//}

static void SetCachedLevel(uint8 level, const GBASC *gbasc)
{
    uint8* remote_level = NULL;
    if(gbasc)
    {
        if(gbasc == battery_data[0].lib_info)
        {
            remote_level = &battery_data[0].remote_level;
        }
        else if(gbasc == battery_data[1].lib_info)
        {
            remote_level = &battery_data[1].remote_level;
        }
        else
        {
            /* Shouldn't be possible */
        }
    }

    /* Shouldn't be possible */
    PanicFalse(remote_level);

    *remote_level = level;
}

static void gattBatteryInitCfm(const GATT_BATTERY_CLIENT_INIT_CFM_T *cfm)
{
    DEBUG_LOG("GATT_BATTERY_CLIENT_INIT_CFM status[%u]\n", cfm->status);

    if((GattClient_Protected_GetStatus(&gatt_battery_client) == gatt_client_status_service_attached) &&
            (cfm->status == gatt_battery_client_status_success) )
    {
        /* Read battery level */
        GattBatteryClientReadLevelRequest(cfm->battery_client);
        /* Set notifications */
        GattBatteryClientSetNotificationEnableRequest(cfm->battery_client, TRUE);
    }
}

static void gattBatteryReadLevelCfm(const GATT_BATTERY_CLIENT_READ_LEVEL_CFM_T *cfm)
{
    DEBUG_LOG("GATT_BATTERY_CLIENT_READ_LEVEL_CFM status[%u] level[%u]\n", cfm->status,cfm->battery_level);

    if (cfm->status == gatt_battery_client_status_success)
    {
        SetCachedLevel(cfm->battery_level, cfm->battery_client);
    }
}

static void GattBatteryClient_MsgHandler (Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
        case GATT_BATTERY_CLIENT_INIT_CFM:
        {
            gattBatteryInitCfm((GATT_BATTERY_CLIENT_INIT_CFM_T*)message);
        }
        break;
        case GATT_BATTERY_CLIENT_READ_LEVEL_CFM:
        {
            gattBatteryReadLevelCfm((GATT_BATTERY_CLIENT_READ_LEVEL_CFM_T*)message);
        }
        break;
        default:
            break;
    }
}
