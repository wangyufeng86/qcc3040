/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_phy_state.h
\brief      Handling of physical state events in the state proxy.
*/

#ifndef STATE_PROXY_PHY_STATE_H
#define STATE_PROXY_PHY_STATE_H

#include <phy_state.h>

/*! \brief Get phy state for initial state message. */
void stateProxy_GetInitialPhyState(void);

/*! \brief Handle local phy state events.
    \param[in] ind Phy state changed event message.
*/
void stateProxy_HandlePhyStateChangedInd(const PHY_STATE_CHANGED_IND_T* ind);

/*! \brief Handle remote phy state events.
    \param[in] ind Phy state changed event message.
*/
void stateProxy_HandleRemotePhyStateChangedInd(const PHY_STATE_CHANGED_IND_T* ind);

/*! \brief Function to send PHY_STATE_CHANGED_IND message for remote device

    \param[in] phy_state Physical state.
    \param[in] phy_event Physical state event.
*/
void stateProxy_SendRemotePhyStateChangedInd(phyState phy_state, phy_state_event phy_event);

#endif /* STATE_PROXY_PHY_STATE_H */
