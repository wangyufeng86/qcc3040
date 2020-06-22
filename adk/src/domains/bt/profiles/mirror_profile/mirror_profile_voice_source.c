/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      The voice source interface implementation for Mirror Profile
*/

#ifdef INCLUDE_MIRRORING

#include "mirror_profile_private.h"
#include "mirror_profile_voice_source.h"
#include "source_param_types.h"
#include "voice_sources.h"
#include "kymera_adaptation_audio_protected.h"
#include "kymera_adaptation.h"
#include "volume_system.h"

#include <stdlib.h>
#include <panic.h>

static bool mirrorProfile_IsAudioActive(voice_source_t source);

static const voice_source_audio_interface_t mirror_voice_interface =
{
    .GetConnectParameters = NULL,
    .ReleaseConnectParameters = NULL,
    .GetDisconnectParameters = NULL,
    .ReleaseDisconnectParameters = NULL,
    .IsAudioAvailable = mirrorProfile_IsAudioActive,
};

static inline bool mirrorProfile_IsSecondaryAndEscoActive(void)
{
    return MirrorProfile_IsSecondary() && MirrorProfile_IsEscoActive();
}

static bool mirrorProfile_IsAudioActive(voice_source_t source)
{
    bool is_active = FALSE;
    if(source == voice_source_hfp_1 && mirrorProfile_IsSecondaryAndEscoActive())
    {
        is_active = TRUE;
    }
    return is_active;
}

const voice_source_audio_interface_t * MirrorProfile_GetVoiceInterface(void)
{
    return &mirror_voice_interface;
}

void MirrorProfile_StartScoAudio(void)
{
    voice_connect_parameters_t params;
    connect_parameters_t connect_params = { .source_type = source_type_voice,
                                            .source_params = {sizeof(params), &params}};
    mirror_profile_esco_t *esco = MirrorProfile_GetScoState();

    assert(!MirrorProfile_IsPrimary());

    params.audio_sink = StreamScoSink(esco->conn_handle);
    params.codec_mode = esco->codec_mode;
    params.wesco = esco->wesco;
    params.volume = Volume_CalculateOutputVolume(VoiceSources_GetVolume(voice_source_hfp_1));
    params.volume.value = esco->volume;
    params.pre_start_delay = 0;
    params.allow_scofwd = FALSE;
    params.allow_micfwd = FALSE;

    KymeraAdaptation_Connect(&connect_params);
}

void MirrorProfile_StopScoAudio(void)
{
    disconnect_parameters_t disconnect_params = {.source_type = source_type_voice,
                                                 .source_params = {0, NULL}};

    assert(!MirrorProfile_IsPrimary());

    KymeraAdaptation_Disconnect(&disconnect_params);
}

#endif /* INCLUDE_MIRRORING */

