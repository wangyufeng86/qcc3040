/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       anc_state_manager.c
\brief      State manager implementation for Active Noise Cancellation (ANC) which handles transitions
            between init, powerOff, powerOn, enable, disable and tuning states.
*/


#ifdef ENABLE_ANC
#include "anc_state_manager.h"
#include "anc_config.h"
#include "kymera.h"
#include "microphones.h"
#include "state_proxy.h"
#include <panic.h>
#include <task_list.h>
#include <logging.h>

#define DEBUG_ASSERT(x, y) {if (!(x)) {DEBUG_LOG(y);Panic();}}

static void ancstateManager_HandleMessage(Task task, MessageId id, Message message);

/* ANC state manager data */
typedef struct
{
    /*! Anc StateManager task */
    TaskData task_data;
    /*! List of tasks registered for notifications */
    task_list_t *client_tasks;
    unsigned requested_enabled:1;
    unsigned actual_enabled:1;
    unsigned power_on:1;
    unsigned persist_anc_mode:1;
    unsigned persist_anc_enabled:1;
    unsigned enable_dsp_clock_boostup:1;
    unsigned unused:10;
    anc_state_manager_t state;
    anc_mode_t current_mode;
    anc_mode_t requested_mode;
    uint8 num_modes;
    uint8 anc_leakthrough_gain;
} anc_state_manager_data_t;

static anc_state_manager_data_t anc_data;

/*! Get pointer to Anc state manager structure */
#define GetAncData()  (&anc_data)
#define GetAncClients() TaskList_GetBaseTaskList(&GetAncData()->client_tasks)

TaskData *GetAncTask(void)
{
   return (&anc_data.task_data);
}

bool AncStateManager_CheckIfDspClockBoostUpRequired(void)
{
   return (anc_data.enable_dsp_clock_boostup);
}

/* ANC State Manager Events which are triggered during state transitions */
typedef enum
{
    anc_state_manager_event_initialise,
    anc_state_manager_event_power_on,
    anc_state_manager_event_power_off,
    anc_state_manager_event_enable,
    anc_state_manager_event_disable,
    anc_state_manager_event_set_mode_1,
    anc_state_manager_event_set_mode_2,
    anc_state_manager_event_set_mode_3,
    anc_state_manager_event_set_mode_4,
    anc_state_manager_event_set_mode_5,
    anc_state_manager_event_set_mode_6,
    anc_state_manager_event_set_mode_7,
    anc_state_manager_event_set_mode_8,
    anc_state_manager_event_set_mode_9,
    anc_state_manager_event_set_mode_10,
    anc_state_manager_event_activate_tuning_mode,
    anc_state_manager_event_deactivate_tuning_mode,
    anc_state_manager_event_set_anc_leakthrough_gain
} anc_state_manager_event_id_t;

static anc_mode_t getModeFromSetModeEvent(anc_state_manager_event_id_t event)
{
    anc_mode_t mode = anc_mode_1;
    
    switch(event)
    {
        case anc_state_manager_event_set_mode_2:
            mode = anc_mode_2;
            break;
        case anc_state_manager_event_set_mode_3:
            mode = anc_mode_3;
            break;
        case anc_state_manager_event_set_mode_4:
            mode = anc_mode_4;
            break;
        case anc_state_manager_event_set_mode_5:
            mode = anc_mode_5;
            break;
        case anc_state_manager_event_set_mode_6:
            mode = anc_mode_6;
            break;
        case anc_state_manager_event_set_mode_7:
            mode = anc_mode_7;
            break;
        case anc_state_manager_event_set_mode_8:
            mode = anc_mode_8;
            break;
        case anc_state_manager_event_set_mode_9:
            mode = anc_mode_9;
            break;
        case anc_state_manager_event_set_mode_10:
            mode = anc_mode_10;
            break;
        case anc_state_manager_event_set_mode_1:
        default:
            break;
    }
    return mode;
}

static anc_state_manager_event_id_t getSetModeEventFromMode(anc_mode_t mode)
{
    anc_state_manager_event_id_t state_event = anc_state_manager_event_set_mode_1;
    
    switch(mode)
    {
        case anc_mode_2:
            state_event = anc_state_manager_event_set_mode_2;
            break;
        case anc_mode_3:
            state_event = anc_state_manager_event_set_mode_3;
            break;
        case anc_mode_4:
            state_event = anc_state_manager_event_set_mode_4;
            break;
        case anc_mode_5:
            state_event = anc_state_manager_event_set_mode_5;
            break;
        case anc_mode_6:
            state_event = anc_state_manager_event_set_mode_6;
            break;
        case anc_mode_7:
            state_event = anc_state_manager_event_set_mode_7;
            break;
        case anc_mode_8:
            state_event = anc_state_manager_event_set_mode_8;
            break;
        case anc_mode_9:
            state_event = anc_state_manager_event_set_mode_9;
            break;
        case anc_mode_10:
            state_event = anc_state_manager_event_set_mode_10;
            break;
        case anc_mode_1:
        default:
            break;
    }
    return state_event;
}

static void ancStateManager_UpdateState(bool new_anc_state)
{
    bool current_anc_state = AncStateManager_IsEnabled();
    DEBUG_LOG("ancStateManager_UpdateState: current state = %u, new state = %u", current_anc_state, new_anc_state);

    if(current_anc_state != new_anc_state)
    {

        if(new_anc_state)
        {
            AncStateManager_Enable();
        }
        else
        {
            AncStateManager_Disable();
        }
    }
}

static void ancStateManager_UpdateMode(uint8 new_anc_mode)
{
    uint8 current_anc_mode = AncStateManager_GetMode();
    DEBUG_LOG("ancStateManager_UpdateMode: current mode = %u, new mode = %u", current_anc_mode, new_anc_mode);

    if(current_anc_mode != new_anc_mode)
    {
        AncStateManager_SetMode(new_anc_mode);
    }
}

/******************************************************************************
DESCRIPTION
    Set ANC Leakthrough gain for FeedForward path
    FFA path is used in FeedForward mode and FFB path in Hybrid mode
	ANC Leakthrough gain is not applicable for 'Mode 1'
*/
static void setAncLeakthroughGain(void)
{
    anc_state_manager_data_t *anc_sm = GetAncData();
    uint8 gain=  anc_sm -> anc_leakthrough_gain;
    bool ret_val = FALSE;
    anc_path_enable anc_path = appConfigAncPathEnable();

    DEBUG_LOG("setAncLeakthroughGain: %d \n", gain);

    if(AncStateManager_GetCurrentMode() != anc_mode_1)
    {
        switch(anc_path)
        {
            case hybrid_mode_left_only:
                ret_val = AncConfigureFFBPathGain(AUDIO_ANC_INSTANCE_0, gain);
                break;

            case feed_forward_mode_left_only:
                ret_val = AncConfigureFFAPathGain(AUDIO_ANC_INSTANCE_0, gain);
                break;

            default:
                DEBUG_LOG("setAncLeakthroughGain, cannot set Anc Leakthrough gain for anc_path:  %u", anc_path);
                break;
        }
    }
    else
    {
    DEBUG_LOG("Anc Leakthrough gain cannot be set in mode 0!");
    }
    if(!ret_val)
    {
    DEBUG_LOG("setAncLeakthroughGain failed!");
    }
}

static void ancStateManager_StoreAndUpdateAncLeakthroughGain(uint8 new_anc_leakthrough_gain)
{
    uint8 current_anc_leakthrough_gain = AncStateManager_GetAncLeakthroughGain();
    DEBUG_LOG("ancStateManager_StoreAndUpdateAncLeakthroughGain: current anc leakthrough gain  = %u, new anc leakthrough gain  = %u", current_anc_leakthrough_gain, new_anc_leakthrough_gain);

    if(current_anc_leakthrough_gain != new_anc_leakthrough_gain)
    {
        AncStateManager_StoreAncLeakthroughGain(new_anc_leakthrough_gain);
        setAncLeakthroughGain();
    }
}

static void ancStateManager_HandleStateProxyEvent(const STATE_PROXY_EVENT_T* event)
{
    switch(event->type)
    {
        case state_proxy_event_type_anc:
            DEBUG_LOG("ancStateManager_HandleStateProxyEvent: state proxy anc sync");
            ancStateManager_UpdateState(event->event.anc_data.state);
            ancStateManager_UpdateMode(event->event.anc_data.mode);
            ancStateManager_StoreAndUpdateAncLeakthroughGain(event->event.anc_data.anc_leakthrough_gain);

        break;

        default:
            break;
    }
}

static void ancstateManager_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        case STATE_PROXY_EVENT:
            ancStateManager_HandleStateProxyEvent((const STATE_PROXY_EVENT_T*)message);
            break;

        default:
            DEBUG_LOG("ancstateManager_HandleMessage: Event not handled");
            break;
    }
}

/******************************************************************************
DESCRIPTION
    This function is responsible for persisting any of the ANC session data
    that is required.
*/
static void setSessionData(void)
{
    anc_writeable_config_def_t *write_data = NULL;

    if(ancConfigManagerGetWriteableConfig(ANC_WRITEABLE_CONFIG_BLK_ID, (void **)&write_data, 0))
    {
        if (anc_data.persist_anc_enabled)
        {
            DEBUG_LOG("setSessionData: Persisting ANC enabled state %d\n", anc_data.requested_enabled);
            write_data->initial_anc_state =  anc_data.requested_enabled;
        }
    
        if (anc_data.persist_anc_mode)
        {
            DEBUG_LOG("setSessionData: Persisting ANC mode %d\n", anc_data.requested_mode);
            write_data->initial_anc_mode = anc_data.requested_mode;
        }
    
        ancConfigManagerUpdateWriteableConfig(ANC_WRITEABLE_CONFIG_BLK_ID);
    }
}

/*! \brief Notify Anc update to registered clients. */
static void ancstateManager_MsgRegisteredClients(void)
{
    anc_state_manager_data_t *anc_sm = GetAncData();

    if(anc_sm->client_tasks) /* Check if any of client registered */
    {
        MESSAGE_MAKE(ind, ANC_UPDATE_IND_T);
        ind->state = AncStateManager_IsEnabled();
        ind->mode = AncStateManager_GetMode();
        ind->anc_leakthrough_gain = AncStateManager_GetAncLeakthroughGain();

        TaskList_MessageSend(anc_sm->client_tasks, ANC_UPDATE_IND, ind);
    }
}

/******************************************************************************
DESCRIPTION
    Handle the transition into a new state. This function is responsible for
    generating the state related system events.
*/
static void changeState(anc_state_manager_t new_state)
{
    DEBUG_LOG("changeState: ANC State %d -> %d\n", anc_data.state, new_state);

    if ((new_state == anc_state_manager_power_off) && (anc_data.state != anc_state_manager_uninitialised))
    {
        /* When we power off from an on state persist any state required */
        setSessionData();
    }
    /* Update state */
    anc_data.state = new_state;
}

/******************************************************************************
DESCRIPTION
    Setup Enter tuning mode by disabling ANC and changes state to tuning mode.
*/
static void setupEnterTuningMode(void)
{
    DEBUG_LOG("setupTuningMode: KymeraAnc_EnterTuning \n");

    if(AncIsEnabled())
    {
       AncEnable(FALSE);
    }
        
    KymeraAnc_EnterTuning();
    
    changeState(anc_state_manager_tuning_mode_active);
}

/***************************************************************************earbud_ui
DESCRIPTION
    Get path configured for ANC

RETURNS
    None
*/
static audio_anc_path_id ancStateManager_GetAncPath(void)
{
    audio_anc_path_id path=AUDIO_ANC_PATH_ID_NONE;

    if (appConfigAncPathEnable() == feed_forward_mode_left_only)
    {
        path = AUDIO_ANC_PATH_ID_FFA;
    }
    else if (appConfigAncPathEnable() == hybrid_mode_left_only)
    {
        path = AUDIO_ANC_PATH_ID_FFB;
    }

    return path;
}

static uint8 AncStateManager_ReadFineGainFromInstance(void)
{
    uint8 gain;
    audio_anc_path_id gain_path = ancStateManager_GetAncPath();

    AncReadFineGainFromInstance(AUDIO_ANC_INSTANCE_0, gain_path, &gain);

    return gain;
}

/******************************************************************************
DESCRIPTION
    Update the state of the ANC VM library. This is the 'actual' state, as opposed
    to the 'requested' state and therefore the 'actual' state variables should
    only ever be updated in this function.
    
    RETURNS
    Bool indicating if updating lib state was successful or not.

*/  
static bool updateLibState(bool enable, anc_mode_t mode)
{
    bool retry_later = TRUE;
    anc_data.enable_dsp_clock_boostup = TRUE;

    /* Enable operator framwork before updating DSP clock */
    OperatorFrameworkEnable(1);

    /*Change the DSP clock before enabling ANC and changing up the mode*/
    KymeraAnc_UpdateDspClock();

    /* Check to see if we are changing mode */
    if (anc_data.current_mode != mode)
    {
        /* Set ANC Mode */
        if (!AncSetMode(mode) || (anc_data.requested_mode >= AncStateManager_GetNumberOfModes()))
        {
            DEBUG_LOG("updateLibState: ANC Set Mode failed %d \n", mode + 1);
            retry_later = FALSE;
            /* fallback to previous success mode set */
            anc_data.requested_mode = anc_data.current_mode;
        }
        else
        {
            /* Update mode state */
            DEBUG_LOG("updateLibState: ANC Set Mode %d\n", mode + 1);
            anc_data.current_mode = mode;
        }
     }

     /* Determine state to update in VM lib */
     if (anc_data.actual_enabled != enable)
     {
         appKymeraExternalAmpControl(enable);
         
         if (!AncEnable(enable))
         {
             /* If this does fail in a release build then we will continue
                 and updating the ANC state will be tried again next time
                 an event causes a state change. */
             DEBUG_LOG("updateLibState: ANC Enable failed %d\n", enable);
             retry_later = FALSE;
         }

         /* Update enabled state */
         DEBUG_LOG("updateLibState: ANC Enable %d\n", enable);
         anc_data.actual_enabled = enable;
     }
     anc_data.enable_dsp_clock_boostup = FALSE;

     /*Revert DSP clock to its previous speed*/
     KymeraAnc_UpdateDspClock();

     /*Disable operator framwork after reverting DSP clock*/
     OperatorFrameworkEnable(0);
     return retry_later;
}

/******************************************************************************
DESCRIPTION
    Update session data retrieved from config

RETURNS
    Bool indicating if updating config was successful or not.
*/
static bool getSessionData(void)
{
    anc_writeable_config_def_t *write_data = NULL;

    ancConfigManagerGetWriteableConfig(ANC_WRITEABLE_CONFIG_BLK_ID, (void **)&write_data, (uint16)sizeof(anc_writeable_config_def_t));

    /* Extract session data */
    anc_data.requested_enabled = write_data->initial_anc_state;
    anc_data.persist_anc_enabled = write_data->persist_initial_state;
    anc_data.requested_mode = write_data->initial_anc_mode;
    anc_data.persist_anc_mode = write_data->persist_initial_mode;
    
    ancConfigManagerReleaseConfig(ANC_WRITEABLE_CONFIG_BLK_ID);

    return TRUE;
}

/******************************************************************************
DESCRIPTION
    Read the configuration from the ANC Mic params.
*/
static bool readMicConfigParams(anc_mic_params_t *anc_mic_params)
{
    anc_readonly_config_def_t *read_data = NULL;

    if (ancConfigManagerGetReadOnlyConfig(ANC_READONLY_CONFIG_BLK_ID, (const void **)&read_data))
    {
        microphone_number_t feedForwardLeftMic = read_data->anc_mic_params_r_config.feed_forward_left_mic;
        microphone_number_t feedForwardRightMic = read_data->anc_mic_params_r_config.feed_forward_right_mic;
        microphone_number_t feedBackLeftMic = read_data->anc_mic_params_r_config.feed_back_left_mic;
        microphone_number_t feedBackRightMic = read_data->anc_mic_params_r_config.feed_back_right_mic;

        memset(anc_mic_params, 0, sizeof(anc_mic_params_t));

        if (feedForwardLeftMic)
        {
            anc_mic_params->enabled_mics |= feed_forward_left;
            anc_mic_params->feed_forward_left = *Microphones_GetMicrophoneConfig(feedForwardLeftMic);
        }

        if (feedForwardRightMic)
        {
            anc_mic_params->enabled_mics |= feed_forward_right;
            anc_mic_params->feed_forward_right = *Microphones_GetMicrophoneConfig(feedForwardRightMic);
        }

        if (feedBackLeftMic)
        {
            anc_mic_params->enabled_mics |= feed_back_left;
            anc_mic_params->feed_back_left = *Microphones_GetMicrophoneConfig(feedBackLeftMic);
        }

        if (feedBackRightMic)
        {
            anc_mic_params->enabled_mics |= feed_back_right;
            anc_mic_params->feed_back_right = *Microphones_GetMicrophoneConfig(feedBackRightMic);
        }

        ancConfigManagerReleaseConfig(ANC_READONLY_CONFIG_BLK_ID);

        return TRUE;
    }
    DEBUG_LOG("readMicConfigParams: Failed to read ANC Config Block\n");
    return FALSE;
}

/****************************************************************************    
DESCRIPTION
    Read the number of configured Anc modes.
*/
static uint8 readNumModes(void)
{
    anc_readonly_config_def_t *read_data = NULL;
    uint8 num_modes = 0;

    /* Read the existing Config data */
    if (ancConfigManagerGetReadOnlyConfig(ANC_READONLY_CONFIG_BLK_ID, (const void **)&read_data))
    {
        num_modes = read_data->num_anc_modes;
        ancConfigManagerReleaseConfig(ANC_READONLY_CONFIG_BLK_ID);
    }
    return num_modes;
}

anc_mode_t AncStateManager_GetMode(void)
{
    return (anc_data.requested_mode);
}

/******************************************************************************
DESCRIPTION
    This function reads the ANC configuration and initialises the ANC library
    returns TRUE on success FALSE otherwise 
*/ 
static bool configureAndInit(void)
{
    anc_mic_params_t anc_mic_params;
    bool init_success = FALSE;

    if(readMicConfigParams(&anc_mic_params) && getSessionData())
    {
        if(AncInit(&anc_mic_params, AncStateManager_GetMode()))
        {
            /* Update local state to indicate successful initialisation of ANC */
            anc_data.current_mode = anc_data.requested_mode;
            anc_data.actual_enabled = FALSE;
            anc_data.num_modes = readNumModes();
            anc_data.task_data.handler = ancstateManager_HandleMessage;
            /* ANC state manger task creation */
            anc_data.client_tasks = TaskList_Create();
            changeState(anc_state_manager_power_off);
            init_success = TRUE;
        }
    }
    return init_success;
}

/******************************************************************************
DESCRIPTION
    Event handler for the Uninitialised State

RETURNS
    Bool indicating if processing event was successful or not.
*/ 
static bool handleUninitialisedEvent(anc_state_manager_event_id_t event)
{
    bool init_success = FALSE;

    switch (event)
    {
        case anc_state_manager_event_initialise:
        {
            if(configureAndInit())
            {
                init_success = TRUE;
            }
            else
            {
                DEBUG_ASSERT(init_success, "handleUninitialisedEvent: ANC Failed to initialise due to incorrect mic configuration/ licencing issue \n");
                /* indicate error by Led */
            }
        }
        break;

        default:
        {
            DEBUG_LOG("handleUninitialisedEvent: Unhandled event [%d]\n", event);
        }
        break;
    }
    return init_success;
}

/******************************************************************************
DESCRIPTION
    Event handler for the Power Off State

RETURNS
    Bool indicating if processing event was successful or not.
*/ 
static bool handlePowerOffEvent(anc_state_manager_event_id_t event)
{
    bool event_handled = FALSE;

    DEBUG_ASSERT(!anc_data.actual_enabled, "handlePowerOffEvent: ANC actual enabled in power off state\n");

    switch (event)
    {
        case anc_state_manager_event_power_on:
        {
            anc_state_manager_t next_state = anc_state_manager_disabled;
            anc_data.power_on = TRUE;

            /* If we were previously enabled then enable on power on */
            if (anc_data.requested_enabled)
            {
                if(updateLibState(anc_data.requested_enabled, anc_data.requested_mode))
                {
                    /* Lib is enabled */
                    next_state = anc_state_manager_enabled;
                }
            }
            /* Update state */
            changeState(next_state);
            
            event_handled = TRUE;
        }
        break;

        default:
        {
            DEBUG_LOG("handlePowerOffEvent: Unhandled event [%d]\n", event);
        }
        break;
    }
    return event_handled;
}

/******************************************************************************
DESCRIPTION
    Event handler for the Enabled State

RETURNS
    Bool indicating if processing event was successful or not.
*/
static bool handleEnabledEvent(anc_state_manager_event_id_t event)
{
    /* Assume failure until proven otherwise */
    bool event_handled = FALSE;
    anc_state_manager_t next_state = anc_state_manager_disabled;

    switch (event)
    {
        case anc_state_manager_event_power_off:
        {
            /* When powering off we need to disable ANC in the VM Lib first */
            next_state = anc_state_manager_power_off;
            anc_data.power_on = FALSE;
        }
        /* fallthrough */
        case anc_state_manager_event_disable:
        {
            /* Only update requested enabled if not due to a power off event */
            anc_data.requested_enabled = (next_state == anc_state_manager_power_off);

#ifdef INCLUDE_ANC_PASSTHROUGH_SUPPORT_CHAIN
            KymeraAnc_DisconnectPassthroughSupportChainFromDac();
            KymeraAnc_DestroyPassthroughSupportChain();
#endif

            /* Disable ANC */
            updateLibState(FALSE, anc_data.requested_mode);
            
            /* Update state */
            changeState(next_state);
            event_handled = TRUE;
            /* Notify ANC state update to registered clients */
            ancstateManager_MsgRegisteredClients();
        }
        break;

        case anc_state_manager_event_set_mode_1: /* fallthrough */
        case anc_state_manager_event_set_mode_2:
        case anc_state_manager_event_set_mode_3:
        case anc_state_manager_event_set_mode_4:
        case anc_state_manager_event_set_mode_5:
        case anc_state_manager_event_set_mode_6:
        case anc_state_manager_event_set_mode_7:
        case anc_state_manager_event_set_mode_8:
        case anc_state_manager_event_set_mode_9:
        case anc_state_manager_event_set_mode_10:            
        {
            anc_data.requested_mode = getModeFromSetModeEvent(event);           

            /* Update the ANC Mode */
            updateLibState(anc_data.requested_enabled, anc_data.requested_mode);

            /* Update Leakthrough gain in ANC data structure */
            AncStateManager_StoreAncLeakthroughGain(AncStateManager_ReadFineGainFromInstance());

            /* Notify ANC mode update to registered clients */
            ancstateManager_MsgRegisteredClients();


            event_handled = TRUE;
        }
        break;

        case anc_state_manager_event_activate_tuning_mode:
        {            
            setupEnterTuningMode();
            event_handled = TRUE;
        }
        break;

        case anc_state_manager_event_set_anc_leakthrough_gain:
        {
            setAncLeakthroughGain();

            /* Notify ANC gain update to registered clients */
            ancstateManager_MsgRegisteredClients();

            event_handled = TRUE;
        }
        break;
            
        default:
        {
            DEBUG_LOG("handleEnabledEvent: Unhandled event [%d]\n", event);
        }
        break;
    }
    return event_handled;
}

/******************************************************************************
DESCRIPTION
    Event handler for the Disabled State

RETURNS
    Bool indicating if processing event was successful or not.
*/
static bool handleDisabledEvent(anc_state_manager_event_id_t event)
{
    /* Assume failure until proven otherwise */
    bool event_handled = FALSE;

    switch (event)
    {
        case anc_state_manager_event_power_off:
        {
            /* Nothing to do, just update state */
            changeState(anc_state_manager_power_off);
            anc_data.power_on = FALSE;
            event_handled = TRUE;
        }
        break;

        case anc_state_manager_event_enable:
        {
            /* Try to enable */
            anc_state_manager_t next_state = anc_state_manager_enabled;
            anc_data.requested_enabled = TRUE;
			
#ifdef INCLUDE_ANC_PASSTHROUGH_SUPPORT_CHAIN
            KymeraAnc_CreatePassthroughSupportChain();
            KymeraAnc_ConnectPassthroughSupportChainToDac();
#endif

            /* Enable ANC */
            updateLibState(anc_data.requested_enabled, anc_data.requested_mode);
            
            /* Update state */
            changeState(next_state);
           
            event_handled = TRUE;

            /* Notify ANC state update to registered clients */
            ancstateManager_MsgRegisteredClients();
        }
        break;

        case anc_state_manager_event_set_mode_1: /* fallthrough */
        case anc_state_manager_event_set_mode_2:
        case anc_state_manager_event_set_mode_3:
        case anc_state_manager_event_set_mode_4:
        case anc_state_manager_event_set_mode_5:
        case anc_state_manager_event_set_mode_6:
        case anc_state_manager_event_set_mode_7:
        case anc_state_manager_event_set_mode_8:
        case anc_state_manager_event_set_mode_9:
        case anc_state_manager_event_set_mode_10:     
        {
            /* Update the requested ANC Mode, will get applied next time we enable */
            anc_data.requested_mode = getModeFromSetModeEvent(event);

            /* Update Leakthrough gain in ANC data structure */
            AncStateManager_StoreAncLeakthroughGain(AncStateManager_ReadFineGainFromInstance());

            event_handled = TRUE;
        }
        break;
        
        case anc_state_manager_event_activate_tuning_mode:
        {
            setupEnterTuningMode();
            event_handled = TRUE;
        }
        break;

        default:
        {
            DEBUG_LOG("handleDisabledEvent: Unhandled event [%d]\n", event);
        }
        break;
    }
    return event_handled;
}

static bool handleTuningModeActiveEvent(anc_state_manager_event_id_t event)
{
    bool event_handled = FALSE;
    
    switch(event)
    {
        case anc_state_manager_event_power_off:
        {
            DEBUG_LOG("handleTuningModeActiveEvent: anc_state_manager_event_power_off\n");

            KymeraAnc_ExitTuning();
            changeState(anc_state_manager_power_off);
            event_handled = TRUE;
        }
        break;

        case anc_state_manager_event_deactivate_tuning_mode:
        {
            DEBUG_LOG("handleTuningModeActiveEvent: anc_state_manager_event_deactivate_tuning_mode\n");

            KymeraAnc_ExitTuning();
            changeState(anc_state_manager_disabled);
            event_handled = TRUE;
        }
        break;
        
        default:
        break;
    }
    return event_handled;
}

/******************************************************************************
DESCRIPTION
    Entry point to the ANC State Machine.

RETURNS
    Bool indicating if processing event was successful or not.
*/
static bool ancStateManager_HandleEvent(anc_state_manager_event_id_t event)
{
    bool ret_val = FALSE;

    DEBUG_LOG("ancStateManager_HandleEvent: ANC Handle Event %d in State %d\n", event, anc_data.state);

    switch(anc_data.state)
    {
        case anc_state_manager_uninitialised:
            ret_val = handleUninitialisedEvent(event);
        break;
        
        case anc_state_manager_power_off:
            ret_val = handlePowerOffEvent(event);
        break;

        case anc_state_manager_enabled:
            ret_val = handleEnabledEvent(event);
        break;

        case anc_state_manager_disabled:
            ret_val = handleDisabledEvent(event);
        break;

        case anc_state_manager_tuning_mode_active:
            ret_val = handleTuningModeActiveEvent(event);
        break;

        default:
            DEBUG_LOG("ancStateManager_HandleEvent: Unhandled state [%d]\n", anc_data.state);
        break;
    }
    return ret_val;
}

/*******************************************************************************
 * All the functions from this point onwards are the ANC module API functions
 * The functions are simply responsible for injecting
 * the correct event into the ANC State Machine, which is then responsible
 * for taking the appropriate action.
 ******************************************************************************/

bool AncStateManager_Init(Task init_task)
{
    UNUSED(init_task);

    /* Initialise the ANC VM Lib */
    if(ancStateManager_HandleEvent(anc_state_manager_event_initialise))
    {
        /* Initialisation successful, go ahead with ANC power ON*/
        AncStateManager_PowerOn();
    }
    return TRUE;
}

void AncStateManager_PowerOn(void)
{
    /* Power On ANC */
    if(!ancStateManager_HandleEvent(anc_state_manager_event_power_on))
    {
        DEBUG_LOG("AncStateManager_PowerOn: Power On ANC failed\n");
    }
}

void AncStateManager_PowerOff(void)
{
    /* Power Off ANC */
    if (!ancStateManager_HandleEvent(anc_state_manager_event_power_off))
    {
        DEBUG_LOG("AncStateManager_PowerOff: Power Off ANC failed\n");
    }
}

void AncStateManager_Enable(void)
{
    /* Enable ANC */
    if (!ancStateManager_HandleEvent(anc_state_manager_event_enable))
    {
        DEBUG_LOG("AncStateManager_Enable: Enable ANC failed\n");
    }
}

void AncStateManager_Disable(void)
{
    /* Disable ANC */
    if (!ancStateManager_HandleEvent(anc_state_manager_event_disable))
    {
        DEBUG_LOG("AncStateManager_Disable: Disable ANC failed\n");
    }
}

void AncStateManager_SetMode(anc_mode_t mode)
{
    anc_state_manager_event_id_t state_event = getSetModeEventFromMode (mode);

    /* Set ANC mode_n */
    if (!ancStateManager_HandleEvent(state_event))
    {
        DEBUG_LOG("AncStateManager_SetMode: Set ANC Mode %d failed\n", mode);
    }
}

void AncStateManager_EnterTuningMode(void)
{
    if(!ancStateManager_HandleEvent(anc_state_manager_event_activate_tuning_mode))
    {
       DEBUG_LOG("AncStateManager_EnterTuningMode: Tuning mode event failed\n");
    }
}

void AncStateManager_ExitTuningMode(void)
{
    if(!ancStateManager_HandleEvent(anc_state_manager_event_deactivate_tuning_mode))
    {
       DEBUG_LOG("AncStateManager_ExitTuningMode: Tuning mode event failed\n");
    }
}

void AncStateManager_UpdateAncLeakthroughGain(void)
{
    if(!ancStateManager_HandleEvent(anc_state_manager_event_set_anc_leakthrough_gain))
    {
       DEBUG_LOG("AncStateManager_UpdateAncLeakthroughGain: Set Anc Leakthrough gain event failed\n");
    }
}

bool AncStateManager_IsEnabled(void)
{
    return (anc_data.state == anc_state_manager_enabled);
}

anc_mode_t AncStateManager_GetCurrentMode(void)
{
    anc_state_manager_data_t *anc_sm = GetAncData();
    return anc_sm -> current_mode;
}

uint8 AncStateManager_GetNumberOfModes(void)
{
    return anc_data.num_modes;
}

anc_mode_t AncStateManager_GetNextMode(anc_mode_t anc_mode)
{
    anc_mode++;
    if(anc_mode > AncStateManager_GetNumberOfModes())
    {
       anc_mode = anc_mode_1;
    }
    return anc_mode;
}

void AncStateManager_SetNextMode(void)
{
    anc_data.requested_mode = AncStateManager_GetNextMode(anc_data.requested_mode);
    AncStateManager_SetMode(anc_data.requested_mode);
 }

bool AncStateManager_IsTuningModeActive(void)
{
    return (anc_data.state == anc_state_manager_tuning_mode_active);
}

void AncStateManager_ClientRegister(Task client_task)
{
    anc_state_manager_data_t *anc_sm = GetAncData();
    TaskList_AddTask(anc_sm->client_tasks, client_task);
}

void AncStateManager_ClientUnregister(Task client_task)
{
    anc_state_manager_data_t *anc_sm = GetAncData();
    TaskList_RemoveTask(anc_sm->client_tasks, client_task);
}

uint8 AncStateManager_GetAncLeakthroughGain(void)
{
    anc_state_manager_data_t *anc_sm = GetAncData();
    return anc_sm -> anc_leakthrough_gain;
}

void AncStateManager_StoreAncLeakthroughGain(uint8 anc_leakthrough_gain)
{

    if(AncStateManager_GetCurrentMode() != anc_mode_1)
    {
        anc_state_manager_data_t *anc_sm = GetAncData();
        anc_sm -> anc_leakthrough_gain = anc_leakthrough_gain;
    }
}


#ifdef ANC_PEER_SUPPORT
void AncStateManager_SychroniseStateWithPeer (void)
{
    if (ancPeerIsLinkMaster())
    {
        ancPeerSendAncState();
        ancPeerSendAncMode();
    }
}

MessageId AncStateManager_GetNextState(void)
{
    MessageId id;

    if (AncStateManager_IsEnabled())
    {
        id = EventUsrAncOff;
    }
    else
    {
        id = EventUsrAncOn;
    }

    return id;
}

MessageId AncStateManager_GetUsrEventFromMode(anc_mode_t anc_mode)
{
    MessageId id = EventUsrAncMode1;

    switch(anc_mode)
    {
        case anc_mode_1:
            id = EventUsrAncMode1;
            break;
        case anc_mode_2:
            id = EventUsrAncMode2;
            break;
        case anc_mode_3:
            id = EventUsrAncMode3;
            break;
        case anc_mode_4:
            id = EventUsrAncMode4;
            break;
        case anc_mode_5:
            id = EventUsrAncMode5;
            break;
        case anc_mode_6:
            id = EventUsrAncMode6;
            break;
        case anc_mode_7:
            id = EventUsrAncMode7;
            break;
        case anc_mode_8:
            id = EventUsrAncMode8;
            break;
        case anc_mode_9:
            id = EventUsrAncMode9;
            break;
        case anc_mode_10:
            id = EventUsrAncMode10;
            break;
        default:
            break;
    }
    return id;
}
#endif /* ANC_PEER_SUPPORT */

#ifdef ANC_TEST_BUILD
void AncStateManager_ResetStateMachine(anc_state_manager_t state)
{
    anc_data.state = state;
}
#endif /* ANC_TEST_BUILD */

#endif /* ENABLE_ANC */
