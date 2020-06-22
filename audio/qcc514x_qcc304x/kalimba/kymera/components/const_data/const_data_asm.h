/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file const_data_asm.h
 * \ingroup const_data
 *
 * This module handles platform independent constant data access.
 * To be called from assembly.
 */

#ifndef CONST_DATA_ASM_H
#define CONST_DATA_ASM_H

#define DSC_SIZE 2

/* Memory types */
#define MEM_TYPE_DMCONST            1
#define MEM_TYPE_DMCONST16          2
#define MEM_TYPE_DMCONST_WINDOWED16 3
#define MEM_TYPE_CONST              4
#define MEM_TYPE_CONST16            5
#define MEM_TYPE_INTERNAL           0
#define MEM_TYPE_MASK               0xF
#define MEM_TYPE_POSN               20

/* Data format encodings (packing types) */
#define FORMAT_DSP_NATIVE           0
#define FORMAT_16BIT_ZERO_PAD       1
#define FORMAT_16BIT_SIGN_EXT       2
#define FORMAT_PACKED16             3
#define FORMAT_MASK                 0xF
#define FORMAT_POSN                 16

#define DEFINE_CONST_TYPE_FORMAT(type,format)  (((type&MEM_TYPE_MASK)<<MEM_TYPE_POSN) | ((format&FORMAT_MASK)<<FORMAT_POSN))

/** This macro defines the descriptor in a platform independent manner */
#define DEFINE_CONST_DATA_DESCRIPTOR(type, format, const_data) DEFINE_CONST_TYPE_FORMAT(type,format), (const_data)

#endif /* CONSTANT_DATA_ASM_H */

