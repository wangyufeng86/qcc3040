/*******************************************************************************
 * Copyright (c) 2009 - 2018 Qualcomm Technologies International, Ltd.
*******************************************************************************/

/**
 * \file  stream_loopback.c
 * \ingroup stream
 *
 * Control the loopback audio HW <br>
 * This file contains the stream loopback shim functions. <br>
 *
 *  \note This file contains the operator access functions for audio loopback control.
 *  It consists of several shim functions that provide an interface between instances of
 *  (downloaded) Kymera capabilities and the HAL layer (without which access would not
 *  be possible).
 *
 */

#if defined(INSTALL_AUDIO_LOOPBACK_ENHANCEMENTS)

/*******************************************************************************
Include Files
*/

#include "hal_audio_private.h"
#include "patch/patch.h"
#include "stream_for_opmgr.h"

/*******************************************************************************
 * Private macros/consts
 */

/*******************************************************************************
Private Function Declarations
*/

/*******************************************************************************
Private Function Definitions
*/

/*******************************************************************************
Public Function Definitions
*/

/**
 * \brief Control the Streplus audio loopback.
 *
 * \param stream_loopback_mode Desired loopback mode.
 * \param chan_swap_en FALSE: No-swap, TRUE: Swap; channel swap.
 * \param pwm_to_ana_en FALSE: Disable, TRUE: Enable; PWM to Analogue control.
 * \param pwm_to_ana_lpf_en FALSE: Disable, TRUE: Enable; PWM to Analogue LPF control.
 *
 */
void stream_loopback_config_test_enables(
    stream_loopback_mode_type stream_loopback_mode,
    bool chan_swap_en,
    bool pwm_to_ana_en,
    bool pwm_to_ana_lpf_en)
{
    uint16 hal_loopback_mode;

    patch_fn_shared(stream_audio_hydra);

    /* Translate stream type to HAL type */
    switch(stream_loopback_mode)
    {
        case STREAM_TEST_ENABLES_KCODEC_SHORT_DIG_LOOPBACK:
            hal_loopback_mode = AUDIO_TEST_ENABLES_KCODEC_SHORT_DIG_LOOPBACK;
        break;

        case STREAM_TEST_ENABLES_KCODEC_LONG_DIG_LOOPBACK:
            hal_loopback_mode = AUDIO_TEST_ENABLES_KCODEC_LONG_DIG_LOOPBACK;
        break;

        case STREAM_TEST_ENABLES_KCODEC_SHORT_ANA_LOOPBACK:
            hal_loopback_mode = AUDIO_TEST_ENABLES_KCODEC_SHORT_ANA_LOOPBACK;
        break;

        case STREAM_TEST_ENABLES_KCODEC_LONG_ANA_LOOPBACK:
            hal_loopback_mode = AUDIO_TEST_ENABLES_KCODEC_LONG_ANA_LOOPBACK;
        break;

        case STREAM_TEST_ENABLES_KCODEC_LOOPBACK_OFF:
            /* ...Drop through */

        default:
            /* Default to loopback off */
            hal_loopback_mode = AUDIO_TEST_ENABLES_KCODEC_LOOPBACK_OFF;
        break;
    }

    /* Call HAL layer loopback control function */
    hal_loopback_config_test_enables(
        hal_loopback_mode,
        chan_swap_en,
        pwm_to_ana_en,
        pwm_to_ana_lpf_en);
}

/**
 * \brief Control the Streplus audio loopback routing.
 *
 * \param adc_channel Destination ADC channel.
 * \param dac_channel Source DAC channel.
 *
 */
void stream_loopback_config_routing(
    uint16 adc_channel,
    uint16 dac_channel)
{
    patch_fn_shared(stream_audio_hydra);

    /* Call HAL layer loopback routing function */
    hal_loopback_config_routing(
        adc_channel,
        dac_channel);
}
#endif /* defined(INSTALL_AUDIO_LOOPBACK_ENHANCEMENTS) */
