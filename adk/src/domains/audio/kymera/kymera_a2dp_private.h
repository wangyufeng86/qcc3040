/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_a2dp_private.h
\brief      Private (internal) kymera A2DP header file.

*/

#ifndef KYMERA_A2DP_PRIVATE_H
#define KYMERA_A2DP_PRIVATE_H

#include <a2dp.h>
#include <source.h>
#include <chain.h>

/*!@{ \name Useful gains in kymera operators format */
#define GAIN_HALF (-6 * KYMERA_DB_SCALE)
#define GAIN_FULL (0)
#define GAIN_MIN (-90 * KYMERA_DB_SCALE)
/*!@} */

#define MIXER_GAIN_RAMP_SAMPLES 24000

/*! Defines used for audio sync source configuration */

/* Synchronisation interval (in msec) for audio sync source stream.*/
#define AUDIO_SYNC_MS_INTERVAL (100)

/* MTU for audio sync source stream. It should be multiple of audio
 * sync sample (6 bytes). It has been set to 48 in order to fit the
 * the source stream packet in 2-DH1 (56 bytes) radio packet which
 * also contains 2 byte header and 4 bytes L2CAP header.
 */
#define AUDIO_SYNC_PACKET_MTU (48)

/*! \brief Timeout for A2DP Data Sync event (in milliseconds) */
#define A2DP_MIRROR_DATA_SYNC_IND_TIMEOUT_MS  (100)

/*! After starting A2DP mirroring, the maximum time to wait (in milliseconds) for a
    #MESSAGE_MORE_DATA before reverting to unsynchronised primary/secondary start */
#define A2DP_MIRROR_MESSAGE_MORE_DATA_TIMEOUT_MS (500)

/*! \brief Start A2DP master.

    \param codec_settings The A2DP codec settings to use.
    \param max_bitrate The max bitrate for the input stream (in bps). Ignored if zero.
    \param volume_in_db The initial volume to use.
    \param nq2q_ttp The aptX adaptive NQ2Q TTP Latency settings.

    \return TRUE if start is completed, else FALSE.

    In TWS legacy, the 'master' is the device that is connected to the handset
    and forwards media to the 'slave' device.
    In TWM, both devices are 'master' and no media is forwarded.

    The same API is used here to simplify the code in higher layers.
 */
bool appKymeraA2dpStartMaster(const a2dp_codec_settings *codec_settings, uint32 max_bitrate, int16 volume_in_db,
                              aptx_adaptive_ttp_latencies_t nq2q_ttp);

/*! \brief Start A2DP forwarding.

    \param codec_settings The A2DP codec settings to use.

    In TWS legacy, this function starts the master forwarding media to the slave.
    In TWM, this function starts 'forwarding' audio synchronisation.
 */
void appKymeraA2dpStartForwarding(const a2dp_codec_settings *codec_settings);

/*! \brief Stop A2DP forwarding.

    In TWS legacy, this function stops the master forwarding media to the slave.
    In TWM, this function stops 'forwarding' audio synchronisation.
 */
void appKymeraA2dpStopForwarding(Source source);

/*! \brief Stop A2DP operation.

    Common function to master and slave. */
void appKymeraA2dpCommonStop(Source source);

/*! \brief Start A2DP slave.

    \param codec_settings The A2DP codec settings to use.
    \param volume_in_db The initial volume to use.

    In TWS legacy, the 'master' is the device that is connected to the handset
    and forwards media to the 'slave' device.
    In TWM, both devices are 'master' and no media is forwarded,
    therefore this function is not used.
 */
void appKymeraA2dpStartSlave(a2dp_codec_settings *codec_settings, int16 volume_in_db);


/*! \brief Helper function to unpack a2dp codec settings into individual variables.

    \param codec_settings The A2DP codec settings to unpack.
    \param seid [out] The stream endpoint id.
    \param source [out] The media source.
    \param rate [out] The media sample rate in Hz.
    \param cp_enabled [out] Content protection enabled.
    \param mtu [out] Media channel L2CAP mtu.
 */
void appKymeraGetA2dpCodecSettingsCore(const a2dp_codec_settings *codec_settings,
                                              uint8 *seid, Source *source, uint32 *rate,
                                              bool *cp_enabled, uint16 *mtu);

/*! \brief Helper function to configure the RTP decoder.

    \param op The operator id of the RTP decoder.
    \param codec_type The codec type to configure.
    \param mode working mode to configure.
    \param rate The sample rate to configure in Hz.
    \param cp_enabled Content protection enabled.
    \param buffer_size The size of the buffer to use in words. If zero the buffer size will not be configured.
 */
void appKymeraConfigureRtpDecoder(Operator op, rtp_codec_type_t codec_type, rtp_working_mode_t mode, uint32 rate, bool cp_header_enabled, unsigned buffer_size);

/*! \brief Helper function to initially configure the l/r mixer in the A2DP input chain.

    \param chain The chain containing the left/right mixer.
    \param rate The sample rate to configure in Hz.
    \param stereo_lr_mix If TRUE the mixer will output a 50%/50% mix of the
            incoming stereo channels. If FALSE the mixer will output 100% left/right
            based on the is_left parameter.
    \param is_left Earbud is left/right.
 */
void appKymeraConfigureLeftRightMixer(kymera_chain_handle_t chain, uint32 rate, bool stereo_lr_mix, bool is_left);

/*! \brief Helper function to set/change the l/r mixer mode in the A2DP input chain.
    \param stereo_lr_mix If TRUE the mixer will output a 50%/50% mix of the
            incoming stereo channels. If FALSE the mixer will output 100% left/right
            based on the is_left parameter.
    \param is_left If TRUE, the earbud is left and 100% left channel media will
            be output by the mixer when stereo_lr_mix is FALSE.
            If FALSE, the earbud if right and 100% right channel media will be
            output by the mixer when stereo_lr_mix is FALSE.

 */
void appKymeraSetLeftRightMixerMode(kymera_chain_handle_t chain, bool stereo_lr_mix, bool is_left);

/*! \brief Get the audio buffer size required for SBC.
    \sbc_params Encoder parameters
    \latency_in_ms Milliseconds of audio the buffer should be able to hold.
    \return Audio buffer size in words (16-bit).
 */
unsigned appKymeraGetSbcEncodedDataBufferSize(const sbc_encoder_params_t *sbc_params, uint32 latency_in_ms);

/*! \brief Get the audio buffer size required based on bitrate and latency.
    \max_bitrate The maximum bitrate of the audio stream (in bps).
    \latency_in_ms Milliseconds of audio the buffer should be able to hold.
    \return Audio buffer size in words (16-bit).
 */
unsigned appKymeraGetAudioBufferSize(uint32 max_bitrate, uint32 latency_in_ms);

/*! \brief Reconfigure the SPC and mixer when in aptx classic mode
    \chain The chain containing the left/right mixer.
    \stereo_lr_mix if TRUE, enable the left / right mix. Feed data to both aptx decoders and mix at 50% volume each
    \is_left TRUE if we are the left channel
    \return Audio buffer size in words (16-bit).
 */
void appKymeraReConfigureClassicChain(kymera_chain_handle_t chain, bool stereo_lr_mix, bool is_left);

/*! \brief Convert the TTP Latency values passed in the capability exchange into values in milliseconds
    \param ttp_in_non_q2q_mode NQ2Q TTP Latency values from capability exchange.
    \param aptx_ad_ttp NQ2Q TTP Latency values for SSRC in milliseconds.
 */
void convertAptxAdaptiveTtpToOperatorsFormat(const aptx_adaptive_ttp_latencies_t ttp_in_non_q2q_mode,
                                             aptx_adaptive_ttp_in_ms_t *aptx_ad_ttp);

/*! \brief Adjust TTP Latency values to make sure they are between the recommended minimum and maximum values
    \param aptx_ad_ttp NQ2Q TTP Latency values for SSRC in milliseconds.
 */
void getAdjustedAptxAdaptiveTtpLatencies(aptx_adaptive_ttp_in_ms_t *aptx_ad_ttp);
#endif /* KYMERA_A2DP_PRIVATE_H */
