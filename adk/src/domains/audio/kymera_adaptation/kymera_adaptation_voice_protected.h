/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Voice source specific parameters used in the kymera adaptation layer
*/

#ifndef KYMERA_VOICE_PROTECTED_H_
#define KYMERA_VOICE_PROTECTED_H_

#include "volume_types.h"
#include <stream.h>

typedef enum
{
    hfp_codec_mode_none,
    hfp_codec_mode_narrowband,
    hfp_codec_mode_wideband,
    hfp_codec_mode_ultra_wideband,
    hfp_codec_mode_super_wideband
} hfp_codec_mode_t;

typedef struct
{
    Sink audio_sink;
    hfp_codec_mode_t codec_mode;
    uint8 wesco;
    volume_t volume;
    uint8 pre_start_delay;
    bool allow_scofwd;
    bool allow_micfwd;
    uint8 tesco;
} voice_connect_parameters_t;

#endif /* KYMERA_VOICE_PROTECTED_H_ */
