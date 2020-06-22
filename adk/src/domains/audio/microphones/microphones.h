/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   microphones Microphones
\ingroup    audio_domain
\brief      Component responsible for interacting with microphones and microphone user tracking
*/

#ifndef MICROPHONES_H_
#define MICROPHONES_H_

#include <audio_plugin_if.h>
#include <stream.h>

/*!\{*/

typedef enum
{
    microphone_none,
    microphone_1,
    microphone_2,
    microphone_3,
    microphone_4,
    microphone_5,
    microphone_6,
} microphone_number_t;

/*! \brief Enumeration of microphone user types.

    The user type is used to aid tracking of microphone users and grant access based their priority.
    Normal and high priority users require exclusive access to a microphone resource
    and so cannot co-exist with other exclusive users. 
    Existing high priority user > new high priority user > existing normal priority user > new normal priority user
    
    Non exclusive users can access a microphone resource regardless of whether it is currently in use,
    and they do not block access from other exclusive users.
 */
typedef enum
{
    invalid_user,
    normal_priority_user,
    high_priority_user,
    non_exclusive_user,
} microphone_user_type_t;

/*! \brief Gets a specific microphone configuration.

    \param microphone_number

    \return Pointer to the specified microphone config
 */
const audio_mic_params * Microphones_GetMicrophoneConfig(microphone_number_t microphone_number);

/*! \brief Turn on and configure the specified microphone.

    \param microphone_number
    \param sample_rate the sample rate in Hz
    \param microphone_user_type The user type requesting to turn on the microphone

    \return Source for the configured microphone, NULL if not available
 */
Source Microphones_TurnOnMicrophone(microphone_number_t microphone_number, uint32 sample_rate, microphone_user_type_t microphone_user_type);

/*! \brief Turn off the specified microphone.

    \param microphone_number
    \param microphone_user_type The user type requesting to turn off the microphone
 */
void Microphones_TurnOffMicrophone(microphone_number_t microphone_number, microphone_user_type_t microphone_user_type);

/*! \brief Initialises the internal state of the component.

 */
void Microphones_Init(void);

/*! \brief Returns the maximum number of microphones supported.
    \param null
    \return maximum number of microphones supported
 */
uint8 Microphones_MaxSupported(void);

/*! \brief Get the source for a specific microphone

     \param microphone_number

     \return Source for the microphone, NULL if not turned on
 */
Source Microphones_GetMicrophoneSource(microphone_number_t microphone_number);

/*\}*/

#endif /* MICROPHONES_H_ */
