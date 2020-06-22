/*!
\copyright  Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_signalling.c
\brief	    Implementation of module providing signalling to headset peer device.
*/

#include "sdp.h"
#include "peer_signalling_private.h"
#include "peer_signalling_config.h"
#include "init.h"
#include "bt_device.h"
#include "adk_log.h"
#include <led_manager.h>

#include <service.h>
#include <connection_manager.h>
#include <bluestack/l2cap_prim.h>

#include <source.h>
#include <panic.h>
#include <message.h>
#include <bdaddr.h>
#include <stdio.h>
#include <stdlib.h>
#include <sink.h>
#include <stream.h>
#include <marshal.h>

//#define DUMP_MARSHALL_DATA
#ifdef DUMP_MARSHALL_DATA
static void dump_buffer(const uint8* bufptr, size_t size)
{
    for (int i=0; i<size; i++)
        DEBUG_LOG("%x", bufptr[i]);
}
#endif

//#define AUTOCONNECT_WORKAROUND

/* A temporary workaround for when a peer_signaling API is called before
   peer_signalling has been connected. This replicates the previous auto
   connect behaviour and may be needed while the new topology is under
   development. */
#ifdef AUTOCONNECT_WORKAROUND
bool peer_sig_enable_auto_connect = FALSE;
bool peer_sig_panic_if_not_connected = FALSE;

#define checkPeerSigConnected(peer_sig, function_name) do { \
    if (!SinkIsValid(peer_sig->link_sink)) \
    { \
        DEBUG_LOG(#function_name " WARNING - peer_sig not connected (yet) state 0x%x", peer_sig->state); \
        \
        if (peer_sig_panic_if_not_connected) \
            Panic(); \
        \
        if (peer_sig_enable_auto_connect) \
        { \
            bdaddr peer_addr; \
            PanicFalse(appDeviceGetSecondaryBdAddr(&peer_addr)); \
            appPeerSigConnect(&peer_sig->task, &peer_addr); \
        } \
    } \
} while(0);
#else
#define checkPeerSigConnected(peer_sig, function_name)
#endif

/******************************************************************************
 * General Definitions
 ******************************************************************************/

/*! Macro to make a message. */
#define MAKE_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);
/*! Macro to make message with variable length for array fields. */
#define MAKE_PEER_SIG_MESSAGE_WITH_LEN(TYPE, LEN) \
    TYPE##_T *message = (TYPE##_T *) PanicUnlessMalloc(sizeof(TYPE##_T) + LEN);

/*! MTU of the L2CAP */
#define PEER_SIG_L2CAP_MTU 672

/*! MTU for each marshal message. Peer signalling pauses writing to the sink
    while space in sink is less than this limit. */
#define PEER_SIG_MARSHAL_MESSAGE_MTU (PEER_SIG_L2CAP_MTU/2)


static void appPeerSigMsgConnectionInd(peerSigStatus status);
static void appPeerSigStartInactivityTimer(void);
static void appPeerSigCancelInactivityTimer(void);
static void appPeerSigMarshal(marshal_msg_channel_data_t *mmcd,
                              marshal_type_t type,
                              void *msg_ptr);
static void appPeerSigMsgConnectCfm(Task task, peerSigStatus status);
static void appPeerSigMsgDisconnectCfm(Task task, peerSigStatus status);
static void appPeerSigSendConnectConfirmation(peerSigStatus status);
static void appPeerSigSendDisconnectConfirmation(peerSigStatus status);
static marshal_msg_channel_data_t* appPeerSigGetChannelData(peerSigMsgChannel channel);

/*!< Peer earbud signalling */
peerSigTaskData app_peer_sig;

static void appPeerSigEnterConnectingAcl(void)
{
    DEBUG_LOG("appPeerSigEnterConnectingAcl");
}

static void appPeerSigExitConnectingAcl(void)
{
    DEBUG_LOG("appPeerSigExitConnectingAcl");
}

static void appPeerSigEnterConnectingLocal(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    DEBUG_LOG("appPeerSigEnterConnectingLocal");

    /* Reset the sdp retry count */
    peer_sig->sdp_search_attempts = 0;
}

static void appPeerSigExitConnectingLocal(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    DEBUG_LOG("appPeerSigExitConnectingLocal");

    /* We have finished (successfully or not) attempting to connect, so
     * we can relinquish our lock on the ACL.  Bluestack will then close
     * the ACL when there are no more L2CAP connections */
    ConManagerReleaseAcl(&peer_sig->peer_addr);
}

static void appPeerSigEnterConnectingRemote(void)
{
    DEBUG_LOG("appPeerSigEnterConnectingRemote");
}

static void appPeerSigExitConnectingRemote(void)
{
    DEBUG_LOG("appPeerSigExitConnectingRemote");
}

static void appPeerSigEnterConnected(void)
{
    Sink sink = appPeerSigGetSink();

    DEBUG_LOG("appPeerSigEnterConnected setting wallclock for UI synchronisation %p", sink);
    LedManager_SetWallclock(sink);

    /* If we have any clients inform them of peer signalling connection */
    appPeerSigMsgConnectionInd(peerSigStatusConnected);

    /* If the connect was because of a client request, send a CFM to
       the client. */
    appPeerSigSendConnectConfirmation(peerSigStatusSuccess);

    appPeerSigStartInactivityTimer();
}

static void appPeerSigExitConnected(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();

    DEBUG_LOG("appPeerSigExitConnected clearing wallclock for UI synchronisation");
    LedManager_SetWallclock((Sink)0);

    appPeerSigCancelInactivityTimer();

    /* If we have any clients inform them of peer signalling disconnection */
    if (peer_sig->link_loss_occurred)
    {
        appPeerSigMsgConnectionInd(peerSigStatusLinkLoss);

        TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(PeerSigGetClientTasks()), PEER_SIG_LINK_LOSS_IND);
    }
    else
    {
        appPeerSigMsgConnectionInd(peerSigStatusDisconnected);
    }

}

static void appPeerSigEnterDisconnected(void)
{
    peerSigMsgChannel channel;
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    DEBUG_LOG("appPeerSigEnterDisconnected");

    /* Clear peer address, as we use that to detect if we've previously reject a peer connection */
    BdaddrSetZero(&peer_sig->peer_addr);

    /* If the disconnect was because of a client request, send a CFM to
       the client. */
    appPeerSigSendDisconnectConfirmation(peerSigStatusSuccess);

    /* If a client requested a connect, send a CFM to the client. */
    appPeerSigSendConnectConfirmation(peerSigStatusFail);

    /* Set the sink in the marshal_common module */
    MarshalCommon_SetSink(NULL);

    /* Reset the sdp retry count */
    peer_sig->sdp_search_attempts = 0;

    /* Reset tx and rx message sequence number */
    peer_sig->tx_seq = 0;
    peer_sig->rx_seq = 0;

    for (channel = 0; channel < PEER_SIG_MSG_CHANNEL_MAX; channel++)
    {
        marshal_msg_channel_data_t *mmcd = appPeerSigGetChannelData(channel);
        if (mmcd->client_task)
        {
            MessageFlushTask(&mmcd->channel_task);
        }
    }

    peer_sig->lock &= ~peer_sig_lock_marshal;
}

static void appPeerSigExitDisconnected(void)
{
    DEBUG_LOG("appPeerSigExitDisconnected");
}

static void appPeerSigEnterInitialising(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();

    DEBUG_LOG("appPeerSigEnterInitialising");

    /* Register a Protocol/Service Multiplexor (PSM) that will be
       used for this application. The same PSM is used at both
       ends. */
    ConnectionL2capRegisterRequest(&peer_sig->task, L2CA_PSM_INVALID, 0);
}

static void appPeerSigExitInitialising(void)
{
    DEBUG_LOG("appPeerSigExitInitialising");

    MessageSend(Init_GetInitTask(), PEER_SIG_INIT_CFM, NULL);
}

static void appPeerSigEnterSdpSearch(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();

    DEBUG_LOG("appPeerSigEnterSdpSearch");

    /* Perform SDP search */
    ConnectionSdpServiceSearchAttributeRequest(&peer_sig->task, &peer_sig->peer_addr, 0x32,
                                               appSdpGetPeerSigServiceSearchRequestSize(), appSdpGetPeerSigServiceSearchRequest(),
                                               appSdpGetPeerSigAttributeSearchRequestSize(), appSdpGetPeerSigAttributeSearchRequest());

    peer_sig->sdp_search_attempts++;
}

static void appPeerSigExitSdpSearch(void)
{
    DEBUG_LOG("appPeerSigExitSdpSearch");
}

static void appPeerSigEnterDisconnecting(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();

    DEBUG_LOG("appPeerSigEnterDisconnecting");

    if (SinkIsValid(peer_sig->link_sink))
    {
        ConnectionL2capDisconnectRequest(&peer_sig->task, peer_sig->link_sink);
    }
}

static void appPeerSigExitDisconnecting(void)
{
    DEBUG_LOG("appPeerSigExitDisconnecting");
}

static appPeerSigState appPeerSigGetState(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    return peer_sig->state;
}

static void appPeerSigSetState(appPeerSigState state)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();

    appPeerSigState old_state = appPeerSigGetState();
    DEBUG_LOG_STATE("appPeerSigSetState, old %x new %x", old_state, state);

    /* Handle state exit functions */
    switch (old_state)
    {
        case PEER_SIG_STATE_INITIALISING:
            appPeerSigExitInitialising();
            break;
        case PEER_SIG_STATE_DISCONNECTED:
            appPeerSigExitDisconnected();
            break;
        case PEER_SIG_STATE_CONNECTING_ACL:
            appPeerSigExitConnectingAcl();
            break;
        case PEER_SIG_STATE_CONNECTING_SDP_SEARCH:
            appPeerSigExitSdpSearch();
            break;
        case PEER_SIG_STATE_CONNECTING_LOCAL:
            appPeerSigExitConnectingLocal();
            break;
        case PEER_SIG_STATE_CONNECTING_REMOTE:
            appPeerSigExitConnectingRemote();
            break;
        case PEER_SIG_STATE_CONNECTED:
            appPeerSigExitConnected();
            break;
        case PEER_SIG_STATE_DISCONNECTING:
            appPeerSigExitDisconnecting();
            break;
        default:
            break;
    }

    /* Set new state */
    peer_sig->state = state;

    /* Update lock according to state */
    if (state & PEER_SIG_STATE_LOCK)
        peer_sig->lock |= peer_sig_lock_fsm;
    else
        peer_sig->lock &= ~peer_sig_lock_fsm;

    /* Handle state entry functions */
    switch (state)
    {
        case PEER_SIG_STATE_INITIALISING:
            appPeerSigEnterInitialising();
            break;
        case PEER_SIG_STATE_DISCONNECTED:
            appPeerSigEnterDisconnected();
            break;
        case PEER_SIG_STATE_CONNECTING_ACL:
            appPeerSigEnterConnectingAcl();
            break;
        case PEER_SIG_STATE_CONNECTING_SDP_SEARCH:
            appPeerSigEnterSdpSearch();
            break;
        case PEER_SIG_STATE_CONNECTING_LOCAL:
            appPeerSigEnterConnectingLocal();
            break;
        case PEER_SIG_STATE_CONNECTING_REMOTE:
            appPeerSigEnterConnectingRemote();
            break;
        case PEER_SIG_STATE_CONNECTED:
            appPeerSigEnterConnected();
            break;
        case PEER_SIG_STATE_DISCONNECTING:
            appPeerSigEnterDisconnecting();
            break;
        default:
            break;
    }
}

static void appPeerSigError(MessageId id)
{
    UNUSED(id);
    DEBUG_LOG("appPeerSigError, state %u, id %u", appPeerSigGetState(), id);
    Panic();
}

/******************************************************************************
 * Messages sent to API clients
 ******************************************************************************/

/*! \brief Send indication of connection state to registered clients. */
static void appPeerSigMsgConnectionInd(peerSigStatus status)
{
    Task next_client = 0;

    while (TaskList_Iterate(TaskList_GetFlexibleBaseTaskList(PeerSigGetClientTasks()), &next_client))
    {
        MAKE_MESSAGE(PEER_SIG_CONNECTION_IND);
        message->status = status;
        MessageSend(next_client, PEER_SIG_CONNECTION_IND, message);
    }
}

/*! \brief Send confirmation of result of marshalled message transmission to client. */
static void appPeerSigMarshalledMsgChannelTxCfm(Task task,
                                                marshal_type_t type,
                                                peerSigStatus status,
                                                peerSigMsgChannel channel)
{
    MAKE_MESSAGE(PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM);
    message->status = status;
    message->channel = channel;
    message->type = type;
    MessageSend(task, PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM, message);
}

static void appPeerSigMsgConnectCfm(Task task, peerSigStatus status)
{
    MAKE_MESSAGE(PEER_SIG_CONNECT_CFM);
    message->status = status;
    MessageSend(task, PEER_SIG_CONNECT_CFM, message);
}

/*! \brief Send confirmation of a connection request to registered clients. */
static void appPeerSigSendConnectConfirmation(peerSigStatus status)
{
    Task next_client = 0;

    /* Send PEER_SIG_CONNECT_CFM to all clients who made a connect request,
       then remove them from the list. */
    while (TaskList_Iterate(TaskList_GetFlexibleBaseTaskList(PeerSigGetConnectTasks()), &next_client))
    {
        appPeerSigMsgConnectCfm(next_client, status);
        TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(PeerSigGetConnectTasks()), next_client);
        next_client = 0;
    }
}

static void appPeerSigMsgDisconnectCfm(Task task, peerSigStatus status)
{
    MAKE_MESSAGE(PEER_SIG_DISCONNECT_CFM);
    message->status = status;
    MessageSend(task, PEER_SIG_DISCONNECT_CFM, message);
}

/*! \brief Send confirmation of a disconnect request to registered clients. */
static void appPeerSigSendDisconnectConfirmation(peerSigStatus status)
{
    Task next_client = 0;

    /* Send PEER_SIG_DISCONNECT_CFM to all clients who made a disconnect
       request, then remove them from the list. */
    while (TaskList_Iterate(TaskList_GetFlexibleBaseTaskList(PeerSigGetDisconnectTasks()), &next_client))
    {
        appPeerSigMsgDisconnectCfm(next_client, status);
        TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(PeerSigGetDisconnectTasks()), next_client);
        next_client = 0;
    }
}

/* Cancel any pending internal startup requests and send a
   PEER_SIG_CONNECT_CFM(failed) to the client(s). */
static void appPeerSigCancelStartup(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    MessageCancelAll(&peer_sig->task, PEER_SIG_INTERNAL_STARTUP_REQ);
    appPeerSigSendConnectConfirmation(peerSigStatusFail);
}

/* Cancel any pending internal shutdown requests and send a
   PEER_SIG_DISCONNECT_CFM(failed) to the client(s). */
static void appPeerSigCancelShutdown(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    MessageCancelAll(&peer_sig->task, PEER_SIG_INTERNAL_SHUTDOWN_REQ);
    appPeerSigSendDisconnectConfirmation(peerSigStatusFail);
}

/******************************************************************************
 * Internal Peer Signalling management functions
 ******************************************************************************/

/*! \brief Start (or connect) the peer signalling channel to a peer. */
static void appPeerSigStartup(Task task, const bdaddr *peer_addr)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();

    DEBUG_LOG("appPeerSigStartup peer_addr [ %04x,%02x,%06lx ]",
                  peer_addr->nap, peer_addr->uap, peer_addr->lap);

    /* Cancel any pending disconnects first. */
    appPeerSigCancelShutdown();

    MAKE_MESSAGE(PEER_SIG_INTERNAL_STARTUP_REQ);
    message->peer_addr = *peer_addr;
    MessageSendConditionally(&peer_sig->task, PEER_SIG_INTERNAL_STARTUP_REQ, message, &peer_sig->lock);

    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(PeerSigGetConnectTasks()), task);
}

/*! \brief Shutdown (or disconnect) the peer signalling channel. */
static void appPeerSigShutdown(Task task)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();

    /* Cancel any queued startup requests first. */
    appPeerSigCancelStartup();

    MessageSend(&peer_sig->task, PEER_SIG_INTERNAL_SHUTDOWN_REQ, NULL);

    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(PeerSigGetDisconnectTasks()), task);
}

/*! \brief Set the inactivity timer.
 */
static void appPeerSigStartInactivityTimer(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    DEBUG_LOG("appPeerSigStartInactivityTimer");

    if (appConfigPeerSignallingChannelTimeoutSecs())
    {
        MessageCancelAll(&peer_sig->task, PEER_SIG_INTERNAL_INACTIVITY_TIMER);
        MessageSendLater(&peer_sig->task, PEER_SIG_INTERNAL_INACTIVITY_TIMER, NULL,
                         appConfigPeerSignallingChannelTimeoutSecs() * 1000);
    }
}

/*! \brief Stop the inactivity timer.
 */
static void appPeerSigCancelInactivityTimer(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    DEBUG_LOG("appPeerSigCancelInactivityTimer");

    MessageCancelAll(&peer_sig->task, PEER_SIG_INTERNAL_INACTIVITY_TIMER);
}

/*! \brief Handle inactivity timer, teardown signalling channel.
 */
static void appPeerSigInactivityTimeout(void)
{
    DEBUG_LOG("appPeerSigInactivityTimeout, state %u", appPeerSigGetState());

    /* Both earbuds have an inactivity timeout, protect against race where
     * the peer signalling link may have just been disconnected by the other earbud */
    switch (appPeerSigGetState())
    {
        case PEER_SIG_STATE_CONNECTED:
            appPeerSigSetState(PEER_SIG_STATE_DISCONNECTING);
            break;

        default:
            break;
    }
}

static uint32 appPeerSigReadUint32(const uint8 *data)
{
    return data[0] + (data[1] << 8) + ((uint32)data[2] << 16) + ((uint32)data[3] << 24);
}

static void appPeerSigWriteUint32(uint8 *data, uint32 val)
{
    data[0] = val & 0xFF;
    data[1] = (val >> 8) & 0xFF;
    data[2] = (val >> 16) & 0xFF;
    data[3] = (val >> 24) & 0xFF;
}


static void appPeerSigHandleInternalStartupRequest(PEER_SIG_INTERNAL_STARTUP_REQ_T *req)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    DEBUG_LOG("appPeerSigHandleInternalStartupRequest, state %u, bdaddr %04x,%02x,%06lx",
               appPeerSigGetState(),
               req->peer_addr.nap, req->peer_addr.uap, req->peer_addr.lap);

    switch (appPeerSigGetState())
    {
        case PEER_SIG_STATE_CONNECTING_ACL:
        case PEER_SIG_STATE_DISCONNECTED:
        {
            /* Check if ACL is now up */
            if (ConManagerIsConnected(&req->peer_addr))
            {
                DEBUG_LOG("appPeerSigHandleInternalStartupRequest, ACL connected");

                /* Initiate if we created the ACL, or was previously rejected (peer_addr is not zero) */
                /*! \todo this mechanism was to avoid poor AVRCP handling of crossovers, we've
                 * since moved to L2CAP for the transport and we know only the primary will be
                 * creating the peer signalling profile, so this could be simplified */
                if (!ConManagerIsAclLocal(&req->peer_addr) || BdaddrIsSame(&peer_sig->peer_addr, &req->peer_addr))
                {
                    DEBUG_LOG("appPeerSigHandleInternalStartupRequest, ACL locally initiated");

                    /* Store address of peer */
                    peer_sig->peer_addr = req->peer_addr;

                    /* Begin the search for the peer signalling SDP record */
                    appPeerSigSetState(PEER_SIG_STATE_CONNECTING_SDP_SEARCH);

                }
                else
                {
                    DEBUG_LOG("appPeerSigHandleInternalStartupRequest, ACL remotely initiated");

                    /* Not locally initiated ACL, move to 'Disconnected' state */
                    appPeerSigSetState(PEER_SIG_STATE_DISCONNECTED);
                }
            }
            else
            {
                if (appPeerSigGetState() == PEER_SIG_STATE_DISCONNECTED)
                {
                    DEBUG_LOG("appPeerSigHandleInternalStartupRequest, ACL not connected, attempt to open ACL");

                    /* Post message back to ourselves, blocked on creating ACL */
                    MAKE_MESSAGE(PEER_SIG_INTERNAL_STARTUP_REQ);
                    message->peer_addr = req->peer_addr;
                    MessageSendConditionally(&peer_sig->task, PEER_SIG_INTERNAL_STARTUP_REQ, message, ConManagerCreateAcl(&req->peer_addr));

                    /* Wait in 'Connecting ACL' state for ACL to open */
                    appPeerSigSetState(PEER_SIG_STATE_CONNECTING_ACL);
                    return;
                }
                else
                {
                    DEBUG_LOG("appPeerSigHandleInternalStartupRequest, ACL failed to open, giving up");

                    /* ACL failed to open, move to 'Disconnected' state */
                    appPeerSigSetState(PEER_SIG_STATE_DISCONNECTED);
                }
            }
        }
        break;

        case PEER_SIG_STATE_CONNECTED:
            /* Already connected, send cfm back to client */
            appPeerSigSendConnectConfirmation(peerSigStatusConnected);
            break;

        default:
            appPeerSigError(PEER_SIG_INTERNAL_STARTUP_REQ);
            break;
    }
}

static void appPeerSigHandleInternalShutdownReq(void)
{
    peerSigTaskData* peer_sig = PeerSigGetTaskData();
    DEBUG_LOG("appPeerSigHandleInternalShutdownReq, state %u", appPeerSigGetState());

    switch (appPeerSigGetState())
    {
        case PEER_SIG_STATE_CONNECTING_SDP_SEARCH:
            {
                /* Cancel SDP search */
                ConnectionSdpTerminatePrimitiveRequest(&peer_sig->task);
            }
            /* Intentional fall-through to go to DISCONNECTING state */
        case PEER_SIG_STATE_CONNECTING_LOCAL:
            /* Intentional fall-through to go to DISCONNECTING state */
        case PEER_SIG_STATE_CONNECTED:
            {
                appPeerSigSetState(PEER_SIG_STATE_DISCONNECTING);
            }
            break;

        case PEER_SIG_STATE_DISCONNECTED:
            {
                appPeerSigSendDisconnectConfirmation(peerSigStatusSuccess);
            }
            break;

        default:
            break;
    }
}

/******************************************************************************
 * Handlers for peer signalling channel L2CAP messages
 ******************************************************************************/


/*! \brief Handle result of L2CAP PSM registration request. */
static void appPeerSigHandleClL2capRegisterCfm(const CL_L2CAP_REGISTER_CFM_T *cfm)
{
    DEBUG_LOG("appPeerSigHandleClL2capRegisterCfm, status %u, psm %u", cfm->status, cfm->psm);
    PanicFalse(appPeerSigGetState() == PEER_SIG_STATE_INITIALISING);

    /* We have registered the PSM used for peer signalling links with
       connection manager, now need to wait for requests to process
       an incoming connection or make an outgoing connection. */
    if (success == cfm->status)
    {
        peerSigTaskData *peer_sig = PeerSigGetTaskData();

        /* Keep a copy of the registered L2CAP PSM, maybe useful later */
        peer_sig->local_psm = cfm->psm;

        /* Copy and update SDP record */
        uint8 *record = PanicUnlessMalloc(appSdpGetPeerSigServiceRecordSize());
        memcpy(record, appSdpGetPeerSigServiceRecord(), appSdpGetPeerSigServiceRecordSize());

        /* Write L2CAP PSM into service record */
        appSdpSetPeerSigPsm(record, cfm->psm);

        /* Register service record */
        ConnectionRegisterServiceRecord(&peer_sig->task, appSdpGetPeerSigServiceRecordSize(), record);
    }
    else
    {
        DEBUG_LOG("appPeerSigHandleClL2capRegisterCfm, failed to register L2CAP PSM");
        Panic();
    }
}

/*! \brief Handle result of the SDP service record registration request. */
static void appPeerSigHandleClSdpRegisterCfm(const CL_SDP_REGISTER_CFM_T *cfm)
{
    DEBUG_LOG("appPeerSigHandleClSdpRegisterCfm, status %d", cfm->status);
    PanicFalse(appPeerSigGetState() == PEER_SIG_STATE_INITIALISING);

    if (cfm->status == sds_status_success)
    {
        /* Move to 'disconnected' state */
        appPeerSigSetState(PEER_SIG_STATE_DISCONNECTED);
    }
    else
        Panic();
}

/*! \brief Extract the remote PSM value from a service record returned by a SDP service search. */
static bool PeerSigGetL2capPSM(const uint8 *begin, const uint8 *end, uint16 *psm, uint16 id)
{
    ServiceDataType type;
    Region record, protocols, protocol, value;
    record.begin = begin;
    record.end   = end;

    while (ServiceFindAttribute(&record, id, &type, &protocols))
        if (type == sdtSequence)
            while (ServiceGetValue(&protocols, &type, &protocol))
            if (type == sdtSequence
               && ServiceGetValue(&protocol, &type, &value)
               && type == sdtUUID
               && RegionMatchesUUID32(&value, (uint32)UUID16_L2CAP)
               && ServiceGetValue(&protocol, &type, &value)
               && type == sdtUnsignedInteger)
            {
                *psm = (uint16)RegionReadUnsigned(&value);
                return TRUE;
            }

    return FALSE;
}

/*! \brief Initiate an L2CAP connection request to the peer. */
static void appPeerSigConnectL2cap(const bdaddr *bd_addr)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();

    static const uint16 l2cap_conftab[] =
    {
        /* Configuration Table must start with a separator. */
        L2CAP_AUTOPT_SEPARATOR,
        /* Flow & Error Control Mode. */
        L2CAP_AUTOPT_FLOW_MODE,
        /* Set to Basic mode with no fallback mode */
            BKV_16_FLOW_MODE( FLOW_MODE_BASIC, 0 ),
        /* Local MTU exact value (incoming). */
        L2CAP_AUTOPT_MTU_IN,
        /*  Exact MTU for this L2CAP connection - 672. */
            PEER_SIG_L2CAP_MTU,
        /* Remote MTU Minumum value (outgoing). */
        L2CAP_AUTOPT_MTU_OUT,
        /*  Minimum MTU accepted from the Remote device. */
            48,
        /* Local Flush Timeout  - Accept Non-default Timeout*/
        L2CAP_AUTOPT_FLUSH_OUT,
            BKV_UINT32R(DEFAULT_L2CAP_FLUSH_TIMEOUT,0),
        /* Configuration Table must end with a terminator. */
        L2CAP_AUTOPT_TERMINATOR
    };

    DEBUG_LOG("appPeerSigConnectL2cap");

    ConnectionL2capConnectRequest(&peer_sig->task,
                                  bd_addr,
                                  peer_sig->local_psm, peer_sig->remote_psm,
                                  CONFTAB_LEN(l2cap_conftab),
                                  l2cap_conftab);

    peer_sig->pending_connects++;
}

/*! \brief Handle the result of a SDP service attribute search.

    The returned attributes are checked to make sure they match the expected
    format of a peer signalling service record.
 */
static void appPeerSigHandleClSdpServiceSearchAttributeCfm(const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *cfm)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();

    DEBUG_LOG("appPeerSigHandleClSdpServiceSearchAttributeCfm, status %d", cfm->status);

    switch (appPeerSigGetState())
    {
        case PEER_SIG_STATE_CONNECTING_SDP_SEARCH:
        {
            /* Find the PSM in the returned attributes */
            if (cfm->status == sdp_response_success)
            {
                if (PeerSigGetL2capPSM(cfm->attributes, cfm->attributes + cfm->size_attributes,
                                         &peer_sig->remote_psm, saProtocolDescriptorList))
                {
                    DEBUG_LOG("appPeerSigHandleClSdpServiceSearchAttributeCfm, peer psm 0x%x", peer_sig->remote_psm);

                    /* Initate outgoing peer L2CAP connection */
                    appPeerSigConnectL2cap(&peer_sig->peer_addr);
                    appPeerSigSetState(PEER_SIG_STATE_CONNECTING_LOCAL);
                }
                else
                {
                    /* No PSM found, malformed SDP record on peer? */
                    DEBUG_LOG("appPeerSigHandleClSdpServiceSearchAttributeCfm, malformed SDP record");
                    appPeerSigSetState(PEER_SIG_STATE_DISCONNECTED);
                }
            }
            else if (cfm->status == sdp_no_response_data)
            {
                /* Peer Earbud doesn't support Peer Signalling service */
                DEBUG_LOG("appPeerSigHandleClSdpServiceSearchAttributeCfm, unsupported");
                appPeerSigSetState(PEER_SIG_STATE_DISCONNECTED);
            }
            else
            {
                /* SDP seach failed, retry? */
                if (ConManagerIsConnected(&peer_sig->peer_addr) && peer_sig->sdp_search_attempts < appConfigPeerSigSdpSearchTryLimit())
                {
                    DEBUG_LOG("appPeerSigHandleClSdpServiceSearchAttributeCfm, retry attempts %d",
                                    peer_sig->sdp_search_attempts);
                    appPeerSigSetState(PEER_SIG_STATE_CONNECTING_SDP_SEARCH);
                }
                else
                {
                    DEBUG_LOG("appPeerSigHandleClSdpServiceSearchAttributeCfm, moving to disconnected state. Retry attempts %d",
                                    peer_sig->sdp_search_attempts);
                    appPeerSigSetState(PEER_SIG_STATE_DISCONNECTED);
                }
            }
        }
        break;

        case PEER_SIG_STATE_DISCONNECTING:
        {
            /* Search cancelled by a client. */
            DEBUG_LOG("appPeerSigHandleClSdpServiceSearchAttributeCfm, cancelled");
            appPeerSigSetState(PEER_SIG_STATE_DISCONNECTED);
        }
        break;

        default:
        {
            DEBUG_LOG("appPeerSigHandleClSdpServiceSearchAttributeCfm, unexpected state 0x%x", appPeerSigGetState());
            Panic();
        }
        break;
    }
}

/*! \brief Handle a L2CAP connection request that was initiated by the remote peer device. */
static void appPeerSigHandleL2capConnectInd(const CL_L2CAP_CONNECT_IND_T *ind)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    DEBUG_LOG("appPeerSigHandleL2capConnectInd, state %u, psm %u", appPeerSigGetState(), ind->psm);
    PanicFalse(ind->psm == peer_sig->local_psm);
    bool accept = FALSE;

    static const uint16 l2cap_conftab[] =
    {
        /* Configuration Table must start with a separator. */
        L2CAP_AUTOPT_SEPARATOR,
        /* Local Flush Timeout  - Accept Non-default Timeout*/
        L2CAP_AUTOPT_FLUSH_OUT,
            BKV_UINT32R(DEFAULT_L2CAP_FLUSH_TIMEOUT,0),
        L2CAP_AUTOPT_TERMINATOR
    };

    switch (appPeerSigGetState())
    {
        case PEER_SIG_STATE_DISCONNECTED:
        {
            /* only accept Peer Signalling connections from paired peer devices. */
            if (appDeviceIsPeer(&ind->bd_addr))
            {
                DEBUG_LOG("appPeerSigHandleL2capConnectInd, accepted");

                /* Move to 'connecting local' state */
                appPeerSigSetState(PEER_SIG_STATE_CONNECTING_REMOTE);

                /* Accept connection */
                accept = TRUE;
            }
            else
            {
                DEBUG_LOG("appPeerSigHandleL2capConnectInd, rejected, unknown peer");

                /* Not a known peer, remember it just in case we're in the middle of pairing */
                peer_sig->peer_addr = ind->bd_addr;
            }
        }
        break;

        default:
        {
            DEBUG_LOG("appPeerSigHandleL2capConnectInd, rejected, state %u", appPeerSigGetState());
        }
        break;
    }

    /* Keep track of this connection */
    peer_sig->pending_connects++;

    /* Send a response accepting or rejcting the connection. */
    ConnectionL2capConnectResponse(&peer_sig->task,     /* The client task. */
                                   accept,                 /* Accept/reject the connection. */
                                   ind->psm,               /* The local PSM. */
                                   ind->connection_id,     /* The L2CAP connection ID.*/
                                   ind->identifier,        /* The L2CAP signal identifier. */
                                   CONFTAB_LEN(l2cap_conftab),
                                   l2cap_conftab);          /* The configuration table. */
}

/*! \brief Handle the result of a L2CAP connection request.

    This is called for both local and remote initiated L2CAP requests.
*/
static void appPeerSigHandleL2capConnectCfm(const CL_L2CAP_CONNECT_CFM_T *cfm)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    DEBUG_LOG("appPeerSigHandleL2capConnectCfm, status %u, pending %u", cfm->status, peer_sig->pending_connects);

    /* Pending connection, return, will get another message in a bit */
    if (l2cap_connect_pending == cfm->status)
    {
        DEBUG_LOG("appPeerSigHandleL2capConnectCfm, connect pending, wait");
        return;
    }

    /* Decrement number of pending connect confirms, panic if 0 */
    PanicFalse(peer_sig->pending_connects > 0);
    peer_sig->pending_connects--;

    switch (appPeerSigGetState())
    {
        case PEER_SIG_STATE_CONNECTING_LOCAL:
        case PEER_SIG_STATE_CONNECTING_REMOTE:
        {
            /* If connection was succesful, get sink, attempt to enable wallclock and move
             * to connected state */
            if (l2cap_connect_success == cfm->status)
            {
                DEBUG_LOG("appPeerSigHandleL2capConnectCfm, connected, conn ID %u, flush remote %u", cfm->connection_id, cfm->flush_timeout_remote);

                PanicNull(cfm->sink);
                peer_sig->link_sink = cfm->sink;
                peer_sig->link_source = StreamSourceFromSink(cfm->sink);

                /* Set the sink in the marshal_common module */
                MarshalCommon_SetSink(peer_sig->link_sink);

                /* Configure the tx (sink) & rx (source) */
                appLinkPolicyUpdateRoleFromSink(peer_sig->link_sink);

                MessageStreamTaskFromSink(peer_sig->link_sink, &peer_sig->task);
                MessageStreamTaskFromSource(peer_sig->link_source, &peer_sig->task);

                PanicFalse(SinkConfigure(peer_sig->link_sink, VM_SINK_MESSAGES, VM_MESSAGES_ALL));
                PanicFalse(SourceConfigure(peer_sig->link_source, VM_SOURCE_MESSAGES, VM_MESSAGES_ALL));

                /* Connection successful; go to connected state */
                appPeerSigSetState(PEER_SIG_STATE_CONNECTED);
            }
            else
            {
                /* Connection failed, if no more pending connections, return to disconnected state */
                if (peer_sig->pending_connects == 0)
                {
                    DEBUG_LOG("appPeerSigHandleL2capConnectCfm, failed, go to disconnected state");

                    appPeerSigSendConnectConfirmation(peerSigStatusFail);

                    appPeerSigSetState(PEER_SIG_STATE_DISCONNECTED);
                }
                else
                {
                    DEBUG_LOG("appPeerSigHandleL2capConnectCfm, failed, wait");
                }
            }
        }
        break;

        case PEER_SIG_STATE_DISCONNECTING:
        {
            /* The L2CAP connect request was cancelled by a SHUTDOWN_REQ
               before we received the L2CAP_CONNECT_CFM. */
            DEBUG_LOG("appPeerSigHandleL2capConnectCfm, cancelled, pending %u", peer_sig->pending_connects);

            if (l2cap_connect_success == cfm->status)
            {
                peer_sig->link_sink = cfm->sink;

                /* Set the sink in the marshal_common module */
                MarshalCommon_SetSink(peer_sig->link_sink);

                /* Re-enter the DISCONNECTING state - this time the L2CAP
                   disconnect request will be sent because link_sink is valid. */

                appPeerSigSetState(PEER_SIG_STATE_DISCONNECTING);
            }
            else
            {
                /* There is no valid L2CAP link to disconnect so go straight
                   to DISCONNECTED. */
                appPeerSigSetState(PEER_SIG_STATE_DISCONNECTED);
            }
        }
        break;

        default:
        {
            /* Connect confirm receive not in connecting state, connection must have failed */
            PanicFalse(l2cap_connect_success != cfm->status);
            DEBUG_LOG("appPeerSigHandleL2capConnectCfm, failed, pending %u", peer_sig->pending_connects);
        }
        break;
    }
}

/*! \brief Handle a L2CAP disconnect initiated by the remote peer. */
static void appPeerSigHandleL2capDisconnectInd(const CL_L2CAP_DISCONNECT_IND_T *ind)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    DEBUG_LOG("appPeerSigHandleL2capDisconnectInd, status %u", ind->status);

    /* Always send reponse */
    ConnectionL2capDisconnectResponse(ind->identifier, ind->sink);

    /* Only change state if sink matches */
    if (ind->sink == peer_sig->link_sink)
    {
        /* Inform clients if link loss and we initiated the original connection */
        if (ind->status == l2cap_disconnect_link_loss && !BdaddrIsZero(&peer_sig->peer_addr))
        {
            DEBUG_LOG("appPeerSigHandleL2capDisconnectInd, link-loss");

            /* Set link-loss flag */
            peer_sig->link_loss_occurred = TRUE;
        }
        else
        {
            /* Clear link-loss flag */
            peer_sig->link_loss_occurred = FALSE;
        }

        appPeerSigSetState(PEER_SIG_STATE_DISCONNECTED);
    }
}

/*! \brief Handle a L2CAP disconnect confirmation.

    This is called for both local and remote initiated disconnects.
 */
static void appPeerSigHandleL2capDisconnectCfm(const CL_L2CAP_DISCONNECT_CFM_T *cfm)
{
    UNUSED(cfm);
    DEBUG_LOG("appPeerSigHandleL2capDisconnectCfm, status %u", cfm->status);

    /* Move to disconnected state if we're in the disconnecting state */
    if (appPeerSigGetState() == PEER_SIG_STATE_DISCONNECTING)
    {
        appPeerSigSetState(PEER_SIG_STATE_DISCONNECTED);
    }
}

/* Peer Signalling L2CAP message format

octet
0       |   Header
1       |   Tx seq number
2       |   opid
3       |   opid
4       |
5       |
6       |
7+n     |

*/
#define PEER_SIG_HEADER_OFFSET 0
#define PEER_SIG_TX_SEQ_NUMBER_OFFSET 1
#define PEER_SIG_OPID_OFFSET 2

#define PEER_SIG_TYPE_MASK      0x3
/*! Marshalled peer signalling message type.
 *  Peer signaling makes no assumptions about the contents, including
 *  not requiring that there be a response message in return. */
#define PEER_SIG_TYPE_MARSHAL   0x1

#define PEER_SIG_GET_HEADER_TYPE(hdr) ((hdr) & PEER_SIG_TYPE_MASK)
#define PEER_SIG_SET_HEADER_TYPE(hdr, type) ((hdr) = (((hdr) & ~PEER_SIG_TYPE_MASK) | type))

/* Marshal type */
#define PEER_SIG_MARSHAL_CHANNELID_OFFSET   2
#define PEER_SIG_MARSHAL_PAYLOAD_OFFSET     6
#define PEER_SIG_MARSHAL_HEADER_SIZE        6

enum peer_sig_reponse
{
    peer_sig_success = 0,
    peer_sig_fail,
};

/*! \brief Claim the requested number of octets from a sink. */
static uint8 *appPeerSigClaimSink(Sink sink, uint16 size)
{
    uint8 *dest = SinkMap(sink);
    uint16 available = SinkSlack(sink);
    uint16 claim_size = 0;
    uint16 already_claimed = SinkClaim(sink, 0);
    uint16 offset = 0;
    
    if (size > (available + already_claimed))
    {
        DEBUG_LOG("appPeerSigMarshalClaimSink attempt to claim too much, size %u available %u, already_claimed %u",
                   size, available, already_claimed);
        return NULL;
    }

    /* We currently have to claim an extra margin in the sink for bytes the
     * marshaller will add, describing the marshalled byte stream. This can
     * accumulate in the sink, so use them up first. */
    if (already_claimed < size)
    {
        claim_size = size - already_claimed;
        offset = SinkClaim(sink, claim_size);
    }
    else
    {
        offset = already_claimed;
    }

//    DEBUG_LOG("appPeerSigClaimSink sz_req %u sz_avail %u dest %p already_claimed %u offset %u",
//                                    size, available, dest, already_claimed, offset);

    if ((NULL == dest) || (offset == 0xffff))
    {
        DEBUG_LOG("appPeerSigClaimSink SinkClaim returned Invalid Offset");
        return NULL;
    }

    return (dest + offset - already_claimed);
}

static marshal_msg_channel_data_t* appPeerSigGetChannelData(peerSigMsgChannel channel)
{
    peerSigTaskData* peer_sig = PeerSigGetTaskData();
    PanicFalse(channel < PEER_SIG_MSG_CHANNEL_MAX);
    return &peer_sig->marshal_msg_channel_state[channel];
}

static void appPeerSigL2capProcessMarshal(const uint8* data, uint16 size)
{
    marshal_msg_channel_data_t* mmcd = NULL;
    uint16 marshal_size = size - PEER_SIG_MARSHAL_HEADER_SIZE; 
    peerSigMsgChannel channel = appPeerSigReadUint32(&data[PEER_SIG_MARSHAL_CHANNELID_OFFSET]);
    marshal_type_t type;
    void* rx_msg;

//    DEBUG_LOG("appPeerSigL2capProcessMarshal channel %u data %p size %u", channel, data, size);

#ifdef DUMP_MARSHALL_DATA
    dump_buffer(data, size);
#endif

    mmcd = appPeerSigGetChannelData(channel);

    if (mmcd->client_task)
    {
        unmarshaller_t unmarshaller = PanicNull(UnmarshalInit(mmcd->type_desc, mmcd->num_type_desc));

        UnmarshalSetBuffer(unmarshaller, &data[PEER_SIG_MARSHAL_PAYLOAD_OFFSET], marshal_size); 

        if (Unmarshal(unmarshaller, &rx_msg, &type))
        {
            MAKE_MESSAGE(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND);

            message->channel = channel;
            message->msg = rx_msg;
            message->type = type;
            MessageSend(mmcd->client_task, PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND, message);
        }
        UnmarshalDestroy(unmarshaller, FALSE);
    }
}

/*! \brief Process incoming peer signalling data packets. */
static void appPeerSigL2capProcessData(void)
{
    uint16 size = 0;
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    Source source = peer_sig->link_source;

    while((size = SourceBoundary(source)) != 0)
    {
        const uint8 *data = SourceMap(source);

        uint8 type = PEER_SIG_GET_HEADER_TYPE(data[PEER_SIG_HEADER_OFFSET]);
        peer_sig->rx_seq = data[PEER_SIG_TX_SEQ_NUMBER_OFFSET];

        /*DEBUG_LOG("appPeerSigL2capProcessData type 0x%x opid 0x%x", type, opid);*/

        switch (type)
        {
        case PEER_SIG_TYPE_MARSHAL:
//            DEBUG_LOG("appPeerSigL2capProcessData type:marshal size %d", size);
            appPeerSigL2capProcessMarshal(data, size);
            break;

        default:
#ifdef DUMP_MARSHALL_DATA
            dump_buffer(data, size);
#endif
            Panic();
            break;
        }

        SourceDrop(source, size);
    }

    SourceClose(source);

    /* Restart in-activity timer */
    if (appPeerSigGetState() == PEER_SIG_STATE_CONNECTED)
        appPeerSigStartInactivityTimer();
}

/*! \brief Handle notifcation of new data arriving on the incoming peer signalling channel. */
static void appPeerSigHandleMMD(const MessageMoreData *mmd)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();

    if (mmd->source == peer_sig->link_source)
    {
        appPeerSigL2capProcessData();
    }
    else
    {
        DEBUG_LOG("MMD received that doesn't match a link");
        if (peer_sig->link_source)
        {
            Panic();
        }
    }
}

/*! Check space in buffer and set/clear lock based on slack vs MTU. */
static void appPeerSigSetLockBasedOnSinkSlack(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    uint16 slack;

    if(!SinkIsValid(peer_sig->link_sink))
    {
        return;
    }

    slack = SinkSlack(peer_sig->link_sink);

    if (slack < PEER_SIG_MARSHAL_MESSAGE_MTU)
    {
        DEBUG_LOG("appPeerSigSetLockBasedOnSinkSlack %u locking", slack);
        peer_sig->lock |= peer_sig_lock_marshal;
    }
    else
    {
        peer_sig->lock &= ~peer_sig_lock_marshal;
    }
}

/*! \brief Handle notification of more space in the peer signalling channel. */
static void appPeerSigHandleMMS(const MessageMoreSpace *mms)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();

    PanicFalse(mms->sink == peer_sig->link_sink);
    appPeerSigSetLockBasedOnSinkSlack();
}

/******************************************************************************
 * Handlers for peer signalling internal messages
 ******************************************************************************/


/*! \brief Write the marshalled message header into a buffer. */
static void appPeerSigWriteMarshalMsgChannelHeader(uint8* bufptr, uint8 tx_seq, peerSigMsgChannel channel)
{
    uint8 hdr = 0;

    bufptr[PEER_SIG_HEADER_OFFSET] = PEER_SIG_SET_HEADER_TYPE(hdr, PEER_SIG_TYPE_MARSHAL);
    bufptr[PEER_SIG_TX_SEQ_NUMBER_OFFSET] = tx_seq;

    appPeerSigWriteUint32(&bufptr[PEER_SIG_MARSHAL_CHANNELID_OFFSET], channel);
}

/*! \brief Attempt to marshal a message to the peer. */
static void appPeerSigMarshal(
                    marshal_msg_channel_data_t *mmcd,
                    marshal_type_t type,
                    void *msg_ptr)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();

    DEBUG_LOG("appPeerSigMarshal chan %u msgptr %p type %u",
                                mmcd->msg_channel_id, msg_ptr, type);

    switch (appPeerSigGetState())
    {
        case PEER_SIG_STATE_CONNECTED:
        {
            marshaller_t marshaller;
            size_t space_required = 0;
            uint8* bufptr = NULL;

            /* get the marshaller for this msg channel */
            marshaller = PanicNull(MarshalInit(mmcd->type_desc, mmcd->num_type_desc));

            /* determine how much space the marshaller will need in order to claim
             * it from the l2cap sink, then try and claim that amount */
            MarshalSetBuffer(marshaller, NULL, 0);
            Marshal(marshaller, msg_ptr, type);
            space_required = MarshalRemaining(marshaller);
            bufptr = appPeerSigClaimSink(peer_sig->link_sink, PEER_SIG_MARSHAL_HEADER_SIZE + space_required);
            PanicNull(bufptr);

            /*Increment peer signalling tx sequence number*/
            peer_sig->tx_seq++;

            /* write the marshal msg header */
            appPeerSigWriteMarshalMsgChannelHeader(bufptr, peer_sig->tx_seq, mmcd->msg_channel_id);

            /* tell the marshaller where in the buffer it can write */
            MarshalSetBuffer(marshaller, &bufptr[PEER_SIG_MARSHAL_PAYLOAD_OFFSET], space_required); 

            /* actually marshal this time and flush the sink to transmit it */
            PanicFalse(Marshal(marshaller, msg_ptr, type));
#ifdef DUMP_MARSHALL_DATA
            dump_buffer(bufptr, PEER_SIG_MARSHAL_HEADER_SIZE + space_required);
#endif
            SinkFlush(peer_sig->link_sink, PEER_SIG_MARSHAL_HEADER_SIZE + space_required);

            MarshalDestroy(marshaller, FALSE);

            /* tell the client the message was sent */
            appPeerSigMarshalledMsgChannelTxCfm(mmcd->client_task, type,
                                                peerSigStatusSuccess, mmcd->msg_channel_id);

            appPeerSigSetLockBasedOnSinkSlack();
        }
        break;

        default:
        {
            DEBUG_LOG("appPeerSigMarshal not connected");
            appPeerSigMarshalledMsgChannelTxCfm(mmcd->client_task, type,
                                                peerSigStatusMarshalledMsgChannelTxFail,
                                                mmcd->msg_channel_id);
        }
        break;
    }
}

/*! \brief Handler for per-channel task marshal messages. */
static void appPeerSigHandleMarshalMessage(Task task, MessageId id, Message message)
{
    marshal_msg_channel_data_t *mmcd =
        STRUCT_FROM_MEMBER(marshal_msg_channel_data_t, channel_task, task);

    appPeerSigMarshal(mmcd, id, (void*)message);
}

/*! \brief Peer signalling task message handler.
 */
static void appPeerSigHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        /* Connection library messages */
        case CL_L2CAP_REGISTER_CFM:
            appPeerSigHandleClL2capRegisterCfm((const CL_L2CAP_REGISTER_CFM_T *)message);
            break;

        case CL_SDP_REGISTER_CFM:
            appPeerSigHandleClSdpRegisterCfm((const CL_SDP_REGISTER_CFM_T *)message);
            break;

        case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM:
            appPeerSigHandleClSdpServiceSearchAttributeCfm((const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *)message);
            return;

        case CL_L2CAP_CONNECT_IND:
            appPeerSigHandleL2capConnectInd((const CL_L2CAP_CONNECT_IND_T *)message);
            break;

        case CL_L2CAP_CONNECT_CFM:
            appPeerSigHandleL2capConnectCfm((const CL_L2CAP_CONNECT_CFM_T *)message);
            break;

        case CL_L2CAP_DISCONNECT_IND:
            appPeerSigHandleL2capDisconnectInd((const CL_L2CAP_DISCONNECT_IND_T *)message);
            break;

        case CL_L2CAP_DISCONNECT_CFM:
            appPeerSigHandleL2capDisconnectCfm((const CL_L2CAP_DISCONNECT_CFM_T *)message);
            break;

        case MESSAGE_MORE_DATA:
            appPeerSigHandleMMD((const MessageMoreData *)message);
            break;

        case MESSAGE_MORE_SPACE:
            appPeerSigHandleMMS((const MessageMoreSpace *)message);
            break;

        /* Internal Peer Signalling Messages */
        case PEER_SIG_INTERNAL_STARTUP_REQ:
            appPeerSigHandleInternalStartupRequest((PEER_SIG_INTERNAL_STARTUP_REQ_T *)message);
            break;

        case PEER_SIG_INTERNAL_INACTIVITY_TIMER:
            appPeerSigInactivityTimeout();
            break;

        case PEER_SIG_INTERNAL_SHUTDOWN_REQ:
            appPeerSigHandleInternalShutdownReq();
            break;

        default:
            DEBUG_LOG("appPeerSigHandleMessage. Unhandled message 0x%04x (%d)",id,id);
            break;
    }
}

static void peerSig_RegisterMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group == PEER_SIG_MESSAGE_GROUP);
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(PeerSigGetClientTasks()), task);
}

/******************************************************************************
 * PUBLIC API
 ******************************************************************************/
/* Initialise the peer signalling module.
 */
bool appPeerSigInit(Task init_task)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    memset(peer_sig, 0, sizeof(*peer_sig));

    /* Set task's message handler */
    peer_sig->task.handler = appPeerSigHandleMessage;

    /* Set initial state and ensure lock is cleared */
    peer_sig->state = PEER_SIG_STATE_NULL;
    peer_sig->lock = peer_sig_lock_none;
    peer_sig->link_loss_occurred = FALSE;

    /* Create the list of peer signalling clients that receive
     * PEER_SIG_CONNECTION_IND messages. */
    TaskList_InitialiseWithCapacity(PeerSigGetClientTasks(), PEER_SIG_CLIENT_TASKS_LIST_INIT_CAPACITY);

    /* Initialise task lists for connect and disconnect requests */
    TaskList_InitialiseWithCapacity(PeerSigGetConnectTasks(), PEER_SIG_CONNECT_TASKS_LIST_INIT_CAPACITY);
    TaskList_InitialiseWithCapacity(PeerSigGetDisconnectTasks(), PEER_SIG_DISCONNECT_TASKS_LIST_INIT_CAPACITY);

    /* Move to 'initialising' state */
    appPeerSigSetState(PEER_SIG_STATE_INITIALISING);

    Init_SetInitTask(init_task);
    return TRUE;
}

/* Try and connect peer signalling channel with specified peer earbud.
*/
void appPeerSigConnect(Task task, const bdaddr* peer_addr)
{
    DEBUG_LOG("appPeerSigConnect - startup");
    appPeerSigStartup(task, peer_addr);
}

/* Register to receive peer signalling notifications. */
void appPeerSigClientRegister(Task client_task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(PeerSigGetClientTasks()), client_task);
}

/* Unregister to stop receiving peer signalling notifications. */
void appPeerSigClientUnregister(Task client_task)
{
    TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(PeerSigGetClientTasks()), client_task);
}

/* Force peer signalling channel to disconnect if it is up. */
void appPeerSigDisconnect(Task task)
{
    DEBUG_LOG("appPeerSigDisconnect");

    appPeerSigShutdown(task);
}

/*! \brief Register a task for a marshalled message channel(s). */
void appPeerSigMarshalledMsgChannelTaskRegister(Task task, peerSigMsgChannel channel,
                                                const marshal_type_descriptor_t * const * type_desc,
                                                size_t num_type_desc)
{
    marshal_msg_channel_data_t* mmcd = appPeerSigGetChannelData(channel);

    mmcd->client_task = task;
    mmcd->channel_task.handler = appPeerSigHandleMarshalMessage;
    mmcd->msg_channel_id = channel;
    mmcd->type_desc = PanicNull((void*)type_desc);
    mmcd->num_type_desc = PanicZero(num_type_desc);

    DEBUG_LOG("MarshalInit %p for task %p", mmcd->type_desc, task);
}

/*! \brief Unregister peerSigMsgChannel(s) for the a marshalled message channel. */
void appPeerSigMarshalledMsgChannelTaskUnregister(Task task, peerSigMsgChannel channel)
{
    marshal_msg_channel_data_t* mmcd = appPeerSigGetChannelData(channel);

    PanicFalse(mmcd->client_task == task);

    memset(mmcd, 0, sizeof(*mmcd));
}

/*! \brief Transmit a marshalled message channel message to the peer. */
void appPeerSigMarshalledMsgChannelTx(Task task,
                                      peerSigMsgChannel channel,
                                      void* msg, marshal_type_t type)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    marshal_msg_channel_data_t* mmcd = appPeerSigGetChannelData(channel);

    DEBUG_LOG("appPeerSigMarshalledMsgChannelTx task %u channel %u type %u", task, channel, type);

    PanicFalse(mmcd->client_task == task);

    checkPeerSigConnected(peer_sig, appPeerSigMarshalledMsgChannelTx);

    /* Send to task, potentially blocked */
    MessageSendConditionally(&mmcd->channel_task, type, msg, &peer_sig->lock);
}

/*! 
    \brief Check if there any pending messages in the channel tasks. 

    \return TRUE: If there is message pending in one of the channel tasks.
            FALSE: There is no message pending in any of the channel tasks.

*/
bool appPeerSigCheckForPendingMarshalledMsg(void)
{
    peerSigMsgChannel channel;
    bool msg_pending = FALSE;

    DEBUG_LOG("appPeerSigCheckForPendingMarshalledMsg");

    /*Check all channels for any pending messages in the message queue */
    for (channel = 0; channel < PEER_SIG_MSG_CHANNEL_MAX; channel++)
    {
        marshal_msg_channel_data_t *mmcd = appPeerSigGetChannelData(channel);
        if (mmcd->client_task && MessagesPendingForTask(&mmcd->channel_task, NULL) != 0)
        {
            msg_pending = TRUE;
            break;
        }
    }

    return msg_pending;
}

void appPeerSigMarshalledMsgChannelTxCancelAll(Task task,
                                               peerSigMsgChannel channel,
                                               marshal_type_t type)
{
    marshal_msg_channel_data_t* mmcd = appPeerSigGetChannelData(channel);
    PanicFalse(mmcd->client_task == task);

    MessageCancelAll(&mmcd->channel_task, type);
}

/*! \brief Test if peer signalling is connected to a peer. */
bool appPeerSigIsConnected(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    return (peer_sig->state == PEER_SIG_STATE_CONNECTED);
}

bool appPeerSigIsDisconnected(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    return (peer_sig->state == PEER_SIG_STATE_DISCONNECTED);
}

Sink appPeerSigGetSink(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    return peer_sig->link_sink;
}

uint8 appPeerSigGetLastTxMsgSequenceNumber(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    return peer_sig->tx_seq;
}

uint8 appPeerSigGetLastRxMsgSequenceNumber(void)
{
    peerSigTaskData *peer_sig = PeerSigGetTaskData();
    return peer_sig->rx_seq;
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(PEER_SIG, peerSig_RegisterMessageGroup, NULL);
