/*!
\copyright  Copyright (c) 2018 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application initialisation framework based on a table-driven approach.
*/

#include <stdlib.h>
#include <panic.h>

#include "init.h"

/*! Value used to reduce the chance that a random RAM value might affect a test
    system reading the initialised flag */
#define APP_INIT_COMPLETED_MAGIC (0x2D)

typedef struct
{
    TaskData task;
    const init_table_entry_t *table;
    uint16 table_count;
    uint16 table_index;
    Task app_task;
} init_context_t;

static init_context_t *init_ctx;
static Task init_task_cache;
static uint8 initialised;

static void completeInit(bool success)
{
    MESSAGE_MAKE(msg, APPS_COMMON_INIT_CFM_T);
    msg->success = success;
    MessageSend(init_ctx->app_task, APPS_COMMON_INIT_CFM, msg);

    free(init_ctx);
    init_ctx = 0;
}

static void processNextInitEntry(void)
{
    bool success = TRUE;

    init_ctx->table_index++;

    while (init_ctx->table_index < init_ctx->table_count)
    {
        success = init_ctx->table[init_ctx->table_index].init(&init_ctx->task);
        if (!success)
            break;

        if (init_ctx->table[init_ctx->table_index].async_message_id)
            return;

        init_ctx->table_index++;
    }

    completeInit(success);
}

static void processAsyncInitMessage(Message message)
{
    init_handler handler = init_ctx->table[init_ctx->table_index].async_handler;

    if (handler && !handler(message))
    {
        /* The handler returned a failure so end the init sequence
           here and return an error to the app. */
        completeInit(FALSE);
    }
    else
    {
        /* Either:
             No custom handler - so assume init step was successful
             Custom handler returned success
           Carry on and process the next init entry. */
        processNextInitEntry();
    }
}

static void initMessageHandler(Task task, MessageId id, Message message)
{
    PanicFalse(task == &init_ctx->task);

    if (init_ctx->table[init_ctx->table_index].async_message_id == id)
    {
        processAsyncInitMessage(message);
    }
    else
    {
        /* Unexpected MessageId should never happen so end the init
           sequence here with an error. */
        completeInit(FALSE);
    }
}

static void initialiseInitContext(Task app_task, const init_table_entry_t* init_table, uint16 init_table_count)
{
    PanicNotNull(init_ctx);

    init_ctx = PanicUnlessMalloc(sizeof(*init_ctx));
    memset(init_ctx, 0, sizeof(*init_ctx));

    init_ctx->task.handler = initMessageHandler;
    init_ctx->app_task = app_task;
    init_ctx->table = init_table;
    init_ctx->table_count = init_table_count;
    init_ctx->table_index = 0xFFFF;
}

void AppsCommon_StartInit(Task app_task, const init_table_entry_t* init_table, uint16 init_table_count)
{
    initialised = 0;

    initialiseInitContext(app_task, init_table, init_table_count);

    processNextInitEntry();
}

void Init_SetInitTask(Task init_task)
{
    init_task_cache = init_task;
}

Task Init_GetInitTask(void)
{
    return init_task_cache;
}

void Init_SetCompleted(void)
{
    initialised = APP_INIT_COMPLETED_MAGIC;
}

bool Init_IsCompleted(void)
{
    return initialised == APP_INIT_COMPLETED_MAGIC;
}
