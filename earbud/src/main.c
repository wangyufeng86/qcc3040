/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       main.c
\brief      Main application task
*/

#include <hfp.h>
#include <a2dp.h>
#include <avrcp.h>
#include <connection.h>
#include <panic.h>
#include <pio.h>
#include <ps.h>
#include <string.h>
#include <boot.h>
#include <os.h>
#include <logging.h>
#include <app/message/system_message.h>

#include "app_task.h"
#include "earbud_init.h"
#include "init.h"
#include "earbud_ui.h"
#include "earbud_anc_tuning.h"
#include "earbud_sm.h"
#include "device_upgrade.h"
#include "usb_common.h"
#include "timestamp_event.h"
#include "unexpected_message.h"
#include "av.h"


/*! Application data structure */
appTaskData globalApp;

/*! \brief Handle subsystem event report. */
static void appHandleSubsystemEventReport(MessageSubsystemEventReport *evt)
{
    UNUSED(evt);
    DEBUG_LOG_VERBOSE("appHandleSubsystemEventReport, ss_id=%d, level=%d, id=%d, cpu=%d, occurrences=%d, time=%d",
        evt->ss_id, evt->level, evt->id, evt->cpu, evt->occurrences, evt->time);
}

/*! \brief System Message Handler

    This function is the message handler for system messages. They are
    routed to existing handlers. If execution reaches the end of the
    function then it is assumed that the message is unhandled.
*/
static void appHandleSystemMessage(Task task, MessageId id, Message message)
{
    bool handled = FALSE;

    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case MESSAGE_SUBSYSTEM_EVENT_REPORT:
            appHandleSubsystemEventReport((MessageSubsystemEventReport *)message);
            return;

        case MESSAGE_IMAGE_UPGRADE_ERASE_STATUS:
        case MESSAGE_IMAGE_UPGRADE_COPY_STATUS:
        case MESSAGE_IMAGE_UPGRADE_AUDIO_STATUS:
        case MESSAGE_IMAGE_UPGRADE_HASH_ALL_SECTIONS_UPDATE_STATUS:
            handled = appUpgradeHandleSystemMessages(id, message, FALSE);
            break;

        case MESSAGE_USB_ENUMERATED:
        case MESSAGE_USB_SUSPENDED:
             Usb_HandleMessage(task, id, message);
             handled = TRUE;
             break;

        default:
            break;
    }

    if (!handled)
    {
        UnexpectedMessage_HandleSysMessage(id);
    }
}

/*  Handler for the INIT_CFM message.

    Used to register the handler that decides whether to allow entry
    to low power mode, before passing the #APPS_COMMON_INIT_CFM message to
    the state machine handler.

    \param message The APPS_COMMON_INIT_CFM message received (if any).
 */
static void appHandleCommonInitCfm(Message message)
{
    TimestampEvent(TIMESTAMP_EVENT_INITIALISED);
    appInitSetInitialised();
    appSmHandleMessage(SmGetTask(), APPS_COMMON_INIT_CFM, message);
}

/*! \brief Message Handler

    This function is the main message handler for the main application task, every
    message is handled in it's own seperate handler function.  The switch
    statement is broken into seperate blocks to reduce code size, if execution
    reaches the end of the function then it is assumed that the message is
    unhandled.
*/
static void appHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
            /* AV messages */
        case AV_CREATE_IND:
        case AV_DESTROY_IND:
            return;

        case APPS_COMMON_INIT_CFM:
            appHandleCommonInitCfm(message);
            return;
    }

    UnexpectedMessage_HandleMessage(id);
}

extern void _init(void);
void _init(void)
{
    Usb_TimeCriticalInit();
}

/*! Application init start task data */
typedef struct appInitStartTaskData
{
    TaskData            task;                   /*!< Application task */
} appInitStartTaskData;

static void handleMessageAppInitStart(void)
{
    /* Start the application module and library initialisation sequence */

    DEBUG_LOG_INFO("handleMessageAppInitStart");

    appInit();
}

#define MSG_APP_INIT_START 0xFFF0

static void appHandleAppInit(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case MSG_APP_INIT_START:
        handleMessageAppInitStart();
        return;
    }

    UnexpectedMessage_HandleMessage(id);
}

/*! \brief Application entry point

    This function is the entry point for the application, it performs basic
    initialisation of the application task state machine and then sets
    the state to 'initialising' which will start the initialisation procedure.

    \returns Nothing. Only exits by powering down.
*/
int main(void)
{
    OsInit();

    TimestampEvent(TIMESTAMP_EVENT_BOOTED);

#ifdef SET_LOG_LEVEL
    /* Define SET_LOG_LEVEL in the project to change default log level.
       Use values defined in debug_log_level_t */
    LOG_LEVEL_CURRENT_SYMBOL = SET_LOG_LEVEL;
    debug_log_level__global = SET_LOG_LEVEL;
#endif

    /* Set up task handlers */
    appGetApp()->task.handler = appHandleMessage;
    appGetApp()->systask.handler = appHandleSystemMessage;

    appInitStartTaskData td;
    td.task.handler = appHandleAppInit;

    MessageSystemTask(appGetSysTask());

    MessageSend(&td.task, MSG_APP_INIT_START, NULL);

    /* Start the message scheduler loop */
    MessageLoop();

    /* We should never get here, keep compiler happy */
    return 0;
}
