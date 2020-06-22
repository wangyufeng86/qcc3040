/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   voice_sources Voice Sources
\ingroup    audio_domain
\brief      The voice sources component provides generic API to control any voice source (like HFP or USB).

The voice source are analogous to the \ref audio_source audio source.

The interfaces are:
 - Audio - getting parameters required to use an voice source in the audio subsystem
 - Volume - controlling volume of typical voice sources
 - Volume Control - controlling volume of voice sources where the volume value is determined by a remote device
 - Observer - currently notify on volume change only
 - Telephony Control - performing telephony related actions

*/

#ifndef VOICE_SOURCES_H_
#define VOICE_SOURCES_H_

#include "voice_sources_list.h"
#include "voice_sources_audio_interface.h"
#include "voice_sources_telephony_control_interface.h"
#include "voice_sources_volume_interface.h"
#include "voice_sources_volume_control_interface.h"
#include "voice_sources_observer_interface.h"
#include "source_param_types.h"

/*\{*/
/*! \brief Initialise the voice sources domain

    \param init_task Unused
 */
bool VoiceSources_Init(Task init_task);

/* Voice Source Audio Interface */

/*! \brief Initialises the voice source audio registry.
 */
void VoiceSources_AudioRegistryInit(void);

/*! \brief Registers an audio interface for an voice source.

    \param source The voice source
    \param interface The voice source audio interface to register
 */
void VoiceSources_RegisterAudioInterface(voice_source_t source, const voice_source_audio_interface_t * interface);

/*! \brief Get the connect parameters for a source using its registered audio interface.

    This may involve allocating memory therefore the complimenting VoiceSources_ReleaseConnectParameters()
    must be called once the connect parameters are no longer required

    \param source The voice source
    \param source_params Pointer to the structure the source is to populate

    \return TRUE if parameters were populated, else FALSE
 */
bool VoiceSources_GetConnectParameters(voice_source_t source, source_defined_params_t * source_params);

/*! \brief Cleanup/free the connect parameters for a source using its registered audio interface.

    The complimentary function to AudioSources_GetConnectParameters()

    \param source The voice source
    \param source_params Pointer to the structure originally populated by the equivalent get function
 */
void VoiceSources_ReleaseConnectParameters(voice_source_t source, source_defined_params_t * source_params);

/*! \brief Get the disconnect parameters for a source using its registered audio interface.

    This may involve allocating memory therefore the complimenting VoiceSources_ReleaseDisconnectParameters()
    must be called once the connect parameters are no longer required

    \param source The voice source
    \param source_params Pointer to the structure the source is to populate

    \return TRUE if parameters were populated, else FALSE
 */
bool VoiceSources_GetDisconnectParameters(voice_source_t source, source_defined_params_t * source_params);

/*! \brief Cleanup/free the disconnect parameters for a source using its registered audio interface.

    The complimentary function to AudioSources_GetConnectParameters()

    \param source The voice source
    \param source_params Pointer to the structure originally populated by the equivalent get function
 */
void VoiceSources_ReleaseDisconnectParameters(voice_source_t source, source_defined_params_t * source_params);

/*! \brief Check to determine if a voice sources audio is available.

    \param source The voice source

    \return TRUE if voice sources audio is available, else FALSE
 */
bool VoiceSources_IsAudioAvailable(voice_source_t source);

/* Volume Interface */

/*! \brief Initialises the voice source volume registry.
 */
void VoiceSources_VolumeRegistryInit(void);

/*! \brief Registers a volume interface for a voice source.

    \param source The voice source
    \param interface The voice source volume interface to register
 */
void VoiceSources_RegisterVolume(voice_source_t source, const voice_source_volume_interface_t * interface);

/*! \brief Get the current volume for a source using its registered volume interface.

    \param source The voice source

    \return The volume of the specified source
 */
volume_t VoiceSources_GetVolume(voice_source_t source);

/*! \brief Set the current volume for a source using its registered volume interface.

    \param source The voice source
    \param volume The new volume to set
 */
void VoiceSources_SetVolume(voice_source_t source, volume_t volume);


/* Volume Control Interface */

/*! \brief Initialises the voice source volume control registry.
 */
void VoiceSources_VolumeControlRegistryInit(void);

/*! \brief Registers a volume control interface for a voice source.

    \param source The voice source
    \param interface The voice source volume control interface to register
 */
void VoiceSources_RegisterVolumeControl(voice_source_t source, const voice_source_volume_control_interface_t * interface);

/*! \brief Checks to see whether a volume control interface has been registered for a given voice source.

    \param source The voice source

    \return TRUE if a volume control has been registered for the specified source, else FALSE
 */
bool VoiceSources_IsVolumeControlRegistered(voice_source_t source);

/*! \brief Calls the volume up function of a sources registered volume control interface.

    \param source The voice source
 */
void VoiceSources_VolumeUp(voice_source_t source);

/*! \brief Calls the volume down function of a sources registered volume control interface.

    \param source The voice source
 */
void VoiceSources_VolumeDown(voice_source_t source);

/*! \brief Sets a specific volume using a sources registered volume control interface.

    \param source The voice source
    \param volume The new volume
 */
void VoiceSources_VolumeSetAbsolute(voice_source_t source, volume_t volume);

/*! \brief Calls the mute function of a sources registered volume control interface.

    \param source The voice source
    \param state The mute state
 */
void VoiceSources_Mute(voice_source_t source, mute_state_t state);


/* Observer Interface */

/*! \brief Initialises the voice source observer registry.
 */
void VoiceSources_ObserverRegistryInit(void);

/*! \brief Registers an observer interface for a voice source.

    \param source The voice source
    \param interface The voice source observer interface to register
 */
void VoiceSources_RegisterObserver(voice_source_t source, const voice_source_observer_interface_t * interface);

/*! \brief Calls the volume observer function of a sources registered observer interface.

    \param source The voice source
    \param origin The origin of the volume change event
    \param volume The new volume
 */
void VoiceSources_OnVolumeChange(voice_source_t source, event_origin_t origin, volume_t volume);


/* Telephony Control Interface */

/*! \brief Initialises the voice source telephony control registry.
 */
void VoiceSources_TelephonyControlRegistryInit(void);

/*! \brief Registers an telephony control interface for a voice source.

    \param source The voice source
    \param interface The voice source telephony control interface to register
 */
void VoiceSources_RegisterTelephonyControlInterface(voice_source_t source, const voice_source_telephony_control_interface_t * interface);

/*! \brief Calls the accept incoming call function of a sources registered telephony control interface.

    \param source The voice source
 */
void VoiceSources_AcceptIncomingCall(voice_source_t source);

/*! \brief Calls the reject incoming call function of a sources registered telephony control interface.

    \param source The voice source
 */
void VoiceSources_RejectIncomingCall(voice_source_t source);

/*! \brief Calls the terminate ongoing call function of a sources registered telephony control interface.

    \param source The voice source
 */
void VoiceSources_TerminateOngoingCall(voice_source_t source);

/*! \brief Transfers the audio of an ongoing call to the AG using its registered telephony control interface.

    \param source The voice source
 */
void VoiceSources_TransferOngoingCallAudioToAg(voice_source_t source);

/*! \brief Transfers the audio of an ongoing call back to self using a sources registered telephony control interface.

    \param source The voice source
 */
void VoiceSources_TransferOngoingCallAudioToSelf(voice_source_t source);

/*! \brief Initiates a call using the given number via a sources registered telephony control interface.

    \param source The voice source
    \param number The phone number to dial
 */
void VoiceSources_InitiateCallUsingNumber(voice_source_t source, phone_number_t number);

/*! \brief Initiates a voice dial with the handsets native voice service via a sources registered telephony control interface.

    \param source The voice source
 */
void VoiceSources_InitiateVoiceDial(voice_source_t source);

/*\}*/

/* Misc Functions */

/*! \brief Gets the currently routed voice source.

    \return the currently routed voice source
 */
voice_source_t VoiceSources_GetRoutedSource(void);

/*! \brief To determine whether voice is currently routed.

\return TRUE if voice routed , else FALSE
 */
bool VoiceSources_IsVoiceRouted(void);

#endif /* VOICE_SOURCES_H_ */
