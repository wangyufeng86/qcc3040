/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */
/*!
\file
    Functions to manage the state of the client code.
*/

#ifndef GATT_ROOT_KEY_CLIENT_STATE_H_
#define GATT_ROOT_KEY_CLIENT_STATE_H_


#include "gatt_root_key_client_private.h"

/*! Enumerated type for the internal state of the root key client.

    The state controls what operations are allowed & what messages
    are expected. 

    Unexpected messages cause a jump to the error state, as this
    service is secure.

    Unexpected commands may just be ignored.

\startuml

    note "For clarity not all state transitions shown" as N1

    [*] -down-> UNINITIALISED : Start

    UNINITIALISED : App module and library init
    UNINITIALISED --> FINDING_HANDLES : GattRootKeyClientInit()
    UNINITIALISED --> ERROR : Failed to register with Gatt Manager

    DISABLED : Client service is disabled. This can occur once devices are paired and the client is not needed.

    FINDING_HANDLES : Discovering characteristics of the remote server so that handles are known for commands
    FINDING_HANDLES --> FINDING_INDICATION_HANDLE : Characteristic discovery completed 

    FINDING_INDICATION_HANDLE : Having found the handle for challenge control, finding the descriptor so that indications can be enabled
    FINDING_INDICATION_HANDLE --> ENABLING_INDICATIONS : Descriptor search complete, auto-enable indications

    ENABLING_INDICATIONS : Waiting for acknowledgment that indications have been enabled at the server
    ENABLING_INDICATIONS --> INITIALISED : Characteristic configuration write successful

    INITIALISED : Handles known, awaiting command
    INITIALISED --> STARTING_CHALLENGE : GattRootKeyClientChallengePeer()

    STARTING_CHALLENGE : First stage of challenge sent to the server
    STARTING_CHALLENGE --> FINISHING_CHALLENGE : Received random from server, written hash
    STARTING_CHALLENGE --> ERROR : Incorrect message / opcode received

    FINISHING_CHALLENGE : Final stage of challenge sent to the server
    FINISHING_CHALLENGE --> AUTHENTICATED :Hash received from server and verified
    FINISHING_CHALLENGE --> ERROR : Incorrect HASH received

    AUTHENTICATED : Challenge has been completed successfully
    AUTHENTICATED --> WRITING_IR : GattRootKeyClientWriteKey()

    WRITING_IR : IR key has been sent to server
    WRITING_IR --> WRITING_ER : IR write successful

    WRITING_ER : ER key has been sent to server
    WRITING_ER --> COMMITTING : Write of ER confirmed, requested write of keys

    COMMITTING : Keys have been sent, asking server to apply changes
    COMMITTING --> EXCHANGED: Acknowledgment that keys applied

    EXCHANGED : Keys have been confirmed

    ERROR : Interaction with the server has failed
    ERROR --> DISABLED: Only allowed transition from error

\enduml
*/

typedef enum root_key_client_state_t
{
    root_key_client_uninitialised,
    root_key_client_disabled,
    root_key_client_finding_handles,
    root_key_client_finding_indication_handle,
    root_key_client_enabling_indications,
    root_key_client_initialised,
    root_key_client_starting_challenge,
    root_key_client_finishing_challenge,
    root_key_client_authenticated,
    root_key_client_writing_ir,
    root_key_client_writing_er,
    root_key_client_committing,
    root_key_client_exchanged,
    root_key_client_error,
} root_key_client_state_t;


void gattRootKeyClientSetState(GATT_ROOT_KEY_CLIENT *instance, root_key_client_state_t state);


#define gattRootKeyClientGetState(inst) ((root_key_client_state_t)((inst)->state))


#endif /* GATT_ROOT_KEY_CLIENT_STATE_H_ */

