/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\brief      Provides some helper APIs relating to SBC codec
*/

#include "sbc.h"

static uint32 sbc_DivideAndRoundUp(uint32 dividend, uint32 divisor)
{
    if (dividend == 0)
        return 0;
    else
        return ((dividend - 1) / divisor) + 1;
}

unsigned Sbc_GetFrameLength(const sbc_encoder_params_t *params)
{
    unsigned frame_length = 0;

    unsigned num_subbands = params->number_of_subbands;
    unsigned num_blocks = params->number_of_blocks;
    unsigned bitpool = params->bitpool_size;

    unsigned join = 0;
    unsigned num_channels = 2;

    /* As described in Bluetooth A2DP specification v1.3.2 section 12.9 */
    switch (params->channel_mode)
    {
        case sbc_encoder_channel_mode_mono:
            num_channels = 1;
        // fall-through
        case sbc_encoder_channel_mode_dual_mono:
            frame_length = 4 + ((4 * num_subbands * num_channels) / 8) + sbc_DivideAndRoundUp(num_blocks * num_channels * bitpool, 8);
        break;

        case sbc_encoder_channel_mode_joint_stereo:
            join = 1;
        // fall-through
        case sbc_encoder_channel_mode_stereo:
            frame_length = 4 + ((4 * num_subbands * num_channels) / 8) + sbc_DivideAndRoundUp((join * num_subbands) + (num_blocks * bitpool), 8);
        break;
    }

    return frame_length;
}

unsigned Sbc_GetBitrate(const sbc_encoder_params_t *params)
{
    uint32 sample_rate = params->sample_rate;
    uint32 num_subbands = params->number_of_subbands;
    uint32 num_blocks = params->number_of_blocks;
    uint32 frame_length = Sbc_GetFrameLength(params);

    /* As described in Bluetooth A2DP specification v1.3.2 section 12.9 */
    return sbc_DivideAndRoundUp((8 * frame_length * sample_rate) / num_subbands, num_blocks);
}
