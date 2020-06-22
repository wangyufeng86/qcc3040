/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_ui_leakthrough.h
\brief      Header file of the peer synchronization of leakthrough state component.
*/

#ifndef PEER_UI_LEAKTHROUGH_H_
#define PEER_UI_LEAKTHROUGH_H_

#include "peer_signalling.h"

#define PEER_LEAKTHROUGH_ON_OFF_DELAY_MS  (300U)
#define PEER_LEAKTHROUGH_ON_OFF_DELAY_US  (US_PER_MS * PEER_LEAKTHROUGH_ON_OFF_DELAY_MS)

/*! \brief Sends the Leak-through UI input message to the secondary earbud. */

#ifdef ENABLE_AEC_LEAKTHROUGH
void peerUiLeakthrough_SendLeakthroughInputToSecondary(ui_input_t ui_input,TaskData *task);
#else
#define peerUiLeakthrough_SendLeakthroughInputToSecondary(x,y) ((void)(0))
#endif

/*! \brief Handle incoming Leak-through related marshalled messages.*/

#ifdef ENABLE_AEC_LEAKTHROUGH
void peerUiLeakthrough_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind);
#else
#define peerUiLeakthrough_HandleMarshalledMsgChannelRxInd(x) ((void)(0))
#endif

#endif /* PEER_UI_LEAKTHROUGH_H_ */
