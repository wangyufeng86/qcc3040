/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application support for GATT ANCS client
*/

#include "gatt_client_ancs_event_handler.h"
#include <gatt_apple_notification_client.h>
#include <panic.h>
#include <logging.h>

/* Forward declarations of private funcs */
static void AncsEventHandler(gatt_ancs_event event);

typedef void (*EventHandlerFunc)(gatt_ancs_event event);
typedef struct
{
    uint32              id;
    gatt_ancs_event     event;
    EventHandlerFunc    eventHandler;
}const notification_map_t;

static notification_map_t notification[] =
{
    { ANCS_OTHER_CATEGORY_ID, EventSysAncsOtherAlert, AncsEventHandler },
    { ANCS_INCOMING_CALL_CATEGORY_ID, EventSysAncsIncomingCallAlert, AncsEventHandler },
    { ANCS_MISSED_CALL_CATEGORY_ID, EventSysAncsMissedCallAlert, AncsEventHandler },
    { ANCS_VOICE_MAIL_CATEGORY_ID, EventSysAncsVoiceMailAlert, AncsEventHandler },
    { ANCS_SOCIAL_CATEGORY_ID, EventSysAncsSocialAlert, AncsEventHandler },
    { ANCS_SCHEDULE_CATEGORY_ID, EventSysAncsScheduleAlert, AncsEventHandler },
    { ANCS_EMAIL_CATEGORY_ID, EventSysAncsEmailAlert, AncsEventHandler },
    { ANCS_NEWS_CATEGORY_ID, EventSysAncsNewsAlert, AncsEventHandler },
    { ANCS_HEALTH_N_FITNESS_CATEGORY_ID, EventSysAncsHealthNFittnessAlert, AncsEventHandler },
    { ANCS_BUSINESS_N_FINANCE_CATEGORY_ID, EventSysAncsBusinessNFinanceAlert, AncsEventHandler },
    { ANCS_LOCATION_CATEGORY_ID, EventSysAncsLocationAlert, AncsEventHandler },
    { ANCS_ENTERTAINMENT_CATEGORY_ID, EventSysAncsEntertainmentAlert, AncsEventHandler },
};
#define NOTIFICATION_TABLE_SIZE sizeof(notification)/sizeof(notification_map_t)

static void AncsEventHandler(gatt_ancs_event event)
{
    DEBUG_LOG("Gatt ANCS Event = %d", event);
}

void GattClientAncs_eventHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    uint8 table_index = 0;
    for(; table_index < NOTIFICATION_TABLE_SIZE; table_index++)
    {
        if(notification[table_index].id == (uint32)id)
        {
            notification->eventHandler(notification->event);
            break;
        }
    }

    if(table_index == NOTIFICATION_TABLE_SIZE)
    {
        DEBUG_LOG("Gatt ANCS: Invalid Event");
    }
}
