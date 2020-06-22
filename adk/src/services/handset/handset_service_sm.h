/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    handset_service
\brief      Handset service state machine
*/

#ifndef HANDSET_SERVICE_SM_H_
#define HANDSET_SERVICE_SM_H_

#include <device.h>
#include <task_list.h>

#include <profile_manager.h>
#include <connection_manager.h>
#include "handset_service.h"

/*@{*/

/* Bitmask to signify that a state is part of the CONNECTING pseudo-state. */
#define HANDSET_SERVICE_CONNECTING_BREDR_STATE_MASK 0x10

/*! \brief Handset Service states.

@startuml Handset Service States

state DISCONNECTED : Handset not connected
state CONNECTING : Pseudo-state for connecting sub-states.
state CONNECTED : Handset profile(s) connected
state DISCONNECTING : Handset profile(s) disconnecting


[*] -d-> DISCONNECTED : Create new handset state machine

DISCONNECTED --> CONNECTING : HandsetConnect REQ
DISCONNECTED --> CONNECTED : Handset connects\nHandsetConnect IND
DISCONNECTED --> DISCONNECTED : HandsetDisconnect REQ\nHandsetDisconnect CFM (success)

state CONNECTING {
    state CONNECTING_ACL : ACL connecting
    state CONNECTING_PROFILES : Handset profile(s) connecting

    CONNECTING_ACL --> CONNECTING_PROFILES : ACL connected
    CONNECTING_ACL --> DISCONNECTED : CONNECT_STOP_REQ
}

CONNECTING --> CONNECTING : HandsetConnect REQ
CONNECTING --> CONNECTED : ACL & Profiles connected\nHandsetConnect CFM (success)
CONNECTING --> DISCONNECTING : HandsetDisconnect REQ\nHandsetConnect CFM (cancelled)
CONNECTING --> DISCONNECTED : ACL or Profiles failed\nHandsetConnect CFM (fail)

CONNECTED --> DISCONNECTED : Handset disconnects\nHandsetDisconnect IND
CONNECTED --> DISCONNECTING : HandsetDisconnect REQ
CONNECTED --> CONNECTED : HandsetConnect REQ\nHandsetConnect CFM (success)

DISCONNECTING --> CONNECTING : HandsetConnect REQ\nHandsetDisconnect CFM (cancelled)
DISCONNECTING --> CONNECTED : Profile connected\nHandsetDisconnect CFM (fail)
DISCONNECTING --> DISCONNECTING : HandsetDisconnect REQ
DISCONNECTING --> DISCONNECTED : Profiles disconnected\nHandsetDisconnect CFM (success)

@enduml
*/
typedef enum
{
    HANDSET_SERVICE_STATE_NULL = 0,
    HANDSET_SERVICE_STATE_DISCONNECTED = 1,
    HANDSET_SERVICE_STATE_CONNECTING_BREDR_ACL = 2 + HANDSET_SERVICE_CONNECTING_BREDR_STATE_MASK,
    HANDSET_SERVICE_STATE_CONNECTING_BREDR_PROFILES = 3 + HANDSET_SERVICE_CONNECTING_BREDR_STATE_MASK,
    HANDSET_SERVICE_STATE_CONNECTED_BREDR = 4,
    HANDSET_SERVICE_STATE_DISCONNECTING_BREDR = 5,
    HANDSET_SERVICE_STATE_CONNECTED_LE = 6,
    HANDSET_SERVICE_STATE_DISCONNECTING_LE = 7
} handset_service_state_t;


/*! \brief Context for an instance of a handset service state machine */
typedef struct
{
    /* Task for this instance. */
    TaskData task_data;

    /*! Current state */
    handset_service_state_t state;

    /*! Device instance this state machine represents */
    device_t handset_device;

    /*! Cached bdaddr from the handset_device (not LE) */
    bdaddr handset_addr;

    /*! Mask of profiles that have been requested */
    uint8 profiles_requested;

    /*! Number of BR/EDR ACL connection attempts made. */
    uint8 acl_attempts;

    /* Was the ACL connection requested during the connection? */
    bool acl_create_called;

    /*! Client list for connect requests */
    task_list_t connect_list;

    /*! Client list for disconnect requests */
    task_list_t disconnect_list;

    /*! Client for connect-stop request. */
    Task connect_stop_task;
    
    /*! Address of LE connected device. */
    tp_bdaddr le_addr;

    /*! Reason for handset disconnection. */
    hci_status disconnect_reason;
} handset_service_state_machine_t;

#define HandsetServiceSm_GetLeTpBdaddr(sm) sm->le_addr

/*! \brief Initialise a state machine instance.

    After this is complete the state machine wil be in the
    HANDSET_SERVICE_STATE_NULL state.

    \param sm State machine to initialise.
*/
void HandsetServiceSm_Init(handset_service_state_machine_t *sm);

/*! \brief De-initialise a state machine instance.

    After this is complete the state machine will be in the
    HANDSET_SERVICE_STATE_NULL state and all of its internal context will have
    been reset.

    Note: This does not free the memory used by the state machine.

    \param sm State machine to de-initialise.
*/
void HandsetServiceSm_DeInit(handset_service_state_machine_t *sm);

/*! \brief Set the device in a state machine entry

    This helper function sets the device and deals with caching
    the Bluetooth Address for the device.

    \param sm The state machine to update
    \param device The device to set, NULL is allowed.
 */
void HandsetServiceSm_SetDevice(handset_service_state_machine_t *sm, device_t device);

/*! \brief Tell a handset_service state machine to go to a new state.

    Changing state always follows the same procedure:
    \li Call the Exit function of the current state (if it exists)
    \li Change the current state
    \li Call the Entry function of the new state (if it exists)

    \param sm The state machine instance.
    \param state New state to go to.
*/
void HandsetServiceSm_SetState(
    handset_service_state_machine_t *sm, handset_service_state_t state);

/*! \brief Complete all queued connect requests

    Complete all connect requests with the given status and cancel
    any HANDSET_SERVICE_INTERNAL_CONNECT_REQ msgs queued for the state machine.

    \param sm State machine to complete the requests for.
    \param status Status code to complete the requests with.
 */
void HandsetServiceSm_CompleteConnectRequests(
    handset_service_state_machine_t *sm, handset_service_status_t status);

/*! \brief Complete all queued disconnect requests

    Complete all disconnect requests with the given status and cancel
    any HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ msgs queued for the state
    machine.

    \param sm State machine to complete the requests for.
    \param status Status code to complete the requests with.
 */
void HandsetServiceSm_CompleteDisconnectRequests(
    handset_service_state_machine_t *sm, handset_service_status_t status);

/*! \brief Complete any queued Connect-Stop request.

    Complete any queued connect stop requests with the given status.

    \param sm State machine to complete the requests for.
    \param status Status code to complete the requests with.
 */
void HandsetServiceSm_CompleteConnectStopRequests(
    handset_service_state_machine_t *sm, handset_service_status_t status);

/*! \brief Handle a CONNECTED_PROFILE_IND.

    Note: This handler is called directly instead of via an internal message
    to save on the extra message send & delivery delay.

    \param sm State machine to handle the message.
    \param ind The CONNECTED_PROFILE_IND payload.
*/
void HandsetServiceSm_HandleProfileManagerConnectedInd(
    handset_service_state_machine_t *sm, const CONNECTED_PROFILE_IND_T *ind);

/*! \brief Handle a DISCONNECTED_PROFILE_IND.

    Note: This handler is called directly instead of via an internal message
    to save on the extra message send & delivery delay.

    \param sm State machine to handle the message.
    \param ind The DISCONNECTED_PROFILE_IND payload.
*/
void HandsetServiceSm_HandleProfileManagerDisconnectedInd(
    handset_service_state_machine_t *sm, const DISCONNECTED_PROFILE_IND_T *ind);

/*! \brief Handle a CON_MANAGER_TP_CONNECT_IND_T

    Note: This handler is called directly instead of via an internal message
    to save on the extra message send & delivery delay.

    \param sm State machine to handle the message.
    \param ind The CON_MANAGER_TP_CONNECT_IND_T payload.
*/
void HandsetServiceSm_HandleConManagerBredrTpConnectInd(handset_service_state_machine_t *sm,
    const CON_MANAGER_TP_CONNECT_IND_T *ind);

/*! \brief Handle a CON_MANAGER_TP_DISCONNECT_IND_T

    Note: This handler is called directly instead of via an internal message
    to save on the extra message send & delivery delay.

    \param sm State machine to handle the message.
    \param ind The CON_MANAGER_TP_DISCONNECT_IND_T payload.
*/
void HandsetServiceSm_HandleConManagerBredrTpDisconnectInd(handset_service_state_machine_t *sm,
    const CON_MANAGER_TP_DISCONNECT_IND_T *ind);

/*! \brief Handle a CON_MANAGER_TP_CONNECT_IND for BLE connections

    Note: This handler is called directly instead of via an internal message
    to save on the extra message send & delivery delay.

    \param sm State machine to handle the message.
    \param ind The CON_MANAGER_TP_CONNECT_IND payload.
*/
void HandsetServiceSm_HandleConManagerBleTpConnectInd(handset_service_state_machine_t *sm,
    const CON_MANAGER_TP_CONNECT_IND_T *ind);
    
/*! \brief Handle a CON_MANAGER_TP_DISCONNECT_IND for BLE connections

    Note: This handler is called directly instead of via an internal message
    to save on the extra message send & delivery delay.

    \param sm State machine to handle the message.
    \param ind The CON_MANAGER_TP_DISCONNECT_IND payload.
*/
void HandsetServiceSm_HandleConManagerBleTpDisconnectInd(handset_service_state_machine_t *sm,
    const CON_MANAGER_TP_DISCONNECT_IND_T *ind);
    
/*! \brief Checks if BLE is connected.

    \param sm State machine to handle the message.
    
    \return TRUE if BLE connected for the state machine. FALSE otherwise.
*/
bool HandsetServiceSm_IsLeConnected(handset_service_state_machine_t *sm);

/*! \brief Check if any BR/EDR ACL or profile is connected

    This function will check if a BR/EDR ACL or profile (e.g. hfp, a2dp, etc.)
    is currently connected to the handset represented by the given sm.

    \note This function does not check for any LE connections.

    \param sm Handset state machine to check.

    \return TRUE if sm is valid and BR/EDR is connected; FALSE otherwise.
*/
bool HandsetServiceSm_IsBredrAclConnected(handset_service_state_machine_t *sm);

/*! \brief Helper to cancel any queued BR/EDR connection requests.

    This will cancel any queued HANDSET_SERVICE_INTERNAL_CONNECT_REQ and
    HANDSET_SERVICE_INTERNAL_CONNECT_ACL_RETRY_REQ messages.

    \note This function only cancels the internal messages. It does not send
          and IND or CFM notifications to clients.

    \param sm Handset state machine to cancel the pending messages on.
*/
void HandsetServiceSm_CancelInternalConnectRequests(handset_service_state_machine_t *sm);

/*@}*/

#endif /* HANDSET_SERVICE_SM_H_ */
