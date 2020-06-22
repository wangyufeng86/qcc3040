/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Header file for state machine transitions in the service finding the peer 
            and selecting role using LE
*/

#ifndef PEER_FIND_ROLE_SM_H_
#define PEER_FIND_ROLE_SM_H_

#include "peer_find_role.h"


/*! \addtogroup peer_find_role

    <H2>State transitions</H2>

    The LE peer find role service uses a state variable to record its current 
    status see #PEER_FIND_ROLE_STATE for state names. The use of the state 
    allows the processing of messages and function calls to check what 
    action is required, or error to generate.

    The state is set using the function peer_find_role_set_state(). This 
    function can trigger actions based on the state that has been entered 
    (or exited).

    The basics of the state are documented in the diagram below.

    \startuml

    note "For clarity not all state transitions shown" as N1

    [*] -down-> UNINITIALISED : Start
    note left of UNINITIALISED : State names shortened, removing prefix of PEER_FIND_ROLE_STATE_

    UNINITIALISED : Initial state on power up
    UNINITIALISED --> INITIALISED : peer_find_role_init()

    INITIALISED: Awaiting a request to find role
    INITIALISED --> CHECKING_PEER : PeerFindRole_FindRole()

    CHECKING_PEER: Verifying that we have previously paired
    CHECKING_PEER --> INITIALISED : Not yet paired
    CHECKING_PEER --> DISCOVER : Paired

    DISCOVER: Looking for a peer device.\nWill \b not enable scanning if streaming/in call.
    DISCOVER: Start a timeout to enable advertising
    DISCOVER --> DISCOVER_CONNECTABLE : Internal timeout to start advertising
    DISCOVER --> DISCOVERED_DEVICE : Received an advert for matching device

    DISCOVER_CONNECTABLE : Looking for peer
    DISCOVER_CONNECTABLE : Also advertising
    DISCOVER_CONNECTABLE --> DISCOVERED_DEVICE : Received an advert for matching device
    DISCOVER_CONNECTABLE --> CLIENT : GATT Connect observer notification.\nRemote device connected to us.
    DISCOVER_CONNECTABLE --> DISCOVER : No longer streaming/in call.\n(re)start scanning.
    DISCOVER_CONNECTABLE --> DISCOVER_CONNECTABLE : streaming/in call.\nstop scanning.

    DISCOVERED_DEVICE: Found a peer device. 
    DISCOVERED_DEVICE: Advertising continues until we get a connection
    DISCOVERED_DEVICE --> CONNECTING_TO_DISCOVERED : Scanning/Advertising ended
    DISCOVERED_DEVICE --> CLIENT : GATT Connect observer notification.\nRemote device connected to us.

    CONNECTING_TO_DISCOVERED: Connecting to the device we found
    CONNECTING_TO_DISCOVERED: Advertising continues. Otherwise if our peer is in the same state there may be nothing to connect to
    CONNECTING_TO_DISCOVERED --> SERVER_AWAITING_ENCRYPTION : CON_MANAGER_TP_CONNECT_IND (outgoing connection)
    CONNECTING_TO_DISCOVERED --> CLIENT : GATT Connect observer notification.\nRemote device connected to us (crossover)
    CONNECTING_TO_DISCOVERED --> DISCOVER : Link disconnected\nConnection manager

    CLIENT: Connected as a GATT client
    CLIENT --> CLIENT_AWAITING_ENCRYPTION : Connected to the peers server
    CLIENT --> DISCOVER : Link disconnected\nConnection manager

    SERVER_AWAITING_ENCRYPTION : Encrypt the link on entry
    SERVER_AWAITING_ENCRYPTION : Wait for encryption to complete
    SERVER_AWAITING_ENCRYPTION --> SERVER_PREPARING : Link encrypted successfully
    SERVER_AWAITING_ENCRYPTION --> DISCOVER : Link disconnected\nConnection manager
    SERVER_AWAITING_ENCRYPTION --> INITIALISED : Error encrypting the link

    SERVER_PREPARING : Request & wait for system to be ready for role selection
    SERVER_PREPARING --> SERVER : Received "prepared" response from client
    SERVER_PREPARING --> SERVER : No client registered to receive prepare indication
    SERVER_PREPARING --> DISCOVER : Link disconnected\nConnection manager

    SERVER : Connected as a GATT server
    SERVER : Calculate score
    SERVER : Wait for client to select role
    SERVER --> SERVER_AWAITING_COMPLETION : Commanded to change state.
    SERVER --> DISCOVER : Link disconnected\nConnection manager

    CLIENT_AWAITING_ENCRYPTION : Connected as a GATT client, link not yet encrypted
    CLIENT_AWAITING_ENCRYPTION --> CLIENT_PREPARING : Link encrypted successfully
    CLIENT_AWAITING_ENCRYPTION --> DISCOVER : Link disconnected\nConnection manager
    
    CLIENT_PREPARING : Request & wait for system to be ready for role selection
    CLIENT_PREPARING --> CLIENT_DECIDING : Received "prepared" response from client
    CLIENT_PREPARING --> CLIENT_DECIDING : No client registered to receive prepare indication
    CLIENT_PREPARING --> DISCOVER : Link disconnected\nConnection manager

    CLIENT_DECIDING : Deciding which role we should assume
    CLIENT_DECIDING : Wait for score from server
    CLIENT_DECIDING --> CLIENT_AWAITING_CONFIRM : Have score, informed peer of requested state
    CLIENT_DECIDING --> DISCOVER : Link disconnected\nConnection manager

    CLIENT_AWAITING_CONFIRM : Awaiting confirmation of role
    CLIENT_AWAITING_CONFIRM --> COMPLETED : Server confirmed change.
    CLIENT_AWAITING_CONFIRM --> DISCOVER : Link disconnected\nConnection manager

    SERVER_AWAITING_COMPLETION : We have informed client of new state (we were server)
    SERVER_AWAITING_COMPLETION : Waiting for external notification that we have completed
    SERVER_AWAITING_COMPLETION --> COMPLETED : Link disconnected
    SERVER_AWAITING_COMPLETION --> SERVER_AWAITING_COMPLETION : Time out expired\nDisconnected ourselves.

    COMPLETED : Transition state when we have finished role selection
    COMPLETED : May wait here for the link to be disconnected
    COMPLETED : Decide whether to enter INITIALISED or DISCOVER state
    COMPLETED --> INITIALISED : Did not complete with a primary role
    COMPLETED --> DISCOVER : Completed with a primary role

    \enduml

*/

/*! Possible states of the find role service */
typedef enum
{
        /*! Peer find role module is not yet initialised */
    PEER_FIND_ROLE_STATE_UNINITIALISED,
        /*! Initialised. No action in progress. */
    PEER_FIND_ROLE_STATE_INITIALISED,
        /*! Waiting to find out if we are paired with a peer */
    PEER_FIND_ROLE_STATE_CHECKING_PEER,
        /*! Scanning to find a compatible device */
    PEER_FIND_ROLE_STATE_DISCOVER,
        /*! Scanning - but also using connectable advertising for 
            other devices to connect */
    PEER_FIND_ROLE_STATE_DISCOVER_CONNECTABLE,
        /*! We have discovered a device while scanning. Waiting a short
            time then will cancel scanning */
    PEER_FIND_ROLE_STATE_DISCOVERED_DEVICE,
        /*! Trying to connect to the device we discovered (still
            advertising) */
    PEER_FIND_ROLE_STATE_CONNECTING_TO_DISCOVERED,
        /*! Waiting for confirmation the link is encrypted. */
    PEER_FIND_ROLE_STATE_SERVER_AWAITING_ENCRYPTION,
        /*! Waiting for the response to a "prepare for role selection" indication */
    PEER_FIND_ROLE_STATE_SERVER_PREPARING,
        /*! Connected as a GATT client (someone connected to us) */
    PEER_FIND_ROLE_STATE_CLIENT,
        /*! Connected as a GATT server */
    PEER_FIND_ROLE_STATE_SERVER,
        /*! Waiting for indication the link is encrypted */
    PEER_FIND_ROLE_STATE_CLIENT_AWAITING_ENCRYPTION,
        /*! Waiting for the response to a "prepare for role selection" indication */
    PEER_FIND_ROLE_STATE_CLIENT_PREPARING,
        /*! Deciding which role to use */
    PEER_FIND_ROLE_STATE_CLIENT_DECIDING,
        /*! Waiting for the role to be confirmed */
    PEER_FIND_ROLE_STATE_CLIENT_AWAITING_CONFIRM,
        /*! Completed role selection, but waiting for acknowledgment that rolw now active */
    PEER_FIND_ROLE_STATE_SERVER_AWAITING_COMPLETION,
        /*! Completed role selection decide next state */
    PEER_FIND_ROLE_STATE_COMPLETED,
} PEER_FIND_ROLE_STATE;


/*! Change the state of the peer find role service

    This function changes the state. It can perform actions as a result of
    \li leaving the previous state
    \li entering the new state

    \param state The new state to change to
*/
void peer_find_role_set_state(PEER_FIND_ROLE_STATE state);


/*! Retrieve the current state of the peer find role service.

    See #PEER_FIND_ROLE_STATE */
#define peer_find_role_get_state() (PeerFindRoleGetTaskData()->state)


/*! Indicate whether peer find role should currently be advertising.

    \return TRUE if in a state that represents advertising, FALSE otherwise
*/
bool peer_find_role_is_in_advertising_state(void);

#endif /* PEER_FIND_ROLE_SM_H_ */
