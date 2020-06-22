// *****************************************************************************
// Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

// *****************************************************************************
// NAME:
//    DC remove operator
//
// DESCRIPTION:
//    It is useful to remove any dc component from the signal going to the DAC
// (as this is bad for speakers) and also from the ADC (as this might affect
// the operation of the compression codec used).
//
// The dc component is estimated as follows:
// @verbatim
//    dc_est = old_dc_est * (1.0 - filter_coeff) + current_sample * filter_coeff
//
// where: old_dc_est, double precision fractional, is initialised to 0
//        filter_coeff, single precision fractional (= $cbuffer.dc_remove.FILTER_COEF)
// @endverbatim
//
// The dc is removed from the sample as follows:
// @verbatim
//    sample = sample - dc_est
// @endverbatim
//
//
// When using the multichannel operator the following data structure is used:
//    - header:
//              nr inputs
//              nr outputs (equal in this case)
//              <nr inputs> indexes for input channels (some may be marked as unused)
//              <nr outputs> indexes for output channels (some may be marked as unused)
//    - DC estimate values for each channel.
// *****************************************************************************

#include "stack.h"
#include "cbops.h"

.MODULE $M.cbops.dc_remove;
   .DATASEGMENT DM;

   // ** function vector for multi-channel cbop **
   // Recommendation is to standardise this table to include create() and such
   .VAR $cbops.dc_remove[$cbops.function_vector.STRUC_SIZE] =
      &$cbops.dc_remove.reset,        // reset function
      $cbops.basic_multichan_amount_to_use,   // amount to use function
      &$cbops.dc_remove.main;               // main function

.ENDMODULE;

// Expose the location of this table to C
.set $_cbops_dc_remove_table, $cbops.dc_remove


// *****************************************************************************
// MODULE:
//    $cbops.dc_remove.reset
//
// DESCRIPTION:
//    Reset routine for the DC remove operator multi-channel version,
//    see $cbops.dc_remove.main
//
// INPUTS:
//    - r8 = pointer to operator structure
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    DoLoop
//
// *****************************************************************************
.MODULE $M.cbops.dc_remove.reset;
   .CODESEGMENT CBOPS_DC_REMOVE_RESET_PM;
   .DATASEGMENT DM;

   // ** reset routine **
   $cbops.dc_remove.reset:


   // get number of input channels and start with first channel
   r10 = M[r8 + $cbops.param_hdr.NR_INPUT_CHANNELS_FIELD];

   r0 = M[r8 + $cbops.param_hdr.OPERATOR_DATA_PTR_FIELD];
   I0 = r0;
   r0 = NULL;

   do reset_channel;
      M[I0,MK1] = r0;    // DC offset estimate MSW
      M[I0,MK1] = r0;    // DC offset estimate LSW
   reset_channel:

   rts;

.ENDMODULE;



// *****************************************************************************
// MODULE:
//    $cbops.dc_remove.main
//
// DESCRIPTION:
//    Operator that removes any DC component from the input data (multi-channel version)
//
// INPUTS:
//    - r4 = buffer table
//    - r8 = pointer to operator structure
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    rMAC, r0-4, r10, I0, L0, I4, L4, DoLoop
//
// *****************************************************************************
.MODULE $M.cbops.dc_remove.main;
   .CODESEGMENT CBOPS_DC_REMOVE_MAIN_PM;
   .DATASEGMENT DM;

   $cbops.dc_remove.main:

   push rLink;
   // start profiling if enabled
   #ifdef ENABLE_PROFILER_MACROS
      .VAR/DM1 $cbops.profile_dc_remove[$profiler.STRUC_SIZE] = $profiler.UNINITIALISED, 0 ...;
      r0 = &$cbops.profile_dc_remove;
      call $profiler.start;
   #endif

   call $cbops.get_transfer_and_update_multi_channel;
   M3 = r0;
   if LE jump jp_done;

   // Offset to filter coefficients
   r6 = M[r8 + $cbops.param_hdr.OPERATOR_DATA_PTR_FIELD];
   I2 = r6;

   // channel counter, r9=num channels
   r7 = Null;

   // M3 = amount, r9=num_chans in addresses, r7=chan_num in addresses
 process_channel:
   // get the input index for current channel
   r5 = r7 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;

   // Setup Input Buffer
   r0 = M[r8 + r5];     // input index
   call $cbops.get_buffer_address_and_length;
   I0 = r0;
   if Z jump next_channel;
   L0 = r1;
   push r2;
   pop B0;

   // Setup Output Buffer
   r0 = r5 + r9;
   r0 = M[r8 + r0];     // output index
   call $cbops.get_buffer_address_and_length;
   I4 = r0;
   if Z jump next_channel;
   L4 = r1;
   push r2;
   pop B4;

   // Get the current dc estimate for this channel, with the knowledge that we have
   // a 2-word param for each channel that are right after the header part.
   // ($cbops.dc_remove.DC_ESTIMATE_FIELD)
   // r3:r5 -> DC estimate for this channel
   // NOTE: it is actually -dc_estimate
   r3 = M[I2, MK1];     // r3 = current dc estimate, MSW
   r5 = M[I2, -MK1];    // r5 = current dc estimate, LSW

   r1 = $cbops.dc_remove.FILTER_COEF;

   // used as -1.0 below
   r6 = MININT;

   // r2 = 1.0 - coeff
   // Note 0x800000 represents +1.0 so long as you subtract something from it
   // i.e. this works as long as $cbops.dc_remove.FILTER_COEF is not 0
   r2 = r6 - r1;

   // Samples to process
   r10 = M3;

   // new_dc_est = old_dc_est * (1.0 - filter_coeff) + current_sample * coeff
   // use double precision for first multiply (DOUBLE_P * SINGLE_P -> DOUBLE_P)
   // new_dc_est:-(r3:r5), (1.0 - filter_coeff):r2, coeff:r1
   rMAC = r2 * r5 (SU);
   do loop;
      // only DOUBLE_P result, so we don't need LSW
      rMAC = rMAC ASHIFT -DAWTH(56bit);
      // rMAC =  old_dc_est * (1 - filter_coeff)
      rMAC = rMAC + r2 * r3 (SS), r0 = M[I0,MK1];
      // rMAC =  old_dc_est * (1 - filter_coeff) + current_sample * filter_coeff
      rMAC = rMAC - r0*r1;
      // r3:r5 = -new_dc_offset
      r3 = rMAC1;
      r5 = rMAC0;
      // output = input - new_dc_offset
      rMAC = rMAC - r0*r6;
      rMAC = r2 * r5 (SU), M[I4, MK1] = rMAC;
   loop:
   M[I2, MK1] = r3;     // store dc estimate, MSW
   M[I2, -MK1] = r5;    // store dc estimate, LSW

 next_channel:

   // we move to next channel. In the case of this cbop, it is enough to
   // count based on input channels here.
   // both current and total channel number is in ADD_PER_WORDs so just compare them
   I2 = I2 + ($cbops.dc_remove.STRUC_SIZE*ADDR_PER_WORD);
   r7 = r7 + 1*ADDR_PER_WORD;
   Null = r7 - r9;
   if LT jump process_channel;

   // zero the length registers
   L0 = 0;
   L4 = 0;
   // Zero the base registers
   push Null;
   B4 = M[SP - 1*ADDR_PER_WORD];
   pop B0;

jp_done:

   // stop profiling if enabled
   #ifdef ENABLE_PROFILER_MACROS
      r0 = &$cbops.profile_dc_remove;
      call $profiler.stop;
   #endif

   pop rlink;
   rts;


.ENDMODULE;
