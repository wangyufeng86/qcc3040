/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Functions for generating telephony notification messages
*/

#include "telephony_messages.h"

#include <task_list.h>
#include <panic.h>
#include <logging.h>
#include <stdlib.h>

static task_list_t * client_list;

static task_list_t * telephony_GetMessageClients(void)
{
    return client_list;
}

void Telephony_NotifyMessage(MessageId id, voice_source_t source)
{
    telephony_message_t * const message = (telephony_message_t *)PanicNull(calloc(1, sizeof(telephony_message_t)));
    message->voice_source = source;
    TaskList_MessageSendWithSize(telephony_GetMessageClients(), id, message, sizeof(telephony_message_t));
}

bool Telephony_InitMessages(Task init_task)
{
    UNUSED(init_task);
    client_list = TaskList_Create();
    return TRUE;
}

void Telephony_RegisterForMessages(Task task_to_register)
{
    TaskList_AddTask(telephony_GetMessageClients(), task_to_register);
}

static void Telephony_RegisterMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group == TELEPHONY_MESSAGE_GROUP);
    TaskList_AddTask(telephony_GetMessageClients(), task);
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(TELEPHONY, Telephony_RegisterMessageGroup, NULL);

