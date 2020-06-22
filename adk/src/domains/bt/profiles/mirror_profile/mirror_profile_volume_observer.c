/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      The audio source observer interface implementation provided by Mirror Profile
*/

#ifdef INCLUDE_MIRRORING

#include "mirror_profile_private.h"
#include "mirror_profile_signalling.h"
#include "mirror_profile_volume_observer.h"

static void mirrorProfile_NotifyAbsoluteVolume(audio_source_t source, event_origin_t origin, volume_t volume);

static const audio_source_observer_interface_t mirror_observer_interface =
{
    .OnVolumeChange = mirrorProfile_NotifyAbsoluteVolume,
};

static void mirrorProfile_NotifyAbsoluteVolume(audio_source_t source, event_origin_t origin, volume_t volume)
{
    PanicFalse(source == audio_source_a2dp_1);

    if(origin != event_origin_peer)
    {
        MirrorProfile_SendA2dpVolumeToSecondary(volume.value);
    }
}

const audio_source_observer_interface_t * MirrorProfile_GetObserverInterface(void)
{
    return &mirror_observer_interface;
}

#endif /* INCLUDE_MIRRORING */
