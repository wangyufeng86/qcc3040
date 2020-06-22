/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  conv_from_audio_struct.h
 * \ingroup capabilities
 *
 * convert_from_audio capability C header file to be translated to asm header. <br>
 *
 */

#ifndef CONVERT_FROM_AUDIO_STRUCT_H
#define CONVERT_FROM_AUDIO_STRUCT_H

/****************************************************************************
Private Type Declarations
*/

/* convert_from_audio based on ENCODER infrastructure,
    here is "codec" specific data */
typedef struct conv_from_audio_codec
{
    /* used for assessing whether the inner (C part) frame_process produced something */
    unsigned samples_to_convert;
} conv_from_audio_codec;

#endif /* CONVERT_FROM_AUDIO_STRUCT_H */
