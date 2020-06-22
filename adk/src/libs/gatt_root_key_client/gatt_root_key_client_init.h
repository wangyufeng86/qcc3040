/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROOT_KEY_CLIENT_INIT_H_
#define GATT_ROOT_KEY_CLIENT_INIT_H_


#include "gatt_root_key_client_private.h"


/*!
    Send a GATT_ROOT_KEY_CLIENT_INIT_CFM message to the registered client task.

    \param instance The client task.
    \param status The status code to add to the message.
*/
void gattRootKeyInitSendInitCfm(GATT_ROOT_KEY_CLIENT *instance, gatt_root_key_client_status_t status);


#endif /* GATT_ROOT_KEY_CLIENT_INIT_H_ */
