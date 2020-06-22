/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  aac_decode_c.h
 * \ingroup  capabilities
 *
 *  Declaration of assembly functions used by the AAC decoder.
 */
 
#ifndef AAC_DECODE_C_H
#define AAC_DECODE_C_H

#include "codec/codec_c.h"

/* From "lib/aac/init_decoder.asm". */
extern void aac_decode_lib_init(DECODER *decoder);

/* From "lib/aac/reset_decoder.asm". */
extern void aac_decode_lib_reset(DECODER *decoder);
extern void aac_decode_free_decoder_twiddle(void);

/* From "capabilities/aac_decode/aac_decode_cap.asm" */
extern void populate_aac_asm_funcs(void (**decode_frame)(void), void (**silence)(void));

/* From "capabilities/aac_decode/aac_decode_cap.asm" */
extern void populate_strip_aac_asm_funcs(void (**decode_frame)(void),
                                         void (**silence)(void),
                                         void (**get_bits)(void));

#endif /* AAC_DECODE_C_H */