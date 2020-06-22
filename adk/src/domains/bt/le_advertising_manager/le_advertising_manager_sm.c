/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      LE advertising manager state machine
*/

#include <adc.h>
#include <panic.h>
#include <stdlib.h>

#include <connection.h>
#include <connection_no_ble.h>

#include "le_advertising_manager.h"
#include "le_advertising_manager_private.h"
#include "le_advertising_manager_sm.h"

static void leAdvertisingManager_SuspendAdvertising(void);

/* Pointer to the instance of LE advertising manager internal state machine */
static le_adv_mgr_state_machine_t * sm = NULL;

/* Local Function to get the internal state of LE Advertising Manager */
static le_adv_mgr_state_t leAdvertisingManager_GetState(void)
{
    PanicNull(sm);
    
    return sm->state;
}

/* Local Function to Exit Starting Advertising State */
static void leAdvertisingManager_ExitStartingState(void)
{
    return;
    
}

/* Local Function to Exit Started Advertising State */
static void leAdvertisingManager_ExitStartedState(void)
{
    return;
    
}

/* Local Function to Exit Initialised Advertising State */
static void leAdvertisingManager_ExitInitialisedState(void)
{
    return;
    
}

/* Local Function to Exit Uninitialised Advertising State */
static void leAdvertisingManager_ExitUninitialisedState(void)
{
    return;
    
}

/* Local Function to Exit Suspending Advertising State */
static void leAdvertisingManager_ExitSuspendingState(void)
{
    return;
    
}

/* Local Function to Exit Suspended Advertising State */
static void leAdvertisingManager_ExitSuspendedState(void)
{
    return;
    
}

/* Local Function to Enter Starting Advertising State */
static void leAdvertisingManager_EnterStartingState(void)
{
    sm->state = le_adv_mgr_state_starting;
    return;
    
}

/* Local Function to Enter Started Advertising State */
static void leAdvertisingManager_EnterStartedState(void)
{
    sm->state = le_adv_mgr_state_started;
    return;
    
}

/* Local Function to Enter Initialised Advertising State */
static void leAdvertisingManager_EnterInitialisedState(void)
{
    sm->state = le_adv_mgr_state_initialised;
    return;
    
}

/* Local Function to Enter Uninitialised Advertising State */
static void leAdvertisingManager_EnterUninitialisedState(void)
{
        sm->state = le_adv_mgr_state_uninitialised;
    return;
    
}

/* Local Function to Enter Suspending Advertising State */
static void leAdvertisingManager_EnterSuspendingState(void)
{
    sm->state = le_adv_mgr_state_suspending;
    leAdvertisingManager_SuspendAdvertising();   
    return;
    
}

/* Local Function to Enter Suspended Advertising State */
static void leAdvertisingManager_EnterSuspendedState(void)
{
    sm->state = le_adv_mgr_state_suspended;
    return;
    
}

/* Local Function to Suspend Advertising */
static void leAdvertisingManager_SuspendAdvertising(void)
{
    ConnectionDmBleSetAdvertiseEnableReq(AdvManagerGetTask(), FALSE);
}
           
/* Local Function to Resume Advertising with the Existing Advertising Data and Parameter Set */
static void leAdvertisingManager_ResumeAdvertising(void)
{    
    ConnectionDmBleSetAdvertiseEnableReq(AdvManagerGetTask(), TRUE);    
}

/* API Function to initialise the state machine */
le_adv_mgr_state_machine_t * LeAdvertisingManagerSm_Init(void)
{
    if(NULL == sm)
    {
        sm = PanicUnlessMalloc(sizeof(le_adv_mgr_state_machine_t));
        memset(sm, 0, sizeof(le_adv_mgr_state_machine_t));
        sm->state = le_adv_mgr_state_uninitialised;
    }
    
    return sm;    
}

/* API Function to deinitialise the state machine */
void LeAdvertisingManagerSm_DeInit(le_adv_mgr_state_machine_t * state_machine)
{
    if(state_machine != sm)
        Panic();
    
    free(sm);
    sm = NULL;
    state_machine = NULL;
}

/* API Function to change the existing state of the state machine */
void LeAdvertisingManagerSm_SetState(le_adv_mgr_state_t state)
{
    le_adv_mgr_state_t old_state = sm->state;

    DEBUG_LOG_STATE("LeAdvertisingManagerSm_SetState Transition %d->%d", old_state, state);

    switch(old_state)
    {
        case le_adv_mgr_state_uninitialised:
        leAdvertisingManager_ExitUninitialisedState();        
        break;
        
        case le_adv_mgr_state_initialised:
        leAdvertisingManager_ExitInitialisedState();
        break;        
        
        case le_adv_mgr_state_starting:
        leAdvertisingManager_ExitStartingState();
        break;        
        
        case le_adv_mgr_state_suspending:
        leAdvertisingManager_ExitSuspendingState();
        break;
        
        case le_adv_mgr_state_started:
        leAdvertisingManager_ExitStartedState();
        break;
                
        case le_adv_mgr_state_suspended:
        leAdvertisingManager_ExitSuspendedState();
        break;
        
        default:        
        break;
    }
    
    switch(state)
    {
        case le_adv_mgr_state_uninitialised:
        leAdvertisingManager_EnterUninitialisedState();        
        break;
        
        case le_adv_mgr_state_initialised:
        leAdvertisingManager_EnterInitialisedState();
        break;        
        
        case le_adv_mgr_state_starting:
        if(le_adv_mgr_state_initialised != old_state )
            leAdvertisingManager_ResumeAdvertising();
        leAdvertisingManager_EnterStartingState();
        break;        
        
        case le_adv_mgr_state_suspending:
        leAdvertisingManager_EnterSuspendingState();     
        break;
        
        case le_adv_mgr_state_started:
        leAdvertisingManager_EnterStartedState();
        break;
                
        case le_adv_mgr_state_suspended:
        leAdvertisingManager_EnterSuspendedState();
        break;
        
        default:        
        break;
    }
    
}

/* Local Function to check if there is an active advertising operation already in the process of starting */
bool LeAdvertisingManagerSm_IsAdvertisingStarting(void)
{
    return (le_adv_mgr_state_starting == leAdvertisingManager_GetState());
}

/* Local Function to check if there is an active advertising operation already started and in progress */
bool LeAdvertisingManagerSm_IsAdvertisingStarted(void)
{
    return (le_adv_mgr_state_started == leAdvertisingManager_GetState());
}

/* Local Function to check if Initialisation has already been completed with success */
bool LeAdvertisingManagerSm_IsInitialised(void)
{
    if(NULL == sm)
        return FALSE;

    if(le_adv_mgr_state_uninitialised == leAdvertisingManager_GetState())
        return FALSE;

    return TRUE;
}

/* Local Function to check if LE advertising manager is in the process of suspending the active advertising */
bool LeAdvertisingManagerSm_IsSuspending(void)
{
    return (le_adv_mgr_state_suspending == leAdvertisingManager_GetState());
}

/* Local Function to check if the active advertising is already in suspended state */
bool LeAdvertisingManagerSm_IsSuspended(void)
{

    return (le_adv_mgr_state_suspended == leAdvertisingManager_GetState());
}
