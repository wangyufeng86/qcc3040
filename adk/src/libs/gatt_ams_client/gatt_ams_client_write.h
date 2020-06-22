/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_AMS_CLIENT_WRITE_H_
#define GATT_AMS_CLIENT_WRITE_H_

#include "gatt_ams_client.h"
#include <gatt_manager.h>

void gattAmsWriteCharacteristicNotifyConfig(GAMS *ams, bool notifications_enable, uint16 handle);
void gattAmsWriteCharacteristic(GAMS *ams, uint16 handle, uint16 size_value, const uint8 *value);
void gattAmsHandleWriteCharacteristicValueCfmMsg(GAMS *ams, const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *msg);

#endif /* GATT_AMS_CLIENT_WRITE_H_ */
