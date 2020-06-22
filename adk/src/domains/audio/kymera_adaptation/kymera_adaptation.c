/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Adaptation layer providing a generalised API into the world of kymera audio
*/

#include "kymera_adaptation.h"
#include "kymera_adaptation_audio_protected.h"
#include "kymera_config.h"
#include "kymera_adaptation_voice_protected.h"
#include "kymera.h"
#include "volume_utils.h"

#include <logging.h>

static volume_config_t kymeraAdaptation_GetOutputVolumeConfig(void)
{
    volume_config_t config = { .range = { .min = appConfigMinVolumedB(), .max = appConfigMaxVolumedB() },
                                    .number_of_steps = ((appConfigMaxVolumedB() - appConfigMinVolumedB())+1) };
    return config;
}

static int16 kymeraAdaptation_ConvertVolumeToDb(volume_t volume)
{
    int32 gain = VOLUME_MUTE_IN_DB;
    if(volume.value > volume.config.range.min)
    {
        gain = VolumeUtils_ConvertToVolumeConfig(volume, kymeraAdaptation_GetOutputVolumeConfig());
    }
    return gain;
}

static appKymeraScoMode kymeraAdaptation_ConvertToScoMode(hfp_codec_mode_t codec_mode)
{
    appKymeraScoMode sco_mode = NO_SCO;
    switch(codec_mode)
    {

        case hfp_codec_mode_narrowband:
            sco_mode = SCO_NB;
            break;
        case hfp_codec_mode_wideband:
            sco_mode = SCO_WB;
            break;
        case hfp_codec_mode_ultra_wideband:
            sco_mode = SCO_UWB;
            break;
        case hfp_codec_mode_super_wideband:
            sco_mode = SCO_SWB;
            break;
        case hfp_codec_mode_none:
        default:
            break;
    }
    return sco_mode;
}

static void kymeraAdaptation_ConnectVoice(connect_parameters_t * params)
{
    if(params->source_params.data_length == sizeof(voice_connect_parameters_t))
    {
        voice_connect_parameters_t * voice_params = (voice_connect_parameters_t *)params->source_params.data;
        bool allow_sco_forward = voice_params->allow_scofwd;
        bool allow_mic_forward = voice_params->allow_micfwd;
        appKymeraScoMode sco_mode = kymeraAdaptation_ConvertToScoMode(voice_params->codec_mode);
        int16 volume_in_db = kymeraAdaptation_ConvertVolumeToDb(voice_params->volume);
        appKymeraScoStart(voice_params->audio_sink, sco_mode, &allow_sco_forward, &allow_mic_forward,
                            voice_params->wesco, volume_in_db, voice_params->pre_start_delay);

        if(allow_sco_forward != voice_params->allow_scofwd || allow_mic_forward != voice_params->allow_micfwd)
        {
            DEBUG_LOG("kymeraAdaptation_ConnectVoice: forwarding requested but no chain to support forwarding exists");
            Panic();
        }
    }
    else
    {
        Panic();
    }
}

static void kymeraAdaptation_ConnectAudio(connect_parameters_t * params)
{
    if(params->source_params.data_length == sizeof(audio_connect_parameters_t))
    {
        audio_connect_parameters_t * connect_params = (audio_connect_parameters_t *)params->source_params.data;
        a2dp_codec_settings codec_settings =
        {
            .rate = connect_params->rate, .channel_mode = connect_params->channel_mode,
            .seid = connect_params->seid, .sink = connect_params->sink,
            .codecData =
            {
                .content_protection = connect_params->content_protection, .bitpool = connect_params->bitpool,
                .format = connect_params->format, .packet_size = connect_params->packet_size
            }
        };

        int16 volume_in_db = kymeraAdaptation_ConvertVolumeToDb(connect_params->volume);
        appKymeraA2dpStart(connect_params->client_lock, connect_params->client_lock_mask,
                            &codec_settings, connect_params->max_bitrate,
                            volume_in_db, connect_params->master_pre_start_delay, connect_params->q2q_mode,
                            connect_params->nq2q_ttp);
    }
    else
    {
        Panic();
    }
}

static void kymeraAdaptation_DisconnectVoice(disconnect_parameters_t * params)
{
    if(params->source_params.data_length == 0)
    {
        appKymeraScoStop();
    }
    else
    {
        Panic();
    }
}

static void kymeraAdaptation_DisconnectAudio(disconnect_parameters_t * params)
{
    if(params->source_params.data_length == sizeof(audio_disconnect_parameters_t))
    {
        audio_disconnect_parameters_t * disconnect_params = (audio_disconnect_parameters_t *)params->source_params.data;
        appKymeraA2dpStop(disconnect_params->seid, disconnect_params->source);
    }
    else
    {
        Panic();
    }
}

void KymeraAdaptation_Connect(connect_parameters_t * params)
{
    switch(params->source_type)
    {
        case source_type_voice:
            kymeraAdaptation_ConnectVoice(params);
            break;
        case source_type_audio:
            kymeraAdaptation_ConnectAudio(params);
            break;
        default:
            Panic();
            break;
    }
}

void KymeraAdaptation_Disconnect(disconnect_parameters_t * params)
{
    switch(params->source_type)
    {
        case source_type_voice:
            kymeraAdaptation_DisconnectVoice(params);
            break;
        case source_type_audio:
            kymeraAdaptation_DisconnectAudio(params);
            break;
        default:
            Panic();
            break;
    }
}

void KymeraAdaptation_SetVolume(volume_parameters_t * params)
{
    switch(params->source_type)
    {
        case source_type_voice:
            appKymeraScoSetVolume(kymeraAdaptation_ConvertVolumeToDb(params->volume));
            break;
        case source_type_audio:
            appKymeraA2dpSetVolume(kymeraAdaptation_ConvertVolumeToDb(params->volume));
            break;
        default:
            Panic();
            break;
    }
}


