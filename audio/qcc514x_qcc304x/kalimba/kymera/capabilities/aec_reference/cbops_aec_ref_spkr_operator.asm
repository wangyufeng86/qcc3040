// *****************************************************************************
// Copyright (c) 2007 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// $Change: 1141152 $  $DateTime: 2011/11/02 20:31:09 $
// *****************************************************************************

// *****************************************************************************
// NAME:
//    Buffer Insert operator
//
// DESCRIPTION:
//    This operator keeps an cbops chain fed.
//
//    If the input is below the threshold, then insert data
//
// *****************************************************************************

#include "stack.h"
#include "cbops/cbops.h"
#include "cbuffer_asm.h"
#include "cbops_aec_ref_spkr_op_asm_defs.h"

#include "patch/patch_asm_macros.h"

.MODULE $M.cbops.aec_ref_spkr_op;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   // ** function vector **
   .VAR $cbops.aec_ref_spkr_op[$cbops.function_vector.STRUC_SIZE] =
      &$cbops.aec_ref_spkr_op.pre_main,            // pre-main function
      &$cbops.aec_ref_spkr_op.amount_to_use,       // amount to use function
      &$cbops.aec_ref_spkr_op.post_main;           // post-main function

// Expose the location of this table to C
.set $_cbops_aec_ref_spkr_table , $cbops.aec_ref_spkr_op

// *****************************************************************************
// MODULE:
//   $cbops.insert_op.amount_to_use
//
// DESCRIPTION:
//   Get the amount to use, across all channels (it acts "in sync").
//
// INPUTS:
//    - r4 = buffer table
//    - r8 = pointer to operator structure
//
// OUTPUTS:
//
// TRASHED REGISTERS:
//    r0-r3, r5, r7
//
// *****************************************************************************

// Called before amount_to_use of graph
$cbops.aec_ref_spkr_op.amount_to_use:

    LIBS_SLOW_SW_ROM_PATCH_POINT($cbops.aec_ref_spkr_op.amount_to_use.PATCH_ID_0, r7)

   push rlink;

   // Get data pointer
   r7 = M[r8 + $cbops.param_hdr.OPERATOR_DATA_PTR_FIELD];
   // Get amount of input
   r0 = M[r8 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD];
   call $cbops.get_amount_ptr;
   r5 = M[r0];
   // Save data at input and make input large, will re-adjust for insertion
   M[r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.AMOUNT_DATA_FIELD] = r5;
   r5 = 0x7FFF;
   M[r0] = r5;

   // Get first output index
   r9 = M[r8 + $cbops.param_hdr.NR_INPUT_CHANNELS_FIELD];
   Words2Addr(r9);
   r9 = r9 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;
   r0 = M[r8 + r9];
   call $cbops.get_cbuffer;
   NULL = r0;
   if Z jump aec_ref_spkr_op.amount_to_use_done;

   // First Port (r0);
   r5 = NULL;
   r6 = M[r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.MAX_ADVANCE_FIELD];
   r1 = M[r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.MAX_JITTER_FIELD];
   r6 = r6 + r1;
   call calc_dac_amount_of_data;

   // r2 is data in port (negative if wrap), r10 is max advance, r0 is adjustment
   // Limit amount of data after transfer to two times maximum advance
   NULL = r2 + r0;
   if POS r1 = r6 - r2;
   if NEG r1 = Null;

   // Setup the limited transfer at output
   r0 = M[r8 + r9];
   call $cbops.get_amount_ptr;
   M[r0]=r1;
   if Z call $cbops.force_processing;

aec_ref_spkr_op.amount_to_use_done:
   pop rlink;
   rts;

// Called after amount to use of graph but before main processing of graph
$cbops.aec_ref_spkr_op.pre_main:
   push rLink;

   LIBS_SLOW_SW_ROM_PATCH_POINT($cbops.aec_ref_spkr_op.pre_main.PATCH_ID_0, r6)
   /* -----------------------------------------------------------------------
    * pre main function will transfer audio from input to intermediate stage
    * and it will also insert silence into the intermediate stage so cbops
    * will not write samples into input buffer.
    *
    * INPUT_BUFS ------->INTERM_BUFFS---> Rest of the cbops chain
    *                       ^
    *                       |
    *                       |
    *                Possible Silence Insert
    *-----------------------------------------------------------------------*/
   // Get data pointer
   r7 = M[r8 + $cbops.param_hdr.OPERATOR_DATA_PTR_FIELD];

   // Get intermediate buffer indexes
   I7 = r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.INTERM_IDXS_FIELD;

   // Get input buffer indexes
   I2 = r8 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;

   // Get number of input channels
   r0 = M[r8 + $cbops.param_hdr.NR_INPUT_CHANNELS_FIELD];
   M2 = r0;

   // r5 will hold the amount of silence needed
   // to insert this run
   r5 = 0;

   // transfer amount from input
   r0 = M[I2, 0];
   call $cbops.get_amount_ptr;
   r10 = r0;

   // transfer amount to interm stage
   r0 = M[I7, 0];
   call $cbops.get_amount_ptr;
   r6 = M[r0];

   // optimisation, early exit if nothing can transfer
   // to interm stage.
   if Z M[r10] = r6;
   if Z jump aec_ref_spkr_op.pre_main_done;

   // Get amount of data and threshold. Determine if need to insert.
   r3 = M[r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.AMOUNT_DATA_FIELD];
   NULL = r6 - r3;
   if LE jump aec_ref_spkr_op.pre_main_transfer;

   // Insufficient data, limit transfer to threshold.
   r2 = M[r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.IN_THRESHOLD_FIELD];
   r6 = MIN r2;
   // Now:
   // r3: amount of data in the input buffer
   // r6: amount of data that "must" be pushed into next stage
   //  if input has enough data then transfer needed samples from
   //  input buffer otherwise silence will be inserted to make sure
   //  exactly r6 samples will be pushed into next stage.
   r5 = r6 - r3;
   if LE r5 = 0;
   if Z jump aec_ref_spkr_op.pre_main_transfer;
   // Insert (r5) zeroes with this transfer, update counter
   r1 = M[r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.NUM_INSERTS_FIELD];
   r1 = r1 + r5;
   M[r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.NUM_INSERTS_FIELD]=r1;

aec_ref_spkr_op.pre_main_transfer:
   // transfer from input buffer to intermediate buffers
   // and also insert silence if needed, at this point:
   // At this point:
   //  r6: total samples that will be written
   //      to intermediate buffers
   //  r5: of r6 this amount will be silence,
   //      the rest are real audio from input
   //  I2: holds array of input buffers indexes
   //  I7: holds array of intermediate buffers indexes
   //  M2: number of input channels
   //  r7: data pointer of the operator
   //  r0: transfer amount ptr for intermediate buffer
   //  r10: transfer amount ptr for input buffer

   // update transfer amount for interm bufs
   M[r0] = r6;

   // amount that should be transferred from input
   r6 = r6 - r5;

   // update transfer amount for input bufs
   M[r10] = r6;

   // r5: amount of silence to insert
   // r6: amount of transfer from input buffer
   transfer_and_silence_insert_channel:
      // Setup Input Buffer
      r0 = M[I2,0];
      call $cbops.get_buffer_address_and_length;
      I0 = r0;
      if Z jump next_transfer_and_silence_insert_channel;
      L0 = r1;
      push r2;
      pop B0;

      // Setup Interm Buffer
      r0 = M[I7,0];
      call $cbops.get_buffer_address_and_length;
      I4 = r0;
      if Z jump next_transfer_and_silence_insert_channel;
      L4 = r1;
      push r2;
      pop B4;

      // first silence insertion if needed
      r10 = r5;
      r0 = 0;
      do silence_insert_loop;
         M[I4, MK1] = r0;
      silence_insert_loop:

      // then transfer real audio from input
      r10 = r6;
      do transfer_loop;
         r0 = M[I0, MK1];
         M[I4, MK1] = r0;
      transfer_loop:

      next_transfer_and_silence_insert_channel:
      r0 = M[I2, MK1], r1 = M[I7, MK1];
      M2 = M2 - 1;
   if GT jump transfer_and_silence_insert_channel;

   // Reset Buffering control
   L0 = 0;
   L4 = 0;
   push NULL;
   pop B0;
   push NULL;
   pop B4;

   /* Something written at the interim stage
    * force going ahead
    */
   call $cbops.force_processing;

aec_ref_spkr_op.pre_main_done:
   pop rlink;
   rts;

// unsigned get_aec_ref_cbops_inserts_total(cbops_op *op);
$_get_aec_ref_cbops_inserts_total:
    r0 = M[r0 + ($cbops_c.cbops_op_struct.PARAMETER_AREA_START_FIELD+$cbops.param_hdr.OPERATOR_DATA_PTR_FIELD)];
    r1 = M[r0 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.WRAP_COUNT_FIELD];
    r0 = M[r0 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.NUM_INSERTS_FIELD];
    r0 = r0 + r1;
    rts;

// unsigned get_aec_ref_cbops_insert_op_insert_total(cbops_op *op);
$_get_aec_ref_cbops_insert_op_insert_total:
    r0 = M[r0 + ($cbops_c.cbops_op_struct.PARAMETER_AREA_START_FIELD+$cbops.param_hdr.OPERATOR_DATA_PTR_FIELD)];
    r0 = M[r0 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.NUM_INSERTS_FIELD];
    rts;

// unsigned get_aec_ref_cbops_wrap_op_insert_total(cbops_op *op);
$_get_aec_ref_cbops_wrap_op_insert_total:
    r0 = M[r0 + ($cbops_c.cbops_op_struct.PARAMETER_AREA_START_FIELD+$cbops.param_hdr.OPERATOR_DATA_PTR_FIELD)];
    r0 = M[r0 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.WRAP_COUNT_FIELD];
    rts;

// Called after main processing of graph before buffer update
$cbops.aec_ref_spkr_op.post_main:

   // Check for Buffer Wrapping
    push rLink;

    LIBS_SLOW_SW_ROM_PATCH_POINT($cbops.aec_ref_spkr_op.post_main.PATCH_ID_0, r9)

    // Get first output index
    r9 = M[r8 + $cbops.param_hdr.NR_INPUT_CHANNELS_FIELD];
    Words2Addr(r9);
    r9 = r9 + r8;
    I4 = r9 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;

    // Get first buffer entry
    r0 = M[I4,0];
    r3 = r0 * $CBOP_BUFTAB_ENTRY_SIZE_IN_ADDR (int);
    r3 = r3 + r4;

    // Get Cbuffer Ptr
    r0 = M[r3 + $cbops_c.cbops_buffer_struct.BUFFER_FIELD];
    if Z jump $pop_rLink_and_rts;

#if !defined(CHIP_BASE_BC7)
    // Get transfer amount (r5)
    r1 = M[r3 + $cbops_c.cbops_buffer_struct.TRANSFER_PTR_FIELD];
    r5 = M[r1];
#else
    r5 = NULL;
#endif

    // Compute amount of data in source
    r7 = M[r8 + $cbops.param_hdr.OPERATOR_DATA_PTR_FIELD];
    r6 = M[r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.MAX_JITTER_FIELD];
    call calc_dac_amount_of_data;

    r6 = M[r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.MAX_ADVANCE_FIELD];

    // r2 is number of samples in port, negative if overflow
    r10 = r6 - r2;
    if LE jump $pop_rLink_and_rts;

    // Increment Wrap count
    r1 = M[r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.WRAP_COUNT_FIELD];
    r1 = r1 + r10;
    M[r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.WRAP_COUNT_FIELD]=r1;

    // setup amounts for insertions
    r5 = r5 + r10;
    r6 = r10;

    // Number of Ports
    r9 = M[r8 + $cbops.param_hdr.NR_OUTPUT_CHANNELS_FIELD];

    // Perform insertion
 process_channel:
    r0 = M[I4,0];
    call $cbops.get_buffer_address_and_length;
    I0 = r0;
    if Z jump process_channel_next;
      // Insert r6 zeros
      r10 = r6;
      L0 = r1;
      push r2;
      pop B0;
      r1  = Null;
      do lp_insert_loop;
         M[I0, MK1] = r1;
      lp_insert_loop:
      // Update amount (r5)
      r0 = M[I4,MK1];
      call $cbops.get_amount_ptr;
      M[r0]=r5;
    process_channel_next:
    r9 = r9 - 1;
    if GT jump process_channel;

    // Clear circular buffer
    L0=NULL;
    push NULL;
    pop B0;

    pop rlink;
    rts;


// *****************************************************************************
// MODULE:
//   calc_dac_amount_of_data
//
// DESCRIPTION:
//   Calculate amount of data in port
//
//
// INPUTS:
//    - r0 = cbuffer structure
//    - r5 = amount transferred
//    - r6 = minimum space that must be available in the buffer now
//
// OUTPUTS:
//    - r2 = data words in port (size-space)
//      negative if a wrap occurred.  Assumes that advance is
//      always less than half the buffer
//
// TRASHED REGISTERS:
//    r0,r1,r2
//
// *****************************************************************************
calc_dac_amount_of_data:
    push rLink;
    call $cbuffer.calc_amount_space_in_words;

#if defined(CHIP_BASE_HYDRA)
    // Hydra: r2 is local buffer size in addr
    Addr2Words(r2);
    // Adjust space for amount written, buffer not port
    r0 = r0 - r5;
#else
    // BlueCore: r2 is MMU buffer size in bytes
    r2  = r2 LSHIFT -1;
#endif

    // r0 is space in port minus one
    // r2 = amount data in port
    // r6 min required space
    r2  = r2 - r0;

    // check minimum space, if less than that
    // wrap has happened
    Null = r0 - r6;
    if NEG r2 = NULL - r0;

    r0 = M[r7 + $cbops_aec_ref_spkr_op.aec_ref_op_struct.BUFFER_ADJ_FIELD];
    r2 = r2 - r0;
    // r2 is number of samples in port, negative if overflow
    pop rLink;
    rts;

.ENDMODULE;
