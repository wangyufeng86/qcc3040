/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_client_external_msg_send.h"
#include "gatt_ams_client_private.h"
#include <stdlib.h>
#include <string.h>

static void sendResponseToCaller(const GAMS *ams, MessageId id, void *message)
{
    Task caller_task = ams->task_pending_cfm;

    if (caller_task)
        MessageSend(caller_task, id, message);
    else
        free(message);
}

static void setClientToNoPendingCommandState(GAMS *ams)
{
    ams->pending_cmd = ams_pending_none;
}

static gatt_ams_status_t getAmsStatusFromGattStatus(gatt_status_t gatt_status)
{
    switch(gatt_status)
    {
        case gatt_status_success:
            return gatt_ams_status_success;
        case gatt_status_request_not_supported:
            return gatt_ams_status_not_supported;
        case 0xA0:
            return gatt_ams_status_unknown_command;
        case 0xA1:
            return gatt_ams_status_invalid_command;
        case 0xA2:
            return gatt_ams_status_invalid_parameter;
        case 0xA3:
            return gatt_ams_status_action_failed;
        default:
            return gatt_ams_status_failed;
    }
}

static void sendWriteRemoteCommandResponse(const GAMS *ams, gatt_status_t gatt_status)
{
    MAKE_AMS_CLIENT_MESSAGE(GATT_AMS_CLIENT_WRITE_REMOTE_COMMAND_CFM);
    message->ams         = ams;
    message->cid         = ams->cid;
    message->status      = getAmsStatusFromGattStatus(gatt_status);
    message->gatt_status = gatt_status;
    sendResponseToCaller(ams, GATT_AMS_CLIENT_WRITE_REMOTE_COMMAND_CFM, message);
}

static void sendWriteEntityUpdateResponse(const GAMS *ams, gatt_status_t gatt_status)
{
    MAKE_AMS_CLIENT_MESSAGE(GATT_AMS_CLIENT_WRITE_ENTITY_UPDATE_CFM);
    message->ams         = ams;
    message->cid         = ams->cid;
    message->status      = getAmsStatusFromGattStatus(gatt_status);
    message->gatt_status = gatt_status;
    sendResponseToCaller(ams, GATT_AMS_CLIENT_WRITE_ENTITY_UPDATE_CFM, message);
}

static void sendWriteEntityAttributeResponse(const GAMS *ams, gatt_status_t gatt_status)
{
    MAKE_AMS_CLIENT_MESSAGE(GATT_AMS_CLIENT_WRITE_ENTITY_ATTRIBUTE_CFM);
    message->ams         = ams;
    message->cid         = ams->cid;
    message->status      = getAmsStatusFromGattStatus(gatt_status);
    message->gatt_status = gatt_status;
    sendResponseToCaller(ams, GATT_AMS_CLIENT_WRITE_ENTITY_ATTRIBUTE_CFM, message);
}

static void sendReadEntityAttributeResponse(const GAMS *ams, gatt_status_t gatt_status, uint16 value_size, const uint8 *value)
{
    MAKE_AMS_CLIENT_MESSAGE_WITH_LEN(GATT_AMS_CLIENT_READ_ENTITY_ATTRIBUTE_CFM, value_size);
    message->ams         = ams;
    message->cid         = ams->cid;
    message->status      = getAmsStatusFromGattStatus(gatt_status);
    message->gatt_status = gatt_status;
    message->value_size  = value_size;
    memcpy(message->value, value, value_size);
    sendResponseToCaller(ams, GATT_AMS_CLIENT_READ_ENTITY_ATTRIBUTE_CFM, message);
}

void gattAmsSendWriteCharacteristicResponse(GAMS *ams, gatt_status_t gatt_status)
{
    switch (ams->pending_cmd)
    {
        case ams_pending_write_remote_command:
            sendWriteRemoteCommandResponse(ams, gatt_status);
            break;

        case ams_pending_write_entity_update:
            sendWriteEntityUpdateResponse(ams, gatt_status);
            break;

        case ams_pending_write_entity_attribute:
            sendWriteEntityAttributeResponse(ams, gatt_status);
            break;

        default:
            DEBUG_PANIC(("AMS: Wrong state for write characteristic response [0x%04x]\n", ams->pending_cmd));
            break;
    }

    setClientToNoPendingCommandState(ams);
}

void gattAmsSendReadCharacteristicResponse(GAMS *ams, gatt_status_t gatt_status, uint16 value_size, const uint8 *value)
{
    switch (ams->pending_cmd)
    {
        case ams_pending_read_entity_attribute:
            sendReadEntityAttributeResponse(ams, gatt_status, value_size, value);
            break;

        default:
            DEBUG_PANIC(("AMS: Wrong state for read characteristic response [0x%04x]\n", ams->pending_cmd));
            break;
    }

    setClientToNoPendingCommandState(ams);
}

void gattAmsSendInitResponse(GAMS *ams, gatt_ams_status_t ams_status)
{
    MAKE_AMS_CLIENT_MESSAGE(GATT_AMS_CLIENT_INIT_CFM);
    message->ams    = ams;
    message->status = ams_status;
    sendResponseToCaller(ams, GATT_AMS_CLIENT_INIT_CFM, message);

    setClientToNoPendingCommandState(ams);
}

void gattAmsSendSetRemoteCommandNotificationResponse(GAMS *ams, gatt_status_t gatt_status)
{
    MAKE_AMS_CLIENT_MESSAGE(GATT_AMS_CLIENT_SET_REMOTE_COMMAND_NOTIFICATION_CFM);
    message->ams         = ams;
    message->cid         = ams->cid;
    message->status      = getAmsStatusFromGattStatus(gatt_status);
    message->gatt_status = gatt_status;
    sendResponseToCaller(ams, GATT_AMS_CLIENT_SET_REMOTE_COMMAND_NOTIFICATION_CFM, message);

    setClientToNoPendingCommandState(ams);
}

void gattAmsSendSetEntityUpdateNotificationResponse(GAMS *ams, gatt_status_t gatt_status)
{
    MAKE_AMS_CLIENT_MESSAGE(GATT_AMS_CLIENT_SET_ENTITY_UPDATE_NOTIFICATION_CFM);
    message->ams         = ams;
    message->cid         = ams->cid;
    message->status      = getAmsStatusFromGattStatus(gatt_status);
    message->gatt_status = gatt_status;
    sendResponseToCaller(ams, GATT_AMS_CLIENT_SET_ENTITY_UPDATE_NOTIFICATION_CFM, message);

    setClientToNoPendingCommandState(ams);
}
