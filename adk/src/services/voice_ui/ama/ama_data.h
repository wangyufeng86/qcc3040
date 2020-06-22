/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       ama_data.h
\brief  File consists of function to get/set data for ama
*/

#ifndef AMA_DATA_H
#define AMA_DATA_H

#include "ama_protocol.h"
#include <csrtypes.h>
#include <sink.h>

#define AMA_OPUS_16KBPS (1<<0)
#define AMA_OPUS_32KBPS (1<<1)

#define MSBC_ENCODER_BITPOOL_VALUE 26


/* Defines different state of ama */
typedef enum {
    ama_state_initialized,
    ama_state_idle,
    ama_state_sending,
    ama_state_last
}ama_state_t;

/* \brief Defines different supported codec of ama */
typedef enum {
    ama_codec_sbc = 0,
    ama_codec_msbc,
    ama_codec_opus,
    ama_codec_last
}ama_codec_t;

/* \brief Defines the ama audio configuration support */
typedef struct
{
    union
    {
        /* msbc codec just requires bitpool size */
        unsigned msbc_bitpool_size;
        unsigned opus_req_kbps;
        /* Any other codec specific configuration */
    }u;
    ama_codec_t codec;
}ama_audio_data_t;

/*!
    \brief Utility function to set the AMA internal state
*/
#ifdef INCLUDE_AMA
void AmaData_SetState(ama_state_t state);
#else
#define AmaData_SetState(state) ((void)(0))
#endif

/*!
    \brief Utility function to get the AMA internal state
*/
#ifdef INCLUDE_AMA
ama_state_t AmaData_GetState(void);
#else
#define AmaData_GetState() ama_state_last
#endif

/*!
    \brief Utility function to check if Ama is ready to start voice session
*/
#ifdef INCLUDE_AMA
bool AmaData_IsReadyToSendStartSpeech(void);
#else
#define AmaData_IsReadyToSendStartSpeech() FALSE
#endif

/*!
    \brief Utility function to check if Ama is in correct state to send voice data
*/
#ifdef INCLUDE_AMA
bool AmaData_IsSendingVoiceData(void);
#else 
#define AmaData_IsSendingVoiceData() ((void)(0))
#endif

/*!
    \brief Utility function to get the Ama audio configuration data
*/
#ifdef INCLUDE_AMA
ama_audio_data_t* AmaData_GetAudioData(void);
#else
#define AmaData_GetAudioData() NULL
#endif

/*!
    \brief Utility function to set the Ama audio configuration
*/
#ifdef INCLUDE_AMA
void AmaData_SetAudioData(ama_audio_data_t *config);
#else
#define AmaData_SetAudioData(config) ((void)(0))
#endif

#ifdef INCLUDE_AMA
void AmaData_SetActiveTransport(ama_transport_t type);
#else
#define AmaData_SetActiveTransport(type) ((void)(0))
#endif

#ifdef INCLUDE_AMA
ama_transport_t AmaData_GetActiveTransport(void);
#else
#define AmaData_GetActiveTransport() ama_transport_none
#endif

#endif /* AMA_DATA_H */

