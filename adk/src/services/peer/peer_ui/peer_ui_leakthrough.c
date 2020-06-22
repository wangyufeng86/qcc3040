/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_ui_leakthrough.c
\brief      Intercept leak-though input messages, add the delay and re-inject locally for
            synchronization of leak-through input messages.
*/

#ifdef ENABLE_AEC_LEAKTHROUGH

#include "logging.h"
#include "peer_signalling.h"
#include "system_clock.h"
#include "ui.h"
#include "peer_ui_leakthrough.h"
#include "peer_ui_typedef.h"
#include "peer_ui_marshal_typedef.h"
/*! system includes */
#include <stdlib.h>
#include <panic.h>
#include <phy_state.h>

#define US_TO_MS(us) ((us) / US_PER_MS)

void peerUiLeakthrough_SendLeakthroughInputToSecondary(ui_input_t ui_input,Task task)
{
    peer_ui_input_t* msg = PanicUnlessMalloc(sizeof(peer_ui_input_t));

    /*! Get the current system time */
    rtime_t now = SystemClockGetTimerTime();

    /*! Add the delay of PEER_LEAKTHROUGH_ON_OFF_DELAY_US to current system time which is sent to secondary,
    this is the time when we want primary and secondary to handle the UI input */
    marshal_rtime_t timestamp = rtime_add(now, PEER_LEAKTHROUGH_ON_OFF_DELAY_US);
    msg->ui_input = ui_input;
    msg->timestamp = timestamp;

    DEBUG_LOG("peerUiLeakthrough_SendLeakthroughInputToSecondary send ui_input (0x%x) with timestamp %d us", ui_input, timestamp);
    appPeerSigMarshalledMsgChannelTx(task,
                                     PEER_SIG_MSG_CHANNEL_PEER_UI,
                                     msg, MARSHAL_TYPE_peer_ui_input_t);
}

void peerUiLeakthrough_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    DEBUG_LOG("peerUiLeakthrough_HandleMarshalledMsgChannelRxInd");

    /* Received message at secondary EB */
    peer_ui_input_t* rcvd = (peer_ui_input_t*)ind->msg;

    PanicNull(rcvd);
    ui_input_t ui_input = rcvd->ui_input;
    marshal_rtime_t timestamp = rcvd->timestamp;

    /* Difference between timestamp (sent by primary by when to handle the UI input) and
    actual system time when UI input is received by secondary */
    int32 delta;

    /* System time when message received by secondary earbud */
    rtime_t now = SystemClockGetTimerTime();

    /* Time left for secondary to handle the leak-through UI input */
    delta = rtime_sub(timestamp, now);

    /* Inject Leakthrough UI input to secondary with the time left, so that it can handle Leak-through UI input,
    we only inject Leak-through UI input if secondary earbud is in OutOfCase */

    if (appPhyStateIsOutOfCase())
    {
        if(rtime_gt(delta, 0))
        {
            DEBUG_LOG("peerUiLeakthrough_HandleMarshalledMsgChannelRxInd send ui_input(0x%x) in %d ms", ui_input, US_TO_MS(delta));
            Ui_InjectUiInputWithDelay(ui_input, US_TO_MS(delta));
        }
        else
        {
            DEBUG_LOG("peerUiLeakthrough_HandleMarshalledMsgChannelRxInd send ui_input(0x%x) in %d ms", ui_input, US_TO_MS(delta));
            Ui_InjectUiInputWithDelay(ui_input, 1);
        }
    }
}

#endif
