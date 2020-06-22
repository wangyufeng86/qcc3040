/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Unexpected message handler callbacks.

*/

#include "unexpected_message.h"

void (*MessageHandler)(MessageId id) = NULL;
void (*SysMessageHandler)(MessageId id) = NULL;

void UnexpectedMessage_RegisterHandler(void (*handler)(MessageId id))
{
    MessageHandler = handler;
}

void UnexpectedMessage_RegisterSysHandler(void (*handler)(MessageId id))
{
    SysMessageHandler = handler;
}

void UnexpectedMessage_HandleMessage(MessageId id)
{
    if(MessageHandler)
    {
        MessageHandler(id);
    }
}

void UnexpectedMessage_HandleSysMessage(MessageId id)
{
    if(SysMessageHandler)
    {
        SysMessageHandler(id);
    }
}
