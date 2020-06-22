/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       a2dp_profile_caps_aptx_adaptive.c
\defgroup   a2dp_profile_aptx_adaptive A2DP profile aptx adaptive
\ingroup    a2dp_profile
\brief      A2DP profile for aptX Adaptive
*/

#include "a2dp_profile_caps.h"
#include "a2dp_profile_caps_aptx_adaptive.h"
#include <a2dp.h>
#include <av.h>
#include <byte_utils.h>


#define RESERVED                      (0x00)
#define LENGTH_OF_CP_TYPE_SCMS_VALUE  (2)
#define OFFSET_FOR_LENGTH_OF_TWS_CAPS (10)


/*!
 *  Hardcoded aptX adaptive LL Mode latency values
 *  Value is set by latency / 5
 *  AOSP Generic Mode enabled
 */

#ifdef INCLUDE_STEREO
#define APTX_AD_MIN_LL_TTP_5G   (uint8)(0x30)  /* base TTP + 15 (3*5) */
#define APTX_AD_MIN_LL_TTP_2G4  (uint8)(0x7)   /* 5G TTP + 35 (7*5) */
#define APTX_AD_MAX_LL_TTP_5G   (uint8)(0x60)  /* base TTP + 30 */
#define APTX_AD_MAX_LL_TTP_2G4  (uint8)(0xA)   /* Max LL 5G TTP + 50 */
#else
/*!
 *  Earbud min/max latency is higher than headset for LL
 */
#define APTX_AD_MIN_LL_TTP_5G   (uint8)(0xF0)  /* base TTP + 75 (15*5) */
#define APTX_AD_MIN_LL_TTP_2G4  (uint8)(0x7)   /* 5G TTP + 35 (7*5) */
#define APTX_AD_MAX_LL_TTP_5G   (uint8)(0xF0)  /* base TTP + 75 */
#define APTX_AD_MAX_LL_TTP_2G4  (uint8)(0x7)   /* Max LL 5G TTP + 35 */
#endif

/*!
 *  Hardcoded aptX adaptive LL Mode latency values
 *  AOSP Generic Mode disabled
 */
#define APTX_AD_LL_TTP_MIN_IN_1MS  0        // Minimum latency in milliseconds for low-latency mode
#define APTX_AD_LL_TTP_MAX_IN_4MS  125      // Max latency for low-latency mode in 4ms units (i.e. 125*4ms)

/*!
 *  Hardcoded aptX adaptive HQ and TWS Mode latency values
 *  AOSP Generic Mode disabled
 */

#ifdef INCLUDE_STEREO
#define APTX_AD_HQ_TTP_MIN_IN_1MS  200      /* Minimum latency in milliseconds for HQ mode */
#else
#define APTX_AD_HQ_TTP_MIN_IN_1MS  250      /* Minimum latency in milliseconds for HQ mode for earbuds */
#endif
#define APTX_AD_HQ_TTP_MAX_IN_4MS  125      /* Max latency for HQ mode in 4ms units (i.e. 125*4ms) */
#define APTX_AD_TWS_TTP_MIN_IN_1MS 100      /* Minimum latency in milliseconds for TWS mode */
#define APTX_AD_TWS_TTP_MAX_IN_4MS 125      /* Max latency for TWS mode in 4ms units (i.e. 125*4ms) */

#define APTX_AD_CAPABILITY_EXTENSION_END (0xAA)

/* Max TTP latency values in the service capability are not in units of 1ms */
#define MAX_TTP_LATENCY_UNIT_IN_MS (4)

#define REPEAT_OCTET_5_TIMES(x)  (x), (x), (x), (x), (x)
#define REPEAT_OCTET_10_TIMES(x) REPEAT_OCTET_5_TIMES(x), REPEAT_OCTET_5_TIMES(x)
#define REPEAT_OCTET_26_TIMES(x) REPEAT_OCTET_10_TIMES(x), REPEAT_OCTET_10_TIMES(x), REPEAT_OCTET_5_TIMES(x), (x)


/*!
 * The number of octets the aptX Adaptive service capability
 */
typedef enum
{
    length_of_aptx_adaptive_cap = 42,
    length_of_tws_aptx_adaptive_cap = OFFSET_FOR_LENGTH_OF_TWS_CAPS + length_of_aptx_adaptive_cap
} length_of_aptx_ad_service_capability_t;


#define APTX_AD_EMBEDDED_SERVICE_CAPABILITY \
    AVDTP_SERVICE_MEDIA_CODEC, \
    length_of_aptx_adaptive_cap, \
    AVDTP_MEDIA_TYPE_AUDIO << 2, \
    AVDTP_MEDIA_CODEC_NONA2DP,\
    SPLIT_IN_4_OCTETS(A2DP_QTI_VENDOR_ID), \
    SPLIT_IN_2_OCTETS(A2DP_QTI_APTX_AD_CODEC_ID), \
    aptx_ad_sample_rate_48000 | aptx_ad_sample_rate_44100 | aptx_ad_generic_supported ,\
    aptx_ad_channel_mode_joint_stereo | aptx_ad_channel_mode_stereo, \
    APTX_AD_MIN_LL_TTP_5G | APTX_AD_MIN_LL_TTP_2G4, \
    APTX_AD_MAX_LL_TTP_5G | APTX_AD_MAX_LL_TTP_2G4, \
    APTX_AD_HQ_TTP_MIN_IN_1MS, \
    APTX_AD_HQ_TTP_MAX_IN_4MS, \
    APTX_AD_TWS_TTP_MIN_IN_1MS, \
    APTX_AD_TWS_TTP_MAX_IN_4MS, \
    REPEAT_OCTET_26_TIMES(0),\
    AVDTP_SERVICE_CONTENT_PROTECTION, \
    LENGTH_OF_CP_TYPE_SCMS_VALUE, \
    AVDTP_CP_TYPE_SCMS_LSB, \
    AVDTP_CP_TYPE_SCMS_MSB, \
    AVDTP_SERVICE_DELAY_REPORTING, \
    0


/*!
 * Default aptX Adaptive Capabilities for the application to pass to the A2DP library during initialisation.
 *  NOTE: The capability is modified by A2dpProfileAptxAdInitServiceCapability() before passing it to the A2DP library,
 *  therefore it is the end result of this modification that reflects the "real" service capability and this array
 *  initialisation is merely used as a base to simplify the code it.
 *  The octets populated by A2dpProfileAptxAdInitServiceCapability() are initialised here as RESERVED (as well as octets
 *  that are actually reserved for future use).
 */
uint8 aptx_ad_caps_sink[52] =
{
    AVDTP_SERVICE_MEDIA_TRANSPORT,
    0,
    APTX_AD_EMBEDDED_SERVICE_CAPABILITY
};

/*!
 * True Wireless Stereo service capability for aptX Adaptive
 */
const uint8 tws_aptx_ad_caps[62] =
{
    AVDTP_SERVICE_MEDIA_TRANSPORT,
    0,
    AVDTP_SERVICE_MEDIA_CODEC,
    length_of_tws_aptx_adaptive_cap,
    AVDTP_MEDIA_TYPE_AUDIO << 2,
    AVDTP_MEDIA_CODEC_NONA2DP,
    SPLIT_IN_4_OCTETS(A2DP_CSR_VENDOR_ID),
    SPLIT_IN_2_OCTETS(A2DP_CSR_TWS_APTX_AD_CODEC_ID),
    APTX_AD_EMBEDDED_SERVICE_CAPABILITY
};


/*!@{ \name Standard sink endpoints
    \brief Predefined endpoints for audio Sink end point configurations  */
    /*! APTX Adaptive */
const sep_config_type av_aptx_adaptive_snk_sep = {AV_SEID_APTX_ADAPTIVE_SNK,     DECODE_RESOURCE_ID, sep_media_type_audio, a2dp_sink, TRUE, 0, sizeof(aptx_ad_caps_sink), aptx_ad_caps_sink};
/*!@} */

static uint8 * getStartOfCodecSpecificInformation(void)
{
    uint8 *service_caps = aptx_ad_caps_sink;

    while (service_caps[0] != AVDTP_SERVICE_MEDIA_CODEC)
        service_caps += service_caps[1] + 2;

    return service_caps;
}

static void setCapabilityVersionNumber(uint8 version_number)
{
    uint8 *aptx_adaptive_codec_caps = getStartOfCodecSpecificInformation();
    aptx_adaptive_codec_caps[aptx_ad_version_number_offset] = version_number;
}

static void setSupportedFeatures(uint32 supported_features)
{
    uint8 *aptx_adaptive_codec_caps = getStartOfCodecSpecificInformation();
    ByteUtilsSet4Bytes(aptx_adaptive_codec_caps, aptx_ad_supported_features_start_offset, supported_features);
}

static void setSetupPreference(uint8 setup_preference, setup_preference_priority_t priority)
{
    uint8 *aptx_adaptive_codec_caps = getStartOfCodecSpecificInformation();
    aptx_adaptive_codec_caps[aptx_ad_setup_preference_start_offset + priority] = setup_preference;
}

static void setCapabilityExtensionEndForR1(void)
{
    uint8 *aptx_adaptive_codec_caps = getStartOfCodecSpecificInformation();
    aptx_adaptive_codec_caps[aptx_ad_capability_extension_end_offset_for_r1] = APTX_AD_CAPABILITY_EXTENSION_END;
}

static void setCapabilityExtensionEndForR2(void)
{
    uint8 *aptx_adaptive_codec_caps = getStartOfCodecSpecificInformation();
    aptx_adaptive_codec_caps[aptx_ad_capability_extension_end_offset_for_r2] = APTX_AD_CAPABILITY_EXTENSION_END;
}

#ifdef ATPX_ADAPTIVE_SUPPORT_96K
static void addSampleRate(aptx_ad_sample_rates_t rate)
{
    uint8 *aptx_adaptive_codec_caps = getStartOfCodecSpecificInformation();
    aptx_adaptive_codec_caps[aptx_ad_sample_rate_offset] |= rate;
}
#endif
static void updateCapabilityBasedOnTheAdvertiseOption(aptx_ad_advertise_options_t advertise_option)
{
    switch (advertise_option)
    {
        case aptx_ad_advertise_r2:
            setCapabilityVersionNumber(0x01);
            setSupportedFeatures(0x0300000F);
            setSetupPreference(0x02, aptx_ad_setup_preference_1);
            setSetupPreference(0x03, aptx_ad_setup_preference_2);
            setSetupPreference(0x03, aptx_ad_setup_preference_3);
            setSetupPreference(0x03, aptx_ad_setup_preference_4);
            setCapabilityExtensionEndForR2();
            break;

        case aptx_ad_advertise_r1:
        default:
            setCapabilityVersionNumber(0x00);
            setCapabilityExtensionEndForR1();
            break;
    }
}

void A2dpProfileAptxAdInitServiceCapability(void)
{
#ifdef ATPX_ADAPTIVE_SUPPORT_R1_ONLY
     updateCapabilityBasedOnTheAdvertiseOption(aptx_ad_advertise_r1);
#else
    updateCapabilityBasedOnTheAdvertiseOption(aptx_ad_advertise_r2);
#endif

#ifdef ATPX_ADAPTIVE_SUPPORT_96K
    addSampleRate(aptx_ad_sample_rate_96000);
#endif
}

