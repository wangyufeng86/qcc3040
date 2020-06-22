/****************************************************************************
 * Copyright (c) 2009 - 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file hal_audio_warp.h
 * \ingroup hal
 *
 * Audio hardware abstraction layer for rate warping.
 */
#ifndef HAL_AUDIO_WARP_H
#define HAL_AUDIO_WARP_H

#include "hal_audio_alias.h"

#if defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM)

/**
 * \brief   List of KCODEC instances to which to apply a warp change,
 *          both as instance/channel of one device (to get the base warp),
 *          and a bit mask of all affected devices.
 */
typedef struct WARP_UPDATE_DESC
{
    audio_instance base_instance;
    audio_channel  base_channel;
    hal_audio_dir  direction    : 1;
    unsigned       channel_mask;
} WARP_UPDATE_DESC;

/**
 * \brief   Initialize a WARP_UPDATE_DESC.
 * \param   desc        The WARP_UPDATE_DESC struct
 * \param   direction   In/Out
 */
void hal_audio_init_warp_update_desc(WARP_UPDATE_DESC* desc,
                                       hal_audio_dir direction);

/**
 * \brief   Add a device instance/channel to a WARP_UPDATE_DESC
 * \param   desc        The WARP_UPDATE_DESC struct
 * \param   hw          Hardware type
 * \param   instance    Instance index
 * \param   channel     Channel index
 */
bool hal_audio_add_warp_update_desc(WARP_UPDATE_DESC* desc,
                                    audio_hardware hw,
                                    audio_instance instance,
                                    audio_channel channel);

/**
 * \brief  Apply warping
 *
 * \param  desc   Description of warp update destination
 * \param  value  The signed adjustment in FRACTIONAL (Q31 / Q23)
 * \param  report Rate to report back for measurement, positive value
 *                in right-justified Q1.22
 */
void hal_audio_set_warp(WARP_UPDATE_DESC* desc, int adjust, int* report);

#endif /* defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM) */

#endif /* HAL_AUDIO_WARP_H */