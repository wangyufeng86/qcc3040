/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       anc_state_manager.h
\defgroup   anc_state_manager anc 
\ingroup    audio_domain
\brief      State manager for Active Noise Cancellation (ANC).

Responsibilities:
  Handles state transitions between init, enable and disable states.
  The ANC audio domain is used by \ref audio_curation.
*/

#ifndef ANC_STATE_MANAGER_H_
#define ANC_STATE_MANAGER_H_

/*\{*/
#include <anc.h>
#include "domain_message.h"

/*! \brief ANC state manager defines the various states handled in ANC. */
typedef enum
{
    anc_state_manager_uninitialised,
    anc_state_manager_power_off,
    anc_state_manager_enabled,
    anc_state_manager_disabled,
    anc_state_manager_tuning_mode_active
} anc_state_manager_t;

typedef struct
{
    bool state;
    uint8 mode;
    uint8 anc_leakthrough_gain; /* Leakthrough gain */
} anc_sync_data_t;

typedef anc_sync_data_t ANC_UPDATE_IND_T;

/*! \brief Events sent by Anc_Statemanager to other modules. */
typedef enum
{
    ANC_UPDATE_IND = ANC_MESSAGE_BASE,
} anc_msg_t;

/* Register with state proxy after initialization */
#ifdef ENABLE_ANC
#define AncStateManager_PostInitSetup() StateProxy_EventRegisterClient(GetAncTask(), state_proxy_event_type_anc)
#else
#define AncStateManager_PostInitSetup() ((void)(0))
#endif

/*!
    \brief Initialisation of ANC feature, reads microphone configuration  
           and default mode.
    \param init_task Unused
    \return TRUE always.
*/
#ifdef ENABLE_ANC
bool AncStateManager_Init(Task init_task);
#else
#define AncStateManager_Init(x) (FALSE)
#endif

#ifdef ENABLE_ANC
TaskData *GetAncTask(void);
#else
#define GetAncTask() (NULL)
#endif

#ifdef ENABLE_ANC
bool AncStateManager_CheckIfDspClockBoostUpRequired(void);
#else
#define AncStateManager_CheckIfDspClockBoostUpRequired() (FALSE)
#endif
/*!
    \brief ANC specific handling due to the device Powering On.
*/
#ifdef ENABLE_ANC
void AncStateManager_PowerOn(void);
#else
#define AncStateManager_PowerOn() ((void)(0))
#endif

/*!
    \brief ANC specific handling due to the device Powering Off.
*/  
#ifdef ENABLE_ANC
void AncStateManager_PowerOff(void);
#else
#define AncStateManager_PowerOff() ((void)(0))
#endif

/*!
    \brief Enable ANC functionality.  
*/   
#ifdef ENABLE_ANC
void AncStateManager_Enable(void);
#else
#define AncStateManager_Enable() ((void)(0))
#endif

/*! 
    \brief Disable ANC functionality.
 */   
#ifdef ENABLE_ANC
void AncStateManager_Disable(void);
#else
#define AncStateManager_Disable() ((void)(0))
#endif

/*!
    \brief Is ANC supported in this build ?

    This just checks if ANC may be supported in the build.
    Separate checks are needed to determine if ANC is permitted
    (licenced) or enabled.

    \return TRUE if ANC is enabled in the build, FALSE otherwise.
 */
#ifdef ENABLE_ANC
#define AncStateManager_IsSupported() TRUE
#else
#define AncStateManager_IsSupported() FALSE
#endif


/*!
    \brief Set the operating mode of ANC to configured mode_n. 
    \param mode To be set from existing avalaible modes 0 to 9.
*/
#ifdef ENABLE_ANC
void AncStateManager_SetMode(anc_mode_t mode);
#else
#define AncStateManager_SetMode(x) ((void)(0 * (x)))
#endif

/*! 
    \brief Get the Anc mode configured.
    \return mode which is set (from available mode 0 to 9).
 */
#ifdef ENABLE_ANC
anc_mode_t AncStateManager_GetMode(void);
#else
#define AncStateManager_GetMode() (0)
#endif

/*! 
    \brief Checks if ANC is due to be enabled.
    \return TRUE if it is enabled else FALSE.
 */
#ifdef ENABLE_ANC
bool AncStateManager_IsEnabled (void);
#else
#define AncStateManager_IsEnabled() (FALSE)
#endif

/*! 
    \brief Get the Anc mode configured.
    \return mode which is set (from available mode 0 to 9).
 */
#ifdef ENABLE_ANC
anc_mode_t AncStateManager_GetCurrentMode(void);
#else
#define AncStateManager_GetCurrentMode() (0)
#endif

/*! 
    \brief The function returns the number of modes configured.
    \return total modes in anc_modes_t.
 */
#ifdef ENABLE_ANC
uint8 AncStateManager_GetNumberOfModes(void);
#else
#define AncStateManager_GetNumberOfModes() (0)
#endif

/*! 
    \brief Checks the next mode of ANC.
    \param mode     Change to the next mode which is to be configured.
    \return anc_mode_t   The next mode to be set is returned from available modes 0 to 9.
 */
#ifdef ENABLE_ANC
anc_mode_t AncStateManager_GetNextMode(anc_mode_t mode);
#else
#define AncStateManager_GetNextMode(x) (0)
#endif

/*! 
    \brief Checks whether tuning mode is currently active.
    \return TRUE if it is active, else FALSE.
 */
#ifdef ENABLE_ANC
bool AncStateManager_IsTuningModeActive(void);
#else
#define AncStateManager_IsTuningModeActive() (FALSE)
#endif

/*! 
    \brief Cycles through next mode and sets it.
 */
#ifdef ENABLE_ANC
void AncStateManager_SetNextMode(void);
#else
#define AncStateManager_SetNextMode() ((void)(0))
#endif

/*! 
    \brief Enters ANC tuning mode.
 */
#ifdef ENABLE_ANC
void AncStateManager_EnterTuningMode(void);
#else
#define AncStateManager_EnterTuningMode() ((void)(0))
#endif

/*! 
    \brief Exits the ANC tuning mode.
 */
#ifdef ENABLE_ANC
void AncStateManager_ExitTuningMode(void);
#else
#define AncStateManager_ExitTuningMode() ((void)(0))
#endif

/*! 
    \brief Updates ANC feedforward fine gain from ANC Data structure to ANC H/W. This is not applicable when in 'Mode 1'.
		   AncStateManager_StoreAncLeakthroughGain(uint8 leakthrough_gain) has to be called BEFORE calling AncStateManager_UpdateAncLeakthroughGain()
		   
		   This function shall be called for "World Volume Leakthrough".
		   
 */
#ifdef ENABLE_ANC
void AncStateManager_UpdateAncLeakthroughGain(void);
#else
#define AncStateManager_UpdateAncLeakthroughGain() ((void)(0))
#endif

/*! \brief Register a Task to receive notifications from Anc_StateManager.

    Once registered, #client_task will receive #shadow_profile_msg_t messages.

    \param client_task Task to register to receive shadow_profile notifications.
*/
#ifdef ENABLE_ANC
void AncStateManager_ClientRegister(Task client_task);
#else
#define AncStateManager_ClientRegister(x) ((void)(0))
#endif

/*! \brief Un-register a Task that is receiving notifications from Anc_StateManager.

    If the task is not currently registered then nothing will be changed.

    \param client_task Task to un-register from shadow_profile notifications.
*/
#ifdef ENABLE_ANC
void AncStateManager_ClientUnregister(Task client_task);
#else
#define AncStateManager_ClientUnregister(x) ((void)(0))
#endif

/*! \brief To obtain Leakthrough gain for current mode stored in ANC data structure

    \returns leakthrough gain of ANC H/w 
*/
#ifdef ENABLE_ANC
uint8 AncStateManager_GetAncLeakthroughGain(void);
#else
#define AncStateManager_GetAncLeakthroughGain() (0)
#endif

/*! \brief To store Leakthrough gain in ANC data structure

    \param leakthrough_gain Leakthrough gain to be stored
*/
#ifdef ENABLE_ANC
void AncStateManager_StoreAncLeakthroughGain(uint8 leakthrough_gain);
#else
#define AncStateManager_StoreAncLeakthroughGain(x) ((void)(0))
#endif

/*!
    \brief  Configure ANC settings when a Peer connects.
 */
#ifdef ANC_PEER_SUPPORT
void AncStateManager_SychroniseStateWithPeer(void);
#else
#define AncStateManager_SychroniseStateWithPeer() ((void)(0))
#endif

/*! 
    \brief Get the next state of ANC.
    \return MessageId  Corresponding ANC message id either on/off is retrieved.
 */
#ifdef ANC_PEER_SUPPORT
MessageId AncStateManager_GetNextState(void);
#else
#define AncStateManager_GetNextState() (0)
#endif

/*! 
    \brief Get user event for the corresponding Anc mode.
    \param mode   current mode set.
    \return MessageId   Retrieve the message id for modes 0 to 9 set.
 */
#ifdef ANC_PEER_SUPPORT
MessageId AncStateManager_GetUsrEventFromMode(anc_mode_t mode);
#else
#define AncStateManager_GetUsrEventFromMode(x) (0)
#endif

/*! 
    \brief Test hook for unit tests to reset the ANC state.
    \param state  Reset the particular state
 */
#ifdef ANC_TEST_BUILD

#ifdef ENABLE_ANC
void AncStateManager_ResetStateMachine(anc_state_manager_t state);
#else
#define AncStateManager_ResetStateMachine(x) ((void)(0))
#endif

#endif /* ANC_TEST_BUILD*/

/*\}*/
#endif /* ANC_STATE_MANAGER_H_ */
