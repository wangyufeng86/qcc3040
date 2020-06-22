/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      HDMA client Notification.
*/

#ifdef INCLUDE_HDMA

#include "hdma.h"
#include "hdma_client_msgs.h"
#include "hdma_private.h"
#include <logging.h>
#include <panic.h>

/*! \brief This function will notify registered client whenever there is a change in handover decision

    \param[in] hdma_msg hdma message to send.
    \param[in] timestamp Time at which event is received. Timestamp is in ms.
    \param[in] reason Reason of handover.
    \param[in] urgency urgency of handover.
*/
void hdma_NotifyHandoverClients(hdma_messages_t hdma_msg,uint32 timestamp, hdma_handover_reason_t reason, hdma_handover_urgency_t urgency)
{
    Task task = hdma->client_task;

    DEBUG_LOG("hdma_NotifyHandoverClients: Message %u Timestamp %u reason %d urgency %d", hdma_msg, timestamp, reason, urgency);

    hdma_handover_decision_t  *message = PanicUnlessMalloc(sizeof(hdma_handover_decision_t));
    message->timestamp = timestamp;
    message->reason = reason;
    message->urgency = urgency;
    MessageSend(task, hdma_msg, message);
}

#endif /* INCLUDE_HDMA */
