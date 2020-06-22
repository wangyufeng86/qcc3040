/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   kymera_adaptation Kymera Adaptation
\ingroup    audio_domain
\brief      Adaptation layer providing a generalised API into the world of kymera audio

It is temporary API to allow audio domain components to use earbuds kymera code.

*/

#ifndef KYMERA_ADAPTATION_H_
#define KYMERA_ADAPTATION_H_

#include "source_param_types.h"
#include "volume_types.h"

/*\{*/

typedef struct
{
    audio_source_type_t source_type;
    source_defined_params_t source_params;
} connect_parameters_t;

typedef struct
{
    audio_source_type_t source_type;
    source_defined_params_t source_params;
} disconnect_parameters_t;

typedef struct
{
    audio_source_type_t source_type;
    volume_t volume;
} volume_parameters_t;


/*! \brief Connect the audio.

    \param params Parameters to use when making the connection
 */
void KymeraAdaptation_Connect(connect_parameters_t * params);

/*! \brief Disconnect the audio.

    \param params Parameters to use when making the disconnection
 */
void KymeraAdaptation_Disconnect(disconnect_parameters_t * params);

/*! \brief Sets the volume.

    \param params Parameters to use when setting the volume
 */
void KymeraAdaptation_SetVolume(volume_parameters_t * params);

/*\}*/

#endif /* KYMERA_ADAPTATION_H_ */
