/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_signalling_private.h
\brief      Header file for Peer-Signalling internal data types.
*/

#ifndef PEER_SIGNALLING_PRIVATE_H_
#define PEER_SIGNALLING_PRIVATE_H_

#include "peer_signalling.h"
#include <marshal_common.h>
#include <domain_message.h>
#include <task_list.h>
#include <rtime.h>

#include <bdaddr.h>
#include <csrtypes.h>
#include <source.h>
#include <panic.h>
#include <message.h>
#include <sink.h>
#include <stream.h>
#include <marshal.h>

#define PEER_SIG_CLIENT_TASKS_LIST_INIT_CAPACITY 3
#define PEER_SIG_CONNECT_TASKS_LIST_INIT_CAPACITY 1
#define PEER_SIG_DISCONNECT_TASKS_LIST_INIT_CAPACITY 1

/*! \brief Data held per client task for marshalled message channels. */
typedef struct
{
    /*! Each channel has a task, so messages can be cancelled for each channel
        independently. */
    TaskData channel_task;

    /*! The client for the channel */
    Task client_task;

    /*! The channel's type descriptors */
    const marshal_type_descriptor_t * const * type_desc;

    /*! The number of type descriptors */
    uint8 num_type_desc;

    /*! The channel */
    peerSigMsgChannel msg_channel_id;
} marshal_msg_channel_data_t;

/*! \brief Types of lock used to control receipt of messages by the peer sig task. */
typedef enum
{
    /*! Unlocked. */
    peer_sig_lock_none = 0x00,

    /*! Lock for FSM transition states. */
    peer_sig_lock_fsm = 0x01,

    /*! Lock for busy handling a marshalled message. */
    peer_sig_lock_marshal = 0x02,
} peer_sig_lock;

/*! Peer signalling module state. */
typedef struct
{
    /* State for managing this peer signalling application module */
    TaskData task;                  /*!< Peer Signalling module task */
    appPeerSigState state:5;        /*!< Current state */
    uint16 lock;                    /*!< State machine lock */
    TASK_LIST_WITH_INITIAL_CAPACITY(PEER_SIG_CLIENT_TASKS_LIST_INIT_CAPACITY) peer_sig_client_tasks;/*!< List of tasks registered for notifications
                                         of peer signalling channel availability */
    bool link_loss_occurred;        /*!< TRUE if link-loss has occurred */

    /* State related to maintaining signalling channel with peer */
    bdaddr peer_addr;               /*!< Bluetooth address of the peer we are signalling */

    /* State related to L2CAP peer signalling channel */
    uint16 local_psm;               /*!< L2CAP PSM registered */
    uint16 remote_psm;              /*!< L2CAP PSM registered by peer device */
    uint8 sdp_search_attempts;      /*!< Count of failed SDP searches */
    uint16 pending_connects;
    Sink link_sink;                 /*!< The sink of the L2CAP link */
    Source link_source;             /*!< The source of the L2CAP link */

    /* Transmitted and receieved peer signalling message sequence numbers */
    uint8 tx_seq;
    uint8 rx_seq;

    /*! Per-channel state */
    marshal_msg_channel_data_t marshal_msg_channel_state[PEER_SIG_MSG_CHANNEL_MAX];

    /* Record the Task which first requested a connect or disconnect */
    TASK_LIST_WITH_INITIAL_CAPACITY(PEER_SIG_CONNECT_TASKS_LIST_INIT_CAPACITY) connect_tasks;
    TASK_LIST_WITH_INITIAL_CAPACITY(PEER_SIG_DISCONNECT_TASKS_LIST_INIT_CAPACITY) disconnect_tasks;

} peerSigTaskData;

/*!< Peer earbud signalling */
extern peerSigTaskData app_peer_sig;

/*! Get pointer to the peer signalling modules data structure */
#define PeerSigGetTaskData() (&app_peer_sig)

/*! Get pointer to the peer signalling modules data structure */
#define PeerSigGetClientTasks() (task_list_flexible_t *)(&app_peer_sig.peer_sig_client_tasks)

/*! Get pointer to the peer signalling modules data structure */
#define PeerSigGetConnectTasks() (task_list_flexible_t *)(&app_peer_sig.connect_tasks)

/*! Get pointer to the peer signalling modules data structure */
#define PeerSigGetDisconnectTasks() (task_list_flexible_t *)(&app_peer_sig.disconnect_tasks)

/*! \brief Check if there any pending messages in the channel tasks. 

    \return TRUE: If there is message pending in one of the channel tasks.
            FALSE: There is no message pending in any of the channel tasks.

*/
bool appPeerSigCheckForPendingMarshalledMsg(void);

#endif /* PEER_SIGNALLING_PRIVATE_H_ */
