/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       voice_audio_tuning_mode.h
\defgroup   audio_tuning_mode 
\brief      Interface to the voice assistant audio tuning functionality 
 */

#ifndef VOICE_AUDIO_TUNING_H_
#define VOICE_AUDIO_TUNING_H_

/*@{*/

/*!   
    \brief     This function initialises the audio tuning mode.
    \param   init task
    \return   True if initialisation is sucessfull
*/
bool VoiceAudioTuningMode_Init(Task init_task);
/*@}*/
#endif /*_VOICE_AUDIO_TUNING_H_ */
