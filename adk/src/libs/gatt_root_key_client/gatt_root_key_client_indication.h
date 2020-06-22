/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROOT_KEY_CLIENT_NOTIFICATION_H_
#define GATT_ROOT_KEY_CLIENT_NOTIFICATION_H_


#include "gatt_root_key_client_private.h"


#define ROOT_KEY_SERVICE_NOTIFICATION_VALUE 0x0001


/*!
    Sends a GATT_ROOT_KEY_CLIENT_SET_INDICATION_ENABLE_CFM message to the application task.
*/
void makeRootKeySetIndicationCfmMsg(GATT_ROOT_KEY_CLIENT *instance, 
                                    gatt_root_key_client_status_t status);


/*!
    Handles the internal GATT_MANAGER_NOTIFICATION_IND message.
*/
void handleRootKeyIndication(GATT_ROOT_KEY_CLIENT *instance, 
                               const GATT_MANAGER_REMOTE_SERVER_INDICATION_IND_T *ind);


#endif /* GATT_ROOT_KEY_CLIENT_NOTIFICATION_H_ */
