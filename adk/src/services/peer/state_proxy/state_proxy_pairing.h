/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_pairing.h
\brief      
*/

#ifndef STATE_PROXY_PAIRING_H
#define STATE_PROXY_PAIRING_H

#include <pairing.h>

void stateProxy_GetInitialPairingState(void);
void stateProxy_HandlePairingHandsetActivity(const PAIRING_ACTIVITY_T* pha);
void stateProxy_HandleRemotePairingHandsetActivity(const PAIRING_ACTIVITY_T* pha);
void stateProxy_handleActiveHandsetAddr(const state_proxy_active_handset_addr_t* aha);

#endif /* STATE_PROXY_PAIRING_H */
