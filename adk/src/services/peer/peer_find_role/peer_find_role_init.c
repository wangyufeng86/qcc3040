/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Initialisation functions for the Peer find role service (using LE)
*/

#include <panic.h>

#include <task_list.h>
#include <logging.h>
#include <phy_state.h>
#include <charger_monitor.h>
#include <acceleration.h>
#include <battery_monitor.h>
#include <gatt_role_selection_client.h>
#include <gatt_role_selection_server.h>

/* Ensure GATT handles being obtained by correct method */
#ifdef __GATT_HANDLER_DB_H
#error GATT Handles directly included from gatt_handler_db.h
#endif
#include <gatt_handler_db_if.h>

#include <connection_manager.h>
#include <handset_service.h>
#include <hfp_profile.h>
#include <av.h>
#include <gatt_connect.h>
#include <gatt_handler.h>

#include "peer_find_role.h"
#include "peer_find_role_private.h"
#include "peer_find_role_scoring.h"
#include "peer_find_role_init.h"
#include "peer_find_role_config.h"
#include "peer_find_role_scan.h"
#include "telephony_messages.h"

#include "timestamp_event.h"


peerFindRoleTaskData peer_find_role = {0};


#include <gatt_role_selection_server_uuids.h>
#include <uuid.h>
#define NUMBER_OF_ADVERT_DATA_ITEMS     1
#define SIZE_PEER_FIND_ROLE_ADVERT      4

static unsigned int peer_find_role_NumberOfAdvItems(const le_adv_data_params_t * params);
static le_adv_data_item_t peer_find_role_GetAdvDataItems(const le_adv_data_params_t * params, unsigned int id);
static void peer_find_role_ReleaseAdvDataItems(const le_adv_data_params_t * params);
static void peer_find_role_GattConnect(uint16 cid);
static void peer_find_role_GattDisconnect(uint16 cid);

static const gatt_connect_observer_callback_t peer_find_role_connect_callback =
{
    .OnConnection = peer_find_role_GattConnect,
    .OnDisconnection = peer_find_role_GattDisconnect
};

static const le_adv_data_callback_t peer_find_role_advert_callback =
{
    .GetNumberOfItems = peer_find_role_NumberOfAdvItems,
    .GetItem = peer_find_role_GetAdvDataItems,
    .ReleaseItems = peer_find_role_ReleaseAdvDataItems
};

static const uint8 peer_find_role_advert_data[SIZE_PEER_FIND_ROLE_ADVERT] = {
    SIZE_PEER_FIND_ROLE_ADVERT - 1,
    ble_ad_type_complete_uuid16,
    UUID_ROLE_SELECTION_SERVICE & 0xFF,
    UUID_ROLE_SELECTION_SERVICE >> 8
};

static le_adv_data_item_t peer_find_role_advert;

void peer_find_role_advertising_activity_set(void)
{
    uint16 old_busy = PeerFindRoleGetAdvertBusy();

    if (!old_busy)
    {
        PeerFindRoleSetAdvertBusy(TRUE);
        DEBUG_LOG("peer_find_role_advertising_activity_set.");
    }
}


void peer_find_role_advertising_activity_clear(void)
{
    uint16 old_busy = PeerFindRoleGetAdvertBusy();

    if (old_busy)
    {
        PeerFindRoleSetAdvertBusy(FALSE);
        DEBUG_LOG("peer_find_role_advertising_activity_clear.");
    }
}


void peer_find_role_message_send_when_inactive(MessageId id, void *message)
{
    MessageSendConditionally(PeerFindRoleGetTask(), id, message, &PeerFindRoleGetScanBusy());
}


void peer_find_role_message_cancel_inactive(MessageId id)
{
    if (PeerFindRoleGetScanBusy())
    {
        if (MessageCancelAll(PeerFindRoleGetTask(), id))
        {
            DEBUG_LOG("peer_find_role_busy_message_cancel. Queued messages were cancelled.");
        }
    }
}


void peer_find_role_cancel_initial_timeout(void)
{
    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_TIMEOUT_CONNECTION);
}


static void peer_find_role_notify_clients_immediately(peer_find_role_message_t role)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG_FN_ENTRY("peer_find_role_notify_clients_immediately role= 0x%x", role);

    TimestampEvent(TIMESTAMP_EVENT_PEER_FIND_ROLE_NOTIFIED_ROLE);

    peer_find_role_cancel_initial_timeout();
    pfr->timeout_means_timeout = FALSE;

    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(PeerFindRoleGetTaskList()), role);
}


void peer_find_role_notify_clients_if_pending(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();
    peer_find_role_message_t role = pfr->selected_role;

    DEBUG_LOG_FN_ENTRY("peer_find_role_notify_clients_if_pending");

    pfr->selected_role = (peer_find_role_message_t)0;

    if (role)
    {
        DEBUG_LOG_INFO("peer_find_role_notify_clients_if_pending role= 0x%x", role);
        peer_find_role_notify_clients_immediately(role);
    }
}


static bool peer_find_role_role_means_primary(peer_find_role_message_t role)
{
    switch (role)
    {
        case PEER_FIND_ROLE_ACTING_PRIMARY:
        case PEER_FIND_ROLE_PRIMARY:
            return TRUE;

        default:
            break;
    }

    return FALSE;
}


void peer_find_role_completed(peer_find_role_message_t role)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    if (peer_find_role_role_means_primary(role))
    {
        DEBUG_LOG_INFO("peer_find_role_completed. Role is a primary one:0x%x", role);

        pfr->selected_role = (peer_find_role_message_t )0;
        peer_find_role_notify_clients_immediately(role);
    }
    else
    {
        DEBUG_LOG_INFO("peer_find_role_completed. Role is not a primary one:0x%x. Wait for links to go.", role);

        pfr->selected_role = role;
        pfr->timeout_means_timeout = FALSE;
    }

    /* When we report a primary role, find role will restart.
       But we will want to block scanning while we wait for the
       primary to be completed. Mark us as 'busy'. */
    peer_find_role_update_media_flag(PEER_FIND_ROLE_PRIMARY == role, PEER_FIND_ROLE_CONFIRMING_PRIMARY);

    switch (peer_find_role_get_state())
    {
        case PEER_FIND_ROLE_STATE_INITIALISED:
            peer_find_role_notify_clients_if_pending();
            break;

        case PEER_FIND_ROLE_STATE_SERVER:
            /* If we disconnect the GATT link at this point, then the client
                does not always receive a confirmation */
            peer_find_role_set_state(PEER_FIND_ROLE_STATE_SERVER_AWAITING_COMPLETION);
            break;

        case PEER_FIND_ROLE_STATE_COMPLETED:
            /* Nothing more to do as we're already in the correct state */
            break;

        default:
            peer_find_role_set_state(PEER_FIND_ROLE_STATE_COMPLETED);
            break;
    }

    GattRoleSelectionClientDestroy(&pfr->role_selection_client);
    pfr->gatt_cid = INVALID_CID;
    pfr->gatt_encrypted = FALSE;
}


void peer_find_role_notify_timeout(void)
{
    if(PeerFindRole_GetFixedRole() == peer_find_role_fixed_role_secondary)
    {
        DEBUG_LOG("peer_find_role_notify_timeout. Don't notify clients. We're always secondary");
    }
    else
    {
        peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

        DEBUG_LOG_VERBOSE("peer_find_role_notify_timeout. Now acting primary");

        /*! \todo We will use different states for fast/slow
            We only timeout once, clear the flag */
        pfr->role_timeout_ms = 0;
        peer_find_role_notify_clients_immediately(PEER_FIND_ROLE_ACTING_PRIMARY);
        /* Switch to low duty scan after timeout have expired to save power */
        peer_find_role_update_scan_mode_if_active();
    }
}


bool peer_find_role_disconnect_link(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    pfr->gatt_encrypted = FALSE;

    if (INVALID_CID != pfr->gatt_cid)
    {
        tp_bdaddr tp_addr;

        DEBUG_LOG("peer_find_role_disconnect_link. Disconnecting ACL");

        tp_addr.transport = TRANSPORT_BLE_ACL;
        tp_addr.taddr = pfr->peer_connection_typed_bdaddr;

        ConManagerReleaseTpAcl(&tp_addr);
        return TRUE;
    }

    return FALSE;
}


void peer_find_role_select_state_after_completion(void)
{
    if (peer_find_role_awaiting_primary_completion())
    {
        peer_find_role_set_state(PEER_FIND_ROLE_STATE_DISCOVER);
    }
    else
    {
        peer_find_role_set_state(PEER_FIND_ROLE_STATE_INITIALISED);
    }
}


void peer_find_role_request_prepare_for_role_selection(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_request_prepare_for_role_selection");

    PanicFalse(peer_find_role_prepare_client_registered());
    TaskList_MessageSendId(&pfr->prepare_tasks, PEER_FIND_ROLE_PREPARE_FOR_ROLE_SELECTION);
}

/*! Handle response to setting advertising dataset

    If successful start connectable advertising using the just configured
    advertising data.

    In case of failure, indicate that we are not advertising

    \param cfm Confirmation for setting advertising dataset

    \todo Is clearing activity enough to kick any error handling off ?
 */
static void peer_find_role_handle_adv_mgr_select_dataset_cfm(const LE_ADV_MGR_SELECT_DATASET_CFM_T *cfm)
{
    DEBUG_LOG("peer_find_role_handle_adv_mgr_select_dataset_cfm. sts:%d",
                    cfm->status);

    if (le_adv_mgr_status_success != cfm->status)
    {
        peer_find_role_advertising_activity_clear();
    }
}


/*! Handle response to releasing the advertising dataset

    In all cases indicate that we are not advertising

    \param cfm Confirmation for releasing advertising dataset

    \todo Is clearing activity enough to kick any error handling off ?
 */
static void peer_find_role_handle_adv_mgr_release_dataset_cfm(const LE_ADV_MGR_RELEASE_DATASET_CFM_T *cfm)
{
    DEBUG_LOG("peer_find_role_handle_adv_mgr_release_dataset_cfm. sts:%d",
                    cfm->status);

    peer_find_role_advertising_activity_clear();
}


/*! Handle message confirming initialisation of the GATT client

    Change state to #PEER_FIND_ROLE_STATE_AWAITING_ENCRYPTION, which will
    block further activity until the link is encrypted.

    \param init Message confirming initialisation
 */
static void peer_find_role_handle_client_init(const GATT_ROLE_SELECTION_CLIENT_INIT_CFM_T *init)
{
    DEBUG_LOG("peer_find_role_handle_client_init. sts:%d", init->status);

    if (gatt_role_selection_client_status_success == init->status)
    {
        peer_find_role_set_state(PEER_FIND_ROLE_STATE_CLIENT_AWAITING_ENCRYPTION);
    }
    else
    {
        /*! \todo Handle init failure ! */
    }

}


/*! Handle command from our peer to change role

    Indicate completion of the PeerFindRole_FindRole() procedure by sending an appropriate
    message to our clients, either #PEER_FIND_ROLE_PRIMARY or
    PEER_FIND_ROLE_SECONDARY

    \param role Role message received from our peer
 */
static void peer_find_role_handle_change_role_ind(const GATT_ROLE_SELECTION_SERVER_CHANGE_ROLE_IND_T *role)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_handle_change_role_ind. cmd:%d selected_role 0x%x state %u",
              role->command, pfr->selected_role, pfr->state);

    /* If we have already selected a role, e.g. by being cancelled, ignore the
       selected role from the peer eb. */
    if (pfr->selected_role == (peer_find_role_message_t)0)
    {
        peer_find_role_completed((role->command == GrssOpcodeBecomePrimary)
                                            ? PEER_FIND_ROLE_PRIMARY
                                            : PEER_FIND_ROLE_SECONDARY);
    }
}


/*! Handle confirmation that our peer received its new state

    Complete the PeerFindRole_FindRole() procedure by sending an appropriate
    message to our clients, either #PEER_FIND_ROLE_PRIMARY or
    PEER_FIND_ROLE_SECONDARY

    \param cfm Confirmation message - effectively received from our peer

    \todo Error case !success needs to disconnect and retry.
 */
static void peer_find_role_handle_command_cfm(const GATT_ROLE_SELECTION_CLIENT_COMMAND_CFM_T *cfm)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_handle_command_cfm. sts:%d. state:%d",
              cfm->result, peer_find_role_get_state());

    if (   gatt_status_success == cfm->result
        && PEER_FIND_ROLE_STATE_CLIENT_AWAITING_CONFIRM == peer_find_role_get_state())
    {
        if (GrssOpcodeBecomeSecondary == pfr->remote_role)
        {
            peer_find_role_completed(PEER_FIND_ROLE_PRIMARY);
        }
        else
        {
            peer_find_role_completed(PEER_FIND_ROLE_SECONDARY);
        }
    }
}


/*! Handler for physical state messages

   Update our information about the physical state and update the score
   used to later decide role

   \param phy Physical state indication
*/
static void peer_find_role_handle_phy_state(const PHY_STATE_CHANGED_IND_T *phy)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_handle_phy_state. New physical state:%d", phy->new_state);

    pfr->scoring_info.phy_state = phy->new_state;
}


/*! Handler for charger status

    Update our information about the charge and update the score used to later decide role

    \param attached Is the charger currently attached ?
 */
static void peer_find_role_handle_charger_state(bool attached)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_handle_charger_state. Attached:%d", attached);

    pfr->scoring_info.charger_present = attached;
}


/*! Handler for accelerometer status

    Update our latest moving state and update the score used to later decide role

    \param moving Did the update indicate we are moving?
 */
static void peer_find_role_handle_accelerometer_state(bool moving)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_handle_accelerometer_state. Moving:%d", moving);

    pfr->scoring_info.accelerometer_moving = moving;
}


/*! Handler for battery level updates

    Store our latest battery level and update the score used to later decide role

    \param battery Battery reading (contains a percentage)
 */
static void peer_find_role_handle_battery_level(const MESSAGE_BATTERY_LEVEL_UPDATE_PERCENT_T *battery)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_handle_battery_level. Level:%d", battery->percent);

    pfr->scoring_info.battery_level_percent = battery->percent;
}

/*! Calculate which role the peer device should take based of the figure of merits

    \param their_figure_of_merit Peer device figure of merit
 */
static GattRoleSelectionServiceControlOpCode
            peerFindRole_CalculatePeerRoleOpcode(grss_figure_of_merit_t their_figure_of_merit)
{
    GattRoleSelectionServiceControlOpCode opcode = GrssOpcodeBecomeSecondary;

    grss_figure_of_merit_t my_figure_of_merit;

    peer_find_role_calculate_score();
    my_figure_of_merit = peer_find_role_score();

    DEBUG_LOG("peerFindRole_CalculatePeerRoleOpcode. My figure of merit:0x%x, Theirs:0x%x",
               my_figure_of_merit,
               their_figure_of_merit);

    if (their_figure_of_merit > my_figure_of_merit)
    {
        opcode = GrssOpcodeBecomePrimary;
    }

    return opcode;
}

/*! Handler for the figure of merit from our peer

    This can be received for two reasons:
    * We have requested a read of the figure of merit
    * The peer has sent a notification of a new figure of merit value.

    Decide which role the peer should be in by calculating its own figure of 
    merit and comparing it to the value received from the peer.

    \param ind The merit message from our peer
 */
static void peer_find_role_handle_figure_of_merit(const GATT_ROLE_SELECTION_CLIENT_FIGURE_OF_MERIT_IND_T *ind)
{
    DEBUG_LOG("peer_find_role_handle_figure_of_merit. peer FOM = 0x%x", ind->figure_of_merit);

    PeerFindRole_SetPeerScore(ind->figure_of_merit);

    TimestampEvent(TIMESTAMP_EVENT_PEER_FIND_ROLE_MERIT_RECEIVED);

    if(peer_find_role_get_state() == PEER_FIND_ROLE_STATE_CLIENT_DECIDING)
    {
        PeerFindRole_DecideRoles();
    }
}

void PeerFindRole_DecideRoles(void)
{
    grss_figure_of_merit_t peer_figure_of_merit = PeerFindRole_GetPeerScore();

    if (GRSS_FIGURE_OF_MERIT_INVALID != peer_figure_of_merit)
    {
        peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

        switch(PeerFindRole_GetFixedRole())
        {
            case peer_find_role_fixed_role_primary:
                DEBUG_LOG("PeerFindRole_DecideRoles. Fixed as a primary, tell other device to become secondary");
                pfr->remote_role = GrssOpcodeBecomeSecondary;
                break;

            case peer_find_role_fixed_role_secondary:
                DEBUG_LOG("PeerFindRole_DecideRoles. Fixed as a secondary, tell other device to become primary");
                pfr->remote_role = GrssOpcodeBecomePrimary;
                break;

            default:
                DEBUG_LOG("PeerFindRole_DecideRoles. Calculate peer role");
                pfr->remote_role  = peerFindRole_CalculatePeerRoleOpcode(peer_figure_of_merit);
                break;
        }

        GattRoleSelectionClientChangePeerRole(&pfr->role_selection_client,
                                                    pfr->remote_role);

        peer_find_role_set_state(PEER_FIND_ROLE_STATE_CLIENT_AWAITING_CONFIRM);
    }
    else
    {
        DEBUG_LOG("PeerFindRole_DecideRoles FOM invalid");
    }
}



/*! Handler for the internal message to connect to the peer

    This message is sent conditionally when advertising and scan
    activities have terminated. Changing to the state
    #PEER_FIND_ROLE_STATE_CONNECTING_TO_DISCOVERED starts the
    connection process.
 */
static void peer_find_role_handle_connect_to_peer(void)
{
    peer_find_role_set_state(PEER_FIND_ROLE_STATE_CONNECTING_TO_DISCOVERED);
}


/*! Handler for the internal message to enable scanning

    If we are not already scanning, we check the current state
    and if necessary transition to a state that will enable scanning.
 */
static void peer_find_role_handle_enable_scanning(void)
{
    if (!LeScanManager_IsTaskScanning(PeerFindRoleGetTask()))
    {
        DEBUG_LOG("peer_find_role_handle_enable_scanning. State: %d", peer_find_role_get_state());

        switch (peer_find_role_get_state())
        {
            case PEER_FIND_ROLE_STATE_DISCOVER:
            case PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE:
                peer_find_role_start_scanning_if_inactive();
                break;

            default:
                break;
        }
    }
}


/*! Handle internal message for the command PeerFindRole_FindRoleCancel

    Use a standard handler to send a message to clients and finish
    any current activities.
 */
static void peer_find_role_handle_cancel_find_role(void)
{
    peer_find_role_completed(PEER_FIND_ROLE_CANCELLED);
}


void peer_find_role_update_media_flag(bool busy, peerFindRoleMediaBusyStatus_t mask)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();
    uint16 old_media_busy = pfr->media_busy;

    if (busy)
    {
        pfr->media_busy |= (uint16)mask;
        peer_find_role_stop_scan_if_active();

        MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_ENABLE_SCANNING);
        MessageSendConditionally(PeerFindRoleGetTask(),
                                 PEER_FIND_ROLE_INTERNAL_ENABLE_SCANNING, NULL,
                                 &pfr->media_busy);
    }
    else
    {
        pfr->media_busy &= ~((uint16)mask);
    }

    if (   busy 
        || (old_media_busy != pfr->media_busy))
    {
        DEBUG_LOG("peer_find_role_update_media_flag. Now 0x%x. Scan:%p. State:%d",
                    pfr->media_busy,
                    !LeScanManager_IsTaskScanning(PeerFindRoleGetTask()), pfr->state);
    }

}


/*! Handler for message #PEER_FIND_ROLE_INTERNAL_TELEPHONY_ACTIVE
    and #PEER_FIND_ROLE_INTERNAL_TELEPHONY_IDLE.

    Update our status for media with the bit PEER_FIND_ROLE_CALL_ACTIVE

    \param busy Whether we are now busy (TRUE) or idle (FALSE)
*/
static void peer_find_role_handle_telephony_busy(bool busy)
{
    peer_find_role_update_media_flag(busy, PEER_FIND_ROLE_CALL_ACTIVE);
}


/*! Handler for message #PEER_FIND_ROLE_INTERNAL_STREAMING_ACTIVE
    and #PEER_FIND_ROLE_INTERNAL_STREAMING_IDLE.

    Update our status for media with the bit PEER_FIND_ROLE_AUDIO_STREAMING

    \param busy Whether we are now busy (TRUE) or idle (FALSE)
*/
static void peer_find_role_handle_streaming_busy(bool busy)
{
    peer_find_role_update_media_flag(busy, PEER_FIND_ROLE_AUDIO_STREAMING);
}

/*! Handler for the Handset Service connected indication

    This message is received when a Handset is connected. When a
    handset is connected ensure that peer find role knows about
    handset connectivity in order to includes this in the score.

    \param ind  The Handset Service connected indication
*/
static void peer_find_role_handle_handset_connected_ind(const HANDSET_SERVICE_CONNECTED_IND_T *ind)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();
    UNUSED(ind);
    pfr->scoring_info.handset_connected = TRUE;
}


/*! Handler for the connection manager message for disconnection

    This message is received for creation of a connection as well as
    disconnection. We only act upon disconnection, using the
    loss of a connection to change state.

    \param ind  The connection manager indication
*/
static void peer_find_role_handle_connection_ind(const CON_MANAGER_CONNECTION_IND_T *ind)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    /* update score with handset connection status */
    if (!ind->ble && appDeviceIsHandset(&ind->bd_addr))
    {
        pfr->scoring_info.handset_connected = ind->connected;
    }

    if (ind->ble && !ind->connected && BdaddrIsSame(&ind->bd_addr, &pfr->peer_connection_typed_bdaddr.addr))
    {
        switch (peer_find_role_get_state())
        {
            case PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE:
            case PEER_FIND_ROLE_STATE_CLIENT:
            case PEER_FIND_ROLE_STATE_CLIENT_AWAITING_ENCRYPTION:
            case PEER_FIND_ROLE_STATE_CLIENT_PREPARING:
            case PEER_FIND_ROLE_STATE_SERVER:
            case PEER_FIND_ROLE_STATE_SERVER_PREPARING:
            case PEER_FIND_ROLE_STATE_SERVER_AWAITING_ENCRYPTION:
            case PEER_FIND_ROLE_STATE_CLIENT_DECIDING:
            case PEER_FIND_ROLE_STATE_CLIENT_AWAITING_CONFIRM:
            case PEER_FIND_ROLE_STATE_CONNECTING_TO_DISCOVERED:
                DEBUG_LOG("peer_find_role_handle_connection_ind. BLE disconnect");

                peer_find_role_set_state(PEER_FIND_ROLE_STATE_DISCOVER);
                break;

            case PEER_FIND_ROLE_STATE_UNINITIALISED:
            case PEER_FIND_ROLE_STATE_CHECKING_PEER:
                /* See no reason to expect in this case */
                break;

            case PEER_FIND_ROLE_STATE_INITIALISED:
                /* We have disconnected. If this is an initial timeout, notify clients */
                if (pfr->timeout_means_timeout)
                {
                    peer_find_role_notify_timeout();
                }
                else
                {
                    peer_find_role_notify_clients_if_pending();
                }
                break;

            case PEER_FIND_ROLE_STATE_SERVER_AWAITING_COMPLETION:
                pfr->gatt_cid = INVALID_CID;
                peer_find_role_set_state(PEER_FIND_ROLE_STATE_COMPLETED);
                break;

            case PEER_FIND_ROLE_STATE_COMPLETED:
                pfr->gatt_cid = INVALID_CID;
                peer_find_role_select_state_after_completion();
                break;

            default:
                DEBUG_LOG("peer_find_role_handle_connection_ind. BLE disconnect. Unhandled state:%d",
                          peer_find_role_get_state());
                break;
        }
    }
}


/*! Helper function to cancel internal messages for telephony status */
static void peer_find_role_cancel_telephony_events(void)
{
    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_TELEPHONY_ACTIVE);
    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_TELEPHONY_IDLE);
}


/*! Handler for events sent by telephony

    The messages are interpreted to indicate whether we have an active
    call and hence busy.

    Messages are sent internally to record the new state. The message
    for busy is sent immediately but the message for being idle is
    delayed, allowing for common usage where another activity may be
    started.

    \param id The identifier of the message from sys_msg
 */
static void peer_find_role_handle_telephony_event(MessageId id)
{
    switch(id)
    {
        case TELEPHONY_INCOMING_CALL:
        case TELEPHONY_INCOMING_CALL_OUT_OF_BAND_RINGTONE:
        case TELEPHONY_CALL_ANSWERED:
        case TELEPHONY_CALL_ONGOING:
        case TELEPHONY_UNENCRYPTED_CALL_STARTED:
        case TELEPHONY_CALL_AUDIO_RENDERED_LOCAL:
        case APP_HFP_SCO_CONNECTED_IND:
        case APP_HFP_SCO_INCOMING_RING_IND:
            DEBUG_LOG("peer_find_role_handle_telephony_event: 0x%x(%d) setting flag", id, id&0xFF);

            peer_find_role_cancel_telephony_events();
            MessageSend(PeerFindRoleGetTask(),
                        PEER_FIND_ROLE_INTERNAL_TELEPHONY_ACTIVE, NULL);
            break;

        case TELEPHONY_CALL_REJECTED:
        case TELEPHONY_CALL_ENDED:
        case TELEPHONY_CALL_HUNG_UP:
        case TELEPHONY_CALL_CONNECTION_FAILURE:
        case TELEPHONY_LINK_LOSS_OCCURRED:
        case TELEPHONY_DISCONNECTED:
        case TELEPHONY_CALL_AUDIO_RENDERED_REMOTE:
        case APP_HFP_SCO_INCOMING_ENDED_IND:
            DEBUG_LOG("peer_find_role_handle_telephony_event: 0x%x(%d) clearing flag", id, id&0xFF);

            peer_find_role_cancel_telephony_events();
            MessageSendLater(PeerFindRoleGetTask(),
                             PEER_FIND_ROLE_INTERNAL_TELEPHONY_IDLE, NULL,
                             PeerFindRoleConfigAllowScanAfterActivityDelayMs());
            break;

        case PAGING_START:
        case PAGING_STOP:
        case TELEPHONY_CONNECTED:
        case TELEPHONY_ERROR:
        case TELEPHONY_MUTE_ACTIVE:
        case TELEPHONY_MUTE_INACTIVE:
        case TELEPHONY_TRANSFERED:
            /* These don't affect us. No debug as would be noise */
            break;

        default:
            break;
    }
}


/*! Helper function to cancel internal messages for streaming status */
static void peer_find_role_cancel_streaming_events(void)
{
    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_STREAMING_ACTIVE);
    MessageCancelAll(PeerFindRoleGetTask(), PEER_FIND_ROLE_INTERNAL_STREAMING_IDLE);
}


/*! Handler for events sent by AV

    The messages are interpreted to indicate whether we are streaming
    media and hence busy.

    Messages are sent internally to record the new state. The message
    for busy is sent immediately but the message for being idle is
    delayed, allowing for common usage where another activity may be
    started.

    \param id The identifier of the message from #av_messages
 */
static void peer_find_role_handle_streaming_event(MessageId id)
{
    switch(id)
    {
        case AV_STREAMING_ACTIVE:
        case AV_STREAMING_ACTIVE_APTX:
            DEBUG_LOG("peer_find_role_handle_streaming_event: 0x%x(%d) setting flag", id, id&0xFF);

            peer_find_role_cancel_streaming_events();
            MessageSend(PeerFindRoleGetTask(),
                        PEER_FIND_ROLE_INTERNAL_STREAMING_ACTIVE, NULL);
            break;

        case AV_STREAMING_INACTIVE:
        case AV_DISCONNECTED:
            DEBUG_LOG("peer_find_role_handle_streaming_event: 0x%x(%d) clearing flag", id, id&0xFF);

            peer_find_role_cancel_streaming_events();
            MessageSendLater(PeerFindRoleGetTask(),
                             PEER_FIND_ROLE_INTERNAL_STREAMING_IDLE, NULL,
                             PeerFindRoleConfigAllowScanAfterActivityDelayMs());
            break;

        default:
            break;
    }
}

static inline void peer_find_role_restart_scanning(void)
{
    peer_find_role_set_state(PEER_FIND_ROLE_STATE_DISCOVER);
}

/*! Handle confirmation of the BLE security request (for encryption)

    If we receive a failure, setting our state to DISCOVER to restart the find role process

    \param[in] cfm  The confirmation to our call to ConnectionDmBleSecurityReq()
 */
static void peer_find_role_handle_ble_security(const CL_DM_BLE_SECURITY_CFM_T *cfm)
{
    if (cfm->status != ble_security_success)
    {
        DEBUG_LOG("peer_find_role_handle_ble_security. FAILED sts:%d", cfm->status);
        peer_find_role_restart_scanning();
    }
    else
    {
        DEBUG_LOG("peer_find_role_handle_ble_security. success");
        peer_find_role_set_state(PEER_FIND_ROLE_STATE_SERVER_PREPARING);
    }
}


/*! Internal handler for adverts

    Check if the advert we have received is expected (matches address), 
    changing state if so.

    \param[in] advert The advertising indication from the connection library
*/
static void peer_find_role_handle_adv_report_ind(const LE_SCAN_MANAGER_ADV_REPORT_IND_T *advert)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();
    PEER_FIND_ROLE_STATE state =peer_find_role_get_state();

    if (   PEER_FIND_ROLE_STATE_DISCOVER == state
        || PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE == state)
    {
        DEBUG_LOG("peer_find_role_handle_adv_report_ind ADDR:%06x PERM:%06x",
                        advert->current_taddr.addr.lap, advert->permanent_taddr.addr.lap);

        if (appDeviceIsPeer(&advert->permanent_taddr.addr))
        {
            DEBUG_LOG("peer_find_role_handle_adv_report_ind. Is peer");
            memcpy(&pfr->peer_connection_typed_bdaddr, &advert->current_taddr, sizeof(typed_bdaddr));

            peer_find_role_set_state(PEER_FIND_ROLE_STATE_DISCOVERED_DEVICE);
        }
    }
    else
    {
        DEBUG_LOG("peer_find_role_handle_adv_report_ind ADDR:%06x PERM:%06x NOT in discover state",
                        advert->current_taddr.addr.lap, advert->permanent_taddr.addr.lap);
    }
}


/*! \name Handlers for internal timeouts 

    These functions handle timers used internally */
/*! @{ */

/*! Handler for the timeout message #PEER_FIND_ROLE_INTERNAL_TIMEOUT_CONNECTION

    This message may be started when PeerFindRole_FindRole() is called.
    When it expires, we inform clients that we have not yet managed to
    find a role. The service continues to try to find a role until
    cancelled.
 */
static void peer_find_role_handle_initial_timeout(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    if (pfr->timeout_means_timeout)
    {
        peer_find_role_set_state(PEER_FIND_ROLE_STATE_INITIALISED);
    }
    else
    {
        /* We only timeout once, so clear the timeout value */
        pfr->role_timeout_ms = 0;

        peer_find_role_notify_timeout();
    }
}


/*! Handler for the timeout message #PEER_FIND_ROLE_INTERNAL_TIMEOUT_ADVERT_BACKOFF

    This timeout message is received to start advertising, and changes the
    state to #PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE which starts
    the advertising.
 */
static void peer_find_role_handle_advertising_backoff(void)
{
    if (PEER_FIND_ROLE_STATE_DISCOVER == peer_find_role_get_state())
    {
        peer_find_role_set_state(PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE);
    }
    else
    {
        DEBUG_LOG("peer_find_role_handle_advertising_backoff. Expired after already left state");
    }
}


/*! Internal handler for the timeout message
    #PEER_FIND_ROLE_INTERNAL_TIMEOUT_NOT_DISCONNECTED

    This timeout message is received if we have not disconnected.
    We disconnect by changing state, which forces the disconnection.
 */
static void peer_find_role_handle_disconnect_timeout(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    if (PEER_FIND_ROLE_STATE_SERVER_AWAITING_COMPLETION == peer_find_role_get_state())
    {
        pfr->gatt_cid = INVALID_CID;
        peer_find_role_set_state(PEER_FIND_ROLE_STATE_COMPLETED);
    }
    else
    {
        DEBUG_LOG("peer_find_role_handle_disconnect_timeout. Expired after already left state");
    }
}

/*! Internal handler for the timeout message
    #PEER_FIND_ROLE_INTERNAL_TIMEOUT_SERVER_ROLE_SELECTED

    This timeout message is received if we are the server and have not
    received the result of role selection from the client.
 */
static void peer_find_role_handle_server_role_selected_timeout(void)
{
    DEBUG_LOG("peer_find_role_handle_server_role_selected_timeout");

    if (PEER_FIND_ROLE_STATE_SERVER == peer_find_role_get_state())
    {
        /* Disconnect the link - after disconnection advertising will be re-started. */
        peer_find_role_disconnect_link();
    }
    else
    {
        DEBUG_LOG("peer_find_role_handle_server_role_selected_timeout. Expired after already left state");
    }
}

static bool peer_find_role_is_peer_le_address(const tp_bdaddr *tpaddr)
{
    bool peer_address = FALSE;
    
    DEBUG_LOG("peer_find_role_is_peer_le_address. type %d tpaddr %04x:%02x:%06x",
                        tpaddr->taddr.type,
                        tpaddr->taddr.addr.nap,
                        tpaddr->taddr.addr.uap,
                        tpaddr->taddr.addr.lap
                        );
    
    if (tpaddr->transport == TRANSPORT_BLE_ACL)
    {
        if (BtDevice_LeDeviceIsPeer(tpaddr))
        {
            DEBUG_LOG("peer_find_role_is_peer_le_address. Is peer");
            peer_address = TRUE;
        }
    }
    
    return peer_address;
}

static void peer_find_role_handle_con_manager_connection(const CON_MANAGER_TP_CONNECT_IND_T *conn)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_handle_con_manager_connection. 0x%06x Incoming:%d state:%d",
                    conn->tpaddr.taddr.addr.lap, conn->incoming, peer_find_role_get_state());

    if (conn->tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        if (peer_find_role_is_peer_le_address(&conn->tpaddr))
        {
            /* Set the quality of service to use (effectively the speed 
               of data exchange) */
			if(conn->incoming)
			{
            	ConManagerRequestDeviceQos(&conn->tpaddr, cm_qos_passive);
            }
            /* Store address of the connected peer device */
            memcpy(&pfr->peer_connection_typed_bdaddr, &conn->tpaddr.taddr, sizeof(typed_bdaddr));
            
            DEBUG_LOG("peer_find_role_handle_con_manager_connection. Store addr 0x%06x",
                    pfr->peer_connection_typed_bdaddr.addr.lap);
        }

        if (   PEER_FIND_ROLE_STATE_CONNECTING_TO_DISCOVERED == peer_find_role_get_state()
            && !conn->incoming)
        {
            peer_find_role_set_state(PEER_FIND_ROLE_STATE_SERVER_AWAITING_ENCRYPTION);

            TimestampEvent(TIMESTAMP_EVENT_PEER_FIND_ROLE_CONNECTED_SERVER);
        }
    }
}

static void peer_find_role_handle_con_manager_disconnection(const CON_MANAGER_TP_DISCONNECT_IND_T *conn)
{
    UNUSED(conn);
}

static void peer_find_role_handle_prepared(void)
{
    PEER_FIND_ROLE_STATE state = peer_find_role_get_state();

    DEBUG_LOG("peer_find_role_handle_prepared state 0x%x", state);

    switch (state)
    {
    case PEER_FIND_ROLE_STATE_SERVER_PREPARING:
        peer_find_role_set_state(PEER_FIND_ROLE_STATE_SERVER);
        break;

    case PEER_FIND_ROLE_STATE_CLIENT_PREPARING:
        peer_find_role_set_state(PEER_FIND_ROLE_STATE_CLIENT_DECIDING);
        break;

    default:
        DEBUG_LOG("peer_find_role_handle_prepared unhandled");
        break;
    }
}


static void peerFindRole_HandleTimeoutNoFomReceived(void)
{
    DEBUG_LOG("peerFindRole_HandleFomTimeout state = %d", peer_find_role_get_state());

    if(PEER_FIND_ROLE_STATE_CLIENT_DECIDING == peer_find_role_get_state())
    {
        /* No fom? Ask for it */
        peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();
        PanicFalse(GattRoleSelectionClientEnablePeerFigureOfMeritNotifications(&pfr->role_selection_client));
    }
}

/*! @} */


/*! The message handler for the \ref peer_find_role service

    This handler is called for messages sent to the service, including internal
    messages.

    \note These messages can be received even if Peer Find Role is not running.
        Must be careful in deciding whether to act on a message.

    \param task     The task called. Ignored.
    \param id       The message identifier
    \param message  Pointer to the message content. Often NULL
 */
static void peer_find_role_handler(Task task, MessageId id, Message message)
{
    PEER_FIND_ROLE_STATE state = peer_find_role_get_state();
    bool handle_as_active = state > PEER_FIND_ROLE_STATE_INITIALISED;

    UNUSED(task);

    /* Always handled messages */
    switch (id)
    {
        /* ---- LE SCAN Manager messages ----
            LESM can respond with LE_SCAN_MANAGER_RESULT_BUSY. If this happens
            peer find role may need to re-try starting/stopping scanning.
        */
        case LE_SCAN_MANAGER_START_CFM:
            peer_find_role_handle_le_scan_manager_start_cfm((LE_SCAN_MANAGER_START_CFM_T*)message);
            return;

        case LE_SCAN_MANAGER_STOP_CFM:
            peer_find_role_handle_le_scan_manager_stop_cfm((LE_SCAN_MANAGER_STOP_CFM_T*)message);
            return;

        default:
            break;
    }

            /****************************************
             * MOST MESSAGES RECEIVED EVEN WHEN PEER
             * FIND ROLE IS NOT ACTIVE
             ****************************************/

    if (handle_as_active)
    {
        switch (id)
        {
            /* ---- GATT Role Selection messages ---- */
            case GATT_ROLE_SELECTION_CLIENT_INIT_CFM:
                peer_find_role_handle_client_init((const GATT_ROLE_SELECTION_CLIENT_INIT_CFM_T *)message);
                break;

            case GATT_ROLE_SELECTION_SERVER_CHANGE_ROLE_IND:
                peer_find_role_handle_change_role_ind((const GATT_ROLE_SELECTION_SERVER_CHANGE_ROLE_IND_T*)message);
                break;

            case GATT_ROLE_SELECTION_CLIENT_COMMAND_CFM:
                peer_find_role_handle_command_cfm((const GATT_ROLE_SELECTION_CLIENT_COMMAND_CFM_T*)message);
                break;

            case GATT_ROLE_SELECTION_CLIENT_FIGURE_OF_MERIT_IND:
                peer_find_role_handle_figure_of_merit((const GATT_ROLE_SELECTION_CLIENT_FIGURE_OF_MERIT_IND_T*)message);
                break;

             /* ---- Advertising Manager messages ---- */
             case LE_ADV_MGR_SELECT_DATASET_CFM:
                 peer_find_role_handle_adv_mgr_select_dataset_cfm((const LE_ADV_MGR_SELECT_DATASET_CFM_T *)message);
                 break;

             case LE_ADV_MGR_RELEASE_DATASET_CFM:
                 peer_find_role_handle_adv_mgr_release_dataset_cfm((const LE_ADV_MGR_RELEASE_DATASET_CFM_T *)message);
                 break;

            case LE_SCAN_MANAGER_ADV_REPORT_IND:
                peer_find_role_handle_adv_report_ind((const LE_SCAN_MANAGER_ADV_REPORT_IND_T*)message);
                break;

            /* ---- Direct Connection Library Messages ---- */
            case CL_DM_BLE_SECURITY_CFM:
                peer_find_role_handle_ble_security((const CL_DM_BLE_SECURITY_CFM_T*)message);
                break;

            /* ---- Connection Manager messages ---- */
            case CON_MANAGER_TP_CONNECT_IND:
                peer_find_role_handle_con_manager_connection((const CON_MANAGER_TP_CONNECT_IND_T *)message);
                break;

            case CON_MANAGER_TP_DISCONNECT_IND:
                peer_find_role_handle_con_manager_disconnection((const CON_MANAGER_TP_DISCONNECT_IND_T *)message);
                break;

            /* ---- Internal messages ---- */
            case PEER_FIND_ROLE_INTERNAL_UPDATE_SCORE:
                peer_find_role_update_server_score();
                break;

            case PEER_FIND_ROLE_INTERNAL_CONNECT_TO_PEER:
                peer_find_role_handle_connect_to_peer();
                break;

            case PEER_FIND_ROLE_INTERNAL_ENABLE_SCANNING:
                peer_find_role_handle_enable_scanning();
                break;

            case PEER_FIND_ROLE_INTERNAL_TIMEOUT_CONNECTION:
                peer_find_role_handle_initial_timeout();
                break;

            case PEER_FIND_ROLE_INTERNAL_TIMEOUT_ADVERT_BACKOFF:
                peer_find_role_handle_advertising_backoff();
                break;

            case PEER_FIND_ROLE_INTERNAL_TIMEOUT_NOT_DISCONNECTED:
                peer_find_role_handle_disconnect_timeout();
                break;

            case PEER_FIND_ROLE_INTERNAL_TIMEOUT_SERVER_ROLE_SELECTED:
                peer_find_role_handle_server_role_selected_timeout();
                break;

            case PEER_FIND_ROLE_INTERNAL_TIMEOUT_NO_FOM_RECEIVED:
                peerFindRole_HandleTimeoutNoFomReceived();
                break;

            case PEER_FIND_ROLE_INTERNAL_PREPARED:
                peer_find_role_handle_prepared();
                break;

            default:
                handle_as_active = FALSE;
                break;
        }
    }

    if (handle_as_active)
    {
        return;
    }

    switch (id)
    {
        /* ---- Internal messages ---- */
            /* Cancel can be sent even when not running */
        case PEER_FIND_ROLE_INTERNAL_CANCEL_FIND_ROLE:
            peer_find_role_handle_cancel_find_role();
            break;

            /* These update the media flag, used for controlling scanning
               when starting find role */
        case PEER_FIND_ROLE_INTERNAL_TELEPHONY_ACTIVE:
        case PEER_FIND_ROLE_INTERNAL_TELEPHONY_IDLE:
            peer_find_role_handle_telephony_busy(PEER_FIND_ROLE_INTERNAL_TELEPHONY_ACTIVE == id);
            break;
        
        case PEER_FIND_ROLE_INTERNAL_STREAMING_ACTIVE:
        case PEER_FIND_ROLE_INTERNAL_STREAMING_IDLE:
            peer_find_role_handle_streaming_busy(PEER_FIND_ROLE_INTERNAL_STREAMING_ACTIVE == id);
            break;

        /* ---- Physical state messages ---- */
        case PHY_STATE_CHANGED_IND:
            peer_find_role_handle_phy_state((const PHY_STATE_CHANGED_IND_T*)message);
            break;

        /* ---- Charger monitor messages ---- */
        case CHARGER_MESSAGE_ATTACHED:
        case CHARGER_MESSAGE_DETACHED:
            peer_find_role_handle_charger_state(CHARGER_MESSAGE_ATTACHED == id);
            break;

        case CHARGER_MESSAGE_CHARGING_OK: /* Spam */
        case CHARGER_MESSAGE_CHARGING_LOW:
            break;

        /* ---- Accelerometer messages ---- */
        case ACCELEROMETER_MESSAGE_IN_MOTION:
        case ACCELEROMETER_MESSAGE_NOT_IN_MOTION:
            peer_find_role_handle_accelerometer_state(ACCELEROMETER_MESSAGE_IN_MOTION == id);
            break;

        /* ---- Battery messages ---- */
        case MESSAGE_BATTERY_LEVEL_UPDATE_PERCENT:
            peer_find_role_handle_battery_level((const MESSAGE_BATTERY_LEVEL_UPDATE_PERCENT_T *)message);
            break;

        /* ---- Connection Manager messages ---- */
        case CON_MANAGER_CONNECTION_IND:
            peer_find_role_handle_connection_ind((const CON_MANAGER_CONNECTION_IND_T *)message);
            break;

        /* ---- Handset Service messages ---- */
        case HANDSET_SERVICE_CONNECTED_IND:
            peer_find_role_handle_handset_connected_ind((const HANDSET_SERVICE_CONNECTED_IND_T *)message);
            break;

        /* ---- HFP Profile messages ---- */
        case PAGING_START:
        case PAGING_STOP:
        case TELEPHONY_CONNECTED:
        case TELEPHONY_INCOMING_CALL:
        case TELEPHONY_INCOMING_CALL_OUT_OF_BAND_RINGTONE:
        case TELEPHONY_CALL_ANSWERED:
        case TELEPHONY_CALL_REJECTED:
        case TELEPHONY_CALL_ONGOING:
        case TELEPHONY_CALL_ENDED:
        case TELEPHONY_CALL_HUNG_UP:
        case TELEPHONY_UNENCRYPTED_CALL_STARTED:
        case TELEPHONY_CALL_CONNECTION_FAILURE:
        case TELEPHONY_LINK_LOSS_OCCURRED:
        case TELEPHONY_DISCONNECTED:
        case TELEPHONY_CALL_AUDIO_RENDERED_LOCAL:
        case TELEPHONY_CALL_AUDIO_RENDERED_REMOTE:
        case TELEPHONY_ERROR:
        case TELEPHONY_MUTE_ACTIVE:
        case TELEPHONY_MUTE_INACTIVE:
        case TELEPHONY_TRANSFERED:
        case APP_HFP_SCO_INCOMING_ENDED_IND:
        case APP_HFP_SCO_CONNECTED_IND:
        case APP_HFP_SCO_INCOMING_RING_IND:
            peer_find_role_handle_telephony_event(id);
            break;

        /* ---- AV Profile messages ---- */
        case AV_STREAMING_ACTIVE:
        case AV_STREAMING_ACTIVE_APTX:
        case AV_STREAMING_INACTIVE:
        case AV_DISCONNECTED:
            peer_find_role_handle_streaming_event(id);
            break;

        default:
            DEBUG_LOG("peer_find_role_handler. Unhandled message %d(0x%x)", id, id);
            break;
    }
}


/*! Helper function to initialise the GATT server for role selection
 */
static void peer_find_role_gatt_role_selection_server_init(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    pfr->gatt_cid = INVALID_CID;

    if (!GattRoleSelectionServerInit(&pfr->role_selection_server,
                                    PeerFindRoleGetTask(),
                                    HANDLE_ROLE_SELECTION_SERVICE,
                                    HANDLE_ROLE_SELECTION_SERVICE_END,
                                    TRUE))
    {
        DEBUG_LOG("peer_find_role_gatt_role_selection_server_init. Server init failed");
        Panic();
    }
}


/* Return the number of items in the advert.
   For simplicity/safety don't make the same check when getting data items. */
static unsigned int peer_find_role_NumberOfAdvItems(const le_adv_data_params_t * params)
{
    unsigned items = 0;

    if (peer_find_role_is_in_advertising_state())
    {
        if((le_adv_data_set_peer == params->data_set) && \
           (le_adv_data_completeness_full == params->completeness) && \
           (le_adv_data_placement_advert == params->placement))
        {
            items = NUMBER_OF_ADVERT_DATA_ITEMS;
        }
    }

    return items;
}


static le_adv_data_item_t peer_find_role_GetAdvDataItems(const le_adv_data_params_t * params, unsigned int id)
{
    UNUSED(id);

    if((le_adv_data_set_peer == params->data_set) && \
        (le_adv_data_completeness_full == params->completeness) && \
        (le_adv_data_placement_advert == params->placement))
    {
        return peer_find_role_advert;
    }
    else
    {
        Panic();
        return peer_find_role_advert;
    };
}

static void peer_find_role_ReleaseAdvDataItems(const le_adv_data_params_t * params)
{
    UNUSED(params);

    return;
}

static void peer_find_role_GattConnect(uint16 cid)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();
    PEER_FIND_ROLE_STATE state = peer_find_role_get_state();
    tp_bdaddr tpaddr;

    switch (state)
    {
        case PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE:
        case PEER_FIND_ROLE_STATE_DISCOVERED_DEVICE:
        case PEER_FIND_ROLE_STATE_CONNECTING_TO_DISCOVERED:
            DEBUG_LOG_FN_ENTRY("peer_find_role_GattConnect. client cid:0x%x state:%d", cid, state);

            peer_find_role_advertising_activity_clear();

            /* Do not panic. Connection could have dropped or failed to establish */
            if (VmGetBdAddrtFromCid(cid, &tpaddr))
            {
                DEBUG_LOG_INFO("peer_find_role_GattConnect. addr:%06x", tpaddr.taddr.addr.lap);

                if (peer_find_role_is_peer_le_address(&tpaddr))
                {
                    pfr->gatt_cid = cid;
                    peer_find_role_set_state(PEER_FIND_ROLE_STATE_CLIENT);

                    /* Store address of the connected peer device */
                    memcpy(&pfr->peer_connection_typed_bdaddr, &tpaddr.taddr, sizeof(typed_bdaddr));

                    TimestampEvent(TIMESTAMP_EVENT_PEER_FIND_ROLE_CONNECTED_CLIENT);
                }
                else
                {
                    peer_find_role_set_state(PEER_FIND_ROLE_STATE_DISCOVER);
                }
            }
            else
            {
                DEBUG_LOG_WARN("peer_find_role_GattConnect. Gatt 'gone' by time received GattConnect.");
            }
            break;

        case PEER_FIND_ROLE_STATE_SERVER_AWAITING_ENCRYPTION:
        case PEER_FIND_ROLE_STATE_SERVER:
            DEBUG_LOG_FN_ENTRY("peer_find_role_GattConnect. server cid:0x%x", cid);

            if (VmGetBdAddrtFromCid(cid, &tpaddr))
            {
                if(peer_find_role_is_peer_le_address(&tpaddr))
                {
                    DEBUG_LOG_INFO("peer_find_role_GattConnect. addr:%06x", tpaddr.taddr.addr.lap);

                    pfr->gatt_cid = cid;

                    /* Store address of the connected peer device */
                    memcpy(&pfr->peer_connection_typed_bdaddr, &tpaddr.taddr, sizeof(typed_bdaddr));

                    /* Set the server score to "invalid" until this device is prepared
                        and ready to calculate its score. */
                    peer_find_role_reset_server_score();
                }
            }
            else
            {
                DEBUG_LOG_WARN("peer_find_role_GattConnect. Gatt server 'gone' by time received GattConnect.");
            }
            break;

        case PEER_FIND_ROLE_STATE_UNINITIALISED:
        case PEER_FIND_ROLE_STATE_INITIALISED:
            /* No point in debugging in these states.
               Not able to unregister an observer */
            break;

        default:
            DEBUG_LOG_FN_ENTRY("peer_find_role_GattConnect. cid:0x%x. Not handled in state:%d", cid, state);
            break;
    }
}

static void peer_find_role_GattDisconnect(uint16 cid)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG("peer_find_role_GattDisconnect pfr->cid 0x%x cid 0x%x (state:%d)", 
                    pfr->gatt_cid, cid, peer_find_role_get_state());

    /* We choose not to do anything when GATT is disconnected, as the link
       will be disconnected (if it hasn't already been). */
}

bool PeerFindRole_Init(Task init_task)
{
    UNUSED(init_task);

    peer_find_role._task.handler = peer_find_role_handler;
    TaskList_InitialiseWithCapacity(PeerFindRoleGetTaskList(), PEER_FIND_ROLE_REGISTERED_TASKS_LIST_INIT_CAPACITY);

    peer_find_role_scoring_setup();

    peer_find_role_gatt_role_selection_server_init();

    /* track connections */
    ConManagerRegisterConnectionsClient(PeerFindRoleGetTask());
    ConManagerRequestDefaultQos(cm_transport_ble, cm_qos_low_latency);
    ConManagerRegisterTpConnectionsObserver(cm_transport_ble, PeerFindRoleGetTask());
    GattConnect_UpdateMinAcceptableMtu(PeerFindRoleConfigMtu());
    GattConnect_RegisterObserver(&peer_find_role_connect_callback);

    /* track streaming activities */
    appAvUiClientRegister(PeerFindRoleGetTask());
    appHfpStatusClientRegister(PeerFindRoleGetTask());
    Telephony_RegisterForMessages(PeerFindRoleGetTask());
    HandsetService_ClientRegister(PeerFindRoleGetTask());

        /* setup advertising */
    peer_find_role_advert.size = SIZE_PEER_FIND_ROLE_ADVERT;
    peer_find_role_advert.data = peer_find_role_advert_data;
    LeAdvertisingManager_Register(NULL, &peer_find_role_advert_callback);

    TaskList_Initialise(&peer_find_role.prepare_tasks);

    peer_find_role_set_state(PEER_FIND_ROLE_STATE_INITIALISED);

    /* invalidate the fixed role setting forcing a PsRetrieve */
    peer_find_role.fixed_role = peer_find_role_fixed_role_invalid;

    return TRUE;
}
