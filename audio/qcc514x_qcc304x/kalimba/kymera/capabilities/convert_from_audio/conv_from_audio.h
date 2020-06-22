/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  conv_from_audio.h
 * \ingroup capabilities
 *
 * convert_from_audio capability public header file. <br>
 *
 */

#ifndef CONVERT_FROM_AUDIO_CAPABILITY_H
#define CONVERT_FROM_AUDIO_CAPABILITY_H

#include "encoder/common_encode.h"


/** The capability data structure for conv_from_audio capability */
extern const CAPABILITY_DATA convert_from_audio_cap_data;

/****************************************************************************
Private Function Definitions
*/
/* Message handlers */
extern bool conv_from_audio_create( OPERATOR_DATA *op_data, void *message_data,
                                unsigned *response_id, void **response_data);

/**
 * \brief asm stub to call the C frame_process function
 *
 * \param encoder The generic encoder structure for the capability
 * \param encode_fn The address of the frame_encode function that will perform the encode
 *
 * \return True if something was encoded. False if nothing encoded.
 */
extern bool conv_from_audio_frame_process_asm( ENCODER *encoder, ENCODE_FN encode_fn);

extern void conv_from_audio_frame_process( ENCODER_PARAMS *params);

#endif /* CONVERT_FROM_AUDIO_CAPABILITY_H */
