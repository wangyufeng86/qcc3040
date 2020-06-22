// *****************************************************************************
// Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef CBOPS_HEADER_INCLUDED
#define CBOPS_HEADER_INCLUDED

#include "portability_macros.h"
#ifdef PATCH_LIBS
#include "patch_asm_macros.h"
#endif
// ** function vector parameters **
#include "cbops_vector_table.h"

   // multi-channel paraphernalia, proposed to be common to all cbops
   .CONST   $cbops.param_hdr.OPERATOR_DATA_PTR_FIELD    0*ADDR_PER_WORD;
   .CONST   $cbops.param_hdr.NR_INPUT_CHANNELS_FIELD    1*ADDR_PER_WORD;
   .CONST   $cbops.param_hdr.NR_OUTPUT_CHANNELS_FIELD   2*ADDR_PER_WORD;
   // input channel indexes start here, followed by output channel indexes later on in struct
   // after these input indexes.
   .CONST   $cbops.param_hdr.CHANNEL_INDEX_START_FIELD  3*ADDR_PER_WORD;

   // ** function vector parameters **
   #include "cbops_vector_table.h"

   // ** dc remove operator fields **
   #include "operators/cbops_dc_remove.h"

   // ** shift operator fields **
   #include "operators/cbops_shift.h"

   // ** rate adjustment and shift fields **
   #include "operators/cbops_rate_adjustment_and_shift.h"

   // ** simple copy operator fields **
   #include "operators/cbops_copy_op.h"

#endif // CBOPS_HEADER_INCLUDED
