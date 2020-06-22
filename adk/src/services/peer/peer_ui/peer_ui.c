/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_ui.c
\brief      Intercept UI input messages, add the delay and re-inject locally for 
            synchronisation of UI input messages such as prompts.
*/

#include "peer_ui.h"
#include "peer_ui_typedef.h"
#include "peer_ui_marshal_typedef.h"

#include "logging.h"
#include <av.h>
#include <bt_device.h>
#include <connection_manager.h>
#include <hfp_profile.h>
#include <mirror_profile.h>
#include "peer_signalling.h"
#include "system_clock.h"
#include "ui.h"
#include <ui_prompts.h>
#include "peer_ui_anc.h"
#include "peer_ui_leakthrough.h"
#include "anc_state_manager.h"
#include "aec_leakthrough.h"

/* system includes */
#include <stdlib.h>
#include <panic.h>

/*! \brief Peer UI task data structure. */
typedef struct
{
    /*! Peer UI task */
    TaskData task;

} peer_ui_task_data_t;

/*! Instance of the peer Ui. */
peer_ui_task_data_t peer_ui;

#define peerUi_GetTaskData()        (&peer_ui)
#define peerUi_GetTask()            (&peer_ui.task)

/*! The default UI interceptor function */
static inject_ui_input ui_func_ptr_to_return = NULL;

static bool peerUi_IsSynchronisedAudioIndication(ui_indication_type_t ind_type)
{
    return ind_type == ui_indication_type_audio_prompt || ind_type == ui_indication_type_audio_tone;
}

static uint32 peerUi_GetPeerRelayDelayBasedOnSystemContext(void)
{
    uint32 peerRelayDelay_usecs = 0;
    if (MirrorProfile_IsConnected())
    {
        peerRelayDelay_usecs = MirrorProfile_GetExpectedPeerLinkTransmissionTime();
    }
    else
    {
        if (appAvIsStreaming() || appHfpIsScoActive())
        {
            peerRelayDelay_usecs = 5*US_PER_MS;
        }
        else
        {
            uint16 sniff_interval_slots = 0;
            tp_bdaddr peer_addr = {0};

            PanicFalse(appDeviceGetPeerBdAddr(&peer_addr.taddr.addr));
            ConManagerGetSniffInterval(&peer_addr, &sniff_interval_slots);

            if (sniff_interval_slots)
            {
                peerRelayDelay_usecs = SNIFF_INTERVAL_US(sniff_interval_slots);
            }
            else
            {
                peerRelayDelay_usecs = 20*US_PER_MS;
            }
        }
    }
    return peerRelayDelay_usecs;
}

/*! \brief Sends the message to secondary earbud. */
static uint32 peerUi_ForwardUiEventToSecondary(ui_indication_type_t ind_type, uint16 ind_index, uint32 time_to_play)
{
    marshal_rtime_t updated_ttp = time_to_play;
    peer_ui_event_t* msg = PanicUnlessMalloc(sizeof(peer_ui_event_t));

    if (peerUi_IsSynchronisedAudioIndication(ind_type))
    {
        uint32 peerRelayDelay_usecs = peerUi_GetPeerRelayDelayBasedOnSystemContext();
        updated_ttp = rtime_add(time_to_play, peerRelayDelay_usecs);
    }

    msg->indication_type = ind_type;
    msg->indication_index = ind_index;
    msg->timestamp = updated_ttp;

    DEBUG_LOG("peerUi_ForwardUiEventToSecondary ind type=%d index=%d timestamp=%d us", ind_type, ind_index, updated_ttp);
    appPeerSigMarshalledMsgChannelTx(peerUi_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_PEER_UI,
                                     msg, MARSHAL_TYPE_peer_ui_event_t);

    return updated_ttp;
}

/***********************************
 * Marshalled Message TX CFM and RX
 **********************************/

/*! \brief Handle confirmation of transmission of a marshalled message. */
static void peerUi_HandleMarshalledMsgChannelTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm)
{
    peerSigStatus status = cfm->status;
    
    if (peerSigStatusSuccess != status)
    {
        DEBUG_LOG("peerUi_HandleMarshalledMsgChannelTxCfm reports failure status 0x%x(%d)",status,status);
    }
}

static void peerUi_InjectUiEvent(peer_ui_event_t* rcvd)
{
    PanicNull(rcvd);

    ui_indication_type_t ind_type = rcvd->indication_type;
    uint16 ind_index = rcvd->indication_index;
    marshal_rtime_t timestamp = rcvd->timestamp;
    int32 time_left_usecs = 0;

    if (peerUi_IsSynchronisedAudioIndication(ind_type))
    {
        rtime_t now = SystemClockGetTimerTime();

        time_left_usecs = rtime_sub(timestamp, now);

        DEBUG_LOG("peerUi_InjectUiEvent now=%u, ttp=%u, time_left=%d", now, timestamp, time_left_usecs);
    }

    /* Notify UI component of the forwarded UI Event, we only notify when the Secondary has
    time left to handle the event, otherwise there is no hope of synchronisation and we shall
    not play a badly synchronised indication to the user. */
    if(rtime_gt(time_left_usecs, UI_SYNC_IND_AUDIO_SS_FIXED_DELAY) ||
       !peerUi_IsSynchronisedAudioIndication(ind_type))
    {
        Ui_NotifyUiEvent(ind_type, ind_index, timestamp);
    }
}

static void peerUi_InjectUiInput(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    peer_ui_input_t* rcvd = (peer_ui_input_t*)ind->msg;

    PanicNull(rcvd);

    ui_input_t ui_input = rcvd->ui_input;

    if (ID_TO_MSG_GRP(ui_input) == UI_INPUTS_AUDIO_CURATION_MESSAGE_GROUP)
    {
        DEBUG_LOG("UI_INPUTS_AUDIO_CURATION_MESSAGE_GROUP : ANC/Leak-through UI EVENTS");
        peerAnc_HandleMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)ind);
        peerUiLeakthrough_HandleMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)ind);
    }
}

/*! \brief Handle incoming marshalled messages.*/
static void peerUi_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    PanicNull(ind);

    DEBUG_LOG("peerUi_HandleMarshalledMsgChannelRxInd channel %u type %u", ind->channel, ind->type);
    switch (ind->type)
    {
    case MARSHAL_TYPE_peer_ui_input_t:
        peerUi_InjectUiInput(ind);
        break;

    case MARSHAL_TYPE_peer_ui_event_t:
        peerUi_InjectUiEvent((peer_ui_event_t*)ind->msg);
        break;

    default:
        /* Do not expect any other messages*/
        Panic();
        break;
    }

    /* free unmarshalled msg */
    free(ind->msg);
}

static uint32 peerUi_ForwardToPeer(ui_indication_type_t type, uint16 index, uint32 time_to_play)
{
    if (appPeerSigIsConnected())
    {
        /* Peer connection exists so incorporate a transmission delay in the time_to_play and forward it to the Peer device. */
        time_to_play = peerUi_ForwardUiEventToSecondary(type, index, time_to_play);
    }
    return time_to_play;
}

/*! brief Interceptor call back function called by UI module on reception of UI input messages */
static void peerUi_Interceptor_FuncPtr(ui_input_t ui_input, uint32 delay)
{
    switch(ui_input)
    {
        case ui_input_anc_toggle_on_off:
            if(appPeerSigIsConnected())
            {
                /* peer connection exists so need to delay in handling ANC UI input at
                primary EB so need to add delay when to handle the ui_input */
                delay = PEER_ANC_ON_OFF_DELAY_MS;
                /* send ANC ui_input to secondary */
                if(AncStateManager_IsEnabled())
                {
                    ui_input = ui_input_anc_off;
                    DEBUG_LOG("peerUi_Interceptor_FuncPtr : ANC ENABLE");
                    peerAnc_SendAncInputToSecondary(ui_input,peerUi_GetTask());
                }
                else
                {
                    ui_input = ui_input_anc_on;
                    DEBUG_LOG("peerUi_Interceptor_FuncPtr : ANC DISABLE");
                    peerAnc_SendAncInputToSecondary(ui_input,peerUi_GetTask());
                }
            }
            break;
        case ui_input_anc_on:
        case ui_input_anc_off:
        case ui_input_anc_set_mode_1:
        case ui_input_anc_set_mode_2:
        case ui_input_anc_set_mode_3:
        case ui_input_anc_set_mode_4:
        case ui_input_anc_set_mode_5:
        case ui_input_anc_set_mode_6:
        case ui_input_anc_set_mode_7:
        case ui_input_anc_set_mode_8:
        case ui_input_anc_set_mode_9:
        case ui_input_anc_set_mode_10:
        case ui_input_anc_set_next_mode:
        case ui_input_anc_set_leakthrough_gain:
            if(appPeerSigIsConnected())
            {
                /* peer connection exists so need to delay in handling ANC UI input at
                primary EB so need to add delay when to handle the ui_input */
                delay = PEER_ANC_ON_OFF_DELAY_MS;
                /* send ANC ui_input to secondary */
                peerAnc_SendAncInputToSecondary(ui_input,peerUi_GetTask());
            }
            break;

        case ui_input_leakthrough_toggle_on_off:
            if(appPeerSigIsConnected())
            {
                /* peer connection exists so need to delay in handling Leak-through UI input at
                primary EB, adding delay when to handle the ui_input */
                delay = PEER_LEAKTHROUGH_ON_OFF_DELAY_MS;
                /* send Leak-through ui_input to secondary */
                if(AecLeakthrough_IsLeakthroughEnabled())
                {
                    ui_input = ui_input_leakthrough_off;
                    DEBUG_LOG("peerUi_Interceptor_FuncPtr : Disable Leak-through");
                    peerUiLeakthrough_SendLeakthroughInputToSecondary(ui_input, peerUi_GetTask());
                }
                else
                {
                    ui_input = ui_input_leakthrough_on;
                    DEBUG_LOG("peerUi_Interceptor_FuncPtr : Enable Leak-through");
                    peerUiLeakthrough_SendLeakthroughInputToSecondary(ui_input, peerUi_GetTask());
                }
            }
            break;
        case ui_input_leakthrough_on:
        case ui_input_leakthrough_off:
        case ui_input_leakthrough_set_mode_1:
        case ui_input_leakthrough_set_mode_2:
        case ui_input_leakthrough_set_mode_3:
        case ui_input_leakthrough_set_next_mode:
            if(appPeerSigIsConnected())
            {
                /* peer connection exists so need to delay in handling Leak-through UI input at
                primary EB, adding delay when to handle the ui_input */
                delay = PEER_LEAKTHROUGH_ON_OFF_DELAY_MS;
                /* send Leak-through ui_input to secondary */
                peerUiLeakthrough_SendLeakthroughInputToSecondary(ui_input, peerUi_GetTask());
            }
            break;

        default:
            break;
    }

    /* pass ui_input back to UI module */
    ui_func_ptr_to_return(ui_input, delay);
}

/*! brief Register the peer_ui interceptor function pointer with UI module
    to receive all the ui_input messages */
static void peerUi_Register_Interceptor_Func(void)
{
    /* original UI function pointer received */
    ui_func_ptr_to_return = Ui_RegisterUiInputsInterceptor(peerUi_Interceptor_FuncPtr);
}

/*! Peer UI Message Handler. */
static void peerUi_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        /* marshalled messaging */
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            peerUi_HandleMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)message);
            break;

        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            peerUi_HandleMarshalledMsgChannelTxCfm((PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T*)message);
            break;

        default:
            DEBUG_LOG("peerUi_HandleMessage unhandled message id %u", id);
            break;
    }
}

/*! brief Initialise Peer Ui  module */
bool PeerUi_Init(Task init_task)
{
    DEBUG_LOG("PeerUi_Init");
    peer_ui_task_data_t *theTaskData = peerUi_GetTaskData();
    
    theTaskData->task.handler = peerUi_HandleMessage;

    /* Register with peer signalling to use the peer UI msg channel */
    appPeerSigMarshalledMsgChannelTaskRegister(peerUi_GetTask(), 
                                               PEER_SIG_MSG_CHANNEL_PEER_UI,
                                               peer_ui_marshal_type_descriptors,
                                               NUMBER_OF_PEER_UI_MARSHAL_TYPES);

    /* get notification of peer signalling availability to send ui_input messages to peer */
    appPeerSigClientRegister(peerUi_GetTask()); 

    /* register the UI event sniffer function pointer with UI module */
    Ui_RegisterUiEventSniffer(peerUi_ForwardToPeer);

    /* register the peer_ui interceptor function pointer with UI module
    to receive all the ui_inputs messages */
    peerUi_Register_Interceptor_Func();

    UNUSED(init_task);  
    return TRUE;   
}
