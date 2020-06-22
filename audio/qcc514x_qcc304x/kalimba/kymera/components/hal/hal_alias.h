/****************************************************************************
 * Copyright (c) 2011 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup HAL Hardware Abstraction Layer
 * \file  hal.h
 *
 * Public header file for HAL functions.
 * Currently just initialisation
 * Likely to get split between functional areas later.
 * \ingroup HAL
 *
 */

/****************************************************************************
Include Files
*/

#include "hal_macros.h"
#include "io_defs.h"

#ifndef HAL_ALIAS_H
#define HAL_ALIAS_H

#define hal_get_reg_proc_pio_status() hal_get_reg_audio_sys_pio_status()
#define hal_get_reg_proc_pio_status2() hal_get_reg_audio_sys_pio_status2()

#if defined(CHIP_AURA) || defined(CHIP_STREPLUS)
#define hal_set_audio_ana_ref_micbias_en(x) hal_set_audio_di_ref_micbias_en(x)
#define hal_get_audio_ana_ref_micbias_en(x) hal_get_audio_di_ref_micbias_en(x)
#define hal_set_reg_enable_private_ram(x) hal_set_reg_enable_fast_private_ram(x)
#define hal_set_audio_ana_adc_ch1_gain_sel(x) hal_set_audio_di_li1_selgain(x)
#define hal_set_audio_ana_adc_ch2_gain_sel(x) hal_set_audio_di_li2_selgain(x)
#endif /* CHIP_AURA || CHIP_STREPLUS*/

#define hal_set_clkgen_audio_enables_pcm0_en(x) hal_set_clkgen_audio_pcm0_en(x)

/* Add macros for other PCM instances, if they exist */
#ifdef INSTALL_AUDIO_INTERFACE_PCM
#if NUMBER_PCM_INSTANCES > 1
#define hal_set_clkgen_audio_enables_pcm1_en(x) hal_set_clkgen_audio_pcm1_en(x)
#endif
#if NUMBER_PCM_INSTANCES > 2
#define hal_set_clkgen_audio_enables_pcm2_en(x) hal_set_clkgen_audio_pcm2_en(x)
#endif
#if NUMBER_PCM_INSTANCES > 3
#define hal_set_clkgen_audio_enables_pcm3_en(x) hal_set_clkgen_audio_epcm0_en(x)
#define hal_set_audio_enables_pcm3_en(x) hal_set_audio_enables_epcm0_en(x)
#define hal_set_audio_enables_pcm3_in_en(x) hal_set_audio_enables_epcm0_in_en(x)
#define hal_set_audio_enables_pcm3_out_en(x) hal_set_audio_enables_epcm0_out_en(x)
#define hal_get_audio_enables_pcm3_en()  hal_get_audio_enables_epcm0_en()
#define hal_get_audio_enables_pcm3_in_en()  hal_get_audio_enables_epcm0_in_en()
#define hal_get_audio_enables_pcm3_out_en()  hal_get_audio_enables_epcm0_out_en()
#endif
#endif /* INSTALL_AUDIO_INTERFACE_PCM */

#define  hal_set_reg_proc_pio_drive(x) hal_set_reg_audio_sys_proc_pio_drive(x)
#define  hal_get_reg_proc_pio_drive() hal_get_reg_audio_sys_proc_pio_drive()

#define  hal_set_reg_proc_pio_drive2(x) hal_set_reg_audio_sys_proc_pio_drive2(x)
#define  hal_get_reg_proc_pio_drive2() hal_get_reg_audio_sys_proc_pio_drive2()

#define  hal_set_reg_proc_pio_drive_enable(x) hal_set_reg_audio_sys_proc_pio_drive_enable(x)
#define  hal_get_reg_proc_pio_drive_enable() hal_get_reg_audio_sys_proc_pio_drive_enable()

#define  hal_set_reg_proc_pio_drive_enable2(x) hal_set_reg_audio_sys_proc_pio_drive_enable2(x)
#define  hal_get_reg_proc_pio_drive_enable2() hal_get_reg_audio_sys_proc_pio_drive_enable2()


#if defined( CHIP_BASE_HYDRA)
#endif /* CHIP_BASE_HYDRA */


#ifdef CHIP_BASE_CRESCENDO
#endif /* CHIP_BASE_CRESCENDO */

#if defined(CHIP_GORDON)
#define INT_SOURCE_LOW_PRI_SW0            INT_SOURCE_INT_EVENT_SW0_LSB_POSN
#define INT_SOURCE_LOW_PRI_SW1            INT_SOURCE_INT_EVENT_SW1_LSB_POSN
#define INT_SOURCE_LOW_PRI_SW2            INT_SOURCE_INT_EVENT_SW2_LSB_POSN
#define INT_SOURCE_LOW_PRI_SW3            INT_SOURCE_INT_EVENT_SW3_LSB_POSN

#define INT_SOURCE_LOW_PRI_TIMER2         INTERRUPTS_INT_EVENT_TIMER2_LSB_POSN
#endif /* CHIP_GORDON */

#if defined (CHIP_BASE_CRESCENDO)
#define CHIP_INT_SOURCE_SW0 INT_SOURCE_SW0
#else
#define CHIP_INT_SOURCE_SW0 INT_SOURCE_LOW_PRI_SW0
#endif

#endif /* HAL_ALIAS_H */
