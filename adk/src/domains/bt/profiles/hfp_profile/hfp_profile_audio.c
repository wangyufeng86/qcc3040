/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      The audio interface implementation for hfp voice sources
*/

#include "hfp_profile_audio.h"
#include "kymera_adaptation.h"
#include "kymera_adaptation_voice_protected.h"
#include "scofwd_profile.h"
#include "mirror_profile.h"
#include "source_param_types.h"
#include "state_proxy.h" // this shouldn't be here
#include "voice_sources.h"
#include "volume_system.h"

#include <stdlib.h>
#include <hfp.h>
#include <panic.h>

/*  This is the number of message sends from scofwd calling
    SendOTAControlMessage(SFWD_OTA_MSG_SETUP);
    to the L2CAP flushing the message to the sink. It comprises:
        SendOTAControlMessage()
            appPeerSigMsgChannelTxRequest() -> PEER_SIG_INTERNAL_MSG_CHANNEL_TX_REQ
        appPeerSigHandleInternalMsgChannelTxRequest()
            appPeerSigL2capRequest()
                appPeerSigL2capSendRequest()
                    SinkFlush()
    For SCO forwarding, the Kymera SCO start is delayed by this number of message
    sends to allow the OTA control message to be flushed before incurring the long,
    blocking kymera start calls.
*/
#define SFWD_SCO_START_MSG_DELAY 2

static bool hfpProfile_GetConnectParameters(voice_source_t source, source_defined_params_t * source_params);
static void hfpProfile_FreeConnectParameters(voice_source_t source, source_defined_params_t * source_params);
static bool hfpProfile_GetDisconnectParameters(voice_source_t source, source_defined_params_t * source_params);
static void hfpProfile_FreeDisconnectParameters(voice_source_t source, source_defined_params_t * source_params);
static bool hfpProfile_IsAudioActive(voice_source_t source);

static const voice_source_audio_interface_t hfp_audio_interface =
{
    .GetConnectParameters = hfpProfile_GetConnectParameters,
    .ReleaseConnectParameters = hfpProfile_FreeConnectParameters,
    .GetDisconnectParameters = hfpProfile_GetDisconnectParameters,
    .ReleaseDisconnectParameters = hfpProfile_FreeDisconnectParameters,
    .IsAudioAvailable = hfpProfile_IsAudioActive,
};

static hfp_codec_mode_t hfpProfile_GetCodecMode(void)
{
    hfpTaskData* hfp = appGetHfp();
    hfp_codec_mode_t codec_mode = (hfp->codec == hfp_wbs_codec_mask_msbc) ?
                                   hfp_codec_mode_wideband : hfp_codec_mode_narrowband;

#ifdef INCLUDE_SWB
    if(hfp->qce_codec_mode_id != CODEC_MODE_ID_UNSUPPORTED)
    {
        switch (hfp->qce_codec_mode_id)
        {
            case aptx_adaptive_64_2_EV3:
            case aptx_adaptive_64_2_EV3_QHS3:
            case aptx_adaptive_64_QHS3:
                codec_mode = hfp_codec_mode_super_wideband;
                break;

            case aptx_adaptive_128_QHS3:
                codec_mode = hfp_codec_mode_ultra_wideband;
                break;

            default:
                Panic();
                break;
        }
    }
#endif
    return codec_mode;
}

static uint8 hfpProfile_GetPreStartDelay(void)
{
    uint8 pre_start_delay = 0;
    if(ScoFwdProfile_IsScoForwardingAllowed())
    {
        if(ScoFwdIsConnected() && StateProxy_IsPeerInEar())
        {
            pre_start_delay = (SFWD_SCO_START_MSG_DELAY - 1);
        }
    }
    return pre_start_delay;
}

static bool hfpProfile_GetConnectParameters(voice_source_t source, source_defined_params_t * source_params)
{
    PanicNull(source_params);
    hfpTaskData* hfp = appGetHfp();
    voice_connect_parameters_t * voice_connect_params = (voice_connect_parameters_t *)PanicNull(malloc(sizeof(voice_connect_parameters_t)));
    memset(voice_connect_params, 0, sizeof(voice_connect_parameters_t));

    voice_connect_params->audio_sink = hfp->sco_sink;
    voice_connect_params->codec_mode = hfpProfile_GetCodecMode();
    voice_connect_params->wesco = hfp->wesco;
    voice_connect_params->tesco = hfp->tesco;
    voice_connect_params->volume = Volume_CalculateOutputVolume(VoiceSources_GetVolume(source));
    voice_connect_params->pre_start_delay = hfpProfile_GetPreStartDelay();
    voice_connect_params->allow_scofwd = ScoFwdProfile_IsScoForwardingAllowed();
    voice_connect_params->allow_micfwd = ScoFwdProfile_IsMicForwardingAllowed();

    source_params->data = (void *)voice_connect_params;
    source_params->data_length = sizeof(voice_connect_parameters_t);

    UNUSED(source);
    return TRUE;
}

static void hfpProfile_FreeConnectParameters(voice_source_t source, source_defined_params_t * source_params)
{
    PanicNull(source_params);
    PanicFalse(source_params->data_length == sizeof(voice_connect_parameters_t));
    if(source_params->data_length)
    {
        free(source_params->data);
        source_params->data = (void *)NULL;
        source_params->data_length = 0;
    }
    UNUSED(source);
}

static bool hfpProfile_GetDisconnectParameters(voice_source_t source, source_defined_params_t * source_params)
{
    PanicNull(source_params);
    source_params->data = (void *)NULL;
    source_params->data_length = 0;

    UNUSED(source);
    return TRUE;
}

static void hfpProfile_FreeDisconnectParameters(voice_source_t source, source_defined_params_t * source_params)
{
    PanicNull(source_params);
    UNUSED(source);
    source_params->data = (void *)NULL;
    source_params->data_length = 0;
}

static bool hfpProfile_IsAudioActive(voice_source_t source)
{
    bool is_active = FALSE;
    if(source == voice_source_hfp_1 && (appHfpIsScoActive() || ScoFwdIsStreaming()))
    {
        is_active = TRUE;
    }
    return is_active;
}

const voice_source_audio_interface_t * HfpProfile_GetAudioInterface(void)
{
    return &hfp_audio_interface;
}

void HfpProfile_StoreConnectParams(const HFP_AUDIO_CONNECT_CFM_T *cfm)
{
    hfpTaskData* hfp = appGetHfp();
    hfp->codec = cfm->codec;
    hfp->wesco = cfm->wesco;
    hfp->tesco = cfm->tesco;

#ifdef INCLUDE_SWB
    hfp->qce_codec_mode_id = cfm->qce_codec_mode_id;
#endif
}


