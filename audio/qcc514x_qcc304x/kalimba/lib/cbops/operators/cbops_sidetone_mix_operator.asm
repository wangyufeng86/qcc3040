// *****************************************************************************
// Copyright (c) 2007 - 2020 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

// *****************************************************************************
// NAME:
//    Frame Sync Sidetone mix operator
//
// DESCRIPTION:
//    In a headset type application it is often desirable to feed some of the
// received microphone signal back out on the speaker. This gives the user some
// indication the headset is on. This operator mixes data from a supplied buffer
// in with the "main" data stream.
//
// @verbatim
//
//   Input Channel---------------->+------------------> output channel
//                                 |
//                                 |
//                                 |
//                                 |
//                                 |
//                             SIDETONE CHANNEL
// @endverbatim
//
//
//    The number of samples in the sidetone buffer is monitored at this point.
// If there are too few, extra samples are created by repeating the last sample
// from the buffer. If too many samples are present, samples will be
// discarded during each call.
//
// This is a multi-channel and multi-sidetone operator, i.e. it accepts multiple
// sidetone channel and individual input channels can choose a specific sidetone
// channel to mix. Example:
//                       _____________
//                      |             |
// Input channel 0 -----|--+----------|-----> Output channel 0 (Input channel 0 + SIDETONE Channel 0)
//                      |  |          |
// Input channel 1 -----|------+------|-----> Output channel 1 (Input channel 0 + SIDETONE Channel 1)
//                      |  |   |      |
// Input channel 2 -----|--+----------|-----> Output channel 2 (Input channel 0 + SIDETONE Channel 0)
//                      |  |   |      |
// Input channel 3 -----|-------------|-----> Output channel 3 (Input channel 3, no sidetone mix)
//                      |__|___|______|
//                         |   |
//                         |   |
//                         0   1
//                        SIDETONE
//                        Channels
//
//
// NOTES:
//   - This operator doesn't apply any gain to SIDETONE channels.
//   - Each main channel can mix into maximum one sidetone channel.
//   - A sidetone channel can mix into multiple main channel.
//
// *****************************************************************************

#include "stack.h"
#include "cbops.h"
#include "cbuffer_asm.h"
#include "cbops_sidetone_mix_op_asm_defs.h"


// Private Library Exports
.PUBLIC $cbops.sidetone_mix_op;

.MODULE $M.cbops.sidetone_mix_op;
   .DATASEGMENT DM;

   // ** function vector **
   .VAR $cbops.multichan_sidetone_mix_op[$cbops.function_vector.STRUC_SIZE] =
      // reset function
      $cbops.function_vector.NO_FUNCTION,
      // amount to use function
      &$cbops.basic_multichan_amount_to_use,
      // main function
      &$cbops.multichan_sidetone_mix_op.main;
.ENDMODULE;

.MODULE $M.cbops.sidetone_mix_op;
   .CODESEGMENT CBOPS_SIDETONE_MIX_OPERATOR_PM;
   .DATASEGMENT DM;

// *****************************************************************************
// MODULE:
//    $cbops.sidetone_mix_op.main
//
// DESCRIPTION:
//    Operator that copies the output/input word and reads/writes the samples
//    to/from the sidetone buffer
//
// INPUTS:
//    - r4 = pointer to list of buffers
//    - r8 = pointer to cbops object
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    r0-r3, r5, r6, r7, r9, r10, rMAC, M1-M3, I0-I7, L0, L1, L4, DoLoop
//
// *****************************************************************************
$cbops.multichan_sidetone_mix_op.main:
   // push rLink onto stack
   push rLink;

   r7 = M[r8 + $cbops.param_hdr.OPERATOR_DATA_PTR_FIELD];

   // used throughout this function
   M1 = MK1;

   // Go through all sidetone buffers and and find minimum
   // amount of data among them.
   // Note: We could just look at first buffer since we expect
   // all sidetone buffers to be synchronised but this will
   // prevent failing in dramatic way if they aren't synced.
   r6 = 0x7FFF; // start with reasonably large value
   r10 = M[r7 + $cbops_sidetone_mix_op.multichan_st_mix_struct.NR_ST_CHANNELS_FIELD];
   I6 = r7 + $cbops_sidetone_mix_op.multichan_st_mix_struct.ST_IDXS_FIELD;
   // r6: Minimum amount of data in sidetone buffers
   do sidetone_buffers_get_amount_loop;
      // Get buffer index for sidetone buffer
      r0 = M[I6, M1];

      // get sidetone buffer
      call $cbops.get_cbuffer;

      // get the amount of sidetone data
      call $cbuffer.calc_amount_data_in_words;

      // r6 holds minimum data in all connected sidetone channels
      r6  = MIN r0;
  sidetone_buffers_get_amount_loop:

   // Get transfer amount from input to output
   //  r0: transfer amount in samples
   //  r9: offset to output channels in addrs (from the start of index table)
   call $cbops.get_transfer_and_update_multi_channel;

   // M3: amount of sidetone to invent this run
   //     if sidetone buffers don't have enough data.
   M3 = r0 - r6;
   if NEG M3 = 0;

   // update insert counter
   r1 = M[r7 + $cbops_sidetone_mix_op.multichan_st_mix_struct.NUM_INSERTS_FIELD];
   r1 = r1 + M3;
   M[r7 +  $cbops_sidetone_mix_op.multichan_st_mix_struct.NUM_INSERTS_FIELD]=r1;

   // M2: Amount of real sidetone that will be mixed this run
   M2 = r0 - M3;

   // If amount of sidetone after transfer is > 2*SIDETONE_MAX_SAMPLES_FIELD
   // drop sidetone to leave SIDETONE_MAX_SAMPLES_FIELD samples
   // r5 is amount to drop
   r1 = M[r7 + $cbops_sidetone_mix_op.multichan_st_mix_struct.MAX_SAMPLES_FIELD];
   r0 = r6 - r0;    //Amount remaining sidetone samples in the buffers after mix
   r5 = r0 - r1;    // remaining sidetone samples - SIDETONE_MAX_SAMPLES_FIELD
   NULL = r5 - r1;
   if NEG r5 = NULL; // remaining data < 2*SIDETONE_MAX_SAMPLES_FIELD (no drop)

   // r5 sidetone samples are dropped to control latency, update the accumulator
   r2 = M[r7 + $cbops_sidetone_mix_op.multichan_st_mix_struct.NUM_DROPS_FIELD];
   r2 = r2 + r5;
   M[r7 + $cbops_sidetone_mix_op.multichan_st_mix_struct.NUM_DROPS_FIELD]=r2;

   // Total sidetone samples consumed =
   //   M2: mixed sidetone samples +
   //   r5: dropped sidetone samples
   r5 = M2 + r5;

   // Go through all sidetone buffers and
   // update transfer amount (=r5)
   r10 = M[r7 + $cbops_sidetone_mix_op.multichan_st_mix_struct.NR_ST_CHANNELS_FIELD];
   I6 = r7 + $cbops_sidetone_mix_op.multichan_st_mix_struct.ST_IDXS_FIELD;
   do sidetone_buffers_update_transfer_amount_loop;
      r0 = M[I6, M1];
      call $cbops.get_amount_ptr;
      M[r0] = r5;
   sidetone_buffers_update_transfer_amount_loop:

   // see if we have something to transfer
   // from input to output
   NULL = M2 + M3;
   if LE jump all_done;

   // set M0 to number of channels
   r0 = M[r7 + $cbops_sidetone_mix_op.multichan_st_mix_struct.NR_CHANNELS_FIELD];
   M0 = r0;

   // set I6 to sidetone map indexes
   r0 = M[r7 + $cbops_sidetone_mix_op.multichan_st_mix_struct.NR_ST_CHANNELS_FIELD];
   Words2Addr(r0);
   I6 = r7 +  $cbops_sidetone_mix_op.multichan_st_mix_struct.ST_IDXS_FIELD;
   I6 = I6 + r0;

   // set I2 to input indexes
   I2 = r8 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;

   // set I3 to output indexes
   I3 = I2 + r9;

   // Sidetone attenuation (0dB)
   r5 = 1.0;

   transfer_and_sidetone_mix_channel_loop:
      // get input, output and sidetone indexes
      // for this channel
      r0 = M[I6, M1];          // get sidetone index
      r7 = r0, r0 = M[I3, M1]; // get output index
      r3 = r0, r0 = M[I2, M1]; // get input index
      I7 = r3 - r0;
      // r7 = sidetone index
      // r3 = output index
      // r0 = input index
      // I7 = output index - input index (useful to skip transfer only)

      // Setup Input Buffer
      call $cbops.get_buffer_address_and_length;
      I0 = r0;
      if Z jump transfer_and_sidetone_mix_next_channel;
      L0 = r1;
      push r2;
      pop B0;

      // Setup Output Buffer
      r0 = r3;
      call $cbops.get_buffer_address_and_length;
      I4 = r0;
      if Z jump transfer_and_sidetone_mix_next_channel;
      L4 = r1;
      push r2;
      pop B4;

      // setup sidetone buffer
      r0 = r7;
      // if not mapped to any sidetone buffer just
      // simple transfer from input to output
      if NEG jump just_transfer_input_to_output;
      call $cbops.get_buffer_address_and_length;
      push r2;
      pop B1;
      // set the length & get the first input sample
      I1 = r0;
      L1 = r1, rMAC = M[I0,M1];

      // M2: amount of sidetone to mix
      // r6  is the amount of data in the sidetone buffer
      // M3  is number of invented sidetone samples to mix
      // I1/L1/B1 = Sidetone buffer
      // I0/L0/B0 = Input buffer
      // I4/L4/B4 = Output buffer

      // Mixing loop
      r10 = M2;
      do mix_real_sidetone_loop;
         // calculate the current output sample and read a side tone sample
         r0 = M[I1,M1];
         rMAC = rMAC + r0 * r5;

         // get the next input value and write current output
         rMAC = M[I0,M1], M[I4,M1] = rMAC;
      mix_real_sidetone_loop:

      // continue only of we need to invent sidetone samples
      r10 = M3;
      if Z jump transfer_and_sidetone_mix_next_channel;
      // Repeat Last sample of sidetone buffer (insert sidetone samples)
      r0 = M[I1,-MK1];
      r0 = M[I1,M1];
      // Ensure sidetone is connected repeat last sample, else mix silence
      NULL = r6;
      if Z r0=NULL;
      do mix_invented_sidetone_loop;
         // calculate the current output sample
         rMAC = rMAC + r0 * r5;
         // get the next input value and write the result
         rMAC = M[I0,M1], M[I4,M1] = rMAC;
      mix_invented_sidetone_loop:
      jump transfer_and_sidetone_mix_next_channel;

      just_transfer_input_to_output:
      NULL = I7; // if output index == input index then skip transfer
      if Z jump transfer_and_sidetone_mix_next_channel;
      r10 = M2 + M3;
      do just_transfer_loop;
         r0 = M[I0,M1];
         M[I4,M1] = r0;
      just_transfer_loop:
   transfer_and_sidetone_mix_next_channel:
   M0 = M0 - 1;
   if NZ jump transfer_and_sidetone_mix_channel_loop;

   // zero the remaining length registers we have used
   L0 = 0;
   L4 = 0;
   L1 = 0;
   push Null;
   B0 = M[SP - 1*ADDR_PER_WORD];
   B1 = M[SP - 1*ADDR_PER_WORD];
   pop B4;
all_done:
   // pop rLink from stack
   pop rLink;
   rts;

.ENDMODULE;

// Expose the location of this table to C
.set $_cbops_multichan_sidetone_mix_table,  $cbops.multichan_sidetone_mix_op
