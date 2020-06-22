/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_AMS_CLIENT_NOTIFICATION_H_
#define GATT_AMS_CLIENT_NOTIFICATION_H_

#include "gatt_ams_client.h"

/*
    Handles the internal AMS_INTERNAL_MSG_SET_xx_NOTIFICATION message.
*/
void amsSetNotificationRequest(GAMS *ams, bool notifications_enable, uint8 characteristic);

#endif /* GATT_AMS_CLIENT_NOTIFICATION_H_ */
