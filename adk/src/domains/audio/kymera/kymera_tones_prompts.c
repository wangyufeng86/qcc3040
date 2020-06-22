/*!
\copyright  Copyright (c) 2017 - 2019  Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_prompts.c
\brief      Kymera tones / prompts
*/

#include "kymera_private.h"
#include "kymera_config.h"
#include "kymera_aec.h"
#include "system_clock.h"

#define VOLUME_CONTROL_SET_AUX_TTP_VERSION_MSB 0x2
#define BUFFER_SIZE_FACTOR 4

static enum
{
    kymera_tone_idle,
    kymera_tone_playing
} kymera_tone_state = kymera_tone_idle;

/*! \brief Setup the prompt audio source.
    \param source The prompt audio source.
*/
static void appKymeraPromptSourceSetup(Source source)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    MessageStreamTaskFromSource(source, &theKymera->task);
}

/*! \brief Close the prompt audio source.
    \param source The prompt audio source.
*/
static void appKymeraPromptSourceClose(Source source)
{
    if (source)
    {
        MessageStreamTaskFromSource(source, NULL);
        StreamDisconnect(source, NULL);
        SourceClose(source);
    }
}

/*! \brief Calculates buffer size for tone/vp chain.

    Pessimistic buffer calculation, it assumes slow
    clock rare which used most often. Also BUFFER_SIZE_FACTOR = 4
    is conservative, 2 may also work.

    \param output_rate Kymera output rate in Hz.

 */
static unsigned appKymeraCalculateBufferSize(unsigned output_rate)
{
    unsigned scaled_rate = output_rate / 1000;
    return (KICK_PERIOD_SLOW * scaled_rate * BUFFER_SIZE_FACTOR) / 1000;
}

/*! \brief Create the tone / prompt audio chain.
    \param msg Message containing the create parameters.
*/
static Source appKymeraCreateTonePromptChain(const KYMERA_INTERNAL_TONE_PROMPT_PLAY_T *msg)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    kymera_chain_handle_t chain = NULL;
    const chain_config_t *config = NULL;
    const bool has_resampler = (theKymera->output_rate != msg->rate);
    const bool is_tone = (msg->tone != NULL);
    const bool is_prompt = (msg->prompt != FILE_NONE);

    /* Can play tone or prompt, not both */
    PanicFalse(is_tone != is_prompt);

    if (is_tone)
    {
        config = has_resampler ? Kymera_GetChainConfigs()->chain_tone_gen_config : Kymera_GetChainConfigs()->chain_tone_gen_no_iir_config;
    }
    else
    {
        if (msg->prompt_format == PROMPT_FORMAT_SBC)
        {
            config = has_resampler ? Kymera_GetChainConfigs()->chain_prompt_decoder_config : Kymera_GetChainConfigs()->chain_prompt_decoder_no_iir_config;
        }
        else if (msg->prompt_format == PROMPT_FORMAT_PCM)
        {
            /* If PCM at the correct rate, no chain required at all. */
            config = has_resampler ? Kymera_GetChainConfigs()->chain_prompt_pcm_config : NULL;
        }
    }

    if (config)
    {
        Operator op;
        unsigned buffer_size;
        chain = ChainCreate(config);

        if (has_resampler)
        {
            /* Configure resampler */
            op = ChainGetOperatorByRole(chain, OPR_TONE_PROMPT_RESAMPLER);
            OperatorsResamplerSetConversionRate(op, msg->rate, theKymera->output_rate);
        }

        if (is_tone)
        {
            /* Configure ringtone generator */
            op = ChainGetOperatorByRole(chain, OPR_TONE_GEN);
            OperatorsStandardSetSampleRate(op, msg->rate);
            OperatorsConfigureToneGenerator(op, msg->tone, &theKymera->task);
        }

        /* Configure pass-through buffer */
        op = ChainGetOperatorByRole(chain, OPR_TONE_PROMPT_BUFFER);
        buffer_size = appKymeraCalculateBufferSize(theKymera->output_rate);
        OperatorsStandardSetBufferSize(op, buffer_size);

        ChainConnect(chain);
        theKymera->chain_tone_handle = chain;
    }


    if (is_prompt)
    {
        /* Configure prompt file source */
        theKymera->prompt_source = PanicNull(StreamFileSource(msg->prompt));
        appKymeraPromptSourceSetup(theKymera->prompt_source);
        if (chain)
        {
            PanicFalse(ChainConnectInput(chain, theKymera->prompt_source, EPR_PROMPT_IN));
        }
        else
        {
            /* No chain (prompt is PCM at the correct sample rate) so the source
            is just the file */
            return theKymera->prompt_source;
        }
    }
    return ChainGetOutput(chain, EPR_TONE_PROMPT_CHAIN_OUT);
}

static bool kymeraTonesPrompts_isAuxTtpSupported(capablity_version_t cap_version)
{
    return cap_version.version_msb >= VOLUME_CONTROL_SET_AUX_TTP_VERSION_MSB ? TRUE : FALSE;
}

bool appKymeraIsPlayingPrompt(void)
{
    return (kymera_tone_state == kymera_tone_playing);
}

void appKymeraHandleInternalTonePromptPlay(const KYMERA_INTERNAL_TONE_PROMPT_PLAY_T *msg)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    Operator op;

    DEBUG_LOG("appKymeraHandleInternalTonePromptPlay, prompt %x, tone %p, ttp %d, int %u, lock 0x%x, mask 0x%x",
                msg->prompt, msg->tone, msg->time_to_play, msg->interruptible, msg->client_lock, msg->client_lock_mask);

    /* If there is a tone still playing at this point, it must be an interruptable tone, so cut it off */
    appKymeraTonePromptStop();

    kymera_tone_state = kymera_tone_playing;

    switch (appKymeraGetState())
    {
        case KYMERA_STATE_IDLE:

            theKymera->output_rate = msg->rate;

            /* Update state variables */
            appKymeraSetState(KYMERA_STATE_TONE_PLAYING);

            /* Need to set up audio output chain to play tone from scratch */
            appKymeraCreateOutputChain(KICK_PERIOD_TONES, 0, 0);
            appKymeraExternalAmpControl(TRUE);

            ChainStart(theKymera->chainu.output_vol_handle);
            op = ChainGetOperatorByRole(theKymera->chainu.output_vol_handle, OPR_VOLUME_CONTROL);
            OperatorsVolumeMute(op, FALSE);

        // fall-through
        case KYMERA_STATE_SCO_ACTIVE:
        case KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING:
        case KYMERA_STATE_SCO_SLAVE_ACTIVE:
        case KYMERA_STATE_A2DP_STREAMING:
        case KYMERA_STATE_A2DP_STREAMING_WITH_FORWARDING:
        case KYMERA_STATE_STANDALONE_LEAKTHROUGH:
        {
            capablity_version_t vol_op_version;
            kymera_chain_handle_t output_chain = theKymera->chainu.output_vol_handle;

            /* May need to exit low power mode to play tone simultaneously */
            appKymeraConfigureDspPowerMode();

            Source aux_source = appKymeraCreateTonePromptChain(msg);
            int16 volume_db = (msg->tone != NULL) ? (KYMERA_DB_SCALE * KYMERA_CONFIG_TONE_VOLUME) :
                                                    (KYMERA_DB_SCALE * KYMERA_CONFIG_PROMPT_VOLUME);

            /* Connect tone/prompt chain to output */
            ChainConnectInput(output_chain, aux_source, EPR_VOLUME_AUX);
            /* Set tone/prompt volume */
            op = ChainGetOperatorByRole(theKymera->chainu.output_vol_handle, OPR_VOLUME_CONTROL);
            OperatorsVolumeSetAuxGain(op, volume_db);

            vol_op_version = OperatorGetCapabilityVersion(op);
            if (kymeraTonesPrompts_isAuxTtpSupported(vol_op_version))
            {
                rtime_t now = SystemClockGetTimerTime();
                rtime_t delta = rtime_sub(msg->time_to_play, now);

                DEBUG_LOG("appKymeraHandleInternalTonePromptPlay now=%u, ttp=%u, left=%d", now, msg->time_to_play, delta);

                OperatorsVolumeSetAuxTimeToPlay(op, msg->time_to_play,  0);
            }

            /* Start tone */
            if (theKymera->chain_tone_handle)
            {
                ChainStart(theKymera->chain_tone_handle);
            }
        }
        break;

        case KYMERA_STATE_TONE_PLAYING:
        default:
            /* Unknown state / not supported */
            DEBUG_LOG("appKymeraHandleInternalTonePromptPlay, unsupported state %u", appKymeraGetState());
            Panic();
            break;
    }
    if (!msg->interruptible)
    {
        appKymeraSetToneLock(theKymera);
    }
    theKymera->tone_client_lock = msg->client_lock;
    theKymera->tone_client_lock_mask = msg->client_lock_mask;
}

void appKymeraTonePromptStop(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    /* Exit if there isn't a tone or prompt playing */
    if (!theKymera->chain_tone_handle && !theKymera->prompt_source)
        return;

    DEBUG_LOG("appKymeraTonePromptStop, state %u", appKymeraGetState());

    switch (appKymeraGetState())
    {
        case KYMERA_STATE_SCO_ACTIVE:
        case KYMERA_STATE_SCO_SLAVE_ACTIVE:
        case KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING:
        case KYMERA_STATE_A2DP_STREAMING:
        case KYMERA_STATE_A2DP_STREAMING_WITH_FORWARDING:
        case KYMERA_STATE_TONE_PLAYING:
        case KYMERA_STATE_STANDALONE_LEAKTHROUGH:
        {
            Operator op = ChainGetOperatorByRole(theKymera->chainu.output_vol_handle, OPR_VOLUME_CONTROL);
            uint16 volume = volTo60thDbGain(0);
            OperatorsVolumeSetAuxGain(op, volume);

            if (theKymera->prompt_source)
            {
                appKymeraPromptSourceClose(theKymera->prompt_source);
                theKymera->prompt_source = NULL;
            }

            if (theKymera->chain_tone_handle)
            {
                ChainStop(theKymera->chain_tone_handle);
                ChainDestroy(theKymera->chain_tone_handle);
                theKymera->chain_tone_handle = NULL;
            }

            kymera_tone_state = kymera_tone_idle;

            if (appKymeraGetState() != KYMERA_STATE_TONE_PLAYING)
            {
                /* Return to low power mode (if applicable) */
                appKymeraConfigureDspPowerMode();
            }
            else
            {
                OperatorsVolumeSetMainGain(op, volume);
                OperatorsVolumeMute(op, TRUE);

                /* Disable external amplifier if required */
                appKymeraExternalAmpControl(FALSE);

#ifdef ENABLE_AEC_LEAKTHROUGH
                if(!Kymera_IsStandaloneLeakthroughActive())
                {
                    appKymeraDestroyOutputChain();
                    /* Move back to idle state if standalone leak-through is not active */
                    appKymeraSetState(KYMERA_STATE_IDLE);
                    theKymera->output_rate = 0;
                }
#else
                appKymeraDestroyOutputChain();
                /* Move back to idle state */
                appKymeraSetState(KYMERA_STATE_IDLE);
                theKymera->output_rate = 0;
#endif
            }
        }
        break;

        case KYMERA_STATE_IDLE:
        default:
            /* Unknown state / not supported */
            DEBUG_LOG("appKymeraTonePromptStop, unsupported state %u", appKymeraGetState());
            Panic();
            break;
    }

    appKymeraClearToneLock(theKymera);

    PanicZero(theKymera->tone_count);
    theKymera->tone_count--;

    /* Tone now stopped, clear the client's lock */
    if (theKymera->tone_client_lock)
    {
        *theKymera->tone_client_lock &= ~theKymera->tone_client_lock_mask;
        theKymera->tone_client_lock = 0;
        theKymera->tone_client_lock_mask = 0;
    }
}
