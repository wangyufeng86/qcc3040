/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle VA mic chain

*/

#ifndef KYMERA_VA_MIC_CHAIN_H_
#define KYMERA_VA_MIC_CHAIN_H_

/*! \brief Parameters used to determine the chain to use/create */
typedef struct
{
    bool  wake_up_word_detection;
    bool  clear_voice_capture;
    uint8 number_of_mics;
} va_mic_chain_params_t;

/*! \brief Parameters used to configure the VA mic chain operators */
typedef struct
{
    /*! Only used when WuW detection is enabled, max milliseconds of audio to buffer */
    uint16 max_pre_roll_in_ms;
    uint32 mic_sample_rate;
} va_mic_chain_op_params_t;

/*! \brief Parameters used to create the VA mic chain */
typedef struct
{
    va_mic_chain_params_t    chain_params;
    va_mic_chain_op_params_t operators_params;
} va_mic_chain_create_params_t;

/*! \brief Parameters used to connect to the VA mic chain */
typedef struct
{
    Sink capture_output;
    Sink detection_output;
} va_mic_chain_connect_params_t;

/*! \param params Parameters used to create/configure the chain.
*/
void Kymera_CreateVaMicChain(const va_mic_chain_create_params_t *params);
void Kymera_DestroyVaMicChain(void);

/*! \param params Parameters used to connect to the chain.
*/
void Kymera_ConnectToVaMicChain(const va_mic_chain_connect_params_t *params);

void Kymera_StartVaMicChain(void);
void Kymera_StopVaMicChain(void);

void Kymera_VaMicChainSleep(void);
void Kymera_VaMicChainWake(void);

void Kymera_VaMicChainStartGraphManagerDelegation(Operator graph_manager, Operator wuw_engine);
void Kymera_VaMicChainStopGraphManagerDelegation(Operator graph_manager, Operator wuw_engine);

/*! \param start_timestamp Timestamp from which to start the stream.
*/
void Kymera_ActivateVaMicChainEncodeOutputAfterTimestamp(uint32 start_timestamp);
void Kymera_ActivateVaMicChainEncodeOutput(void);
void Kymera_DeactivateVaMicChainEncodeOutput(void);
/*! \brief Start buffering mic data for encode output.
*/
void Kymera_BufferVaMicChainEncodeOutput(void);

void Kymera_ActivateVaMicChainWuwOutput(void);
void Kymera_DeactivateVaMicChainWuwOutput(void);

Source Kymera_GetVaMicChainEncodeOutput(void);
Source Kymera_GetVaMicChainWuwOutput(void);

uint32 Kymera_GetVaMicChainOutputSampleRate(void);

unsigned Kymera_GetVaMicChainMaxMicrophonesSupported(void);

#endif /* KYMERA_VA_MIC_CHAIN_H_ */
