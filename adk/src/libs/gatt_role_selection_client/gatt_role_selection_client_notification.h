/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROLE_SELECTION_CLIENT_NOTIFICATION_H_
#define GATT_ROLE_SELECTION_CLIENT_NOTIFICATION_H_


#include "gatt_role_selection_client_private.h"

#define ROLE_SELECTION_SERVICE_NOTIFICATION_VALUE 0x0001


/*!
    Handles the internal GATT_MANAGER_NOTIFICATION_IND message.
*/
void handleRoleSelectionNotification(GATT_ROLE_SELECTION_CLIENT *instance, 
                                     const GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND_T *ind);

#endif /* GATT_ROLE_SELECTION_CLIENT_NOTIFICATION_H_ */
