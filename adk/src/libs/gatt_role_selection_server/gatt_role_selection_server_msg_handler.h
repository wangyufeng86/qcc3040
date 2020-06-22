/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROLE_SELECTION_SERVER_MSG_HANDLER_H_
#define GATT_ROLE_SELECTION_SERVER_MSG_HANDLER_H_


/***************************************************************************
NAME
    rootKeyServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void roleSelectionServerMsgHandler(Task task, MessageId id, Message payload);


#endif /* GATT_ROLE_SELECTION_SERVER_MSG_HANDLER_H_ */