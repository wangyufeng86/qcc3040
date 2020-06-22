/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROLE_SELECTION_SERVER_ACCESS_H_
#define GATT_ROLE_SELECTION_SERVER_ACCESS_H_

#include <gatt.h>

#include "gatt_role_selection_server.h"

/*! Handles the GATT_MANAGER_SERVER_ACCESS_IND message that was sent to 
    the role selection library.
*/
void handleRoleSelectionServiceAccess(GATT_ROLE_SELECTION_SERVER *instance, 
                                      const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind);


/*!  Sends an client configuration access response back to the GATT Manager library.
*/
void sendRoleSelectionConfigAccessRsp(const GATT_ROLE_SELECTION_SERVER *instance, 
                                uint16 cid, uint16 client_config);


/*!  Sends an access response back to the GATT Manager library.
*/
void sendRoleSelectionAccessRsp(Task task, uint16 cid, uint16 handle,
                                uint16 result, uint16 size_value, const uint8 *value);


#endif /* GATT_ROLE_SELECTION_SERVER_ACCESS_H_ */
