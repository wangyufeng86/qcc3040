/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_client_msgs.h
\brief      
*/

#ifndef STATE_PROXY_CLIENT_MSGS_H
#define STATE_PROXY_CLIENT_MSGS_H

#include "state_proxy.h"

//size_t istateProxy_GetEventSpecificSize(state_proxy_event_type type);

void stateProxy_MsgStateProxyEventClients(state_proxy_source source,
                                                 state_proxy_event_type type,
                                                 const void* event);

void stateProxy_MsgStateProxyEventInitialStateSent(void);

/*! \brief Send STATE_PROXY_EVENT_INITIAL_STATE_RECEIVED to clients. */
void stateProxy_MsgStateProxyEventInitialStateReceived(void);

/*! \brief Query if any clients are registered for an event type.
    \param type The event type.
 */
bool stateProxy_AnyClientsRegisteredForEvent(state_proxy_event_type type);

#endif /* STATE_PROXY_CLIENT_MSGS_H */
