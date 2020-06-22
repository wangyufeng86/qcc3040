/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_a2d_common.c
\brief      Kymera A2DP common functions.
*/

#include <a2dp.h>
#include <operator.h>
#include <operators.h>
#include <panic.h>
#include <opmsg_prim.h>

#include "kymera_chain_roles.h"

#include "kymera_a2dp_private.h"
#include "kymera_private.h"
#include "sbc.h"

#define CONVERSION_FACTOR_2MS_TO_1MS (2U)

/*! Default values for aptX adaptive NQ2Q TTP latency (in ms) */
#ifdef INCLUDE_STEREO
#define APTX_AD_TTP_LL0_MIN  55
#define APTX_AD_TTP_LL0_MAX 200
#define APTX_AD_TTP_LL1_MIN  75
#define APTX_AD_TTP_LL1_MAX 230
#define APTX_AD_TTP_HQ_MIN  200
#define APTX_AD_TTP_HQ_MAX  500
#define APTX_AD_TTP_TWS_MIN 200
#define APTX_AD_TTP_TWS_MAX 500
#else
/*! Earbud minimum latency is higher than max negotiated latency value  */
#define APTX_AD_TTP_LL0_MIN 120
#define APTX_AD_TTP_LL0_MAX 200
#define APTX_AD_TTP_LL1_MIN 130
#define APTX_AD_TTP_LL1_MAX 230
#define APTX_AD_TTP_HQ_MIN  300
#define APTX_AD_TTP_HQ_MAX  500
#define APTX_AD_TTP_TWS_MIN 300
#define APTX_AD_TTP_TWS_MAX 500
#endif

static uint32 divideAndRoundUp(uint32 dividend, uint32 divisor)
{
    if (dividend == 0)
        return 0;
    else
        return ((dividend - 1) / divisor) + 1;
}

void convertAptxAdaptiveTtpToOperatorsFormat(const aptx_adaptive_ttp_latencies_t ttp_in_non_q2q_mode,
                                             aptx_adaptive_ttp_in_ms_t *aptx_ad_ttp)
{
    aptx_ad_ttp->low_latency_0 = ttp_in_non_q2q_mode.low_latency_0_in_ms;
    aptx_ad_ttp->low_latency_1 = ttp_in_non_q2q_mode.low_latency_1_in_ms;
    aptx_ad_ttp->high_quality  = (uint16) (CONVERSION_FACTOR_2MS_TO_1MS * ttp_in_non_q2q_mode.high_quality_in_2ms);
    aptx_ad_ttp->tws_legacy    = (uint16) (CONVERSION_FACTOR_2MS_TO_1MS * ttp_in_non_q2q_mode.tws_legacy_in_2ms);
}

/* Adjust requested latency figures against defined minimum and maximum values for TWM */
void getAdjustedAptxAdaptiveTtpLatencies(aptx_adaptive_ttp_in_ms_t *aptx_ad_ttp)
{
    if (aptx_ad_ttp->low_latency_0 < APTX_AD_TTP_LL0_MIN || aptx_ad_ttp->low_latency_0 > APTX_AD_TTP_LL0_MAX)
        aptx_ad_ttp->low_latency_0 = APTX_AD_TTP_LL0_MIN;
    if (aptx_ad_ttp->low_latency_1 < APTX_AD_TTP_LL1_MIN || aptx_ad_ttp->low_latency_1 > APTX_AD_TTP_LL1_MAX)
        aptx_ad_ttp->low_latency_1 = APTX_AD_TTP_LL1_MIN;
    if (aptx_ad_ttp->high_quality < APTX_AD_TTP_HQ_MIN || aptx_ad_ttp->high_quality > APTX_AD_TTP_HQ_MAX)
        aptx_ad_ttp->high_quality = APTX_AD_TTP_HQ_MIN;
    if (aptx_ad_ttp->tws_legacy < APTX_AD_TTP_TWS_MIN || aptx_ad_ttp->tws_legacy > APTX_AD_TTP_TWS_MAX)
        aptx_ad_ttp->tws_legacy = APTX_AD_TTP_TWS_MIN;
}

void appKymeraGetA2dpCodecSettingsCore(const a2dp_codec_settings *codec_settings,
                                       uint8 *seid, Source *source, uint32 *rate,
                                       bool *cp_enabled, uint16 *mtu)
{
    if (seid)
    {
        *seid = codec_settings->seid;
    }
    if (source)
    {
        *source = StreamSourceFromSink(codec_settings->sink);
    }
    if (rate)
    {
        *rate = codec_settings->rate;
    }
    if (cp_enabled)
    {
        *cp_enabled = !!(codec_settings->codecData.content_protection);
    }
    if (mtu)
    {
        *mtu = codec_settings->codecData.packet_size;
    }
}

void appKymeraConfigureRtpDecoder(Operator op, rtp_codec_type_t codec_type, rtp_working_mode_t mode, uint32 rate, bool cp_header_enabled, unsigned buffer_size)
{
    const uint32 filter_gain = FRACTIONAL(0.997);
    const uint32 err_scale = FRACTIONAL(-0.00000001);
    /* Disable the Kymera TTP startup period, the other parameters are defaults. */
    const OPMSG_COMMON_MSG_SET_TTP_PARAMS ttp_params_msg = {
        OPMSG_COMMON_MSG_SET_TTP_PARAMS_CREATE(OPMSG_COMMON_SET_TTP_PARAMS,
            UINT32_MSW(filter_gain), UINT32_LSW(filter_gain),
            UINT32_MSW(err_scale), UINT32_LSW(err_scale),
            0)
    };

    OperatorsRtpSetCodecType(op, codec_type);
    OperatorsRtpSetWorkingMode(op, mode);
    OperatorsStandardSetTimeToPlayLatency(op, TWS_STANDARD_LATENCY_US);

    /* The RTP decoder controls the audio latency by assigning timestamps
    to the incoming audio stream. If the latency falls outside the limits (e.g.
    because the source delivers too much/little audio in a given time) the
    RTP decoder will reset its timestamp generator, returning to the target
    latency immediately. This will cause an audio glitch, but the AV sync will
    be correct and the system will operate correctly.

    Since audio is forwarded to the slave earbud, the minimum latency is the
    time at which the packetiser transmits packets to the slave device.
    If the latency were lower than this value, the packetiser would discard the audio
    frames and not transmit any audio to the slave, resulting in silence.
    */

    OperatorsStandardSetLatencyLimits(op, appConfigTwsTimeBeforeTx(), US_PER_MS*TWS_STANDARD_LATENCY_MAX_MS);

    if (buffer_size)
    {
        OperatorsStandardSetBufferSizeWithFormat(op, buffer_size, operator_data_format_encoded);
    }

    OperatorsRtpSetContentProtection(op, cp_header_enabled);
    /* Sending this message trashes the RTP operator sample rate */
    PanicFalse(OperatorMessage(op, ttp_params_msg._data, OPMSG_COMMON_MSG_SET_TTP_PARAMS_WORD_SIZE, NULL, 0));
    OperatorsStandardSetSampleRate(op, rate);
}

static void appKymeraGetLeftRightMixerGains(bool stereo_lr_mix, bool is_left, int *gain_l, int *gain_r)
{
    int gl, gr;

    if (stereo_lr_mix)
    {
        gl = gr = GAIN_HALF;
    }
    else
    {
        gl = is_left ? GAIN_FULL : GAIN_MIN;
        gr = is_left ? GAIN_MIN : GAIN_FULL;
    }
    *gain_l = gl;
    *gain_r = gr;
}

void appKymeraConfigureLeftRightMixer(kymera_chain_handle_t chain, uint32 rate, bool stereo_lr_mix, bool is_left)
{
    Operator mixer;

    /* The aptX adaptive decoder uses it's own internal downmix */
    if (GET_OP_FROM_CHAIN(mixer, chain, OPR_APTX_ADAPTIVE_DECODER))
    {
       DEBUG_LOG("appKymeraConfigureLeftRightMixer (aptx ad mode), %d, %d", stereo_lr_mix, is_left);
       OperatorsStandardSetAptXADChannelSelection(mixer, stereo_lr_mix, is_left);
    }
    else if (GET_OP_FROM_CHAIN(mixer, chain, OPR_LEFT_RIGHT_MIXER))
    {
        int gain_l, gain_r;

        appKymeraGetLeftRightMixerGains(stereo_lr_mix, is_left, &gain_l, &gain_r);

        OperatorsConfigureMixer(mixer, rate, 1, gain_l, gain_r, GAIN_MIN, 1, 1, 0);
        OperatorsMixerSetNumberOfSamplesToRamp(mixer, MIXER_GAIN_RAMP_SAMPLES);
    }
}

void appKymeraSetLeftRightMixerMode(kymera_chain_handle_t chain, bool stereo_lr_mix, bool is_left)
{
    Operator mixer;

    /* The aptX adaptive decoder uses it's own internal downmix */
    if (GET_OP_FROM_CHAIN(mixer, chain, OPR_APTX_ADAPTIVE_DECODER))
    {
       DEBUG_LOG("appKymeraSetLeftRightMixerMode (aptx ad mode), %d, %d", stereo_lr_mix, is_left);
       OperatorsStandardSetAptXADChannelSelection(mixer, stereo_lr_mix, is_left);
    }
    else if (GET_OP_FROM_CHAIN(mixer, chain, OPR_APTX_CLASSIC_MONO_DECODER_NO_AUTOSYNC))
    {/* Check for one instance of the classic decoder. This means we are aptX classic
       and we need to reconfigure the chain. */
        if appConfigEnableAptxStereoMix()
        {
            appKymeraReConfigureClassicChain(chain, stereo_lr_mix, is_left);
        }
    }
    else if (GET_OP_FROM_CHAIN(mixer, chain, OPR_LEFT_RIGHT_MIXER))
    {
        int gain_l, gain_r;

        appKymeraGetLeftRightMixerGains(stereo_lr_mix, is_left, &gain_l, &gain_r);

        OperatorsMixerSetGains(mixer, gain_l, gain_r, GAIN_MIN);
    }
}

void appKymeraReConfigureClassicChain(kymera_chain_handle_t chain, bool stereo_lr_mix, bool is_left)
{
    Operator mixer;

    DEBUG_LOG("appKymeraReConfigureClassicChain, %d, %d", stereo_lr_mix, is_left);

    if (GET_OP_FROM_CHAIN(mixer, chain, OPR_LEFT_RIGHT_MIXER))
    {
        int gain_l, gain_r;
        Operator op;
        /* Initialise vales to the right ear bud */
        chain_operator_role_t role = OPR_APTX_CLASSIC_MONO_DECODER_NO_AUTOSYNC;
        uint16_t mixer_port = 0;

        if (stereo_lr_mix) /* To dual passthrough mode */
        {
            if (is_left)
            { /* we we are left, we need to restart right channel */
                role = OPR_APTX_CLASSIC_MONO_DECODER_NO_AUTOSYNC_SECONDARY;
                mixer_port = 1;
            }
            op = PanicZero(ChainGetOperatorByRole(chain, role));
            Source aptx_mono = StreamSourceFromOperatorTerminal(op, 0);
            Sink mixer_in = StreamSinkFromOperatorTerminal(mixer, mixer_port);
            StreamConnect(aptx_mono, mixer_in);

            Operator op_list[] = {op};
            PanicFalse(OperatorStartMultiple(1, op_list, NULL));

            op = PanicZero(ChainGetOperatorByRole(chain, OPR_SWITCHED_PASSTHROUGH_CONSUMER));
            OperatorsSetSwitchedPassthruMode(op, spc_op_mode_tagsync_dual);
        }
        else /* To mono-mode */
        {
            spc_mode_t spc_mode = spc_op_mode_tagsync_1;
            if (is_left)
            {
                role = OPR_APTX_CLASSIC_MONO_DECODER_NO_AUTOSYNC_SECONDARY;
                spc_mode = spc_op_mode_tagsync_0;
                mixer_port = 1;
            }

            op = PanicZero(ChainGetOperatorByRole(chain, OPR_SWITCHED_PASSTHROUGH_CONSUMER));
            DEBUG_LOG("appKymeraReConfigureClassicChain [to mono] mode=%d, spc=%x", spc_mode, op);
            OperatorsSetSwitchedPassthruMode(op, spc_mode);

            op = PanicZero(ChainGetOperatorByRole(chain, role));
            Operator op_list[] = {op};
            PanicFalse(OperatorStopMultiple(1, op_list, NULL));


            Source aptx_mono_out = StreamSourceFromOperatorTerminal(op, 0);
            Sink mixer_in = StreamSinkFromOperatorTerminal(mixer, mixer_port);
            StreamDisconnect(aptx_mono_out, mixer_in);
        }

        appKymeraGetLeftRightMixerGains(stereo_lr_mix, is_left, &gain_l, &gain_r);
        DEBUG_LOG("appKymeraReConfigureClassicChain gainl=%d, gainr=%d", gain_l, gain_r);
        OperatorsMixerSetGains(mixer, gain_l, gain_r, GAIN_MIN);
        OperatorsMixerSetNumberOfSamplesToRamp(mixer, MIXER_GAIN_RAMP_SAMPLES);
    }
}

unsigned appKymeraGetSbcEncodedDataBufferSize(const sbc_encoder_params_t *sbc_params, uint32 latency_in_ms)
{
    uint32 frame_length = Sbc_GetFrameLength(sbc_params);
    uint32 bitrate = Sbc_GetBitrate(sbc_params);
    uint32 size_in_bits = divideAndRoundUp(latency_in_ms * bitrate, 1000);
    /* Round up this number if not perfectly divided by frame length to make sure we can buffer the latency required in SBC frames */
    uint32 num_frames = divideAndRoundUp(size_in_bits, frame_length * 8);
    size_in_bits = num_frames * frame_length * 8;
    unsigned size_in_words = divideAndRoundUp(size_in_bits, CODEC_BITS_PER_MEMORY_WORD);

    DEBUG_LOG("appKymeraGetSbcEncodedDataBufferSize: frame_length %u, bitrate %u, num_frames %u, buffer_size %u", frame_length, bitrate, num_frames, size_in_words);

    return size_in_words;
}

unsigned appKymeraGetAudioBufferSize(uint32 max_bitrate, uint32 latency_in_ms)
{
    uint32 size_in_bits = divideAndRoundUp(latency_in_ms * max_bitrate, 1000);
    unsigned size_in_words = divideAndRoundUp(size_in_bits, CODEC_BITS_PER_MEMORY_WORD);
    return size_in_words;
}
