/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_client_msg_handler.h"
#include "gatt_ams_client.h"
#include "gatt_ams_client_private.h"
#include "gatt_ams_client_discover.h"
#include "gatt_ams_client_write.h"
#include "gatt_ams_client_read.h"
#include "gatt_ams_client_notification.h"
#include "gatt_ams_client_external_msg_send.h"
#include <gatt_manager.h>
#include <stdlib.h>

static uint16 getHandleFromPendingCmdId(const GAMS *ams, pending_cmd_ids_t cmd_id)
{
    switch (cmd_id)
    {
        case ams_pending_write_remote_command:
            return ams->remote_command_handle;
        case ams_pending_write_entity_update:
            return ams->entity_update_handle;
        case ams_pending_write_entity_attribute:
        case ams_pending_read_entity_attribute:
            return ams->entity_attribute_handle;
        default:
            PANIC(("AMS: Pending command id not mapped to a handle\n"));
            return GATT_AMS_INVALID_HANDLE;
    }
}

static uint8 getCharacteristicIdFromPendingCmdId(pending_cmd_ids_t cmd_id)
{
    switch (cmd_id)
    {
        case ams_pending_set_remote_command_notification:
            return GATT_AMS_CLIENT_REMOTE_COMMAND;
        case ams_pending_set_entity_update_notification:
            return GATT_AMS_CLIENT_ENTITY_UPDATE;
        default:
            PANIC(("AMS: Pending command id not mapped to a characteristic id\n"));
            return 0;
    }
}

static void handleSetCharacteristicNotificationMsg(GAMS *ams, const MSG_SET_CHARACTERISTIC_NOTIFICATION_T *msg)
{
    ams->task_pending_cfm = msg->task_pending_cfm;
    ams->pending_cmd      = msg->cmd_to_set_as_pending;

    amsSetNotificationRequest(ams, msg->notifications_enable, getCharacteristicIdFromPendingCmdId(msg->cmd_to_set_as_pending));
}

static void handleWriteCharacteristicMsg(GAMS *ams, const MSG_WRITE_CHARACTERISTIC_T *msg)
{
    uint16 handle = getHandleFromPendingCmdId(ams, msg->cmd_to_set_as_pending);

    ams->task_pending_cfm = msg->task_pending_cfm;
    ams->pending_cmd      = msg->cmd_to_set_as_pending;

    if (CHECK_VALID_HANDLE(handle))
        gattAmsWriteCharacteristic(ams, handle, msg->size_command_data, msg->command_data);
    else
        gattAmsSendWriteCharacteristicResponse(ams, gatt_status_request_not_supported);
}

static void handleReadCharacteristicMsg(GAMS *ams, const MSG_READ_CHARACTERISTIC_T *msg)
{
    uint16 handle = getHandleFromPendingCmdId(ams, msg->cmd_to_set_as_pending);

    ams->task_pending_cfm = msg->task_pending_cfm;
    ams->pending_cmd      = msg->cmd_to_set_as_pending;

    if(CHECK_VALID_HANDLE(handle))
        gattAmsReadCharacteristic(ams, handle);
    else
        gattAmsSendReadCharacteristicResponse(ams, gatt_status_request_not_supported, 0, NULL);
}

static void handleGattManagerMsg(GAMS *ams, MessageId id, Message msg)
{
    switch (id)
    {
        case GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM:
            handleAmsDiscoverAllCharacteristicsResp(ams, (GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *) msg);
            break;

        case GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM:
            handleAmsDiscoverAllCharacteristicDescriptorsResp(ams, (GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *) msg);
            break;

        case GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM:
            gattAmsHandleWriteCharacteristicValueCfmMsg(ams, (GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *) msg);
            break;

        case GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM:
            gattAmsHandleReadCharacteristicValueCfmMsg(ams, (GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM_T *) msg);
            break;

        case GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND:
            /* TODO: Not implemented yet */
            break;

        default:
            DEBUG_PANIC(("AMS: GATT Msg not handled [0x%04x]\n", id));
            break;
    }
}

static void handleInternalAmsMsg(GAMS *ams, MessageId id, Message msg)
{
    switch (id)
    {
        case MSG_SET_CHARACTERISTIC_NOTIFICATION:
            handleSetCharacteristicNotificationMsg(ams, (MSG_SET_CHARACTERISTIC_NOTIFICATION_T *) msg);
            break;

        case MSG_WRITE_CHARACTERISTIC:
            handleWriteCharacteristicMsg(ams, (MSG_WRITE_CHARACTERISTIC_T *) msg);
            break;

        case MSG_READ_CHARACTERISTIC:
            handleReadCharacteristicMsg(ams, (MSG_READ_CHARACTERISTIC_T *) msg);
            break;

        default:
            DEBUG_PANIC(("AMS: Unknown Internal Msg [0x%04x]\n", id));
            break;
    }
}

void gattAmsMsgHandler(Task task, MessageId id, Message msg)
{
    GAMS *ams = (GAMS *) task;

    if ((id >= GATT_MANAGER_MESSAGE_BASE) && (id < GATT_MANAGER_MESSAGE_TOP))
        handleGattManagerMsg(ams, id, msg);
    else
        handleInternalAmsMsg(ams, id, msg);
}
