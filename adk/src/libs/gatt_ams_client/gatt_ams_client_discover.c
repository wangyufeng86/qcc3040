/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_client_discover.h"
#include "gatt_ams_client_private.h"
#include "gatt_ams_client_write.h"
#include "gatt_ams_client_notification.h"
#include "gatt_ams_client_external_msg_send.h"
#include "gatt_ams_client_ready_state.h"

static void nextAfterDiscoverCharacteristics(GAMS *ams)
{
    switch (ams->pending_cmd)
    {
        case ams_pending_init:
        {
            /* All characteristics found, there are no mandatory characteristics */
            gattAmsSendInitResponse(ams, gatt_ams_status_success);
            gattAmsReadyStateUpdate(ams, TRUE);
        }
        break;

        default:
        {
            DEBUG_PANIC(("AMS: No action after characteristic discovery [0x%04x]\n", ams->pending_cmd));
        }
        break;
    }
}

static void processDiscoveredDescriptor(GAMS *ams, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    switch (ams->pending_cmd)
    {
        case ams_pending_remote_command_notify_enable:
        case ams_pending_remote_command_notify_disable:
        {
            if (cfm->uuid_type == gatt_uuid16)
            {
                if (cfm->uuid[0] == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID)
                {
                    bool notify_pending = (ams->pending_cmd == ams_pending_remote_command_notify_enable);

                    PRINT(("AMS: Found Remote Command CCD handle = [0x%04x]\n", cfm->handle));

                    gattAmsWriteCharacteristicNotifyConfig(ams, notify_pending, cfm->handle);
                    ams->remote_command_ccd = cfm->handle;
                    ams->pending_cmd = ams_pending_write_remote_command_cconfig;
                }
            }
        }
        break;

        case ams_pending_entity_update_notify_enable:
        case ams_pending_entity_update_notify_disable:
        {
            if (cfm->uuid_type == gatt_uuid16)
            {
                if (cfm->uuid[0] == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID)
                {
                    bool notify_pending = (ams->pending_cmd == ams_pending_entity_update_notify_enable);

                    PRINT(("AMS: Found Entity Update CCD handle = [0x%04x]\n", cfm->handle));

                    gattAmsWriteCharacteristicNotifyConfig(ams, notify_pending, cfm->handle);
                    ams->entity_update_ccd = cfm->handle;
                    ams->pending_cmd = ams_pending_write_entity_update_cconfig;
                }
            }
        }
        break;

        case ams_pending_write_remote_command_cconfig:
        case ams_pending_write_entity_update_cconfig:
            PRINT(("AMS: Processing descriptor, state [0x%04x]\n", ams->pending_cmd));
        break;
        
        default:
            DEBUG_PANIC(("AMS: Wrong state for descriptor processing, state [0x%04x], handle [0x%04x]\n", ams->pending_cmd, cfm->handle));
        break;
    }
}

static void nextAfterDiscoverDescriptors(GAMS *ams)
{
    switch (ams->pending_cmd)
    {
        case ams_pending_remote_command_notify_enable:
        case ams_pending_remote_command_notify_disable:
            gattAmsSendSetRemoteCommandNotificationResponse(ams, gatt_status_request_not_supported);
            break;

        case ams_pending_entity_update_notify_enable:
        case ams_pending_entity_update_notify_disable:
            gattAmsSendSetEntityUpdateNotificationResponse(ams, gatt_status_request_not_supported);
            break;

        case ams_pending_write_remote_command_cconfig:
        case ams_pending_write_entity_update_cconfig:
            /* No action needed as write of client configuration descriptor will happen next */
            break;

        default:
            DEBUG_PANIC(("AMS: No action after descriptor discovery [0x%04x]\n", ams->pending_cmd));
            break;
    }
}

uint16 gattAmsfindEndHandleForCharDesc(GAMS *ams, uint16 startHandle, uint16 endHandle, uint8 characteristic)
{
    uint8 charIndex = 0;
    uint8 charVal = 0;
    uint8 mask;
    uint8 char_report_mask = ams->char_report_mask;

    unsigned retHandle = 0;

    /* The characteristics are 2 bit fields overlaid in the same byte
       Our task data has a mask for what to report, and the required
       characteristic value
     */

    /* if and only if there is proper characteristic request for the descriptor */
    while( charIndex < GATT_AMS_CLIENT_MAX_CHAR )
    {
        if(char_report_mask)
        {
            mask = GATT_AMS_CLIENT_FIELD_MASK(charIndex);
            /* Mask the value and shift */
            charVal = char_report_mask & mask;
            charVal = charVal >> GATT_AMS_CLIENT_FIELD_START(charIndex);

            /* Remove the value we have just checked from the report mask */
            char_report_mask = (char_report_mask & ~mask);

            /* Did the value match the one we wanted */
            if( charVal == characteristic)
            {
                /* Check the next field */
                mask = GATT_AMS_CLIENT_FIELD_MASK(charIndex+1);;
                charVal = (char_report_mask & mask);
                charVal = charVal >> GATT_AMS_CLIENT_FIELD_START(charIndex+1);

                switch( charVal )
                {
                    case GATT_AMS_CLIENT_REMOTE_COMMAND:
                        retHandle = ams->remote_command_handle- 1;
                    break;

                    case GATT_AMS_CLIENT_ENTITY_UPDATE:
                        retHandle = ams->entity_update_handle - 1;
                    break;

                    case GATT_AMS_CLIENT_ENTITY_ATTRIBUTE:
                        retHandle = ams->entity_attribute_handle - 1;
                    break;

                    default:
                    {
                        /* TODO : Need to check this */
                        if(startHandle < endHandle)
                        {
                            retHandle = endHandle;
                        }
                        else
                        {
                            retHandle = startHandle;
                        }
                    }
                    break;
                }
                /* Exit loop */
                break;
            }
       }
       charIndex ++;
    }

    return (uint16)retHandle;
}

void handleAmsDiscoverAllCharacteristicsResp(GAMS *ams, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    uint8  charIndex = 0;
    uint8  charVal = 0;
        /* char_report_mask is 8 bit value which is divided as 2 bits each for the 3 characteristic. 
        The least significant 2 bits indicates the first characteristic discovered, 
        the second significant 2 bits indicates the second characteristic discovered and so on & so forth.
        This mask is later used for getting the start & end handle for discovering characteristic descriptor
        for each characteristic
    */
    charIndex = ams->char_report_mask_index;
        
    if (cfm->status == gatt_status_success)
    {
        if (cfm->uuid_type == gatt_uuid128)
        {
            if (CHECK_AMS_REMOTE_COMMAND_UUID(cfm))
            {
                PRINT(("AMS: Found Remote Command handle [0x%04x], status [0x%04x]\n", cfm->handle , cfm->status));
                ams->remote_command_handle = cfm->handle;
                charVal = GATT_AMS_CLIENT_REMOTE_COMMAND;
            }
            else if(CHECK_AMS_ENTITY_UPDATE_UUID(cfm))
            {
                PRINT(("AMS: Found Entity Update handle [0x%04x], status [0x%04x]\n", cfm->handle , cfm->status));
                ams->entity_update_handle = cfm->handle;
                charVal = GATT_AMS_CLIENT_ENTITY_UPDATE;
            }
            else if(CHECK_ENTITY_ATTRIBUTE_UUID(cfm))
            {
                PRINT(("AMS: Found Entity Attribute handle [0x%04x], status [0x%04x]\n", cfm->handle , cfm->status));
                ams->entity_attribute_handle = cfm->handle;
                charVal = GATT_AMS_CLIENT_ENTITY_ATTRIBUTE;
            }

            if (charVal)
            {
                charVal = (uint8)(charVal << GATT_AMS_CLIENT_FIELD_START(charIndex));
                ams->char_report_mask |= charVal;
                charIndex++;
            }
            ams->char_report_mask_index = charIndex;
        }
        /* Ignore unwanted characteristics */
    }

    /* No more to come, so process the characteristics */
    if (!cfm->more_to_come)
    {
        /* Reset the index as this is going to be used in getting the descriptor */
        ams->char_report_mask_index = 0;
        nextAfterDiscoverCharacteristics(ams);
    }
}

bool gattAmsDiscoverAllCharacteristicDescriptors(GAMS *ams, uint16 start_handle, uint16 end_handle)
{
    GattManagerDiscoverAllCharacteristicDescriptors(&ams->lib_task, start_handle, end_handle);
    return TRUE;
}

void handleAmsDiscoverAllCharacteristicDescriptorsResp(GAMS *ams, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    if (cfm->status == gatt_status_success)
    {
        processDiscoveredDescriptor(ams, cfm);
    }
    
    if (!cfm->more_to_come)
    {
        nextAfterDiscoverDescriptors(ams);
    }
}
