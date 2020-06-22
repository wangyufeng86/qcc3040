/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Header file containg private interfaces for HDMA module
*/

#ifdef INCLUDE_HDMA
#ifndef HDMA_PRIVATE_H
#define HDMA_PRIVATE_H

#include <state_proxy.h>
#include "battery_monitor.h"
#include "hdma_core.h"
#include "hdma.h"
#include <task_list.h>
#include "domain_message.h"

/*! Value used to reduce the chance that a random RAM value might affect a test
    system reading the initialised flag */
#define HDMA_INIT_COMPLETED_MAGIC (0x2D)


/*! \brief HDMA internal state. */
typedef struct
{
    /*!< Flag used to indicate that the full initialisation has completed */
    uint8 initialised;
    /*! HDMA task */
    TaskData task;
    /*! Client registered to receive #HDMA_HANDOVER_NOTIFICATION_T
       messages  */
    Task client_task;
} hdma_task_data_t;

/*! Internal messages sent by hdma to itself. */
typedef enum
{
    HDMA_INTERNAL_TIMER_EVENT = INTERNAL_MESSAGE_BASE
}hdma_internal_messages;

/* Defined in hdma.c */

extern hdma_task_data_t *hdma;

/*! \brief Handle the battery level status event from the State Proxy

    \param[in] is_this_bud source of the event (true:-this bud, false:- peer bud).
    \param[in] timestamp Time at which event is received.
    \param[in] battery_level current battery level of battery.

*/
void hdma_HandleBatteryLevelStatus(bool is_this_bud,uint32 timestamp,
                        MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T* battery_level);


/*! \brief Handle the voice quality event from the State Proxy.
           This event will be raised only during an active  HFP call.

    \param[in] is_this_bud source of the event (true:-this bud, false:- peer bud).
    \param[in] timestamp Time at which event is raised.
    \param[in] voice_quality voice quality indicator, 0 = worst, 15 = best, 0xFF unknown.

*/
void hdma_HandleVoiceQuality(bool is_this_bud,uint32 timestamp,
                        STATE_PROXY_MIC_QUALITY_T* voice_quality);


/*! \brief Handle the phy state event from the State Proxy

    \param[in] is_this_bud source of the event (true:-this bud, false:- peer bud).
    \param[in] timestamp Time at which event is received.
    \param[in] phy_state phy state event.
*/
void hdma_HandlePhyState(bool is_this_bud,uint32 timestamp,
                        phy_state_event phy_state);


/*! \brief Handle the link quality event from State Proxy

    \param[in] is_this_bud source of the event (true:-this bud, false:- peer bud).
    \param[in] isPeerLink is link quality for peer link or phone
    \param[in] timestamp Time at which event is received.
    \param[in] link_quality link quality indicator (rssi, link_quality)
*/
void hdma_HandleLinkQuality(bool is_this_bud,bool isPeerLink,uint32 timestamp,
                       STATE_PROXY_LINK_QUALITY_T* link_quality);

/*! \brief This function will force handover with specified urgency

    \param[in] timestamp Time at which event is received.
    \param[in] urgency urgency of handover.
*/
void hdma_HandleExternalReq(uint32 timestamp, hdma_handover_urgency_t urgency);

/*! \brief Handle the Call connect/disconnect event from the State Proxy.

    \param[in] timestamp Time at which event is raised.
    \param[in] isconnect Call is connected or disconnected, 1 = connect, 2 = disconnect
*/

void hdma_HandleCallEvent(uint32 timestamp, bool isconnect);

/*! \brief Handle the Call connect/disconnect event from the State Proxy.

    \param[in] timestamp Time at which event is raised.
    \param[in] isconnect Call is connected or disconnected, 0 = connect, 1 = disconnect
*/

void hdma_HandleScoEvent(uint32 timestamp, bool is_sco_active);

/*! \brief HDMA Message Handler.

    \param[in] task Task.
    \param[in] id Message id
    \param[in] message Message data
*/
void hdma_HandleMessage(Task task, MessageId id, Message message);
#endif

#endif /* INCLUDE_HDMA */
