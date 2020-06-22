/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   audio_sources Audio Sources
\ingroup    audio_domain
\brief      The audio sources component provides generic API to control any audio source.

The audio sources component allows multiple components to register its implementation of one or more interfaces.
Then the component using audio sources can 'route' its call to the specific implementation using audio_source_t source parameter.
In effect component using audio sources and in fact audio sources itself doesn't depend on the code implementing paricular audio source.

Each of interfaces works independently from others.
As such any combination of interfaces can be implemented and registered by a component.

Typical example would be A2DP profile in BT domain implementing the audio and volume interfaces,
AVRCP profile implementing media control and observer (for absolute volume) interfaces,
and Media Player service using the audio sources component to control it.

The interfaces are:
 - Audio - getting parameters required to use an audio source in the audio subsystem
 - Media Control - controlling playback of audio, like pause, fast forward
 - Volume - controlling volume of typical audio sources
 - Volume Control - controlling volume of audio sources where the volume value is determined by a remote device
 - Observer - currently notify on volume change only, and it is used to implement AVRCP absolute volume

*/

#ifndef AUDIO_SOURCES_H_
#define AUDIO_SOURCES_H_

#include "audio_sources_list.h"
#include "audio_sources_audio_interface.h"
#include "audio_sources_media_control_interface.h"
#include "audio_sources_volume_interface.h"
#include "audio_sources_volume_control_interface.h"
#include "audio_sources_observer_interface.h"
#include "source_param_types.h"

/*\{*/
/*! \brief Initialise the audio sources domain

    \param init_task Unused
 */
bool AudioSources_Init(Task init_task);

/*! \brief Get the current context of source

    \param source The source to get the context of.

    \return The context of source
 */
unsigned AudioSources_GetSourceContext(audio_source_t source);

/* Audio Interface */

/*! \brief Registers an audio interface for an audio source.

    \param source The audio source
    \param interface The audio source audio interface to register
 */
void AudioSources_RegisterAudioInterface(audio_source_t source, const audio_source_audio_interface_t * interface);

/*! \brief Get the connect parameters for a source using its registered audio interface.

    This may involve allocating memory therefore the complimenting AudioSources_ReleaseConnectParameters()
    must be called once the connect parameters are no longer required

    \param source The audio source
    \param source_params Pointer to the structure the source is to populate

    \return TRUE if parameters were populated, else FALSE
 */
bool AudioSources_GetConnectParameters(audio_source_t source, source_defined_params_t * source_params);

/*! \brief Cleanup/free the connect parameters for a source using its registered audio interface.

    The complimentary function to AudioSources_GetConnectParameters()

    \param source The audio source
    \param source_params Pointer to the structure originally populated by the equivalent get function
 */
void AudioSources_ReleaseConnectParameters(audio_source_t source, source_defined_params_t * source_params);

/*! \brief Get the disconnect parameters for a source using its registered audio interface.

    This may involve allocating memory therefore the complimenting AudioSources_ReleaseDisconnectParameters()
    must be called once the connect parameters are no longer required

    \param source The audio source
    \param source_params Pointer to the structure the source is to populate

    \return TRUE if parameters were populated, else FALSE
 */
bool AudioSources_GetDisconnectParameters(audio_source_t source, source_defined_params_t * source_params);

/*! \brief Cleanup/free the disconnect parameters for a source using its registered audio interface.

    The complimentary function to AudioSources_GetDisconnectParameters()

    \param source The audio source
    \param source_params Pointer to the structure originally populated by the equivalent get function
 */
void AudioSources_ReleaseDisconnectParameters(audio_source_t source, source_defined_params_t * source_params);

/*! \brief Check to determine if a audio sources audio is available.

    \param source The audio source

    \return TRUE if audio sources audio is available, else FALSE
 */
bool AudioSources_IsAudioAvailable(audio_source_t source);

/* Media control Interface */
void AudioSources_RegisterMediaControlInterface(audio_source_t source, const media_control_interface_t * interface);
void AudioSources_Play(audio_source_t source);
void AudioSources_Pause(audio_source_t source);
void AudioSources_PlayPause(audio_source_t source);
void AudioSources_Stop(audio_source_t source);
void AudioSources_Forward(audio_source_t source);
void AudioSources_Back(audio_source_t source);
void AudioSources_FastForward(audio_source_t source, bool state);
void AudioSources_FastRewind(audio_source_t source, bool state);
void AudioSources_NextGroup(audio_source_t source);
void AudioSources_PreviousGroup(audio_source_t source);
void AudioSources_Shuffle(audio_source_t source, shuffle_state_t state);
void AudioSources_Repeat(audio_source_t source, repeat_state_t state);

/* Volume Interface */

/*! \brief Registers a volume interface for an audio source.

    \param source The audio source
    \param interface The audio source volume interface to register
 */
void AudioSources_RegisterVolume(audio_source_t source, const audio_source_volume_interface_t * interface);

/*! \brief Get the current volume for a source using its registered volume interface.

    \param source The audio source

    \return The volume of the specified audio source
 */
volume_t AudioSources_GetVolume(audio_source_t source);

/*! \brief Set the current volume for a source using its registered volume interface.

    \param source The audio source
    \param volume The new volume to set
 */
void AudioSources_SetVolume(audio_source_t source, volume_t volume);


/* Volume Control Interface */

/*! \brief Registers a volume control interface for an audio source.

    \param source The audio source
    \param interface The audio source volume control interface to register
 */
void AudioSources_RegisterVolumeControl(audio_source_t source, const audio_source_volume_control_interface_t * interface);

/*! \brief Checks to see whether a volume control interface has been registered for a given audio source.

    \param source The audio source

    \return TRUE if a volume control has been registered for the specified source, else FALSE
 */
bool AudioSources_IsVolumeControlRegistered(audio_source_t source);

/*! \brief Calls the volume up function of a sources registered volume control interface.

    \param source The audio source
 */
void AudioSources_VolumeUp(audio_source_t source);

/*! \brief Calls the volume down function of a sources registered volume control interface.

    \param source The audio source
 */
void AudioSources_VolumeDown(audio_source_t source);

/*! \brief Sets a specific volume using a sources registered volume control interface.

    \param source The audio source
    \param volume The new volume
 */
void AudioSources_VolumeSetAbsolute(audio_source_t source, volume_t volume);

/*! \brief Calls the mute function of a sources registered volume control interface.

    \param source The audio source
    \param state The mute state
 */
void AudioSources_Mute(audio_source_t source, mute_state_t state);


/* Observer Interface */

/*! \brief Registers an observer interface for an audio source.

    \param source The audio source
    \param interface The audio source observer interface to register
 */
void AudioSources_RegisterObserver(audio_source_t source, const audio_source_observer_interface_t * interface);

/*! \brief Calls the volume observer function of a sources registered observer interface.

    \param source The audio source
    \param origin The origin of the volume change event
    \param volume The new volume
 */
void AudioSources_OnVolumeChange(audio_source_t source, event_origin_t origin, volume_t volume);

/*\}*/


/* Misc Functions */

/*! \brief Gets the currently routed audio source.

    \return the currently routed audio source
 */
audio_source_t AudioSources_GetRoutedSource(void);

#endif /* AUDIO_SOURCES_H_ */
