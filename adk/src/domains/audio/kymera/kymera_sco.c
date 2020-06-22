/*!
\copyright  Copyright (c) 2017 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_sco.c
\brief      Kymera SCO
*/

#include <vmal.h>
#include <packetiser_helper.h>
#include <anc_state_manager.h>

#include "kymera_config.h"
#include "kymera_private.h"
#include "kymera_aec.h"
#include "kymera_cvc.h"
#include "av.h"

#include <scofwd_profile.h>
#include "scofwd_profile_config.h"

#define AWBSDEC_SET_BITPOOL_VALUE    0x0003
#define AWBSENC_SET_BITPOOL_VALUE    0x0001

#define AEC_TX_BUFFER_SIZE_MS 15

#define SCOFWD_BASIC_PASS_BUFFER_SIZE 512





typedef struct set_bitpool_msg_s
{
    uint16 id;
    uint16 bitpool;
}set_bitpool_msg_t;


void OperatorsAwbsSetBitpoolValue(Operator op, uint16 bitpool, bool decoder)
{
    set_bitpool_msg_t bitpool_msg;
    bitpool_msg.id = decoder ? AWBSDEC_SET_BITPOOL_VALUE : AWBSENC_SET_BITPOOL_VALUE;
    bitpool_msg.bitpool = (uint16)(bitpool);

    PanicFalse(VmalOperatorMessage(op, &bitpool_msg, SIZEOF_OPERATOR_MESSAGE(bitpool_msg), NULL, 0));
}


kymera_chain_handle_t appKymeraGetScoChain(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    if (theKymera)
    {
        return theKymera->chainu.sco_handle;
    }
    return (kymera_chain_handle_t)NULL;
}

/* Set the SPC switch to a specific input
    Inputs are renumbered 1,2 etc.
    0 will equal consume
 */
void appKymeraSelectSpcSwitchInput(Operator op, micSelection input)
{
    uint16 msg[2];
    msg[0] = 5 /* Temporary as OPMSG_SPC_ID_SELECT_PASSTHROUGH not avaioable */;
    msg[1] = input;
    PanicFalse(OperatorMessage(op, msg, 2, NULL, 0));
}

Operator appKymeraGetMicSwitch(void)
{
    kymera_chain_handle_t chain = appKymeraGetScoChain();

    if (chain)
    {
        return ChainGetOperatorByRole(chain, OPR_MICFWD_SPC_SWITCH);
    }
    return INVALID_OPERATOR;
}

void appKymeraSwitchSelectMic(micSelection mic)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    /* always remember currently selected mic */
    theKymera->mic = mic;

    if (appKymeraGetState() == KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING)
    {
        Operator spc_switch = appKymeraGetMicSwitch();
        if (spc_switch)
        {
            DEBUG_LOG("appKymeraSwitchSelectMic %u", mic);
            /* Tell the peer to start or stop sending MIC data */
            ScoFwdMicForwardingEnable(mic == MIC_SELECTION_REMOTE);
            appKymeraSelectSpcSwitchInput(spc_switch, mic);
        }
        else
        {
            DEBUG_LOG("appKymeraSwitchSelectMic failed to get OPR_MICFWD_SPC_SWITCH");
        }
    }
    else
    {
        DEBUG_LOG("appKymeraSwitchSelectMic invalid state to switch mic %u", appKymeraGetState());
    }
}

void appKymeraSetTerminalBufferSize(Operator op, uint32 rate, uint32 buffer_size_ms,
                                           uint16 input_terminals, uint16 output_terminals)
{
    uint16 msg[4];
    msg[0] = OPMSG_COMMON_ID_SET_TERMINAL_BUFFER_SIZE;
    msg[1] = (rate * buffer_size_ms) / 1000;
    msg[2] = input_terminals;
    msg[3] = output_terminals;
    OperatorMessage(op, msg, ARRAY_DIM(msg), NULL, 0);
}

const appKymeraScoChainInfo *appKymeraScoFindChain(const appKymeraScoChainInfo *info, appKymeraScoMode mode, bool sco_fwd, bool mic_fwd, bool cvc_2_mic)
{
    while (info->mode != NO_SCO)
    {        
        if ((info->mode == mode) && (info->cvc_2_mic == cvc_2_mic) &&
            (info->sco_fwd == sco_fwd) && (info->mic_fwd == mic_fwd))
        {
            return info;               
        }
        
        info++;
    }
    return NULL;
}

static void appKymeraScoConfigureChain(uint16 wesco)
{
    kymera_chain_handle_t sco_chain = appKymeraGetScoChain();
    kymeraTaskData *theKymera = KymeraGetTaskData();
    PanicNull((void *)theKymera->sco_info);
    
    const uint32_t rate = theKymera->sco_info->rate;

    /* Configure SCO forwarding parts of the chain */
    if (theKymera->sco_info->sco_fwd)
    {   
        /* Confiure SBC encoder bitpool for forwarded audio */
        Operator awbs_op = PanicZero(ChainGetOperatorByRole(sco_chain, OPR_SCOFWD_SEND));
        OperatorsAwbsSetBitpoolValue(awbs_op, SFWD_MSBC_BITPOOL, FALSE);
    
        /* Configure upsampler from 8K to 16K for narrowband, or passthough from wideband */
        if (theKymera->sco_info->mode == SCO_NB)
        {
            PanicFalse(rate == 8000);        
            Operator upsampler_op = PanicZero(ChainGetOperatorByRole(sco_chain, OPR_SCO_UP_SAMPLE));
            OperatorsResamplerSetConversionRate(upsampler_op, rate, 16000);
        }
        else if (theKymera->sco_info->mode == SCO_WB)
        {
            /* Wideband chains add a basic passthrough in place of the resampler.
               This is currently required to avoid issues when the splitter
               is connected directly to the encoder. */
            Operator basic_pass = PanicZero(ChainGetOperatorByRole(sco_chain, OPR_SCOFWD_BASIC_PASS));
            OperatorsStandardSetBufferSizeWithFormat(basic_pass, SCOFWD_BASIC_PASS_BUFFER_SIZE, operator_data_format_pcm);
        }
        else
        {
            /* SCO forwarding attempt on SWB or UWB */   
            Panic();
        }
    
        /* Configure MIC forwarding parts of the chain */        
        if (theKymera->sco_info->mic_fwd)
        {
            /* Setup the MIC receive SPC for encoded data */
            Operator spc_op;
            PanicFalse(GET_OP_FROM_CHAIN(spc_op, sco_chain, OPR_MICFWD_RECV_SPC));
            OperatorsSetSwitchedPassthruEncoding(spc_op, spc_op_format_16bit_with_metadata);
    
            Operator mic_recv = ChainGetOperatorByRole(sco_chain,OPR_MICFWD_RECV);
            OperatorsAwbsSetBitpoolValue(mic_recv, SFWD_MSBC_BITPOOL, TRUE);
            OperatorsStandardSetBufferSizeWithFormat(mic_recv, SFWD_MICFWD_RECV_CHAIN_BUFFER_SIZE, operator_data_format_pcm);
    
            Operator mic_switch = ChainGetOperatorByRole(sco_chain, OPR_MICFWD_SPC_SWITCH);
            OperatorsSetSwitchedPassthruEncoding(mic_switch, spc_op_format_pcm);
    
            /* Resample the incoming mic data to the SCO sample rate if necessary */
            if (theKymera->sco_info->mode == SCO_NB)
            {
                Operator downsampler_op;
                PanicFalse(GET_OP_FROM_CHAIN(downsampler_op, sco_chain, OPR_MIC_DOWN_SAMPLE));
                OperatorsResamplerSetConversionRate(downsampler_op, 16000, 8000);
            }
        }
    
        Operator splitter_op;
        PanicFalse(GET_OP_FROM_CHAIN(splitter_op, sco_chain, OPR_SCOFWD_SPLITTER));
        OperatorsConfigureSplitter(splitter_op, SFWD_SEND_CHAIN_BUFFER_SIZE, TRUE, operator_data_format_pcm);
    
        /* Configure passthrough for encoded data so we can connect. */
        Operator switch_op;
        PanicFalse(GET_OP_FROM_CHAIN(switch_op, sco_chain, OPR_SWITCHED_PASSTHROUGH_CONSUMER));
        appKymeraConfigureScoSpcDataFormat(switch_op,  ADF_GENERIC_ENCODED);
    
        Operator sco_op = PanicZero(ChainGetOperatorByRole(sco_chain, OPR_SCO_RECEIVE));
        OperatorsStandardSetTimeToPlayLatency(sco_op, SFWD_TTP_DELAY_US);
    }
    else
    {
        /*! \todo Need to decide ahead of time if we need any latency.
            Simple enough to do if we are legacy or not. Less clear if
            legacy but no peer connection */
        /* Enable Time To Play if supported */
        if (appConfigScoChainTTP(wesco) != 0)
        {
            Operator sco_op = PanicZero(ChainGetOperatorByRole(sco_chain, OPR_SCO_RECEIVE));
            OperatorsStandardSetTimeToPlayLatency(sco_op, appConfigScoChainTTP(wesco));
            OperatorsStandardSetBufferSize(sco_op, appConfigScoBufferSize(rate));
        }
    }
        
    appKymeraConfigureOutputChainOperators(sco_chain, theKymera->sco_info->rate, KICK_PERIOD_VOICE, 0, 0);
    appKymeraSetOperatorUcids(TRUE);
}

static void appKymeraScoConnectToAecChain(const aec_connect_audio_input_t* input, const aec_connect_audio_output_t* output, const aec_audio_config_t *config)
{
    Kymera_SetAecUseCase(aec_usecase_voice_call);
    /* Connect AEC Input */
    Kymera_ConnectAudioInputToAec(input, config);
    /* Connect AEC Ouput */
    Kymera_ConnectAudioOutputToAec(output, config);
}

static void appKymeraScoDisconnectFromAecChain(void)
{
    /* Disconnect AEC Input */
    Kymera_DisconnectAudioInputFromAec();

    /* Disconnect AEC Ouput */
    Kymera_DisconnectAudioOutputFromAec();

    /* Reset the AEC ref usecase*/
    Kymera_SetAecUseCase(aec_usecase_default);
}

void appKymeraHandleInternalScoStart(Sink sco_snk, const appKymeraScoChainInfo *info,
                                     uint8 wesco, int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    aec_connect_audio_output_t aec_output_param;
    aec_connect_audio_input_t aec_input_param;
    aec_audio_config_t aec_config = {0};

    DEBUG_LOG("appKymeraHandleInternalScoStart, sink 0x%x, mode %u, wesco %u, state %u", sco_snk, info->mode, wesco, appKymeraGetState());

    if (appKymeraGetState() == KYMERA_STATE_TONE_PLAYING)
    {
        /* If there is a tone still playing at this point,
         * it must be an interruptible tone, so cut it off */
        appKymeraTonePromptStop();
    }

    if(appKymeraGetState() == KYMERA_STATE_STANDALONE_LEAKTHROUGH)
    {
        Kymera_LeakthroughStopChainIfRunning();
        appKymeraSetState(KYMERA_STATE_IDLE);
    }

    /* Can't start voice chain if we're not idle */
    PanicFalse(appKymeraGetState() == KYMERA_STATE_IDLE);

    /* SCO chain must be destroyed if we get here */
    PanicNotNull(appKymeraGetScoChain());

    /* Move to SCO active state now, what ever happens we end up in this state
      (even if it's temporary) */
    appKymeraSetState(KYMERA_STATE_SCO_ACTIVE);

    /* Create appropriate SCO chain */
    appKymeraScoCreateChain(info);
    kymera_chain_handle_t sco_chain = PanicNull(appKymeraGetScoChain());

    Source mic_src_1a = Kymera_GetMicrophoneSource(appConfigScoMic1(), NULL, appKymeraGetOptimalMicSampleRate(theKymera->sco_info->rate), high_priority_user);
    Source mic_src_1b = Kymera_GetMicrophoneSource(appConfigScoMic2(), mic_src_1a, appKymeraGetOptimalMicSampleRate(theKymera->sco_info->rate), high_priority_user);

    /* Get left speaker sink */
    Sink spk_snk_left = StreamAudioSink(appConfigLeftAudioHardware(), appConfigLeftAudioInstance(), appConfigLeftAudioChannel());
    SinkConfigure(spk_snk_left, STREAM_CODEC_OUTPUT_RATE, theKymera->sco_info->rate);

    /* Get right speaker sink */
    Sink spk_snk_right=NULL;
    if(appConfigRightAudioHardware()==AUDIO_HARDWARE_CODEC && appConfigRightAudioInstance()==AUDIO_INSTANCE_0 && appConfigRightAudioChannel()==AUDIO_CHANNEL_B)
    {
        spk_snk_right = StreamAudioSink(appConfigRightAudioHardware(), appConfigRightAudioInstance(), appConfigRightAudioChannel());
        SinkConfigure(spk_snk_right, STREAM_CODEC_OUTPUT_RATE, theKymera->sco_info->rate);
    }

    /* Get SCO source from SCO sink */
    Source sco_src = StreamSourceFromSink(sco_snk);

    /* Fill the AEC Ref Output params */
    aec_output_param.speaker_output_1 = spk_snk_left;
    if(spk_snk_right)
    {
        aec_output_param.speaker_output_2=spk_snk_right;
    }
    aec_output_param.input_1 = ChainGetOutput(sco_chain, EPR_SCO_VOL_OUT);

    /* Fill the AEC Ref Input params */
    aec_input_param.mic_input_1 = mic_src_1a;
    aec_input_param.mic_input_2 = mic_src_1b;
    aec_input_param.mic_output_1 = ChainGetInput(sco_chain, EPR_CVC_SEND_IN1);
    aec_input_param.mic_output_2 = (theKymera->sco_info->cvc_2_mic) ? ChainGetInput(sco_chain, EPR_CVC_SEND_IN2) : NULL;
    aec_input_param.reference_output = ChainGetInput(sco_chain, EPR_CVC_SEND_REF_IN);

    /* sample rate */
    aec_config.spk_sample_rate = theKymera->sco_info->rate;
    aec_config.mic_sample_rate = theKymera->sco_info->rate;
    /* terminal buffer size */
    aec_config.buffer_size = AEC_TX_BUFFER_SIZE_MS;
    /* TTP Gate */
     if (!theKymera->sco_info->sco_fwd && appConfigScoChainTTP(wesco) != 0)
     {
#ifndef __QCC514X_APPS__
        aec_config.ttp_gate_delay = 50;
#endif
     }
    /* Get sources and sinks for chain endpoints */
    Source sco_ep_src  = ChainGetOutput(sco_chain, EPR_SCO_TO_AIR);
    Sink sco_ep_snk    = ChainGetInput(sco_chain, EPR_SCO_FROM_AIR);

    PanicFalse(OperatorsFrameworkSetKickPeriod(KICK_PERIOD_VOICE));

    /* Connect to AEC Reference Chain */
    appKymeraScoConnectToAecChain(&aec_input_param, &aec_output_param, &aec_config);

    /* Configure chain specific operators */
    appKymeraScoConfigureChain(wesco);

    /* Connect SCO to chain SCO endpoints */
    StreamConnect(sco_ep_src, sco_snk);
    StreamConnect(sco_src, sco_ep_snk);
    
    /* Connect chain */
    ChainConnect(sco_chain);
   
    /* Chain connection sets the switch into consume mode,
       select the local Microphone if MIC forward enabled */
    if (theKymera->sco_info->mic_fwd)
        appKymeraSelectSpcSwitchInput(appKymeraGetMicSwitch(), MIC_SELECTION_LOCAL);

    /* Enable external amplifier if required */
    appKymeraExternalAmpControl(TRUE);

    /* The chain can fail to start if the SCO source disconnects whilst kymera
    is queuing the SCO start request or starting the chain. If the attempt fails,
    ChainStartAttempt will stop (but not destroy) any operators it started in the chain. */
    if (ChainStartAttempt(sco_chain))
    {
        theKymera->output_rate = theKymera->sco_info->rate;
        appKymeraHandleInternalScoSetVolume(volume_in_db);

        if(theKymera->enable_cvc_passthrough)
        {
            kymera_EnableCvcPassthroughMode();
        }
        if(AecLeakthrough_IsLeakthroughEnabled())
        {
           Kymera_SetAecUseCase(aec_usecase_sco_leakthrough);
           Kymera_LeakthroughUpdateAecOperatorUcid();
           Kymera_LeakthroughEnableAecSideToneAfterTimeout();
        }
    }
    else
    {
        DEBUG_LOG("appKymeraHandleInternalScoStart, could not start chain");
        /* Stop/destroy the chain, returning state to KYMERA_STATE_IDLE.
        This needs to be done here, since between the failed attempt to start
        and the subsequent stop (when appKymeraScoStop() is called), a tone
        may need to be played - it would not be possible to play a tone in a
        stopped SCO chain. The state needs to be KYMERA_STATE_SCO_ACTIVE for
        appKymeraHandleInternalScoStop() to stop/destroy the chain. */
        appKymeraHandleInternalScoStop();
    }
}

void appKymeraHandleInternalScoStop(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraHandleInternalScoStop, state %u", appKymeraGetState());
    
    /* Get current SCO chain */
    kymera_chain_handle_t sco_chain = appKymeraGetScoChain();
    if (appKymeraGetState() != KYMERA_STATE_SCO_ACTIVE)
    {
        if (!sco_chain)        
        {
            /* Attempting to stop a SCO chain when not ACTIVE. This happens when the user
            calls appKymeraScoStop() following a failed attempt to start the SCO
            chain - see ChainStartAttempt() in appKymeraHandleInternalScoStart().
            In this case, there is nothing to do, since the failed start attempt
            cleans up by calling this function in state KYMERA_STATE_SCO_ACTIVE */
            DEBUG_LOG("appKymeraHandleInternalScoStop, not stopping - already idle");
            return;
        }
        else
        {
            Panic();
        }
    }

    /* Stop forwarding first */
    if (appKymeraGetState() == KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING)
        PanicFalse(appKymeraHandleInternalScoForwardingStopTx());

    Source sco_ep_src  = ChainGetOutput(sco_chain, EPR_SCO_TO_AIR);
    Sink sco_ep_snk    = ChainGetInput(sco_chain, EPR_SCO_FROM_AIR);

    /* Disable AEC_REF sidetone path */
    Kymera_LeakthroughEnableAecSideTonePath(FALSE);

    /* A tone still playing at this point must be interruptable */
    appKymeraTonePromptStop();

    /* Stop chains */
    ChainStop(sco_chain);

    /* Disconnect SCO from chain SCO endpoints */
    StreamDisconnect(sco_ep_src, NULL);
    StreamDisconnect(NULL, sco_ep_snk);

    /* Disconnect from AEC chain */
   appKymeraScoDisconnectFromAecChain();

    Kymera_CloseMicrophone(appConfigScoMic1(), high_priority_user);
    Kymera_CloseMicrophone(appConfigScoMic2(), high_priority_user);

    /* Destroy chains */
    ChainDestroy(sco_chain);
    theKymera->chainu.sco_handle = sco_chain = NULL;

    /* Disable external amplifier if required */
    if (!AncStateManager_IsEnabled())
        appKymeraExternalAmpControl(FALSE);

    /* Update state variables */
    appKymeraSetState(KYMERA_STATE_IDLE);
    theKymera->output_rate = 0;

    Kymera_LeakthroughResumeChainIfSuspended();
}
void appKymeraHandleInternalScoSetVolume(int16 volume_in_db)
{
    kymera_chain_handle_t scoChain = appKymeraGetScoChain();

    DEBUG_LOG("appKymeraHandleInternalScoSetVolume, vol %d", volume_in_db);

    switch (KymeraGetTaskData()->state)
    {
        case KYMERA_STATE_SCO_ACTIVE:
        case KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING:
        case KYMERA_STATE_SCO_SLAVE_ACTIVE:
        {
            appKymeraSetMainVolume(scoChain, volume_in_db);
        }
        break;
        default:
            break;
    }
}

void appKymeraHandleInternalScoMicMute(bool mute)
{
    DEBUG_LOG("appKymeraHandleInternalScoMicMute, mute %u", mute);

    switch (KymeraGetTaskData()->state)
    {
        case KYMERA_STATE_SCO_ACTIVE:
        case KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING:
        {
            Operator aec_op = Kymera_GetAecOperator();
            if (aec_op != INVALID_OPERATOR)
            {
                OperatorsAecMuteMicOutput(aec_op, mute);
            }
        }
        break;

        default:
            break;
    }
}

uint8 appKymeraScoVoiceQuality(void)
{
    uint8 quality = appConfigVoiceQualityWorst();

    if (appConfigVoiceQualityMeasurementEnabled())
    {
        Operator cvc_send_op;
        if (GET_OP_FROM_CHAIN(cvc_send_op, appKymeraGetScoChain(), OPR_CVC_SEND))
        {
            uint16 rx_msg[2], tx_msg = OPMSG_COMMON_GET_VOICE_QUALITY;
            PanicFalse(OperatorMessage(cvc_send_op, &tx_msg, 1, rx_msg, 2));
            quality = MIN(appConfigVoiceQualityBest() , rx_msg[1]);
            quality = MAX(appConfigVoiceQualityWorst(), quality);
        }
    }
    else
    {
        quality = appConfigVoiceQualityWhenDisabled();
    }

    DEBUG_LOG("appKymeraScoVoiceQuality %u", quality);

    return quality;
}

