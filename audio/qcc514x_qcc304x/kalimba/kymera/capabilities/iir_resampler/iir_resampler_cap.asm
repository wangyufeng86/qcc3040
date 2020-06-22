/****************************************************************************
 * Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/

#include "portability_macros.h"
#include "stack.h"
#include "iir_resampler_private_asm_defs.h"

#include "patch_library.h"

// *****************************************************************************
//    void iir_resampler_reset_internal(IIR_RESAMPLER_OP_DATA* op_extra_data,iir_resampler_channels *chan_ptr)
//
// MODULE:
//    $_iir_resampler_reset_internal
//
// DESCRIPTION:
//    reset/initialize the filter stages for each active channel
//    clear all history data and initializes history buffer pointers
//
// INPUTS:
//    - r0 = OPERATOR_EXTRA_DATA struct address
//    - r1 = channel pointer
//
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    follows C calling convention
//
// *****************************************************************************
.MODULE $M.iir_resampler_reset_internal;
   .CODESEGMENT PM;

$_iir_resampler_reset_internal:
   pushm <r4, r5, r6, r7, r8, rLink>;
   
   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($iir_resampler_cap.IIR_RESAMPLER_CAP_ASM.IIR_RESAMPLER_RESET.IIR_RESAMPLER_RESET_INTERNAL.PATCH_ID_0, r6)     // iir_patchers
   
   pushm <I0, I2>;
   push M0;

   // iir_resampler active channel list head
   r6 = r1;
   if Z jump done;

   // iir_resamplerv2 common config
   r7 = M[r0 + $iir_resampler_private.IIR_RESAMPLER_OP_DATA_struct.IIR_RESAMPLERV2_FIELD];

   // iir_resamplerv2 channel[0] struct pointer
   r8 = r7 + $iir_resampler_private.iir_resampler_internal_struct.CHANNEL_FIELD;

   // iir_resamplerv2 channel[0] working data block
   r4 = M[r7 + $iir_resampler_private.iir_resampler_internal_struct.WORKING_FIELD];

   // while(chan)
   next_channel:
      r0 = r7;                     // iir_resamplerv2 common
      call $reset_iir_resampler;

      // r4 has been updated to -> channel[n] working data block

      // iir_resamplerv2 channel[n] working data block
      r8 = r8 + ($iir_resampler_private.iir_resampler_channel_struct.STRUC_SIZE * ADDR_PER_WORD);

      // chan = chan->next
      r6 = M[r6 + $iir_resampler_private.multi_chan_channel_struc_struct.NEXT_ACTIVE_FIELD];     
   if NZ jump next_channel;

done:
   pop M0;
   popm <I0, I2>;
   popm <r4, r5, r6, r7, r8, rLink>;
   rts;

.ENDMODULE;

// *****************************************************************************
//    extern int iir_resampler_amount_to_use(IIR_RESAMPLER_OP_DATA* op_extra_data,iir_resampler_channels *chan_ptr,unsigned *available_data);
//
// MODULE:
//    $_iir_resampler_amount_to_use
//
// DESCRIPTION:
//    determine the number of input samples the resampler should consume.
//    this is based on the amount of data in the input buffers and amount
//    space in the output buffers. the resampler conversion ratio is used
//    to estimate how many samples may be consumed based on output space
//
// INPUTS:
//    - r0 = OPERATOR_EXTRA_DATA struct address
//    - r1 = chan_ptr
//    - r2 = &available_data
//
// OUTPUTS:
//    - r0 = amount to use
//
// TRASHED REGISTERS:
//    follows C calling convention
//
// *****************************************************************************
.MODULE $M.iir_resampler_amount_to_use;
   .CODESEGMENT PM;

$_iir_resampler_amount_to_use:
   pushm <r4,r5,r6,r7,r8,r9,rLink>;
   
   r9 = r1;    // chan_ptr 
   if Z jump jp_no_data_space;
   r8 = M[r0 + $iir_resampler_private.IIR_RESAMPLER_OP_DATA_struct.SOURCE_BLOCK_SIZE_FIELD];
   r5 = r0;    // OPERATOR_EXTRA_DATA
   r4 = r2;    // &available_data

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($iir_resampler_cap.IIR_RESAMPLER_CAP_ASM.IIR_RESAMPLER_AMOUNT_TO_USE.IIR_RESAMPLER_AMOUNT_TO_USE.PATCH_ID_0, r6)     // iir_patchers
      
   r7 = MAXINT; // min_space
   r6 = r7;     // min_data
   lp_next_channel:
      // Check space
      r0 = M[r9 + $iir_resampler_private.multi_chan_channel_struc_struct.SOURCE_BUFFER_PTR_FIELD];
      call $_cbuffer_calc_amount_space_in_words;
      NULL = r0 - r8;
      if NEG jump jp_no_data_space;
      r7 = MIN r0;

      // Check Data
      r0 = M[r9 + $iir_resampler_private.multi_chan_channel_struc_struct.SINK_BUFFER_PTR_FIELD];
      call $cbuffer.calc_amount_data_in_words;
      r6 = MIN r0;
      if Z jump jp_no_data_space;
      
      // chan = chan->next
      r9 = M[r9 + $iir_resampler_private.multi_chan_channel_struc_struct.NEXT_ACTIVE_FIELD];
   if NZ jump lp_next_channel;

   // pointer to iir_resamplerv2_common structure
   r0 = M[r5 + $iir_resampler_private.IIR_RESAMPLER_OP_DATA_struct.IIR_RESAMPLERV2_FIELD];
   r1 = r7;    // available_space
   // INPUTS:
   //    r0 = pointer to common config
   //    r1 = available space
   //
   // OUTPUTS:
   //    r1 = amount consumed (estimate)
   //
   // TRASHED REGISTERS:
   //    r0,r2,rMAC,I2,M0
   push I2;
   push M0;
   /* If not pass-through, then compute amout we will consume */
   Null = M[r0 + $iir_resampler_private.iir_resampler_common_struct.FILTER_FIELD];
   if NZ call $estimate_iir_resampler_consumed;
   pop M0;
   pop I2;
   // return amount consumed (r1)

   // For downsampling (consumed > produced). Can not consume more than available data 
   // For upsampling   (consumed < produced). Consumed may be zero if insufficient space 
   NULL = r6-r1;
   if NEG r1=r6;

   // Save available_data
   NULL = r4;
   if NZ M[r4]=r6;

   r0 = r1;       // consumed
   popm <r4,r5,r6,r7,r8,r9,rLink>;
   rts;

jp_no_data_space:
   r0 = -1;       // no data or space or channels
   popm <r4,r5,r6,r7,r8,r9,rLink>;
   rts;

.ENDMODULE;

// *****************************************************************************
//    unsigned iir_resampler_processing(IIR_RESAMPLER_OP_DATA* op_extra_data, unsigned amount_to_use,iir_resampler_channels *chan_ptr);
//
// MODULE:
//    $_iir_resampler_processing
//
// DESCRIPTION:
//    perform resampler processing on all active channels
//
// INPUTS:
//    - r0 = OPERATOR_EXTRA_DATA struct address
//    - r1 = number of input samples to consume
//    - r2 = chan_ptr
//
// OUTPUTS:
//    - r0 = number of output samples produced
//
// TRASHED REGISTERS:
//    follows C calling convention
//
// *****************************************************************************
.MODULE $M.iir_resampler_processing;
   .CODESEGMENT PM;

   .CONST IIR_RESAMPLEV2     1*ADDR_PER_WORD;
   .CONST AMOUNT_TO_USE      2*ADDR_PER_WORD;
   .CONST LOCAL_DATA_SIZE    AMOUNT_TO_USE;

$_iir_resampler_processing:
   PUSH_ALL_C
   
   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($iir_resampler_cap.IIR_RESAMPLER_CAP_ASM.IIR_RESAMPLER_PROCESSING.IIR_RESAMPLER_PROCESSING.PATCH_ID_0, r3)     // iir_patchers
   
   r0 = M[r0 + $iir_resampler_private.IIR_RESAMPLER_OP_DATA_struct.IIR_RESAMPLERV2_FIELD];

   // iir_resamplerv2 channel struct pointer
   r8 = r0 + $iir_resampler_private.iir_resampler_internal_struct.CHANNEL_FIELD;

   // save: iir_resamplev2, amount_to_use
   pushm <FP(=SP), r0, r1>;

   // iir_resamplerv2 channel[0] working data block
   r4 = M[r0 + $iir_resampler_private.iir_resampler_internal_struct.WORKING_FIELD];

   // r8 = iir_resamplerv2_channel pointer

   // while(chan)
   r9 = r2;
   next_channel:

      // setup input buffers I1/L1/B1
      r0 = M[r9 + $iir_resampler_private.multi_chan_channel_struc_struct.SINK_BUFFER_PTR_FIELD];
      call $cbuffer.get_read_address_and_size_and_start_address;
      push r2;
      pop B1;
      I1 = r0;
      L1 = r1;

      // setup output buffers I5/L5/B5
      r0 = M[r9 + $iir_resampler_private.multi_chan_channel_struc_struct.SOURCE_BUFFER_PTR_FIELD];
      call $cbuffer.get_write_address_and_size_and_start_address;
      push r2;
      pop B5;
      I5 = r0;
      L5 = r1;

      // iir_resamplerv2_common
      r0 = M[FP + IIR_RESAMPLEV2];

      // amount of input data to process
      r10 = M[FP + AMOUNT_TO_USE];

      pushm <r8,r9>;
      call $iir_perform_resample;
      popm <r8,r9>;

      // r4 has been updated to -> channel[n] working data block
      // r7 = amount_produced

      // increment iir_resamplerv2_channel pointer
      r8 = r8 + ($iir_resampler_private.iir_resampler_channel_struct.STRUC_SIZE * ADDR_PER_WORD);

      // chan = chan->next
      r9 = M[r9 + $iir_resampler_private.multi_chan_channel_struc_struct.NEXT_ACTIVE_FIELD];
   if NZ jump next_channel;

   SP = SP - LOCAL_DATA_SIZE;
   pop FP;

   // return amount produced (r7) (should be same across all channels)
   r0 = r7;

   POP_ALL_C
   rts;

.ENDMODULE;

