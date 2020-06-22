/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  sbc_decode_c.h
 * \ingroup  capabilities
 *
 *  Declaration of assembly functions used by the SBC decoder.
 */
 
#ifndef SBC_DECODE_C_H
#define SBC_DECODE_C_H

/* From "init_decoder.asm". */
extern void sbc_decode_lib_init(void *decoder);

/* From "reset_decoder.asm". */
extern void sbc_decode_lib_reset(void *decoder);

/* From "sbc_decode_cap.asm". */
extern void populate_sbc_asm_funcs(void (**decode_frame)(void), void (**silence)(void));
extern void populate_strip_sbc_asm_funcs(void (**decode_frame)(void),
                                         void (**strip_decode_frame)(void),
                                         void (**get_bits)(void));

#endif /* SBC_DECODE_C_H */