/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_a2dp_mirror.c
\brief      Kymera A2DP for TWM.
*/

#ifdef INCLUDE_MIRRORING
#include <operators.h>

#include "init.h"
#include "kymera_private.h"
#include "kymera_a2dp_private.h"
#include "kymera_config.h"
#include "av.h"
#include "a2dp_profile_config.h"
#include "multidevice.h"
#include "mirror_profile.h"

/*! Helper defines for RTP header format. These
    are used for hash transform configuration */
#define RTP_HEADER_LENGTH (12)
#define RTP_HEADER_SEQUENCE_NO_OFFSET (2)

static void appKymeraCreateInputChain(kymeraTaskData *theKymera, uint8 seid, bool is_left)
{
    const chain_config_t *config = NULL;
    /* Create input chain */
    switch (seid)
    {
        case AV_SEID_SBC_SNK:
            DEBUG_LOG("appKymeraCreateInputChain, create TWM SBC input chain");
            config = Kymera_GetChainConfigs()->chain_input_sbc_stereo_mix_config;
        break;

        case AV_SEID_AAC_SNK:
            DEBUG_LOG("appKymeraCreateInputChain, create TWM AAC input chain");
            config = Kymera_GetChainConfigs()->chain_input_aac_stereo_mix_config;
        break;

        case AV_SEID_APTX_SNK:
            DEBUG_LOG("appKymeraCreateInputChain, create TWM aptX input chain");
            if (appConfigEnableAptxStereoMix())
            {
                config = Kymera_GetChainConfigs()->chain_input_aptx_stereo_mix_config;
            }
            else
            {
                config = is_left ? Kymera_GetChainConfigs()->chain_forwarding_input_aptx_left_config :
                                   Kymera_GetChainConfigs()->chain_forwarding_input_aptx_right_config;
            }
        break;
#ifdef INCLUDE_APTX_ADAPTIVE
    case AV_SEID_APTX_ADAPTIVE_SNK:
        DEBUG_LOG("appKymeraCreateInputChain, create TWM aptX adaptive input chain, Q2Q %u", theKymera->q2q_mode);
        if (appConfigEnableAptxAdaptiveStereoMix())
        {
            if (theKymera->q2q_mode)
                config =  Kymera_GetChainConfigs()->chain_input_aptx_adaptive_stereo_mix_q2q_config;
            else
                config =  Kymera_GetChainConfigs()->chain_input_aptx_adaptive_stereo_mix_config;

        }
        else
        {/* We do not support forwarding for aptX adaptive */
            Panic();
        }
    break;

#endif
        default:
            Panic();
        break;
    }
    theKymera->chain_input_handle = PanicNull(ChainCreate(config));
}

static void appKymeraConfigureInputChain(kymeraTaskData *theKymera,
                                         uint8 seid, uint32 rate, uint32 max_bitrate,
                                         bool cp_header_enabled, bool is_left,
                                         aptx_adaptive_ttp_latencies_t nq2q_ttp)
{
    kymera_chain_handle_t chain_handle = theKymera->chain_input_handle;
    rtp_codec_type_t rtp_codec = -1;
    Operator op;
    unsigned rtp_buffer_size = 0;
    Operator op_rtp_decoder = ChainGetOperatorByRole(chain_handle, OPR_RTP_DECODER);

    max_bitrate = (max_bitrate) ? max_bitrate : (MAX_CODEC_RATE_KBPS * 1000);
    rtp_buffer_size = appKymeraGetAudioBufferSize(max_bitrate, TWS_STANDARD_LATENCY_MAX_MS);

    switch (seid)
    {
        case AV_SEID_SBC_SNK:
            DEBUG_LOG("appKymeraConfigureAndConnectInputChain, configure TWM SBC input chain");
            rtp_codec = rtp_codec_type_sbc;
        break;

        case AV_SEID_AAC_SNK:
            DEBUG_LOG("appKymeraConfigureAndConnectInputChain, configure TWM AAC input chain");
            rtp_codec = rtp_codec_type_aac;
            op = PanicZero(ChainGetOperatorByRole(chain_handle, OPR_AAC_DECODER));
            OperatorsRtpSetAacCodec(op_rtp_decoder, op);
        break;

        case AV_SEID_APTX_SNK:
            DEBUG_LOG("appKymeraConfigureAndConnectInputChain, configure TWM aptX input chain");
            rtp_codec = rtp_codec_type_aptx;

            op = PanicZero(ChainGetOperatorByRole(chain_handle, OPR_APTX_DEMUX));
            OperatorsStandardSetSampleRate(op, rate);
            op = PanicZero(ChainGetOperatorByRole(chain_handle, OPR_SWITCHED_PASSTHROUGH_CONSUMER));
            OperatorsSetSwitchedPassthruEncoding(op, spc_op_format_encoded);

            if (appConfigEnableAptxStereoMix())
            {
                spc_mode_t sync_mode = spc_op_mode_tagsync_dual;
                OperatorsSetSwitchedPassthruMode(op, sync_mode);
            }


        break;
#ifdef INCLUDE_APTX_ADAPTIVE
    case AV_SEID_APTX_ADAPTIVE_SNK:
        DEBUG_LOG("appKymeraConfigureAndConnectInputChain, configure TWM aptX adaptive input chain");

        aptx_adaptive_ttp_in_ms_t aptx_ad_ttp;

        if (theKymera->q2q_mode)
        {
            op = PanicZero(ChainGetOperatorByRole(chain_handle, OPR_SWITCHED_PASSTHROUGH_CONSUMER));
            OperatorsSetSwitchedPassthruEncoding(op, spc_op_format_encoded);
            OperatorsStandardSetBufferSizeWithFormat(op, rtp_buffer_size, operator_data_format_encoded);
            OperatorsSetSwitchedPassthruMode(op, spc_op_mode_passthrough);
        }
        else
        {
            convertAptxAdaptiveTtpToOperatorsFormat(nq2q_ttp, &aptx_ad_ttp);
            getAdjustedAptxAdaptiveTtpLatencies(&aptx_ad_ttp);

            OperatorsRtpSetAptxAdaptiveTTPLatency(op_rtp_decoder, aptx_ad_ttp);
            rtp_codec = rtp_codec_type_aptx_ad;
        }

    break;

#endif
        default:
            Panic();
        break;
    }

    appKymeraConfigureLeftRightMixer(chain_handle, rate, theKymera->enable_left_right_mix, is_left);

    if (!theKymera->q2q_mode) /* We don't use rtp decoder for Q2Q */
        appKymeraConfigureRtpDecoder(op_rtp_decoder, rtp_codec, rtp_decode, rate, cp_header_enabled, rtp_buffer_size);

    ChainConnect(chain_handle);
}

static void appKymeraCreateAndConfigureOutputChain(uint8 seid, uint32 rate,
                                                   int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    unsigned kick_period = KICK_PERIOD_FAST;

    DEBUG_LOG("appKymeraCreateAndConfigureOutputChain, creating output chain, completing startup");
    switch (seid)
    {
        case AV_SEID_SBC_SNK:  kick_period = KICK_PERIOD_MASTER_SBC;  break;
        case AV_SEID_AAC_SNK:  kick_period = KICK_PERIOD_MASTER_AAC;  break;
        case AV_SEID_APTX_SNK: kick_period = KICK_PERIOD_MASTER_APTX; break;
#ifdef INCLUDE_APTX_ADAPTIVE
        case AV_SEID_APTX_ADAPTIVE_SNK: kick_period = KICK_PERIOD_MASTER_APTX_ADAPTIVE; break;
#endif
    }
    theKymera->output_rate = rate;
    appKymeraCreateOutputChain(kick_period, 0, volume_in_db);
    appKymeraSetOperatorUcids(FALSE);
    appKymeraExternalAmpControl(TRUE);
}

static void appKymeraJoinInputOutputChains(kymeraTaskData *theKymera)
{
    /* Connect input and output chains together */
    PanicFalse(ChainConnectInput(theKymera->chainu.output_vol_handle,
                                 ChainGetOutput(theKymera->chain_input_handle,
                                 EPR_SOURCE_DECODED_PCM),
                                 EPR_SINK_MIXER_MAIN_IN));
}

static void appKymeraConfigureAndStartHashTransform(kymeraTaskData *theKymera, uint8 seid, Source source)
{
    Sink chain_input = ChainGetInput(theKymera->chain_input_handle, EPR_SINK_MEDIA);
    bool cp_header_enabled = theKymera->cp_header_enabled;

    DEBUG_LOG("appKymeraConfigureAndStartHashTransform");

    /* Stop and destroy hash transform if already present */
    if (theKymera->hashu.hash)
    {
        DEBUG_LOG("appKymeraConfigureAndStartHashTransform, destroy"
                  "already present hash transform:0x%x", theKymera->hashu.hash);
        TransformDisconnect(theKymera->hashu.hash);
        theKymera->hashu.hash = NULL;
    }

    /* Connect source with chain input via hash transform */
    theKymera->hashu.hash = TransformHash(source, chain_input);

    if (theKymera->hashu.hash)
    {
        /* Configure the hash transform based on the codec type */
        switch (seid)
        {
            /* SBC and AAC codec packets have RTP header by default. No need to
            * prefix the header in hash transform */
            case AV_SEID_SBC_SNK:
            case AV_SEID_AAC_SNK:
#ifdef INCLUDE_APTX_ADAPTIVE
            case AV_SEID_APTX_ADAPTIVE_SNK: /* Non-Q2Q mode uses RTP */
#endif
            {
                (void) TransformConfigure(theKymera->hashu.hash, VM_TRANSFORM_HASH_PREFIX_RTP_HEADER, 0);
                /* Set source size to 0xFFFF to calculate hash for complete packet */
                (void) TransformConfigure(theKymera->hashu.hash, VM_TRANSFORM_HASH_SOURCE_SIZE, 0xFFFF);
                (void) TransformConfigure(theKymera->hashu.hash, VM_TRANSFORM_HASH_SOURCE_OFFSET, 0);
                (void) TransformConfigure(theKymera->hashu.hash, VM_TRANSFORM_HASH_SOURCE_MODIFY_OFFSET, RTP_HEADER_SEQUENCE_NO_OFFSET);
            }
            break;

            /* aptX codec packet only has RTP header if content protection is enabled. If conent protection
             * is not enabled then configure hash transform to prefix the header to codec data */
            case AV_SEID_APTX_SNK:
            {
                if (cp_header_enabled)
                {
                    (void) TransformConfigure(theKymera->hashu.hash, VM_TRANSFORM_HASH_PREFIX_RTP_HEADER, 0);
                    (void) TransformConfigure(theKymera->hashu.hash, VM_TRANSFORM_HASH_SOURCE_MODIFY_OFFSET, RTP_HEADER_SEQUENCE_NO_OFFSET);
                }
                else
                {
                    (void) TransformConfigure(theKymera->hashu.hash, VM_TRANSFORM_HASH_PREFIX_RTP_HEADER, 1);
                    /* payload type and SSRC aren't needed as such. Setting them to 0 for sanity */
                    (void) TransformConfigure(theKymera->hashu.hash, VM_TRANSFORM_HASH_RTP_PAYLOAD_TYPE, 0);
                    (void) TransformConfigure(theKymera->hashu.hash, VM_TRANSFORM_HASH_RTP_SSRC_LOWER, 0);
                    (void) TransformConfigure(theKymera->hashu.hash, VM_TRANSFORM_HASH_RTP_SSRC_UPPER, 0);
                }
                /* Set source size to 0xFFFF to calculate hash for complete packet */
                (void) TransformConfigure(theKymera->hashu.hash, VM_TRANSFORM_HASH_SOURCE_SIZE, 0xFFFF);
                (void) TransformConfigure(theKymera->hashu.hash, VM_TRANSFORM_HASH_SOURCE_OFFSET, 0);
            }
            break;
        }
        if (!TransformStart(theKymera->hashu.hash))
        {
            DEBUG_LOG("appKymeraConfigureAndStartHashTransform, failed to start transform");
            TransformDisconnect(theKymera->hashu.hash);
            StreamConnectDispose(source);
        }
    }
    else
    {
        DEBUG_LOG("appKymeraConfigureAndStartHashTransform, failed to create hash transform");
        /* This typically occurs when the source is destroyed before media is started.
           Tidy up by disposing */
        StreamConnectDispose(source);
    }
}

static void appKymeraDestroyClockConvertTransform(Transform transform)
{
    if (transform)
    {
        DEBUG_LOG("appKymeraDestroyClockConvertTransform, destroy"
                  " convert clock transform:0x%x", transform);
        TransformDisconnect(transform);
    }
}

static Transform appKymeraCreateAndConfigureClockConvertTransform(Source source, Sink sink)
{
    /* Create the transform */
    StreamDisconnect(source, 0);
    Transform cc_transform = TransformConvertClock(source, sink);
    if (cc_transform)
    {
        /* Configure the transform */
        (void) TransformConfigure(cc_transform, VM_TRANSFORM_CLK_CONVERT_START_OFFSET, 8);
        (void) TransformConfigure(cc_transform, VM_TRANSFORM_CLK_CONVERT_REPETITION_OFFSET, 6);
        (void) TransformConfigure(cc_transform, VM_TRANSFORM_CLK_CONVERT_NUM_REPETITIONS, 0xFFFF);
    }
    else
    {
        DEBUG_LOG("appKymeraCreateAndConfigureClockConvertTransform, failed to create transform");
    }
    return cc_transform;
}

static void appKymeraConfigureAudioSyncMode(kymeraTaskData *theKymera,
                                            mirror_profile_a2dp_start_mode_t a2dp_start_mode)
{
    DEBUG_LOG("appKymeraConfigureAudioSyncMode, a2dp_start_mode:%d", a2dp_start_mode);

    switch(a2dp_start_mode)
    {
    case MIRROR_PROFILE_A2DP_START_PRIMARY_UNSYNCHRONISED:
        theKymera->sync_info.mode = KYMERA_AUDIO_SYNC_START_PRIMARY_UNSYNCHRONISED;
        break;
    case MIRROR_PROFILE_A2DP_START_PRIMARY_SYNCHRONISED:
        theKymera->sync_info.mode = KYMERA_AUDIO_SYNC_START_PRIMARY_SYNCHRONISED;
        break;
    case MIRROR_PROFILE_A2DP_START_SECONDARY_SYNCHRONISED:
        theKymera->sync_info.mode = KYMERA_AUDIO_SYNC_START_SECONDARY_SYNCHRONISED;
        break;
    case MIRROR_PROFILE_A2DP_START_SECONDARY_JOINS_SYNCHRONISED:
        theKymera->sync_info.mode = KYMERA_AUDIO_SYNC_START_SECONDARY_JOINS_SYNCHRONISED;
        break;
    case MIRROR_PROFILE_A2DP_START_Q2Q_MODE:
        theKymera->sync_info.mode = KYMERA_AUDIO_SYNC_START_Q2Q;
        break;
    default:
        Panic();
        break;
    }
}

static void appKymeraCreateAndConfigureAudioSync(kymeraTaskData *theKymera, Sink sink)
{
    if (theKymera->q2q_mode)
    {
        DEBUG_LOG("appKymeraCreateAndConfigureAudioSync, q2q mode doing dothing");
        return;
    }

    Operator op_rtp = ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_RTP_DECODER);

    if (op_rtp)
        theKymera->sync_info.source = PanicNull(StreamAudioSyncSource(op_rtp));

    MessageStreamTaskFromSource(theKymera->sync_info.source, &theKymera->task);
    MessageStreamTaskFromSink(StreamSinkFromSource(theKymera->sync_info.source), &theKymera->task);

    DEBUG_LOG("appKymeraCreateAndConfigureAudioSync, created source:0x%x,"
              " mode:%d", theKymera->sync_info.source, theKymera->sync_info.mode);
    switch(theKymera->sync_info.mode)
    {
    case KYMERA_AUDIO_SYNC_START_PRIMARY_SYNCHRONISED:
    {
        appKymeraDestroyClockConvertTransform(theKymera->convert_ttp_to_wc);
        appKymeraDestroyClockConvertTransform(theKymera->convert_wc_to_ttp);

        /* Create transform to convert ttp info from local system time into wallclock domain before writing to sink */
        theKymera->convert_ttp_to_wc = appKymeraCreateAndConfigureClockConvertTransform(theKymera->sync_info.source,
                                                                                        sink);
        /* Create transform to convert ttp info from wallclock into local system time domain before writing to sink */
        theKymera->convert_wc_to_ttp = appKymeraCreateAndConfigureClockConvertTransform(StreamSourceFromSink(sink),
                                                                                        StreamSinkFromSource(theKymera->sync_info.source));
        (void) SourceConfigure(theKymera->sync_info.source,
                               STREAM_AUDIO_SYNC_SOURCE_INTERVAL,
                               AUDIO_SYNC_MS_INTERVAL * US_PER_MS);

        (void) SourceConfigure(theKymera->sync_info.source,
                               STREAM_AUDIO_SYNC_SOURCE_MTU,
                               AUDIO_SYNC_PACKET_MTU);
    }
    break;

    case KYMERA_AUDIO_SYNC_START_PRIMARY_UNSYNCHRONISED:
        /* nothing to be done for now */
    break;

    case KYMERA_AUDIO_SYNC_START_SECONDARY_JOINS_SYNCHRONISED:
    case KYMERA_AUDIO_SYNC_START_SECONDARY_SYNCHRONISED:
    {
        Sink sync_sink = StreamSinkFromSource(theKymera->sync_info.source);

        appKymeraDestroyClockConvertTransform(theKymera->convert_ttp_to_wc);
        appKymeraDestroyClockConvertTransform(theKymera->convert_wc_to_ttp);

        /* set audio sync sink data mode to process ttp_info data
         * received from secondary (old primary) */
        (void) SinkConfigure(sync_sink,
                             STREAM_AUDIO_SYNC_SINK_MODE,
                             SINK_MODE_STARTUP);

        /* Setting source MTU is harmless and avoids it's
         * configuration during handover */
        (void) SourceConfigure(theKymera->sync_info.source,
                               STREAM_AUDIO_SYNC_SOURCE_MTU,
                               AUDIO_SYNC_PACKET_MTU);

        /* Create transform to convert ttp info from local system time into wallclock domain before writing to sink */
        theKymera->convert_wc_to_ttp = appKymeraCreateAndConfigureClockConvertTransform(theKymera->sync_info.source,
                                                                                        sink);
        /* Create transform to convert ttp info from wallclock to local system time domain before writing to sink */
        theKymera->convert_ttp_to_wc = appKymeraCreateAndConfigureClockConvertTransform(StreamSourceFromSink(sink),
                                                                                    sync_sink);
    }
    break;

    case KYMERA_AUDIO_SYNC_START_Q2Q: /* should never hit this Q2Q option */
    default:
        Panic();
    break;
    }

    OperatorsRtpSetTtpNotification(op_rtp, TRUE);

    /* start convert clock transform */
    if( theKymera->convert_ttp_to_wc)
    {
        if (TransformStart(theKymera->convert_ttp_to_wc))
        {
            DEBUG_LOG("appKymeraCreateAndConfigureAudioSync, started ttp_to_wc transform:0x%x", theKymera->convert_ttp_to_wc);
        }
        else
        {
            DEBUG_LOG("appKymeraCreateAndConfigureAudioSync, failed starting ttp_to_wc transform:0x%x", theKymera->convert_ttp_to_wc);
        }
    }
    /* start convert clock transform */
    if( theKymera->convert_wc_to_ttp)
    {
        if (TransformStart(theKymera->convert_wc_to_ttp))
        {
            DEBUG_LOG("appKymeraCreateAndConfigureAudioSync, started wc_to_ttp transform:0x%x", theKymera->convert_wc_to_ttp);
        }
        else
        {
            DEBUG_LOG("appKymeraCreateAndConfigureAudioSync, failed starting wc_to_ttp transform:0x%x", theKymera->convert_wc_to_ttp);
        }
    }

}

static void appKymeraStopAudioSync(kymeraTaskData *theKymera, Source source)
{
    if (theKymera->q2q_mode)
    {
        DEBUG_LOG("appKymeraStopAudioSync, q2q mode doing dothing");
        return;
    }

    Operator op_rtp = ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_RTP_DECODER);

    PanicZero(theKymera->sync_info.source);

    MessageStreamTaskFromSource(theKymera->sync_info.source, NULL);

    appKymeraDestroyClockConvertTransform(theKymera->convert_ttp_to_wc);
    theKymera->convert_ttp_to_wc = NULL;

    appKymeraDestroyClockConvertTransform(theKymera->convert_wc_to_ttp);
    theKymera->convert_wc_to_ttp = NULL;

    OperatorsRtpSetTtpNotification(op_rtp, FALSE);

    SourceClose(theKymera->sync_info.source);
    DEBUG_LOG("appKymeraStopAudioSync, closed source:0x%x", theKymera->sync_info.source);

    if (source)
    {
        /* Disconnect source from any connection and dispose data.
        This is required if primary starts to send audio sync messages
        when secondary is not is A2DP streaming state */
        StreamDisconnect(source, 0);
        StreamConnectDispose(source);
    }

    theKymera->sync_info.source = 0;
    theKymera->sync_info.state = KYMERA_AUDIO_SYNC_STATE_INIT;
}

static void appKymeraStartChains(kymeraTaskData *theKymera)
{
    /* Start the output chain regardless of whether the source was connected
    to the input chain. Failing to do so would mean audio would be unable
    to play a tone. This would cause kymera to lock, since it would never
    receive a KYMERA_OP_MSG_ID_TONE_END and the kymera lock would never
    be cleared. */
    ChainStart(theKymera->chainu.output_vol_handle);

    switch(theKymera->sync_info.mode)
    {
    case KYMERA_AUDIO_SYNC_START_PRIMARY_UNSYNCHRONISED:
        /* Start the input chain straightaway since A2DP on primary device
         * needs to start in unsynchronised mode. */
        ChainStart(theKymera->chain_input_handle);
        theKymera->sync_info.state = KYMERA_AUDIO_SYNC_STATE_COMPLETE;
    break;
    case KYMERA_AUDIO_SYNC_START_SECONDARY_JOINS_SYNCHRONISED:
    {
        Operator volop;

        /* Mute audio output before starting the input chain to ensure that
         * audio chains consume audio data and play silence on the output
         * until the application receives sink synchronised indication.
         */
        if (GET_OP_FROM_CHAIN(volop, theKymera->chainu.output_vol_handle, OPR_VOLUME_CONTROL))
        {
            OperatorsVolumeMuteTransitionPeriod(volop, 0);
            OperatorsVolumeMute(volop, TRUE);
            OperatorsVolumeMuteTransitionPeriod(volop, appConfigSecondaryJoinsSynchronisedUnmuteTransitionMs());
            DEBUG_LOG("appKymeraStartChains, mute volume operator");
        }
        ChainStart(theKymera->chain_input_handle);
        theKymera->sync_info.state = KYMERA_AUDIO_SYNC_STATE_IN_PROGRESS;
    }
    break;
    case KYMERA_AUDIO_SYNC_START_Q2Q:
    {
        /* In this case the timestamp contains the exact time to play of the packet.
         * No syncronisation is necessary
         */
        ChainStart(theKymera->chain_input_handle);
        theKymera->sync_info.state = KYMERA_AUDIO_SYNC_STATE_COMPLETE;
    }
    break;
    default:
        /* delay start of input chain until the application receives
         * a2dp data sync indication.
         */
        theKymera->sync_info.state = KYMERA_AUDIO_SYNC_STATE_INIT;
        DEBUG_LOG("appKymeraStartChains, input chain start delayed");
    break;
    }
}

bool appKymeraA2dpStartMaster(const a2dp_codec_settings *codec_settings, uint32 max_bitrate, int16 volume_in_db,
                              aptx_adaptive_ttp_latencies_t nq2q_ttp)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    bool cp_header_enabled;
    uint32 rate;
    uint8 seid;
    Source source;
    uint16 mtu;
    bool is_left = Multidevice_IsLeft();

    UNUSED(max_bitrate);
    appKymeraGetA2dpCodecSettingsCore(codec_settings, &seid, &source, &rate, &cp_header_enabled, &mtu);

    if (source == NULL)
    {
        /* force a return as we do not have a source available */
        DEBUG_LOG("appKymeraA2dpStartMaster, no source, seid %d", seid);
        return FALSE;
    }

    theKymera->cp_header_enabled = cp_header_enabled;

    switch (appKymeraGetState())
    {
        /* Mirror audio chains are started in one step */
        case KYMERA_STATE_A2DP_STARTING_A:
        {
            MessageMoreData mmd = {source};

            appKymeraCreateAndConfigureOutputChain(seid, rate, volume_in_db);
            appKymeraCreateInputChain(theKymera, seid, is_left);
            appKymeraConfigureInputChain(theKymera, seid, rate, max_bitrate, cp_header_enabled, is_left, nq2q_ttp);
            appKymeraJoinInputOutputChains(theKymera);
            appKymeraConfigureDspPowerMode();
            appKymeraConfigureAudioSyncMode(theKymera, MirrorProfile_GetA2dpStartMode());

            StreamDisconnect(source, 0);

            if (theKymera->q2q_mode)
            {
                Sink sink = ChainGetInput(theKymera->chain_input_handle, EPR_SINK_MEDIA);
                Transform packetiser = PanicNull(TransformPacketise(source, sink));
                if (packetiser)
                {
                    PanicFalse(TransformConfigure(packetiser, VM_TRANSFORM_PACKETISE_CODEC, VM_TRANSFORM_PACKETISE_CODEC_APTX));
                    PanicFalse(TransformConfigure(packetiser, VM_TRANSFORM_PACKETISE_MODE, VM_TRANSFORM_PACKETISE_MODE_TWSPLUS));
                    PanicFalse(TransformConfigure(packetiser, VM_TRANSFORM_PACKETISE_SAMPLE_RATE, (uint16) rate));
                    PanicFalse(TransformConfigure(packetiser, VM_TRANSFORM_PACKETISE_CPENABLE, (uint16) cp_header_enabled));
                    PanicFalse(TransformConfigure(packetiser, VM_TRANSFORM_PACKETISE_TTP_DELAY, aptxAdaptiveTTPLatencyAdjust()));
                    PanicFalse(TransformConfigure(packetiser, VM_TRANSFORM_PACKETISE_TTP_DELAY_SSRC_TRIGGER_1,aptxAdaptiveTTPLatencyAdjustLL_SSRC()));
                    PanicFalse(TransformConfigure(packetiser, VM_TRANSFORM_PACKETISE_TTP_DELAY_SSRC_1, aptxAdaptiveTTPLatencyAdjustLL()));
                    PanicFalse(TransformConfigure(packetiser, VM_TRANSFORM_PACKETISE_TTP_DELAY_SSRC_TRIGGER_2,aptxAdaptiveTTPLatencyAdjustHQ_SSRC()));
                    PanicFalse(TransformConfigure(packetiser, VM_TRANSFORM_PACKETISE_TTP_DELAY_SSRC_2, aptxAdaptiveTTPLatencyAdjustHQ()));
                    PanicFalse(TransformStart(packetiser));
                    theKymera->hashu.packetiser = packetiser;
                 }
            }

            appKymeraStartChains(theKymera);

            theKymera->media_source = source;

            /* The hash transform is created/connected when the first packet arrives from the
            source - signalled by a MESSAGE_MORE_DATA.
            This is not applicable for Q2Q mode as we have already started the transform for the packetiser
            and the hash transform is not used */

            if (!theKymera->q2q_mode)
            {
                theKymera->media_source = source;

                /* No data in source, wait for MESSAGE_MORE_DATA */
                MessageStreamTaskFromSource(source, KymeraGetTask());
                PanicFalse(SourceConfigure(source, VM_SOURCE_MESSAGES, VM_MESSAGES_SOME));

                MessageSendLater(KymeraGetTask(),
                                 KYMERA_INTERNAL_A2DP_MESSAGE_MORE_DATA_TIMEOUT,
                                 NULL, A2DP_MIRROR_MESSAGE_MORE_DATA_TIMEOUT_MS);

                /* Check if already data in the source */
                appKymeraA2dpHandleMessageMoreData(&mmd);
            }
        }

        if(AecLeakthrough_IsLeakthroughEnabled())
        {
             /* Setting the usecase for AEC_REF to a2dp_leakthrough */
             Kymera_SetAecUseCase(aec_usecase_a2dp_leakthrough);
             Kymera_ConnectLeakthroughMic();
             Kymera_LeakthroughEnableAecSideToneAfterTimeout();
        }
        return TRUE;

        default:
            Panic();
        return FALSE;
    }
}

void appKymeraA2dpCommonStop(Source source)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraA2dpCommonStop, source(%p)", source);

    PanicNull(theKymera->chain_input_handle);
    PanicNull(theKymera->chainu.output_vol_handle);

    if(AecLeakthrough_IsLeakthroughEnabled())
    {
        /*Setting the sidetone gain to minimum value to avoid pop during A2dp pause*/
        Kymera_SetMinLeakthroughSidetoneGain();
        /* Disable AEC_REF sidetone path */
        Kymera_LeakthroughEnableAecSideTonePath(FALSE);
        /* Disconnect the leakthrough mic */
        Kymera_DisconnectLeakthroughMic();
        /* Set the Default AEC_REF use case */
        Kymera_SetAecUseCase(aec_usecase_default);
    }

    /* A tone still playing at this point must be interruptable */
    appKymeraTonePromptStop();

    /* Stop chains before disconnecting */
    ChainStop(theKymera->chain_input_handle);

    /* Disable external amplifier if required */
    appKymeraExternalAmpControl(FALSE);

    /* Stop and destroy hash transform */
    if (theKymera->hashu.hash)
    {
        DEBUG_LOG("appKymeraA2dpCommonStop, destroy hash transform:0x%x", theKymera->hashu.hash);
        TransformDisconnect(theKymera->hashu.hash);
        theKymera->hashu.hash = NULL;
        StreamConnectDispose(source);
    }
/*
    if (theKymera->hashu.packetiser)
    {
        DEBUG_LOG("appKymeraA2dpCommonStop, destroy packetiser transform:0x%x", theKymera->hashu.packetiser);
        TransformDisconnect(theKymera->hashu.packetiser);
        theKymera->hashu.packetiser = NULL;
        StreamConnectDispose(source);
    }
*/
    appKymeraDestroyOutputChain();
    /* Destroy chains now that input has been disconnected */
    ChainDestroy(theKymera->chain_input_handle);
    theKymera->chain_input_handle = NULL;
    theKymera->media_source = 0;
}

/* This function is called when audio synchronisation messages should be
   transmitted to or received from the other earbud */
void appKymeraA2dpStartForwarding(const a2dp_codec_settings *codec_settings)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraA2dpStartForwarding a2dp_start_mode:%d", MirrorProfile_GetA2dpStartMode());

    /* audio sync mode must be refreshed in the event whereby secondary
     * device has joined pre-synchronised primary device */
    appKymeraConfigureAudioSyncMode(theKymera, MirrorProfile_GetA2dpStartMode());

    appKymeraCreateAndConfigureAudioSync(theKymera, codec_settings->sink);
}

/* This function is called when audio synchronistaion messages should stop being
   transmitted to or received from the other earbud */
void appKymeraA2dpStopForwarding(Source source)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraA2dpStopForwarding");
    /* Stop and destroy audio sync */
    appKymeraStopAudioSync(theKymera, source);
}

void appKymeraA2dpStartSlave(a2dp_codec_settings *codec_settings, int16 volume_in_db)
{
    UNUSED(codec_settings);
    UNUSED(volume_in_db);

    /* Should not be called when mirroring */
    Panic();
}

/*! \brief Switch from a primary/secondary synchronised startup to a unsynchronised
           start on primary with secondary joining muted until synchronised.
*/
static void appKymeraA2dpSwitchToUnsyncStart(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    if (theKymera->state == KYMERA_STATE_A2DP_STREAMING ||
        theKymera->state == KYMERA_STATE_A2DP_STREAMING_WITH_FORWARDING)
    {
        switch(theKymera->sync_info.mode)
        {
            case KYMERA_AUDIO_SYNC_START_PRIMARY_SYNCHRONISED:
                theKymera->sync_info.mode = KYMERA_AUDIO_SYNC_START_PRIMARY_UNSYNCHRONISED;
            break;
            case KYMERA_AUDIO_SYNC_START_SECONDARY_SYNCHRONISED:
                theKymera->sync_info.mode = KYMERA_AUDIO_SYNC_START_SECONDARY_JOINS_SYNCHRONISED;
            break;
            default:
            return;
        }
        appKymeraStartChains(theKymera);
    }
}

void appKymeraA2dpHandleDataSyncIndTimeout(void)
{
    DEBUG_LOG("appKymeraA2dpHandleDataSyncIndTimeout");
    appKymeraA2dpSwitchToUnsyncStart();
}

void appKymeraA2dpHandleMessageMoreDataTimeout(void)
{
    DEBUG_LOG("appKymeraA2dpHandleMessageMoreDataTimeout");
    appKymeraA2dpSwitchToUnsyncStart();
}

/* Convert BT clock (in 1/2 slots) to microseconds */
static rtime_t btclock_to_rtime(uint32 btclock)
{
    rtime_t btclock_us = (btclock / 2) * US_PER_SLOT;
    return (btclock & 1) ? (btclock_us + HALF_SLOT_US) : btclock_us;
}

void appKymeraA2dpHandleDataSyncInd(Sink sink, uint32 clock)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraA2dpHandleDataSyncInd state:%d, sink:0x%x, clock:0x%x", theKymera->state, sink, clock);

    /* Cancel any pending timeout messages */
    MessageCancelAll(&theKymera->task, KYMERA_INTERNAL_A2DP_DATA_SYNC_IND_TIMEOUT);

    if ((theKymera->state == KYMERA_STATE_A2DP_STREAMING_WITH_FORWARDING)
            && (theKymera->sync_info.state == KYMERA_AUDIO_SYNC_STATE_INIT))
    {
        wallclock_state_t wc_state;

        if (RtimeWallClockGetStateForSink(&wc_state, sink))
        {
            Operator op_rtp;
            rtime_t clock_rtime, ttp_us;

            (void)RtimeWallClockToLocal(&wc_state, btclock_to_rtime(clock), &clock_rtime);
            ttp_us = rtime_add(clock_rtime, TWS_STANDARD_LATENCY_US);

            DEBUG_LOG("appKymeraA2dpHandleDataSyncInd, wc_offset:%d, clock_local_us:0x%x, current_us:0x%x, ttp_us:0x%x",
                       wc_state.offset, clock_rtime, VmGetTimerTime(), ttp_us);

            /* Configure the RTP operator in free_run mode until the application receives
             * synchronised indication from audio sync stream
             */
            if (GET_OP_FROM_CHAIN(op_rtp, theKymera->chain_input_handle, OPR_RTP_DECODER))
            {
                DEBUG_LOG("appKymeraA2dpHandleDataSyncInd, configure RTP operator in ttp_free_run mode");
                OperatorsStandardSetTtpState(op_rtp, ttp_free_run, ttp_us, 0, TWS_STANDARD_LATENCY_US);
            }

            /* start the input chain */
            ChainStart(theKymera->chain_input_handle);
            DEBUG_LOG("appKymeraA2dpHandleDataSyncInd, started input chain");
            theKymera->sync_info.state = KYMERA_AUDIO_SYNC_STATE_IN_PROGRESS;
        }
        else
        {
            /* schedule a message to start audio in unsychronised mode as wallclock
             * information is not available for synchronised start.
             */
            MessageSendLater(&theKymera->task,
                             KYMERA_INTERNAL_A2DP_DATA_SYNC_IND_TIMEOUT,
                             NULL, D_IMMEDIATE);
        }
    }
}

void appKymeraA2dpHandleAudioSyncStreamInd(MessageId id, Message msg)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    Operator op_rtp;
    /* Now that audio sync source stream has been sychronised, switch
     * the audio RTP operator mode to normal (or TTP_FULL)
     */
    GET_OP_FROM_CHAIN(op_rtp, theKymera->chain_input_handle, OPR_RTP_DECODER);

    DEBUG_LOG("appKymeraA2dpHandleAudioSyncStreamInd state:%d, msg_id:%d", theKymera->state, id);

    if ((theKymera->state != KYMERA_STATE_A2DP_STREAMING_WITH_FORWARDING)
            || (theKymera->sync_info.state == KYMERA_AUDIO_SYNC_STATE_COMPLETE))
    {
        return;
    }

    switch(theKymera->sync_info.mode)
    {
    case KYMERA_AUDIO_SYNC_START_PRIMARY_SYNCHRONISED:
        if (id == MESSAGE_SOURCE_AUDIO_SYNCHRONISED)
        {
            OperatorsStandardSetTtpState(op_rtp, ttp_full_only, 0, 0, TWS_STANDARD_LATENCY_US);
            DEBUG_LOG("appKymeraA2dpHandleAudioSyncStreamInd, configure RTP operator in ttp_full_only mode");
            theKymera->sync_info.state = KYMERA_AUDIO_SYNC_STATE_COMPLETE;
        }
    break;

    case KYMERA_AUDIO_SYNC_START_SECONDARY_SYNCHRONISED:
        theKymera->sync_info.state = KYMERA_AUDIO_SYNC_STATE_COMPLETE;
    break;

    case KYMERA_AUDIO_SYNC_START_SECONDARY_JOINS_SYNCHRONISED:
        if (id == MESSAGE_SINK_AUDIO_SYNCHRONISED)
        {
            rtime_t sync_time = ((MessageSinkAudioSynchronised*)msg)->sync_time;
            rtime_t now = VmGetTimerTime();
            rtime_t sched_time = rtime_gt(sync_time, now) ?
                                    (rtime_sub(sync_time, now) + US_PER_MS) / US_PER_MS :
                                    D_IMMEDIATE;
            sched_time += appConfigSecondaryJoinsSynchronisedTrimMs();

            /* Cancel any pending messages */
            MessageCancelAll(&theKymera->task, KYMERA_INTERNAL_A2DP_AUDIO_SYNCHRONISED);
            /* schedule a message to unmute the audio output just after
             * audio sync stream will be in sync.
             */
            MessageSendLater(&theKymera->task,
                             KYMERA_INTERNAL_A2DP_AUDIO_SYNCHRONISED,
                             NULL,
                             sched_time);
            DEBUG_LOG("appKymeraA2dpHandleAudioSyncStreamInd, sync time:0x%x sync in %dms", sync_time, sched_time);
            OperatorsStandardSetTtpState(op_rtp, ttp_free_run_only, 0, 0, TWS_STANDARD_LATENCY_US);

        }
    break;
    default:
        /* Nothing to be done for other configurations */
    break;
    }
}

void appKymeraA2dpHandleAudioSynchronisedInd(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraA2dpHandleAudioSynchronisedInd");

    /* Cancel any pending messages */
    MessageCancelAll(&theKymera->task, KYMERA_INTERNAL_A2DP_AUDIO_SYNCHRONISED);

    if ((theKymera->state != KYMERA_STATE_A2DP_STREAMING_WITH_FORWARDING)
            || (theKymera->sync_info.state == KYMERA_AUDIO_SYNC_STATE_COMPLETE))
    {
        return;
    }

    switch(theKymera->sync_info.mode)
    {
    case KYMERA_AUDIO_SYNC_START_SECONDARY_JOINS_SYNCHRONISED:
    {
        Operator volop;

        /* Now that A2DP audio is synchronised, unmute the audio output */
        if ( GET_OP_FROM_CHAIN(volop, theKymera->chainu.output_vol_handle, OPR_VOLUME_CONTROL))
        {
            DEBUG_LOG("appKymeraA2dpHandleAudioSynchronisedInd, unmute volume operator %d", VmGetTimerTime());
            OperatorsVolumeMute(volop, FALSE);
        }
        theKymera->sync_info.state = KYMERA_AUDIO_SYNC_STATE_COMPLETE;
    }
    break;
    default:
        Panic();
    break;
    }
}

void appKymeraA2dpHandleMessageMoreData(const MessageMoreData *mmd)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    if (mmd->source == theKymera->media_source)
    {
        uint16 boundary = SourceBoundary(mmd->source);
        if (boundary)
        {
            DEBUG_LOG("appKymeraA2dpHandleMessageMoreData boundary=%d", boundary);

#ifdef PRINT_RTP_HEADER
            const uint8 *ptr = SourceMap(mmd->source);
            DEBUG_LOG("**** %x %x %x %x ****", ptr[0], ptr[1], ptr[2], ptr[3]);
            DEBUG_LOG("**** %x %x %x %x ****", ptr[4], ptr[5], ptr[6], ptr[7]);
            DEBUG_LOG("**** %x %x %x %x ****", ptr[8], ptr[9], ptr[10], ptr[11]);
#endif

            MessageCancelFirst(KymeraGetTask(), KYMERA_INTERNAL_A2DP_MESSAGE_MORE_DATA_TIMEOUT);

            appKymeraConfigureAndStartHashTransform(theKymera, theKymera->a2dp_seid, mmd->source);

            /* Not interested in any more messages */
            SourceConfigure(mmd->source, VM_SOURCE_MESSAGES, VM_MESSAGES_NONE);
            MessageStreamTaskFromSource(mmd->source, NULL);
            MessageCancelAll(KymeraGetTask(), MESSAGE_MORE_DATA);

            if (theKymera->sync_info.mode != KYMERA_AUDIO_SYNC_START_PRIMARY_UNSYNCHRONISED)
            {
                /* No timeout is required if the MESSAGE_MORE_DATA is received
                   after the data sync indication. The state is set to
                   KYMERA_AUDIO_SYNC_STATE_IN_PROGRESS when data sync indication
                   is received */
                if (theKymera->sync_info.state == KYMERA_AUDIO_SYNC_STATE_INIT)
                {
                    /* Cancel any pending timeout messages */
                    MessageCancelAll(KymeraGetTask(), KYMERA_INTERNAL_A2DP_DATA_SYNC_IND_TIMEOUT);

                    /* schedule a message to start audio in unsychronised mode if A2DP data sync
                    * indication doesn't arrive within expected time.
                    */
                    MessageSendLater(KymeraGetTask(),
                                    KYMERA_INTERNAL_A2DP_DATA_SYNC_IND_TIMEOUT,
                                    NULL, A2DP_MIRROR_DATA_SYNC_IND_TIMEOUT_MS);
                }
            }
        }
    }
}

#endif /* INCLUDE_MIRRORING */
