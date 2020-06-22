/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Handset service state machine
*/

#include <bdaddr.h>
#include <device.h>

#include <bt_device.h>
#include <connection_manager.h>
#include <device_properties.h>
#include <profile_manager.h>
#include <timestamp_event.h>

#include "handset_service_config.h"
#include "handset_service_protected.h"
#include "handset_service_sm.h"


/*! \brief Cast a Task to a handset_service_state_machine_t.
    This depends on task_data being the first member of handset_service_state_machine_t. */
#define handsetServiceSm_GetSmFromTask(task) ((handset_service_state_machine_t *)task)

/*! \brief Test if the current state is in the "CONNECTING" pseudo-state. */
#define handsetServiceSm_IsConnectingBredrState(state) \
    ((state & HANDSET_SERVICE_CONNECTING_BREDR_STATE_MASK) == HANDSET_SERVICE_CONNECTING_BREDR_STATE_MASK)

/*! \brief Add one mask of profiles to another. */
#define handsetServiceSm_MergeProfiles(profiles, profiles_to_merge) \
    ((profiles) |= (profiles_to_merge))

/*! \brief Remove a set of profiles from another. */
#define handsetServiceSm_RemoveProfiles(profiles, profiles_to_remove) \
    ((profiles) &= ~(profiles_to_remove))

#define handsetServiceSm_ProfileIsSet(profiles, profile) \
    (((profiles) & (profile)) == (profile))

/*
    Helper Functions
*/

/*! \brief Convert a profile bitmask to Profile Manager profile connection list. */
static void handsetServiceSm_ConvertProfilesToProfileList(uint8 profiles, uint8 *profile_list, size_t profile_list_count)
{
    int entry = 0;
    profile_t pm_profile = profile_manager_hfp_profile;

    /* Loop over the profile manager profile_t enum values and if the matching
       profile mask from bt_device.h is set, add it to profile_list.

       Write up to (profile_list_count - 1) entries and leave space for the
       'last entry' marker at the end. */
    while ((pm_profile < profile_manager_max_number_of_profiles) && (entry < (profile_list_count - 1)))
    {
        switch (pm_profile)
        {
        case profile_manager_hfp_profile:
            if (handsetServiceSm_ProfileIsSet(profiles, DEVICE_PROFILE_HFP))
            {
                profile_list[entry++] = profile_manager_hfp_profile;
            }
            break;

        case profile_manager_a2dp_profile:
            if (handsetServiceSm_ProfileIsSet(profiles, DEVICE_PROFILE_A2DP))
            {
                profile_list[entry++] = profile_manager_a2dp_profile;
            }
            break;

        case profile_manager_avrcp_profile:
            if (handsetServiceSm_ProfileIsSet(profiles, DEVICE_PROFILE_AVRCP))
            {
                profile_list[entry++] = profile_manager_avrcp_profile;
            }
            break;

        default:
            break;
        }

        pm_profile++;
    }

    /* The final entry in the list is the 'end of list' marker */
    profile_list[entry] = profile_manager_max_number_of_profiles;
}

static bool handsetServiceSm_AllConnectionsDisconnected(handset_service_state_machine_t *sm, bool bredr_only)
{
    bool bredr_connected = FALSE;
    bool ble_connected = FALSE;
    uint8 connected_profiles = 0;
    
    if (!BdaddrIsZero(&sm->handset_addr))
    {
        bredr_connected = ConManagerIsConnected(&sm->handset_addr);
    }

    if (BtDevice_DeviceIsValid(sm->handset_device))
    {
        connected_profiles = BtDevice_GetConnectedProfiles(sm->handset_device);
    }
    
    if (!bredr_only)
    {
        ble_connected = HandsetServiceSm_IsLeConnected(sm);
    }

    DEBUG_LOG("handsetServiceSm_AllConnectionsDisconnected bredr %d profiles 0x%x le %d",
              bredr_connected, connected_profiles, ble_connected);

    return (!bredr_connected 
            && (connected_profiles == 0)
            && !ble_connected
            );
}

/*  Helper to request a BR/EDR connection to the handset from connection manager. */
static void handsetService_ConnectAcl(handset_service_state_machine_t *sm)
{
    HS_LOG("handsetService_ConnectAcl");

    /* Post message back to ourselves, blocked on creating ACL */
    MessageSendConditionally(&sm->task_data,
                             HANDSET_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE,
                             NULL, ConManagerCreateAcl(&sm->handset_addr));

    sm->acl_create_called = TRUE;
    sm->acl_attempts++;
}

/*! \brief Get the client facing address for a state machine

    This function returns the bdaddr of the handset this state machine
    represents. Typically this would be the bdaddr a client uses to
    refer to this particular handset sm.

    A handset can be connected by BR/EDR and / or LE, so this function will
    first try to return the BR/EDR address but if that is invalid it will
    return the LE address.

    \param[in] sm Handset state machine to get the address for.
    \param[out] addr Client facing address of the handset sm.
*/
static void handsetServiceSm_GetBdAddr(handset_service_state_machine_t *sm, bdaddr *addr)
{
    if (!BdaddrIsZero(&sm->handset_addr))
    {
        *addr = sm->handset_addr;
    }
    else
    {
        *addr = HandsetServiceSm_GetLeTpBdaddr(sm).taddr.addr;
    }
}

/*
    State Enter & Exit functions.
*/

static void handsetServiceSm_EnterDisconnected(handset_service_state_machine_t *sm)
{
    bdaddr addr;
    
    HS_LOG("handsetServiceSm_EnterDisconnected");

    /* Complete any outstanding connect stop request */
    HandsetServiceSm_CompleteConnectStopRequests(sm, handset_service_status_disconnected);

    /* Complete any outstanding connect requests. */
    HandsetServiceSm_CompleteConnectRequests(sm, handset_service_status_failed);

    /* Complete any outstanding disconnect requests. */
    HandsetServiceSm_CompleteDisconnectRequests(sm, handset_service_status_success);

    /* Notify registered clients of this disconnect event. */
    handsetServiceSm_GetBdAddr(sm, &addr);
    if (sm->disconnect_reason == hci_error_conn_timeout)
    {
        HandsetService_SendDisconnectedIndNotification(&addr, handset_service_status_link_loss);
    }
    else
    {
        HandsetService_SendDisconnectedIndNotification(&addr, handset_service_status_disconnected);
    }

    /* If there are no open connections to this handset, destroy this state machine. */
    if (handsetServiceSm_AllConnectionsDisconnected(sm, FALSE))
    {
        HS_LOG("handsetServiceSm_EnterDisconnected destroying sm for dev 0x%x", sm->handset_device);
        HandsetServiceSm_DeInit(sm);
    }
}

static void handsetServiceSm_ExitDisconnected(handset_service_state_machine_t *sm)
{
    UNUSED(sm);
    HS_LOG("handsetServiceSm_ExitDisconnected");
}

static void handsetServiceSm_EnterConnectingBredrAcl(handset_service_state_machine_t *sm)
{
    HS_LOG("handsetServiceSm_EnterConnectingBredrAcl");

    handsetService_ConnectAcl(sm);
}

static void handsetServiceSm_ExitConnectingBredrAcl(handset_service_state_machine_t *sm)
{
    HS_LOG("handsetServiceSm_ExitConnectingBredrAcl");

    /* Cancel any queued internal ACL connect retry requests */
    MessageCancelAll(&sm->task_data, HANDSET_SERVICE_INTERNAL_CONNECT_ACL_RETRY_REQ);

    /* Reset ACL connection attempt count. */
    sm->acl_attempts = 0;
}

static void handsetServiceSm_EnterConnectingBredrProfiles(handset_service_state_machine_t *sm)
{
    uint8 profile_list[4];

    HS_LOG("handsetServiceSm_EnterConnectingBredrProfiles");

    /* Connect the requested profiles.
       The requested profiles bitmask needs to be converted to the format of
       the profiles_connect_order device property and set on the device before
       calling profile manager to do the connect. */
    handsetServiceSm_ConvertProfilesToProfileList(sm->profiles_requested,
                                                  profile_list, ARRAY_DIM(profile_list));
    Device_SetProperty(sm->handset_device,
                       device_property_profiles_connect_order, profile_list, sizeof(profile_list));
    ProfileManager_ConnectProfilesRequest(&sm->task_data, sm->handset_device);
}

static void handsetServiceSm_ExitConnectingBredrProfiles(handset_service_state_machine_t *sm)
{
    UNUSED(sm);
    HS_LOG("handsetServiceSm_ExitConnectingBredrProfiles");
}

/* Enter the CONNECTING pseudo-state */
static void handsetServiceSm_EnterConnectingBredr(handset_service_state_machine_t *sm)
{
    HS_LOG("handsetServiceSm_EnterConnectingBredr");

    sm->acl_create_called = FALSE;
}

/* Exit the CONNECTING pseudo-state */
static void handsetServiceSm_ExitConnectingBredr(handset_service_state_machine_t *sm)
{
    HS_LOG("handsetServiceSm_ExitConnectingBredr");

    if (sm->acl_create_called)
    {
        /* We have finished (successfully or not) attempting to connect, so
        we can relinquish our lock on the ACL.  Bluestack will then close
        the ACL when there are no more L2CAP connections */
        ConManagerReleaseAcl(&sm->handset_addr);
    }
}

static void handsetServiceSm_EnterConnectedBredr(handset_service_state_machine_t *sm)
{
    uint8 connected_profiles = BtDevice_GetConnectedProfiles(sm->handset_device);

    HS_LOG("handsetServiceSm_EnterConnectedBredr");

    /* Complete any outstanding stop connect request */
    HandsetServiceSm_CompleteConnectStopRequests(sm, handset_service_status_connected);

    /* Complete outstanding connect requests */
    HandsetServiceSm_CompleteConnectRequests(sm, handset_service_status_success);

    /* Complete any outstanding disconnect requests. */
    HandsetServiceSm_CompleteDisconnectRequests(sm, handset_service_status_failed);

    /* Notify registered clients about this connection */
    HandsetService_SendConnectedIndNotification(sm->handset_device, connected_profiles);

    /* Update the "last connected" profile list */
    BtDevice_SetLastConnectedProfilesForDevice(sm->handset_device, connected_profiles, TRUE);
}

static void handsetServiceSm_ExitConnectedBredr(handset_service_state_machine_t *sm)
{
    UNUSED(sm);
    HS_LOG("handsetServiceSm_ExitConnectedBredr");
}

static void handsetServiceSm_EnterDisconnectingBredr(handset_service_state_machine_t *sm)
{
    uint8 profiles_connected = BtDevice_GetConnectedProfiles(sm->handset_device);
    uint8 profiles_to_disconnect = (sm->profiles_requested | profiles_connected);
    uint8 profile_list[4];

    HS_LOG("handsetServiceSm_EnterDisconnectingBredr requested 0x%x connected 0x%x, to_disconnect 0x%x",
           sm->profiles_requested, profiles_connected, profiles_to_disconnect);

    /* Disconnect any profiles that were either requested or are currently
       connected. */
    handsetServiceSm_ConvertProfilesToProfileList(profiles_to_disconnect,
                                                  profile_list, ARRAY_DIM(profile_list));
    Device_SetProperty(sm->handset_device,
                       device_property_profiles_disconnect_order,
                       profile_list, sizeof(profile_list));
    ProfileManager_DisconnectProfilesRequest(&sm->task_data, sm->handset_device);
}

static void handsetServiceSm_ExitDisconnectingBredr(handset_service_state_machine_t *sm)
{
    UNUSED(sm);
    HS_LOG("handsetServiceSm_ExitDisconnectingBredr");
}

static void handsetServiceSm_EnterConnectedLe(handset_service_state_machine_t *sm)
{
    HS_LOG("handsetServiceSm_EnterConnectedLe");
    
    /* Need to call functions in the case it is transitioning from BRDER state */
    
    /* Complete any outstanding connect stop request */
    HandsetServiceSm_CompleteConnectStopRequests(sm, handset_service_status_disconnected);

    /* Complete any outstanding connect requests. */
    HandsetServiceSm_CompleteConnectRequests(sm, handset_service_status_failed);
    
    if (handsetServiceSm_AllConnectionsDisconnected(sm, TRUE))
    {
        HandsetServiceSm_SetDevice(sm, (device_t)0);
    }
}

static void handsetServiceSm_ExitConnectedLe(handset_service_state_machine_t *sm)
{
    UNUSED(sm);
    HS_LOG("handsetServiceSm_ExitConnectedLe");
}

static void handsetServiceSm_EnterDisconnectingLe(handset_service_state_machine_t *sm)
{
    HS_LOG("handsetServiceSm_EnterDisconnectingLe");
    /* Remove LE ACL */
    if (!BdaddrTpIsEmpty(&sm->le_addr))
    {   
        ConManagerReleaseTpAcl(&sm->le_addr);
    }
}

static void handsetServiceSm_ExitDisconnectingLe(handset_service_state_machine_t *sm)
{
    UNUSED(sm);
    HS_LOG("handsetServiceSm_ExitDisconnectingLe");
}

static void handsetServiceSm_SetBdedrDisconnectedState(handset_service_state_machine_t *sm)
{
    if (HandsetServiceSm_IsLeConnected(sm))
    {
        HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_CONNECTED_LE);
    }
    else
    {
        HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_DISCONNECTED);
    }
}

static void handsetServiceSm_SetBdedrDisconnectingCompleteState(handset_service_state_machine_t *sm)
{
    if (HandsetServiceSm_IsLeConnected(sm))
    {
        HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_DISCONNECTING_LE);
    }
    else
    {
        HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_DISCONNECTED);
    }
}


/*
    Public functions
*/

void HandsetServiceSm_SetDevice(handset_service_state_machine_t *sm, device_t device)
{
    if (sm)
    {
        if (device)
        {
            sm->handset_addr = DeviceProperties_GetBdAddr(device);
        }
        else
        {
            BdaddrSetZero(&sm->handset_addr);
        }
        sm->handset_device = device;
    }
}

/* */
void HandsetServiceSm_SetState(handset_service_state_machine_t *sm, handset_service_state_t state)
{
    handset_service_state_t old_state = sm->state;

    /* It is not valid to re-enter the same state */
    assert(old_state != state);

    DEBUG_LOG_STATE("HandsetServiceSm_SetState %p %d->%d", sm, old_state, state);

    /* Handle state exit functions */
    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_DISCONNECTED:
        handsetServiceSm_ExitDisconnected(sm);
        break;
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
        handsetServiceSm_ExitConnectingBredrAcl(sm);
        break;
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES:
        handsetServiceSm_ExitConnectingBredrProfiles(sm);
        break;
    case HANDSET_SERVICE_STATE_CONNECTED_BREDR:
        handsetServiceSm_ExitConnectedBredr(sm);
        break;
    case HANDSET_SERVICE_STATE_DISCONNECTING_BREDR:
        handsetServiceSm_ExitDisconnectingBredr(sm);
        break;
    case HANDSET_SERVICE_STATE_CONNECTED_LE:
        handsetServiceSm_ExitConnectedLe(sm);
        break;
    case HANDSET_SERVICE_STATE_DISCONNECTING_LE:
        handsetServiceSm_ExitDisconnectingLe(sm);
        break;
    default:
        break;
    }

    /* Check for a exit transition from the CONNECTING pseudo-state */
    if (handsetServiceSm_IsConnectingBredrState(old_state) && !handsetServiceSm_IsConnectingBredrState(state))
        handsetServiceSm_ExitConnectingBredr(sm);

    /* Set new state */
    sm->state = state;

    /* Check for a transition to the CONNECTING pseudo-state */
    if (!handsetServiceSm_IsConnectingBredrState(old_state) && handsetServiceSm_IsConnectingBredrState(state))
        handsetServiceSm_EnterConnectingBredr(sm);

    /* Handle state entry functions */
    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_DISCONNECTED:
        if (old_state != HANDSET_SERVICE_STATE_NULL)
        {
            handsetServiceSm_EnterDisconnected(sm);
        }
        break;
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
        handsetServiceSm_EnterConnectingBredrAcl(sm);
        break;
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES:
        handsetServiceSm_EnterConnectingBredrProfiles(sm);
        break;
    case HANDSET_SERVICE_STATE_CONNECTED_BREDR:
        handsetServiceSm_EnterConnectedBredr(sm);
        break;
    case HANDSET_SERVICE_STATE_DISCONNECTING_BREDR:
        handsetServiceSm_EnterDisconnectingBredr(sm);
        break;
    case HANDSET_SERVICE_STATE_CONNECTED_LE:
        handsetServiceSm_EnterConnectedLe(sm);
        break;
    case HANDSET_SERVICE_STATE_DISCONNECTING_LE:
        handsetServiceSm_EnterDisconnectingLe(sm);
        break;
    default:
        break;
    }
}

/*
    Message handler functions
*/

static void handsetServiceSm_HandleInternalConnectReq(handset_service_state_machine_t *sm,
    const HANDSET_SERVICE_INTERNAL_CONNECT_REQ_T *req)
{
    HS_LOG("handsetServiceSm_HandleInternalConnectReq state 0x%x device 0x%x profiles 0x%x",
           sm->state, req->device, req->profiles);

    /* Confirm requested addr is actually for this instance. */
    assert(sm->handset_device == req->device);

    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_DISCONNECTED:
    case HANDSET_SERVICE_STATE_DISCONNECTING_BREDR: /* Allow a new connect req to cancel an in-progress disconnect. */
    case HANDSET_SERVICE_STATE_CONNECTED_LE:
    case HANDSET_SERVICE_STATE_DISCONNECTING_LE:
        {
            bdaddr handset_addr = sm->handset_addr;

            HS_LOG("handsetServiceSm_HandleInternalConnectReq bdaddr %04x,%02x,%06lx",
                    handset_addr.nap, handset_addr.uap, handset_addr.lap);

            /* Store profiles to be connected */
            sm->profiles_requested = req->profiles;

            if (ConManagerIsConnected(&handset_addr))
            {
                HS_LOG("handsetServiceSm_HandleInternalConnectReq, ACL connected");

                if (sm->profiles_requested)
                {
                    HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES);
                }
                else
                {
                    HS_LOG("handsetServiceSm_HandleInternalConnectReq, no profiles to connect");
                    HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_CONNECTED_BREDR);
                }
            }
            else
            {
                HS_LOG("handsetServiceSm_HandleInternalConnectReq, ACL not connected, attempt to open ACL");
                HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL);
            }
        }
        break;

    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
        /* Already connecting ACL link - nothing more to do but wait for that to finish. */
        break;

    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES:
        /* Profiles already being connected.
           TBD: Too late to merge new profile mask with in-progress one so what to do? */
        break;

    case HANDSET_SERVICE_STATE_CONNECTED_BREDR:
        /* Check requested profiles are all connected;
           if not go back to connecting the missing ones */
        {
            uint8 connected_profiles = BtDevice_GetConnectedProfiles(sm->handset_device);
            if((connected_profiles & req->profiles) != req->profiles)
            {
                sm->profiles_requested |= req->profiles;
                HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES);
            }
            else
            {
                /* Already connected, so complete the request immediately. */
                HandsetServiceSm_CompleteConnectRequests(sm, handset_service_status_success);
            }
        }
        break;

    default:
        HS_LOG("handsetServiceSm_HandleInternalConnectReq, unhandled");
        break;
    }
}

static void handsetServiceSm_HandleInternalDisconnectReq(handset_service_state_machine_t *sm,
    const HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ_T *req)
{
    HS_LOG("handsetServiceSm_HandleInternalDisconnectReq state 0x%x addr [%04x,%02x,%06lx]", 
            sm->state, req->addr.nap, req->addr.uap, req->addr.lap);

    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_DISCONNECTED:
        {
            /* Already disconnected, so complete the request immediately. */
            HandsetServiceSm_CompleteDisconnectRequests(sm, handset_service_status_success);
        }
        break;

    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
        /* Cancelled before profile connect was requested; go to disconnected */
        handsetServiceSm_SetBdedrDisconnectedState(sm);
        break;

    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES:
        /* Cancelled in-progress connect; go to disconnecting to wait for CFM */
        HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_DISCONNECTING_BREDR);
        break;

    case HANDSET_SERVICE_STATE_CONNECTED_BREDR:
        {
            if (BtDevice_GetConnectedProfiles(sm->handset_device))
            {
                HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_DISCONNECTING_BREDR);
            }
            else
            {
                handsetServiceSm_SetBdedrDisconnectedState(sm);
            }
        }
        break;

    case HANDSET_SERVICE_STATE_DISCONNECTING_BREDR:
        /* Already in the process of disconnecting so nothing more to do. */
        break;
        
    case HANDSET_SERVICE_STATE_CONNECTED_LE:
        HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_DISCONNECTING_LE);
        break;
        
    case HANDSET_SERVICE_STATE_DISCONNECTING_LE:
        /* Already in the process of disconnecting so nothing more to do. */
        break;

    default:
        HS_LOG("handsetServiceSm_HandleInternalConnectReq, unhandled");
        break;
    }
}

static void handsetServiceSm_HandleInternalConnectAclComplete(handset_service_state_machine_t *sm)
{
    HS_LOG("handsetServiceSm_HandleInternalConnectAclComplete state 0x%x", sm->state);

    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
        {
            if (BtDevice_DeviceIsValid(sm->handset_device))
            {
                if (handsetService_CheckHandsetCanConnect(&sm->handset_addr))
                {
                    if (ConManagerIsConnected(&sm->handset_addr))
                    {
                        HS_LOG("handsetServiceSm_HandleInternalConnectAclComplete, ACL connected");

                        TimestampEvent(TIMESTAMP_EVENT_HANDSET_CONNECTED_ACL);

                        if (sm->profiles_requested)
                        {
                            HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES);
                        }
                        else
                        {
                            HS_LOG("handsetServiceSm_HandleInternalConnectAclComplete, no profiles to connect");
                            HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_CONNECTED_BREDR);
                        }
                    }
                    else
                    {
                        if (sm->acl_attempts < handsetService_BredrAclConnectAttemptLimit())
                        {
                            HS_LOG("handsetServiceSm_HandleInternalConnectAclComplete, ACL not connected, retrying");

                            /* Send a delayed message to re-try the ACL connection */
                            MessageSendLater(&sm->task_data,
                                     HANDSET_SERVICE_INTERNAL_CONNECT_ACL_RETRY_REQ,
                                     NULL, handsetService_BredrAclConnectRetryDelayMs());
                        }
                        else
                        {
                            HS_LOG("handsetServiceSm_HandleInternalConnectAclComplete, ACL failed to connect");
                        	handsetServiceSm_SetBdedrDisconnectedState(sm);
                        }
                    }
                }
                else
                {
                    /* Not allowed to connect this handset so disconnect it now
                       before the profiles are connected. */
                    HS_LOG("handsetServiceSm_HandleInternalConnectAclComplete, new handset connection not allowed");
                    handsetServiceSm_SetBdedrDisconnectedState(sm);
                }
            }
            else
            {
                /* Handset device is no longer valid - usually this is because
                   it was deleted from the device database before it was
                   disconnected. Reject this ACL connection */
                handsetServiceSm_SetBdedrDisconnectedState(sm);
            }
        }
        break;

    default:
        HS_LOG("handsetServiceSm_HandleInternalConnectAclComplete, unhandled");
        break;
    }
}

/*! \brief Handle a HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ */
static void handsetService_HandleInternalConnectStop(handset_service_state_machine_t *sm,
    const HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ_T *req)
{
    HS_LOG("handsetService_HandleInternalConnectStop state 0x%x", sm->state);

    UNUSED(req);

    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
        /* ACL has not connected yet so go to disconnected to stop it */
        HS_LOG("handsetService_HandleInternalConnectStop, Cancel ACL connecting");
        handsetServiceSm_SetBdedrDisconnectedState(sm);
        break;
    
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES:
        /* Wait for profiles connect to complete */
        break;

    case HANDSET_SERVICE_STATE_CONNECTED_BREDR:
        /* Already in a stable state, so send a CFM back immediately. */
        HandsetServiceSm_CompleteConnectStopRequests(sm, handset_service_status_connected);
        break;

    default:
        break;
    }
}

/*! \brief Handle a HANDSET_SERVICE_INTERNAL_CONNECT_ACL_RETRY_REQ */
static void handsetService_HandleInternalConnectAclRetryReq(handset_service_state_machine_t *sm)
{
    HS_LOG("handsetService_HandleInternalConnectAclRetryReq state 0x%x", sm->state);

    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
        {
            /* Retry the ACL connection */
            handsetService_ConnectAcl(sm);
        }
        break;

    default:
        break;
    }
}

/*! \brief Handle a CONNECT_PROFILES_CFM */
static void handsetServiceSm_HandleProfileManagerConnectCfm(handset_service_state_machine_t *sm,
    const CONNECT_PROFILES_CFM_T *cfm)
{
    HS_LOG("handsetServiceSm_HandleProfileManagerConnectCfm state 0x%x result %d", sm->state, cfm->result);

    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES:
        /* Timestamp at this point so that failures could be timed */
        TimestampEvent(TIMESTAMP_EVENT_HANDSET_CONNECTED_PROFILES);

        if (cfm->result == profile_manager_success)
        {
            /* Assume all requested profiles were connected. */
            HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_CONNECTED_BREDR);
        }
        else
        {
            if (handsetServiceSm_AllConnectionsDisconnected(sm, TRUE))
            {
                handsetServiceSm_SetBdedrDisconnectedState(sm);
            }
            else
            {
                HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_DISCONNECTING_BREDR);
            }
        }
        break;

    case HANDSET_SERVICE_STATE_CONNECTED_BREDR:
        /* Nothing more to do as we are already connected.
           Note: Should only get a CONNECT_PROFILES_CFM in this state if a
                 client has requested to connect more profiles while already
                 connected. */
        break;

    case HANDSET_SERVICE_STATE_DISCONNECTING_BREDR:
        /* Connect has been cancelled already but this CFM may have been
           in-flight already. */
        if (handsetServiceSm_AllConnectionsDisconnected(sm, TRUE))
        {
            handsetServiceSm_SetBdedrDisconnectedState(sm);
        }
        break;

    default:
        HS_LOG("handsetServiceSm_HandleProfileManagerConnectCfm, unhandled");
        break;
    }
}

/*! \brief Handle a DISCONNECT_PROFILES_CFM */
static void handsetServiceSm_HandleProfileManagerDisconnectCfm(handset_service_state_machine_t *sm,
    const DISCONNECT_PROFILES_CFM_T *cfm)
{
    HS_LOG("handsetServiceSm_HandleProfileManagerDisconnectCfm state 0x%x result %d", sm->state, cfm->result);

    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_DISCONNECTING_BREDR:
        if (cfm->result == profile_manager_success)
        {
            if (handsetServiceSm_AllConnectionsDisconnected(sm, TRUE))
            {
            	handsetServiceSm_SetBdedrDisconnectingCompleteState(sm);
            }
            else
            {
                HS_LOG("handsetServiceSm_HandleProfileManagerDisconnectCfm something connected");
            }
        }
        else
        {
            HS_LOG("handsetServiceSm_HandleProfileManagerDisconnectCfm, failed to disconnect");
        }
        break;

    default:
        HS_LOG("handsetServiceSm_HandleProfileManagerDisconnectCfm, unhandled");
        break;
    }
}

/*! \brief Handle a CONNECTED_PROFILE_IND */
void HandsetServiceSm_HandleProfileManagerConnectedInd(handset_service_state_machine_t *sm,
    const CONNECTED_PROFILE_IND_T *ind)
{
    HS_LOG("HandsetServiceSm_HandleProfileManagerConnectedInd state 0x%x profile 0x%x",
           sm->state, ind->profile);

    assert(sm->handset_device == ind->device);

    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_DISCONNECTED:
    case HANDSET_SERVICE_STATE_CONNECTED_LE:
        HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_CONNECTED_BREDR);
        break;

    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES:
        /* A profile that was requested has connected. Nothing else to do
           at this moment as we are waiting for the CFM when all requested
           profiles have connected. */
        break;

    case HANDSET_SERVICE_STATE_CONNECTED_BREDR:
        {
            uint8 connected_profiles = BtDevice_GetConnectedProfiles(sm->handset_device);

            /* Stay in the same state but send an IND with all the profile(s) currently connected. */
            HandsetService_SendConnectedIndNotification(sm->handset_device, connected_profiles);

            /* Update the "last connected" profile list */
            BtDevice_SetLastConnectedProfilesForDevice(sm->handset_device, connected_profiles, TRUE);
        }
        break;

    case HANDSET_SERVICE_STATE_DISCONNECTING_BREDR:
        /* Although we are disconnecting, if a profile re-connects the handset
           state should reflect the actual situation, so go back to the
           CONNECTED state. */

        DEBUG_LOG("HandsetServiceSm_HandleProfileManagerConnectedInd something connected %d",
                  !handsetServiceSm_AllConnectionsDisconnected(sm, TRUE));

        HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_CONNECTED_BREDR);
        break;

    default:
        HS_LOG("HandsetServiceSm_HandleProfileManagerConnectedInd, unhandled");
        break;
    }
}

/*! \brief Handle a DISCONNECTED_PROFILE_IND */
void HandsetServiceSm_HandleProfileManagerDisconnectedInd(handset_service_state_machine_t *sm,
    const DISCONNECTED_PROFILE_IND_T *ind)
{
    HS_LOG("HandsetServiceSm_HandleProfileManagerDisconnectedInd state 0x%x profile 0x%x reason %d",
           sm->state, ind->profile, ind->reason);

    assert(sm->handset_device == ind->device);

    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_DISCONNECTED:
        break;

    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES:
        /* If a profile disconnects for any reason the handset may be fully
           disconnected so we need to check that and go to a disconnected
           state if necessary. */
        /*  Intentional fall-through */
    case HANDSET_SERVICE_STATE_CONNECTED_BREDR:
        {
            /* Note: don't remove the profile from the 'last connected'
               profiles because we don't have enough information to know if the
               handset disconnected the profile on its own, or as part of
               a full disconnect. */

            /* Only go to disconnected state if there are no other handset connections. */
            if (handsetServiceSm_AllConnectionsDisconnected(sm, TRUE))
            {
                handsetServiceSm_SetBdedrDisconnectedState(sm);
            }
        }
        break;

    case HANDSET_SERVICE_STATE_DISCONNECTING_BREDR:
        /* A disconnect request to the profile manager is in progress, so wait
           for the DISCONNECT_PROFILES_CFM and the ACL to be disconnected. */
        DEBUG_LOG("HandsetServiceSm_HandleProfileManagerDisconnectedInd something connected %d",
                  handsetServiceSm_AllConnectionsDisconnected(sm, TRUE));
        break;

    default:
        HS_LOG("HandsetServiceSm_HandleProfileManagerDisconnectedInd, unhandled");
        break;
    }
}

void HandsetServiceSm_HandleConManagerBleTpConnectInd(handset_service_state_machine_t *sm,
    const CON_MANAGER_TP_CONNECT_IND_T *ind)
{
    HS_LOG("HandsetServiceSm_HandleConManagerBleTpConnectInd");
    
    if (BdaddrTpIsEmpty(&sm->le_addr))
    {
        sm->le_addr = ind->tpaddr;
    }
    
    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_DISCONNECTED:
        HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_CONNECTED_LE);
        break;
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES:
    case HANDSET_SERVICE_STATE_CONNECTED_BREDR:
    case HANDSET_SERVICE_STATE_DISCONNECTING_BREDR:
        break;
    case HANDSET_SERVICE_STATE_CONNECTED_LE:
    case HANDSET_SERVICE_STATE_DISCONNECTING_LE:
        /* Shouldn't ever happen */
        Panic();
        break;
    default:
        HS_LOG("HandsetServiceSm_HandleConManagerBleTpConnectInd unhandled");
        break;
    }
}

void HandsetServiceSm_HandleConManagerBleTpDisconnectInd(handset_service_state_machine_t *sm,
    const CON_MANAGER_TP_DISCONNECT_IND_T *ind)
{
    UNUSED(ind);

    HS_LOG("HandsetServiceSm_HandleConManagerBleTpDisconnectInd");

    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES:
    case HANDSET_SERVICE_STATE_CONNECTED_BREDR:
    case HANDSET_SERVICE_STATE_DISCONNECTING_BREDR:
        break;
    case HANDSET_SERVICE_STATE_CONNECTED_LE:
    case HANDSET_SERVICE_STATE_DISCONNECTING_LE:
        HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_DISCONNECTED);
        break;
    default:
        HS_LOG("HandsetServiceSm_HandleConManagerBleTpDisconnectInd unhandled");
        break;
    }
}

/*! \brief Handle a handset initiated ACL connection.

    This represents an ACL connection that was initiated by the handset.

    Usually this will happen in a disconnected state, before any profiles have
    connected. In this case go directly to the BR/EDR connected state.

*/
void HandsetServiceSm_HandleConManagerBredrTpConnectInd(handset_service_state_machine_t *sm,
    const CON_MANAGER_TP_CONNECT_IND_T *ind)
{
    HS_LOG("HandsetServiceSm_HandleConManagerBredrTpConnectInd state %u device 0x%x",
            sm->state, sm->handset_device);

    assert(BdaddrIsSame(&sm->handset_addr, &ind->tpaddr.taddr.addr));

    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_DISCONNECTED:
    case HANDSET_SERVICE_STATE_CONNECTED_LE:
        HandsetServiceSm_SetState(sm, HANDSET_SERVICE_STATE_CONNECTED_BREDR);
        break;

    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
        /* Although we are waiting for the ACL to connect, we use
           HANDSET_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE to detect when the
           ACL is connected. */
        break;

    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES:
    case HANDSET_SERVICE_STATE_CONNECTED_BREDR:
        /* Unexpected but harmless? */
        break;

    case HANDSET_SERVICE_STATE_DISCONNECTING_BREDR:
        /* It would be unusual to get an ACL re-connecting if the state machine
           was in the process of disconnecting.
           Not sure of the best way to handle this? */

        DEBUG_LOG("HandsetServiceSm_HandleConManagerBredrTpConnectInd something connected %d",
                  !handsetServiceSm_AllConnectionsDisconnected(sm, TRUE));
        break;

    default:
        break;
    }
}

/*! \brief Handle a BR/EDR CON_MANAGER_TP_DISCONNECT_IND_T

    This represents the handset ACL has disconnected. Check if any other
    handset connections are active and if not, go into a disconnected state.
*/
void HandsetServiceSm_HandleConManagerBredrTpDisconnectInd(handset_service_state_machine_t *sm,
    const CON_MANAGER_TP_DISCONNECT_IND_T *ind)
{
    HS_LOG("HandsetServiceSm_HandleConManagerBredrTpDisconnectInd state %u hci_reason %u",
            sm->state, ind->reason);

    if(sm->handset_device == 0)
    {
        return;
    }

    assert(BdaddrIsSame(&sm->handset_addr, &ind->tpaddr.taddr.addr));

    /* store the reason for handset disconnection */
    sm->disconnect_reason = ind->reason;

    /* Proceed only if all the profiles are disconnected */
    if(!handsetServiceSm_AllConnectionsDisconnected(sm, TRUE))
        return;

    switch (sm->state)
    {
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL:
    case HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES:
    case HANDSET_SERVICE_STATE_CONNECTED_BREDR:
        handsetServiceSm_SetBdedrDisconnectedState(sm);
        break;
    case HANDSET_SERVICE_STATE_DISCONNECTING_BREDR:
        handsetServiceSm_SetBdedrDisconnectingCompleteState(sm);
        break;

    default:
        HS_LOG("HandsetServiceSm_HandleConManagerBredrTpDisconnectInd unhandled");
        break;
    }
}

static void handsetServiceSm_MessageHandler(Task task, MessageId id, Message message)
{
    handset_service_state_machine_t *sm = handsetServiceSm_GetSmFromTask(task);

    HS_LOG("handsetServiceSm_MessageHandler id 0x%x", id);

    switch (id)
    {
    /* connection_manager messages */

    /* profile_manager messages */
    case CONNECT_PROFILES_CFM:
        handsetServiceSm_HandleProfileManagerConnectCfm(sm, (const CONNECT_PROFILES_CFM_T *)message);
        break;

    case DISCONNECT_PROFILES_CFM:
        handsetServiceSm_HandleProfileManagerDisconnectCfm(sm, (const DISCONNECT_PROFILES_CFM_T *)message);
        break;

    /* Internal messages */
    case HANDSET_SERVICE_INTERNAL_CONNECT_REQ:
        handsetServiceSm_HandleInternalConnectReq(sm, (const HANDSET_SERVICE_INTERNAL_CONNECT_REQ_T *)message);
        break;

    case HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ:
        handsetServiceSm_HandleInternalDisconnectReq(sm, (const HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ_T *)message);
        break;

    case HANDSET_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE:
        handsetServiceSm_HandleInternalConnectAclComplete(sm);
        break;

    case HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ:
        handsetService_HandleInternalConnectStop(sm, (const HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ_T *)message);
        break;

    case HANDSET_SERVICE_INTERNAL_CONNECT_ACL_RETRY_REQ:
        handsetService_HandleInternalConnectAclRetryReq(sm);
        break;

    default:
        HS_LOG("handsetService_MessageHandler unhandled msg id 0x%x", id);
        break;
    }
}

void HandsetServiceSm_Init(handset_service_state_machine_t *sm)
{
    assert(sm != NULL);

    memset(sm, 0, sizeof(*sm));
    sm->state = HANDSET_SERVICE_STATE_NULL;
    sm->task_data.handler = handsetServiceSm_MessageHandler;

    TaskList_Initialise(&sm->connect_list);
    TaskList_Initialise(&sm->disconnect_list);
}

void HandsetServiceSm_DeInit(handset_service_state_machine_t *sm)
{
    TaskList_RemoveAllTasks(&sm->connect_list);
    TaskList_RemoveAllTasks(&sm->disconnect_list);

    MessageFlushTask(&sm->task_data);
    HandsetServiceSm_SetDevice(sm, (device_t)0);
    BdaddrTpSetEmpty(&sm->le_addr);
    sm->profiles_requested = 0;
    sm->acl_create_called = FALSE;
    sm->state = HANDSET_SERVICE_STATE_NULL;
}

void HandsetServiceSm_CancelInternalConnectRequests(handset_service_state_machine_t *sm)
{
    MessageCancelAll(&sm->task_data, HANDSET_SERVICE_INTERNAL_CONNECT_REQ);
    MessageCancelAll(&sm->task_data, HANDSET_SERVICE_INTERNAL_CONNECT_ACL_RETRY_REQ);
}

void HandsetServiceSm_CompleteConnectRequests(handset_service_state_machine_t *sm, handset_service_status_t status)
{
    if (TaskList_Size(&sm->connect_list))
    {
        MESSAGE_MAKE(cfm, HANDSET_SERVICE_CONNECT_CFM_T);
        cfm->addr = sm->handset_addr;
        cfm->status = status;

        /* Send HANDSET_SERVICE_CONNECT_CFM to all clients who made a
           connect request, then remove them from the list. */
        TaskList_MessageSend(&sm->connect_list, HANDSET_SERVICE_CONNECT_CFM, cfm);
        TaskList_RemoveAllTasks(&sm->connect_list);
    }

    /* Flush any queued internal connect requests */
    HandsetServiceSm_CancelInternalConnectRequests(sm);
}

void HandsetServiceSm_CompleteDisconnectRequests(handset_service_state_machine_t *sm, handset_service_status_t status)
{
    if (TaskList_Size(&sm->disconnect_list))
    {
        MESSAGE_MAKE(cfm, HANDSET_SERVICE_DISCONNECT_CFM_T);
        handsetServiceSm_GetBdAddr(sm, &cfm->addr);
        cfm->status = status;

        /* Send HANDSET_SERVICE_DISCONNECT_CFM to all clients who made a
           disconnect request, then remove them from the list. */
        TaskList_MessageSend(&sm->disconnect_list, HANDSET_SERVICE_DISCONNECT_CFM, cfm);
        TaskList_RemoveAllTasks(&sm->disconnect_list);
    }

    /* Flush any queued internal disconnect requests */
    MessageCancelAll(&sm->task_data, HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ);
}

void HandsetServiceSm_CompleteConnectStopRequests(handset_service_state_machine_t *sm, handset_service_status_t status)
{
    if (sm->connect_stop_task)
    {
        MESSAGE_MAKE(cfm, HANDSET_SERVICE_CONNECT_STOP_CFM_T);
        cfm->addr = sm->handset_addr;
        cfm->status = status;

        MessageSend(sm->connect_stop_task, HANDSET_SERVICE_CONNECT_STOP_CFM, cfm);
        sm->connect_stop_task = (Task)0;
    }
}

bool HandsetServiceSm_IsLeConnected(handset_service_state_machine_t *sm)
{
    bool le_connected = FALSE;
    
    if (!BdaddrTpIsEmpty(&sm->le_addr) && ConManagerIsTpConnected(&sm->le_addr))
    {
        le_connected = TRUE;
    }
    
    return le_connected;
}

bool HandsetServiceSm_IsBredrAclConnected(handset_service_state_machine_t *sm)
{
    bool bredr_connected = FALSE;

    PanicNull(sm);

    if (   sm->state != HANDSET_SERVICE_STATE_NULL
        && !handsetServiceSm_AllConnectionsDisconnected(sm, TRUE))
    {
        bredr_connected = TRUE;
    }

    return bredr_connected;
}
