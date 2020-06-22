/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_leakthrough.c
\brief      Kymera implmentation to accommodate software leak-through.
*/

#ifdef ENABLE_AEC_LEAKTHROUGH

#include <audio_clock.h>
#include <audio_power.h>
#include <vmal.h>
#include <file.h>
#include <cap_id_prim.h>
#include <opmsg_prim.h>
#include <bt_device.h>
#include <av.h>

#include "kymera.h"
#include "kymera_private.h"
#include "kymera_va.h"

/*The value sidetone_exp[0] and sidetone_mantissa[0] corresponds to value of -46dB and are starting point for leakthrough ramp*/
#define SIDETONE_EXP_MINIMUM sidetone_exp[0]
#define SIDETONE_MANTISSA_MINIMUM sidetone_mantissa[0]

#define AEC_REF_SETTLING_TIME  (100)
#define appConfigLeakthroughMic()                        (microphone_1)
#define appConfigMaxLeakthroughModes()                   (3)

#define LEAKTHROUGH_OUTPUT_RATE  (8000U)
#define DEFAULT_TERMINAL_BUFFER_SIZE 15
#define ENABLED TRUE
#define DISABLED FALSE

/*! The initial value in the Array given below corresponds to -46dB and ramp up is happening till 0dB with increment of 2dB/cycle */
const unsigned long sidetone_exp[] =      {0xFFFFFFFAUL, 0xFFFFFFFAUL, 0xFFFFFFFBUL, 0xFFFFFFFBUL, 0xFFFFFFFBUL, 0xFFFFFFFCUL, 0xFFFFFFFCUL, 0xFFFFFFFCUL, 0xFFFFFFFDUL, 0xFFFFFFFDUL, 0xFFFFFFFDUL, 0xFFFFFFFEUL, 0xFFFFFFFEUL, 0xFFFFFFFEUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000001UL, 0x00000001UL, 0x00000001UL, 0x00000001UL};
const unsigned long sidetone_mantissa[] = {0x290EA879UL, 0x33B02273UL, 0x2089229EUL, 0x28F5C28FUL ,0x3390CA2BUL, 0x207567A2UL, 0x28DCEBBFUL, 0x337184E6UL, 0x2061B89DUL, 0x28C423FFUL, 0x33525297UL, 0x204E1588UL, 0x28AB6B46UL, 0x33333333UL, 0x203A7E5BUL, 0x2892C18BUL, 0x331426AFUL, 0x2026F310UL, 0x287A26C5UL, 0x32F52CFFUL, 0x2013739EUL, 0x28619AEAUL, 0x32D64618UL, 0x40000000UL};
static unsigned gain_index = 0;


uint32 kymera_leakthrough_mic_sample_rate;
bool kymera_standalone_leakthrough_status;

static bool kymera_LeakthroughIsCurrentStepValueLastOne(void)
{
    return (gain_index >= ARRAY_DIM(sidetone_exp));
}

static uint32 kymera_GetLeakthroughMicSampleRate(void)
{
    return kymera_leakthrough_mic_sample_rate;
}

static void kymera_UpdateStandaloneLeakthroughStatus(bool status)
{
    DEBUG_LOG("kymera_UpdateStandaloneLeakthroughStatus");
    kymera_standalone_leakthrough_status = status;
}

void Kymera_LeakthroughResetGainIndex(void)
{
    gain_index = 0;
}

void Kymera_LeakthroughStepupSTGain(void)
{
    if(!kymera_LeakthroughIsCurrentStepValueLastOne())
    {
        kymeraTaskData *theKymera = KymeraGetTaskData();
        Kymera_AecSetSidetoneGain(sidetone_exp[gain_index], sidetone_mantissa[gain_index]);
        gain_index++;
        MessageSendLater(&theKymera->task,KYMERA_INTERNAL_LEAKTHROUGH_SIDETONE_GAIN_RAMPUP,NULL,ST_GAIN_RAMP_STEP_TIME_MS);
    }
    else
    {
        /* End of ramp is achived reset the gain index */
        Kymera_LeakthroughResetGainIndex();
    }
}

void Kymera_setLeakthroughMicSampleRate(uint32 sample_rate)
{
     kymera_leakthrough_mic_sample_rate = sample_rate;
}

void Kymera_DisconnectLeakthroughMic(void)
{
    Source mic_in0;
    uint8 mic0=appConfigLeakthroughMic();
    uint32 sample_rate = kymera_GetLeakthroughMicSampleRate();

    /*AEC operator disconnection*/
    Kymera_DisconnectAudioInputFromAec();

    mic_in0 = Kymera_GetMicrophoneSource(mic0, NULL, sample_rate, high_priority_user);

    /* Disconnect Microphone */
    StreamDisconnect(mic_in0, 0);
    Kymera_CloseMicrophone(mic0, high_priority_user);
}


void Kymera_ConnectLeakthroughMic(void)
{
    aec_connect_audio_input_t aec_input_param = {0};
    aec_audio_config_t aec_config= {0};
    uint8 mic0;
    uint32 sample_rate;

    sample_rate=kymera_GetLeakthroughMicSampleRate();

    /* Configure leakthrough microphone */
    mic0= appConfigLeakthroughMic();
    aec_input_param.mic_input_1= Kymera_GetMicrophoneSource(mic0, NULL, sample_rate, high_priority_user);
    aec_config.mic_sample_rate = sample_rate;
    aec_config.is_source_clock_same = TRUE; //Same clock source for speaker and mic path for AEC-leakthrough.
                                        //Should have no implication on normal aec operation.
    aec_config.buffer_size = DEFAULT_TERMINAL_BUFFER_SIZE;


    /* Mic connection */
    Kymera_ConnectAudioInputToAec(&aec_input_param,&aec_config);
}

void Kymera_CreateLeakthroughChain(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("KymeraLeakthrough_CreateChain");

    theKymera->output_rate = LEAKTHROUGH_OUTPUT_RATE;

    Kymera_SetAecUseCase(aec_usecase_standalone_leakthrough);
    appKymeraCreateOutputChain(KICK_PERIOD_LEAKTHROUGH,0,0);

    ChainStart(theKymera->chainu.output_vol_handle);
    Kymera_ConnectLeakthroughMic();

    /* Ensure audio amp is on */
    appKymeraExternalAmpControl(TRUE);

    appKymeraConfigureDspPowerMode();
    kymera_UpdateStandaloneLeakthroughStatus(ENABLED);
    Kymera_LeakthroughEnableAecSideToneAfterTimeout();
}

void Kymera_DestroyLeakthroughChain(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("Kymera_DestroyLeakthroughChain");

    /* Set Minimum sidetone gain for AEC ref Operator*/
    Kymera_SetMinLeakthroughSidetoneGain();

    /* Disable the sidetone path*/
    Kymera_LeakthroughEnableAecSideTonePath(FALSE);

    /* Disconnect the leakthrough mic*/
    Kymera_DisconnectLeakthroughMic();

    /* Get the DAC */
    Sink DAC_SNK_L = (Sink)PanicFalse(StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));

    /* Disconnect Dac */
    StreamDisconnect(0, DAC_SNK_L);

    /* Destroy the output chain */
    appKymeraDestroyOutputChain();

    theKymera->output_rate = 0;

    /* Audio amp needs to be turned OFF */
    appKymeraExternalAmpControl(FALSE);

    kymera_UpdateStandaloneLeakthroughStatus(DISABLED);
    Kymera_SetAecUseCase(aec_usecase_default);
    appKymeraSetState(KYMERA_STATE_IDLE);
}

void Kymera_SetMinLeakthroughSidetoneGain(void)
{
    Kymera_AecSetSidetoneGain(SIDETONE_EXP_MINIMUM,SIDETONE_MANTISSA_MINIMUM);
}

void Kymera_LeakthroughEnableAecSideTonePath(bool enable)
{
    Kymera_AecEnableSidetonePath(enable);
}


void Kymera_LeakthroughEnableAecSideToneAfterTimeout(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    MessageCancelAll(&theKymera->task,KYMERA_INTERNAL_LEAKTHROUGH_SIDETONE_ENABLE);
    MessageSendLater(&theKymera->task,KYMERA_INTERNAL_LEAKTHROUGH_SIDETONE_ENABLE,NULL,AEC_REF_SETTLING_TIME);
}

bool Kymera_IsStandaloneLeakthroughActive(void)
{
    DEBUG_LOG("Kymera_IsStandaloneLeakthroughActive");
    return kymera_standalone_leakthrough_status;
}

void Kymera_LeakthroughStopChainIfRunning(void)
{
    if(Kymera_IsStandaloneLeakthroughActive())
    {
        Kymera_DestroyLeakthroughChain();
    }
}

void Kymera_LeakthroughResumeChainIfSuspended(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("Kymera_LeakthroughResumeChainIfSuspended");
    DEBUG_LOG("Kymera State is %u", theKymera->state);
    if(AecLeakthrough_IsLeakthroughEnabled() && (theKymera->state == KYMERA_STATE_IDLE))
    {
        Kymera_SetAecUseCase(aec_usecase_standalone_leakthrough);
        Kymera_CreateLeakthroughChain();
        appKymeraSetState(KYMERA_STATE_STANDALONE_LEAKTHROUGH);
    }
}

void Kymera_LeakthroughUpdateAecOperatorUcid(void)
{
    if(AecLeakthrough_IsLeakthroughEnabled())
    {
        uint8 ucid;
        Operator aec_ref = Kymera_GetAecOperator();
        ucid = Kymera_GetAecUcid();
        OperatorsStandardSetUCID(aec_ref,ucid);
    }
}

void Kymera_EnableLeakthrough(void)
{
    DEBUG_LOG("Kymera_EnableLeakthrough");
    kymeraTaskData *theKymera = KymeraGetTaskData();

    if(Kymera_IsVaCaptureActive())
    {
        Kymera_SetAecUseCase(aec_usecase_va_leakthrough);
        Kymera_LeakthroughUpdateAecOperatorUcid();
        Kymera_LeakthroughEnableAecSideToneAfterTimeout();
    }
    else
    {
        switch(theKymera->state)
        {
            case KYMERA_STATE_IDLE:
            case KYMERA_STATE_TONE_PLAYING:
                 MessageSendConditionally(&theKymera->task,KYMERA_INTERNAL_AEC_LEAKTHROUGH_CREATE_STANDALONE_CHAIN,NULL,&theKymera->lock);
                break;

            case KYMERA_STATE_A2DP_STREAMING:
            case KYMERA_STATE_A2DP_STREAMING_WITH_FORWARDING:
                Kymera_SetAecUseCase(aec_usecase_a2dp_leakthrough);
            	Kymera_ConnectLeakthroughMic();
                Kymera_LeakthroughEnableAecSideToneAfterTimeout();
                break;

            case KYMERA_STATE_SCO_ACTIVE:
            case KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING:
            case KYMERA_STATE_SCO_SLAVE_ACTIVE:
                Kymera_SetAecUseCase(aec_usecase_sco_leakthrough);
                Kymera_LeakthroughUpdateAecOperatorUcid();
                Kymera_LeakthroughEnableAecSideToneAfterTimeout();
                break;

            default:
                break;
        }
    }
}

void Kymera_DisableLeakthrough(void)
{
    DEBUG_LOG("Kymera_DisableLeakthrough");
    kymeraTaskData *theKymera = KymeraGetTaskData();

    /* cancel all the pending messsage used for leakthrough ramp */
    MessageCancelAll(&theKymera->task,KYMERA_INTERNAL_LEAKTHROUGH_SIDETONE_GAIN_RAMPUP);

    /* Reset the gain index used for leakthrough ramp*/
    Kymera_LeakthroughResetGainIndex();

    if(Kymera_IsVaCaptureActive())
    {
        Kymera_SetMinLeakthroughSidetoneGain();
        Kymera_LeakthroughEnableAecSideTonePath(FALSE);
        Kymera_SetAecUseCase(aec_usecase_voice_assistant);
        Kymera_LeakthroughUpdateAecOperatorUcid();
    }
    else
    {
        switch (theKymera->state)
        {
            case KYMERA_STATE_STANDALONE_LEAKTHROUGH:
                MessageSendConditionally(&theKymera->task,KYMERA_INTERNAL_AEC_LEAKTHROUGH_DESTROY_STANDALONE_CHAIN,NULL,&theKymera->lock);
                break;

            case KYMERA_STATE_A2DP_STREAMING:
            case KYMERA_STATE_A2DP_STREAMING_WITH_FORWARDING:
            	Kymera_SetMinLeakthroughSidetoneGain();
                Kymera_LeakthroughEnableAecSideTonePath(FALSE);
                Kymera_DisconnectLeakthroughMic();
                Kymera_SetAecUseCase(aec_usecase_default);
                Kymera_LeakthroughUpdateAecOperatorUcid();
                break;

            case KYMERA_STATE_SCO_ACTIVE:
            case KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING:
            case KYMERA_STATE_SCO_SLAVE_ACTIVE:
                Kymera_SetMinLeakthroughSidetoneGain();
                Kymera_LeakthroughEnableAecSideTonePath(FALSE);
                Kymera_SetAecUseCase(aec_usecase_voice_call);
                Kymera_LeakthroughUpdateAecOperatorUcid();
                break;

            default:
                break;
        }
    }
}

void Kymera_LeakthroughInit(void)
{
    kymera_leakthrough_mic_sample_rate = DEFAULT_MIC_RATE;
    kymera_standalone_leakthrough_status = DISABLED;
}
#endif
