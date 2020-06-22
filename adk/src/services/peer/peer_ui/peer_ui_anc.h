/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_anc.h
\brief	    Intercept ANC input messages, add the delay and re-inject locally for
            synchronisation of ANC input messages.
*/

#ifndef PEER_ANC_H_
#define PEER_ANC_H_

#define PEER_ANC_ON_OFF_DELAY_MS  (300U)
#define PEER_ANC_ON_OFF_DELAY_US  (US_PER_MS * PEER_ANC_ON_OFF_DELAY_MS)


#include "peer_signalling.h"

/*! \brief Sends the ANC UI input message to secondary earbud. */

#ifdef ENABLE_ANC
void peerAnc_SendAncInputToSecondary(ui_input_t ui_input,TaskData *task);
#else
#define peerAnc_SendAncInputToSecondary(x,y) ((void)(0))
#endif

/*! \brief Handle incoming ANC related marshalled messages.*/

#ifdef ENABLE_ANC
void peerAnc_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind);
#else
#define peerAnc_HandleMarshalledMsgChannelRxInd(x) ((void)(0))
#endif

#endif /* PEER_ANC_H_ */
