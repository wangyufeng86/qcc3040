/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       ama_audio.h
\brief  File consists of function decalration for Amazon Voice Service's audio specific interface
*/
#ifndef AMA_AUDIO_H
#define AMA_AUDIO_H

#include "ama.h"
#include "ama_protocol.h"

typedef enum {
    ama_audio_trigger_tap,
    ama_audio_trigger_press,
    ama_audio_trigger_wake_word
}ama_audio_trigger_t;
/*!
    \brief This triggers Voice Session with AVS
*/
#ifdef INCLUDE_AMA
bool AmaAudio_Start(ama_audio_trigger_t type);
#else
#define AmaAudio_Start(type) FALSE
#endif

/*!
    \brief Stops the voice capture chain
*/
#ifdef INCLUDE_AMA
void AmaAudio_Stop(void);
#else
#define AmaAudio_Stop() ((void)(0))
#endif

/*!
    \brief Stops the voice capture chain
*/
#ifdef INCLUDE_AMA
bool AmaAudio_Provide(const AMA_SPEECH_PROVIDE_IND_T* ind);
#else
#define AmaAudio_Provide(ind) FALSE
#endif

/*!
    \brief Ends the AVS speech session
*/
#ifdef INCLUDE_AMA
void AmaAudio_End(void);
#else
#define AmaAudio_End() ((void)(0))
#endif

/*!
    \brief Suspends the AVS session
*/
#ifdef INCLUDE_AMA
void AmaAudio_Suspend(void);
#else
#define AmaAudio_Suspend() ((void)(0))
#endif

/*!
    \brief Resumes the AVS session
*/
#ifdef INCLUDE_AMA
void AmaAudio_Resume(void);
#else
#define AmaAudio_Resume() ((void)(0))
#endif
#endif /* AMA_AUDIO_H*/

