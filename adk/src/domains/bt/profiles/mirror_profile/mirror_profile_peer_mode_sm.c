/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      State machine to control peer mode (sniff or active)
*/

#ifdef INCLUDE_MIRRORING

#include "mirror_profile_private.h"
#include "mirror_profile_sm.h"
#include "mirror_profile_peer_mode_sm.h"
#include "bt_device.h"
#include "kymera.h"

static mirror_profile_peer_mode_state_t mirrorProfilePeerMode_SmTransition(void);
static void mirrorProfilePeerMode_SmKick(void);
static void mirrorProfilePeerMode_SetState(mirror_profile_peer_mode_state_t new_state);
static void mirrorProfilePeerMode_HandleDmModeChangeEvent(CL_DM_MODE_CHANGE_EVENT_T *event);

void mirrorProfile_HandleTpConManagerDisconnectInd(const CON_MANAGER_TP_DISCONNECT_IND_T *ind)
{
    if (appDeviceIsPeer(&ind->tpaddr.taddr.addr))
    {
        DEBUG_LOG("mirrorProfile_HandleTpConManagerDisconnectInd");
        /* If peer disconnects, reset the state */
        mirrorProfilePeerMode_SetState(MIRROR_PROFILE_PEER_MODE_STATE_ACTIVE);
        MirrorProfilePeerMode_SetTargetState(MIRROR_PROFILE_PEER_MODE_STATE_ACTIVE);
    }
}

void mirrorProfile_HandleTpConManagerConnectInd(const CON_MANAGER_TP_CONNECT_IND_T *ind)
{
    if (appDeviceIsPeer(&ind->tpaddr.taddr.addr))
    {
        DEBUG_LOG("mirrorProfile_HandleTpConManagerConnectInd");
        /* Message to trigger putting the peer link into sniff mode
        if state remains unchanged */
        MessageSendLater(MirrorProfile_GetTask(),
                        MIRROR_INTERNAL_IDLE_PEER_ENTER_SNIFF, NULL,
                        mirrorProfileConfig_IdlePeerEnterSniffTimeout());
    }
}

bool MirrorProfile_HandleConnectionLibraryMessages(MessageId id, Message message,
                                                   bool already_handled)
{
    switch (id)
    {
        case CL_DM_MODE_CHANGE_EVENT:
            mirrorProfilePeerMode_HandleDmModeChangeEvent((CL_DM_MODE_CHANGE_EVENT_T *)message);
        break;
        default:
        break;
    }
    return already_handled;
}

/*! \brief Logic to transition from current state to target state.

    \return The next state to enter in the transition to the target state.

    Generally, the logic determines the transitionary state to enter from the
    current steady state.
 */
static mirror_profile_peer_mode_state_t mirrorProfilePeerMode_SmTransition(void)
{
    switch ((MirrorProfilePeerMode_GetTargetState()))
    {
    case MIRROR_PROFILE_PEER_MODE_STATE_SNIFF:
        if (MirrorProfilePeerMode_GetState() == MIRROR_PROFILE_PEER_MODE_STATE_ACTIVE)
        {
            return MIRROR_PROFILE_PEER_MODE_STATE_ENTER_SNIFF;
        }
        break;

    case MIRROR_PROFILE_PEER_MODE_STATE_ACTIVE:
        if (MirrorProfilePeerMode_GetState() == MIRROR_PROFILE_PEER_MODE_STATE_SNIFF)
        {
            return MIRROR_PROFILE_PEER_MODE_STATE_EXIT_SNIFF;
        }
        break;

    default:
        Panic();
        break;
    }
    return MirrorProfilePeerMode_GetState();
}

/*! \brief Kick the state machine to transition to a new state if required */
static void mirrorProfilePeerMode_SmKick(void)
{
    /* Only allow when in steady state. */
    if (mirrorProfilePeerMode_IsInSteadyState() && MirrorProfile_IsAudioSyncL2capConnected())
    {
        mirror_profile_peer_mode_state_t next = mirrorProfilePeerMode_SmTransition();

        if (next == MIRROR_PROFILE_PEER_MODE_STATE_EXIT_SNIFF)
        {
            if (MirrorProfile_GetState() != MIRROR_PROFILE_STATE_ACL_CONNECTED)
            {
                /* Only allow transition to active mode from base ACL_CONNECTED
                   state */
                return;
            }
        }

        if (next != MirrorProfilePeerMode_GetState())
        {
            mirrorProfilePeerMode_SetState(next);
        }
    }
}


/*! \brief Set a new state.
    \param new_state The new state to set.
*/
static void mirrorProfilePeerMode_SetState(mirror_profile_peer_mode_state_t new_state)
{
    mirror_profile_peer_mode_state_t current_state = MirrorProfilePeerMode_GetState();

    DEBUG_LOG("mirrorProfilePeerMode_SetState %d->%d", current_state, new_state);

    // Handle exiting states
    switch (current_state)
    {
        case MIRROR_PROFILE_PEER_MODE_STATE_ENTER_SNIFF:
            break;
        case MIRROR_PROFILE_PEER_MODE_STATE_SNIFF:
            break;
        case MIRROR_PROFILE_PEER_MODE_STATE_EXIT_SNIFF:
            break;
        case MIRROR_PROFILE_PEER_MODE_STATE_ACTIVE:
            break;
        default:
            Panic();
            break;
    }

    MirrorProfilePeerMode_SetState(new_state);

    // Handle entering states
    switch (new_state)
    {
        case MIRROR_PROFILE_PEER_MODE_STATE_ENTER_SNIFF:
            MirrorProfile_UpdatePeerLinkPolicy(lp_sniff);
            break;
        case MIRROR_PROFILE_PEER_MODE_STATE_SNIFF:
            break;
        case MIRROR_PROFILE_PEER_MODE_STATE_EXIT_SNIFF:
            MirrorProfile_UpdatePeerLinkPolicy(lp_active);
            break;
        case MIRROR_PROFILE_PEER_MODE_STATE_ACTIVE:
            break;
        default:
            Panic();
            break;
    }

    if (MirrorProfile_IsPrimary())
    {
        mirrorProfilePeerMode_SmKick();
        MirrorProfile_SmKick();
    }
}

bool mirrorProfilePeerMode_IsInSteadyState(void)
{
    switch (MirrorProfilePeerMode_GetState())
    {
        case MIRROR_PROFILE_PEER_MODE_STATE_ACTIVE:
        case MIRROR_PROFILE_PEER_MODE_STATE_SNIFF:
            return TRUE;
        default:
            return FALSE;
    }
}

bool mirrorProfilePeerMode_SetTargetState(mirror_profile_peer_mode_state_t target)
{
    DEBUG_LOG_INFO("mirrorProfilePeerMode_SetTargetState 0x%x", target);
    MirrorProfilePeerMode_SetTargetState(target);

    if (MirrorProfilePeerMode_GetState() != target)
    {
        mirrorProfilePeerMode_SmKick();
        return FALSE;
    }
    return TRUE;

}

static void mirrorProfilePeerMode_HandleDmModeChangeEvent(CL_DM_MODE_CHANGE_EVENT_T *event)
{
    if (appDeviceIsPeer(&event->bd_addr))
    {
        DEBUG_LOG("mirrorProfile_HandleDmModeChangeEvent state 0x%x/0x%x, mode 0x%x", 
            MirrorProfile_GetState(), MirrorProfilePeerMode_GetState(), event->mode);

        mirror_profile_peer_mode_state_t new_state = (event->mode == lp_active) ?
            MIRROR_PROFILE_PEER_MODE_STATE_ACTIVE :
            MIRROR_PROFILE_PEER_MODE_STATE_SNIFF;

        mirrorProfilePeerMode_SetState(new_state);

        if (MirrorProfile_IsPrimary())
        {
            mirrorProfilePeerMode_SmKick();
        }
        else
        {
            /* As secondary, ensure the target state is tracking the state
               controlled by the primary */
            MirrorProfilePeerMode_SetTargetState(new_state);
        }

        /* The link exiting sniff indicates impending audio activity, so start
           the DSP. */
        if (event->mode == lp_active)
        {
            appKymeraProspectiveDspPowerOn();
        }

        MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_IDLE_PEER_ENTER_SNIFF);
    }
}

void mirrorProfile_HandleIdlePeerEnterSniff(void)
{
    MirrorProfile_PeerLinkPolicyInit();
    mirrorProfilePeerMode_SetTargetState(MIRROR_PROFILE_PEER_MODE_STATE_SNIFF);
}

#endif /* INCLUDE_MIRRORING */
