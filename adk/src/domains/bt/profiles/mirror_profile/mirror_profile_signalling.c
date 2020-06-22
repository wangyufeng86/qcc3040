/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Mirror profile channel for sending messages between Primary & Secondary.
*/

#ifdef INCLUDE_MIRRORING

#include <stdlib.h>

#include <bdaddr.h>
#include <sink.h>

#include "audio_sources.h"
#include "kymera_adaptation_audio_protected.h"
#include "volume_messages.h"
#include "kymera.h"

#include "mirror_profile_signalling.h"
#include "mirror_profile_typedef.h"
#include "mirror_profile_marshal_typedef.h"
#include "mirror_profile_private.h"
#include "mirror_profile_voice_source.h"

/*! The stream context rate is represented as Hz/25 */
#define STREAM_CONTEXT_RATE_MULTIPLIER 25

#define peerSigTx(message, type) appPeerSigMarshalledMsgChannelTx(\
    MirrorProfile_GetTask(), \
    PEER_SIG_MSG_CHANNEL_MIRROR_PROFILE, \
    (message), MARSHAL_TYPE(type))


/*
    Functions sending a mirror_profile channel message
*/

void MirrorProfile_SendHfpVolumeToSecondary(uint8 volume)
{
    mirror_profile_hfp_volume_ind_t* msg = PanicUnlessMalloc(sizeof(*msg));
    msg->volume = volume;
    peerSigTx(msg, mirror_profile_hfp_volume_ind_t);
}

void MirrorProfile_SendHfpCodecAndVolumeToSecondary(hfp_codec_mode_t codec_mode, uint8 volume)
{
    mirror_profile_hfp_codec_and_volume_ind_t* msg = PanicUnlessMalloc(sizeof(*msg));
    msg->codec_mode = codec_mode;
    msg->volume = volume;
    peerSigTx(msg, mirror_profile_hfp_codec_and_volume_ind_t);
}

void MirrorProfile_SendA2dpVolumeToSecondary(uint8 volume)
{
    mirror_profile_a2dp_volume_ind_t* msg = PanicUnlessMalloc(sizeof(*msg));
    msg->volume = volume;
    peerSigTx(msg, mirror_profile_a2dp_volume_ind_t);
}

void MirrorProfile_SendA2dpStreamContextToSecondary(void)
{
    mirror_profile_a2dp_t *a2dp_state = MirrorProfile_GetA2dpState();

    if (appPeerSigIsConnected())
    {
        mirror_profile_stream_context_t *context = PanicUnlessMalloc(sizeof(*context));
        memset(context, 0, sizeof(*context));

        context->cid = a2dp_state->cid;
        context->mtu = a2dp_state->mtu;
        context->seid = a2dp_state->seid;
        context->sample_rate = (uint16)((a2dp_state->sample_rate) / STREAM_CONTEXT_RATE_MULTIPLIER);
        context->content_protection_type = a2dp_state->content_protection ?
                    UINT16_BUILD(AVDTP_CP_TYPE_SCMS_MSB, AVDTP_CP_TYPE_SCMS_LSB) : 0;
        context->volume = AudioSources_GetVolume(audio_source_a2dp_1).value;
        context->audio_state = a2dp_state->state;
        context->q2q_mode = a2dp_state->q2q_mode;

        peerSigTx(context, mirror_profile_stream_context_t);

        MIRROR_LOG("MirrorProfile_SendA2dpStreamContextToSecondary. %d", context->audio_state);
    }
}

static void mirrorProfile_UpdateAudioVolumeFromPeer(int new_volume)
{
    volume_t volume = AudioSources_GetVolume(audio_source_a2dp_1);
    if (volume.value != new_volume)
    {
        Volume_SendAudioSourceVolumeUpdateRequest(audio_source_a2dp_1, event_origin_peer, new_volume);
    }
}

static void mirrorProfile_HandleA2dpStreamContext(const mirror_profile_stream_context_t *context)
{
    mirror_profile_a2dp_t *a2dp_state = MirrorProfile_GetA2dpState();
    MIRROR_LOG("mirrorProfile_HandleA2dpStreamContext a2dp_state:%d ind_state:%d q2q:%d", a2dp_state->state, context->audio_state, context->q2q_mode);
    a2dp_state->cid = context->cid;
    a2dp_state->mtu = context->mtu;
    a2dp_state->seid = context->seid;
    a2dp_state->sample_rate = context->sample_rate * STREAM_CONTEXT_RATE_MULTIPLIER;
    a2dp_state->content_protection = (context->content_protection_type != 0);
    a2dp_state->state = context->audio_state;
    a2dp_state->q2q_mode = context->q2q_mode;
    mirrorProfile_UpdateAudioVolumeFromPeer(context->volume);

    if (a2dp_state->state == AUDIO_SYNC_STATE_CONNECTED ||
        a2dp_state->state == AUDIO_SYNC_STATE_ACTIVE)
    {
        /* Receiving this message from the peer often indicates impending activity.
           Prospectively power on the DSP */
        appKymeraProspectiveDspPowerOn();
    }
}

/*
    Handlers for receiving mirror_profile channel messages.
*/

/* \brief Handle PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND */
void MirrorProfile_HandlePeerSignallingMessage(const PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *ind)
{
    MIRROR_LOG("MirrorProfile_HandlePeerSignallingMessage. Channel 0x%x, type %d", ind->channel, ind->type);

    switch (ind->type)
    {
    case MARSHAL_TYPE(mirror_profile_hfp_volume_ind_t):
        {
            const mirror_profile_hfp_volume_ind_t* vol_ind = (const mirror_profile_hfp_volume_ind_t*)ind->msg;

            MirrorProfile_SetScoVolume(vol_ind->volume);
        }
        break;

    case MARSHAL_TYPE(mirror_profile_hfp_codec_and_volume_ind_t):
        {
            const mirror_profile_hfp_codec_and_volume_ind_t* cv_ind = (const mirror_profile_hfp_codec_and_volume_ind_t*)ind->msg;

            MirrorProfile_SetScoCodec(cv_ind->codec_mode);
            MirrorProfile_SetScoVolume(cv_ind->volume);
            if (MirrorProfile_IsEscoConnected() && MirrorProfile_GetScoState()->codec_mode != hfp_codec_mode_none)
            {
                MirrorProfile_StartScoAudio();
            }
        }
        break;

    case MARSHAL_TYPE(mirror_profile_a2dp_volume_ind_t):
        {
            const mirror_profile_a2dp_volume_ind_t* vol_ind = (const mirror_profile_a2dp_volume_ind_t*)ind->msg;

            MIRROR_LOG("MirrorProfile_HandlePeerSignallingMessage volume %d", vol_ind->volume);
            mirrorProfile_UpdateAudioVolumeFromPeer(vol_ind->volume);
        }
        break;

    case MARSHAL_TYPE(mirror_profile_stream_context_t):
        mirrorProfile_HandleA2dpStreamContext((const mirror_profile_stream_context_t*)ind->msg);
        break;

    default:
        MIRROR_LOG("MirrorProfile_HandlePeerSignallingMessage unhandled type 0x%x", ind->type);
        break;
    }

    /* free unmarshalled msg */
    free(ind->msg);
}

/* \brief Handle PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM */
void MirrorProfile_HandlePeerSignallingMessageTxConfirm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *cfm)
{
    UNUSED(cfm);
}

#endif /* INCLUDE_SCOFWD */
