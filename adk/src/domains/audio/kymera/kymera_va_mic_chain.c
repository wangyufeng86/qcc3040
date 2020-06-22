/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle VA mic chain

*/

#include "kymera_va_mic_chain.h"
#include "kymera_private.h"
#include "kymera_aec.h"
#include "kymera_va_common.h"
#include "kymera_config.h"

#define METADATA_REFRAMING_SIZE (384)

typedef struct
{
    va_mic_chain_params_t chain_params;
    const chain_config_t *chain_config;
} chain_config_map_t;

typedef void (* SourceFunction) (Source *array, unsigned length_of_array);

static uint32 chain_output_sample_rate = 0;

static void kymera_ConfigureVad(Operator vad, const void *params)
{
    va_mic_chain_op_params_t *op_params = (va_mic_chain_op_params_t *)params;
    OperatorsStandardSetSampleRate(vad, op_params->mic_sample_rate);
}

static void kymera_ConfigureCvc(Operator cvc, const void *params)
{
    UNUSED(params);
    OperatorsStandardSetUCID(cvc, UCID_CVC_SEND_VA);
}

static uint32 kymera_DivideRoundingUp(uint32 dividend, uint32 divisor)
{
    if (dividend == 0)
        return 0;
    else
        return ((dividend - 1) / divisor) + 1;
}

static void kymera_ConfigureSplitter(Operator splitter, const void *params)
{
    const va_mic_chain_op_params_t *op_params = params;
    uint32 buffer_size = kymera_DivideRoundingUp(op_params->max_pre_roll_in_ms * op_params->mic_sample_rate, 1000);
    OperatorsSplitterSetWorkingMode(splitter, splitter_mode_buffer_input);
#ifdef HAVE_SRAM
    OperatorsSplitterSetBufferLocation(splitter, splitter_buffer_location_sram);
#else
    OperatorsSplitterSetBufferLocation(splitter, splitter_buffer_location_internal);
#endif
    OperatorsSplitterSetPacking(splitter, splitter_packing_packed);
    OperatorsSplitterSetDataFormat(splitter, operator_data_format_pcm);
    OperatorsStandardSetBufferSize(splitter, buffer_size);
    OperatorsStandardSetSampleRate(splitter, op_params->mic_sample_rate);
    OperatorsSplitterSetMetadataReframing(splitter, splitter_reframing_enable, METADATA_REFRAMING_SIZE);
}

static const appKymeraVaMicChainTable *chain_config_map;

static const operator_config_map_t operator_config_map[] =
{
    {OPR_VAD,      kymera_ConfigureVad},
    {OPR_CVC_SEND, kymera_ConfigureCvc},
    {OPR_SPLITTER, kymera_ConfigureSplitter}
};

static kymera_chain_handle_t va_mic_chain = NULL;

static bool kymera_IsChainCompatible(const appKymeraVaMicChainInfo *chain_info, const va_mic_chain_params_t *params)
{
    return (chain_info->wake_up_word_detection == params->wake_up_word_detection) &&
           (chain_info->clear_voice_capture    == params->clear_voice_capture) &&
           (chain_info->number_of_mics         == params->number_of_mics);
}

static const chain_config_t * kymera_GetChainConfig(const va_mic_chain_params_t *params)
{
    unsigned i;
    for(i = 0; i < chain_config_map->table_length; i++)
    {
        if (kymera_IsChainCompatible(&chain_config_map->chain_table[i], params))
        {
            return chain_config_map->chain_table[i].chain_config;
        }
    }

    PANIC("kymera_GetChainConfig: No compatible chain configuration found!");
    return NULL;
}

static Operator kymera_GetChainOperator(unsigned operator_role)
{
    PanicNull(va_mic_chain);
    return ChainGetOperatorByRole(va_mic_chain, operator_role);
}

static Sink kymera_GetChainInput(unsigned input_role)
{
    PanicNull(va_mic_chain);
    return ChainGetInput(va_mic_chain, input_role);
}

static Source kymera_GetChainOutput(unsigned output_role)
{
    PanicNull(va_mic_chain);
    return ChainGetOutput(va_mic_chain, output_role);
}

static void kymera_CreateChain(const va_mic_chain_params_t *params)
{
    PanicNotNull(va_mic_chain);
    va_mic_chain = PanicNull(ChainCreate(kymera_GetChainConfig(params)));
}

static void kymera_ConfigureChain(const va_mic_chain_op_params_t *params)
{
    Kymera_ConfigureChain(va_mic_chain, operator_config_map, ARRAY_DIM(operator_config_map), params);
}

#ifdef INCLUDE_KYMERA_AEC
static aec_usecase_t kymera_GetVoiceCaptureUsecase(void)
{
    aec_usecase_t aec_usecase=aec_usecase_voice_assistant;

    if(AecLeakthrough_IsLeakthroughEnabled())
    {
        aec_usecase = aec_usecase_va_leakthrough;
    }

    return aec_usecase;
}
#endif

static bool kymera_ConnectToAec(uint32 mic_sample_rate, Source mic1, Source mic2)
{
    bool connected_to_aec = FALSE;

#ifdef INCLUDE_KYMERA_AEC
    Sink aec_input  = kymera_GetChainInput(EPR_VA_MIC_AEC_IN);
    if (aec_input)
    {
        aec_connect_audio_input_t connect_params = {0};
        aec_audio_config_t config = {0};
        
        config.mic_sample_rate  = mic_sample_rate;
        connect_params.reference_output = aec_input;
        connect_params.mic_input_1      = mic1;
        connect_params.mic_input_2      = mic2;
        connect_params.mic_output_1     = kymera_GetChainInput(EPR_VA_MIC_MIC1_IN);
        connect_params.mic_output_2     = kymera_GetChainInput(EPR_VA_MIC_MIC2_IN);
        Kymera_SetAecUseCase(kymera_GetVoiceCaptureUsecase());
        Kymera_ConnectAudioInputToAec(&connect_params, &config);
        connected_to_aec = TRUE;
    }
#else
    UNUSED(mic_sample_rate);
    UNUSED(mic1);
    UNUSED(mic2);
#endif

    return connected_to_aec;
}

static bool kymera_DisconnectChainFromAec(void)
{
    bool disconnected_from_aec = FALSE;

#ifdef INCLUDE_KYMERA_AEC
    Sink aec_input  = kymera_GetChainInput(EPR_VA_MIC_AEC_IN);
    if (aec_input)
    {
        Kymera_DisconnectAudioInputFromAec();
        disconnected_from_aec = TRUE;
        Kymera_SetAecUseCase(aec_usecase_default);
    }
#endif

    return disconnected_from_aec;
}

static void kymera_ConnectChainToMics(const va_mic_chain_op_params_t *params)
{
    Source mic1 = NULL, mic2 = NULL;
    Sink mic1_input = kymera_GetChainInput(EPR_VA_MIC_MIC1_IN);
    Sink mic2_input = kymera_GetChainInput(EPR_VA_MIC_MIC2_IN);

    if (mic1_input)
    {
        mic1 = Kymera_GetMicrophoneSource(appConfigVaMic1(), mic1, params->mic_sample_rate, high_priority_user);
    }
    if (mic2_input)
    {
        mic2 = Kymera_GetMicrophoneSource(appConfigVaMic2(), mic1, params->mic_sample_rate, high_priority_user);
    }

    if (kymera_ConnectToAec(params->mic_sample_rate, mic1, mic2) == FALSE)
    {
        if (mic1_input)
        {
            PanicNull(StreamConnect(mic1, mic1_input));
        }
        if (mic2_input)
        {
            PanicNull(StreamConnect(mic2, mic2_input));
        }
    }
}

static void kymera_DisconnectChainFromMics(void)
{
    Sink mic1_input = kymera_GetChainInput(EPR_VA_MIC_MIC1_IN);
    Sink mic2_input = kymera_GetChainInput(EPR_VA_MIC_MIC2_IN);

    if (kymera_DisconnectChainFromAec() == FALSE)
    {
        if (mic1_input)
        {
            StreamDisconnect(NULL, mic1_input);
        }
        if (mic2_input)
        {
            StreamDisconnect(NULL, mic2_input);
        }
    }

    if (mic1_input)
    {
        Kymera_CloseMicrophone(appConfigVaMic1(), high_priority_user);
    }
    if (mic2_input)
    {
        Kymera_CloseMicrophone(appConfigVaMic2(), high_priority_user);
    }
}

static void kymera_RunUsingConnectedMics(SourceFunction function)
{
    unsigned number_of_mics = 0;
    Source mics[2] = {0};

    if (kymera_GetChainInput(EPR_VA_MIC_MIC1_IN))
    {
        mics[number_of_mics] = PanicNull(Microphones_GetMicrophoneSource(appConfigVaMic1()));
        number_of_mics++;
    }

    if (kymera_GetChainInput(EPR_VA_MIC_MIC2_IN))
    {
        mics[number_of_mics] = PanicNull(Microphones_GetMicrophoneSource(appConfigVaMic2()));
        number_of_mics++;
    }

    function(mics, number_of_mics);
}

static void kymera_PreserveSources(Source *array, unsigned length_of_array)
{
    PanicFalse(OperatorFrameworkPreserve(0, NULL, length_of_array, array, 0, NULL));
}

static void kymera_ReleaseSources(Source *array, unsigned length_of_array)
{
    PanicFalse(OperatorFrameworkRelease(0, NULL, length_of_array, array, 0, NULL));
}

static void kymera_ConnectChain(const va_mic_chain_op_params_t *params)
{
    kymera_ConnectChainToMics(params);
    ChainConnect(va_mic_chain);
}

static void kymera_DisconnectChain(void)
{
    StreamDisconnect(Kymera_GetVaMicChainEncodeOutput(), NULL);
    StreamDisconnect(Kymera_GetVaMicChainWuwOutput(), NULL);
    kymera_DisconnectChainFromMics();
}

static graph_manager_delegate_op_t kymera_GetOperatorsToDelegate(void)
{
    graph_manager_delegate_op_t ops = {INVALID_OPERATOR};

    ops.cvc = kymera_GetChainOperator(OPR_CVC_SEND);
    ops.splitter = kymera_GetChainOperator(OPR_SPLITTER);
    ops.vad = kymera_GetChainOperator(OPR_VAD);

    return ops;
}

static void kymera_RunUsingOperatorsToDelegate(OperatorFunction function)
{
    graph_manager_delegate_op_t delegate_ops = kymera_GetOperatorsToDelegate();
    Operator ops[] =
    {
        delegate_ops.cvc,
        delegate_ops.splitter,
        delegate_ops.vad,
    };

    function(ops, ARRAY_DIM(ops));
}

static void kymera_ChainSleep(Operator *array, unsigned length_of_array)
{
    operator_list_t operators_to_exclude = {array, length_of_array};
    ChainSleep(va_mic_chain, &operators_to_exclude);
}

static void kymera_ChainWake(Operator *array, unsigned length_of_array)
{
    operator_list_t operators_to_exclude = {array, length_of_array};
    ChainWake(va_mic_chain, &operators_to_exclude);
}

void Kymera_CreateVaMicChain(const va_mic_chain_create_params_t *params)
{
    PanicFalse(params != NULL);
    chain_output_sample_rate = params->operators_params.mic_sample_rate;
    kymera_CreateChain(&params->chain_params);
    kymera_ConfigureChain(&params->operators_params);
    kymera_ConnectChain(&params->operators_params);
}

void Kymera_DestroyVaMicChain(void)
{
    PanicNull(va_mic_chain);
    chain_output_sample_rate = 0;
    kymera_DisconnectChain();
    ChainDestroy(va_mic_chain);
    va_mic_chain = NULL;
}

void Kymera_StartVaMicChain(void)
{
    ChainStart(va_mic_chain);
}

void Kymera_StopVaMicChain(void)
{
    ChainStop(va_mic_chain);
}

void Kymera_VaMicChainSleep(void)
{
    kymera_RunUsingConnectedMics(kymera_PreserveSources);
    kymera_RunUsingOperatorsToDelegate(kymera_ChainSleep);

#ifdef INCLUDE_KYMERA_AEC
    Kymera_AecSleep();
#endif
}

void Kymera_VaMicChainWake(void)
{
#ifdef INCLUDE_KYMERA_AEC
    Kymera_AecWake();
#endif

    kymera_RunUsingOperatorsToDelegate(kymera_ChainWake);
    kymera_RunUsingConnectedMics(kymera_ReleaseSources);
}

void Kymera_VaMicChainStartGraphManagerDelegation(Operator graph_manager, Operator wuw_engine)
{
    graph_manager_delegate_op_t operators = kymera_GetOperatorsToDelegate();
    operators.wuw_engine = wuw_engine;
    OperatorsGraphManagerStartDelegation(graph_manager, &operators);
}

void Kymera_VaMicChainStopGraphManagerDelegation(Operator graph_manager, Operator wuw_engine)
{
    graph_manager_delegate_op_t operators = kymera_GetOperatorsToDelegate();
    operators.wuw_engine = wuw_engine;
    OperatorsGraphManagerStopDelegation(graph_manager, &operators);
}

void Kymera_ActivateVaMicChainEncodeOutputAfterTimestamp(uint32 start_timestamp)
{
    Operator splitter = kymera_GetChainOperator(OPR_SPLITTER);
    OperatorsSplitterActivateOutputStreamAfterTimestamp(splitter, start_timestamp, splitter_output_stream_1);
}

void Kymera_ActivateVaMicChainEncodeOutput(void)
{
    Operator splitter = kymera_GetChainOperator(OPR_SPLITTER);
    OperatorsSplitterActivateOutputStream(splitter, splitter_output_stream_1);
}

void Kymera_DeactivateVaMicChainEncodeOutput(void)
{
    Operator splitter = kymera_GetChainOperator(OPR_SPLITTER);
    OperatorsSplitterDeactivateOutputStream(splitter, splitter_output_stream_1);
}

void Kymera_BufferVaMicChainEncodeOutput(void)
{
    Operator splitter = kymera_GetChainOperator(OPR_SPLITTER);
    OperatorsSplitterBufferOutputStream(splitter, splitter_output_stream_1);
}

void Kymera_ActivateVaMicChainWuwOutput(void)
{
    Operator splitter = kymera_GetChainOperator(OPR_SPLITTER);
    OperatorsSplitterActivateOutputStream(splitter, splitter_output_stream_0);
}

void Kymera_DeactivateVaMicChainWuwOutput(void)
{
    Operator splitter = kymera_GetChainOperator(OPR_SPLITTER);
    OperatorsSplitterDeactivateOutputStream(splitter, splitter_output_stream_0);
}

Source Kymera_GetVaMicChainEncodeOutput(void)
{
    return kymera_GetChainOutput(EPR_VA_MIC_ENCODE_OUT);
}

Source Kymera_GetVaMicChainWuwOutput(void)
{
    return kymera_GetChainOutput(EPR_VA_MIC_WUW_OUT);
}

uint32 Kymera_GetVaMicChainOutputSampleRate(void)
{
    return chain_output_sample_rate;
}

unsigned Kymera_GetVaMicChainMaxMicrophonesSupported(void)
{
    unsigned i;
    unsigned max_num_of_mics = 0;
    for(i = 0; i < chain_config_map->table_length; i++)
    {
        max_num_of_mics = MAX(chain_config_map->chain_table->number_of_mics, max_num_of_mics);
    }

    return max_num_of_mics;
}

void Kymera_SetVaMicChainTable(const appKymeraVaMicChainTable *chain_table)
{
    chain_config_map = chain_table;
}
