/*!
\copyright  Copyright (c) 2017-2020  Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_common.c
\brief      Kymera common code
*/

#include <audio_clock.h>
#include <audio_power.h>
#include <pio_common.h>
#include <vmal.h>
#include <anc_state_manager.h>

#include "kymera_private.h"
#include "kymera_anc.h"
#include "kymera_aec.h"
#include "kymera_config.h"
#include "kymera_va.h"
#include "av.h"
#include "microphones.h"
#include "microphones_config.h"

/*! Convert a channel ID to a bit mask */
#define CHANNEL_TO_MASK(channel) ((uint32)1 << channel)

/*!@{ \name Port numbers for the Source Sync operator */
#define KYMERA_SOURCE_SYNC_INPUT_PORT (0)
#define KYMERA_SOURCE_SYNC_OUTPUT_PORT (0)

#define KYMERA_SOURCE_SYNC_INPUT_PORT_1 (1)
#define KYMERA_SOURCE_SYNC_OUTPUT_PORT_1 (1)

/*!@} */
#define DEFAULT_AEC_REF_TERMINAL_BUFFER_SIZE 15

#ifdef INCLUDE_KYMERA_AEC
#define isAecUsedInOutputChain() TRUE
#else
#define isAecUsedInOutputChain() FALSE
#endif

static uint8 audio_ss_client_count = 0;

/*! \brief The chain output channels */
typedef enum
{
    output_right_channel = 0,
    output_left_channel
}output_channel_t;

/* Configuration of source sync groups and routes */
static const source_sync_sink_group_t sink_groups[] =
{
    {
        .meta_data_required = TRUE,
        .rate_match = FALSE,
        .channel_mask = CHANNEL_TO_MASK(KYMERA_SOURCE_SYNC_INPUT_PORT)
    }
#if INCLUDE_STEREO
    ,
    {
        .meta_data_required = TRUE,
        .rate_match = FALSE,
        .channel_mask = CHANNEL_TO_MASK(KYMERA_SOURCE_SYNC_INPUT_PORT_1)
    }
#endif
};

static const source_sync_source_group_t source_groups[] =
{
    {
        .meta_data_required = TRUE,
        .ttp_required = TRUE,
        .channel_mask = CHANNEL_TO_MASK(KYMERA_SOURCE_SYNC_OUTPUT_PORT)
    }
#if INCLUDE_STEREO
    ,
    {
        .meta_data_required = TRUE,
        .ttp_required = TRUE,
        .channel_mask = CHANNEL_TO_MASK(KYMERA_SOURCE_SYNC_OUTPUT_PORT_1)
    }
#endif
};

static source_sync_route_t routes[] =
{
    {
        .input_terminal = KYMERA_SOURCE_SYNC_INPUT_PORT,
        .output_terminal = KYMERA_SOURCE_SYNC_OUTPUT_PORT,
        .transition_samples = 0,
        .sample_rate = 0, /* Overridden later */
        .gain = 0
    }
#if INCLUDE_STEREO
    ,
    {
        .input_terminal = KYMERA_SOURCE_SYNC_INPUT_PORT_1,
        .output_terminal = KYMERA_SOURCE_SYNC_OUTPUT_PORT_1,
        .transition_samples = 0,
        .sample_rate = 0, /* Overridden later */
        .gain = 0
    }
#endif
};

static bool isStereo(void)
{
#ifdef INCLUDE_STEREO
    return TRUE;
#else
    return FALSE;
#endif
}

static void connectOutputChainToAudioSink(Source chain_output, Sink audio_sink)
{
#ifdef INCLUDE_KYMERA_AEC
    aec_connect_audio_output_t aec_connect_params = {0};
    aec_audio_config_t config = {0};
    aec_connect_params.input_1 = chain_output;
    aec_connect_params.speaker_output_1 = audio_sink;

    config.spk_sample_rate = KymeraGetTaskData()->output_rate;

#ifdef ENABLE_AEC_LEAKTHROUGH
    config.is_source_clock_same = TRUE; //Same clock source for speaker and mic path for AEC-leakthrough.
                                        //Should have no implication on normal aec operation.
    config.buffer_size = DEFAULT_AEC_REF_TERMINAL_BUFFER_SIZE;
#endif

    Kymera_ConnectAudioOutputToAec(&aec_connect_params, &config);
#else
    PanicNull(StreamConnect(chain_output, audio_sink));
#endif
}

static void disconnectOutputChainFromAudioSink(Source chain_output)
{
#ifdef INCLUDE_KYMERA_AEC
    UNUSED(chain_output);
    Kymera_DisconnectAudioOutputFromAec();
#else
    StreamDisconnect(chain_output, NULL);
#endif
}


int32 volTo60thDbGain(int16 volume_in_db)
{
    int32 gain = VOLUME_MUTE_IN_DB;
    if (volume_in_db > appConfigMinVolumedB())
    {
        gain = volume_in_db;
        if(gain > appConfigMaxVolumedB())
        {
            gain = appConfigMaxVolumedB();
        }
    }
    return (gain * KYMERA_DB_SCALE);
}

void appKymeraSetState(appKymeraState state)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG_STATE("appKymeraSetState, state %u -> %u", theKymera->state, state);
    theKymera->state = state;
    KymeraAnc_PreStateTransition(state);
    /* Set busy lock if not in idle or tone state */
    theKymera->busy_lock = (state != KYMERA_STATE_IDLE) && (state != KYMERA_STATE_TONE_PLAYING) && (state != KYMERA_STATE_STANDALONE_LEAKTHROUGH);
}

void appKymeraConfigureDspPowerMode(void)
{
#if (defined(__QCC302X_APPS__) || defined(__QCC512X_APPS__) || defined(__QCC3400_APP__) || defined(__QCC514X_APPS__))
    kymeraTaskData *theKymera = KymeraGetTaskData();
    bool tone_playing = appKymeraIsPlayingPrompt();

    DEBUG_LOG("appKymeraConfigureDspPowerMode, tone %u, state %u, a2dp seid %u", tone_playing, appKymeraGetState(), theKymera->a2dp_seid);
    
    /* Assume we are switching to the low power slow clock unless one of the
     * special cases below applies */
    audio_dsp_clock_configuration cconfig =
    {
        .active_mode = AUDIO_DSP_SLOW_CLOCK,
        .low_power_mode =  AUDIO_DSP_SLOW_CLOCK,
        .trigger_mode = AUDIO_DSP_CLOCK_NO_CHANGE
    };
    
    audio_dsp_clock kclocks;
    audio_power_save_mode mode = AUDIO_POWER_SAVE_MODE_3;

    switch (appKymeraGetState())
    {
        case KYMERA_STATE_A2DP_STARTING_A:
        case KYMERA_STATE_A2DP_STARTING_B:
        case KYMERA_STATE_A2DP_STARTING_C:
        case KYMERA_STATE_A2DP_STARTING_SLAVE:
        case KYMERA_STATE_A2DP_STREAMING:
        case KYMERA_STATE_A2DP_STREAMING_WITH_FORWARDING:
        case KYMERA_STATE_STANDALONE_LEAKTHROUGH:
        {
            if(AncStateManager_CheckIfDspClockBoostUpRequired())
            {
                cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                mode = AUDIO_POWER_SAVE_MODE_1;
            }
            else if(Kymera_IsVaCaptureActive())
            {
                cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                mode = AUDIO_POWER_SAVE_MODE_1;
            }
            else if(tone_playing)
            {
                /* Always jump up to normal clock for tones - for most codecs there is
                * not enough MIPs when running on a slow clock to also play a tone */
                cconfig.active_mode = AUDIO_DSP_BASE_CLOCK;
                mode = AUDIO_POWER_SAVE_MODE_1;
            }
            else
            {
                /* Either setting up for the first time or returning from a tone, in
                * either case return to the default clock rate for the codec in use */
                switch(theKymera->a2dp_seid)
                {
                    case AV_SEID_APTX_SNK:
                    case AV_SEID_APTX_ADAPTIVE_SNK:
                    case AV_SEID_APTX_ADAPTIVE_TWS_SNK:
                    {
                        /* Not enough MIPs to run aptX master (TWS standard) or
                        * aptX adaptive (TWS standard and TWS+) on slow clock */
                        cconfig.active_mode = AUDIO_DSP_BASE_CLOCK;
                        mode = AUDIO_POWER_SAVE_MODE_1;
                    }
                    break;

                    case AV_SEID_SBC_SNK:
                    {
                        if (isAecUsedInOutputChain() || appConfigSbcNoPcmLatencyBuffer())
                        {
                            cconfig.active_mode = AUDIO_DSP_BASE_CLOCK;
                            mode = AUDIO_POWER_SAVE_MODE_1;
                        }
                    }
                    break;

                    case AV_SEID_APTX_MONO_TWS_SNK:
                    {
                        if (isAecUsedInOutputChain())
                        {
                            cconfig.active_mode = AUDIO_DSP_BASE_CLOCK;
                            mode = AUDIO_POWER_SAVE_MODE_1;
                        }
                    }
                    break;

                    default:
                    break;
                }
            }
        }
        break;

        case KYMERA_STATE_SCO_ACTIVE:
        case KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING:
        case KYMERA_STATE_SCO_SLAVE_ACTIVE:
        {
            DEBUG_LOG("appKymeraConfigureDspPowerMode, sco_info %u, mode %u", theKymera->sco_info, theKymera->sco_info->mode);
         if(AncStateManager_CheckIfDspClockBoostUpRequired())
         {
            cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
            mode = AUDIO_POWER_SAVE_MODE_1;
          }
         else if (theKymera->sco_info)
           {
                switch (theKymera->sco_info->mode)
                {
                    case SCO_NB:
                    case SCO_WB:
                    {
                        /* Always jump up to normal clock (80Mhz) for NB or WB CVC */
                        cconfig.active_mode = AUDIO_DSP_BASE_CLOCK;
                        mode = AUDIO_POWER_SAVE_MODE_1;
                    }
                    break;

                    case SCO_SWB:
                    case SCO_UWB:
                    {
                        /* Always jump up to turbo clock (120Mhz) for SWB or UWB CVC */
                        cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                        mode = AUDIO_POWER_SAVE_MODE_1;
                    }
                    break;

                    default:
                        break;
                }
            }
        }
        break;

        case KYMERA_STATE_ANC_TUNING:
        {
            /* Always jump up to turbo clock (120Mhz) for ANC tuning */
            cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
            mode = AUDIO_POWER_SAVE_MODE_1;
        }
        break;

        case KYMERA_STATE_TONE_PLAYING:
        {
            if(AncStateManager_CheckIfDspClockBoostUpRequired())
            {
                cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                mode = AUDIO_POWER_SAVE_MODE_1;
            }
            else if(Kymera_IsVaCaptureActive())
            {
                cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                mode = AUDIO_POWER_SAVE_MODE_1;
            }
        }
        break;

        /* All other states default to slow */
        case KYMERA_STATE_IDLE:
			if(AncStateManager_CheckIfDspClockBoostUpRequired())
            {
                cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                mode = AUDIO_POWER_SAVE_MODE_1;
            }
            else if(Kymera_IsVaCaptureActive())
            {
                cconfig.active_mode = AUDIO_DSP_TURBO_CLOCK;
                mode = AUDIO_POWER_SAVE_MODE_1;
            }
            break;
    }

#ifdef AUDIO_IN_SQIF
    /* Make clock faster when running from SQIF */
    cconfig.active_mode += 1;
#endif


    PanicFalse(AudioDspClockConfigure(&cconfig));
    PanicFalse(AudioPowerSaveModeSet(mode));

    PanicFalse(AudioDspGetClock(&kclocks));
    mode = AudioPowerSaveModeGet();
    DEBUG_LOG("appKymeraConfigureDspPowerMode, kymera clocks %d %d %d, mode %d", kclocks.active_mode, kclocks.low_power_mode, kclocks.trigger_mode, mode);
#elif defined(__CSRA68100_APP__)
    /* No DSP clock control on CSRA68100 */
#else
    #error DSP clock not handled for this chip
#endif
}

void appKymeraExternalAmpSetup(void)
{
    if (appConfigExternalAmpControlRequired())
    {
        kymeraTaskData *theKymera = KymeraGetTaskData();
        int pio_mask = (1 << (appConfigExternalAmpControlPio() % 32));
        int pio_bank = appConfigExternalAmpControlPio() / 32;

        /* Reset usage count */
        theKymera->dac_amp_usage = 0;

        /* map in PIO */
        PioSetMapPins32Bank(pio_bank, pio_mask, pio_mask);
        /* set as output */
        PioSetDir32Bank(pio_bank, pio_mask, pio_mask);
        /* start disabled */
        PioSet32Bank(pio_bank, pio_mask,
                     appConfigExternalAmpControlDisableMask());
    }
}

void appKymeraExternalAmpControl(bool enable)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    if (appConfigExternalAmpControlRequired())
    {
        theKymera->dac_amp_usage += enable ? 1 : - 1;

        /* Drive PIO high if enabling AMP and usage has gone from 0 to 1,
         * Drive PIO low if disabling AMP and usage has gone from 1 to 0 */
        if ((enable && theKymera->dac_amp_usage == 1) ||
            (!enable && theKymera->dac_amp_usage == 0))
        {
            int pio_mask = (1 << (appConfigExternalAmpControlPio() % 32));
            int pio_bank = appConfigExternalAmpControlPio() / 32;

            PioSet32Bank(pio_bank, pio_mask,
                         enable ? appConfigExternalAmpControlEnableMask() :
                                  appConfigExternalAmpControlDisableMask());
        }
    }

    if(enable)
    {
        /* If we're enabling the amp then also call OperatorFrameworkEnable() so that the audio S/S will
           remain on even if the audio chain is destroyed, this allows us to control the timing of when the audio S/S
           and DACs are powered off to mitigate audio pops and clicks.*/

        /* Cancel any pending audio s/s disable message since we're enabling.  If message was cancelled no need
           to call OperatorFrameworkEnable() as audio S/S is still powered on from previous time */
        if(MessageCancelFirst(&theKymera->task, KYMERA_INTERNAL_AUDIO_SS_DISABLE))
        {
            DEBUG_LOG("appKymeraExternalAmpControl, there is already a client for the audio SS");
        }
        else
        {
            DEBUG_LOG("appKymeraExternalAmpControl, adding a client to the audio SS");
            OperatorsFrameworkEnable();
        }

        audio_ss_client_count++;
    }
    else
    {
        if (audio_ss_client_count > 1)
        {
            OperatorsFrameworkDisable();
            audio_ss_client_count--;
            DEBUG_LOG("appKymeraExternalAmpControl, removed audio source, count is %d", audio_ss_client_count);
        }
        else
        {
            /* If we're disabling the amp then send a timed message that will turn off the audio s/s later rather than 
            immediately */
            DEBUG_LOG("appKymeraExternalAmpControl, sending later KYMERA_INTERNAL_AUDIO_SS_DISABLE, count is %d", audio_ss_client_count);
            MessageSendLater(&theKymera->task, KYMERA_INTERNAL_AUDIO_SS_DISABLE, NULL, appKymeraDacDisconnectionDelayMs());
            audio_ss_client_count = 0;
        }
    }
}

void appKymeraConfigureSpcMode(Operator op, bool is_consumer)
{
    uint16 msg[2];
    msg[0] = OPMSG_SPC_ID_TRANSITION; /* MSG ID to set SPC mode transition */
    msg[1] = is_consumer;
    PanicFalse(OperatorMessage(op, msg, 2, NULL, 0));
}

void appKymeraConfigureScoSpcDataFormat(Operator op, audio_data_format format)
{
    uint16 msg[2];
    msg[0] = OPMSG_OP_TERMINAL_DATA_FORMAT; /* MSG ID to set SPC data type */
    msg[1] = format;
    PanicFalse(OperatorMessage(op, msg, 2, NULL, 0));
}

static void appKymeraConfigureSourceSync(Operator op, uint32 rate, unsigned kick_period_us)
{
    /* Override sample rate in routes config */
    routes[0].sample_rate = rate;

    if(isStereo())
        routes[1].sample_rate = rate;

    /* Send operator configuration messages */
    OperatorsStandardSetSampleRate(op, rate);
    OperatorsSourceSyncSetSinkGroups(op, DIMENSION_AND_ADDR_OF(sink_groups));
    OperatorsSourceSyncSetSourceGroups(op, DIMENSION_AND_ADDR_OF(source_groups));
    OperatorsSourceSyncSetRoutes(op, DIMENSION_AND_ADDR_OF(routes));

    /* Output buffer needs to be able to hold at least SS_MAX_PERIOD worth
     * of audio (default = 2 * Kp), but be less than SS_MAX_LATENCY (5 * Kp).
     * The recommendation is 2 Kp more than SS_MAX_PERIOD, so 4 * Kp. */
    OperatorsStandardSetBufferSize(op, US_TO_BUFFER_SIZE_MONO_PCM(4 * kick_period_us, rate));
}

void appKymeraSetMainVolume(kymera_chain_handle_t chain, int16 volume_in_db)
{
    Operator volop;

    if (GET_OP_FROM_CHAIN(volop, chain, OPR_VOLUME_CONTROL))
    {
        OperatorsVolumeSetMainGain(volop, volTo60thDbGain(volume_in_db));
    }
}

void appKymeraSetVolume(kymera_chain_handle_t chain, int16 volume_in_db)
{
    Operator volop;

    if (GET_OP_FROM_CHAIN(volop, chain, OPR_VOLUME_CONTROL))
    {
        OperatorsVolumeSetMainAndAuxGain(volop, volTo60thDbGain(volume_in_db));
    }
}

void appKymeraConfigureOutputChainOperators(kymera_chain_handle_t chain,
                                            uint32 sample_rate, unsigned kick_period,
                                            unsigned buffer_size, int16 volume_in_db)
{
    Operator sync_op;
    Operator volume_op;

    /* Configure operators */
    if (GET_OP_FROM_CHAIN(sync_op, chain, OPR_SOURCE_SYNC))
    {
        /* SourceSync is optional in chains. */
        appKymeraConfigureSourceSync(sync_op, sample_rate, kick_period);
    }

    volume_op = ChainGetOperatorByRole(chain, OPR_VOLUME_CONTROL);
    OperatorsStandardSetSampleRate(volume_op, sample_rate);

    appKymeraSetVolume(chain, volume_in_db);

    if (buffer_size)
    {
        Operator op = ChainGetOperatorByRole(chain, OPR_LATENCY_BUFFER);
        OperatorsStandardSetBufferSize(op, buffer_size);
    }
}

static chain_endpoint_role_t appkymera_GetOuputRole(output_channel_t is_left)
{
    chain_endpoint_role_t output_role;

    if(isStereo())
    {
        output_role = is_left ? EPR_SOURCE_STEREO_OUTPUT_L : EPR_SOURCE_STEREO_OUTPUT_R;
    }
    else
    {
        output_role = EPR_SOURCE_MIXER_OUT;
    }

    return output_role;
}

void appKymeraCreateOutputChain(unsigned kick_period, unsigned buffer_size, int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    Sink dac_left =NULL;
    Sink dac_right = NULL;
    kymera_chain_handle_t chain;

    DEBUG_LOG("appKymeraCreateOutputChain");

    /* Setting leakthrough mic path sample rate same as speaker path sampling rate */
    Kymera_setLeakthroughMicSampleRate(theKymera->output_rate);

    /* Create chain */
    chain = ChainCreate(Kymera_GetChainConfigs()->chain_output_volume_config);
    theKymera->chainu.output_vol_handle = chain;
    appKymeraConfigureOutputChainOperators(chain, theKymera->output_rate, kick_period, buffer_size, volume_in_db);
    PanicFalse(OperatorsFrameworkSetKickPeriod(kick_period));
    
    ChainConnect(chain);

    /* Configure the DAC channel */
    dac_left = StreamAudioSink(appConfigLeftAudioHardware(), appConfigLeftAudioInstance(), appConfigLeftAudioChannel());
    DEBUG_LOG("appKymeraCreateOutputChain. Output rate %d", theKymera->output_rate);
    PanicFalse(SinkConfigure(dac_left, STREAM_CODEC_OUTPUT_RATE, theKymera->output_rate));
    PanicFalse(SinkConfigure(dac_left, STREAM_RM_ENABLE_DEFERRED_KICK, 0));

    connectOutputChainToAudioSink(ChainGetOutput(chain, appkymera_GetOuputRole(output_left_channel)), dac_left);

    /* Configure the right DAC channel */
    if(isStereo())
    {
        dac_right = StreamAudioSink(appConfigRightAudioHardware(), appConfigRightAudioInstance(), appConfigRightAudioChannel());
        PanicFalse(SinkConfigure(dac_right, STREAM_CODEC_OUTPUT_RATE, theKymera->output_rate));
        PanicFalse(SinkConfigure(dac_right, STREAM_RM_ENABLE_DEFERRED_KICK, 0));

        connectOutputChainToAudioSink(ChainGetOutput(chain, appkymera_GetOuputRole(output_right_channel)), dac_right);
    }
}

void appKymeraDestroyOutputChain(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    kymera_chain_handle_t chain = theKymera->chainu.output_vol_handle;
    Kymera_setLeakthroughMicSampleRate(DEFAULT_MIC_RATE);

    DEBUG_LOG("appKymeraDestroyOutputChain");

    if (chain)
    {
#ifdef INCLUDE_MIRRORING
        /* Destroying the output chain powers-off the DSP, if another
        prompt or activity is pending, the DSP has to start all over again
        which takes a long time. Therefore prospectively power on the DSP
        before destroying the output chain, which will avoid an unneccesary
        power-off/on */
        appKymeraProspectiveDspPowerOn();
#endif

        ChainStop(chain);
        disconnectOutputChainFromAudioSink(ChainGetOutput(chain, appkymera_GetOuputRole(output_left_channel)));

        if(isStereo())
        {
            Source right_source = ChainGetOutput(chain, appkymera_GetOuputRole(output_right_channel));
            if(right_source != NULL)
                disconnectOutputChainFromAudioSink(right_source);
        }

        ChainDestroy(chain);
        theKymera->chainu.output_vol_handle = NULL;

    }
}

/*! \brief Set the UCID for a single operator */
static bool appKymeraSetOperatorUcid(kymera_chain_handle_t chain, chain_operator_role_t role, kymera_operator_ucid_t ucid)
{
    Operator op;
    if (GET_OP_FROM_CHAIN(op, chain, role))
    {
        OperatorsStandardSetUCID(op, ucid);
        return TRUE;
    }
    return FALSE;
}

void appKymeraSetOperatorUcids(bool is_sco)
{
    /* Operators that have UCID set either reside in sco_handle or output_vol_handle
       which are both in the chainu union. */
    kymera_chain_handle_t chain = KymeraGetTaskData()->chainu.output_vol_handle;

    if (is_sco)
    {
        /* SCO/MIC forwarding RX chains do not have CVC send or receive */
        appKymeraSetOperatorUcid(chain, OPR_CVC_SEND, UCID_CVC_SEND);
        appKymeraSetOperatorUcid(chain, OPR_CVC_RECEIVE, UCID_CVC_RECEIVE);
    }

    /* Operators common to SCO/A2DP chains */
    PanicFalse(appKymeraSetOperatorUcid(chain, OPR_VOLUME_CONTROL, UCID_VOLUME_CONTROL));
    PanicFalse(appKymeraSetOperatorUcid(chain, OPR_SOURCE_SYNC, UCID_SOURCE_SYNC));
}

Source Kymera_GetMicrophoneSource(microphone_number_t microphone_number, Source source_to_synchronise_with, uint32 sample_rate, microphone_user_type_t microphone_user_type)
{
    Source mic_source = NULL;
    if(microphone_number != microphone_none)
    {
        mic_source = Microphones_TurnOnMicrophone(microphone_number, sample_rate, microphone_user_type);
    }
    if(mic_source && source_to_synchronise_with)
    {
        SourceSynchronise(source_to_synchronise_with, mic_source);
    }
    return mic_source;
}

void Kymera_CloseMicrophone(microphone_number_t microphone_number, microphone_user_type_t microphone_user_type)
{
    if(microphone_number != microphone_none)
    {
        Microphones_TurnOffMicrophone(microphone_number, microphone_user_type);
    }
}

unsigned AudioConfigGetMicrophoneBiasVoltage(mic_bias_id id)
{
    unsigned bias = 0;
    if (id == MIC_BIAS_0)
    {
        if (appConfigMic0Bias() == BIAS_CONFIG_MIC_BIAS_0)
            bias =  appConfigMic0BiasVoltage();
        else if (appConfigMic1Bias() == BIAS_CONFIG_MIC_BIAS_0)
            bias = appConfigMic1BiasVoltage();
        else
            Panic();
    }
    else if (id == MIC_BIAS_1)
    {
        if (appConfigMic0Bias() == BIAS_CONFIG_MIC_BIAS_1)
            bias = appConfigMic0BiasVoltage();
        else if (appConfigMic1Bias() == BIAS_CONFIG_MIC_BIAS_1)
            bias = appConfigMic1BiasVoltage();
        else
            Panic();
    }
    else
        Panic();

    DEBUG_LOG("AudioConfigGetMicrophoneBiasVoltage, id %u, bias %u", id, bias);
    return bias;
}

void AudioConfigSetRawDacGain(audio_output_t channel, uint32 raw_gain)
{
    DEBUG_LOG("AudioConfigSetRawDacGain, channel %u, gain %u", channel, raw_gain);
    if (channel == audio_output_primary_left)
    {
        Sink sink = StreamAudioSink(appConfigLeftAudioHardware(), appConfigLeftAudioInstance(), appConfigLeftAudioChannel());
        PanicFalse(SinkConfigure(sink, STREAM_CODEC_RAW_OUTPUT_GAIN, raw_gain));
    }
    else
        Panic();
}

uint32 appKymeraGetOptimalMicSampleRate(uint32 sample_rate)
{

#ifdef ENABLE_ADAPTIVE_ANC
    sample_rate = ADAPTIVE_ANC_MIC_SAMPLE_RATE;
#endif

    return sample_rate;
}




