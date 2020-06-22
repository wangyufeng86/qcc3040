/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Private header to connect/manage to AEC chain
*/

#ifndef KYMERA_AEC_H_
#define KYMERA_AEC_H_

#include <chain.h>
#include <operators.h>

#define AEC_REF_STF_GAIN_EXP_PARAM_INDEX 14
#define AEC_REF_STF_GAIN_MANTISSA_PARAM_INDEX 15

#define AEC_REF_CONFIG_PARAM_INDEX 0x0000
#define AEC_REF_CONFIG_PARAM_DEFAULT 0x2080
#define AEC_REF_CONFIG_PARAM_ENABLE_SIDETONE 0x2090

#define AEC_REF_SAME_INPUT_OUTPUT_CLK_SOURCE 0x0008

typedef struct
{
    uint16 id;
    uint16 value;
} aec_ref_set_same_in_out_clk_src_msg_t;

typedef struct
{
    Source input_1;
    Sink   speaker_output_1;
    Sink   speaker_output_2;
} aec_connect_audio_output_t;

typedef struct
{
    Sink   reference_output;
    Source mic_input_1;
    Source mic_input_2;
    Sink   mic_output_1;
    Sink   mic_output_2;
} aec_connect_audio_input_t;

typedef struct
{
    uint32 ttp_delay;
    uint32 ttp_gate_delay;
    uint32 spk_sample_rate;
    uint32 mic_sample_rate;
    uint32 buffer_size;
    bool is_source_clock_same;
}aec_audio_config_t;

/* descibes for what use-case AEC is being used */
typedef enum
{
    aec_usecase_default,
    aec_usecase_voice_call,
    aec_usecase_voice_assistant,
    aec_usecase_standalone_leakthrough,
    aec_usecase_a2dp_leakthrough,
    aec_usecase_sco_leakthrough,
    aec_usecase_va_leakthrough,
} aec_usecase_t ;

/*! \brief AEC_REF operator UCIDs */
typedef enum kymera_operator_ucids
{
    UCID_AEC_NB    = 0,
    UCID_AEC_WB    = 1,
    UCID_AEC_WB_VA = 2,
    UCID_AEC_UWB   = 3,
    UCID_AEC_SWB   = 4,
    UCID_AEC_DEFAULT = 5,
    UCID_AEC_SA_LT_MODE_1 = 6,
    UCID_AEC_SA_LT_MODE_2 = 7,
    UCID_AEC_SA_LT_MODE_3 = 8,
    UCID_AEC_NB_LT_MODE_1 = 9,
    UCID_AEC_NB_LT_MODE_2 = 10,
    UCID_AEC_NB_LT_MODE_3 = 11,
    UCID_AEC_WB_LT_MODE_1 = 12,
    UCID_AEC_WB_LT_MODE_2 = 13,
    UCID_AEC_WB_LT_MODE_3 = 14,
    UCID_AEC_UWB_LT_MODE_1 = 15,
    UCID_AEC_UWB_LT_MODE_2 = 16,
    UCID_AEC_UWB_LT_MODE_3 = 17,
    UCID_AEC_SWB_LT_MODE_1 = 18,
    UCID_AEC_SWB_LT_MODE_2 = 19,
    UCID_AEC_SWB_LT_MODE_3 = 20,
    UCID_AEC_A2DP_44_1_KHZ_LT_MODE_1 = 21,
    UCID_AEC_A2DP_44_1_KHZ_LT_MODE_2 = 22,
    UCID_AEC_A2DP_44_1_KHZ_LT_MODE_3 = 23,
    UCID_AEC_A2DP_48_KHZ_LT_MODE_1 = 24,
    UCID_AEC_A2DP_48_KHZ_LT_MODE_2 = 25,
    UCID_AEC_A2DP_48_KHZ_LT_MODE_3 = 26,
    UCID_AEC_A2DP_OTHER_SAMPLE_RATE_LT_MODE_1 = 27,
    UCID_AEC_A2DP_OTHER_SAMPLE_RATE_LT_MODE_2 = 28,
    UCID_AEC_A2DP_OTHER_SAMPLE_RATE_LT_MODE_3 = 29,
    UCID_AEC_WB_VA_LT_MODE_1 = 30,
    UCID_AEC_WB_VA_LT_MODE_2 = 31,
    UCID_AEC_WB_VA_LT_MODE_3 = 32,
    UCID_CVC_SEND    = 0,
    UCID_CVC_SEND_VA = 1,
    UCID_CVC_RECEIVE = 0,
    UCID_VOLUME_CONTROL = 0,
    UCID_SOURCE_SYNC = 0,
    UCID_ANC_TUNING = 0
} kymera_operator_ucid_t;

/*! \brief Connect audio output source to AEC.
    \param params All parameters required to configure/connect to the AEC chain.
    \note Handles the creation of the AEC chain.
*/
void Kymera_ConnectAudioOutputToAec(const aec_connect_audio_output_t *params, const aec_audio_config_t* config);

/*! \brief Disconnect audio output source from AEC.
    \note Handles the destruction of the AEC chain.
*/
void Kymera_DisconnectAudioOutputFromAec(void);

/*! \brief Connect audio input source to AEC.
    \param params All parameters required to configure/connect to the AEC chain.
    \note Handles the creation of the AEC chain.
*/
void Kymera_ConnectAudioInputToAec(const aec_connect_audio_input_t *params, const aec_audio_config_t* config);

/*! \brief Disconnect audio input source from AEC.
    \note Handles the destruction of the AEC chain.
*/
void Kymera_DisconnectAudioInputFromAec(void);

/*! \brief Get AEC Operator
*/
Operator Kymera_GetAecOperator(void);

/*! \brief Sets the AEC reference use-case
*/
void Kymera_SetAecUseCase(aec_usecase_t usecase);

/*! \brief Enable the sidetone path for AEC */
void Kymera_AecEnableSidetonePath(bool enable);

/*! \brief set Sidetone Gain for AEC */
void Kymera_AecSetSidetoneGain(uint32 exponent_value, uint32 mantissa_value);

/*! \brief get UCID for AEC_REF operator */
kymera_operator_ucid_t Kymera_GetAecUcid(void);

/*! \brief Facilitate transition to low power mode for AEC
*/
void Kymera_AecSleep(void);

/*! \brief Facilitate transition to exit low power mode for AEC
*/
void Kymera_AecWake(void);

#endif /* KYMERA_AEC_H_ */
