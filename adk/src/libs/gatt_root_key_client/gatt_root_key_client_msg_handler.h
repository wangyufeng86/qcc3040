/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROOT_KEY_CLIENT_MSG_HANDLER_H_
#define GATT_ROOT_KEY_CLIENT_MSG_HANDLER_H_

/***************************************************************************
NAME
    batteryClientMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void rootKeyClientMsgHandler(Task task, MessageId id, Message payload);


#endif /* GATT_ROOT_KEY_CLIENT_MSG_HANDLER_H_ */