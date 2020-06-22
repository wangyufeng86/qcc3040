/*!
\copyright  Copyright (c) 2017 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Kymera A2DP for TWS forwarding.
*/

#if (!defined INCLUDE_MIRRORING) && (!defined INCLUDE_STEREO)

#include "init.h"
#include "kymera_private.h"
#include "kymera_a2dp_private.h"
#include "kymera_config.h"
#include "av.h"
#include "a2dp_profile_config.h"
#include "multidevice.h"

/* L2CAP MTU targeting 2-DH5 packets. */
#define L2CAP_MTU_2DH5 672

static const chain_join_roles_t slave_inter_chain_connections[] =
{
    { .source_role = EPR_SOURCE_DECODED_PCM, .sink_role = EPR_SINK_MIXER_MAIN_IN }
};

static void appKymeraGetA2dpCodecSettingsSBC(const a2dp_codec_settings *codec_settings,
                                             sbc_encoder_params_t *sbc_encoder_params)
{
    uint8 sbc_format = codec_settings->codecData.format;
    sbc_encoder_params->channel_mode = codec_settings->channel_mode;
    sbc_encoder_params->bitpool_size = codec_settings->codecData.bitpool;
    sbc_encoder_params->sample_rate = codec_settings->rate;
    sbc_encoder_params->number_of_subbands = (sbc_format & 1) ? 8 : 4;
    sbc_encoder_params->number_of_blocks = (((sbc_format >> 4) & 3) + 1) * 4;
    sbc_encoder_params->allocation_method =  ((sbc_format >> 2) & 1);
}

/* These SBC encoder parameters are used when forwarding is disabled to save power */
static void appKymeraSetLowPowerSBCParams(kymera_chain_handle_t chain, uint32 rate)
{
    Operator op;

    if (GET_OP_FROM_CHAIN(op, chain, OPR_SBC_ENCODER))
    {
        sbc_encoder_params_t params;
        params.number_of_subbands = 4;
        params.number_of_blocks = 16;
        params.bitpool_size = 1;
        params.sample_rate = rate;
        params.channel_mode = sbc_encoder_channel_mode_mono;
        params.allocation_method = sbc_encoder_allocation_method_loudness;
        OperatorsSbcEncoderSetEncodingParams(op, &params);
    }
}

static sbc_encoder_params_t appKymeraGetLocalChannelSBCEncoderParams(uint32 rate)
{
    sbc_encoder_params_t params = {
        .number_of_subbands = 8,
        .number_of_blocks = 16,
        .bitpool_size = 32,
        .sample_rate = rate,
        .channel_mode = sbc_encoder_channel_mode_mono,
        .allocation_method = sbc_encoder_allocation_method_loudness
    };

    return params;
}

bool appKymeraA2dpStartMaster(const a2dp_codec_settings *codec_settings, uint32 max_bitrate, int16 volume_in_db,
                              aptx_adaptive_ttp_latencies_t nq2q_ttp)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    uint16 mtu;
    bool cp_header_enabled;
    uint32 rate;
    uint8 seid;
    Source media_source;

    UNUSED(nq2q_ttp);

    appKymeraGetA2dpCodecSettingsCore(codec_settings, &seid, &media_source, &rate, &cp_header_enabled, &mtu);
    max_bitrate = (max_bitrate) ? max_bitrate : (MAX_CODEC_RATE_KBPS * 1000);

    switch (appKymeraGetState())
    {
        case KYMERA_STATE_A2DP_STARTING_A:
        {
            unsigned kick_period = KICK_PERIOD_FAST;
            unsigned pcm_latency_buffer_size = PCM_LATENCY_BUFFER_SIZE;

            int16 volume_config = appConfigEnableSoftVolumeRampOnStart() ? VOLUME_MUTE_IN_DB : volume_in_db;
            DEBUG_LOG("appKymeraA2dpStartMaster, creating output chain, completing startup");
            switch (seid)
            {
                case AV_SEID_SBC_SNK:
                    kick_period = KICK_PERIOD_MASTER_SBC;
                    if (appConfigSbcNoPcmLatencyBuffer())
                    {
                        pcm_latency_buffer_size = 0;
                    }
                    break;
                case AV_SEID_AAC_SNK:
                    kick_period = KICK_PERIOD_MASTER_AAC;
                    if (appConfigAacStereoForwarding() && appConfigAacNoPcmLatencyBuffer())
                    {
                        pcm_latency_buffer_size = 0;
                    }
                    break;
                case AV_SEID_APTX_SNK:
                    kick_period = KICK_PERIOD_MASTER_APTX;
                    if (appConfigAptxNoPcmLatencyBuffer())
                    {
                        pcm_latency_buffer_size = 0;
                    }
                    break;
            }
            theKymera->output_rate = rate;
            appKymeraCreateOutputChain(kick_period, pcm_latency_buffer_size, volume_config);
            appKymeraSetOperatorUcids(FALSE);

        }
        return FALSE;
        case KYMERA_STATE_A2DP_STARTING_B:
        {
            const chain_config_t *config = NULL;
            bool is_left = Multidevice_IsLeft();
            /* Create input chain */
            switch (seid)
            {
                case AV_SEID_SBC_SNK:
                    DEBUG_LOG("appKymeraA2dpStartMaster, create standard TWS SBC input chain");

                    if (appConfigSbcNoPcmLatencyBuffer())
                    {
                        config = is_left ? Kymera_GetChainConfigs()->chain_forwarding_input_sbc_left_no_pcm_buffer_config :
                                           Kymera_GetChainConfigs()->chain_forwarding_input_sbc_right_no_pcm_buffer_config;
                    }
                    else
                    {
                        config = is_left ? Kymera_GetChainConfigs()->chain_forwarding_input_sbc_left_config :
                                           Kymera_GetChainConfigs()->chain_forwarding_input_sbc_right_config;
                    }
                break;
                case AV_SEID_AAC_SNK:
                    DEBUG_LOG("appKymeraA2dpStartMaster, create standard TWS AAC input chain");
                    switch (appA2dpConvertSeidFromSinkToSource(AV_SEID_AAC_SNK))
                    {
                        case AV_SEID_AAC_STEREO_TWS_SRC:
                            config = is_left ? Kymera_GetChainConfigs()->chain_forwarding_input_aac_stereo_left_config :
                                               Kymera_GetChainConfigs()->chain_forwarding_input_aac_stereo_right_config;
                        break;

                        case AV_SEID_SBC_MONO_TWS_SRC:
                        case AV_SEID_SBC_SRC:
                            config = is_left ? Kymera_GetChainConfigs()->chain_forwarding_input_aac_left_config :
                                               Kymera_GetChainConfigs()->chain_forwarding_input_aac_right_config;
                        break;
                    }
                break;
                case AV_SEID_APTX_SNK:
                    DEBUG_LOG("appKymeraA2dpStartMaster, create standard TWS aptX input chain");

                    if (appConfigAptxNoPcmLatencyBuffer())
                    {
                        config = is_left ? Kymera_GetChainConfigs()->chain_forwarding_input_aptx_left_no_pcm_buffer_config :
                                           Kymera_GetChainConfigs()->chain_forwarding_input_aptx_right_no_pcm_buffer_config;
                    }
                    else
                    {
                        config = is_left ? Kymera_GetChainConfigs()->chain_forwarding_input_aptx_left_config :
                                           Kymera_GetChainConfigs()->chain_forwarding_input_aptx_right_config;
                    }
                break;

                default:
                    Panic();
                break;
            }
            theKymera->chain_input_handle = PanicNull(ChainCreate(config));
        }
        return FALSE;

        case KYMERA_STATE_A2DP_STARTING_C:
        {
            bool connected;
            int16 volume_config = appConfigEnableSoftVolumeRampOnStart() ? VOLUME_MUTE_IN_DB : volume_in_db;
            kymera_chain_handle_t chain_handle = theKymera->chain_input_handle;
            rtp_codec_type_t rtp_codec = -1;
            Operator op;
            Operator op_rtp_decoder = ChainGetOperatorByRole(chain_handle, OPR_RTP_DECODER);
            rtp_working_mode_t mode = rtp_decode;
            unsigned rtp_buffer_size = appKymeraGetAudioBufferSize(max_bitrate, PRE_DECODER_BUFFER_MS);

            switch (seid)
            {
                case AV_SEID_SBC_SNK:
                    DEBUG_LOG("appKymeraA2dpStartMaster, configure standard TWS SBC input chain");
                    rtp_codec = rtp_codec_type_sbc;
                    appKymeraSetLowPowerSBCParams(chain_handle, rate);
                    op = PanicZero(ChainGetOperatorByRole(chain_handle, OPR_SWITCHED_PASSTHROUGH_CONSUMER));
                    OperatorsSetSwitchedPassthruEncoding(op, spc_op_format_encoded);

                    if (appConfigSbcNoPcmLatencyBuffer())
                    {
                        sbc_encoder_params_t sbc_params = appKymeraGetLocalChannelSBCEncoderParams(rate);
                        unsigned latency_buffer_in_ms = TWS_STANDARD_LATENCY_MAX_MS - PRE_FORWARDING_BUFFER_MIN_MS;
                        unsigned latency_buffer_size = appKymeraGetSbcEncodedDataBufferSize(&sbc_params, latency_buffer_in_ms);
                        rtp_buffer_size = appKymeraGetAudioBufferSize(max_bitrate, PRE_FORWARDING_BUFFER_MIN_MS);
                        op = ChainGetOperatorByRole(chain_handle, OPR_LOCAL_CHANNEL_SBC_ENCODER);
                        OperatorsSbcEncoderSetEncodingParams(op, &sbc_params);
                        op = ChainGetOperatorByRole(chain_handle, OPR_LATENCY_BUFFER);
                        OperatorsSetPassthroughDataFormat(op, operator_data_format_encoded);
                        OperatorsStandardSetBufferSizeWithFormat(op, latency_buffer_size, operator_data_format_encoded);
                    }
                break;
                case AV_SEID_AAC_SNK:
                    DEBUG_LOG("appKymeraA2dpStartMaster, configure standard TWS AAC input chain");
                    rtp_codec = rtp_codec_type_aac;
                    op = PanicZero(ChainGetOperatorByRole(chain_handle, OPR_AAC_DECODER));
                    OperatorsRtpSetAacCodec(op_rtp_decoder, op);

                    if (appConfigAacStereoForwarding())
                    {
                        op = PanicZero(ChainGetOperatorByRole(chain_handle, OPR_SPLITTER));
                        OperatorsSplitterEnableSecondOutput(op, FALSE);
                        OperatorsSplitterSetDataFormat(op, operator_data_format_encoded);

                        if (appConfigAacNoPcmLatencyBuffer())
                        {
                            rtp_buffer_size = appKymeraGetAudioBufferSize(max_bitrate, TWS_STANDARD_LATENCY_MAX_MS);
                        }
                    }

                    op = ChainGetOperatorByRole(chain_handle, OPR_CONSUMER);
                    /* Operator is only present in right earbud audio chain */
                    if (op)
                    {
                        OperatorsSetSwitchedPassthruEncoding(op, spc_op_format_pcm);
                    }
                break;
                case AV_SEID_APTX_SNK:
                    DEBUG_LOG("appKymeraA2dpStartMaster, configure standard TWS aptX input chain");
                    rtp_codec = rtp_codec_type_aptx;
                    op = PanicZero(ChainGetOperatorByRole(chain_handle, OPR_APTX_DEMUX));
                    OperatorsStandardSetSampleRate(op, rate);
                    op = PanicZero(ChainGetOperatorByRole(chain_handle, OPR_SWITCHED_PASSTHROUGH_CONSUMER));
                    OperatorsStandardSetBufferSizeWithFormat(op, APTX_LATENCY_BUFFER_SIZE, operator_data_format_encoded);
                    OperatorsSetSwitchedPassthruEncoding(op, spc_op_format_encoded);
                    if (appConfigAptxNoPcmLatencyBuffer())
                    {
                        unsigned latency_buffer_in_ms = TWS_STANDARD_LATENCY_MAX_MS - PRE_FORWARDING_BUFFER_MIN_MS;
                        unsigned latency_buffer_size = MS_TO_BUFFER_SIZE_CODEC(latency_buffer_in_ms, APTX_MONO_CODEC_RATE_KBPS);
                        rtp_buffer_size = MS_TO_BUFFER_SIZE_CODEC(PRE_FORWARDING_BUFFER_MIN_MS, APTX_STEREO_CODEC_RATE_KBPS);
                        op = PanicZero(ChainGetOperatorByRole(chain_handle, OPR_LATENCY_BUFFER));
                        OperatorsSetPassthroughDataFormat(op, operator_data_format_encoded);
                        OperatorsStandardSetBufferSizeWithFormat(op, latency_buffer_size, operator_data_format_encoded);
                    }
                    if (!cp_header_enabled)
                    {
                        mode = rtp_ttp_only;
                    }
                break;
            }

            appKymeraConfigureRtpDecoder(op_rtp_decoder, rtp_codec, mode, rate, cp_header_enabled, rtp_buffer_size);
            ChainConnect(theKymera->chain_input_handle);
            /* Connect input and output chains together */
            PanicFalse(ChainConnectInput(theKymera->chainu.output_vol_handle,
                            ChainGetOutput(theKymera->chain_input_handle, EPR_SOURCE_DECODED_PCM),
                            EPR_SINK_MIXER_MAIN_IN));

            /* Enable external amplifier if required */
            appKymeraExternalAmpControl(TRUE);

            /* Configure DSP for low power */
            appKymeraConfigureDspPowerMode();

            /* Connect media source to chain */
            StreamDisconnect(media_source, 0);
            /* The media source may fail to connect to the input chain if the source
            disconnects between the time A2DP asks Kymera to start and this
            function being called. A2DP will subsequently ask Kymera to stop. */
            connected = ChainConnectInput(theKymera->chain_input_handle, media_source, EPR_SINK_MEDIA);

            /* Start the output chain regardless of whether the source was connected
            to the input chain. Failing to do so would mean audio would be unable
            to play a tone. This would cause kymera to lock, since it would never
            receive a KYMERA_OP_MSG_ID_TONE_END and the kymera lock would never
            be cleared. */
            ChainStart(theKymera->chainu.output_vol_handle);
            if (connected)
            {
                ChainStart(theKymera->chain_input_handle);
            }

            if(AecLeakthrough_IsLeakthroughEnabled())
            {
                 /* Setting the usecase for AEC_REF to a2dp_leakthrough */
                 Kymera_SetAecUseCase(aec_usecase_a2dp_leakthrough);
                 Kymera_ConnectLeakthroughMic();
                 Kymera_LeakthroughEnableAecSideToneAfterTimeout();
            }

            if (volume_config == VOLUME_MUTE_IN_DB)
            {
                /* Setting volume after start results in ramp up */
                appKymeraSetVolume(theKymera->chainu.output_vol_handle, volume_in_db);
            }
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

    /* Disconnect A2DP source from the RTP operator then dispose */
    StreamDisconnect(source, 0);
    StreamConnectDispose(source);

    appKymeraDestroyOutputChain();

    /* Destroy chains now that input has been disconnected */
    ChainDestroy(theKymera->chain_input_handle);
    theKymera->chain_input_handle = NULL;

    /* Destroy packetiser */
    if (theKymera->packetiser)
    {
        TransformStop(theKymera->packetiser);
        theKymera->packetiser = NULL;
    }
}

void appKymeraA2dpStartForwarding(const a2dp_codec_settings *codec_settings)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    sbc_encoder_params_t sbc_encoder_params;
    uint16 mtu;
    bool cp_enabled;
    Operator op;
    vm_transform_packetise_codec p0_codec = VM_TRANSFORM_PACKETISE_CODEC_APTX;
    vm_transform_packetise_mode mode = VM_TRANSFORM_PACKETISE_MODE_TWSPLUS;
    uint8 seid;
    Source media_source;
    kymera_chain_handle_t inchain = theKymera->chain_input_handle;

    appKymeraGetA2dpCodecSettingsCore(codec_settings, &seid, &media_source, NULL, &cp_enabled, &mtu);
    appKymeraGetA2dpCodecSettingsSBC(codec_settings, &sbc_encoder_params);
    Source audio_source = ChainGetOutput(inchain, EPR_SOURCE_FORWARDING_MEDIA);
    DEBUG_LOG("appKymeraA2dpStartForwarding, media_source %p, audio_source %p, seid %d, state %u",
                media_source, audio_source, seid, appKymeraGetState());

    switch (seid)
    {
        case AV_SEID_APTX_MONO_TWS_SRC:
        break;

        case AV_SEID_SBC_MONO_TWS_SRC:
        case AV_SEID_SBC_SRC:
        {
            Operator sbc_encoder= ChainGetOperatorByRole(inchain, OPR_SBC_ENCODER);
            OperatorsSbcEncoderSetEncodingParams(sbc_encoder, &sbc_encoder_params);
            p0_codec = VM_TRANSFORM_PACKETISE_CODEC_SBC;
            mtu = MIN(mtu, L2CAP_MTU_2DH5);
        }
        break;

    case AV_SEID_AAC_STEREO_TWS_SRC:
        p0_codec = VM_TRANSFORM_PACKETISE_CODEC_AAC;
        mode = VM_TRANSFORM_PACKETISE_MODE_TWS;
        break;

        default:
            /* Unsupported SEID, control should never reach here */
            Panic();
        break;
    }

    theKymera->packetiser = TransformPacketise(audio_source, StreamSinkFromSource(media_source));
    TransformConfigure(theKymera->packetiser, VM_TRANSFORM_PACKETISE_CODEC, p0_codec);
    TransformConfigure(theKymera->packetiser, VM_TRANSFORM_PACKETISE_MODE, mode);
    TransformConfigure(theKymera->packetiser, VM_TRANSFORM_PACKETISE_MTU, mtu);
    TransformConfigure(theKymera->packetiser, VM_TRANSFORM_PACKETISE_TIME_BEFORE_TTP, appConfigTwsTimeBeforeTx() / 1000);
    TransformConfigure(theKymera->packetiser, VM_TRANSFORM_PACKETISE_LATEST_TIME_BEFORE_TTP, appConfigTwsDeadline() / 1000);
    TransformConfigure(theKymera->packetiser, VM_TRANSFORM_PACKETISE_CPENABLE, cp_enabled);
    TransformStart(theKymera->packetiser);

    if (GET_OP_FROM_CHAIN(op, inchain, OPR_SPLITTER))
    {
        OperatorsSplitterEnableSecondOutput(op, TRUE);
    }
    if (GET_OP_FROM_CHAIN(op, inchain, OPR_SWITCHED_PASSTHROUGH_CONSUMER))
    {
        appKymeraConfigureSpcMode(op, FALSE);
    }
}

void appKymeraA2dpStopForwarding(Source source)
{
    Operator op;
    kymeraTaskData *theKymera = KymeraGetTaskData();
    kymera_chain_handle_t inchain = theKymera->chain_input_handle;

    UNUSED(source);

    DEBUG_LOG("appKymeraA2dpStopForwarding");

    if (GET_OP_FROM_CHAIN(op, inchain, OPR_SPLITTER))
    {
        OperatorsSplitterEnableSecondOutput(op, FALSE);
    }
    if (GET_OP_FROM_CHAIN(op, inchain, OPR_SWITCHED_PASSTHROUGH_CONSUMER))
    {
        appKymeraConfigureSpcMode(op, TRUE);
    }

    if (theKymera->packetiser)
    {
        TransformStop(theKymera->packetiser);
        theKymera->packetiser = NULL;
    }

    appKymeraSetLowPowerSBCParams(inchain, theKymera->output_rate);
}

void appKymeraA2dpStartSlave(a2dp_codec_settings *codec_settings, int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    unsigned kick_period = 0;
    vm_transform_packetise_codec p0_codec = VM_TRANSFORM_PACKETISE_CODEC_APTX;
    vm_transform_packetise_mode mode = VM_TRANSFORM_PACKETISE_MODE_TWSPLUS;
    Operator op;
    uint16 mtu;
    bool cp_enabled;
    uint32 rate;
    uint8 seid;
    Source media_source;
    int16 volume_config = appConfigEnableSoftVolumeRampOnStart() ? VOLUME_MUTE_IN_DB : volume_in_db;

    appKymeraGetA2dpCodecSettingsCore(codec_settings, &seid, &media_source, &rate, &cp_enabled, &mtu);

    /* Create input chain */
    switch (seid)
    {
        case AV_SEID_APTX_MONO_TWS_SNK:
        {
            DEBUG_LOG("appKymeraA2dpStartSlave, TWS+ aptX");
            theKymera->chain_input_handle = ChainCreate(Kymera_GetChainConfigs()->chain_aptx_mono_no_autosync_decoder_config);
            kick_period = KICK_PERIOD_SLAVE_APTX;
        }
        break;
        case AV_SEID_APTX_ADAPTIVE_TWS_SNK:
        {
            DEBUG_LOG("appKymeraA2dpStartSlave, TWS+ aptX adaptive");
            theKymera->chain_input_handle = ChainCreate(Kymera_GetChainConfigs()->chain_aptx_ad_tws_plus_decoder_config);
            kick_period = KICK_PERIOD_SLAVE_APTX;
        }
        break;
        case AV_SEID_SBC_MONO_TWS_SNK:
        {
            DEBUG_LOG("appKymeraA2dpStartSlave, TWS+ SBC");
            theKymera->chain_input_handle = ChainCreate(Kymera_GetChainConfigs()->chain_sbc_mono_no_autosync_decoder_config);
            kick_period = KICK_PERIOD_SLAVE_SBC;
            p0_codec = VM_TRANSFORM_PACKETISE_CODEC_SBC;
        }
        break;
        case AV_SEID_AAC_STEREO_TWS_SNK:
        {
            DEBUG_LOG("appKymeraA2dpStartSlave, TWS AAC");
            const chain_config_t *config = Multidevice_IsLeft() ?
                                        Kymera_GetChainConfigs()->chain_aac_stereo_decoder_left_config :
                                        Kymera_GetChainConfigs()->chain_aac_stereo_decoder_right_config;
            theKymera->chain_input_handle = ChainCreate(config);
            if (GET_OP_FROM_CHAIN(op, theKymera->chain_input_handle, OPR_CONSUMER))
            {
                OperatorsSetSwitchedPassthruEncoding(op, spc_op_format_pcm);
            }
            kick_period = KICK_PERIOD_SLAVE_AAC;
            p0_codec = VM_TRANSFORM_PACKETISE_CODEC_AAC;
            mode = VM_TRANSFORM_PACKETISE_MODE_TWS;
        }
        break;
        default:
            Panic();
        break;
    }

    op = ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_LATENCY_BUFFER);
    /* The slave should be able to buffer at least the equivalent of target latency in audio data */
    OperatorsStandardSetBufferSizeWithFormat(op, MS_TO_BUFFER_SIZE_CODEC(TWS_STANDARD_LATENCY_MS, MAX_CODEC_RATE_KBPS), operator_data_format_encoded);
    OperatorsSetSwitchedPassthruEncoding(op, spc_op_format_encoded);

    OperatorsFrameworkSetKickPeriod(kick_period);
    ChainConnect(theKymera->chain_input_handle);

    /* Configure DSP for low power */
    appKymeraConfigureDspPowerMode();

    /* Create output chain */
    theKymera->output_rate = rate;
    appKymeraCreateOutputChain(kick_period, 0, volume_config);
    appKymeraSetOperatorUcids(FALSE);

    /* Connect chains together */
    ChainJoin(theKymera->chain_input_handle, theKymera->chainu.output_vol_handle,
              DIMENSION_AND_ADDR_OF(slave_inter_chain_connections));

    /* Disconnect A2DP from dispose */
    StreamDisconnect(media_source, 0);

    theKymera->packetiser = TransformPacketise(media_source, ChainGetInput(theKymera->chain_input_handle, EPR_SINK_MEDIA));
    TransformConfigure(theKymera->packetiser, VM_TRANSFORM_PACKETISE_CODEC, p0_codec);
    TransformConfigure(theKymera->packetiser, VM_TRANSFORM_PACKETISE_MODE, mode);
    TransformConfigure(theKymera->packetiser, VM_TRANSFORM_PACKETISE_SAMPLE_RATE, (uint16)(rate & 0xffff));
    TransformConfigure(theKymera->packetiser, VM_TRANSFORM_PACKETISE_CPENABLE, cp_enabled);
    TransformStart(theKymera->packetiser);

    /* Enable external amplifier if required */
    appKymeraExternalAmpControl(TRUE);

    /* Switch to passthrough now the operator is fully connected */
    appKymeraConfigureSpcMode(op, FALSE);

    /* Start chains */
    ChainStart(theKymera->chain_input_handle);
    ChainStart(theKymera->chainu.output_vol_handle);

    if(AecLeakthrough_IsLeakthroughEnabled())
    {
        /* Setting the usecase for AEC_REF to a2dp_leakthrough */
        Kymera_SetAecUseCase(aec_usecase_a2dp_leakthrough);
        Kymera_ConnectLeakthroughMic();
        Kymera_LeakthroughEnableAecSideToneAfterTimeout();
    }

    if (volume_config == VOLUME_MUTE_IN_DB)
    {
        /* Setting volume after start results in ramp up */
        appKymeraSetVolume(theKymera->chainu.output_vol_handle, volume_in_db);
    }
}

#endif /* (!defined INCLUDE_MIRRORING) && (!defined INCLUDE_STEREO) */
