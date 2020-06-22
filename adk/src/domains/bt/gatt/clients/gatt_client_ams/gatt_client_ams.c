/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application support for GATT AMS client
*/

#include "gatt_client.h"
#include "gatt_client_protected.h"
#include <gatt_ams_client.h>
#include <panic.h>
#include <logging.h>

static GAMS *ams_data = NULL;

/* Always disabled */
#define DISABLE_REMOTE_CLIENT_NOTIFICATIONS     FALSE
#define DISABLE_ENTITY_UPDATE_NOTIFICATIONS     FALSE

#define DEBUG_GATT_CLIENT_AMS                   "Gatt Client AMS:"


/* Forward function declarations */
static void GattAmsClient_MsgHandler(Task task, MessageId id, Message message);

/* Create a AMS gatt client instance */
INIT_SIMPLE_GATT_CLIENT_INSTANCE(gatt_ams_client,
                                GattAmsClient_MsgHandler,
                                gatt_uuid128, 0x89D3502Bu, 0x0F36433Au, 0x8EF4C502u, 0xAD55F8DCu,
                                CONTINUE_SERVICE_DISCOVERY,
                                GattAmsInit,
                                GattAmsDestroy,
                                ams_data);

static void GattAmsClient_MsgHandler (Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
        case GATT_AMS_CLIENT_INIT_CFM:
            if(GattClient_Protected_GetStatus(&gatt_ams_client) == gatt_client_status_service_attached)
            {
                GattAmsSetRemoteCommandNotificationEnableRequest(NULL, ams_data, DISABLE_REMOTE_CLIENT_NOTIFICATIONS);
                GattAmsSetEntityUpdateNotificationEnableRequest(NULL, ams_data, DISABLE_ENTITY_UPDATE_NOTIFICATIONS);
            }
            break;

        default:
            DEBUG_LOG(DEBUG_GATT_CLIENT_AMS "Unhandled AMS msg [0x%04x]\n", id);
            break;
    }
}
