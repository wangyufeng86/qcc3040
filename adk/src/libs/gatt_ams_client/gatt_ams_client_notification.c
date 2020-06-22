/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_client_notification.h"
#include "gatt_ams_client_private.h"
#include "gatt_ams_client_discover.h"
#include "gatt_ams_client_write.h"
#include "gatt_ams_client_external_msg_send.h"
#include <gatt_manager.h>

static bool discoverCharacteristicDescriptors(GAMS *ams, uint8 characteristic)
{
    uint16 startHandle, endHandle;
    gatt_manager_client_service_data_t client_data;

    client_data.start_handle = GATT_AMS_INVALID_HANDLE;
    client_data.end_handle = GATT_AMS_INVALID_HANDLE;
    
    if (!GattManagerGetClientData(&ams->lib_task, &client_data))
        PANIC(("AMS: Could not get client data\n"));

    startHandle = MIN(gattAmsGetCharacteristicHandle(ams, characteristic) + 1, client_data.end_handle);
    endHandle = gattAmsfindEndHandleForCharDesc(ams, startHandle, client_data.end_handle, characteristic);

    if(startHandle && endHandle)
        return gattAmsDiscoverAllCharacteristicDescriptors(ams, startHandle, endHandle);
    else
        return FALSE;
}

void amsSetNotificationRequest(GAMS *ams, bool notifications_enable, uint8 characteristic)
{
    switch(characteristic)
    {
        case GATT_AMS_CLIENT_REMOTE_COMMAND:
            if (CHECK_VALID_HANDLE(ams->remote_command_handle))
            {
                /* First check if ccd handle is found, else find it */
                if (CHECK_VALID_HANDLE(ams->remote_command_ccd))
                {
                    PRINT(("AMS: Write Remote Command config\n"));
                    gattAmsWriteCharacteristicNotifyConfig(ams, notifications_enable, ams->remote_command_ccd);
                    ams->pending_cmd = ams_pending_write_remote_command_cconfig;
                }
                else
                {
                    PRINT(("AMS: DiscoverAllCharacteristicDescriptors for Remote Command\n"));
                    if (discoverCharacteristicDescriptors(ams, GATT_AMS_CLIENT_REMOTE_COMMAND))
                    {
                        if (notifications_enable)
                            ams->pending_cmd = ams_pending_remote_command_notify_enable;
                        else
                            ams->pending_cmd = ams_pending_remote_command_notify_disable;
                    }
                    else
                        gattAmsSendSetRemoteCommandNotificationResponse(ams, gatt_status_failure);
                }
            }
            else
                gattAmsSendSetRemoteCommandNotificationResponse(ams, gatt_status_request_not_supported);
        break;

        case GATT_AMS_CLIENT_ENTITY_UPDATE:
            if (CHECK_VALID_HANDLE(ams->entity_update_handle))
            {
                /* First check if ccd handle is found, else find it */
                if (CHECK_VALID_HANDLE(ams->entity_update_ccd))
                {
                    PRINT(("AMS: Write Entity Update config\n"));
                    gattAmsWriteCharacteristicNotifyConfig(ams, notifications_enable, ams->entity_update_ccd);
                    ams->pending_cmd = ams_pending_write_entity_update_cconfig;
                }
                else
                {
                    PRINT(("AMS: DiscoverAllCharacteristicDescriptors for Entity Update\n"));
                    if (discoverCharacteristicDescriptors(ams, GATT_AMS_CLIENT_ENTITY_UPDATE))
                    {
                        if (notifications_enable)
                            ams->pending_cmd = ams_pending_entity_update_notify_enable;
                        else
                            ams->pending_cmd = ams_pending_entity_update_notify_disable;
                    }
                    else
                        gattAmsSendSetEntityUpdateNotificationResponse(ams, gatt_status_failure);
                }
            }
            else
                gattAmsSendSetEntityUpdateNotificationResponse(ams, gatt_status_request_not_supported);
        break;

        default:
            PANIC(("AMS: Not a valid characteristic for notifications\n"));
        break;
    }
}
