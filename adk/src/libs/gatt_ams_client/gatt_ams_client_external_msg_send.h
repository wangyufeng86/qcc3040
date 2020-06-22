/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_AMS_CLIENT_EXTERNAL_MSG_SEND_H_
#define GATT_AMS_CLIENT_EXTERNAL_MSG_SEND_H_

#include "gatt_ams_client.h"
#include <gatt.h>

void gattAmsSendWriteCharacteristicResponse(GAMS *ams, gatt_status_t gatt_status);
void gattAmsSendReadCharacteristicResponse(GAMS *ams, gatt_status_t gatt_status, uint16 value_size, const uint8 *value);
void gattAmsSendInitResponse(GAMS *ams, gatt_ams_status_t ams_status);
void gattAmsSendSetRemoteCommandNotificationResponse(GAMS *ams, gatt_status_t gatt_status);
void gattAmsSendSetEntityUpdateNotificationResponse(GAMS *ams, gatt_status_t gatt_status);

#endif /* GATT_AMS_CLIENT_EXTERNAL_MSG_SEND_H_ */
