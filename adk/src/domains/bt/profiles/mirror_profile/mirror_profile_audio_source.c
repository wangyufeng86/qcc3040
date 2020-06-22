/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Mirror profile audio source control.
*/

#ifdef INCLUDE_MIRRORING

#include "mirror_profile_private.h"
#include "mirror_profile_audio_source.h"

#include "kymera_adaptation_audio_protected.h"
#include "kymera_adaptation.h"
#include "volume_system.h"
#include "logging.h"
#include "av.h"

#include <panic.h>
#include <stream.h>
#include <stdlib.h>
#include <sink.h>

static bool mirrorProfile_IsAudioAvailable(audio_source_t source);

static const audio_source_audio_interface_t mirror_audio_interface =
{
    .GetConnectParameters = NULL,
    .ReleaseConnectParameters = NULL,
    .GetDisconnectParameters = NULL,
    .ReleaseDisconnectParameters = NULL,
    .IsAudioAvailable = mirrorProfile_IsAudioAvailable
};

static void mirrorProfile_GetConnectParameters(audio_connect_parameters_t *params)
{
    mirror_profile_a2dp_t *a2dp_state = MirrorProfile_GetA2dpState();

    memset(params, 0, sizeof(*params));
    params->client_lock = MirrorProfile_GetA2dpStartLockAddr();
    params->client_lock_mask = MIRROR_PROFILE_AUDIO_START_LOCK;
    params->volume = AudioSources_GetVolume(audio_source_a2dp_1);
    params->rate = a2dp_state->sample_rate;
    //params->channel_mode = 0;
    params->seid = a2dp_state->seid;
    params->sink = StreamL2capSink(a2dp_state->cid);
    params->content_protection = a2dp_state->content_protection;
    //params->bitpool = 0;
    //params->format = 0;
    params->packet_size = a2dp_state->mtu;
    params->q2q_mode = a2dp_state->q2q_mode;
}

static bool mirrorProfile_IsAudioAvailable(audio_source_t source)
{
    bool is_available = FALSE;

    if(source == audio_source_a2dp_1 && MirrorProfile_IsSecondary())
    {
        switch (MirrorProfile_GetState())
        {
            case MIRROR_PROFILE_STATE_A2DP_CONNECTING:
            case MIRROR_PROFILE_STATE_A2DP_CONNECTED:
                is_available = TRUE;
            break;
            default:
            break;
        }
    }
    return is_available;
}

static void mirrorProfile_GetDisconnectParameters(audio_disconnect_parameters_t *params)
{
    mirror_profile_a2dp_t *a2dp_state = MirrorProfile_GetA2dpState();

    memset(params, 0, sizeof(*params));
    params->source = StreamL2capSource(a2dp_state->cid);
    params->seid = a2dp_state->seid;
}

static void mirrorProfile_StoreAudioConnectParameters(const audio_connect_parameters_t *params)
{
    mirror_profile_a2dp_t *a2dp = MirrorProfile_GetA2dpState();

    a2dp->cid = SinkGetL2capCid(params->sink);
    a2dp->mtu = params->packet_size;
    a2dp->seid = params->seid;
    a2dp->sample_rate = params->rate;
    a2dp->content_protection = params->content_protection;
    a2dp->q2q_mode= params->q2q_mode;

    if(a2dp->cid)
    {
        Source src = StreamSourceFromSink(params->sink);
        PanicFalse(SourceConfigure(src, STREAM_SOURCE_HANDOVER_POLICY, 0x1));
    }

    DEBUG_LOG("mirrorProfile_StoreAudioConnectParameters sink:0x%x cid:0x%x mtu:%d seid:%d rate:%d cp:%d q2q:%d",
            params->sink, a2dp->cid, a2dp->mtu, a2dp->seid, a2dp->sample_rate, a2dp->content_protection, a2dp->q2q_mode);
}

bool MirrorProfile_StoreAudioSourceParameters(audio_source_t source)
{
    source_defined_params_t source_params = {0, NULL};
    audio_connect_parameters_t *audio_params;
    bool parameters_valid = FALSE;

    if(AudioSources_GetConnectParameters(source, &source_params))
    {
        audio_params = source_params.data;

        mirrorProfile_StoreAudioConnectParameters(audio_params);

        AudioSources_ReleaseConnectParameters(source, &source_params);

        parameters_valid = TRUE;
    }
    else
    {
        DEBUG_LOG("MirrorProfile_StoreAudioSourceParameters connect_params not valid");
    }

    return parameters_valid;
}

void MirrorProfile_StartAudio(void)
{
    audio_connect_parameters_t params;
    connect_parameters_t connect_params = { .source_type = source_type_audio,
                                            .source_params = {sizeof(params), &params}};

    /* Dispose the media source in case audio cannot start immediately */
    mirrorProfile_GetConnectParameters(&params);
    StreamConnectDispose(StreamSourceFromSink(params.sink));
    KymeraAdaptation_Connect(&connect_params);
}

void MirrorProfile_StopAudio(void)
{
    audio_disconnect_parameters_t params;
    disconnect_parameters_t disconnect_params = { .source_type = source_type_audio,
                                                  .source_params = {sizeof(params), &params}};

    mirrorProfile_GetDisconnectParameters(&params);
    KymeraAdaptation_Disconnect(&disconnect_params);
}

void MirrorProfile_StartAudioSynchronisation(void)
{
    audio_connect_parameters_t params;
    connect_parameters_t connect_params = { .source_type = source_type_audio,
                                            .source_params = {sizeof(params), &params}};

    memset(&params, 0, sizeof(params));
    /* Use any source SEID, to trigger the kymera start forwarding function.
       Leave other fields as zero as they are not used in this mode */
    params.seid = AV_SEID_SBC_MONO_TWS_SRC;
    params.sink = MirrorProfile_GetAudioSyncL2capState()->link_sink;
    params.client_lock = MirrorProfile_GetA2dpStartLockAddr();
    params.client_lock_mask = MIRROR_PROFILE_AUDIO_START_LOCK;

    KymeraAdaptation_Connect(&connect_params);
}

void MirrorProfile_StopAudioSynchronisation(void)
{
    audio_disconnect_parameters_t params;
    disconnect_parameters_t disconnect_params = { .source_type = source_type_audio,
                                                  .source_params = {sizeof(params), &params}};

    memset(&params, 0, sizeof(params));
    /* Use any source SEID, to trigger the kymera start forwarding function.
       Leave other fields as zero as they are not used in this mode */
    params.seid = AV_SEID_SBC_MONO_TWS_SRC;
    params.source = MirrorProfile_GetAudioSyncL2capState()->link_source;

    KymeraAdaptation_Disconnect(&disconnect_params);
}

const audio_source_audio_interface_t * MirrorProfile_GetAudioInterface(void)
{
    return &mirror_audio_interface;
}

#endif /* INCLUDE_MIRRORING */
