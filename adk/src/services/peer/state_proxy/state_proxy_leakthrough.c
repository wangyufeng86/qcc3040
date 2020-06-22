/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_leakthrough.c
\brief      State proxy leakthrough event handling.
*/

/* local includes */
#include "state_proxy.h"
#include "state_proxy_private.h"
#include "state_proxy_marshal_defs.h"
#include "state_proxy_leakthrough.h"

/* framework includes */
#include <peer_signalling.h>

/* system includes */
#include <panic.h>
#include <logging.h>
#include <stdlib.h>

/*! \brief Get leakthrough data for initial state message. */
void stateProxy_GetInitialLeakthroughData(void)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    proxy->local_state->flags.leakthrough_state = AecLeakthrough_IsLeakthroughEnabled();
    proxy->local_state->leakthrough_mode = AecLeakthrough_GetMode();
}

/*! \brief Handle remote events for leakthrough data update during reconnect cases. */
void stateProxy_HandleInitialPeerLeakthroughData(state_proxy_data_t * new_state)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();

    /* Update remote device data if local device is a slave; else ignored */
    if(!StateProxy_IsPrimary())
    {
        STATE_PROXY_LEAKTHROUGH_DATA_T leakthrough_data;

        proxy->remote_state->leakthrough_mode = new_state->leakthrough_mode;
        proxy->remote_state->flags.leakthrough_state = new_state->flags.leakthrough_state;

        leakthrough_data.mode = new_state->leakthrough_mode;
        leakthrough_data.state = new_state->flags.leakthrough_state;

        stateProxy_MsgStateProxyEventClients(state_proxy_source_remote,
                                         state_proxy_event_type_leakthrough,
                                         &leakthrough_data);
    }
}

/*! \brief Handle local events for leakthrough data update. */
void stateProxy_HandleLeakthroughUpdateInd(const LEAKTHROUGH_UPDATE_IND_T* leakthrough_data)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    proxy->local_state->flags.leakthrough_state = leakthrough_data->state;
    proxy->local_state->leakthrough_mode = leakthrough_data->mode;

    if(!stateProxy_Paused() && appPeerSigIsConnected())
    {
        void* sync_data;
        marshal_type_t marshal_type = MARSHAL_TYPE(STATE_PROXY_LEAKTHROUGH_DATA_T);

        /* Cancel any pending messages of this type - its more important to send
        the latest state, so cancel any pending messages. */
        appPeerSigMarshalledMsgChannelTxCancelAll(stateProxy_GetTask(),
                                                  PEER_SIG_MSG_CHANNEL_STATE_PROXY,
                                                  marshal_type);

        sync_data = PanicUnlessMalloc(sizeof(*leakthrough_data));
        memcpy(sync_data, leakthrough_data, sizeof(*leakthrough_data));
        appPeerSigMarshalledMsgChannelTx(stateProxy_GetTask(),
                                         PEER_SIG_MSG_CHANNEL_STATE_PROXY,
                                         sync_data, marshal_type);
    }
}

/*! \brief Handle remote events for leakthrough data update. */
void stateProxy_HandleRemoteLeakthroughUpdate(const LEAKTHROUGH_UPDATE_IND_T* leakthrough_data)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    proxy->remote_state->flags.leakthrough_state = leakthrough_data->state;
    proxy->remote_state->leakthrough_mode = leakthrough_data->mode;
}
