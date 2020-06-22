/*!
\copyright  Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_signalling.h
\brief	    Interface to module providing signalling to headset peer device.
*/

#ifndef PEER_SIGNALLING_H_
#define PEER_SIGNALLING_H_

#include "av.h"

#include <marshal_common.h>
#include <domain_message.h>
#include <task_list.h>
#include <rtime.h>

#include <bdaddr.h>
#include <csrtypes.h>
#include <marshal.h>

/*! Flag used on peer signalling states to indicate if the state represents
    an activity that will finish. This is reflected in the lock member of
    \ref peerSigTaskData. */
#define PEER_SIG_STATE_LOCK (0x10)

/*!
    @startuml

    [*] -d-> INITIALISING : Module init

    INITIALISING : Register SDP record for L2CAP
    INITIALISING -d-> DISCONNECTED : CL_SDP_REGISTER_CFM

    DISCONNECTED : No peer connection
    DISCONNECTED --> CONNECTING_ACL : Startup request (ACL not connected)
    DISCONNECTED --> CONNECTING_SDP_SEARCH : Startup request (ACL connected)
    DISCONNECTED --> CONNECTING_REMOTE : Remote L2CAP connect

    CONNECTING_ACL : Creating ACL connection to peer
    CONNECTING_ACL --> DISCONNECTED : Startup request (ACL remote connected)
    CONNECTING_ACL --> DISCONNECTED : Startup request (ACL connect failed)
    CONNECTING_ACL --> CONNECTING_SDP_SEARCH : Startup request (ACL connected)

    CONNECTING_SDP_SEARCH : Performing SDP search for Peer Signaling service
    CONNECTING_SDP_SEARCH --> CONNECTING_LOCAL : SDP success
    CONNECTING_SDP_SEARCH --> DISCONNECTED : SDP error
    CONNECTING_SDP_SEARCH --> DISCONNECTING : Shutdown request
    CONNECTING_SDP_SEARCH --> CONNECTING_SDP_SEARCH : SDP retry

    CONNECTING_LOCAL : Local initiated connection
    CONNECTING_LOCAL --> CONNECTED : L2CAP connect cfm (success)
    CONNECTING_LOCAL --> DISCONNECTED : L2CAP connect cfm (fail)
    CONNECTING_LOCAL --> DISCONNECTED : Remote L2CAP disconnect ind
    CONNECTING_LOCAL --> DISCONNECTING : Shutdown request

    CONNECTING_REMOTE : Remote initiated connection
    CONNECTING_REMOTE --> CONNECTED : L2CAP connect (success)
    CONNECTING_REMOTE --> DISCONNECTED : L2CAP connect (fail)
    CONNECTING_REMOTE --> DISCONNECTED : Remote L2CAP disconnect ind

    CONNECTED : Peer Signaling channel active
    CONNECTED --> DISCONNECTING : Shutdown request
    CONNECTED --> DISCONNECTING : Inactivity timeout
    CONNECTED --> DISCONNECTED : Remote L2CAP disconnect ind

    DISCONNECTING : Waiting for disconnect result
    DISCONNECTING --> DISCONNECTING : L2CAP connect cfm (client shutdown before L2CAP was connected)
    DISCONNECTING --> DISCONNECTED : L2CAP disconnect cfm
    DISCONNECTING --> DISCONNECTED : SDP search cfm

    @enduml
*/

/*! Peer signalling state machine states */
typedef enum
{
    PEER_SIG_STATE_NULL = 0,                                       /*!< Initial state */
    PEER_SIG_STATE_INITIALISING = 1 + PEER_SIG_STATE_LOCK,         /*!< Awaiting L2CAP registration */
    PEER_SIG_STATE_DISCONNECTED = 2,                               /*!< No connection */
    PEER_SIG_STATE_CONNECTING_ACL = 3 + PEER_SIG_STATE_LOCK,       /*!< Connecting ACL */
    PEER_SIG_STATE_CONNECTING_SDP_SEARCH = 4 + PEER_SIG_STATE_LOCK,/*!< Searching for Peer Signalling service */
    PEER_SIG_STATE_CONNECTING_LOCAL = 5 + PEER_SIG_STATE_LOCK,     /*!< Locally initiated connection in progress */
    PEER_SIG_STATE_CONNECTING_REMOTE = 6 + PEER_SIG_STATE_LOCK,    /*!< Remotely initiated connection is progress */
    PEER_SIG_STATE_CONNECTED = 7,                                  /*!< Connnected */
    PEER_SIG_STATE_DISCONNECTING = 8 + PEER_SIG_STATE_LOCK,        /*!< Disconnection in progress */
} appPeerSigState;

/*! Enumeration of peer signalling status codes. */
typedef enum
{
    /*! Operation success. */
    peerSigStatusSuccess,

    /*! Operation fail. */
    peerSigStatusFail,

    /*! Failed to send link key to peer. */
    peerSigStatusLinkKeyTxFail,

    /*! Signalling channel with peer earbud established. */
    peerSigStatusConnected,

    /*! Signalling channel with peer earbud disconnected. */
    peerSigStatusDisconnected,

    /*! Signalling channel with peer earbud disconnected due to link-loss. */
    peerSigStatusLinkLoss,

    /*! Failed to send #AVRCP_PEER_CMD_PAIR_HANDSET_ADDRESS. */
    peerSigStatusPairHandsetTxFail,

    /*! Message channel transmission failed. */
    peerSigStatusMsgChannelTxFail,
    
    /*! Marshalled Message channel transmission failed. */
    peerSigStatusMarshalledMsgChannelTxFail,

    /*! Requested action is already in progress */
    peerSigStatusInProgress,
} peerSigStatus;

/*! Messages that can be sent by peer signalling to client tasks. */

enum peer_signalling_messages
{
    /*! Module initialisation complete */
    PEER_SIG_INIT_CFM = PEER_SIG_MESSAGE_BASE,

    /*! Signalling link to peer established. */
    PEER_SIG_CONNECTION_IND,

    /*! Confirmation of transmission of peer signalling marshalled message channel
     *  transmission.
     *  \note this isn't confirmation of delivery, just transmission. */
    PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM,

    /*! Data received over a peer signalling marshalled message channel. */
    PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND,

    /*! Confirmation of a connection request. */
    PEER_SIG_CONNECT_CFM,

    /*! Confirmation of a disconnect request. */
    PEER_SIG_DISCONNECT_CFM,

    /*! Indication of a Peer link loss. */
    PEER_SIG_LINK_LOSS_IND
};

/*! Channel IDs for peer signalling. */
typedef enum
{
    /*! Channel ID for SCO Forwarding control messages. */
    PEER_SIG_MSG_CHANNEL_SCOFWD,

    /*! Channel ID for State Proxy messages. */
    PEER_SIG_MSG_CHANNEL_STATE_PROXY,

    /*! Channel ID for Earbud application messages.
     *  Current usage is for commands from Primary Earbud to be sent to Secondary Earbud,
     *  at the application level.
     *  Temporarily a lot of the messages will be here, but move down into the TWS topology
     *  component. */
    PEER_SIG_MSG_CHANNEL_APPLICATION,

    PEER_SIG_MSG_CHANNEL_LOGICAL_INPUT_SWITCH,

    /*! Channel ID for Key Sync messages. */
    PEER_SIG_MSG_CHANNEL_KEY_SYNC,

    /*! Channel ID for Fast Pair Account Key Sync messages */
    PEER_SIG_MSG_CHANNEL_FP_ACCOUNT_KEY_SYNC,

#ifdef INCLUDE_MIRRORING
    /*! Channel ID for mirror profile messages. */
    PEER_SIG_MSG_CHANNEL_MIRROR_PROFILE,
#endif

    /*! Channel ID for Peer UI messages. */
    PEER_SIG_MSG_CHANNEL_PEER_UI,

    /*! Channel ID for Topology messages. */
    PEER_SIG_MSG_CHANNEL_TOPOLOGY,

    /*! Channel ID for Peer Link Key messages. */
    PEER_SIG_MSG_CHANNEL_PEER_LINK_KEY,

    /*! Channel ID for VOICE UI messages. */
    PEER_SIG_MSG_CHANNEL_VOICE_UI,

    /* Number of peer sig channels */
    PEER_SIG_MSG_CHANNEL_MAX
} peerSigMsgChannel;

/*! Message sent to clients registered to receive notification of peer signalling
    connection and disconnection events.

    The status can be either #peerSigStatusConnected or
    #peerSigStatusDisconnected.
 */
typedef struct
{
    peerSigStatus status;           /*!< Connected / disconected status (see message description) */
} PEER_SIG_CONNECTION_IND_T;

/*! \brief Confirmation of transmission of a marshalled message channel message. */
typedef struct
{
    peerSigStatus status;           /*!< Result of msg channel transmission. */
    marshal_type_t type;            /*!< Marshal type of the message */
    peerSigMsgChannel channel;      /*!< Msg channel transmission channel used. */
} PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T;

/*! \brief Indication of incoming marshalled message channel message. */
typedef struct
{
    peerSigMsgChannel channel;      /*!< Msg channel transmission channel used. */
    void* msg;                      /*!< Pointer to the message. */
    marshal_type_t type;            /*!< Message type. */
} PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T;

/*! brief Confirmation of the result of a connection request. */
typedef struct
{
    /*! Status of the connection request */
    peerSigStatus status;
} PEER_SIG_CONNECT_CFM_T;

/*! brief Confirmation of the result of a disconnect request. */
typedef struct
{
    /*! Status of the disconnect request */
    peerSigStatus status;
} PEER_SIG_DISCONNECT_CFM_T;

/*! Internal messages used by peer signalling. */
typedef enum
{
    /*! Message to bring up link to peer */
    PEER_SIG_INTERNAL_STARTUP_REQ,

    /*! Message to shut down link to peer */
    PEER_SIG_INTERNAL_SHUTDOWN_REQ,

    /*! Message to teardown peer signalling channel due to inactivity */
    PEER_SIG_INTERNAL_INACTIVITY_TIMER,

} PEER_SIG_INTERNAL_MSG;

/*! Internal message sent to start signalling to a peer */
typedef struct
{
    bdaddr peer_addr;           /*!< Address of peer */
} PEER_SIG_INTERNAL_STARTUP_REQ_T;


/*! \brief Initialise the peer signalling module.
 */
bool appPeerSigInit(Task init_task);

/*! \brief Try and connect peer signalling channel with specified peer earbud.

    A PEER_SIG_CONNECT_CFM will be sent to the client Task with the result of
    the connection attempt.

    The status in PEER_SIG_CONNECT_CFM can have one of these values:
    \li peerSigStatusSuccess
    \li peerSigStatusFail

    \param[in] task The client task that PEER_SIG_CONNECT_CFM will be sent to.
    \param[in] peer_addr BT address of peer earbud to connect.
 */
void appPeerSigConnect(Task task, const bdaddr* peer_addr);

/*! \brief Register to receive peer signalling notifications.

    \param  client_task Task to register to receive peer signalling messages.
 */
void appPeerSigClientRegister(Task client_task);

/*! \brief Unregister task that is currently receiving peer signalling notifications.

    \param  client_task Task that was registered for peer signalling messages.
 */
void appPeerSigClientUnregister(Task client_task);

/*! \brief Disconnect peer signalling channel

    If peer signalling is not connected or is already in the process of
    disconnecting this function does nothing.

    \param[in] task The client task that PEER_SIG_CONNECT_CFM will be sent to.
*/
void appPeerSigDisconnect(Task task);

/*! \brief Register a task for a marshalled message channel(s).
    \param[in] task             Task to associate with the channel(s).
    \param[in] channel          Channel to register.
    \param[in] type_desc        Array of marshal type descriptors for messages.
    \param[in] num_type_desc    Number of entries in the type_desc array.
*/
void appPeerSigMarshalledMsgChannelTaskRegister(Task task, peerSigMsgChannel channel,
                                                const marshal_type_descriptor_t * const * type_desc,
                                                size_t num_type_desc);

/*! \brief Unregister peerSigMsgChannel(s) for the a marshalled message channel.
    \param[in] task             Task associated with the channel(s).
    \param[in] channel          Channel to unregister.
*/
void appPeerSigMarshalledMsgChannelTaskUnregister(Task task,
                                                  peerSigMsgChannel channel);

/*! \brief Transmit a marshalled message channel message to the peer.
    \param[in] task      Task to send confirmation message to.
    \param[in] channel   Channel to transmit on.
    \param[in] msg       Pointer to the message to transmit.
    \param[in] type      Marshal type of the message.
*/
void appPeerSigMarshalledMsgChannelTx(Task task,
                                      peerSigMsgChannel channel,
                                      void* msg, marshal_type_t type);

/*! \brief Cancel any pending marshalled message channel message to the peer.
    \param[in] task      The client's task.
    \param[in] channel   The message channel.
    \param[in] type      Marshal type of the message to cancel.
*/
void appPeerSigMarshalledMsgChannelTxCancelAll(Task task,
                                               peerSigMsgChannel channel,
                                               marshal_type_t type);

/*! \brief Test if peer signalling is connected to a peer.

    \return TRUE if connected; FALSE otherwise.
*/
bool appPeerSigIsConnected(void);

/*! \brief Test if peer signalling is disconnected from a peer.

    \return TRUE if disconnected; FALSE otherwise.
*/
bool appPeerSigIsDisconnected(void);

/*! \brief Get the peer signalling link sink.
    \return The sink.
    The sink should only be used for passing to connection library functions
    that require a sink.
*/
Sink appPeerSigGetSink(void);

/*! \brief Get last transmitted message sequence number sent to 
           the other earbud.

    \return Last transmitted message sequence number
*/
uint8 appPeerSigGetLastTxMsgSequenceNumber(void);

/*! \brief Get last received message sequence number transmitted 
           from the other earbud.

    \return Last received message sequence number
*/
uint8 appPeerSigGetLastRxMsgSequenceNumber(void);


#endif /* PEER_SIGNALLING_H_ */
