/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_client_private.h"

uint16 gattAmsGetCharacteristicHandle(GAMS *ams, uint8 characteristic)
{
    switch(characteristic)
    {
        case GATT_AMS_CLIENT_REMOTE_COMMAND:
            return ams->remote_command_handle;
        case GATT_AMS_CLIENT_ENTITY_UPDATE:
            return ams->entity_update_handle;
        case GATT_AMS_CLIENT_ENTITY_ATTRIBUTE:
            return ams->entity_attribute_handle;
        default:
            PANIC(("AMS: Unknown characteristic [0x%04x]\n", characteristic));
            return GATT_AMS_INVALID_HANDLE;
    }
}
