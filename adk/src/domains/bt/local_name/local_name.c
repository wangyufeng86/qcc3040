/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       local_name.c
\brief      Bluetooth Local Name component

*/

#include <connection.h>
#include <logging.h>
#include <panic.h>
#include <stdlib.h>

#include "local_name.h"


#define LOCAL_NAME_LE_PREFIX ("LE-")
#define LOCAL_NAME_SIZE_LE_PREFIX (3)

static void localName_MessageHandler(Task task, MessageId id, Message message);

static const TaskData local_name_task = {.handler = localName_MessageHandler};

static struct
{
    Task   client_task;
    uint8* name;
    uint16 name_len;
} local_name_task_data;


static void localName_StoreName(CL_DM_LOCAL_NAME_COMPLETE_T *message)
{
    if (message->status == hci_success)
    {
        local_name_task_data.name_len = message->size_local_name + LOCAL_NAME_SIZE_LE_PREFIX;

        free(local_name_task_data.name);
        local_name_task_data.name = PanicUnlessMalloc(local_name_task_data.name_len + 1);
        memcpy(local_name_task_data.name, LOCAL_NAME_LE_PREFIX, LOCAL_NAME_SIZE_LE_PREFIX);
        memcpy(local_name_task_data.name + LOCAL_NAME_SIZE_LE_PREFIX, message->local_name, message->size_local_name);
        local_name_task_data.name[local_name_task_data.name_len] = '\0';

        MessageSend(local_name_task_data.client_task, LOCAL_NAME_INIT_CFM, NULL);
    }
    else
    {
        DEBUG_LOG("localName_StoreName: failed");
        Panic();
    }
}


static void localName_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
    case CL_DM_LOCAL_NAME_COMPLETE:
        localName_StoreName((CL_DM_LOCAL_NAME_COMPLETE_T *) message);
        break;
        
    default:
        DEBUG_LOG("localName_MessageHandler: unhandled 0x%04X", id);
        break;
    }
}


bool LocalName_Init(Task init_task)
{
    DEBUG_LOG("LocalName_Init");

    local_name_task_data.client_task = init_task;
    ConnectionReadLocalName((Task) &local_name_task);
    return TRUE;
}


const uint8 *LocalName_GetName(uint16* name_len)
{
    const uint8* name = LocalName_GetPrefixedName(name_len);
    name += LOCAL_NAME_SIZE_LE_PREFIX;
    *name_len -= LOCAL_NAME_SIZE_LE_PREFIX;
    return name;
}


const uint8 *LocalName_GetPrefixedName(uint16* name_len)
{
    PanicNull(name_len);
    *name_len = local_name_task_data.name_len;
    return PanicNull(local_name_task_data.name);
}
