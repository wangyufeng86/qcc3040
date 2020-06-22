/*****************************************************************************
Copyright (c) 2018 Qualcomm Technologies International, Ltd.

FILE NAME
    ama_speech_.h

DESCRIPTION
    Functions for high level control of the speech state and interation
    with the phone application
*/

#ifndef __AMA_SPEECH_H_
#define __AMA_SPEECH_H_

#include "ama_protocol.h"
#include "ama_private.h"
#include "speech.pb-c.h"

typedef enum{
    ama_audio_format_pcm_l16_16khz_mono,
    ama_audio_format_opus_16khz_32kbps_cbr_0_20ms,
    ama_audio_format_opus_16khz_16kbps_cbr_0_20ms,
    ama_audio_format_msbc
}ama_audio_format_t;

/***************************************************************************
DESCRIPTION
    Set the Audio Source, format, profile, initiator and dailog id to default values
*/
void AmaSpeech_SetToDefault(void);

/***************************************************************************
DESCRIPTION
    Gets the audio source parameter that will be passed to the phone
    when speech is started.
*/
AudioSource AmaSpeech_GetAudioSource(void);

/***************************************************************************
DESCRIPTION
    Gets the audio format parameter that will be passed to the phone
    when speech is started.
*/
AudioFormat AmaSpeech_GetAudioFormat(void);

/***************************************************************************
DESCRIPTION
    Gets the audio profile parameter that will be passed to the phone
    when speech is started.
*/
AudioProfile AmaSpeech_GetAudioProfile(void);

/***************************************************************************
DESCRIPTION
    Returns the current DialogId
*/
uint32 AmaSpeech_GetCurrentDialogId(void);

/***************************************************************************
DESCRIPTION
    Updates the current DialogId
*/
void AmaSpeech_UpdateProvidedDialogId(uint32 provided_id);

/***************************************************************************
DESCRIPTION
    Starts the Tap to talk session with AVS

RETURN
    If successfully able to send START_SPEECH command then returns TRUE else FALSE
*/
bool AmaSpeech_StartTapToTalk(void);

/***************************************************************************
DESCRIPTION
    Starts the Push to talk session with AVS
    
RETURN
    If successfully able to send START_SPEECH command then returns TRUE else FALSE
*/
bool AmaSpeech_StartPushToTalk(void);
/***************************************************************************
DESCRIPTION
    Stops the AVS session
*/
void AmaSpeech_Stop(void);

/***************************************************************************
DESCRIPTION
    Sends end of speech command
*/
void AmaSpeech_End(void);

/***************************************************************************
DESCRIPTION
    Set the AVS Audio Format
*/
void AmaSpeech_SetAudioFormat(ama_audio_format_t format);

#endif /* __AMA_SPEECH_H_ */
