/****************************************************************************
 * Copyright (c) 2016 - 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file hal_audio_sidetone.h
 * \ingroup hal
 *
 * Audio hardware abstraction layer for sidetone.
 */

#ifndef HAL_AUDIO_SIDETONE_H
#define HAL_AUDIO_SIDETONE_H

#include "hal/hal_audio_alias.h"

/*
    The following enum defines the different sidetone sources.
    SIDETONE_SOURCE_ADC_AB_OR_MIC_AB : ADC A is sidetone for DAC A.
                                       and ADC B is sidetone for DAC B.

    SIDETONE_SOURCE_MIC_CD           : ADC C is sidetone for DAC A.
                                       and ADC D is sidetone for DAC B.

    SIDETONE_SOURCE_MIC_EF           : ADC E is sidetone for DAC A.
                                       and ADC F is sidetone for DAC B.
*/
typedef enum
{
    SIDETONE_SOURCE_ADC_AB_OR_MIC_AB,
    SIDETONE_SOURCE_MIC_CD,
    SIDETONE_SOURCE_MIC_EF,
    SIDETONE_SOURCE_MIC_GH,
    SIDETONE_SOURCE_NONE
} sidetone_source_id;

/*
    The following enum defines the node in the ADC chain from which
    sidetone signal will be taken(and eventually fed to the DAC chain).

    SIDETONE_SOURCE_POINT_FROM_FINAL_FILTER : Sidetone data is taken from the
                                              o/p of digital filter.

    SIDETONE_SOURCE_POINT_FROM_DIGITAL_GAIN : Sidetone data is taken from the
                                              o/p of digital gain.
*/
typedef enum
{
    SIDETONE_SOURCE_POINT_FROM_FINAL_FILTER,
    SIDETONE_SOURCE_POINT_FROM_DIGITAL_GAIN
} sidetone_source_point;

/*
    The following enum defines the node in the DAC chain at which
    sidetone signal is inserted into DAC chain.

    SIDETONE_INJECTION_POINT_TO_INTERPOLATION : Sidetone data is inserted at
                                                i/p of interpolation.

    SIDETONE_INJECTION_POINT_TO_DIGITAL_GAIN   : Sidetone data is inserted at
                                                 i/p of digital gain stage.
*/
typedef enum
{
    SIDETONE_INJECTION_POINT_TO_INTERPOLATION,
    SIDETONE_INJECTION_POINT_TO_DIGITAL_GAIN
} sidetone_injection_point;

#define HAL_SIDETONE_MAX_GAIN_LEVELS 16
/**
 * \brief  Set gain for the audio sidetone HW
 *
  * \param  gain Desired value for sidetone gain.
 *
 * Dynamically set the sidetone gain via the stream configure interface.
 *
 *   Mapping of gain values (from CS-110025-SP):
 *
 *   0 = -32.6 dB
 *   1 = -30.1 dB
 *   2 = -26.6 dB
 *   3 = -24.1 dB
 *   4 = -20.6 dB
 *   5 = -18.1 dB
 *   6 = -14.5 dB
 *   7 = -12.0 dB
 */
extern bool hal_audio_sidetone_config_gain(uint16 gain);

/****************************************************************************
NAME
    hal_audio_sidetone_set_invert - Invert Phase of sidetone in DAC chain

FUNCTION
    Dynamically invert phase of sidetone in DAC chain.
*/
extern void hal_audio_sidetone_set_invert(audio_instance instance,
                                          audio_channel channel,
                                          bool val);

/****************************************************************************
NAME
    hal_audio_sidetone_config_source_mask - Configure the sidetone source
                                            mask

FUNCTION
    Dynamically sets the sidetone source mask.
*/
extern void hal_audio_sidetone_config_source_mask(audio_instance instance,
                                                  audio_channel channel,
                                                  unsigned source_mask);

/****************************************************************************
NAME
    hal_audio_sidetone_set_source_point - Configure the sidetone source
                                          point from ADC chain

FUNCTION
    Dynamically sets the sidetone source point in the ADC chain.
    Sidetone can be take either before the digital gain or after the digital
    gain.
*/
extern void hal_audio_sidetone_set_source_point(audio_instance instance,
                                                audio_channel channel,
                                                sidetone_source_point val);

/****************************************************************************
NAME
    hal_audio_sidetone_set_injection_point - Configure the sidetone
                                             injection point in DAC chain

FUNCTION
    Dynamically sets the sidetone injection point in the DAC chain.
    Sidetone can be take inserted before the digital gain or after the digital
    gain in DAC interpolation chain.
*/
extern void hal_audio_sidetone_set_injection_point(audio_instance instance,
                                                   audio_channel channel,
                                                   sidetone_injection_point val);

/****************************************************************************
NAME
    hal_audio_sidetone_set_individual_gain - Configure the sidetone
                                             gain of ADC channel.

FUNCTION
    Dynamically sets the sidetone gain of the ADC channel.
*/
extern bool hal_audio_sidetone_set_individual_gain(audio_instance instance,
                                                   audio_channel channel,
                                                   uint16 gain);

#endif /* HAL_AUDIO_SIDETONE_H */