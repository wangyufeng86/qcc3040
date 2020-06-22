/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_anc.h
\brief      State proxy anc event handling.
*/

#ifndef STATE_PROXY_ANC_H
#define STATE_PROXY_ANC_H

#include <anc_state_manager.h>

/*! \brief Get ANC data for initial state message. */
void stateProxy_GetInitialAncData(void);

/*! \brief Handle remote events for ANC data update during reconnect cases. */
void stateProxy_HandleInitialPeerAncData(state_proxy_data_t * new_state);

/*! \brief Handle local events for ANC data update. */
void stateProxy_HandleAncUpdateInd(const ANC_UPDATE_IND_T* anc_data);

/*! \brief Handle remote events for ANC data update. */
void stateProxy_HandleRemoteAncUpdate(const ANC_UPDATE_IND_T* anc_data);

#endif /* STATE_PROXY_ANC_H */
