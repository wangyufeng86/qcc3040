/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_phy_state.c
\brief      Handling of physical state events in the state proxy.
*/

/* local includes */
#include "state_proxy.h"
#include "state_proxy_private.h"
#include "state_proxy_marshal_defs.h"
#include "state_proxy_phy_state.h"

/* framework includes */
#include <phy_state.h>

/* system includes */
#include <panic.h>
#include <logging.h>
#include <stdlib.h>
#include <peer_signalling.h>

/* forward declarations */
static void stateProxy_UpdatePhyState(state_proxy_data_t* data, phy_state_event event);

/*! \brief Get phy state for initial state message. */
void stateProxy_GetInitialPhyState(void)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    phyState phy_state = appPhyStateGetState();

    switch (phy_state)
    {
        case PHY_STATE_IN_CASE:
            proxy->local_state->flags.in_case = TRUE;
            proxy->local_state->flags.in_ear = FALSE;
            break;
        case PHY_STATE_OUT_OF_EAR:
            /* fall-through */
        case PHY_STATE_OUT_OF_EAR_AT_REST:
            proxy->local_state->flags.in_case = FALSE;
            proxy->local_state->flags.in_ear = FALSE;
            break;
        case PHY_STATE_IN_EAR:
            proxy->local_state->flags.in_case = FALSE;
            proxy->local_state->flags.in_ear = TRUE;
            break;
        default:
            break;
    }
}

static void stateProxy_HandlePhyStateChangedIndImpl(const PHY_STATE_CHANGED_IND_T* ind,
                                                    state_proxy_source source)
{
    DEBUG_LOG("stateProxy_HandlePhyStateChangedInd source %d state %u event %u ", source, ind->new_state, ind->event);

    /* update local state */
    stateProxy_UpdatePhyState(stateProxy_GetData(source), ind->event);

    /* notify event specific clients */
    stateProxy_MsgStateProxyEventClients(source,
                                         state_proxy_event_type_phystate,
                                         ind);

    if (source == state_proxy_source_local)
    {
        stateProxy_MarshalToConnectedPeer(MARSHAL_TYPE(PHY_STATE_CHANGED_IND_T), ind, sizeof(*ind));
    }
}

/*! \brief Function to send PHY_STATE_CHANGED_IND message for remote device

    \param[in] phy_state Physical state.
    \param[in] phy_event Physical state event.
*/
void stateProxy_SendRemotePhyStateChangedInd(phyState phy_state, phy_state_event phy_event)
{
    DEBUG_LOG("stateProxy_SendRemotePhyStateChangedInd state %d event %u",phy_state, phy_event);
    PHY_STATE_CHANGED_IND_T message;
    message.new_state = phy_state;
    message.event = phy_event;

    stateProxy_HandlePhyStateChangedIndImpl(&message, state_proxy_source_remote);
}

/*! \brief Function to determine physical state transitions up to the current received physical state. Create the
    PHY_STATE_CHANGED_IND message and send all the physical state changes based on corresponding flag status for remote 
    device.

    For an example: When Earbud is put into the case Earbud physical state transition through PHY_STATE_OUT_OF_EAR
    before transitioning to PHY_STATE_IN_CASE. (state transisiton: PHY_STATE_IN_EAR -> PHY_STATE_OUT_OF_EAR ->
    PHY_STATE_IN_CASE). First state change PHY_STATE_OUT_OF_EAR informed to state_proxy.It requests peer siganlling to
    send to peer device. But it gets queued up as device is busy. Now second state change PHY_STATE_IN_CASE comes to
    state_proxy.It requests peer signalling about this too, but before sending to peer_signalling state_proxy removes
    the queued up message (which was PHY_STATE_OUT_OF_EAR state change in this case).So first state change was missed
    which could lead to stale state in state_proxy.

    \param[in] ind Physical state indication
*/
static void stateProxy_SendRemotePhyStateTransitions(const PHY_STATE_CHANGED_IND_T* ind)
{
    /* current physical state and event */
    phyState state = ind->new_state;
    phy_state_event event = ind->event;

    /* remote device data */
    state_proxy_data_t* data = stateProxy_GetData(state_proxy_source_remote);

    /* current status of flags */
    bool in_case   = data->flags.in_case;
    bool in_ear    = data->flags.in_ear;
    bool in_motion = data->flags.in_motion;

    DEBUG_LOG("stateProxy_SendRemotePhyStateTransitions state %d event %u in_case %u in_ear %u in_motion %u",
                   state, event, in_case, in_ear, in_motion);

    switch(state)
    {
        /* State transitions to get PHY_STATE_IN_CASE
        1. IN_EAR -> OUT_OF_EAR -> IN_CASE
        2. OUT_OF_EAR_AT_REST -> OUT_OF_EAR -> IN_CASE
        3. OUT_OF_EAR -> IN_CASE */
        case PHY_STATE_IN_CASE:
            if(in_ear)
            {
                stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_OUT_OF_EAR, phy_state_event_out_of_ear);
            }
    
            if(!in_motion)
            {
                stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_OUT_OF_EAR, phy_state_event_in_motion);
            }
    
            if(!in_case)
            {
                stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_IN_CASE, phy_state_event_in_case);
            }
            break;

        /* State transitions to get PHY_STATE_OUT_OF_EAR
        1. IN_CASE -> OUT_OF_EAR
        2. OUT_OF_EAR_AT_REST -> OUT_OF_EAR
        3. IN_EAR -> OUT_OF_EAR */
        case PHY_STATE_OUT_OF_EAR:
            if(in_case)
            {
                stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_OUT_OF_EAR, phy_state_event_out_of_case);
            }

            if(!in_motion)
            {
                stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_OUT_OF_EAR, phy_state_event_in_motion);
            }

            if(in_ear)
            {
                stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_OUT_OF_EAR, phy_state_event_out_of_ear);
            }
            break;

        /* State transitions to get PHY_STATE_OUT_OF_EAR_AT_REST
        1. IN_CASE -> OUT_OF_EAR -> OUT_OF_EAR_AT_REST
        2. IN_EAR -> OUT_OF_EAR -> OUT_OF_EAR_AT_REST
        3. OUT_OF_EAR -> OUT_OF_EAR_AT_REST */
        case PHY_STATE_OUT_OF_EAR_AT_REST:
            if(in_case)
            {
                stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_OUT_OF_EAR, phy_state_event_out_of_case);
            }

            if(in_ear)
            {
                stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_OUT_OF_EAR, phy_state_event_out_of_ear);
            }

            if(in_motion)
            {
                stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_OUT_OF_EAR_AT_REST, phy_state_event_not_in_motion);
            }
            break;

        /* State transitions to get PHY_STATE_IN_EAR
        1. IN_CASE -> OUT_OF_EAR -> IN_EAR
        2. OUT_OF_EAR_AT_REST -> OUT_OF_EAR -> IN_EAR
        3. OUT_OF_EAR -> IN_EAR */
        case PHY_STATE_IN_EAR:
            if(in_case)
            {
                stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_OUT_OF_EAR, phy_state_event_out_of_case);
            }

            if(!in_motion)
            {
                stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_OUT_OF_EAR, phy_state_event_in_motion);
            }

            if(!in_ear)
            {
                stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_IN_EAR, phy_state_event_in_ear);
            }
            break;

        default:
            break;
    }
}

void stateProxy_HandlePhyStateChangedInd(const PHY_STATE_CHANGED_IND_T* ind)
{
    stateProxy_HandlePhyStateChangedIndImpl(ind, state_proxy_source_local);
}

void stateProxy_HandleRemotePhyStateChangedInd(const PHY_STATE_CHANGED_IND_T* ind)
{
    stateProxy_SendRemotePhyStateTransitions(ind);
}

/*! \brief Helper function to update phystate flags for a local or remote dataset.

    \param[in] data Pointer to local or remote dataset.
    \param[in] event Phy state event.
*/
static void stateProxy_UpdatePhyState(state_proxy_data_t* data, phy_state_event event)
{
    /*! \todo just save the event instead? */
    switch (event)
    {
        case phy_state_event_in_case:
            data->flags.in_case = TRUE;
            break;
        case phy_state_event_out_of_case:
            data->flags.in_case = FALSE;
            break;
        case phy_state_event_in_ear:
            data->flags.in_ear = TRUE;
            break;
        case phy_state_event_out_of_ear:
            data->flags.in_ear = FALSE;
            break;
        case phy_state_event_in_motion:
            data->flags.in_motion = TRUE;
            break;
        case phy_state_event_not_in_motion:
            data->flags.in_motion = FALSE;
            break;
    }
}

