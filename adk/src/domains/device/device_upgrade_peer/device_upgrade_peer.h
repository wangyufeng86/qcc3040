/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    device_upgrade_peer.h
    
DESCRIPTION
    Header file for the Device Upgrade Peer L2CAP channel.
*/

#ifndef DEVICE_UPGRADE_PEER_H_
#define DEVICE_UPGRADE_PEER_H_

#ifdef INCLUDE_DFU_PEER

#include <message.h>
#include "domain_message.h"

#include <rtime.h>
#include <upgrade.h>
#include <task_list.h>

/*! Defines the the peer upgrade client task list initial capacity */
#define PEER_UPGRADE_CLIENT_LIST_INIT_CAPACITY 1

/*! \brief Message IDs from Upgrade Peer to main application task */
enum DeviceUpgradePeer_messages
{
    /*! Module initialisation complete */
    DEVICE_UPGRADE_PEER_INIT_CFM = DEVICE_UPGRADE_PEER_MESSAGE_BASE,
    DEVICE_UPGRADE_PEER_STARTED,
    DEVICE_UPGRADE_PEER_DISCONNECT,
};

/*! Internal messages used by peer . */
typedef enum
{
    /*! Message to bring up link to peer */
    DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ,

} DEVICE_UPGRADE_PEER_INTERNAL_MSG;

/*! Internal message sent to start signalling to a peer */
typedef struct
{
    bdaddr peer_addr;           /*!< Address of peer */
} DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ_T;

typedef enum
{
    DEVICE_UPGRADE_PEER_STATE_INITIALISE = 0,
    DEVICE_UPGRADE_PEER_STATE_IDLE = 1,                 /*!< Awaiting L2CAP registration */
    DEVICE_UPGRADE_PEER_STATE_DISCONNECTED = 2,         /*!< No connection */
    DEVICE_UPGRADE_PEER_STATE_CONNECTING_ACL = 3,       /*!< Connecting ACL */
    DEVICE_UPGRADE_PEER_STATE_CONNECTING_SDP_SEARCH = 4,/*!< Searching for Upgrade Peer service */
    DEVICE_UPGRADE_PEER_STATE_CONNECTING_LOCAL = 5,     /*!< Locally initiated connection in progress */
    DEVICE_UPGRADE_PEER_STATE_CONNECTING_REMOTE = 6,    /*!< Remotely initiated connection in progress */
    DEVICE_UPGRADE_PEER_STATE_CONNECTED = 7,            /*!< Connnected */
    DEVICE_UPGRADE_PEER_STATE_DISCONNECTING = 8,        /*!< Disconnection in progress */
} DeviceUpgradePeerState;

typedef struct
{
    TaskData        task;

    /*! List of tasks to notify of DEVICE UPGRADE activity. */
    TASK_LIST_WITH_INITIAL_CAPACITY(PEER_UPGRADE_CLIENT_LIST_INIT_CAPACITY) client_list;

    DeviceUpgradePeerState     state; /*!< Current state of the state machine */

    /* State related to L2CAP device upgrade peer channel */
    uint16          local_psm;        /*!< L2CAP PSM registered */
    uint16          remote_psm;       /*!< L2CAP PSM registered by peer device */

    /* State related to maintaining upgrade signalling with peer */
    bdaddr peer_addr;   /*!< Bluetooth address of the peer we are signalling */
    bool is_primary;   /*!< Act as a flag to ensure specific activities are done only for Primary Earbud */
    Sink            link_sink;       /*!< The sink of the L2CAP link */
    Source          link_source;     /*!< The source of the L2CAP link */
} deviceUpgradePeerTaskData;

bool appDeviceUpgradePeerEarlyInit(Task init_task);

bool appDeviceUpgradePeerInit(Task init_task);

void DeviceUpgradePeerForceLinkToPeer(void);

void DeviceUpgradePeerL2capSendRequest(uint8 *data_in, uint16 data_size);

/****************************************************************************
NAME
    appDeviceUpgradePeerStillInUSeAfterAbort

BRIEF
    Check if Abort is trigerred and device upgrade peer still in use.
    Return TRUE if abort is initiated and device upgrade peer still in use,
    else FALSE.

*/
bool appDeviceUpgradePeerStillInUSeAfterAbort(void);

/****************************************************************************
NAME
    appUpgradePeerClientRegister()

BRIEF
    Add a client to the DEVICE UPGRADE module

DESCRIPTION
    Messages will be sent to any task registered through this API

    \param task Task to register as a client
 */
void appUpgradePeerClientRegister(Task task);
#else
#define appUpgradePeerClientRegister(tsk) ((void)0)

#endif

#endif

