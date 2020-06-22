/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
#include "stack.h"          // for PUSH_ALL_C macros
#include "codec_library.h"  // for ENCODER_DATA_OBJECT_FIELD
#include "swbs_struct_asm_defs.h"
#include "sco_struct_asm_defs.h"
#include "opmgr_for_ops_asm_defs.h"

#include "patch_library.h"

#ifdef ADAPTIVE_R2_BUILD
#include "aptXAdaptive_asm_r2.h"
#else
#include "aptXAdaptive_asm.h"
#endif

#define SWBS_CODEC_MODE0 0
#define SWBS_CODEC_MODE4 4

#define SWBS_MODE0_AUDIO_BLOCK_SIZE 240
#define SWBS_MODE4_AUDIO_BLOCK_SIZE 180

#define SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES  60
#define SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES_WITH_BIT_ERROR 122
.MODULE $M.swbs_enc_c_stubs;
    .CODESEGMENT PM;

// void swbsenc_init_encoder(OPERATOR_DATA *op_data)
$_swbsenc_init_encoder:
    pushm <r7, r8, r9, rLink>;

    r7 = r0;

    // set up the aptX adaptive library to point to the correct data
    r0 = r7;
    call $base_op.base_op_get_instance_data;
    r9 = M[r0 + $swbs_struct.SWBS_ENC_OP_DATA_struct.CODEC_DATA_FIELD];

    popm <r7, r8, r9, rLink>;
    rts;


// void swbsenc_process_frame( OPERATOR_DATA *op_data)
$_swbsenc_process_frame:
    PUSH_ALL_C

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($swbs_cap.SWBS_C_STUBS_ASM.SWBS_ENC_C_STUBS._SWBSENC_PROCESS_FRAME.PATCH_ID_0,r9)

    // set up the aptX adaptive library to point to the correct data
    push r0;
    call $base_op.base_op_get_instance_data;
    r7 = r0;
    pop r0;
    r9 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.CODEC_DATA_FIELD];

   // get the Input buffer
   r0 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.BUFFERS_FIELD + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.IP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_read_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = read address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;
   
   // Copy data from system input buffer to codec input axbuf.
   r2 = r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.AXINBUF_FIELD;  // flattened
   I1 = M[r2 + $AXBUF_WPTR_OFFSET];
   
   r10 = SWBS_MODE0_AUDIO_BLOCK_SIZE;
   r6 = SWBS_MODE4_AUDIO_BLOCK_SIZE;
   r1 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.CODECMODE_FIELD];
   Null = r1 - SWBS_CODEC_MODE4;
   if EQ r10 = r6;
   
   r4 = -8;                           // align input data to 24-bit for aptX adaptive
   do input_copy;                     // copy from circular buffer to linear axbuffer
   r0 = M[I0,M1];
   r0 = r0 ASHIFT r4;
   M[I1,M1] = r0;
   input_copy:
   
   M[r2 + $AXBUF_WPTR_OFFSET] = I1;    // set input buffer write pointer
   
   r0 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.BUFFERS_FIELD + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.IP_BUFFER_FIELD];
   r1 = I0;
   call $cbuffer.set_read_address;

   // Reset circular buffer registers, codec uses linear axbuf
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;
	
   r2 = r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.AXOUTBUF_FIELD;  // flattened
   I1 = M[r2 + $AXBUF_DAT_OFFSET];   
   M[r2 + $AXBUF_RPTR_OFFSET] = I1;   // set read pointer to be base address of data
   M[r2 + $AXBUF_WPTR_OFFSET] = I1;   // set write pointer to be base address of data

   r0 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.CODEC_DATA_FIELD];
   r1 = r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.AXINBUF_FIELD;
   r2 = r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.AXOUTBUF_FIELD;

   
	// aptX Adaptive encode
   call $aptx_adaptive.voice_encode;    /// wrapper around adaptive codec
   
   
      // get the Output buffer
   r0 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.BUFFERS_FIELD + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_write_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = write address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;   
   
   r2 = r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.AXOUTBUF_FIELD;
   I1 = M[r2 + $AXBUF_DAT_OFFSET];  
   
   r10 = 15;                              // voice encoder will produce 60 bytes (32kHz mono)   
   Null = r1 - r10;
   if LT jump no_space;
   do output_copy;                        // copy and shift from 32-bit linear buffer to 16-bit circular buffer
   r0 = M[I1,M1];
   r1 = r0 LSHIFT -16;                    // aptX adaptive produces 32-bit codewords
   r0 = r0 AND 0xFFFF;                    // Shift and mask to get two 16-bit codewords
   M[I0,M1] = r1;
   M[I0,M1] = r0;
   output_copy:

   r0 = M[r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.BUFFERS_FIELD + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];
   r1 = I0;
   
   call $cbuffer.set_write_address;

   // In Mode 4 the codec reads 176/176/176/192 samples so there might be some data left in the input buffer.
   // Do a consume to move the spare data to the top of the buffer for the next call.

   r0 = r7 + $swbs_struct.SWBS_ENC_OP_DATA_struct.AXINBUF_FIELD;
   call $_AxDataConsumeASM;

   // Reset circular buffer registers
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;
   
no_space:
   // do something

   POP_ALL_C
   rts;


// void swbs_enc_reset_aptx_data(OPERATOR_DATA *op_data)
$_swbs_enc_reset_aptx_data:
    pushm <r4, r5, r9, rLink>;

//   LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($swbs_cap.SWBS_C_STUBS_ASM.SWBS_ENC_C_STUBS._SWBS_ENC_RESET_SBC_DATA.PATCH_ID_0)

    push I0;

    // Insert code to reset aptX data if required

    pop I0;
    popm <r4, r5, r9, rLink>;
    rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $_swbsdec_init_dec_param
//
// DESCRIPTION:
//    Function for initialising the swbs decoder parameters.
//
// INPUTS:
//    - r0 = pointer to operator data
//
// OUTPUTS:
//
// TRASHED REGISTERS:
//    r4, r9
// NOTES:
//
// *****************************************************************************
.MODULE $M.swbs_dec._swbsdec_init_dec_param;
   .CODESEGMENT PM;

$_swbsdec_init_dec_param:

   pushm <r4, r9, rLink>;

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($swbs_cap.SWBS_C_STUBS_ASM.SWBS_DEC._SWBSDEC_INIT_DEC_PARAM._SWBSDEC_INIT_DEC_PARAM.PATCH_ID_0,r4)

   push r0;
   call $base_op.base_op_get_instance_data;
   r4 = r0;
   pop r0;
   r9 = M[r4 + $wbs_struct.WBS_DEC_OP_DATA_struct.CODEC_DATA_FIELD];

   popm <r4, r9, rLink>;
   rts;

.ENDMODULE;

// r0 = op_data
// r1 = packet_size

.MODULE $M.swbs_run_plc;

   .DATASEGMENT DM;
   .CODESEGMENT PM;

$_swbs_run_plc:

   PUSH_ALL_C

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($swbs_cap.SWBS_C_STUBS_ASM.SWBS_DEC._SWBS_RUN_PLC._SWBS_RUN_PLC.PATCH_ID_0,r5)
   r5 = r1;  // Store packet_size for later
   
   call $base_op.base_op_get_instance_data;
   r4 = r0;
   r1 = r4 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;  // flattened
     
   // r0 = pDecState
   // r1 = audioOut
   // r2 = packet_size
   r0 = M[r4 + $swbs_struct.SWBS_DEC_OP_DATA_struct.CODEC_DATA_FIELD];
   r2 = r5;
   call $aptx_adaptive.run_plc;
   
   // Copy data from codec output axbuf into system output buffer
   // Get the Output buffer
   r0 = M[r4 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_write_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = write address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   // Set up circular buffer
   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;

   r2 = r4 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;
   I1 = M[r2 + $AXBUF_DAT_OFFSET];  
   
   r10 = r5;                             // Get packet_size from above

   Null = r1 - r10;
   if LT jump no_space;
   do output_copy;                       // copy and shift from linear buffer to circular buffer
   r0 = M[I1,M1];
   r0 = r0 ASHIFT 8;                     // aptX adaptive produces 24-bit aligned audio (in 32-bit word)
   M[I0,M1] = r0;
   output_copy:

   M[r2 + $AXBUF_WPTR_OFFSET ] = Null;   // Reset write pointer

   r0 = M[r4 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];
   r1 = I0;

   call $cbuffer.set_write_address;
 
no_space:
// do something

   // Reset circular buffer registers
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;
   
   POP_ALL_C
   rts;

.ENDMODULE;  
   


// *****************************************************************************
// FUNCTION:
//    $sco_decoder.swbs.process:
//
// DESCRIPTION:
//    Decoding SCO SWBS packets into DAC audio samples.
//
//    The SCO c-buffer contains SWBS packet words to be decoded into DAC
//    audio samples. Refer to the function description of Frame_encode for
//    SWBS packet word definition.  DAC audio could be mono-channel(left
//    only) or stereo-channels(both left and right).
//
// INPUTS:
// r0 swbs_dec.CODEC_DATA_FIELD
// r1 IP_BUFFER pointer
// r2 PLC data
// r3 pointer to the stream PACKET
// OUTPUTS:
// r0 Output packet size

// TRASHED REGISTERS:
//    Assumes everything
//
// CPU USAGE:
//    D-MEMORY: xxx
//    P-MEMORY: xxx
//    CYCLES:   xxx
//
// NOTES:
// *****************************************************************************

.MODULE $M.sco_decoder.swbs._process;

   .DATASEGMENT DM;
   .CODESEGMENT PM;

$_sco_decoder_swbs_process:
// r0 op data
// r1 pointer to the stream PACKET
// r2 swbs_packet_length

   PUSH_ALL_C

   // save OP DATA
   r7 = r0;
   // save pointer to the packet
   r9 = r1;


   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($swbs_cap.SWBS_C_STUBS_ASM.SCO_DECODER.SWBS._PROCESS._SCO_DECODER_SWBS_PROCESS.PATCH_ID_0,r5)

   // save the swbs packet size
   r5 = r2;


   // get extra op data
   call $base_op.base_op_get_instance_data;
   r3 = r0;

   // get the Input buffer
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.IP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_read_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = read address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;

   // Copy data from system input buffer to codec input axbuf.
   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXINBUF_FIELD;  // flattened
   I1 = M[r2 + $AXBUF_DAT_OFFSET];   
   M[r2 + $AXBUF_RPTR_OFFSET] = 0;    // set relative read pointer to be 0
   I2 = I1;                           // store base address
   r0 = M[r2 + $AXBUF_WPTR_OFFSET];
   I1 = I1 + r0;

   Null = r5 - SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES;
   if NE jump not_60_byte_packets;
   r10 = 15;                          // Copy 60 bytes
   r4 = 16;                           // Data is packed in 16 bit words
   do input_copy;                     // copy from circular buffer to linear buffer
   r0 = M[I0,M1];
   r0 = r0 LSHIFT r4,
   r1 = M[I0,M1];
   r1 = r1 AND 0xFFFF;                // Mask upper 16-bits off in case there is stale data
   r0 = r0 OR r1;
   M[I1,M1] = r0;
   input_copy:
jump input_processed;
not_60_byte_packets:
   Null = r0;                         // M[r2 + $AXBUF_WPTR_OFFSET];
   if NZ jump second_packet;
   first_packet:
   r10 = 7;                           // Copy 28 bytes
   r4 = 16;                           // Data is packed in 16 bit words
   do input_copy_first;              // copy from circular buffer to linear buffer
   r0 = M[I0,M1];
   r0 = r0 LSHIFT r4,
   r1 = M[I0,M1];
   r1 = r1 AND 0xFFFF;                // Mask upper 16-bits off in case there is stale data
   r0 = r0 OR r1;
   M[I1,M1] = r0;
   input_copy_first:
   r0 = M[I0,M1];
   r0 = r0 LSHIFT r4;                 // Copy last 2 bytes of input into upper field
   M[I1,M0] = r0;                     // Leave write pointer to the word just written
   jump input_processed;

   second_packet:
   // When joining two 30 byte packets together we need to read the last 2 octets from the
   // first pass and OR with the first two octets from the second pass to make the 32-bits
   r0 = M[I0,M1];                     // New 2 octets
   r1 = M[I1,M0];                     // Previous 2 octets in upper field of word
   r0 = r0 OR r1;
   M[I1,M1] = r0;                     // Update word
   r10 = 7;                           // Copy 28 bytes
   r4 = 16;                           // Data is packed in 16 bit words
   do input_copy_second;               // copy from circular buffer to linear buffer
   r0 = M[I0,M1];
   r0 = r0 LSHIFT r4,
   r1 = M[I0,M1];
   r1 = r1 AND 0xFFFF;                // Mask upper 16-bits off in case there is stale data
   r0 = r0 OR r1;
   M[I1,M1] = r0;
   input_copy_second:

   input_processed:
   r0 = I1 - I2;                      // calculate relative write pointer to base
   M[r2 + $AXBUF_WPTR_OFFSET] = r0;   // set write pointer
   
   r4 = 0;
   Null = r0 - SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES;
   if LT jump not_enough_input_data;  // Full SWBS packet are 60 bytes

   // Reset circular buffer registers, codec uses linear axbuf
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;

   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;  // flattened
   I1 = M[r2 + $AXBUF_DAT_OFFSET];   
   M[r2 + $AXBUF_RPTR_OFFSET] = 0;   // set relative read pointer to be zero
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;   // set relative write pointer to be zero
// INPUTS:
//    r1    - Packet timestamp
//    r2    - Packet status
//    r5    - payload size in bytes
//    I0,L0,B0 - Input CBuffer
//    R9    - data object pointer
//
//    decoupled variant
//    r8    - Output CBuffer

   r0 = M[r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.CODEC_DATA_FIELD];
   r1 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXINBUF_FIELD;
   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;

   r4 = r3;
   call $aptx_adaptive.voice_decode;    /// aptX adaptive decoder call
   r3 = r4;
   
// OUTPUTS:
//    r5    - Output packet status
//    I0    - Input buffer updated (This is not alway true)

//   r0 = r5;

   // Copy data from codec output axbuf into system output buffer

      // get the Output buffer
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_write_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = write address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;
   

   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;
   I1 = M[r2 + $AXBUF_DAT_OFFSET];  
   r0 = M[r2 + $AXBUF_WPTR_OFFSET];
   Addr2Words(r0);                      // Convert output byte count to sample count
   r10 = 240;                           // Max output data is 240 samples (Mode0 - 32kHz)
   Null = r0 - r10;
   if LT r10 = r0;

  // We haev finished with internal buffer, clear
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;
   M[r2 + $AXBUF_RPTR_OFFSET] = 0;

default_mode:
   r4 = 0;
   Null = r1 - r10;
   if LT jump no_space;
   r4 = r10;
   do output_copy;                      // copy and shift from linear buffer to circular buffer
   r0 = M[I1,M1];
   r0 = r0 ASHIFT 8;                    // aptX adaptive produces 24-bit aligned audio (in 32-bit word)
   M[I0,M1] = r0;
   output_copy:
   
   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;      // set relative output write pointer to be zero

   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXINBUF_FIELD;
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;      // set relative input write pointer to be zero

no_space:
not_enough_input_data:

   // Reset circular buffer registers
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;

   r0 = r4;                             // Return packet size
   
   POP_ALL_C
   rts;
   


   
.ENDMODULE;


// *****************************************************************************
// FUNCTION:
//    $_sco_decoder_swbs_validate
//
// DESCRIPTION:
//    C callable version of the $sco_decoder.swbs.validate function.
//    Validates the packet. Must be called before decode.
//    For SWBS this only checks size, input data and output space.
//    There is no sync word checking or data validation. 
//
// INPUTS:
//    r0 - swbs_dec.CODEC_DATA_FIELD
//    r1 - payload length in words
//    r2 - pointer which will be populated with the swbs packet length.
//
// OUTPUTS:
//    r0 - Output in samples, 0 to abort
//
// CPU USAGE:
//    D-MEMORY: xxx
//    P-MEMORY: xxx
//    CYCLES:   xxx
//
// NOTES:
// *****************************************************************************

.MODULE $M.sco_decoder.swbs._validate;

   .DATASEGMENT DM;
   .CODESEGMENT PM;

$_sco_decoder_swbs_validate:

   PUSH_ALL_C
   
    // save OP DATA
   r7 = r0;

   // payload length
   r5 = r1;// this should be already in words.

   LIBS_SLOW_SW_ROM_PATCH_POINT($swbs_cap.WBS_C_STUBS_ASM.SCO_DECODER.SWBS._VALIDATE._SCO_DECODER_SWBS_VALIDATE.PATCH_ID_0,r6)
   
    // save the pointer to the swbs packet length
   push r2;  
   
   // Calc amount of data in input buffer - return r0 = 1 if no data
   r6 = 1;
   // get extra op data
   r0 = r7;
   call $base_op.base_op_get_instance_data;
   r3 = r0;
   // get the Input buffer
   r0 = M[r3 + $sco_struct.SCO_TERMINAL_BUFFERS_struct.IP_BUFFER_FIELD];
   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.calc_amount_data_in_words;

   Null = r0 - 15;     // Smallest input packet supported is 30 bytes (largest is 60 bytes)
   if LT jump return;  // No data
   
   // Calc amount of space in output buffer - return r0 = 0 if no space
   r6 = 0;
   // get extra op data
   r0 = r7;
   call $base_op.base_op_get_instance_data;
   r3 = r0;
   // get the Input buffer
   r0 = M[r3 + $sco_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];
   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.calc_amount_space_in_words;
   // OUTPUTS:
   // r0 = amount of space (for new data) in words or addresses
   // r2 = buffer size in words or addresses
   // save the amount of space available in output buffer


   r4 = SWBS_MODE0_AUDIO_BLOCK_SIZE;
   r6 = SWBS_MODE4_AUDIO_BLOCK_SIZE;
   r1 = M[r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.CODECMODE_FIELD];
   Null = r1 - SWBS_CODEC_MODE4;
   if EQ r4 = r6;

   Null = r0 - r4;      // Check if there is enough enoutput space
   if LT jump return;   // No space
   r6 = r4;             // Set output samples length

   // Else return output samples length

return:
   pop r2;
   // populate the swbs packet length
   M[r2] = r5;

   r0 = r6; // populate the return value
   POP_ALL_C
   rts;

   
.ENDMODULE;

#ifdef ESCO_SUPPORT_ERRORMASK

// *****************************************************************************
// FUNCTION:
//    $sco_decoder.swbs.process:
//
// DESCRIPTION:
//
// INPUTS:
// OUTPUTS:
// TRASHED REGISTERS:
//    Assumes everything
//
// NOTES: Takes as input 62/122 bytes. Output 64/124. Weak HDR in out is 32 bits
// NO_ZEAGLE_RX is what was released for the Aura Plus rom, ready to be patched
// *****************************************************************************

#ifdef NO_ZEAGLE_RX
 #define IZG I5
#else
 #define IZG I0
#endif

.MODULE $M.sco_decoder.swbs._process_bit_error;

   .DATASEGMENT DM;
   .CODESEGMENT PM;

$_sco_decoder_swbs_process_bit_error:

// r0 op data
// r1 pointer to the stream PACKET
// r2 swbs_packet_length
   PUSH_ALL_C

   // save OP DATA
   r7 = r0;
   // save pointer to the packet
   r9 = r1;


   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($swbs_cap.SWBS_C_STUBS_ASM.SCO_DECODER.SWBS._PROCESS._SCO_DECODER_SWBS_PROCESS_BIT_ERROR.PATCH_ID_0,r5)

   // save the swbs packet size
   r5 = r2;

   // get extra op data
   call $base_op.base_op_get_instance_data;
   r3 = r0;

   // get the Input buffer
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.IP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_read_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = read address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;

   // Setup the pointer to the bit error field

#ifdef NO_ZEAGLE_RX
  I5 = M[r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.PBITERRORBUFF_FIELD];
  r6 = SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES;
#else
   r6 = SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES_WITH_BIT_ERROR; // length of full packet with bit error
#endif

   // Copy data from system input buffer to codec input axbuf.
   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXINBUF_FIELD;  // flattened
   I1 = M[r2 + $AXBUF_DAT_OFFSET];
   M[r2 + $AXBUF_RPTR_OFFSET] = 0;    // set relative read pointer to be 0
   I2 = I1;                           // store base address
   r0 = M[r2 + $AXBUF_WPTR_OFFSET];
   I1 = I1 + r0;

   r4 = 16;                           // Data is packed in 16 bit words

   Null = r5 - r6;
   if NE jump not_60_byte_packets;
   r10 = 15;                          // Copy 60 bytes
   do input_copy;                     // copy from circular buffer to linear buffer
       r0 = M[I0,M1];
       r0 = r0 LSHIFT r4,
       r1 = M[I0,M1];
       r1 = r1 AND 0xFFFF;                // Mask upper 16-bits off in case there is stale data
       r0 = r0 OR r1;
       M[I1,M1] = r0;
   input_copy:

// copy in the weak header field (read 16, output 32 )
   r0 = M[IZG,M1];
   M[I1,M1] = r0;

// read in error
   r10 = 15;                          // Copy 60 bytes
   do input_copy_bit_err;             // copy from circular buffer to linear buffer
       r0 = M[IZG,M1];
       r0 = r0 LSHIFT r4,
       r1 = M[IZG,M1];
       r1 = r1 AND 0xFFFF;                // Mask upper 16-bits off in case there is stale data
       r0 = r0 OR r1;
       M[I1,M1] = r0;
   input_copy_bit_err:
   r5 = I1;

jump input_processed;


// handle having two 30 byte packets

not_60_byte_packets:
/******************
 * Write Codewords
 ******************/
   Null = r0;                        // M[r2 + $AXBUF_WPTR_OFFSET];
   if NZ jump second_packet;
   first_packet:
   r10 = 7;                          // Copy 28 bytes
   do input_copy_first;              // copy from circular buffer to linear buffer
       r0 = M[I0,M1];
       r0 = r0 LSHIFT r4,
         r1 = M[I0,M1];
       r1 = r1 AND 0xFFFF;           // Mask upper 16-bits off in case there is stale data
       r0 = r0 OR r1;
       M[I1,M1] = r0;
   input_copy_first:

  // write bytes 29 & 30
   r5 = I1;                          // Store this write pointer to set wptr later on
   r0 = M[I0,M1];                    // Read last 16 bit word
   r0 = r0 LSHIFT r4;                // Copy last 2 bytes of input into upper field
   M[I1,M1] = r0;


   // Written codewords, now write weak header
   r0 = SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES; // size of fully reconstructed block
   I1 = I2 + r0;                     // increment to point in buffer where to write weak header

/********************
 * Write Weak Header
 ********************/
   r0 = M[IZG,M1];                    // Read weak header from stream
   M[I1,M1] = r0;                    // Leave write pointer to the word just written

/***********************
 * Write Bit Error data
 ***********************/
   r10 = 7;                           // Copy 28 bytes
   do input_copy_bit_err_first;       // copy from circular buffer to linear buffer
       r0 = M[IZG,M1];
       r0 = r0 LSHIFT r4,
         r1 = M[IZG,M1];
       r1 = r1 AND 0xFFFF;            // Mask upper 16-bits off in case there is stale data
       r0 = r0 OR r1;
       M[I1,M1] = r0;
   input_copy_bit_err_first:

  // write bytes 29 & 30
   r0 = M[IZG,M1];                    // Read last 16 bit word
   r0 = r0 LSHIFT r4;                // Copy last 2 bytes of input into upper field
   M[I1,M1] = r0;                    // Leave write pointer to the word just written

   jump input_processed;

second_packet:

/******************
 * Write Codewords
 ******************/
   // When joining two 30 byte packets together we need to read the last 2 octets from the
   // first pass and OR with the first two octets from the second pass to make the 32-bits
   r0 = M[I0,M1];                     // New 2 octets
   r1 = M[I1,M0];                     // Previous 2 octets in upper field of word
   r0 = r0 OR r1;
   M[I1,M1] = r0;                     // Update word
   r10 = 7;                           // Copy 28 bytes

   do input_copy_second;               // copy from circular buffer to linear buffer
       r0 = M[I0,M1];
       r0 = r0 LSHIFT r4,
         r1 = M[I0,M1];
       r1 = r1 AND 0xFFFF;                // Mask upper 16-bits off in case there is stale data
       r0 = r0 OR r1;
       M[I1,M1] = r0;
   input_copy_second:

/********************
 * Write Weak Header
 ********************/

   // I0 points to new source weak header
   // I5 should point to the source weak header (16 bit) - allocated memory in test or the same as I0(?) in released/patched
   // I1 is should point to weak header of the flat buffer

   r1 = M[I1, M0];   // read existing weak header  ( 16 bits 0x0000ffff) and first error pkt
   r5 = M[IZG, M1]; // read new weak header ( 16 bits 0x0000ffff)

   // Generate weak header from existing 16 bit
   r0 = r1 OR r5;          // store the flags
   r0 = r0 AND 0xc000;     // mask out count bits r0, now contains the flags

   r1 = r1 + r5;           // add the counts
   r1 = r1 AND 0x3fff;     // mask off flag bits (there should be no overflow!)
   r0 = r0 OR r1;          // add stored flags back

// weak header now in r0 (0x0000ffff)
// Read first bit error
   push M3;
   M3 = 8 * ADDR_PER_WORD;

   M[I1,M3] = r0;     // weak header now stored in output buffer
   pop M3;

/***********************
 * Write Bit Error data
 ***********************/

   // When joining two 30 byte packets together we need to read the last 2 octets from the
   // first pass and OR with the first two octets from the second pass to make the 32-bits

   r0 = M[IZG,M1];                     // New 2 octets (0x0000ffff)
   r1 = M[I1,M0];                   // Previous 2 octets in upper field of word (0xffff0000)
   r0 = r0 OR r1;
   M[I1,M1] = r0;                     // Update word
   r10 = 7;                           // Copy 28 bytes

   do input_copy_second_bit_err_second;// copy from circular buffer to linear buffer
       r0 = M[IZG,M1];
       r0 = r0 LSHIFT r4,
       r1 = M[IZG,M1];
       r1 = r1 AND 0xFFFF;                // Mask upper 16-bits off in case there is stale data
       r0 = r0 OR r1;
       M[I1,M1] = r0;
   input_copy_second_bit_err_second:
   r5 = I1; // should point to end of bit error buffer


   input_processed:
   r0 = r5 - I2;                      // calculate relative write pointer to base (excludes bit error data if not completed, if completed includes it)
   M[r2 + $AXBUF_WPTR_OFFSET] = r0;   // set write pointer

   r4 = 0;
   Null = r0 - r6; // SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES_WITH_BIT_ERROR;
   if LT jump not_enough_input_data;  // Full SWBS packet are 60 bytes

   // Reset circular buffer registers, codec uses linear axbuf
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;

   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;  // flattened
   I1 = M[r2 + $AXBUF_DAT_OFFSET];
   M[r2 + $AXBUF_RPTR_OFFSET] = 0;   // set relative read pointer to be zero
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;   // set relative write pointer to be zero
// INPUTS:
//    r1    - Packet timestamp
//    r2    - Packet status
//    r5    - payload size in bytes
//    I0,L0,B0 - Input CBuffer
//    R9    - data object pointer
//
//    decoupled variant
//    r8    - Output CBuffer

   r0 = M[r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.CODEC_DATA_FIELD];
   r1 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXINBUF_FIELD;
   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;

   r4 = r3;
   call $aptx_adaptive.voice_decode;    /// aptX adaptive decoder call
   r3 = r4;

// OUTPUTS:
//    r5    - Output packet status
//    I0    - Input buffer updated (This is not alway true)

//   r0 = r5;

   // Copy data from codec output axbuf into system output buffer

      // get the Output buffer
   r0 = M[r3 + $swbs_struct.SCO_TERMINAL_BUFFERS_struct.OP_BUFFER_FIELD];

   // INPUTS:
   // r0 = pointer to cbuffer structure
   call $cbuffer.get_write_address_and_size_and_start_address;
   // OUTPUTS:
   // r0 = write address
   // r1 = buffer size in addresses (locations)
   // r2 = buffer start address

   I0 = r0;
   L0 = r1;
   push r2;
   pop B0;


   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;
   I1 = M[r2 + $AXBUF_DAT_OFFSET];
   r0 = M[r2 + $AXBUF_WPTR_OFFSET];
   Addr2Words(r0);                      // Convert output byte count to sample count
   r10 = 240;                           // Max output data is 240 samples (Mode0 - 32kHz)
   Null = r0 - r10;
   if LT r10 = r0;

default_mode:
   r4 = 0;
   Null = r1 - r10;
   if LT jump no_space;
   r4 = r10;
   do output_copy;                      // copy and shift from linear buffer to circular buffer
   r0 = M[I1,M1];
   r0 = r0 ASHIFT 8;                    // aptX adaptive produces 24-bit aligned audio (in 32-bit word)
   M[I0,M1] = r0;
   output_copy:

   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXOUTBUF_FIELD;
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;      // set relative output write pointer to be zero

   r2 = r3 + $swbs_struct.SWBS_DEC_OP_DATA_struct.AXINBUF_FIELD;
   M[r2 + $AXBUF_WPTR_OFFSET] = 0;      // set relative input write pointer to be zero

no_space:
not_enough_input_data:

   // Reset circular buffer registers
   L0 = Null;
   r0 = Null;
   push r0;
   pop B0;

   r0 = r4;                             // Return packet size

   POP_ALL_C
   rts;




.ENDMODULE;

#endif
