/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application support for GATT ANCS client
*/

#ifndef GATT_CLIENT_ANCS_EVENT_HANDLER
#define GATT_CLIENT_ANCS_EVENT_HANDLER

typedef enum
{
    EventInvalid,
    EventSysAncsOtherAlert,
    EventSysAncsIncomingCallAlert,
    EventSysAncsMissedCallAlert,
    EventSysAncsVoiceMailAlert,
    EventSysAncsSocialAlert,
    EventSysAncsScheduleAlert,
    EventSysAncsNewsAlert,
    EventSysAncsHealthNFittnessAlert,
    EventSysAncsBusinessNFinanceAlert,
    EventSysAncsLocationAlert,
    EventSysAncsEntertainmentAlert,
    EventSysAncsEmailAlert,
}gatt_ancs_event;

void GattClientAncs_eventHandler(Task task, MessageId id, Message message);

#endif  /* GATT_CLIENT_ANCS_EVENT_HANDLER */
