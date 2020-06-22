/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Header file for state machine transitions in the PEER PAIRING
            OVER LE service
*/

#ifndef PEER_PAIR_LE_SM_H_
#define PEER_PAIR_LE_SM_H_

#include "peer_pair_le.h"

/*! \addtogroup le_peer_pairing_service

    <H2>State transitions</H2>

    The LE peer pairing service uses a state variable to record its current 
    status see #PEER_PAIR_LE_STATE for state names. The use of the state 
    allows the processing of messages and function calls to check what 
    action is required, or error to generate.

    The state is set using the function peer_pair_le_set_state(). This 
    function can trigger actions based on the state that has been entered 
    (or exited).

    The basics of the state are documented in the diagram below.

    \startuml

    note "For clarity not all state transitions shown" as N1

    [*] -down-> UNINITIALISED : Start
    note left of UNINITIALISED : State names shortened, removing prefix of PEER_PAIR_LE_STATE_

    UNINITIALISED : Initial state on power up
    UNINITIALISED --> INITIALISED : peer_pair_le_init()
    
    INITIALISED : State when first initialised, and after pairing has completed
    INITIALISED --> PENDING_LOCAL_ADDR : peer_pair_le_start_service()

    PENDING_LOCAL_ADDR : On entry to state, the local Bluetooth Device Address is requested from the Coneection Library.
    PENDING_LOCAL_ADDR : This is needed before any subsequent activity.
    PENDING_LOCAL_ADDR --> IDLE : CL_DM_LOCAL_BD_ADDR_CFM
    PENDING_LOCAL_ADDR --> IDLE : CL_DM_LOCAL_BD_ADDR_CFM, PeerPairLe_FindPeer() called first
    PENDING_LOCAL_ADDR --> PENDING_LOCAL_ADDR : PeerPairLe_FindPeer()

    IDLE: Awaiting a request to pair with a peer
    IDLE --> DISCOVERY : PeerPairLe_FindPeer()

    DISCOVERY : Advertising and scanning. No devices yet detected.
    DISCOVERY --> SELECTING : CL_DM_BLE_ADVERTISING_REPORT_IND. 
    DISCOVERY --> PAIRING_AS_SERVER : GATT Connect observer notification

    SELECTING : Advertising and scanning. 
    SELECTING : At least one advert from a matching device has been seen.
    SELECTING --> CONNECTING : PEER_PAIR_LE_TIMEOUT_FROM_FIRST_SCAN, single device found
    SELECTING --> DISCOVERY : PEER_PAIR_LE_TIMEOUT_FROM_FIRST_SCAN, no suitable device found
    SELECTING --> PAIRING_AS_SERVER : GATT Connect observer notification

    CONNECTING: Creating a connection to discovered device
    CONNECTING --> PAIRING_AS_CLIENT : CON_MANAGER_TP_CONNECT_IND

    PAIRING_AS_SERVER : Bluetooth pairing and encryption
    PAIRING_AS_SERVER --> NEGOTIATE_C_ROLE : Pairing successful

    PAIRING_AS_CLIENT : Bluetooth pairing and encryption
    PAIRING_AS_CLIENT --> NEGOTIATE_P_ROLE : Pairing successful

    NEGOTIATE_P_ROLE: Set up as GATT client for root key
    NEGOTIATE_P_ROLE: Challenge the other device
    NEGOTIATE_P_ROLE: Send root keys
    NEGOTIATE_P_ROLE --> COMPLETED : GATT_ROOT_KEY_CLIENT_WRITE_KEY_IND

    NEGOTIATE_C_ROLE: Wait for other device to send keys using the root key service
    NEGOTIATE_C_ROLE --> COMPLETED_WAIT_FOR_DISCONNECT : GATT_ROOT_KEY_SERVER_KEY_UPDATE_IND

    COMPLETED_WAIT_FOR_DISCONNECT: Wait for other device to disconnect
    COMPLETED_WAIT_FOR_DISCONNECT --> DISCONNECTING : Timeout waiting for other device to disconnect, disconnect link
    COMPLETED_WAIT_FOR_DISCONNECT --> INITIALISED : CON_MANAGER_TP_DISCONNECT_IND, link disconnected

    COMPLETED: Peer paired
    COMPLETED: Disconnect link
    COMPLETED --> DISCONNECTING : Automatic

    DISCONNECTING: Waiting for disconnection to complete
    DISCONNECTING --> INITIALISED : CON_MANAGER_TP_DISCONNECT_IND, link disconnected

    \enduml
*/


/*! States used internally by the LE Peer Pairing Service */
typedef enum
{
        /*! Peer pairing module has not yet been initialised */
    PEER_PAIR_LE_STATE_UNINITIALISED,
        /*! Peer pairing module is initialised but otherwise inactive */
    PEER_PAIR_LE_STATE_INITIALISED,
        /*! Awaiting our local BDADDR in order to start using the module */
    PEER_PAIR_LE_STATE_PENDING_LOCAL_ADDR,
        /*! No action in progress, but module is ready to use. */
    PEER_PAIR_LE_STATE_IDLE,
        /*! Connecting to a nearby device */
    PEER_PAIR_LE_STATE_CONNECTING,
        /*! Looking for a peer - using connectable BLE advertising and BLE scanning. */
    PEER_PAIR_LE_STATE_DISCOVERY,
        /*! Looking for a peer - advertising and scanning.
        One device has been found by scanning.  Continuing scan to identify 
            the closest nearby device (strongest signal) */
    PEER_PAIR_LE_STATE_SELECTING,
        /*! Completing Bluetooth pairing with the located device, pairing as a server.
            \todo Should really be peripheral/central */
    PEER_PAIR_LE_STATE_PAIRING_AS_SERVER,
        /*! Completing Bluetooth pairing with the located device, pairing as a client */
    PEER_PAIR_LE_STATE_PAIRING_AS_CLIENT,
        /*! Making sure that the connected device is a matching earbud.
            If successful this device will become the "peripheral" in future */
    PEER_PAIR_LE_STATE_NEGOTIATE_P_ROLE,
        /*! Making sure that the connected device is a matching earbud.
            If successful this device will become the "central" in future */
    PEER_PAIR_LE_STATE_NEGOTIATE_C_ROLE,
        /*! Have completed paired, waiting for other side to disconnect.
            Local device will initiate disconnect anyway after timeout.*/
    PEER_PAIR_LE_STATE_COMPLETED_WAIT_FOR_DISCONNECT,
        /*! Have completed pairing. */
    PEER_PAIR_LE_STATE_COMPLETED,
        /*! Waiting for the connection to disconnect */
    PEER_PAIR_LE_STATE_DISCONNECTING,
} PEER_PAIR_LE_STATE;


/*! Set the internal state of the LE peer pairing service.

    \param state The new state to set

    \note state transitions can cause actions 
    \note this state mechanism does not expect transitions to the same state
*/
void peer_pair_le_set_state(PEER_PAIR_LE_STATE state);

/*! Get the state of the Peer pairing module. Copes with the Task not having been initialised. */
#define peer_pair_le_get_state() (PeerPairLeGetTaskData()->state)


/*! Check if the module is in a pairing state 

    \return TRUE if in a pairing state
*/
bool peer_pair_le_in_pairing_state(void);


/*! Check if the module is in an advertising state

    \return TRUE if in an advertising state, otherwise FALSE
 */
bool peer_pair_le_is_in_advertising_state(void);


#endif /* PEER_PAIR_LE_SM_H_ */
