/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_anc.c
\brief      State proxy anc event handling.
*/

/* local includes */
#include "state_proxy.h"
#include "state_proxy_private.h"
#include "state_proxy_marshal_defs.h"
#include "state_proxy_anc.h"

/* framework includes */
#include <peer_signalling.h>

/* system includes */
#include <panic.h>
#include <logging.h>
#include <stdlib.h>

/*! \brief Get ANC data for initial state message. */
void stateProxy_GetInitialAncData(void)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    proxy->local_state->flags.anc_state = AncStateManager_IsEnabled();
    proxy->local_state->anc_mode = AncStateManager_GetMode();
    proxy->local_state->anc_leakthrough_gain = AncStateManager_GetAncLeakthroughGain();
}

/*! \brief Handle remote events for ANC data update during reconnect cases. */
void stateProxy_HandleInitialPeerAncData(state_proxy_data_t * new_state)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();

    /* Update remote device data if local device is a slave; else ignored */
    if(!StateProxy_IsPrimary())
    {
       STATE_PROXY_ANC_DATA_T anc_data;

       proxy->remote_state->anc_mode = new_state->anc_mode;
       proxy->remote_state->flags.anc_state = new_state->flags.anc_state;
       proxy->remote_state->anc_leakthrough_gain= new_state->anc_leakthrough_gain;

       anc_data.mode = new_state->anc_mode;
       anc_data.state = new_state->flags.anc_state;
       anc_data.anc_leakthrough_gain = new_state->anc_leakthrough_gain;

    stateProxy_MsgStateProxyEventClients(state_proxy_source_remote,
                                         state_proxy_event_type_anc,
                                         &anc_data);
    }
}

/*! \brief Handle local events for ANC data update. */
void stateProxy_HandleAncUpdateInd(const ANC_UPDATE_IND_T* anc_data)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    proxy->local_state->flags.anc_state = anc_data->state;
    proxy->local_state->anc_mode = anc_data->mode;
    proxy->local_state->anc_leakthrough_gain = anc_data->anc_leakthrough_gain;

    if(!stateProxy_Paused() && appPeerSigIsConnected())
    {
        void* copy;
        marshal_type_t marshal_type = MARSHAL_TYPE(STATE_PROXY_ANC_DATA_T);

        /* Cancel any pending messages of this type - its more important to send
        the latest state, so cancel any pending messages. */
        appPeerSigMarshalledMsgChannelTxCancelAll(stateProxy_GetTask(),
                                                  PEER_SIG_MSG_CHANNEL_STATE_PROXY,
                                                  marshal_type);

        copy = PanicUnlessMalloc(sizeof(*anc_data));
        memcpy(copy, anc_data, sizeof(*anc_data));
        appPeerSigMarshalledMsgChannelTx(stateProxy_GetTask(),
                                         PEER_SIG_MSG_CHANNEL_STATE_PROXY,
                                         copy, marshal_type);
    }
}

/*! \brief Handle remote events for ANC data update. */
void stateProxy_HandleRemoteAncUpdate(const ANC_UPDATE_IND_T* anc_data)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    proxy->remote_state->flags.anc_state = anc_data->state;
    proxy->remote_state->anc_mode = anc_data->mode;
    proxy->remote_state->anc_leakthrough_gain = anc_data->anc_leakthrough_gain;
}
