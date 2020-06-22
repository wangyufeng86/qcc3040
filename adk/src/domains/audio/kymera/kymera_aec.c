/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to connect/manage to AEC chain
*/

#include "kymera_aec.h"
#include "kymera_private.h"
#include <operators.h>
#include <vmal.h>

#define DEFAULT_MIC_SAMPLE_RATE     (8000)
#define DEFAULT_OUTPUT_SAMPLE_RATE  (48000)
#define DEFAULT_MIC_TTP_LATENCY     (40000)
#define AEC_A2DP_SAMPLE_RATE_44_1K  (44100U)
#define AEC_A2DP_SAMPLE_RATE_48K    (48000U)

#define AEC_NB_RATE 8000
#define AEC_WB_RATE 16000
#define AEC_SWB_RATE 32000

typedef enum
{
    audio_output_connected = (1 << 0),
    audio_input_connected  = (1 << 1)
} aec_connections_t;

static struct
{
    uint32            mic_sample_rate;
    uint32            spk_sample_rate;
    aec_connections_t aec_connections;
    aec_usecase_t use_case;
} aec_config;

kymera_chain_handle_t aec_chain;

static kymera_chain_handle_t kymera_GetAecChain(void)
{
    DEBUG_LOG("kymera_GetAecChain. AEC chain %d", aec_chain);
    return aec_chain;
}

static void kymera_SetAecChain(kymera_chain_handle_t chain)
{
    DEBUG_LOG("kymera_SetAecChian. AEC chain %d", chain);
    aec_chain = chain;
}

static uint32 kymera_GetOutputSampleRate(void)
{
    uint32 sample_rate = DEFAULT_OUTPUT_SAMPLE_RATE;

    if (aec_config.spk_sample_rate)
    {
        sample_rate = aec_config.spk_sample_rate;
    }
    return sample_rate;
}

static uint32 kymera_GetMicSampleRate(void)
{
    uint32 sample_rate = DEFAULT_MIC_SAMPLE_RATE;

    if (aec_config.mic_sample_rate)
    {
        sample_rate = aec_config.mic_sample_rate;
    }
    return sample_rate;
}

static void kymera_SetMicSampleRate(uint32 sample_rate)
{
    aec_config.mic_sample_rate = sample_rate;
}

static void kymera_SetSpkSampleRate(uint32 sample_rate)
{
    aec_config.spk_sample_rate = sample_rate;
}

static Operator kymera_GetAecOperator(void)
{
    Operator aec = ChainGetOperatorByRole(kymera_GetAecChain(), OPR_AEC);
    if (!aec)
    {
        Panic();
    }
    return aec;
}

static Sink kymera_GetAecInput(chain_endpoint_role_t input_role)
{
    return ChainGetInput(kymera_GetAecChain(), input_role);
}

static Source kymera_GetAecOutput(chain_endpoint_role_t output_role)
{
    return ChainGetOutput(kymera_GetAecChain(), output_role);
}

static bool kymera_IsAudioInputConnected(void)
{
    return (aec_config.aec_connections & audio_input_connected) != 0;
}

static bool kymera_IsAudioOutputConnected(void)
{
    return (aec_config.aec_connections & audio_output_connected) != 0;
}

static bool kymera_IsAecConnected(void)
{
    return kymera_IsAudioInputConnected() || kymera_IsAudioOutputConnected();
}

static void kymera_AddAecConnection(aec_connections_t connection)
{
    aec_config.aec_connections |= connection;
}

static void kymera_RemoveAecConnection(aec_connections_t connection)
{
    aec_config.aec_connections &= ~connection;
}


static void kymera_ConnectIfValid(Source source, Sink sink)
{
    if (source && sink)
    {
        PanicNull(StreamConnect(source, sink));
    }
}

static void kymera_ConfigureAecChain(void)
{
    Operator aec = kymera_GetAecOperator();

    DEBUG_LOG("kymera_ConfigureAecChain: Output sample rate %u, mic sample rate %u", kymera_GetOutputSampleRate(), kymera_GetMicSampleRate());
    OperatorsStandardSetUCID(aec, Kymera_GetAecUcid());
    OperatorsAecSetSampleRate(aec, kymera_GetOutputSampleRate(), kymera_GetMicSampleRate());
}

static void kymera_AecSetSameInputOutputClkSource(Operator op, bool enable)
{
    aec_ref_set_same_in_out_clk_src_msg_t aec_ref_set_same_in_out_clk_src_msg;

    aec_ref_set_same_in_out_clk_src_msg.id = AEC_REF_SAME_INPUT_OUTPUT_CLK_SOURCE;
    aec_ref_set_same_in_out_clk_src_msg.value = (uint16)enable;

    PanicFalse(VmalOperatorMessage(op, &aec_ref_set_same_in_out_clk_src_msg, SIZEOF_OPERATOR_MESSAGE(aec_ref_set_same_in_out_clk_src_msg), NULL, 0));

}

static void kymera_AdditionalConfigureForAec(const aec_audio_config_t* config)
{
    Operator aec = kymera_GetAecOperator();
    DEBUG_LOG("kymera_AdditionalConfigureForAec: Terminal Buffer Size %u", config->buffer_size);
    DEBUG_LOG("kymera_AdditionalConfigureForAec: TTP_Delay %u, TTP_Gate_Delay %u", config->ttp_delay, config->ttp_gate_delay);
    DEBUG_LOG("kymera_AdditionalConfigureForAec: Source Clock %s", config->is_source_clock_same ? "Yes" : "No");
    /* if size is valid */
    if(config->buffer_size != 0)
    {
        /* Set AEC REF input terminal buffer size */
        appKymeraSetTerminalBufferSize(aec, kymera_GetMicSampleRate(), config->buffer_size, 1 << 0, 0);
    }
    /* if TTP delay is required */
    if(config->ttp_gate_delay)
    {
#ifndef __QCC514X_APPS__
        /*! \todo AEC Gate is V2 silicon, downloadable has native TTP support*/
        OperatorsAecEnableTtpGate(aec, TRUE, config->ttp_gate_delay, TRUE);
#endif
    }
    else
    {
        uint32 ttp_latency = (config->ttp_delay) ? config->ttp_delay : DEFAULT_MIC_TTP_LATENCY;
        OperatorsStandardSetTimeToPlayLatency(aec, ttp_latency);
    }

    if(config->is_source_clock_same)
    {

        /*! Message AECREF operator that the back-end of the operator are coming
            from same clock source. This is for optimisation purpose and it's recommended
            to be enabled for use cases where speaker input and microphone output are
            synchronised (e.g. SCO and USB voice use cases). Note: Send/Resend this message
            when all microphone input/output and REFERENCE output are disconnected.
        */
        kymera_AecSetSameInputOutputClkSource(aec,TRUE);
    }
}

static void kymera_CreateAecChain(const aec_audio_config_t* config)
{
    DEBUG_LOG("kymera_CreateAecChain");
    PanicNotNull(kymera_GetAecChain());
    kymera_SetAecChain(PanicNull(ChainCreate(Kymera_GetChainConfigs()->chain_aec_config)));
    kymera_ConfigureAecChain();
    kymera_AdditionalConfigureForAec(config);
    ChainConnect(kymera_GetAecChain());
    ChainStart(kymera_GetAecChain());
}

static void kymera_DestroyAecChain(void)
{
    DEBUG_LOG("kymera_DestroyAecChain");
    PanicNull(kymera_GetAecChain());
    ChainStop(kymera_GetAecChain());
    ChainDestroy(kymera_GetAecChain());
    kymera_SetAecChain(NULL);
}

static void kymera_ConnectAudioOutput(const aec_connect_audio_output_t *params)
{
    DEBUG_LOG("kymera_ConnectAudioOutput: Connect audio output to AEC");
    kymera_AddAecConnection(audio_output_connected);
    kymera_ConnectIfValid(kymera_GetAecOutput(EPR_AEC_SPEAKER1_OUT), params->speaker_output_1);
    kymera_ConnectIfValid(kymera_GetAecOutput(EPR_AEC_SPEAKER2_OUT), params->speaker_output_2);
    /* For a running operator connect the output before the input */
    kymera_ConnectIfValid(params->input_1, kymera_GetAecInput(EPR_AEC_INPUT1));
}

static void kymera_DisconnectAudioOutput(void)
{
    DEBUG_LOG("kymera_DisconnectAudioOutput: Disconnect audio output from AEC");
    kymera_RemoveAecConnection(audio_output_connected);
    /* For a running operator disconnect the output before the input */
    StreamDisconnect(kymera_GetAecOutput(EPR_AEC_SPEAKER1_OUT), NULL);
    StreamDisconnect(NULL, kymera_GetAecInput(EPR_AEC_INPUT1));
}

static void kymera_ConnectAudioInput(const aec_connect_audio_input_t *params)
{
    DEBUG_LOG("kymera_ConnectAudioInput: Connect audio input to AEC");
    kymera_AddAecConnection(audio_input_connected);

    /* For a running operator connect the input before the output */
    kymera_ConnectIfValid(params->mic_input_1, kymera_GetAecInput(EPR_AEC_MIC1_IN));
    kymera_ConnectIfValid(params->mic_input_2, kymera_GetAecInput(EPR_AEC_MIC2_IN));

    kymera_ConnectIfValid(kymera_GetAecOutput(EPR_AEC_REFERENCE_OUT), params->reference_output);
    kymera_ConnectIfValid(kymera_GetAecOutput(EPR_AEC_MIC1_OUT), params->mic_output_1);
    kymera_ConnectIfValid(kymera_GetAecOutput(EPR_AEC_MIC2_OUT), params->mic_output_2);
}

static void kymera_DisconnectAudioInput(void)
{
    DEBUG_LOG("kymera_DisconnectAudioInput: Disconnect audio input from AEC");
    kymera_RemoveAecConnection(audio_input_connected);

    /* For a running operator disconnect the output before the input */
    StreamDisconnect(kymera_GetAecOutput(EPR_AEC_MIC1_OUT), NULL);
    StreamDisconnect(kymera_GetAecOutput(EPR_AEC_MIC2_OUT), NULL);
    StreamDisconnect(kymera_GetAecOutput(EPR_AEC_REFERENCE_OUT), NULL);

    StreamDisconnect(NULL, kymera_GetAecInput(EPR_AEC_MIC1_IN));
    StreamDisconnect(NULL, kymera_GetAecInput(EPR_AEC_MIC2_IN));
}


kymera_operator_ucid_t Kymera_GetAecUcid(void)
{

    kymera_operator_ucid_t ucid = UCID_AEC_NB;
    switch(aec_config.use_case)
    {
        case aec_usecase_voice_assistant:
            ucid = UCID_AEC_WB_VA;
            break;

        case aec_usecase_voice_call:
            {
                if(aec_config.mic_sample_rate == AEC_NB_RATE)
                    ucid = UCID_AEC_NB;
                else if(aec_config.mic_sample_rate == AEC_WB_RATE)
                    ucid = UCID_AEC_WB;
                else if(aec_config.mic_sample_rate == AEC_SWB_RATE)
                    ucid = UCID_AEC_SWB;
                else /* Other bands are currently not supported */
                    Panic();
            }
            break;

        case aec_usecase_a2dp_leakthrough:
            {
                switch(aec_config.spk_sample_rate)
                {
                   case AEC_A2DP_SAMPLE_RATE_44_1K:
                   ucid = UCID_AEC_A2DP_44_1_KHZ_LT_MODE_1 + AecLeakthrough_GetMode();
                   break;

                   case AEC_A2DP_SAMPLE_RATE_48K:
                   ucid = UCID_AEC_A2DP_48_KHZ_LT_MODE_1 + AecLeakthrough_GetMode();
                   break;

                   default:
                   ucid = UCID_AEC_A2DP_OTHER_SAMPLE_RATE_LT_MODE_1 + AecLeakthrough_GetMode();
                   break;
                }
            }
            break;

        case aec_usecase_standalone_leakthrough:
            {
                ucid = UCID_AEC_SA_LT_MODE_1 + AecLeakthrough_GetMode();
            }
            break;

        case aec_usecase_sco_leakthrough:
            {
                if(aec_config.mic_sample_rate == AEC_NB_RATE)
                    ucid = UCID_AEC_NB_LT_MODE_1 + AecLeakthrough_GetMode();
                else if(aec_config.mic_sample_rate == AEC_WB_RATE)
                    ucid = UCID_AEC_WB_LT_MODE_1 + AecLeakthrough_GetMode();
                else if(aec_config.mic_sample_rate == AEC_SWB_RATE)
                    ucid = UCID_AEC_SWB_LT_MODE_1 + AecLeakthrough_GetMode();
                else /* Other bands are currently not supported */
                    Panic();
            }
            break;

        case aec_usecase_va_leakthrough:
            {
                ucid = UCID_AEC_WB_VA_LT_MODE_1 + AecLeakthrough_GetMode();
            }
            break;

        default:
            {
                ucid = UCID_AEC_DEFAULT;
            }
            break;
    }
    return ucid;
}

void Kymera_ConnectAudioOutputToAec(const aec_connect_audio_output_t *params, const aec_audio_config_t* config)
{
    PanicNotZero(kymera_IsAudioOutputConnected() || (params == NULL) || (config == NULL));
    kymera_SetSpkSampleRate(config->spk_sample_rate);
    if (kymera_GetAecChain() == NULL)
    {
        kymera_CreateAecChain(config);
    }
    else
    {
        kymera_ConfigureAecChain();
    }
    kymera_ConnectAudioOutput(params);
}

void Kymera_DisconnectAudioOutputFromAec(void)
{
    PanicFalse(kymera_IsAudioOutputConnected());
    kymera_SetSpkSampleRate(0);
    kymera_DisconnectAudioOutput();
    if (kymera_IsAecConnected() == FALSE)
    {
        kymera_DestroyAecChain();
    }
}

void Kymera_ConnectAudioInputToAec(const aec_connect_audio_input_t *params, const aec_audio_config_t* config)
{
    PanicNotZero(kymera_IsAudioInputConnected() || (params == NULL) || (config == NULL));
    kymera_SetMicSampleRate(config->mic_sample_rate);
    if (kymera_IsAecConnected() == FALSE)
    {
        kymera_CreateAecChain(config);
    }
    else
    {
        kymera_ConfigureAecChain();
    }
    kymera_ConnectAudioInput(params);
}

void Kymera_DisconnectAudioInputFromAec(void)
{
    PanicFalse(kymera_IsAudioInputConnected());
    kymera_SetMicSampleRate(0);
    kymera_DisconnectAudioInput();
    if (kymera_IsAecConnected() == FALSE)
    {
        kymera_DestroyAecChain();
    }
}

void Kymera_AecEnableSidetonePath(bool enable)
{
    Operator aec_ref = Kymera_GetAecOperator();
    set_params_data_t* set_params_data = OperatorsCreateSetParamsData(1);

    set_params_data->number_of_params = 1;
    set_params_data->standard_params[0].id = AEC_REF_CONFIG_PARAM_INDEX;
    set_params_data->standard_params[0].value =(enable) ?
                ((uint32)AEC_REF_CONFIG_PARAM_ENABLE_SIDETONE):((uint32)AEC_REF_CONFIG_PARAM_DEFAULT);

    OperatorsStandardSetParameters(aec_ref, set_params_data);
    free(set_params_data);
}

void Kymera_AecSetSidetoneGain(uint32 exponent_value, uint32 mantissa_value)
{
    Operator aec_ref =kymera_GetAecOperator();
    set_params_data_t* set_params_data = OperatorsCreateSetParamsData(2);

    set_params_data->number_of_params = 2;
    set_params_data->standard_params[0].id = AEC_REF_STF_GAIN_EXP_PARAM_INDEX;
    set_params_data->standard_params[0].value = exponent_value;
    set_params_data->standard_params[1].id = AEC_REF_STF_GAIN_MANTISSA_PARAM_INDEX;
    set_params_data->standard_params[1].value = mantissa_value;

    OperatorsStandardSetParameters(aec_ref, set_params_data);
    free(set_params_data);
}


Operator Kymera_GetAecOperator(void)
{
    return kymera_GetAecOperator();
}

void Kymera_SetAecUseCase(aec_usecase_t usecase)
{
    aec_config.use_case = usecase;
}

void Kymera_AecSleep(void)
{
    ChainSleep(aec_chain, NULL);
}

void Kymera_AecWake(void)
{
    ChainWake(aec_chain, NULL);
}
