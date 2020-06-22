/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      HDMA client Notification APIs.
*/

#ifdef INCLUDE_HDMA

#ifndef HDMA_CLIENT_MSGS_H
#define HDMA_CLIENT_MSGS_H

#include "hdma.h"

/*! \brief This function will notify registered client whenever there is a change in handover decision

    \param[in] hdma_msg hdma message to send.
    \param[in] timestamp Time at which event is received. Timestamp is in ms.
    \param[in] reason Reason of handover.
    \param[in] urgency urgency of handover.
*/
void hdma_NotifyHandoverClients(hdma_messages_t hdma_msg,uint32 timestamp, hdma_handover_reason_t reason, hdma_handover_urgency_t urgency);

#endif /* HDMA_CLIENT_MSGS_H */
#endif /* INCLUDE_HDMA */
