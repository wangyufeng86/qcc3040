/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       ama_data.c
\brief  Implementation of ama data 
*/

#include "ama_data.h"
#include <panic.h>

#ifdef INCLUDE_AMA

/* Defines the ama data structure */
typedef struct
{
    ama_state_t state;
    ama_audio_data_t audio_config;
    ama_transport_t  active_transport;
}ama_data_t;

static ama_data_t ama_data;

/***************************************************************************/
void AmaData_SetState(ama_state_t state)
{
    ama_data.state= state;
}

/***************************************************************************/
ama_state_t AmaData_GetState(void)
{
    return  ama_data.state;
}

/***************************************************************************/
bool AmaData_IsReadyToSendStartSpeech(void)
{
   return (AmaData_GetState() == ama_state_idle);
}

/***************************************************************************/
bool AmaData_IsSendingVoiceData(void)
{
   return (AmaData_GetState() == ama_state_sending);
}

/***************************************************************************/
ama_audio_data_t* AmaData_GetAudioData(void)
{
    return &ama_data.audio_config;
}

/***************************************************************************/
void AmaData_SetAudioData(ama_audio_data_t *config)
{
    ama_data.audio_config = *config;
}

/***************************************************************************/
void AmaData_SetActiveTransport(ama_transport_t type)
{
    ama_data.active_transport = type;
}

/***************************************************************************/
ama_transport_t AmaData_GetActiveTransport(void)
{
    return ama_data.active_transport;
}

#endif /* INCLUDE_AMA */

