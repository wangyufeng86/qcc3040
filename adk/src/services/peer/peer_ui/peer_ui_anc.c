/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_anc.c
\brief      Intercept ANC input messages, add the delay and re-inject locally for
            synchronisation of ANC input messages.
*/

#ifdef ENABLE_ANC

#include "anc_state_manager.h"
#include "logging.h"
#include "peer_signalling.h"
#include "system_clock.h"
#include "ui.h"
#include "peer_ui_anc.h"
#include "peer_ui_typedef.h"
#include "peer_ui_marshal_typedef.h"
/* system includes */
#include <stdlib.h>
#include <panic.h>
#include <phy_state.h>


#define US_TO_MS(us) ((us) / US_PER_MS)


void peerAnc_SendAncInputToSecondary(ui_input_t ui_input,Task task)
{
    peer_ui_input_t* msg = PanicUnlessMalloc(sizeof(peer_ui_input_t));

    /* get the current system time */
    rtime_t now = SystemClockGetTimerTime();

    /* add the delay of PEER_ANC_ON_OFF_DELAY_MS to current system time which is sent to secondary,
    this is the time when we want primary and secondary to handle the UI input*/
    marshal_rtime_t timestamp = rtime_add(now, PEER_ANC_ON_OFF_DELAY_US);
    msg->ui_input = ui_input;
    msg->timestamp = timestamp;

    if(ui_input == ui_input_anc_set_leakthrough_gain)
    {
        uint8 anc_leakthrough_gain = AncStateManager_GetAncLeakthroughGain();
        msg->data = anc_leakthrough_gain;
    }

    DEBUG_LOG("peerAnc_SendAncInputToSecondary send ui_input (0x%x) with timestamp %d us", ui_input, timestamp);
    appPeerSigMarshalledMsgChannelTx(task,
                                     PEER_SIG_MSG_CHANNEL_PEER_UI,
                                     msg, MARSHAL_TYPE_peer_ui_input_t);
}

void peerAnc_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{

    DEBUG_LOG("peerAnc_HandleMarshalledMsgChannelRxInd");

    /* received message at secondary EB */
    peer_ui_input_t* rcvd = (peer_ui_input_t*)ind->msg;

    PanicNull(rcvd);
    ui_input_t ui_input = rcvd->ui_input;
    marshal_rtime_t timestamp = rcvd->timestamp;

    /* difference between timestamp (sent by primary by when to handle the UI input) and
    actual system time when UI input is received by secondary */
    int32 delta;

    /* system time when message received by secondary earbud */
    rtime_t now = SystemClockGetTimerTime();

    /* time left for secondary to handle the ANC UI input */
    delta = rtime_sub(timestamp, now);

    /* Inject ANC UI input to secondary with the time left, so it can handle ANC UI input,
    we only inject ANC UI input if secondary earbud is in OutOfCase*/

    if (appPhyStateIsOutOfCase())
    {
        if(ui_input == ui_input_anc_set_leakthrough_gain)
        {
            uint8 anc_leakthrough_gain = rcvd->data;
            AncStateManager_StoreAncLeakthroughGain(anc_leakthrough_gain);
        }

        if(rtime_gt(delta, 0))
        {
            DEBUG_LOG("peerAnc_HandleMarshalledMsgChannelRxInd send ui_input(0x%x) in %d ms", ui_input, US_TO_MS(delta));
            Ui_InjectUiInputWithDelay(ui_input, US_TO_MS(delta));
        }
        else
        {
            DEBUG_LOG("peerAnc_HandleMarshalledMsgChannelRxInd send ui_input(0x%x) in %d ms", ui_input, US_TO_MS(delta));
            Ui_InjectUiInputWithDelay(ui_input, 1);
        }
    }

}

#endif
