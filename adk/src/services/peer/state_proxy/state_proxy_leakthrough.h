/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_leakthrough.h
\brief      State proxy leakthrough event handling.
*/
#ifndef STATE_PROXY_LEAKTHROUGH_H
#define STATE_PROXY_LEAKTHROUGH_H

#include <aec_leakthrough.h>
#include "state_proxy_private.h"

/*! \brief Get leakthrough data for initial state message. */
void stateProxy_GetInitialLeakthroughData(void);

/*! \brief Handle remote events for leakthrough data update during reconnect cases. */
void stateProxy_HandleInitialPeerLeakthroughData(state_proxy_data_t * new_state);

/*! \brief Handle local events for leakthrough data update. */
void stateProxy_HandleLeakthroughUpdateInd(const LEAKTHROUGH_UPDATE_IND_T* leakthrough_data);

/*! \brief Handle remote events for leakthrough data update. */
void stateProxy_HandleRemoteLeakthroughUpdate(const LEAKTHROUGH_UPDATE_IND_T* leakthrough_data);

#endif /* STATE_PROXY_LEAKTHROUGH_H */