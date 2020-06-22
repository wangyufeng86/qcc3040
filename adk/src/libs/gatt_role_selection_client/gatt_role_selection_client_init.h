/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROLE_SELECTION_CLIENT_INIT_H_
#define GATT_ROLE_SELECTION_CLIENT_INIT_H_


#include "gatt_role_selection_client_private.h"


/*!
    Send a GATT_ROLE_SELECTION_CLIENT_INIT_CFM message to the registered client task
    if a message is required (not already sent).

    \param instance The client task.
    \param status The status code to add to the message.
*/
void gattRoleSelectionInitSendInitCfm(GATT_ROLE_SELECTION_CLIENT *instance, 
                                      gatt_role_selection_client_status_t status);


#endif /* GATT_ROLE_SELECTION_CLIENT_INIT_H_ */
