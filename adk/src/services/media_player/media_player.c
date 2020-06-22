/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the media player service.
*/

#include "media_player.h"
#include <logging.h>
#include <panic.h>

#include "ui.h"
#include "av.h"
#include "audio_sources.h"
#include "kymera_adaptation.h"

static void mediaPlayer_HandleMessage(Task task, MessageId id, Message message);
static void mediaPlayer_HandleMediaMessage(Task task, MessageId id, Message message);

static const TaskData ui_task = {mediaPlayer_HandleMessage};
static const TaskData media_task = {mediaPlayer_HandleMediaMessage};

/* Ui Inputs in which media player service is interested*/
static const message_group_t ui_inputs[] =
{
    UI_INPUTS_MEDIA_PLAYER_MESSAGE_GROUP
};


static Task mediaPlayer_UiTask(void)
{
  return (Task)&ui_task;
}

static Task mediaPlayer_MediaTask(void)
{
  return (Task)&media_task;
}

static void mediaPlayer_HandleUiInput(MessageId ui_input)
{
    audio_source_t routed_source = AudioSources_GetRoutedSource();

    if(routed_source != audio_source_none)
    {
        switch(ui_input)
        {
            case ui_input_toggle_play_pause:
                AudioSources_PlayPause(routed_source);
                break;

            case ui_input_play:
                AudioSources_Play(routed_source);
                break;

            case ui_input_pause:
                AudioSources_Pause(routed_source);
                break;

            case ui_input_stop_av_connection:
                AudioSources_Stop(routed_source);
                break;

            case ui_input_av_forward:
                AudioSources_Forward(routed_source);
                break;

            case ui_input_av_backward:
                AudioSources_Back(routed_source);
                break;

            case ui_input_av_fast_forward_start:
                AudioSources_FastForward(routed_source, TRUE);
                break;

            case ui_input_fast_forward_stop:
                AudioSources_FastForward(routed_source, FALSE);
                break;

            case ui_input_av_rewind_start:
                AudioSources_FastRewind(routed_source, TRUE);
                break;

            case ui_input_rewind_stop:
                AudioSources_FastRewind(routed_source, FALSE);
                break;

            default:
                break;
        }
    }
}

static void mediaPlayer_ConnectAudio(audio_source_t source)
{
    source_defined_params_t source_params;

    DEBUG_LOG("mediaPlayer_ConnectAudio source=%d", source);

    if(AudioSources_GetConnectParameters(source, &source_params))
    {
        connect_parameters_t connect_params = { .source_type = source_type_audio, .source_params = source_params };

        KymeraAdaptation_Connect(&connect_params);

        AudioSources_ReleaseConnectParameters(source, &source_params);
    }
    else
    {
        DEBUG_LOG("mediaPlayer_ConnectAudio connect_params not valid, can't connect");
    }
}

static void mediaPlayer_DisconnectAudio(audio_source_t source)
{
    source_defined_params_t source_params;

    DEBUG_LOG("mediaPlayer_DisconnectAudio source=%d)", source);

    if(AudioSources_GetDisconnectParameters(source, &source_params))
    {
        disconnect_parameters_t disconnect_params = { .source_type = source_type_audio, .source_params = source_params };

        KymeraAdaptation_Disconnect(&disconnect_params);

        AudioSources_ReleaseDisconnectParameters(source, &source_params);
    }
    else
    {
        Panic();
    }
}

static void mediaPlayer_HandleMediaMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        case AV_A2DP_AUDIO_CONNECTED:
            mediaPlayer_ConnectAudio(((AV_A2DP_AUDIO_CONNECT_MESSAGE_T *)message)->audio_source);
            break;

        case AV_A2DP_AUDIO_DISCONNECTED:
            mediaPlayer_DisconnectAudio(((AV_A2DP_AUDIO_DISCONNECT_MESSAGE_T *)message)->audio_source);
            break;

        default:
            DEBUG_LOG("mediaPlayer_HandleUiInput unknown message id, id=%d", id);
            break;
    }
}

static unsigned mediaPlayer_GetRoutedContext(void)
{
	unsigned context = context_av_disconnected;

    audio_source_t routed_source = AudioSources_GetRoutedSource();

    if(routed_source != audio_source_none)
    {
    	context =  AudioSources_GetSourceContext(routed_source);
    }

    return context;
}

static void mediaPlayer_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    if (isMessageUiInput(id))
    {
        mediaPlayer_HandleUiInput(id);
    }
}


/*! \brief Initialise the media player service
*/
bool MediaPlayer_Init(Task init_task)
{
    UNUSED(init_task);

    /* Register av task call back as ui provider*/
    Ui_RegisterUiProvider(ui_provider_media_player, mediaPlayer_GetRoutedContext);

    Ui_RegisterUiInputConsumer(mediaPlayer_UiTask(), ui_inputs, ARRAY_DIM(ui_inputs));

    appAvStatusClientRegister(mediaPlayer_MediaTask());

    return TRUE;
}

