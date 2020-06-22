/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROOT_KEY_SERVER_ACCESS_H_
#define GATT_ROOT_KEY_SERVER_ACCESS_H_


/***************************************************************************
NAME
    handleRootKeyAccess

DESCRIPTION
    Handles the GATT_MANAGER_SERVER_ACCESS_IND message that was sent to 
    the root key library.
*/
void handleRootKeyAccess(GATT_ROOT_KEY_SERVER *instance, 
                         const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind);


/***************************************************************************
NAME
    sendRootKeyConfigAccessRsp

DESCRIPTION
    Sends an client configuration access response back to the GATT Manager library.
*/
void sendRootKeyConfigAccessRsp(const GATT_ROOT_KEY_SERVER *instance, 
                                uint16 cid, uint16 client_config);


void sendRootKeyAccessRsp(Task task, uint16 cid, uint16 handle,
                          uint16 result, uint16 size_value, const uint8 *value);


#endif /* GATT_ROOT_KEY_SERVER_ACCESS_H_ */
