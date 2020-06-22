/****************************************************************************
Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.


FILE NAME
    device_upgrade_peer.c

DESCRIPTION
    Macros and Routines for establishing the Device Upgrade L2CAP Channel

NOTES

*/

#ifdef INCLUDE_DFU_PEER

#include "device_upgrade_peer.h"
#include <bluestack/l2cap_prim.h>
#include <util.h>
#include <service.h>
#include <stdlib.h>
#include <string.h> /* for memset */
#include "sdp.h"
#include "init.h"
#include "phy_state.h"
#include "bt_device.h"
#include <logging.h>
#include <connection_manager.h>
#include <message.h>
#include "device_upgrade.h"
#include <panic.h>
#include <stream.h>
#include <source.h>
#include <sink.h>

#ifdef INCLUDE_MIRRORING
#include "mirror_profile_protected.h"
#endif

/* L2CAP Connection specific macros */
#define MAX_ATTRIBUTES 0x32
#define EXACT_MTU 672
#define MINIMUM_MTU 48

/* Re-try ACL link establishment on link loss 20 times until the link is established*/
#define ACL_CONNECT_RETRY_LIMIT 20

/* Re-try SDP Search 10 times until the link is established */
#define SDP_SEARCH_RETRY_LIMIT 10

/* Counter incremented during re-try of ACL link establishment on link loss or during SDP search */
uint16 acl_connect_attempts, sdp_connect_attempts = 0;

/* Macro to Check for the SDP status for which retry is needed */
#define SDP_STATUS(x) (x == sdp_no_response_data) || \
                      (x == sdp_con_disconnected) || \
                      (x == sdp_connection_error) || \
                      (x == sdp_search_data_error) || \
                      (x == sdp_search_busy) || \
                      (x == sdp_response_timeout_error) || \
                      (x == sdp_response_out_of_memory) || \
                      (x == sdp_connection_error_page_timeout) || \
                      (x == sdp_connection_error_rej_resources) || \
                      (x == sdp_connection_error_signal_timeout) || \
                      (x == sdp_search_unknown)

/* Macros to get Device Upgrade Peer Task information */
deviceUpgradePeerTaskData device_upgrade_peer;
#define GetDeviceUpgradePeer()      (&device_upgrade_peer)
#define GetDeviceUpgradePeerTask()  (&(GetDeviceUpgradePeer()->task))
#define GetDeviceUpgradeClientTask()  (task_list_flexible_t *)(&(GetDeviceUpgradePeer()->client_list))

/* Macro to check if Device Upgrade Peer still in use */
#define IsDeviceUpgradePeerInUse() (device_upgrade_peer.state > DEVICE_UPGRADE_PEER_STATE_IDLE)


/*! Macro to make a message. */
#define MAKE_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/*! Check if the state is Disconnecting and message is NOT CL_L2CAP_DISCONNECT_CFM */
#define isStateDisconnecting(id) \
    (deviceUpgradePeerGetState() == DEVICE_UPGRADE_PEER_STATE_DISCONNECTING \
     && (id) != CL_L2CAP_DISCONNECT_CFM ? TRUE: FALSE)

/******************************************************************************
 * General Definitions
 ******************************************************************************/

/****************************************************************************
NAME
    appDeviceUpgradePeerNotifyStart

BRIEF
    Notify Peer Device Connection Establishment (Peer Device Start)
    to Application
*/

static void appDeviceUpgradePeerNotifyStart(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(GetDeviceUpgradeClientTask()),
                           DEVICE_UPGRADE_PEER_STARTED);
}

/****************************************************************************
NAME
    appDeviceUpgradePeerNotifyDisconnect

BRIEF
    Notify Peer Device disconnection request to Application
*/
static void appDeviceUpgradePeerNotifyDisconnect(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(GetDeviceUpgradeClientTask()),
                           DEVICE_UPGRADE_PEER_DISCONNECT);
}

/****************************************************************************
NAME
    DeviceUpgradePeerSendL2capConnectFailure

BRIEF
    Send L2CAP peer connection failure to Upgrade Library
*/

static void DeviceUpgradePeerSendL2capConnectFailure(void)
{
    DEBUG_LOG("DeviceUpgradePeer L2cap connection failure request");
    upgrade_peer_connect_state_t l2cap_status = UPGRADE_PEER_CONNECT_FAILED;
    UpgradePeerProcessDataRequest(UPGRADE_PEER_CONNECT_CFM,
                                 (uint8 *)&l2cap_status, sizeof(uint16));
}

/****************************************************************************
NAME
    DeviceUpgradePeerSendL2capConnectSuccess

BRIEF
    Send L2CAP peer connection success to Upgrade Library
*/

static void DeviceUpgradePeerSendL2capConnectSuccess(void)
{
    upgrade_peer_connect_state_t l2cap_status = UPGRADE_PEER_CONNECT_SUCCESS;
#ifdef INCLUDE_MIRRORING
    /*Only for IN-Case-DFU, keep the peer link policy to active so as to improve the speed*/
    if(!appUpgradeIsOutofCaseDfu())
    {
        MirrorProfile_UpdatePeerLinkPolicy(lp_active);
    }
#endif
    UpgradePeerProcessDataRequest(UPGRADE_PEER_CONNECT_CFM,
                                 (uint8 *)&l2cap_status, sizeof(uint16));
}

/****************************************************************************
NAME
    DeviceUpgradePeerConnected

BRIEF
    Handle Device Upgrade Peer Connected State
*/
static void DeviceUpgradePeerConnected(DeviceUpgradePeerState old_state)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();

    DEBUG_LOG("DeviceUpgradePeerConnected");
    if(theDeviceUpgradePeer->is_primary)
    {
        (void)UpgradePeerSetDeviceRolePrimary(TRUE);
        DeviceUpgradePeerSendL2capConnectSuccess();
    }
    else
    {
        (void)UpgradePeerSetDeviceRolePrimary(FALSE);
        UpgradeTransportConnectRequest(GetDeviceUpgradePeerTask(),
                                       TRUE, FALSE);
    }
    /* Notify both Initiator and Peer App that Peer Upgrade has started.*/
    appDeviceUpgradePeerNotifyStart();
    if(old_state == DEVICE_UPGRADE_PEER_STATE_CONNECTING_LOCAL)
    {
        /* We have finished (successfully or not) attempting to connect, so
         * we can relinquish our lock on the ACL.  Bluestack will then close
         * the ACL when there are no more L2CAP connections */
        ConManagerReleaseAcl(&theDeviceUpgradePeer->peer_addr);
    }
}

/****************************************************************************
NAME
    DeviceUpgradePeerDisconnected

BRIEF
    Handle Device Upgrade Peer Disconnected State
*/
static void DeviceUpgradePeerDisconnected(DeviceUpgradePeerState old_state)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();

    DEBUG_LOG("DeviceUpgradePeerDisconnected");

    if(old_state == DEVICE_UPGRADE_PEER_STATE_CONNECTED ||
       old_state == DEVICE_UPGRADE_PEER_STATE_DISCONNECTING)
    {
        if (theDeviceUpgradePeer->is_primary)
        {
            UpgradePeerProcessDataRequest(UPGRADE_PEER_DISCONNECT_IND, NULL, 0);
        }
        else
        {
            UpgradeTransportDisconnectRequest();
        }
    }
    else
    {
        DeviceUpgradePeerSendL2capConnectFailure();
    }

    /*
     * Reset the state variables of peer (those that are maintained here and
     * by library), as these can be inappropriate when DFU has ended/aborted
     * owing to a handover.
     * This is needed because currently DFU data is not marshalled during
     * an handover.
     */
    theDeviceUpgradePeer->is_primary = FALSE;
    UpgradePeerResetStateInfo();

    /* Disconnect process is completed. Moved to IDLE state to accept new
     * connection.
     */
    theDeviceUpgradePeer->state = DEVICE_UPGRADE_PEER_STATE_IDLE;
}


/****************************************************************************
NAME
    DeviceUpgradePeerDisconnecting

BRIEF
    Handle Device Upgrade Peer Disconnecting State
*/
static void DeviceUpgradePeerDisconnecting(void)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();
    DEBUG_LOG("DeviceUpgradePeerDisconnecting");

    ConnectionL2capDisconnectRequest(GetDeviceUpgradePeerTask(),
                                     theDeviceUpgradePeer->link_sink);

}

/****************************************************************************
NAME
    DeviceUpgradePeerInitialise

BRIEF
    Handle Device Upgrade Peer Initialise State
*/
static void DeviceUpgradePeerInitialise(void)
{
    DEBUG_LOG("DeviceUpgradePeerInitialise");

    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();

    /* Register a Protocol/Service Multiplexor (PSM) that will be
       used for this application. The same PSM is used at both
       ends. */
    ConnectionL2capRegisterRequest(&theDeviceUpgradePeer->task,
                                    L2CA_PSM_INVALID, 0);
}

/****************************************************************************
NAME
    DeviceUpgradePeerIdle

BRIEF
    Handle Device Upgrade Peer Idle State
*/
static void DeviceUpgradePeerIdle(void)
{
    DEBUG_LOG("DeviceUpgradePeerIdle");

    /* Send messge to forward to the app to unblock initialisation */
    MessageSend(Init_GetInitTask(), DEVICE_UPGRADE_PEER_INIT_CFM, NULL);

    /* Initialize Upgrade Peer DFU.
     * The primary upgrade key size is 18 words. For peer upgrade currently
     * 6 words need to be used. Same EARBUD_UPGRADE_CONTEXT_KEY PSKey is used
     * with offset EARBUD_UPGRADE_PEER_CONTEXT_OFFSET
     */
    UpgradePeerInit(GetDeviceUpgradePeerTask(), EARBUD_UPGRADE_CONTEXT_KEY,
                                                EARBUD_UPGRADE_PEER_CONTEXT_OFFSET);
}

/****************************************************************************
NAME
    DeviceUpgradePeerSdpSearch

BRIEF
    Handle Device Upgrade Peer Sdp Search State
*/
static void DeviceUpgradePeerSdpSearch(void)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();
    bdaddr peer_bd_addr;

    DEBUG_LOG("DeviceUpgradePeerSdpSearch");
    if(!appDeviceGetPeerBdAddr(&peer_bd_addr))
    {
        /* Disconnect the l2cap connection and put the device state to disconnected */
        theDeviceUpgradePeer->state = DEVICE_UPGRADE_PEER_STATE_DISCONNECTED;
        DeviceUpgradePeerSendL2capConnectFailure();
        return;
    }

    /* Perform SDP search */
    ConnectionSdpServiceSearchAttributeRequest(&theDeviceUpgradePeer->task,
                                               &peer_bd_addr, MAX_ATTRIBUTES,
                         appSdpGetDeviceUpgradePeerServiceSearchRequestSize(),
                         appSdpGetDeviceUpgradePeerServiceSearchRequest(),
                         appSdpGetDeviceUpgradePeerAttributeSearchRequestSize(),
                         appSdpGetDeviceUpgradePeerAttributeSearchRequest());
}

/****************************************************************************
NAME
    deviceUpgradePeerGetState

BRIEF
    Get the Device Upgrade Peer FSM state

DESCRIPTION
    Called to get current Device Upgrade Peer FSM state
*/
static DeviceUpgradePeerState deviceUpgradePeerGetState(void)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();
    return theDeviceUpgradePeer->state;
}

/****************************************************************************
NAME
    DeviceUpgradePeerSetState

BRIEF
    Set the Upgrade Peer FSM state

DESCRIPTION
    Called to change state. Handles calling the state entry and exit
    functions for the new and old states.
*/
static void deviceUpgradePeerSetState(DeviceUpgradePeerState state)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();
    DeviceUpgradePeerState old_state = theDeviceUpgradePeer->state;

    DEBUG_LOG("DeviceUpgradePeerSetState(%d) from %d", state, old_state);

    /* Set new state */
    theDeviceUpgradePeer->state = state;

    /* Handle state functions */
    switch (state)
    {
        case DEVICE_UPGRADE_PEER_STATE_INITIALISE:
            DeviceUpgradePeerInitialise();
            break;
        case DEVICE_UPGRADE_PEER_STATE_IDLE:
            DeviceUpgradePeerIdle();
            break;
        case DEVICE_UPGRADE_PEER_STATE_DISCONNECTED:
            DeviceUpgradePeerDisconnected(old_state);
            break;
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_ACL:
            DEBUG_LOG("DeviceUpgradePeerConnectingAcl");
            break;
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_SDP_SEARCH:
            DeviceUpgradePeerSdpSearch();
            break;
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_LOCAL:
            DEBUG_LOG("DeviceUpgradePeerConnectingLocal");
            break;
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_REMOTE:
            DEBUG_LOG("DeviceUpgradePeerConnectingRemote");
            break;
        case DEVICE_UPGRADE_PEER_STATE_CONNECTED:
            DeviceUpgradePeerConnected(old_state);
            break;
        case DEVICE_UPGRADE_PEER_STATE_DISCONNECTING:
            DeviceUpgradePeerDisconnecting();
            break;
        default:
            break;
    }
}

/****************************************************************************
NAME
    DeviceUpgradePeerHandleInternalShutdownReq

BRIEF
    Disconnect the L2CAP connection with Peer device

DESCRIPTION
    Called to disconnect the L2CAP connection with Peer device and change the
    state of the device
*/
static void DeviceUpgradePeerHandleInternalShutdownReq(void)
{
    DEBUG_LOG("DeviceUpgradePeerHandleInternalShutdownReq, state %u",
               deviceUpgradePeerGetState());

    /* Notify application of Peer device disconnect */
    appDeviceUpgradePeerNotifyDisconnect();

    switch (deviceUpgradePeerGetState())
    {
        case DEVICE_UPGRADE_PEER_STATE_CONNECTED:
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_ACL:
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_SDP_SEARCH:
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_LOCAL:
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_REMOTE:
             deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_DISCONNECTING);
             break;
        default:
             break;
    }
}

/****************************************************************************
NAME
    DeviceUpgradePeerError

BRIEF
    Disconnect the l2cap connection when device upgrade error is called and put
    the state to disconnected
*/
static void DeviceUpgradePeerError(MessageId id)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();

    UNUSED(id);
    DEBUG_LOG("DeviceUpgradePeerError, state %u, id %u",
               deviceUpgradePeerGetState(), id);

    theDeviceUpgradePeer->state = DEVICE_UPGRADE_PEER_STATE_DISCONNECTED;
    DeviceUpgradePeerSendL2capConnectFailure();
}

/****************************************************************************
NAME
    DeviceUpgradePeerStartup

BRIEF
    Request for initiating the L2CAP connection with peer device
*/
static void DeviceUpgradePeerStartup(const bdaddr *peer_addr)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();

    MAKE_MESSAGE(DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ);
    message->peer_addr = *peer_addr;
    MessageSend(&theDeviceUpgradePeer->task,
                DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ, message);
}

/****************************************************************************
NAME
    DeviceUpgradePeerHandleInternalStartupRequest

BRIEF
    Request to create a L2CAP connection with the Peer device

DESCRIPTION
    Called to create a L2CAP connection with the Peer device for device upgrade
*/
static void DeviceUpgradePeerHandleInternalStartupRequest(DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ_T *req)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();
    DEBUG_LOG("DeviceUpgradePeerHandleInternalStartupRequest, state %u, bdaddr %04x,%02x,%06lx",
               deviceUpgradePeerGetState(),
               req->peer_addr.nap, req->peer_addr.uap, req->peer_addr.lap);

    switch (deviceUpgradePeerGetState())
    {
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_ACL:
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_SDP_SEARCH:
        {
            /* Check if ACL is now up */
            if (ConManagerIsConnected(&req->peer_addr))
            {
                DEBUG_LOG("DeviceUpgradePeerHandleInternalStartupRequest, ACL connected");

                /* Initiate if we created the ACL, or was previously rejected */
                if (ConManagerIsAclLocal(&req->peer_addr) ||
                    BdaddrIsSame(&theDeviceUpgradePeer->peer_addr,
                                 &req->peer_addr))
                {
                    if(deviceUpgradePeerGetState() == DEVICE_UPGRADE_PEER_STATE_CONNECTING_ACL)
                    {
                        DEBUG_LOG("DeviceUpgradePeerHandleInternalStartupRequest, ACL locally initiated");

                        /* Store address of peer */
                        theDeviceUpgradePeer->peer_addr = req->peer_addr;

                        /* Begin the search for the peer signalling SDP record */
                        deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_CONNECTING_SDP_SEARCH);
                    }
                    else
                    {
                        sdp_connect_attempts += 1;
                        if(sdp_connect_attempts <= SDP_SEARCH_RETRY_LIMIT)
                        {
                            /* Try SDP Search again */
                            DEBUG_LOG("SDP Search retrying again, %d", sdp_connect_attempts);
                            deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_CONNECTING_SDP_SEARCH);
                        }
                        else
                        {
                            /* Peer Earbud doesn't support Device Upgrade service */
                            DEBUG_LOG("Device Upgrade SDP Service unsupported, go to disconnected state");
                            deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_DISCONNECTED);
                        }
                    }
                }
                else
                {
                    DEBUG_LOG("DeviceUpgradePeerHandleInternalStartupRequest, ACL remotely initiated");

                    /* Not locally initiated ACL, move to 'Disconnected' state */
                    deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_DISCONNECTED);
                }
            }
            else
            {
                if(acl_connect_attempts >= ACL_CONNECT_RETRY_LIMIT)
                {
                    DEBUG_LOG("DeviceUpgradePeerHandleInternalStartupRequest, ACL failed to open, giving up");
                    /* ACL failed to open, move to 'Disconnected' state */
                    deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_DISCONNECTED);
                }
                else
                {
                    acl_connect_attempts++;
                    DEBUG_LOG("ACL Connection retrying again, %d", acl_connect_attempts);
                     /* Post message back to ourselves, blocked on creating ACL */
                    MAKE_MESSAGE(DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ);
                    message->peer_addr = req->peer_addr;
                    MessageSendConditionally(&theDeviceUpgradePeer->task,
                          DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ, message,
                          ConManagerCreateAcl(&req->peer_addr));
                    return;
                }
            }
        }
        break;
        case DEVICE_UPGRADE_PEER_STATE_IDLE:
        {
            DEBUG_LOG("DeviceUpgradePeerHandleInternalStartupRequest, ACL not connected, attempt to open ACL");

            /* Post message back to ourselves, blocked on creating ACL */
            MAKE_MESSAGE(DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ);
            message->peer_addr = req->peer_addr;
            MessageSendConditionally(&theDeviceUpgradePeer->task,
                         DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ,
                         message, ConManagerCreateAcl(&req->peer_addr));

            /* Wait in 'Connecting ACL' state for ACL to open */
            deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_CONNECTING_ACL);
            return;
        }

        case DEVICE_UPGRADE_PEER_STATE_CONNECTED:
            /* Already connected, just ignore startup request */
            break;

        default:
            DeviceUpgradePeerError(DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ);
            break;
    }

    /* Cancel any other startup requests */
    MessageCancelAll(&theDeviceUpgradePeer->task,
                     DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ);
}


/******************************************************************************
 * Handlers for upgrade peer channel L2CAP messages
 ******************************************************************************/

/****************************************************************************
NAME
    DeviceUpgradePeerHandleL2capRegisterCfm

BRIEF
    Handle result of L2CAP PSM registration request
*/

static void DeviceUpgradePeerHandleL2capRegisterCfm(const CL_L2CAP_REGISTER_CFM_T *cfm)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();

    DEBUG_LOG("DeviceUpgradePeerHandleL2capRegisterCfm, status %u, psm %u",
                cfm->status, cfm->psm);

    /* We have registered the PSM used for SCO forwarding links with
       connection manager, now need to wait for requests to process
       an incoming connection or make an outgoing connection. */
    if (success == cfm->status)
    {
        /* Keep a copy of the registered L2CAP PSM, maybe useful later */
        theDeviceUpgradePeer->local_psm = cfm->psm;

        /* Copy and update SDP record */
        uint8 *record = PanicUnlessMalloc(appSdpGetDeviceUpgradePeerServiceRecordSize());
        memcpy(record, appSdpGetDeviceUpgradePeerServiceRecord(),
                       appSdpGetDeviceUpgradePeerServiceRecordSize());

        /* Write L2CAP PSM into service record */
        appSdpSetDeviceUpgradePeerPsm(record, cfm->psm);

        /* Register service record */
        ConnectionRegisterServiceRecord(GetDeviceUpgradePeerTask(),
                         appSdpGetDeviceUpgradePeerServiceRecordSize(), record);
    }
    else
    {
     /* Since the L2CAP PSM registration failed, we are moving the state
      * to DISCONNECTED, and won't accept any L2CAP channel creation request
      */
        DEBUG_LOG("DeviceUpgradePeerHan'dleL2capRegisterCfm, failed to register L2CAP PSM");
        theDeviceUpgradePeer->state = DEVICE_UPGRADE_PEER_STATE_DISCONNECTED;
    }
}

/****************************************************************************
NAME
    DeviceUpgradePeerHandleClSdpRegisterCfm

BRIEF
    Handle result of the SDP service record registration request
*/
static void DeviceUpgradePeerHandleClSdpRegisterCfm(const CL_SDP_REGISTER_CFM_T *cfm)
{
    DEBUG_LOG("DeviceUpgradePeerHandleClSdpRegisterCfm, status %d", cfm->status);

    if (cfm->status == sds_status_success)
    {
        /* Move to 'idle' state */
        deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_IDLE);
    }
    else
    {
     /* Since the SDP service record registration failed, we are moving the state
      * to DISCONNECTED, and won't accept any L2CAP channel creation request
      */
        deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();

        DEBUG_LOG("DeviceUpgradePeerHan'dleL2capRegisterCfm, failed to register L2CAP PSM");
        theDeviceUpgradePeer->state = DEVICE_UPGRADE_PEER_STATE_DISCONNECTED;
    }
}

/****************************************************************************
NAME
    DeviceUpgradePeerConnectL2cap

BRIEF
    Initiate an L2CAP connection request to the peer
*/
static void DeviceUpgradePeerConnectL2cap(const bdaddr *bd_addr)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();
    bdaddr peer_bd_addr;

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
            EXACT_MTU,
        /* Remote MTU Minumum value (outgoing). */
            L2CAP_AUTOPT_MTU_OUT,
        /*  Minimum MTU accepted from the Remote device. */
            MINIMUM_MTU,
         /* Local Flush Timeout  - Accept Non-default Timeout*/
            L2CAP_AUTOPT_FLUSH_OUT,
            BKV_UINT32R(DEFAULT_L2CAP_FLUSH_TIMEOUT,0),

        /* Configuration Table must end with a terminator. */
            L2CAP_AUTOPT_TERMINATOR
    };

    DEBUG_LOG("appDeviceUpgradePeerConnectL2cap");

    /* If the below scenarios fails, then send connect failure to upgrade library */
    if(!appDeviceGetPeerBdAddr(&peer_bd_addr) ||
       !BdaddrIsSame(bd_addr, &peer_bd_addr))
    {
        theDeviceUpgradePeer->state = DEVICE_UPGRADE_PEER_STATE_DISCONNECTED;
        DeviceUpgradePeerSendL2capConnectFailure();
        return;
    }

    ConnectionL2capConnectRequest(&theDeviceUpgradePeer->task,
                                  bd_addr,
                                  theDeviceUpgradePeer->local_psm,
                                  theDeviceUpgradePeer->remote_psm,
                                  CONFTAB_LEN(l2cap_conftab),
                                  l2cap_conftab);
}

/****************************************************************************
NAME
    DeviceUpgradePeerGetL2capPSM

BRIEF
    Extract the remote PSM value from a service record
    returned by a SDP service search
*/
static bool DeviceUpgradePeerGetL2capPSM(const uint8 *begin,
                                       const uint8 *end, uint16 *psm, uint16 id)
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

/****************************************************************************
NAME
    DeviceUpgradePeerHandleClSdpServiceSearchAttributeCfm

BRIEF
    Handle the result of a SDP service attribute search
*/
static void DeviceUpgradePeerHandleClSdpServiceSearchAttributeCfm(const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *cfm)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();

    DEBUG_LOG("deviceUpgradePeerHandleClSdpServiceSearchAttributeCfm, status %d",
              cfm->status);

    switch (deviceUpgradePeerGetState())
    {
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_SDP_SEARCH:
        {
            /* Find the PSM in the returned attributes */
            if (cfm->status == sdp_response_success)
            {
                if (DeviceUpgradePeerGetL2capPSM(cfm->attributes,
                                                 cfm->attributes +
                                                 cfm->size_attributes,
                   &theDeviceUpgradePeer->remote_psm, saProtocolDescriptorList))
                {
                    DEBUG_LOG("deviceUpgradePeerHandleClSdpServiceSearchAttributeCfm, peer psm 0x%x", theDeviceUpgradePeer->remote_psm);

                    /* Initate outgoing peer L2CAP connection */
                    DeviceUpgradePeerConnectL2cap(&theDeviceUpgradePeer->peer_addr);
                    deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_CONNECTING_LOCAL);
                }
                else
                {
                    /* No PSM found, malformed SDP record on peer? */
                    DEBUG_LOG("deviceUpgradePeerHandleClSdpServiceSearchAttributeCfm, malformed SDP record");
                    deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_DISCONNECTED);
                }
            }
            /* Check if retry needed for valid status */
            else if (SDP_STATUS(cfm->status))
            {
                MAKE_MESSAGE(DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ);
                message->peer_addr = cfm->bd_addr;
                /* Try the SDP Search again after 1 sec */
                MessageSendLater(&theDeviceUpgradePeer->task,
                    DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ, message, D_SEC(1));
            }
            else
            {
                /* SDP search failed */
                DEBUG_LOG("deviceUpgradePeerHandleClSdpServiceSearchAttributeCfm, SDP search failed");
                deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_DISCONNECTED);
            }
        }
        break;

        default:
        {
            DEBUG_LOG("deviceUpgradePeerHandleClSdpServiceSearchAttributeCfm, unexpected state 0x%x status", deviceUpgradePeerGetState(), cfm->status);
         /* Silently ignore, not the end of the world */
        }
        break;
    }
}

/****************************************************************************
NAME
    DeviceUpgradePeerHandleL2capConnectInd

BRIEF
    Handle a L2CAP connection request that was initiated by the remote peer device
*/
static void DeviceUpgradePeerHandleL2capConnectInd(const CL_L2CAP_CONNECT_IND_T *ind)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();
    bool accept = FALSE;

    DEBUG_LOG("DeviceUpgradePeerL2capConnectInd, state %u, psm %u",
                deviceUpgradePeerGetState(), ind->psm);

    /* If the PSM doesn't macthes, then send l2cap failure message to upgrade
     * library and put the device in disconnected state
     */
    if(!(ind->psm == theDeviceUpgradePeer->local_psm))
    {
        theDeviceUpgradePeer->state = DEVICE_UPGRADE_PEER_STATE_DISCONNECTED;
        DeviceUpgradePeerSendL2capConnectFailure();
        return;
    }

    static const uint16 l2cap_conftab[] =
    {
        /* Configuration Table must start with a separator. */
        L2CAP_AUTOPT_SEPARATOR,
        /* Local Flush Timeout  - Accept Non-default Timeout*/
        L2CAP_AUTOPT_FLUSH_OUT,
            BKV_UINT32R(DEFAULT_L2CAP_FLUSH_TIMEOUT,0),
        L2CAP_AUTOPT_TERMINATOR
    };

    switch (deviceUpgradePeerGetState())
    {
        case DEVICE_UPGRADE_PEER_STATE_IDLE:
        {
            /* only accept Peer connections from paired peer devices. */
            if (appDeviceIsPeer(&ind->bd_addr))
            {
                DEBUG_LOG("DeviceUpgradePeerHandleL2capConnectInd, accepted");

                deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_CONNECTING_REMOTE);

                /* Accept connection */
                accept = TRUE;
            }
            else
            {
                DEBUG_LOG("DeviceUpgradePeerHandleL2capConnectInd, rejected, unknown peer");
            }
        }
        break;

        default:
        {
            DEBUG_LOG("DeviceUpgradePeerHandleL2capConnectInd, rejected, state %u",
                       deviceUpgradePeerGetState());
        }
        break;
    }

    /* Send a response accepting or rejcting the connection. */
    ConnectionL2capConnectResponse(&theDeviceUpgradePeer->task,     /* The client task. */
                                   accept,                 /* Accept/reject the connection. */
                                   ind->psm,               /* The local PSM. */
                                   ind->connection_id,     /* The L2CAP connection ID.*/
                                   ind->identifier,        /* The L2CAP signal identifier. */
                                   CONFTAB_LEN(l2cap_conftab),
                                   l2cap_conftab);          /* The configuration table. */
}

/****************************************************************************
NAME
    DeviceUpgradePeerHandleL2capConnectCfm

BRIEF
    Handle a L2CAP connection request that was initiated by the remote
    peer device. This is called for both local and remote initiated
    L2CAP requests
*/
static void DeviceUpgradePeerHandleL2capConnectCfm(const CL_L2CAP_CONNECT_CFM_T *cfm)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();

    DEBUG_LOG("DeviceUpgradePeerHandleL2capConnectCfm, status %u", cfm->status);

    switch (deviceUpgradePeerGetState())
    {
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_LOCAL:
        case DEVICE_UPGRADE_PEER_STATE_CONNECTING_REMOTE:
            /* If connection was succesful, get sink, attempt to enable wallclock and move
             * to connected state */
            if (l2cap_connect_success == cfm->status)
            {
                DEBUG_LOG("DeviceUpgradePeerHandleL2capConnectCfm, connected, conn ID %u, flush remote %u",
                          cfm->connection_id, cfm->flush_timeout_remote);


                theDeviceUpgradePeer->link_sink = cfm->sink;
                theDeviceUpgradePeer->link_source = StreamSourceFromSink(cfm->sink);

                /* Configure the tx (sink) & rx (source) */
                appLinkPolicyUpdateRoleFromSink(theDeviceUpgradePeer->link_sink);

                MessageStreamTaskFromSink(theDeviceUpgradePeer->link_sink,
                                          &theDeviceUpgradePeer->task);
                MessageStreamTaskFromSource(theDeviceUpgradePeer->link_source,
                                          &theDeviceUpgradePeer->task);

                /* Disconnect the l2cap connection if Sink Configuration fails*/
                if(!SinkConfigure(theDeviceUpgradePeer->link_sink,
                                         VM_SINK_MESSAGES, VM_MESSAGES_ALL)||
                   !SourceConfigure(theDeviceUpgradePeer->link_source,
                                           VM_SOURCE_MESSAGES, VM_MESSAGES_ALL))
                {
                    theDeviceUpgradePeer->state = DEVICE_UPGRADE_PEER_STATE_DISCONNECTED;
                    DeviceUpgradePeerSendL2capConnectFailure();
                    return;
                }

                deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_CONNECTED);
            }
            else
            {
                /* Connection failed, if no more pending connections, return to disconnected state */
                if (cfm->status >= l2cap_connect_failed)
                {
                    DEBUG_LOG("DeviceUpgradePeerHandleL2capConnectCfm, failed, go to disconnected state");
                    deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_DISCONNECTED);
                }
                /* Pending connection, return, will get another message in a bit */
                else
                {
                    DEBUG_LOG("DeviceUpgradePeerHandleL2capConnectCfm, L2CAP connection is Pending");
                    return;
                }
            }
            break;

        default:
            /* Connect confirm receive not in connecting state, connection must have failed */
            if(l2cap_connect_success != cfm->status)
            {
                theDeviceUpgradePeer->state = DEVICE_UPGRADE_PEER_STATE_DISCONNECTED;
                DeviceUpgradePeerSendL2capConnectFailure();
            }
            DEBUG_LOG("DeviceUpgradePeerHandleL2capConnectCfm, failed");
            break;
    }
}

/****************************************************************************
NAME
    DeviceUpgradePeerHandleL2capDisconnectInd

BRIEF
    Handle a L2CAP disconnect initiated by the remote peer
*/
static void DeviceUpgradePeerHandleL2capDisconnectInd(const CL_L2CAP_DISCONNECT_IND_T *ind)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();
    DEBUG_LOG("DeviceUpgradePeerHandleL2capDisconnectInd, status %u", ind->status);

    /* Always send reponse */
    ConnectionL2capDisconnectResponse(ind->identifier, ind->sink);

    /* Only change state if sink matches */
    if (ind->sink == theDeviceUpgradePeer->link_sink)
    {
        /* Print if there is a link loss. Currently, we don't handle link loss during upgrade */
        if (ind->status == l2cap_disconnect_link_loss &&
                           !BdaddrIsZero(&theDeviceUpgradePeer->peer_addr))
        {
            DEBUG_LOG("DeviceUpgradePeerHandleL2capDisconnectInd, link-loss");
        }

        deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_DISCONNECTED);
    }
}

/****************************************************************************
NAME
    DeviceUpgradePeerHandleL2capDisconnectCfm

BRIEF
    Handle a L2CAP disconnect confirmation
*/
static void DeviceUpgradePeerHandleL2capDisconnectCfm(const CL_L2CAP_DISCONNECT_CFM_T *cfm)
{
    UNUSED(cfm);
    DEBUG_LOG("DeviceUpgradePeerHandleL2capDisconnectCfm, status %u", cfm->status);

    /* Move to DISCONNECTED  state if we're in the disconnecting state */
    if (deviceUpgradePeerGetState() == DEVICE_UPGRADE_PEER_STATE_DISCONNECTING)
    {
        deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_DISCONNECTED);
    }
}

/****************************************************************************
NAME
    DeviceUpgradePeerClaimSink

BRIEF
    Claim the requested number of octets from a sink
*/
static uint8 *DeviceUpgradePeerClaimSink(Sink sink, uint16 size)
{
    uint8 *dest = SinkMap(sink);
    uint16 available = SinkSlack(sink);
    uint16 claim_size = 0;
    uint16 already_claimed = SinkClaim(sink, 0);
    uint16 offset = 0;

    if (size > available)
    {
        DEBUG_LOG("DeviceUpgradePeerClaimSink attempt to claim too much %u", size);
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

    if ((NULL == dest) || (offset == 0xffff))
    {
        DEBUG_LOG("DeviceUpgradePeerClaimSink SinkClaim returned Invalid Offset");
        return NULL;
    }

    return (dest + offset - already_claimed);
}

/****************************************************************************
NAME
    DeviceUpgradePeerL2capSendRequest

BRIEF
    Send upgrade peer data packets to peer device
*/
void DeviceUpgradePeerL2capSendRequest(uint8 *data_in, uint16 data_size)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();
    Sink sink = theDeviceUpgradePeer->link_sink;

    uint8 *data = DeviceUpgradePeerClaimSink(sink, data_size );
    if (data)
    {
        memcpy(data,data_in,data_size);
        SinkFlush(sink, data_size);
    }
}

/****************************************************************************
NAME
    DeviceUpgradePeerL2capProcessData

BRIEF
    Process incoming upgrade peer data packets
*/
static void DeviceUpgradePeerL2capProcessData(void)
{
    uint16 size;
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();
    Source source = theDeviceUpgradePeer->link_source;

    while((size = SourceBoundary(source)) != 0)
    {
        uint8 *data = (uint8*)SourceMap(source);

        if(theDeviceUpgradePeer->is_primary)
        {
             UpgradePeerProcessDataRequest(UPGRADE_PEER_GENERIC_MSG, data, size);
        }
        else
        {
            UpgradeProcessDataRequest(size, data);
        }

        SourceDrop(source, size);
    }
}

/****************************************************************************
NAME
    DeviceUpgradePeerHandleMMD

BRIEF
    Handle MessageMoreData request from peer device
*/
static void DeviceUpgradePeerHandleMMD(const MessageMoreData *mmd)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();

    if (mmd->source == theDeviceUpgradePeer->link_source)
    {
        DeviceUpgradePeerL2capProcessData();
    }
    else
    {
        DEBUG_LOG("MMD received that doesn't match a link");
    }
}

/****************************************************************************
NAME
    DeviceUpgradePeerHandleMessage

BRIEF
    Message Handler

DESCRIPTION
    This function is the main message handler for Upgrade Peer, every
    message is handled in it's own seperate handler function.
*/
static void DeviceUpgradePeerHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    /* If the state is Disconnecting, then reject all messages except
     * CL_L2CAP_DISCONNECT_CFM.
     */
    if(isStateDisconnecting(id))
        return;

    switch (id)
    {
       /* Connection library messages */
        case CL_L2CAP_REGISTER_CFM:
            DeviceUpgradePeerHandleL2capRegisterCfm(
                                      (const CL_L2CAP_REGISTER_CFM_T *)message);
            break;

        case CL_SDP_REGISTER_CFM:
            DeviceUpgradePeerHandleClSdpRegisterCfm(
                                        (const CL_SDP_REGISTER_CFM_T *)message);
            break;

        case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM:
            DeviceUpgradePeerHandleClSdpServiceSearchAttributeCfm(
                        (const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *)message);
            return;

        case CL_L2CAP_CONNECT_IND:
            DeviceUpgradePeerHandleL2capConnectInd(
                                       (const CL_L2CAP_CONNECT_IND_T *)message);
            break;

        case CL_L2CAP_CONNECT_CFM:
            DeviceUpgradePeerHandleL2capConnectCfm(
                                       (const CL_L2CAP_CONNECT_CFM_T *)message);
            break;

        case CL_L2CAP_DISCONNECT_IND:
            DeviceUpgradePeerHandleL2capDisconnectInd(
                                    (const CL_L2CAP_DISCONNECT_IND_T *)message);
            break;

        case CL_L2CAP_DISCONNECT_CFM:
            DeviceUpgradePeerHandleL2capDisconnectCfm(
                                    (const CL_L2CAP_DISCONNECT_CFM_T *)message);
            break;

        /* Internal Upgrade Peer Messages */
        case DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ:
            DeviceUpgradePeerHandleInternalStartupRequest(
                         (DEVICE_UPGRADE_PEER_INTERNAL_STARTUP_REQ_T *)message);
            break;

        /* Transport Data Messages */
        case UPGRADE_PEER_DATA_IND:
            {
                UPGRADE_PEER_DATA_IND_T *pdu = (UPGRADE_PEER_DATA_IND_T*)message;
                DeviceUpgradePeerL2capSendRequest(pdu->data, pdu->size_data);
                break;
            }
        case UPGRADE_TRANSPORT_DATA_IND:
            {
                UPGRADE_TRANSPORT_DATA_IND_T *pdu = (UPGRADE_TRANSPORT_DATA_IND_T*)message;
                DeviceUpgradePeerL2capSendRequest(pdu->data, pdu->size_data);
                break;
            }

        /* Peer-Connect/Disconnect Messages */
         case UPGRADE_PEER_CONNECT_REQ:
            DeviceUpgradePeerForceLinkToPeer();
            break;

        case UPGRADE_PEER_DISCONNECT_REQ:
            DeviceUpgradePeerHandleInternalShutdownReq();
            break;

        /* Sink Data Message*/
        case MESSAGE_MORE_DATA:
            DeviceUpgradePeerHandleMMD((const MessageMoreData*)message);
            break;

        case MESSAGE_SOURCE_EMPTY:
        case MESSAGE_MORE_SPACE:
        case UPGRADE_PEER_DATA_CFM:
        case UPGRADE_TRANSPORT_DATA_CFM:
            break;

        default:
            DEBUG_LOG("DeviceUpgradePeerHandleMessage. UNHANDLED Message id=x%x (%d). State %d", id, id, deviceUpgradePeerGetState());
            break;
    }
}

bool appDeviceUpgradePeerEarlyInit(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("appDeviceUpgradePeerEarlyInit");

    TaskList_InitialiseWithCapacity(GetDeviceUpgradeClientTask(), PEER_UPGRADE_CLIENT_LIST_INIT_CAPACITY);

    return TRUE;
}

/****************************************************************************
NAME
    appDeviceUpgradePeerInit

BRIEF
    Initialise Device upgrade peer task

DESCRIPTION
    Called at start up to initialise the Device upgrade peer task
*/
bool appDeviceUpgradePeerInit(Task init_task)
{
    deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();

    /* Set up task handler */
    theDeviceUpgradePeer->task.handler = DeviceUpgradePeerHandleMessage;

    deviceUpgradePeerSetState(DEVICE_UPGRADE_PEER_STATE_INITIALISE);

    Init_SetInitTask(init_task);

    return TRUE;
}

/****************************************************************************
NAME
    DeviceUpgradePeerForceLinkToPeer

BRIEF
    Initiate Peer connect request when UPGRADE_PEER_CONNECT_REQ msg is
    received from Upgrade library
*/
void DeviceUpgradePeerForceLinkToPeer(void)
{
     deviceUpgradePeerTaskData *theDeviceUpgradePeer = GetDeviceUpgradePeer();

     /* Set the connect attempts variable to zero */
     acl_connect_attempts = sdp_connect_attempts = 0;

     theDeviceUpgradePeer->is_primary = TRUE;

    /* Don't try the link connection if another connection is in progress */
    if(theDeviceUpgradePeer->state != DEVICE_UPGRADE_PEER_STATE_IDLE)
    {
        DeviceUpgradePeerSendL2capConnectFailure();
        DEBUG_LOG("A connection is already in progress");
        return;
    }
    if (appDeviceGetPeerBdAddr(&(theDeviceUpgradePeer->peer_addr)))
    {
        DEBUG_LOG("DeviceUpgradePeerConnect");

        /* Initiate the L2CAP connection with peer device */
        DeviceUpgradePeerStartup(&theDeviceUpgradePeer->peer_addr);
    }
    else
    {
        DEBUG_LOG("No peer earbud paired");
    }
}

/****************************************************************************
NAME
    appDeviceUpgradePeerStillInUSeAfterAbort

BRIEF
    Check if Abort is trigerred and device upgrade peer still in use.
    Return TRUE if abort is initiated and device upgrade peer still in use,
    else FALSE.

NOTE
    This API will be used only for Primary device. Since that device is the
    starting point of a new DFU after abort.
*/
bool appDeviceUpgradePeerStillInUSeAfterAbort(void)
{
    if(UpgradePeerIsPeerDFUAborted() && IsDeviceUpgradePeerInUse())
        return TRUE;
    else
        return FALSE;
}

/****************************************************************************
NAME
    appUpgradePeerClientRegister

BRIEF
    Register any Client with Device Upgrade Peer for messages
*/
void appUpgradePeerClientRegister(Task tsk)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(GetDeviceUpgradeClientTask()), tsk);
}
#endif

