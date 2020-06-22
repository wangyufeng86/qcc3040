/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to voice sources.
*/

#ifndef VOICE_SOURCES_AUDIO_INTERFACE_H_
#define VOICE_SOURCES_AUDIO_INTERFACE_H_

#include "source_param_types.h"
#include "voice_sources_list.h"

typedef struct
{
    bool (*GetConnectParameters)(voice_source_t source, source_defined_params_t * source_params);
    void (*ReleaseConnectParameters)(voice_source_t source, source_defined_params_t * source_params);
    bool (*GetDisconnectParameters)(voice_source_t source, source_defined_params_t * source_params);
    void (*ReleaseDisconnectParameters)(voice_source_t source, source_defined_params_t * source_params);
    bool (*IsAudioAvailable)(voice_source_t source);
} voice_source_audio_interface_t;

#endif /* VOICE_SOURCES_AUDIO_INTERFACE_H_ */
