/*!
\copyright  Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Interface to TWS Topology use of peer signalling marshalled message channel.
*/

#ifndef TWS_TOPOLOGY_PEER_SIG_H_
#define TWS_TOPOLOGY_PEER_SIG_H_

#include "tws_topology_private.h"
#include "tws_topology_typedef.h"

#include <peer_signalling.h>

enum tws_topology_peer_sig_messages
{
    TWSTOP_PEER_SIG_MSG_RX = TWSTOP_INTERNAL_PEER_SIG_MSG_BASE,
};

typedef struct
{
    union
    {
        tws_topology_remote_rule_event_t remote_event_req;
        /* add new topology message types for clients to receive here */
    } msg;

    marshal_type_t type;
} TWSTOP_PEER_SIG_MSG_RX_T;

/*! \brief Add a local client to use the topology peer signalling channel.

    Clients will be sent
    \li incoming peer signalling messages
    \li PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM messages
 */
void TwsTopology_RegisterPeerSigClient(Task task);

/*! \brief Unregister a local client from using the topology peer signalling channel. */
void TwsTopology_UnregisterPeerSigClient(Task task);

/*! \brief Handle incoming message on the topology peer signalling channel. */
void TwsTopology_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind);

/*! \brief Handle confirmation that message was transmitted on topology peer signalling channel. */
void TwsTopology_HandleMarshalledMsgChannelTxCfm(PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm);

/*! \brief Send command to Secondary Earbud to execute static handover. */
void TwsTopology_SecondaryStaticHandoverCommand(void);

#endif /* TWS_TOPOLOGY_PEER_SIG_H_ */
