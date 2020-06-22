/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Handover Decision Making Algorithm Event Handler.
*/

#include "hdma.h"
#include "hdma_client_msgs.h"
#include <message.h>
#include <logging.h>
#ifndef INCLUDE_HDMA

/*! Stub Functionality: Initialise the HDMA component.*/
bool Hdma_Init(Task client_task)
{
    (void)client_task;
    DEBUG_LOG("HDMA Module Not included: Hdma_Init");
    return FALSE;
}

/*! Stub Functionality: De-Initialise the HDMA module */
bool Hdma_Destroy(void)
{
    DEBUG_LOG("HDMA Module Not included: Hdma_Destroy");
    return FALSE;
}
#else/* #ifdef INCLUDE_HDMA */
#include "hdma_core.h"
#include "battery_monitor.h"
#include "hdma_private.h"
#include "hdma_utils.h"
#include <panic.h>
#include <stdlib.h>

state_proxy_event_type HDMA_EVENTS_REGISTER = state_proxy_event_type_phystate;
/* HDMA instance. External variable */
hdma_task_data_t *hdma = NULL;
static void hdma_StartIntervalTimerMessage(void);
static void hdma_DestroyIntervalTimerMessage(void);

/*! \brief Initialise the HDMA component.
    \param[in] client_task Task to register for #hdma_messages_t messages.

    \return bool TRUE if initialisation successful
                 FALSE Initialisation failed
*/
bool Hdma_Init(Task client_task)
{
    DEBUG_LOG("Hdma_Init");
	DEBUG_LOG("INCLUDE_HDMA");
    state_proxy_event_type events = 0;
    if(hdma && hdma->initialised == HDMA_INIT_COMPLETED_MAGIC)
    {
        DEBUG_LOG("Hdma_Init: HDMA already initialised");
        return FALSE;
    }
    hdma = PanicUnlessMalloc(sizeof(hdma_task_data_t));
    memset(hdma, 0, sizeof(hdma_task_data_t));
    hdma->task.handler = hdma_HandleMessage;

    /* Initialise TaskList */
    hdma->client_task = client_task;
    Hdma_CoreInit();
#ifdef INCLUDE_HDMA_BATTERY_EVENT
    HDMA_EVENTS_REGISTER |= state_proxy_event_type_battery_state;
	DEBUG_LOG("INCLUDE_HDMA_BATTERY_EVENT = ENABLED");
#else
	DEBUG_LOG("INCLUDE_HDMA_BATTERY_EVENT = DISABLED");
#endif

#ifdef INCLUDE_HDMA_MIC_QUALITY_EVENT
    HDMA_EVENTS_REGISTER |= state_proxy_event_type_mic_quality|state_proxy_event_type_sco_active|state_proxy_event_type_sco_inactive;
	DEBUG_LOG("INCLUDE_HDMA_MIC_QUALITY_EVENT = ENABLED");
#else
	DEBUG_LOG("INCLUDE_HDMA_MIC_QUALITY_EVENT = DISABLED");
#endif
#ifdef INCLUDE_HDMA_RSSI_EVENT
    HDMA_EVENTS_REGISTER |= state_proxy_event_type_link_quality;
	DEBUG_LOG("INCLUDE_HDMA_RSSI_EVENT = ENABLED");
#else
	DEBUG_LOG("INCLUDE_HDMA_RSSI_EVENT = DISABLED");
#endif
    events = HDMA_EVENTS_REGISTER;
    /* get state proxy events */
    StateProxy_EventRegisterClient(&hdma->task, events);
    hdma->initialised = HDMA_INIT_COMPLETED_MAGIC;
    MessageSendLater(&(hdma->task), HDMA_INTERNAL_TIMER_EVENT,
                 NULL, MIN_HANDOVER_RETRY_TIME_LOW_MS);
    return TRUE;
}

/*! \brief De-Initialise the HDMA module
    \return bool TRUE if De-initialisation successful
                 FALSE De-initialisation failed
*/
bool Hdma_Destroy(void)
{
    DEBUG_LOG("Hdma_Destroy");
    state_proxy_event_type events = 0;
    if(hdma && hdma->initialised == HDMA_INIT_COMPLETED_MAGIC)
    {
        events = HDMA_EVENTS_REGISTER;
        StateProxy_EventUnregisterClient(&hdma->task, events);
        hdma_DestroyIntervalTimerMessage();
        Hdma_CoreDestroy();
        free(hdma);
        hdma = NULL;
        return TRUE;
    }
    return FALSE;
}

/*! \brief Trigger an external request to handover.
    \return bool TRUE if external handover request was sent, otherwise FALSE.

    \note This will cause a hdma_handover_decision_t to be sent to the client_task
    with reason HDMA_HANDOVER_REASON_EXTERNAL and HDMA_HANDOVER_URGENCY_CRITICAL.
*/
bool Hdma_ExternalHandoverRequest(void)
{
    DEBUG_LOG("Hdma_ExternalHandoverRequest");

    if(hdma && hdma->initialised == HDMA_INIT_COMPLETED_MAGIC)
    {
        hdma_NotifyHandoverClients(HDMA_HANDOVER_NOTIFICATION,
                                   VmGetTimerTime(),
                                   HDMA_HANDOVER_REASON_EXTERNAL,
                                   HDMA_HANDOVER_URGENCY_CRITICAL);
        return TRUE;
    }
    return FALSE;
}

/*! \brief Handle state proxy events.
    \param[in] sp_event State Proxy event message.
*/
static void hdma_HandleStateProxyEvent(const STATE_PROXY_EVENT_T* sp_event)
{
    DEBUG_LOG("hdma_HandleStateProxyEvent: source %u type %u timestamp %u", sp_event->source, sp_event->type, sp_event->timestamp);
    bool is_this_bud = (sp_event->source != state_proxy_source_remote);;
    switch(sp_event->type)
    {
        case state_proxy_event_type_phystate:
            DEBUG_LOG("hdma_HandleStateProxyEvent: Phy State %u", sp_event->event.phystate);
            hdma_HandlePhyState(is_this_bud, sp_event->timestamp, sp_event->event.phystate.event);
            break;
#ifdef INCLUDE_HDMA_MIC_QUALITY_EVENT
        case state_proxy_event_type_sco_active:
            hdma_HandleScoEvent(sp_event->timestamp, TRUE);
            break;
        case state_proxy_event_type_sco_inactive:
            hdma_HandleScoEvent(sp_event->timestamp, FALSE);
            break;
        case state_proxy_event_type_mic_quality:
            DEBUG_LOG("hdma_HandleStateProxyEvent: Audio Quality %u", sp_event->event.mic_quality);
            hdma_HandleVoiceQuality(is_this_bud, sp_event->timestamp, &(sp_event->event.mic_quality));
            break;
#endif
#ifdef INCLUDE_HDMA_BATTERY_EVENT
        case state_proxy_event_type_battery_state:
            DEBUG_LOG("hdma_HandleStateProxyEvent: Battery State %u", (MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T *)(&(sp_event->event)));
            hdma_HandleBatteryLevelStatus(is_this_bud, sp_event->timestamp, (MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T *)(&(sp_event->event)));
            break;
#endif
#ifdef INCLUDE_HDMA_RSSI_EVENT
        case state_proxy_event_type_link_quality:
            DEBUG_LOG("hdma_HandleStateProxyEvent: Link Quality %u", sp_event->event.link_quality);
            hdma_HandleLinkQuality(is_this_bud, sp_event->event.link_quality.flags.is_peer, sp_event->timestamp, &(sp_event->event.link_quality));
            break;
#endif
        default:
            DEBUG_LOG("hdma_HandleStateProxyEvent: Event not handled: sp_event->type = %d", sp_event->event);
            break;
    }
}

/*! \brief If earbud is out of ear then trigger timer to start handover after OUT_OF_EAR_TIME_BEFORE_HANDOVER_MS ms.

*/
static void hdma_StartIntervalTimerMessage(void)
{
    if(!Hdma_IsOutOfEarEnabled())
    {
        MessageSendLater(&(hdma->task), HDMA_INTERNAL_TIMER_EVENT,
                     NULL, OUT_OF_EAR_TIME_BEFORE_HANDOVER_MS);
    }
}

/*! \brief If earbud is in ear then stop timer which was started when earbud was out of ear.

*/
static void hdma_DestroyIntervalTimerMessage(void)
{
    if(Hdma_IsOutOfEarEnabled())
    {
        MessageCancelAll(&(hdma->task), HDMA_INTERNAL_TIMER_EVENT);
    }
}

/*! \brief HDMA Message Handler.

    \param[in] task Time at which event is raised.
    \param[in] id Message id
    \param[in] message Message data
*/
void hdma_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    uint32 timestamp = VmGetClock();
    switch (id)
    {
        case STATE_PROXY_EVENT:
            hdma_HandleStateProxyEvent((const STATE_PROXY_EVENT_T*)message);
            break;
        case HDMA_INTERNAL_TIMER_EVENT:
            Hdma_CoreHandleInternalEvent(timestamp);
            break;
        default:
            DEBUG_LOG("Hdma_HandleMessage: Event not handled: %d", id);
            break;
    }
}

/*! \brief Handle the phy state event from the State Proxy

    \param[in] is_this_bud source of the event (true:-this bud, false:- peer bud).
    \param[in] timestamp Time at which event is received.
    \param[in] event phy state event.
*/
void hdma_HandlePhyState(bool is_this_bud,uint32 timestamp,
                                phy_state_event event)
{
    DEBUG_LOG("hdma_HandlePhyState: is_this_bud= %u phystate %d", is_this_bud, event);
    hdma_core_event_t evt = HDMA_CORE_IN_EAR;
    switch(event)
    {
        case phy_state_event_in_case:
            DEBUG_LOG("hdma_HandlePhyState: phystate %d", event);
            evt = is_this_bud? HDMA_CORE_IN_CASE : HDMA_CORE_PEER_IN_CASE;
            if (is_this_bud)
            {
                hdma_DestroyIntervalTimerMessage();
            }
            break;

        case phy_state_event_out_of_case:
            DEBUG_LOG("hdma_HandlePhyState: phystate %d", event);
            evt = is_this_bud? HDMA_CORE_OUT_OF_CASE : HDMA_CORE_PEER_OUT_OF_CASE;
            if(is_this_bud)
            {
                hdma_StartIntervalTimerMessage();
            }
            break;

        case phy_state_event_in_ear:
            DEBUG_LOG("hdma_HandlePhyState: phystate %d", event);
            evt = is_this_bud? HDMA_CORE_IN_EAR : HDMA_CORE_PEER_IN_EAR;
            if(is_this_bud)
            {
                hdma_DestroyIntervalTimerMessage();
            }
            break;

        case phy_state_event_out_of_ear:
            DEBUG_LOG("hdma_HandlePhyState: phystate %d", event);
            evt = is_this_bud? HDMA_CORE_OUT_OF_EAR : HDMA_CORE_PEER_OUT_OF_EAR;
            if(is_this_bud)
            {
                hdma_StartIntervalTimerMessage();
            }
            break;
        default:
            DEBUG_LOG("hdma_HandlePhyState: Event not handled %d", event);
            return;
    }
    Hdma_CoreHandleEvent( timestamp, evt);
}

#ifdef INCLUDE_HDMA_BATTERY_EVENT
/*! \brief Handle the battery level status event from the State Proxy

    \param[in] is_this_bud source of the event (true:-this bud, false:- peer bud).
    \param[in] timestamp Time at which event is received.
    \param[in] battery_level current battery level of battery.

*/
void hdma_HandleBatteryLevelStatus(bool is_this_bud,uint32 timestamp,
            MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T* battery_level)
{
    DEBUG_LOG("hdma_HandleBatteryLevelStatus: Timestamp = %u is_this_bud = %u, battery_level = %u", timestamp, is_this_bud, battery_level->state);
    Hdma_CoreHandleBatteryStatus(timestamp, is_this_bud, (hdma_core_battery_state_t)battery_level->state);
}
#endif

#ifdef INCLUDE_HDMA_MIC_QUALITY_EVENT

/*! \brief Handle the Call connect/disconnect event from the State Proxy.

    \param[in] timestamp Time at which event raised.
    \param[in] isconnect Call is connected or disconnected, 0 = connect, 1 = disconnect
*/
void hdma_HandleScoEvent(uint32 timestamp, bool is_sco_active)
{
    DEBUG_LOG("hdma_HandleScoEvent: Timestamp= %u is_sco_active %d", timestamp, is_sco_active);
    if(is_sco_active)
    {
        Hdma_CoreHandleEvent( timestamp, HDMA_CORE_SCO_CONN);
    }
    else
    {
        Hdma_CoreHandleEvent( timestamp, HDMA_CORE_SCO_DISCON);
    }
}


/*! \brief Handle the voice quality event from the State Proxy.
           This event will be raised only during an active  HFP call.

    \param[in] is_this_bud source of the event (true:-this bud, false:- peer bud).
    \param[in] timestamp Time at which event is raised.
    \param[in] voice_quality voice quality indicator, 0 = worst, 15 = best, 0xFF unknown.

*/
void hdma_HandleVoiceQuality(bool is_this_bud, uint32 timestamp,
                        STATE_PROXY_MIC_QUALITY_T* voice_quality)
{
    DEBUG_LOG("hdma_HandleVoiceQuality: Timestamp = %u is_this_bud = %u, mic_quality = %u", timestamp, is_this_bud, voice_quality->mic_quality);
    Hdma_CoreHandleVoiceQuality(timestamp, is_this_bud, voice_quality->mic_quality);
}
#endif

#ifdef INCLUDE_HDMA_RSSI_EVENT
/*! \brief Handle the link quality event from State Proxy

    \param[in] is_this_bud source of the event (true:-this bud, false:- peer bud).
    \param[in] isPeerLink is link quality for peer link or phone
    \param[in] timestamp Time at which event is received.
    \param[in] link_quality link quality indicator (rssi, link_quality)
*/
void hdma_HandleLinkQuality(bool is_this_bud,bool isPeerLink,uint32 timestamp,
                       STATE_PROXY_LINK_QUALITY_T* link_quality)
{
    hdma_core_link_quality_t qual;
    qual.rssi = link_quality->rssi;
    qual.link_quality = link_quality->link_quality;

    DEBUG_LOG("hdma_HandleLinkQuality: Timestamp = %u is_this_bud = %u, isPeerLink = %u, RSSI = %d, link_quality = %u", timestamp, is_this_bud, isPeerLink, link_quality->rssi, link_quality->link_quality);
    Hdma_CoreHandleLinkQuality(timestamp, is_this_bud, isPeerLink, qual);
}
#endif

/*! \brief This function will force handover with specified urgency

    \param[in] timestamp Time at which event is received.
    \param[in] urgency urgency of handover.
*/
void hdma_HandleExternalReq(uint32 timestamp, hdma_handover_urgency_t urgency)
{
    DEBUG_LOG("hdma_HandleExternalReq: Timestamp= %u urgency %d", timestamp, urgency);
    hdma_core_handover_urgency_t core_urgency = HDMA_CORE_HANDOVER_URGENCY_INVALID;
    switch(urgency)
    {
        case HDMA_HANDOVER_URGENCY_INVALID:
            core_urgency = HDMA_CORE_HANDOVER_URGENCY_INVALID;
            break;

        case HDMA_HANDOVER_URGENCY_LOW:
            core_urgency = HDMA_CORE_HANDOVER_URGENCY_LOW;
            break;

        case HDMA_HANDOVER_URGENCY_HIGH:
            core_urgency =HDMA_CORE_HANDOVER_URGENCY_HIGH;
            break;

        case HDMA_HANDOVER_URGENCY_CRITICAL:
            core_urgency = HDMA_CORE_HANDOVER_URGENCY_CRITICAL;
            break;
        default:
            DEBUG_LOG("hdma_HandleExternalReq: Invalid urgency request %d", urgency);
            return;
    }
    Hdma_CoreHandleExternalReq( timestamp, core_urgency);
}
#endif /* INCLUDE_HDMA */
