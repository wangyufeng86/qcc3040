/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_client_write.h"
#include "gatt_ams_client_private.h"
#include "gatt_ams_client_external_msg_send.h"

static void writeCharacteristicValue(GAMS *ams, uint16 handle, uint16 size_value, const uint8* value)
{
    GattManagerWriteCharacteristicValue(&ams->lib_task, handle, size_value, value);
}

void gattAmsWriteCharacteristicNotifyConfig(GAMS *ams, bool notifications_enable, uint16 handle)
{
    uint8 value[2];
    
    value[0] = notifications_enable ? gatt_client_char_config_notifications_enabled : 0;
    value[1] = 0;
    
    PRINT(("AMS: Write characteristic config, handle [0x%04x], notify = %01u\n", handle, notifications_enable));
    
    writeCharacteristicValue(ams, handle, sizeof(value), value);
}

void gattAmsWriteCharacteristic(GAMS *ams, uint16 handle, uint16 size_value, const uint8 *value)
{
    PRINT(("AMS: Write characteristic, handle [0x%04x]\n", handle));

    writeCharacteristicValue(ams, handle, size_value, value);
}

void gattAmsHandleWriteCharacteristicValueCfmMsg(GAMS *ams, const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *cfm)
{
    switch (ams->pending_cmd)
    {
        case ams_pending_write_remote_command_cconfig:
            gattAmsSendSetRemoteCommandNotificationResponse(ams, cfm->status);
            break;

        case ams_pending_write_entity_update_cconfig:
            gattAmsSendSetEntityUpdateNotificationResponse(ams, cfm->status);
            break;

        case ams_pending_write_remote_command:
        case ams_pending_write_entity_update:
        case ams_pending_write_entity_attribute:
            gattAmsSendWriteCharacteristicResponse(ams, cfm->status);
            break;

        default:
            DEBUG_PANIC(("AMS: Wrong state [0x%04x], handle [0x%04x]\n", ams->pending_cmd, cfm->handle));
            break;
    }
}
