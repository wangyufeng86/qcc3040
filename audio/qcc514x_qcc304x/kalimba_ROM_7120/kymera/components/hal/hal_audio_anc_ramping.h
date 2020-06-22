/*****************************************************************************
 * Copyright (c) 2017 - 2017 Qualcomm Technologies International, Ltd.
 *****************************************************************************/

#ifndef HAL_AUDIO_ANC_RAMPING_H
#define HAL_AUDIO_ANC_RAMPING_H

#include "pl_timers/pl_timers.h"
#include "hal_audio.h"

typedef enum
{
    ANC0_FFA,
    ANC0_FFB,
    ANC1_FFA,
    ANC1_FFB
} hal_audio_anc_path_id_type;

/* Structure type to hold ANC path specific gain info. */
typedef struct
{
    hal_audio_anc_path_id_type path_id;
    tTimerId ramp_timer_id;
    uint32 timer_period;
    uint32 current_gain;
    uint32 target_gain;
    int32 gain_step;
    uint32 reg_gain;
} hal_audio_anc_path_struct_type;

/* Structure type to hold ANC instance specific gain info. */
typedef struct
{
    hal_audio_anc_path_struct_type ffa;
    hal_audio_anc_path_struct_type ffb;
    uint32 reg_ffgain_adaptive;
    uint32 reg_ffgain_zcd_en;
    uint32 reg_zcd_shift;
} hal_audio_anc_inst_gain_struct_type;

/* Structure type to pass ANC enable parameters to the timer callback */
typedef struct
{
    uint16 anc_enable_0;
    uint16 anc_enable_1;
    tTimerId enable_delay_tid;
} hal_audio_anc_enable_struct_type;

/* Structure type to hold ANC related MIB data */
typedef struct
{
    uint16 AncEnableDelay;
    uint16 AncRampUpTime;
    uint16 AncRampDownTime;
    bool mibsRead;
} mib_struct_type;

/* Structure type to hold ANC block gain info. */
typedef struct
{
    hal_audio_anc_enable_struct_type enable_struct;
    tTimerId ramp_timer_id;
    hal_audio_anc_inst_gain_struct_type anc0;
    hal_audio_anc_inst_gain_struct_type anc1;
    bool saved;
    mib_struct_type mib_struct;
    HAL_AUDIO_ANC_CALLBACK notifier;
} hal_audio_anc_gain_struct_type;

/* Prototypes */
void hal_audio_anc_alloc_gain_ramping(void);
void hal_audio_anc_read_mibs(void);
void hal_audio_anc_init_gain_ramping(hal_audio_anc_gain_struct_type *gain_struct_ptr, uint16 anc_enable_0, uint16 anc_enable_1);
void hal_audio_anc_ramp_timer_handler(void *params);
void hal_audio_anc_path_ramp_timer_handler(void *params);
void hal_audio_anc_cancel_ramp_timers(hal_audio_anc_gain_struct_type *gain_struct_ptr);

#endif /* HAL_AUDIO_ANC_RAMPING_H */
