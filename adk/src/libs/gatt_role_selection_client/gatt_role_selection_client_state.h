/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */
/*!
\file
    Functions to manage the state of the client code.
*/

#ifndef GATT_ROLE_SELECTION_CLIENT_STATE_H_
#define GATT_ROLE_SELECTION_CLIENT_STATE_H_


#include "gatt_role_selection_client_private.h"

/*! Enumerated type for the internal state of the role selection client.

    The state controls what operations are allowed & what messages
    are expected. 


\startuml

    note "For clarity not all state transitions shown" as N1

    [*] -down-> UNINITIALISED : Start

    UNINITIALISED : App module and library init
    UNINITIALISED --> FINDING_HANDLES : GattRoleSelectionClientInit()
    UNINITIALISED --> ERROR : Failed to register with Gatt Manager

    FINDING_HANDLES : Discovering characteristics of the remote server so that handles are known for commands
    FINDING_HANDLES --> INITIALISED : Characteristic discovery completed 

    INITIALISED : Handles known, awaiting command
    INITIALISED --> WAITING_READ : 
    INITIALISED --> FINDING_NOTIFICATION_HANDLE : 
    INITIALISED --> SETTING_NOTIFICATION : 

    FINDING_NOTIFICATION_HANDLE : Asked to enable notifications, but no handle yet
    FINDING_NOTIFICATION_HANDLE --> SETTING_NOTIFICATION : Setting notification state on server

    SETTING_NOTIFICATION : Waiting for acknowledgment that notificatiin setting updated at the server
    SETTING_NOTIFICATION --> INITIALISED : Characteristic configuration write successful

    WAITING_READ : Awaiting response for a characteristic read
    WAITING_READ --> INITIALISED : response received

    WAITING_WRITE : Awaiting response for a characteristic write
    WAITING_WRITE --> INITIALISED : response received

\enduml
*/

typedef enum role_selection_client_state_t
{
    role_selection_client_uninitialised,
    role_selection_client_finding_handles,
    role_selection_client_initialised,
    role_selection_client_waiting_read,
    role_selection_client_waiting_read_fom,
    role_selection_client_waiting_write,
    role_selection_client_finding_notification_handle,
    role_selection_client_setting_notification,
    role_selection_client_finding_notification_handle_fom,
    role_selection_client_setting_notification_fom,
    role_selection_client_error,
} role_selection_client_state_t;


void gattRoleSelectionClientSetState(GATT_ROLE_SELECTION_CLIENT *instance, role_selection_client_state_t state);


#define gattRoleSelectionClientGetState(inst) ((role_selection_client_state_t)((inst)->state))


#endif /* GATT_ROLE_SELECTION_CLIENT_STATE_H_ */

