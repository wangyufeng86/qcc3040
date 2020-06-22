/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_client_read.h"
#include "gatt_ams_client_private.h"
#include "gatt_ams_client_external_msg_send.h"

void gattAmsReadCharacteristic(GAMS *ams, uint16 handle)
{
    PRINT(("AMS: Read characteristic handle [0x%04x]\n", handle));
    GattManagerReadCharacteristicValue(&ams->lib_task, handle);
}

void gattAmsHandleReadCharacteristicValueCfmMsg(GAMS * ams, const GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM_T *cfm)
{
    PRINT(("AMS: Read characteristic value response handle [0x%04x], status [0x%04x]\n", cfm->handle, cfm->status));
    gattAmsSendReadCharacteristicResponse(ams, cfm->status, cfm->size_value, cfm->value);
}
