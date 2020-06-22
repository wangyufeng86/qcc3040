/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle VA encode chain

*/

#include "kymera_va_encode_chain.h"
#include "kymera_private.h"
#include "kymera_va_mic_chain.h"
#include "kymera_va_common.h"
#include "kymera_chain_roles.h"
#include <operators.h>

#define AUDIO_FRAME_VA_DATA_LENGTH (9)

typedef struct
{
    va_audio_codec_t      encoder;
    const chain_config_t *chain_config;
} chain_config_map_t;

static void kymera_ConfigureSbcEncoder(Operator sbc, const void *params);
static void kymera_ConfigureMsbcEncoder(Operator msbc, const void *params);
static void kymera_ConfigureOpusEncoder(Operator opus, const void *params);

static const operator_config_map_t operator_config_map[] =
{
    {OPR_SBC_ENCODER, kymera_ConfigureSbcEncoder},
    {OPR_MSBC_ENCODER, kymera_ConfigureMsbcEncoder},
    {OPR_OPUS_ENCODER, kymera_ConfigureOpusEncoder}
};

static const appKymeraVaEncodeChainTable *chain_config_map = NULL;

static kymera_chain_handle_t va_encode_chain = NULL;

static void kymera_ConfigureSbcEncoder(Operator sbc, const void *params)
{
    const va_encode_chain_op_params_t *op_params = params;
    sbc_encoder_params_t sbc_params =
    {
        .number_of_subbands = op_params->encoder_params->sbc.number_of_subbands,
        .number_of_blocks = op_params->encoder_params->sbc.block_length,
        .bitpool_size = op_params->encoder_params->sbc.bitpool_size,
        .sample_rate = Kymera_GetVaMicChainOutputSampleRate(),
        .channel_mode = sbc_encoder_channel_mode_mono,
        .allocation_method = op_params->encoder_params->sbc.allocation_method
    };

    OperatorsSbcEncoderSetEncodingParams(sbc, &sbc_params);
}

static void kymera_ConfigureMsbcEncoder(Operator msbc, const void *params)
{
    const va_encode_chain_op_params_t *op_params = params;
    OperatorsMsbcEncoderSetBitpool(msbc, op_params->encoder_params->msbc.bitpool_size);
}

static void kymera_ConfigureOpusEncoder(Operator opus, const void *params)
{
    const va_encode_chain_op_params_t *op_params = params;
    OperatorsSetOpusFrameSize(opus, op_params->encoder_params->opus.frame_size);
}

static const chain_config_t * kymera_GetChainConfig(const va_encode_chain_params_t *params)
{
    unsigned i;
    for(i = 0; i < chain_config_map->table_length; i++)
    {
        if (chain_config_map->chain_table[i].encoder == params->encoder)
        {
            return chain_config_map->chain_table[i].chain_config;
        }
    }

    PANIC("kymera_GetChainConfig: Encoder not supported!");
    return NULL;
}

static Sink kymera_GetChainInput(unsigned input_role)
{
    PanicNull(va_encode_chain);
    return ChainGetInput(va_encode_chain, input_role);
}

static Source kymera_GetChainOutput(unsigned output_role)
{
    PanicNull(va_encode_chain);
    return ChainGetOutput(va_encode_chain, output_role);
}

static void kymera_CreateChain(const va_encode_chain_params_t *params)
{
    PanicNotNull(va_encode_chain);
    va_encode_chain = PanicNull(ChainCreate(kymera_GetChainConfig(params)));
}

static void kymera_ConfigureChain(const va_encode_chain_op_params_t *params)
{
    Kymera_ConfigureChain(va_encode_chain, operator_config_map, ARRAY_DIM(operator_config_map), params);
}

static void kymera_ConnectChain(void)
{
    ChainConnect(va_encode_chain);
    PanicNull(StreamConnect(Kymera_GetVaMicChainEncodeOutput(), kymera_GetChainInput(EPR_VA_ENCODE_IN)));
    PanicFalse(SourceMapInit(Kymera_GetVaEncodeChainOutput(), STREAM_TIMESTAMPED, AUDIO_FRAME_VA_DATA_LENGTH));
}

static void kymera_DisconnectChain(void)
{
    Source capture_output = Kymera_GetVaEncodeChainOutput();
    /* Ignore failed return since it is of no real consequence at this point */
    SourceUnmap(capture_output);
    StreamDisconnect(capture_output, NULL);
    StreamDisconnect(NULL, kymera_GetChainInput(EPR_VA_ENCODE_IN));
}

void Kymera_CreateVaEncodeChain(const va_encode_chain_create_params_t *params)
{
    PanicFalse(params != NULL);
    kymera_CreateChain(&params->chain_params);
    kymera_ConfigureChain(&params->operators_params);
    kymera_ConnectChain();
}

void Kymera_DestroyVaEncodeChain(void)
{
    kymera_DisconnectChain();
    ChainDestroy(va_encode_chain);
    va_encode_chain = NULL;
}

void Kymera_StartVaEncodeChain(void)
{
    ChainStart(va_encode_chain);
}

void Kymera_StopVaEncodeChain(void)
{
    ChainStop(va_encode_chain);
}

Source Kymera_GetVaEncodeChainOutput(void)
{
    return kymera_GetChainOutput(EPR_VA_ENCODE_OUT);
}

void Kymera_SetVaEncodeChainTable(const appKymeraVaEncodeChainTable *chain_table)
{
    chain_config_map = chain_table;
}
