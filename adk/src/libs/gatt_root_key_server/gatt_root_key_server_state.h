/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */
/*!
\file
    Functions to manage the state of the server code.
*/

#ifndef GATT_ROOT_KEY_SERVER_STATE_H_
#define GATT_ROOT_KEY_SERVER_STATE_H_


#include "gatt_root_key_server_private.h"

/*! Enumerated type for the internal state of the root key server.

    The state controls what operations are allowed & what messages
    are expected. 

    Unexpected messages cause a jump to the error state, as this
    service is secure.

    Unexpected commands may just be ignored.

\startuml

    note "For clarity not all state transitions shown" as N1

    [*] -down-> UNINITIALISED : Start

    UNINITIALISED : App module and library init
    UNINITIALISED --> IDLE : GattRootKeyServerInit()
    UNINITIALISED --> ERROR : Failed to register with Gatt Manager

    DISABLED : Server is disabled. This can occur once devices are paired and the client is not needed.

    IDLE : Initialised, but not yet configured for action !
    IDLE --> INITIALISED : GattRootKeyServerReadyForChallenge()

    INITIALISED : Handles known, awaiting command
    INITIALISED --> RESPONDED_RANDOM : Random received from client

    RESPONDED_RANDOM : First stage of challenge sent to the client, awaiting send confirm
    RESPONDED_RANDOM --> AWAITING_HASH : Send of random indication confirmed
    RESPONDED_RANDOM --> ERROR : Incorrect message / opcode received

    AWAITING_HASH : Awaiting hash request
    AWAITING_HASH --> RESPONDED_HASH : Hash received from client
    AWAITING_HASH --> ERROR : Incorrect message / opcode received

    RESPONDED_HASH : Final stage of challenge sent to the client, awaiting send confirm
    RESPONDED_HASH --> AUTHENTICATED :Send of hash indication confirmed
    RESPONDED_HASH --> ERROR : Incorrect HASH received

    AUTHENTICATED : Challenge has been completed successfully
    AUTHENTICATED --> IR_RECEIVED : IR key received

    IR_RECEIVED : IR key has been received from client
    IR_RECEIVED --> ER_RECEIVED : ER key received

    ER_RECEIVED : ER key has been received 
    ER_RECEIVED --> KEYS_EXCHANGED : Command to commit has been receoved

    KEYS_EXCHANGED : Keys have been confirmed

    ERROR : Interaction with the client has failed
    ERROR --> DISABLED: Only allowed transition from error

\enduml
*/

typedef enum root_key_server_state_t
{
    root_key_server_uninitialised,
    root_key_server_disabled,
    root_key_server_idle,
    root_key_server_initialised,
    root_key_server_responded_random,
    root_key_server_awaiting_hash,
    root_key_server_responded_hash,
    root_key_server_authenticated,
    root_key_server_ir_received,
    root_key_server_er_received,
    root_key_server_keys_exchanged,
    root_key_server_error,
} root_key_server_state_t;


void gattRootKeyServerSetState(GATT_ROOT_KEY_SERVER *instance, root_key_server_state_t state);


#define gattRootKeyServerGetState(inst) ((root_key_server_state_t)((inst)->state))


#endif /* GATT_ROOT_KEY_SERVER_STATE_H_ */

