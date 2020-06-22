/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef TX_POWER_MSG_HANDLER_H_
#define TX_POWER_MSG_HANDLER_H_

/***************************************************************************
NAME
    TxPowerTaskHandler

DESCRIPTION
    Handler for external messages sent to the tx power module.
*/
void TxPowerTaskHandler(Task task, MessageId id, Message message);

Task TxPower_GetTaskData(void);

#endif
