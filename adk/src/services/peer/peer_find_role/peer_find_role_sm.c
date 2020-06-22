/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Implementation of state machine transitions for the service finding the peer 
            and selecting role using LE
*/

#include <panic.h>
#include <logging.h>

#include <app/bluestack/dm_prim.h>

#include <bt_device.h>
#include <connection_manager.h>
#include <gatt_role_selection_server_uuids.h>
#include <gatt_role_selection_client.h>

/* Ensure GATT handles being obtained by correct method */
#ifdef __GATT_HANDLER_DB_H
#error GATT Handles directly included from gatt_handler_db.h
#endif
#include <gatt_handler_db_if.h>

#include "peer_find_role_sm.h"
#include "peer_find_role_private.h"
#include "peer_find_role_config.h"
#include "peer_find_role_init.h"
#include "peer_find_role_scan.h"

#include "timestamp_event.h"

#define MAKE_PRIM_C(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->common.op_code = TYPE; prim->common.length = sizeof(TYPE##_T);

/*! Indicate whether advertising can be running in a state

    \param state The state to check for advertising mode

    \return TRUE if advertising can take place in the state, FALSE otherwise */
static bool peer_find_role_advertising_state(PEER_FIND_ROLE_STATE state)
{
    switch (state)
    {
        case PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE:
        case PEER_FIND_ROLE_STATE_DISCOVERED_DEVICE:
        case PEER_FIND_ROLE_STATE_CONNECTING_TO_DISCOVERED:
            return TRUE;

            /* Cover all cases so we can have a panic should
               states be updated later */
        case PEER_FIND_ROLE_STATE_UNINITIALISED:
        case PEER_FIND_ROLE_STATE_INITIALISED:
        case PEER_FIND_ROLE_STATE_CHECKING_PEER:
        case PEER_FIND_ROLE_STATE_DISCOVER:
        case PEER_FIND_ROLE_STATE_SERVER_AWAITING_ENCRYPTION:
        case PEER_FIND_ROLE_STATE_SERVER_PREPARING:
        case PEER_FIND_ROLE_STATE_CLIENT:
        case PEER_FIND_ROLE_STATE_SERVER:
        case PEER_FIND_ROLE_STATE_CLIENT_AWAITING_ENCRYPTION:
        case PEER_FIND_ROLE_STATE_CLIENT_PREPARING:
        case PEER_FIND_ROLE_STATE_CLIENT_DECIDING:
        case PEER_FIND_ROLE_STATE_CLIENT_AWAITING_CONFIRM:
        case PEER_FIND_ROLE_STATE_SERVER_AWAITING_COMPLETION:
        case PEER_FIND_ROLE_STATE_COMPLETED:
            break;

        default:
            DEBUG_LOG("peer_find_role_advertising_state. Unhandled state %d",state);
            Panic();
            break;
    }
    return FALSE;
}


/*! Indicate whether we are a gatt client in a state

    \param state The state to check for being a GATT client

    \return TRUE if we are/could be a GATT client */
static bool peer_find_role_client_state(PEER_FIND_ROLE_STATE state)
{
    switch (state)
    {
        case PEER_FIND_ROLE_STATE_CLIENT:
        case PEER_FIND_ROLE_STATE_CLIENT_AWAITING_ENCRYPTION:
        case PEER_FIND_ROLE_STATE_CLIENT_PREPARING:
        case PEER_FIND_ROLE_STATE_CLIENT_DECIDING:
        case PEER_FIND_ROLE_STATE_CLIENT_AWAITING_CONFIRM:
            /* If we are/were a gatt server, then gatt_cid should be INVALID_CID */
        case PEER_FIND_ROLE_STATE_COMPLETED:
            return TRUE;

        /* Cover all cases so we can have a panic should
           states be updated later */
        case PEER_FIND_ROLE_STATE_UNINITIALISED:
        case PEER_FIND_ROLE_STATE_INITIALISED:
        case PEER_FIND_ROLE_STATE_CHECKING_PEER:
        case PEER_FIND_ROLE_STATE_DISCOVER:
        case PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE:
        case PEER_FIND_ROLE_STATE_DISCOVERED_DEVICE:
        case PEER_FIND_ROLE_STATE_CONNECTING_TO_DISCOVERED:
        case PEER_FIND_ROLE_STATE_SERVER_AWAITING_ENCRYPTION:
        case PEER_FIND_ROLE_STATE_SERVER_PREPARING:
        case PEER_FIND_ROLE_STATE_SERVER:
        case PEER_FIND_ROLE_STATE_SERVER_AWAITING_COMPLETION:
            break;

        default:
            DEBUG_LOG("peer_find_role_client_state. Unhandled state %d",state);
            Panic();
            break;
    }
    return FALSE;
}


bool peer_find_role_is_in_advertising_state(void)
{
    return peer_find_role_advertising_state(peer_find_role_get_state());
}


bool peer_find_role_is_running(void)
{
    return peer_find_role_get_state() > PEER_FIND_ROLE_STATE_INITIALISED;
}


/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_INITIALISED

    This state can be entered in response to errors elsewhere, so if 
    there is an active GATT connection a disconnection is requested.

    There is no further action until PeerFindRole_FindRole() is called.
 */
static void peer_find_role_enter_initialised(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    bool disconnected = peer_find_role_disconnect_link();
    bool keep_going = peer_find_role_awaiting_primary_completion();

    peer_find_role_update_media_flag(FALSE, PEER_FIND_ROLE_CONFIRMING_PRIMARY);

    if (!disconnected)
    {
        if (pfr->timeout_means_timeout)
        {
            peer_find_role_notify_timeout();
        }
        else
        {
            peer_find_role_notify_clients_if_pending();
        }

        if (keep_going)
        {
            peer_find_role_update_media_flag(TRUE, PEER_FIND_ROLE_CONFIRMING_PRIMARY);
            peer_find_role_set_state(PEER_FIND_ROLE_STATE_DISCOVER);
        }
    }

}


/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_CHECKING_PEER

    This function checks whether we know of a peer device. If not
    we notify clients that PeerFindRole_FindRole() has completed 
    with the message #PEER_FIND_ROLE_NO_PEER.

    If we know of a peer we jump to the state #PEER_FIND_ROLE_STATE_DISCOVER

    Other cases are errors
 */
static void peer_find_role_enter_checking_peer(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_enter_checking_peer.");

    if (!appDeviceGetPrimaryBdAddr(&pfr->primary_addr))
    {
        peer_find_role_completed(PEER_FIND_ROLE_NO_PEER);
    }
    else
    {
        if (appDeviceGetMyBdAddr(&pfr->my_addr))
        {
            peer_find_role_set_state(PEER_FIND_ROLE_STATE_DISCOVER);
            return;
        }

        /*! \todo A Panic here is dangerous, Factory reset / remove peer ? */
        DEBUG_LOG("peer_find_role_enter_checking_peer. No attributes !");
        Panic();
    }
}


/*! Calculate time to delay advertising.

    Advertising is started immediately on the first connection attempt,
    but is delayed on retries to reduce the risk of the two devices
    initiating connections to each other.

    There is no delay if we have active media as there will be only 
    advertising.

    In priority order
    \li busy (active) - do not delay, we won't be scanning
    \li connected to a handset - delay by a medium amount
    \li this device has the Peripheral role - delay by a large amount
    \li this device has the Cental role - delay by a small amount

    \param reconnecting FALSE if this is the first connection attempt of a 
        find role process
 */
static void peer_find_role_calculate_backoff(bool reconnecting)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_calculate_backoff. reconnecting:%d a2dp/sco:%d",
                reconnecting , peer_find_role_media_active());

    if (reconnecting && !peer_find_role_media_active())
    {
        if (appDeviceIsHandsetConnected())
        {
            pfr->advertising_backoff_ms = 200;

            DEBUG_LOG("peer_find_role_calculate_backoff as %dms. Connected to a handset.",
                        pfr->advertising_backoff_ms);
        }
        else if (peer_find_role_is_central())
        {
            pfr->advertising_backoff_ms = 100;

            DEBUG_LOG("peer_find_role_calculate_backoff as %dms. We have 'Central' role",
                        pfr->advertising_backoff_ms);
        }
        else
        {
            pfr->advertising_backoff_ms = 300;

            DEBUG_LOG("peer_find_role_calculate_backoff as %dms.",
                        pfr->advertising_backoff_ms);
        }
    }
}


/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_DISCOVER

    On entering the state we start scanning if it is permitted, and not 
    already running.

    If we are entering this state to retry a connection a delay before
    starting advertising is calculated.

    Finally we send ourselves a delayed message to start advertising
    #PEER_FIND_ROLE_INTERNAL_TIMEOUT_ADVERT_BACKOFF. The delay may be zero.

    \param old_state The state we have left to enter this state
 */
static void peer_find_role_enter_discover(PEER_FIND_ROLE_STATE old_state)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_enter_discover");

    pfr->gatt_cid = INVALID_CID;
    pfr->gatt_encrypted = FALSE;

    peer_find_role_calculate_backoff(PEER_FIND_ROLE_STATE_CHECKING_PEER != old_state);

    peer_find_role_start_scanning_if_inactive();

    MessageSendLater(PeerFindRoleGetTask(),
                     PEER_FIND_ROLE_INTERNAL_TIMEOUT_ADVERT_BACKOFF, NULL,
                     pfr->advertising_backoff_ms);

    pfr->advertising_backoff_ms = 0;
}


/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE

    On entering the state we start to set up advertising. The response to this
    command will initiate connectable advertising.

    There is also a check to see if scanning should be started. In normal
    use, scanning will have started by now - but it is possible that
    other activities that block scanning have ended since entry to the state
    #PEER_FIND_ROLE_STATE_DISCOVER
 */
static void peer_find_role_enter_discover_connectable(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();
    const le_adv_select_params_t adv_select_params = {.set = le_adv_data_set_peer};

    DEBUG_LOG("peer_find_role_enter_discover_connectable");
    
    TimestampEvent(TIMESTAMP_EVENT_PEER_FIND_ROLE_DISCOVERING_CONNECTABLE);

    /* This is necessary as media may have stopped between entering
        discover state and arriving here */
    peer_find_role_start_scanning_if_inactive();

    pfr->advert_handle = LeAdvertisingManager_SelectAdvertisingDataSet(PeerFindRoleGetTask(), &adv_select_params);

    peer_find_role_advertising_activity_set();
}


/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_CONNECTING_TO_DISCOVERED

    Having detected a device to connect to, the parameters for the
    next connection are updated. The response to this command will
    initiate a connection.
 */
static void peer_find_role_enter_connecting_to_discovered(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();
    tp_bdaddr tp_addr;

    DEBUG_LOG("peer_find_role_enter_connecting_to_discovered");
    
    tp_addr.transport = TRANSPORT_BLE_ACL;
    tp_addr.taddr = pfr->peer_connection_typed_bdaddr;

    ConManagerCreateTpAcl(&tp_addr);
    /* After a call to create, a connection entry exists and
       the Quality of service can be set for the connection */
    ConManagerRequestDeviceQos(&tp_addr, cm_qos_short_data_exchange);
}

/*! Internal function for exiting the state #PEER_FIND_ROLE_STATE_DISCOVER

    We may have been scanning in this state and will need to cancel 
    scanning if we have seen a device, or are leaving the state for 
    anything other than starting advertising (while scanning).

    \param new_state The next state. Used to determine the action to take.
 */
static void peer_find_role_exit_discover(PEER_FIND_ROLE_STATE new_state)
{
    bool cancel_scan = TRUE;

    DEBUG_LOG("peer_find_role_exit_discover - next state:%d",new_state);

    /* Although we might be exiting the state because of this message...
       ...cancel it anyway */
    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_TIMEOUT_ADVERT_BACKOFF);

    switch (new_state)
    {
        case PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE:
            /* We are simply adding advertising to the mix */
            DEBUG_LOG("peer_find_role_exit_discover - next state:PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE");
            cancel_scan = FALSE;
            break;

        case PEER_FIND_ROLE_STATE_DISCOVERED_DEVICE:
            DEBUG_LOG("peer_find_role_exit_discover - next state:PEER_FIND_ROLE_STATE_DISCOVERED_DEVICE");
            break;

        default:
            break;
    }

    if (cancel_scan)
    {
        peer_find_role_stop_scan_if_active();
    }
}


/*! Internal function for exiting advertising states

    This function terminates advertising (if active)
 */
static void peer_find_role_exit_advertising_state(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_exit_advertising_state");

    if (pfr->advert_handle)
    {
        LeAdvertisingManager_ReleaseAdvertisingDataSet(pfr->advert_handle);
        pfr->advert_handle = NULL;
    }
}


/*! Internal function for exiting the state #PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE

    In this state we may have been scanning and will have been advertising.
    Scanning and advertising will be stopped if still active.

    \param new_state This is the state we are entering, used solely for debug
 */
static void peer_find_role_exit_discover_connectable(PEER_FIND_ROLE_STATE new_state)
{
    DEBUG_LOG("peer_find_role_exit_discover_connectable - next state:%d",new_state);

    peer_find_role_stop_scan_if_active();
}


/*! Internal function called when entering the state #PEER_FIND_ROLE_STATE_DISCOVERED_DEVICE

    This function creates a message #PEER_FIND_ROLE_INTERNAL_CONNECT_TO_PEER
    that will be sent if we manage to stop any ongoing advertising. The message 
    will be cancelled if we leave the state. 

    We may be about to accept a connection from another device, which is why we 
    send a conditional message.
 */
static void peer_find_role_enter_discovered_device(void)
{
    DEBUG_LOG("peer_find_role_enter_discovered_device");

    TimestampEvent(TIMESTAMP_EVENT_PEER_FIND_ROLE_DISCOVERED_DEVICE);

    /* cancel the timeout, now that peer has been seen */
    peer_find_role_cancel_initial_timeout();

    peer_find_role_message_send_when_inactive(PEER_FIND_ROLE_INTERNAL_CONNECT_TO_PEER, NULL);
}


/*! Internal function for exiting the state #PEER_FIND_ROLE_STATE_DISCOVERED_DEVICE

    We make sure that the message #PEER_FIND_ROLE_INTERNAL_CONNECT_TO_PEER is
    cancelled at this point. 

    \note The message might have already expired. Cancelling a non-existent message
    is not harmful.
 */
static void peer_find_role_exit_discovered_device(void)
{
    DEBUG_LOG("peer_find_role_exit_discovered_device");

    peer_find_role_message_cancel_inactive(PEER_FIND_ROLE_INTERNAL_CONNECT_TO_PEER);
}


/*! Internal function for exiting the state #PEER_FIND_ROLE_STATE_CONNECTING_TO_DISCOVERED

    This function makes sure that we cancel an attempt to connect if 
    the next state represents anything other than having made the connection.

    \param new_state The state we are entering next
 */
static void peer_find_role_exit_connecting_to_discovered(PEER_FIND_ROLE_STATE new_state)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();
    tp_bdaddr tp_addr;

    switch (new_state)
    {
            /* Normal case as server */
        case PEER_FIND_ROLE_STATE_SERVER_AWAITING_ENCRYPTION:
            /* Client case. Connection established when we were trying to make */ 
        case PEER_FIND_ROLE_STATE_CLIENT:
            break;

        default:
            {
                DEBUG_LOG("peer_find_role_exit_connecting_to_discovered. Cancelling server connection");

                tp_addr.transport = TRANSPORT_BLE_ACL;
                tp_addr.taddr = pfr->peer_connection_typed_bdaddr;
                
                ConManagerReleaseTpAcl(&tp_addr);
            }
            break;
    }
}


/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_SERVER_AWAITING_COMPLETION

    On entering the state we start a timer #PEER_FIND_ROLE_INTERNAL_TIMEOUT_NOT_DISCONNECTED
    is started. Expiry of this would force a disconnection.
 */
static void peer_find_role_enter_server_awaiting_completion(void)
{
    DEBUG_LOG("peer_find_role_enter_server_awaiting_completion");

    MessageSendLater(PeerFindRoleGetTask(), 
                     PEER_FIND_ROLE_INTERNAL_TIMEOUT_NOT_DISCONNECTED, NULL,
                     PeerFindRoleConfigGattDisconnectTimeout());
}


/*! Internal function for exiting the state #PEER_FIND_ROLE_STATE_SERVER_AWAITING_COMPLETION

    As we have now left the state we cancel the message that 
    forces a disconnect if the connection has not already been 
    dropped 
 */
static void peer_find_role_exit_server_awaiting_completion(void)
{
    DEBUG_LOG("peer_find_role_exit_server_awaiting_completion");

    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_TIMEOUT_NOT_DISCONNECTED);
}

/*! Internal function to setup the known characteristic handles of the role selection service
*/
static void peerFindRole_SetCachedHandlesToMatchServer(gatt_role_selection_handles_t *handles)
{
    handles->handle_state = HANDLE_ROLE_SELECTION_MIRRORING_STATE;
    handles->handle_state_config = HANDLE_ROLE_SELECTION_MIRRORING_STATE_CLIENT_CONFIG;
    handles->handle_state_end = HANDLE_ROLE_SELECTION_MIRRORING_STATE_CLIENT_CONFIG;
    handles->handle_figure_of_merit = HANDLE_ROLE_SELECTION_FIGURE_OF_MERIT;
    handles->handle_figure_of_merit_config = HANDLE_ROLE_SELECTION_FIGURE_OF_MERIT_CLIENT_CONFIG;
    handles->handle_figure_of_merit_end = HANDLE_ROLE_SELECTION_FIGURE_OF_MERIT_CLIENT_CONFIG;
    handles->handle_role_control = HANDLE_ROLE_SELECTION_CONTROL;
}

/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_CLIENT

    On entering the state we request details of the specific role
    selection service from the connected device.

    The device must provide the GATT service UUID_ROLE_SELECTION_SERVICE
 */
static void peer_find_role_enter_client(void)
{
    gatt_role_selection_handles_t cached_handles;

    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_enter_client cid 0x%x", pfr->gatt_cid);

    /* cancel the timeout, now that peer has been seen */
    peer_find_role_cancel_initial_timeout();

    /* Cancel connect message as we have... 
       ...this protects against a race, as we should cancel leaving the discover state */
    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_CONNECT_TO_PEER);

    /*! \todo Ask the client to initialise itself, so we don't have to know about UUID */
    PanicFalse(INVALID_CID != pfr->gatt_cid);

    peerFindRole_SetCachedHandlesToMatchServer(&cached_handles);

    PeerFindRole_SetPeerScore(GRSS_FIGURE_OF_MERIT_INVALID);

    PanicFalse(GattRoleSelectionClientInit(&pfr->role_selection_client,
                                            PeerFindRoleGetTask(),
                                            &cached_handles,
                                            pfr->gatt_cid,
                                            HANDLE_ROLE_SELECTION_SERVICE,
                                            HANDLE_ROLE_SELECTION_SERVICE_END));

}

/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_SERVER_AWAITING_ENCRYPTION

    On entering the state we request security.
 */
static void peer_find_role_enter_server_awaiting_encryption(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();
    typed_bdaddr tb;
    
    tb = pfr->peer_connection_typed_bdaddr;

    DEBUG_LOG("peer_find_role_enter_server_awaiting_encryption 0x%06x", tb.addr.lap);

    ConnectionDmBleSecurityReq(PeerFindRoleGetTask(), &tb,
                               ble_security_encrypted_bonded, 
                               ble_connection_master_directed);
}

/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_SERVER_PREPARING

    On entering the state we request to the prepare client to prepare the
    system/application for role selection.

    If no prepare client is registered assume system is good for role selection
    and go directly to SERVER state.
 */
static void peer_find_role_enter_server_preparing(void)
{
    DEBUG_LOG("peer_find_role_enter_server_preparing");

    if (peer_find_role_prepare_client_registered())
    {
        peer_find_role_request_prepare_for_role_selection();
    }
    else
    {
        peer_find_role_set_state(PEER_FIND_ROLE_STATE_SERVER);
    }
}

/*! Internal function for exiting the state #PEER_FIND_ROLE_STATE_SERVER_PREPARING

    Cancel any outstanding PEER_FIND_ROLE_INTERNAL_PREPARED messages because
    they will be ignored outside of this state anyway.
 */
static void peer_find_role_exit_server_preparing(void)
{
    DEBUG_LOG("peer_find_role_exit_server_preparing");

    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_PREPARED);
}

/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_SERVER

    On entering we calculate the figure of merit for the local device and
    update the gatt server characteristic with this value.

    Then wait for the client to send the result of the role selection.
 */
static void peer_find_role_enter_server(void)
{
    DEBUG_LOG("peer_find_role_enter_server");

    /* Calculate the score and update the gatt server figure of merit. */
    peer_find_role_update_server_score();

    /* Start a timeout in case we don't get given a role by the client. */
    MessageSendLater(PeerFindRoleGetTask(),
                     PEER_FIND_ROLE_INTERNAL_TIMEOUT_SERVER_ROLE_SELECTED, NULL,
                     PeerFindRoleConfigServerRoleSelectedTimeoutMs());
}

/*! Internal function for exiting the state #PEER_FIND_ROLE_STATE_SERVER

    Cancel any outstanding SERVER_ROLE_SELECTED timeout messages.
 */
static void peer_find_role_exit_server(void)
{
    DEBUG_LOG("peer_find_role_exit_server");

    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_TIMEOUT_SERVER_ROLE_SELECTED);
}

/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_AWAITING_ENCRYPTION

    On entering the state we check if the link is already encrypted 
    and move to deciding state if so */
static void peer_find_role_enter_awaiting_encryption(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_enter_awaiting_encryption");

    if (pfr->gatt_encrypted)
    {
        peer_find_role_set_state(PEER_FIND_ROLE_STATE_CLIENT_PREPARING);
    }
}


/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_CLIENT_PREPARING

    On entering the state we check if there is a client registered for 
    PEER_FIND_ROLE_PREPARE_FOR_ROLE_SELECTION indication. If not, go
    straight to DECIDING state.
 */
static void peer_find_role_enter_client_preparing(void)
{
    DEBUG_LOG("peer_find_role_enter_client_preparing client %d", peer_find_role_prepare_client_registered());

    if (peer_find_role_prepare_client_registered())
    {
        peer_find_role_request_prepare_for_role_selection();
    }
    else
    {
        peer_find_role_set_state(PEER_FIND_ROLE_STATE_CLIENT_DECIDING);
    }
}

/*! Internal function for exiting the state #PEER_FIND_ROLE_STATE_CLIENT_PREPARING

    Cancel any outstanding PEER_FIND_ROLE_INTERNAL_PREPARED messages because
    they will be ignored outside of this state anyway.
 */
static void peer_find_role_exit_client_preparing(void)
{
    DEBUG_LOG("peer_find_role_exit_client_preparing");

    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_PREPARED);
}

/*! Internal function for entering the state #PEER_FIND_ROLE_STATE_CLIENT_DECIDING

    On entering the state we wait for a notification from the server of the figure of merit
    from our peer. Set a time in the case this notification is not received.
 */
static void peer_find_role_enter_client_deciding(void)
{
    DEBUG_LOG("peer_find_role_enter_client_deciding");

    TimestampEvent(TIMESTAMP_EVENT_PEER_FIND_ROLE_DECIDING_ROLES);

    MessageSendLater(PeerFindRoleGetTask(),
                    PEER_FIND_ROLE_INTERNAL_TIMEOUT_NO_FOM_RECEIVED,
                    NULL,
                    PeerFindRoleConfigWaitForFomNotificationDelayMs());

    PeerFindRole_DecideRoles();
}

/*! Internal function for entering the state PEER_FIND_ROLE_STATE_CLIENT_AWAITING_CONFIRM

    On entering the state clear timer set in case of no FOM notification
*/
static void peerFindRole_EnterClientAwaitingConfirm(void)
{
    DEBUG_LOG("peerFindRole_EnterClientAwaitingConfirm");
    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_TIMEOUT_NO_FOM_RECEIVED);
}

static void peer_find_role_enter_completed(void)
{
    DEBUG_LOG("peer_find_role_enter_completed");

    /* If link disconnection not initiated jump to next state */
    if (!peer_find_role_disconnect_link())
    {
        peer_find_role_select_state_after_completion();
    }
}


static void peer_find_role_exit_client_state(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    if (pfr->gatt_cid != INVALID_CID)
    {
        DEBUG_LOG("peer_find_role_exit_client_state (cid:x%x)", pfr->gatt_cid);

        GattRoleSelectionClientDestroy(&pfr->role_selection_client);
        pfr->gatt_cid = INVALID_CID;
        pfr->gatt_encrypted = FALSE;
    }
}


void peer_find_role_set_state(PEER_FIND_ROLE_STATE new_state)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    PEER_FIND_ROLE_STATE old_state = peer_find_role_get_state();

    if (old_state == new_state)
    {
        DEBUG_LOG_ERROR("peer_find_role_set_state. Attempt to transition to same state:%d",
                   old_state);
        Panic();    /*! \todo Remove panic once topology implementation stable */
        return;
    }

    DEBUG_LOG_STATE("peer_find_role_set_state. Transition %d->%d. Busy flags were:0x%x",
                old_state, new_state,
                PeerFindRoleGetScanBusy());

    /* Pattern is to run functions for exiting state first */
    switch (old_state)
    {
        case PEER_FIND_ROLE_STATE_DISCOVER:
            peer_find_role_exit_discover(new_state);
            break;

        case PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE:
            peer_find_role_exit_discover_connectable(new_state);
            break;

        case PEER_FIND_ROLE_STATE_DISCOVERED_DEVICE:
            peer_find_role_exit_discovered_device();
            break;

        case PEER_FIND_ROLE_STATE_CONNECTING_TO_DISCOVERED:
            peer_find_role_exit_connecting_to_discovered(new_state);
            break;

        case PEER_FIND_ROLE_STATE_SERVER:
            peer_find_role_exit_server();
            break;

        case PEER_FIND_ROLE_STATE_SERVER_PREPARING:
            peer_find_role_exit_server_preparing();
            break;

        case PEER_FIND_ROLE_STATE_SERVER_AWAITING_COMPLETION:
            peer_find_role_exit_server_awaiting_completion();
            break;

        case PEER_FIND_ROLE_STATE_CLIENT_PREPARING:
            peer_find_role_exit_client_preparing();
            break;

        default:
            break;
    }

    /* Check for any transitions between "super states" */
    if (    peer_find_role_advertising_state(old_state)
        && !peer_find_role_advertising_state(new_state))
    {
        peer_find_role_exit_advertising_state();
    }

    if (    pfr->gatt_cid != INVALID_CID
        &&  peer_find_role_client_state(old_state)
        && !peer_find_role_client_state(new_state))
    {
        peer_find_role_exit_client_state();
    }


    pfr->state = new_state;

    switch (new_state)
    {
        case PEER_FIND_ROLE_STATE_INITIALISED:
            peer_find_role_enter_initialised();
            break;

        case PEER_FIND_ROLE_STATE_CHECKING_PEER:
            peer_find_role_enter_checking_peer();
            break;

        case PEER_FIND_ROLE_STATE_DISCOVER:
            peer_find_role_enter_discover(old_state);
            break;

        case PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE:
            peer_find_role_enter_discover_connectable();
            break;

        case PEER_FIND_ROLE_STATE_DISCOVERED_DEVICE:
            peer_find_role_enter_discovered_device();
            break;

        case PEER_FIND_ROLE_STATE_CONNECTING_TO_DISCOVERED:
            peer_find_role_enter_connecting_to_discovered();
            break;

        case PEER_FIND_ROLE_STATE_CLIENT:
            peer_find_role_enter_client();
            break;

        case PEER_FIND_ROLE_STATE_SERVER:
            peer_find_role_enter_server();
            break;

        case PEER_FIND_ROLE_STATE_SERVER_AWAITING_ENCRYPTION:
            peer_find_role_enter_server_awaiting_encryption();
            break;

        case PEER_FIND_ROLE_STATE_SERVER_PREPARING:
            peer_find_role_enter_server_preparing();
            break;

        case PEER_FIND_ROLE_STATE_CLIENT_AWAITING_ENCRYPTION:
            peer_find_role_enter_awaiting_encryption();
            break;

        case PEER_FIND_ROLE_STATE_CLIENT_PREPARING:
            peer_find_role_enter_client_preparing();
            break;

        case PEER_FIND_ROLE_STATE_CLIENT_DECIDING:
            peer_find_role_enter_client_deciding();
            break;

        case PEER_FIND_ROLE_STATE_CLIENT_AWAITING_CONFIRM:
            peerFindRole_EnterClientAwaitingConfirm();
            break;

        case PEER_FIND_ROLE_STATE_SERVER_AWAITING_COMPLETION:
            peer_find_role_enter_server_awaiting_completion();
            break;

        case PEER_FIND_ROLE_STATE_COMPLETED:
            peer_find_role_enter_completed();
            break;

        default:
            break;
    }
}
