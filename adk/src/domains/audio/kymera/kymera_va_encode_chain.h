/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle VA encode chain

*/

#ifndef KYMERA_VA_ENCODE_CHAIN_H_
#define KYMERA_VA_ENCODE_CHAIN_H_

#include "va_audio_types.h"

/*! \brief Parameters used to determine the chain to use/create */
typedef struct
{
    va_audio_codec_t encoder;
} va_encode_chain_params_t;

/*! \brief Parameters used to configure the VA encode chain operators */
typedef struct
{
    const va_audio_encoder_params_t *encoder_params;
} va_encode_chain_op_params_t;

/*! \brief Parameters used to create the VA encode chain */
typedef struct
{
    va_encode_chain_params_t    chain_params;
    va_encode_chain_op_params_t operators_params;
} va_encode_chain_create_params_t;

/*! \brief Create VA encode chain.
           Must be called after VA mic chain is instantiated, since it will connect to it.
    \param params Parameters used to create/configure the chain.
*/
void Kymera_CreateVaEncodeChain(const va_encode_chain_create_params_t *params);
void Kymera_DestroyVaEncodeChain(void);

void Kymera_StartVaEncodeChain(void);
void Kymera_StopVaEncodeChain(void);

Source Kymera_GetVaEncodeChainOutput(void);

#endif /* KYMERA_VA_ENCODE_CHAIN_H_ */
