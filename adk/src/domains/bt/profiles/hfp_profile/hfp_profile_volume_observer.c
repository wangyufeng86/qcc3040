/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   hfp_profile
\brief      The voice source observer interface implementation for HFP sources
*/

#include "hfp_profile_volume_observer.h"
#include "hfp_profile.h"
#include "hfp_profile_voice_source_device_mapping.h"
#include "scofwd_profile.h"

static void hfpProfile_OnVolumeChange(voice_source_t source, event_origin_t origin, volume_t volume);

static const voice_source_observer_interface_t voice_source_observer_hfp =
{
    .OnVolumeChange = hfpProfile_OnVolumeChange,
};

static void hfpProfile_OnVolumeChange(voice_source_t source, event_origin_t origin, volume_t volume)
{
    if(origin != event_origin_external && appHfpIsScoActive())
    {
        HfpVolumeSyncSpeakerGainRequest(HfpProfile_VoiceSourceDeviceMappingGetIndexForSource(source), (uint8 *)&volume.value);
    }
    if(origin != event_origin_peer)
    {
        appHfpVolumeNotifyClients(appGetHfp()->volume);
    }
}

const voice_source_observer_interface_t * HfpProfile_GetVoiceSourceObserverInterface(void)
{
    return &voice_source_observer_hfp;
}




