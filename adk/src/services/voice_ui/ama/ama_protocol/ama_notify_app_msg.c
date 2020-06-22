/*****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.

FILE NAME
    ama_notify_app.c

*/

#include "ama_private.h"
#include "ama_protocol.h"
#include "logging.h"
#include "panic.h"

void AmaNotifyAppMsg_TransportSwitch(ama_transport_t transport)
{
    MAKE_AMA_MESSAGE(AMA_SWITCH_TRANSPORT_IND);

    DEBUG_LOG(" AmaNotifyAppMsg_TransportSwitch %d", transport );
    message->transport = transport;
    AmaProtocol_SendAppMsg(AMA_SWITCH_TRANSPORT_IND, message);
}

void AmaNotifyAppMsg_StateMsg(ama_speech_state_t state)
{
    MAKE_AMA_MESSAGE(AMA_SPEECH_STATE_IND);
    message->speech_state = state;
    AmaProtocol_SendAppMsg(AMA_SPEECH_STATE_IND, message);
}

void AmaNotifyAppMsg_StopSpeechMsg(void)
{
    AmaProtocol_SendAppMsg(AMA_SPEECH_STOP_IND, NULL);
}

void AmaNotifyAppMsg_ProvideSpeechMsg(uint32 dailog_id)
{
    MAKE_AMA_MESSAGE(AMA_SPEECH_PROVIDE_IND);
    message->dailog_id = dailog_id;
    AmaProtocol_SendAppMsg(AMA_SPEECH_PROVIDE_IND, message);
}

void AmaNotifyAppMsg_SynchronizeSettingMsg(void)
{
    AmaProtocol_SendAppMsg(AMA_SYNCHRONIZE_SETTING_IND, NULL);
}

void AmaNotifyAppMsg_ControlPktMsg(uint8* pkt, uint16 pkt_size)
{
    MAKE_AMA_MESSAGE_WITH_LEN(AMA_SEND_PKT_IND, pkt_size);
    memmove(message->packet, pkt, pkt_size);
    message->pkt_size = pkt_size;
    AmaProtocol_SendAppMsg(AMA_SEND_PKT_IND, message);
}

