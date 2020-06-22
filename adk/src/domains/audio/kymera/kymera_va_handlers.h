/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module that implements basic build block functions to handle Voice Assistant related actions
*/

#ifndef KYMERA_VA_HANDLERS_H_
#define KYMERA_VA_HANDLERS_H_

#include "va_audio_types.h"

typedef struct
{
    Task handler;
    const va_audio_wuw_detection_params_t *params;
} wuw_detection_start_t;

/*! \param params Must be valid pointer to va_audio_voice_capture_params_t
*/
void Kymera_CreateMicChainForLiveCapture(const void *params);

/*! \param params Must be valid pointer to wuw_detection_start_t
*/
void Kymera_CreateMicChainForWuw(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_StartMicChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_StopMicChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_DestroyMicChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_ActivateMicChainWuwOutput(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_DeactivateMicChainWuwOutput(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_ActivateMicChainEncodeOutputForLiveCapture(const void *params);

/*! \param params Must be valid pointer to va_audio_wuw_capture_params_t
*/
void Kymera_ActivateMicChainEncodeOutputForWuwCapture(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_DeactivateMicChainEncodeOutput(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_BufferMicChainEncodeOutput(const void *params);


/*! \param params Must be valid pointer to va_audio_voice_capture_params_t
*/
void Kymera_CreateEncodeChainForLiveCapture(const void *params);

/*! \param params Must be valid pointer to va_audio_wuw_capture_params_t
*/
void Kymera_CreateEncodeChainForWuwCapture(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_StartEncodeChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_StopEncodeChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_DestroyEncodeChain(const void *params);


/*! \param params Must be valid pointer to wuw_detection_start_t
*/
void Kymera_CreateWuwChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_StartWuwChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_StopWuwChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_DestroyWuwChain(const void *params);


/*! \param params Ignored (can be NULL)
*/
void Kymera_StartGraphManagerDelegation(const void *params);

/*! \param params Ignored (can be NULL)
*/
void Kymera_StopGraphManagerDelegation(const void *params);


/*! \param params Ignored (can be NULL)
*/
void Kymera_UpdateAudioFrameworkConfig(const void *params);

#endif /* KYMERA_VA_HANDLERS_H_ */
