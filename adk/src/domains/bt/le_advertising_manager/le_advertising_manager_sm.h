/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    le_advertising_manager
\brief      LE advertising manager state machine
*/

#ifndef LE_ADVERTSING_MANAGER_SM_H_
#define LE_ADVERTSING_MANAGER_SM_H_

#include "domain_message.h"

#include <connection.h>
#include <connection_no_ble.h>

/*
@startuml LE Advertising State Machine

state UNINITIALISED : LE advertising manager is disabled and is not yet ready to be configured and start advertising.
state INITIALISED : LE advertising manager is ready to be configured and start advertising.
state STARTING : There is an active advertising which is now being configured and started
state STARTED : There is an active advertising which has been started
state SUSPENDING : There is an active advertising which is now being put on hold
state SUSPENDED : There is an active advertising which has been suspended

[*] -d-> UNINITIALISED : Create new state machine

INITIALISED --> INITIALISED : LeAdvertisingManager_SelectAdvertisingDataSet() (no data exists)
UNINITIALISED --> INITIALISED: LeAdvertisingManager_Init()
INITIALISED --> UNINITIALISED:  LeAdvertisingManager_DeInit()
INITIALISED --> STARTING : LeAdvertisingManager_SelectAdvertisingDataSet() (data exists)
STARTING --> STARTED : CL_DM_BLE_SET_ADVERTISE_ENABLE_CFM
STARTED --> SUSPENDING : LeAdvertisingManager_AllowAdvertising(FALSE) / \n LeAdvertisingManager_EnableConnectableAdvertising(FALSE)
SUSPENDING --> SUSPENDED : CL_DM_BLE_SET_ADVERTISE_ENABLE_CFM
SUSPENDED --> STARTING : LeAdvertisingManager_AllowAdvertising(TRUE) / \n LeAdvertisingManager_EnableConnectableAdvertising(TRUE)

state STARTING {

    state STARTING_DATA_REQ : Data configure is requested
    state STARTING_SCAN_RESPONSE_REQ : Scan response configure is requested
    state STARTING_PARAMETERS_REQ : Parameter configure is requested
    state STARTING_ENABLE_REQ : Enable is requested
    
    STARTING_DATA_REQ --> STARTING_SCAN_RESPONSE_REQ: CL_DM_BLE_SET_ADVERTISING_DATA_CFM
    STARTING_SCAN_RESPONSE_REQ --> STARTING_PARAMETERS_REQ : CL_DM_BLE_SET_SCAN_RESPONSE_DATA_CFM
    STARTING_PARAMETERS_REQ --> STARTING_ENABLE_REQ : CL_DM_BLE_SET_ADVERTISING_PARAMS_CFM
    
}

@enduml
*/

/* Data type for the state of LE Advertising Manager */
typedef enum{
    le_adv_mgr_state_uninitialised,
    le_adv_mgr_state_initialised,
    le_adv_mgr_state_starting,
    le_adv_mgr_state_started,
    le_adv_mgr_state_suspending,
    le_adv_mgr_state_suspended,
}le_adv_mgr_state_t;

/* Data structure for LE advertising manager state machine */
typedef struct{
    le_adv_mgr_state_t state;
}le_adv_mgr_state_machine_t;

/* API Function to initialise the state machine */
le_adv_mgr_state_machine_t * LeAdvertisingManagerSm_Init(void);

/* API Function to deinitialise the state machine */
void LeAdvertisingManagerSm_DeInit(le_adv_mgr_state_machine_t * sm);

/* API Function to change the existing state of the state machine */
void LeAdvertisingManagerSm_SetState(le_adv_mgr_state_t state);

/* API Function to check if there is an active advertising operation already in the process of starting */
bool LeAdvertisingManagerSm_IsAdvertisingStarting(void);

/* API Function to check if there is an active advertising operation already started and in progress */
bool LeAdvertisingManagerSm_IsAdvertisingStarted(void);

/* API Function to check if Initialisation has already been completed with success */
bool LeAdvertisingManagerSm_IsInitialised(void);

/* API Function to check if LE advertising manager is in the process of suspending the active advertising */
bool LeAdvertisingManagerSm_IsSuspending(void);

/* API Function to check if the active advertising is already in suspended state */
bool LeAdvertisingManagerSm_IsSuspended(void);

#endif /* LE_ADVERTSING_MANAGER_SM_H_ */
