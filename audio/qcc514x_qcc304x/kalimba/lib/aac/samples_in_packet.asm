// *****************************************************************************
// Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#include "aac_library.h"
#include "cbuffer_asm.h"
#include "stack.h"
#include "portability_macros.h"
#include "pmalloc/pl_malloc_preference.h"

#define CORE_AAC_FRAME_LENGTH           1024

// *****************************************************************************
// MODULE:
//    $aacdec.create_dummy_decoder
//
// DESCRIPTION:
//    Create a dummy AAC decoder 
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - pointer to the dummy decoder
//
// TRASHED REGISTERS:
//    - assume everything
//
// NOTES:
//    The dummy decoder created here is used by other libraries/capabilities
//    to determine the number of audio samples contained in an AAC LC packet 
//    transported over LATM (utility to call: aacdec_samples_in_packet_lc).
//    
//    The decoder will only have a minimal set of tables initialised.
//
// *****************************************************************************

.MODULE $M.aacdec.create_dummy_decoder;
   .CODESEGMENT AACDEC_CREATE_DUMMY_DECODER_PM;
   .DATASEGMENT DM;

   $_aacdec_create_dummy_decoder:
   $aacdec.create_dummy_decoder:

   PUSH_ALL_C

   // Allocate memory for the decoder's structure -> early exit if unsuccessful
   r0 = $aac.mem.STRUC_SIZE * ADDR_PER_WORD;
   r1 = MALLOC_PREFERENCE_NONE;
   call $_xzppmalloc;
   r9 = r0;
   if Z jump exit1;

   // Taken from $aacdec.private.init_tables
#ifdef AACDEC_TABLE_NO_FLASH_COPY
   r0 = &$aacdec.bitmask_lookup;
   M[r9 + $aac.mem.BITMASK_LOOKUP_FIELD] = r0;

   r0 = &$aacdec.sample_rate_tags;
   M[r9 + $aac.mem.SAMPLE_RATE_TAGS_FIELD] = r0;

#else
   r4 = 17;
   Words2AddrSD(r4, r0);
   r1 = MALLOC_PREFERENCE_DM2;
   call $_xzppmalloc;
   M[r9 + $aac.mem.BITMASK_LOOKUP_FIELD] = r0;
   if Z jump exit2;
   
   r0 = &$aacdec.bitmask_lookup;
   r1 = r4;
   r2 = M[r9 + $aac.mem.BITMASK_LOOKUP_FIELD];
   call $aacdec.flash_copy;

   r4 = 12;
   Words2AddrSD(r4, r0);
   r1 = MALLOC_PREFERENCE_DM2;
   call $_xzppmalloc;
   M[r9 + $aac.mem.SAMPLE_RATE_TAGS_FIELD] = r0;
   if Z jump exit3;

   r0 = &$aacdec.sample_rate_tags;
   r1 = r4;
   r2 = M[r9 + $aac.mem.SAMPLE_RATE_TAGS_FIELD];
   call $aacdec.flash_copy;

   jump exit1;

   exit3:
      r0 = M[r9 + $aac.mem.BITMASK_LOOKUP_FIELD];
      call $_pfree;
   exit2:
      r0 = r9;
      call $_pfree;
      r9 = Null;
#endif
   exit1:
   // Return the pointer to the dummy decoder or Null
   r0 = r9;
   POP_ALL_C
   rts;

// *****************************************************************************
// FUNCTION:
//    $aacdec.flash_copy
//
// DESCRIPTION:
//    Copy data tables from Flash/ROM to RAM 
//    (declared in module $aacdec.create_dummy_decoder as no one else needs it)
// INPUTS:
//    - r0 - pointer to source data
//    - r1 - data size in words
//    - r2 - pointer to destination
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    - r0, r10, I1, I4, doLoop
//
// *****************************************************************************
   $aacdec.flash_copy:
   I1 = r0;
   I4 = r2;
   r10 = r1 - 1;
   r0 = M[I1, MK1];
   do copy_aac_table_to_ram;
      r0 = M[I1, MK1], M[I4, MK1] = r0;
   copy_aac_table_to_ram:
   M[I4, MK1] = r0;
   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $aacdec.destroy_dummy_decoder
//
// DESCRIPTION:
//    Destroy a dummy AAC decoder 
//
// INPUTS:
//    - pointer to the dummy decoder
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    - assume everything
//
// *****************************************************************************

.MODULE $M.aacdec.destroy_dummy_decoder;
   .CODESEGMENT AACDEC_DESTROY_DUMMY_DECODER_PM;
   .DATASEGMENT DM;

   $_aacdec_destroy_dummy_decoder:
   $aacdec.destroy_dummy_decoder:

   PUSH_ALL_C
   r9 = r0;
   // free up BITMASK_LOOKUP
   r0 = M[r9 + $aac.mem.BITMASK_LOOKUP_FIELD];
   call $_pfree;
   // free up SAMPLE_RATE_TAGS
   r0 = M[r9 + $aac.mem.SAMPLE_RATE_TAGS_FIELD];
   call $_pfree;
   // free up the dummy decoder's structure
   r0 = r9;
   call $_pfree;
   
   POP_ALL_C
   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $aacdec.samples_in_packet_lc
//
// DESCRIPTION:
//    Calculate how many audio samples are in an encoded AAC packet
//
// INPUTS:
//    - r0 = pointer to a $aac.dummy_decode structure
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    - assume everything
//
// NOTES:
//    This module is used as a utility function by other libraries/capabilities
//    to determine the number of audio samples contained in a packet.
//
//    The utility:
//    - supports only AAC-LC transported over LATM.
//    - receives a pointer to a dummy AAC decoder allocated by the RTP decoder. 
//    - doesn't touch the input cbuffer passed by the caller; it operates on a 
//    clone cbuffer allocated internally.
//    -  parses the stream up to AudioMuxElement -> PayloadLengthInfo and skips
//    the rest of the payload.
//    - returns valid == FALSE in frame_dec_struc only if a frame is detected as
//    corrupted. 
//
// API:
// *****************************************************************************
//    The user communicates with the utility via two structures - dummy_decoder
//    and frame_dec_struc. The user ALLOCATES both structures and populates
//    the fields:
//      - aac_decoder_struc
//      - input_buffer
//      - payload_size
//      - frame_dec_struc (RTP_FRAME_DECODE_DATA with all the fields on 0)
//      - bit_position
//
//    and will find the 'number of samples' calculated in the structure pointed
//    to by frame_dec_struc. The user will know how many octets of the AAC
//    stream have been consumed from the frame_length field of frame_dec_struc;
//    bit_position will be updated by the utility.
//
//    The user is responsible for FREEING UP the dummy_decoder and the
//    frame_dec_struc structures.
//
// *****************************************************************************
//
//    typedef struct
//    {
//        void            *aac_decoder_struc;
//        void            *input_buffer;
//        unsigned int    payload_size;
//        void            *frame_dec_struc;
//        unsigned int    bit_position;
//        void            *cbuff_clone;
//        unsigned int    payload_left;
//    } dummy_decoder;
//
// Where:
// -----------------------------------------------------------------------------
// FIELD NAME                |DIR     | Details
// -----------------------------------------------------------------------------
// aac_decoder_struc         | I      | Pointer to the dummy decoder's structure
//                           |        | of type aac_codec structure(see aac_c.h)
// -----------------------------------------------------------------------------
// in_cbuffer                | I      | Pointer to a tCbuffer containing the
//                           |        | encoded AAC data (may be different from
//                           |        | the in_buffer contained in codec_struc)
// -----------------------------------------------------------------------------
// payload_size              | I      | Amount of encoded data in input_buffer
//                           |        | the dummy decoder will look at [octets].
// -----------------------------------------------------------------------------
// bit_position              | I      | It is used to initialise
//                           |        | $aac.mem.GET_BITPOS and should refer to
//                           |        | the stream passed in the input_buffer.
// -----------------------------------------------------------------------------
// frame_dec_struc           | O      | Pointer to structure of type
//                           |        | RTP_FRAME_DECODE_DATA (see
//                           |        | rtp_decode_struct.h)containing:
//                           |        |  - valid [bool]
//                           |        |  - frame_length [octets] (if known)
//                           |        |  - frame_samples
//                           |        |  - nr_of_frames (in the payload)
// -----------------------------------------------------------------------------
// cbuff_clone               | Status | The caller can ignore these fields.
// payload_left              |        | They are used internally by the utility
//                           |        | and have been added to this structure
//                           |        | for convenience.
// -----------------------------------------------------------------------------
//
// *****************************************************************************

.MODULE $M.aacdec.samples_in_packet_lc;
   .CODESEGMENT AACDEC_SAMPLES_IN_PACKET_LC_PM;
   .DATASEGMENT DM;

   .VAR io_struc_ptr_lc = 0;

   $_aacdec_samples_in_packet_lc:
   $aacdec.samples_in_packet_lc:

   // Save I/O structure pointer for later use and check its validity
   M[io_struc_ptr_lc] = r0;
   if Z rts;

   PUSH_ALL_C
#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($aacdec.SAMPLES_IN_PACKET_LC_ASM.SAMPLES_IN_PACKET.SAMPLES_IN_PACKET.PATCH_ID_1, r1)
#endif

   r6 = M[io_struc_ptr_lc];
   r0 = M[r6 + $aac.dummy_decode.IN_CBUFFER];
   r9 = M[r6 + $aac.dummy_decode.AAC_DECODER_STRUC];
   // default is no faults detected
   M[r9 + $aac.mem.FRAME_UNDERFLOW] = Null;
   M[r9 + $aac.mem.FRAME_CORRUPT] = Null;
   M[r9 + $aac.mem.POSSIBLE_FRAME_CORRUPTION] = Null;
#ifdef KYMERA
   call $cbuffer.calc_amount_data_in_words;
#else
   call $cbuffer.calc_amount_data;
#endif
#ifdef DATAFORMAT_32
   r0 = r0 LSHIFT LOG2_ADDR_PER_WORD;
#else
   r0 = r0 + r0;
#endif
   r1 = M[r6 + $aac.dummy_decode.PAYLOAD_SIZE];
   // If payload size is zero there is nothing to decode -> exit
   if Z jump exit1;
   Null = r0 - r1;
   // If the input buffer doesn't hold at least PAYLOAD_SIZE octets -> exit
   if NEG jump exit1;

   // Allocate memory for cloning input buffer -> early exit if unsuccessful
   r0 = $cbuffer.STRUC_SIZE * ADDR_PER_WORD;
   r1 = MALLOC_PREFERENCE_NONE;
   call $_xzppmalloc;
   M[r6 + $aac.dummy_decode.CBUFF_CLONE] = r0;
   if Z jump exit1;

   // Clone input buffer
   r10 = $cbuffer.STRUC_SIZE;
   r0 = M[r6 + $aac.dummy_decode.IN_CBUFFER];
   r1 = M[r6 + $aac.dummy_decode.CBUFF_CLONE];
   I0 = r0;
   I4 = r1;
   do clone_in_buff;
      r4 = M[I0, MK1];
      M[I4, MK1] = r4;
   clone_in_buff:

   // Make sure TMP_MEM_POOL points to a safe place 
   // (in an unused area of the decoder's structure).
   r0 = r9 + $aac.mem.TMP;
   M[r9 + $aac.mem.TMP_MEM_POOL_END_PTR] = r0;
   M[r9 + $aac.mem.TMP_MEM_POOL_PTR] = r0;

   // A few more initialisations before the frame loop
   r0 = M[r6 + $aac.dummy_decode.GET_BITPOS];
   r1 = M[r6 + $aac.dummy_decode.PAYLOAD_SIZE];
   M[r9 + $aac.mem.GET_BITPOS] = r0;
   M[r6 + $aac.dummy_decode.PAYLOAD_LEFT] = r1;

   // Set validation to TRUE and frame_length to 0
   r1 = 1;
   r5 = M[r6 + $aac.dummy_decode.FRAME_DEC_STRUC];
   M[r5 + $aac.frame_decode_data.valid] = r1;
   M[r5 + $aac.frame_decode_data.frame_length] = 0;

   reattempt_decode:
      // Setup aac input stream buffer info
      // Make I0 to point to the cloned input cbuffer
      r0 = M[r6 + $aac.dummy_decode.CBUFF_CLONE];
      #ifdef BASE_REGISTER_MODE
         call $cbuffer.get_read_address_and_size_and_start_address;
         push r2;
         pop B0;
      #else
         call $cbuffer.get_read_address_and_size;
      #endif
      I0 = r0;
      L0 = r1;

      // Determine how much we've got left in the payload in bits/bytes
      // and reset the bit counter for the current raw data block;
      r0 = M[r6 + $aac.dummy_decode.PAYLOAD_LEFT];
      M[r9 + $aac.mem.READ_BIT_COUNT] = Null;
      r1 = r0 ASHIFT 3;
      M[r9 + $aac.mem.FRAME_NUM_BITS_AVAIL] = r1;
      M[r9 + $aac.mem.NUM_BYTES_AVAILABLE] = r0;

      // Do we have enough data to start decoding a frame?
      r4 = $aacdec.MIN_LATM_FRAME_SIZE_IN_BYTES;
      Null = r0 - r4;
      if NEG jump exit2;

      // Parse latm frame up to payload_length_info.
      // This is the top of audio_mux_element() with muxConfigPresent = 1
      call $aacdec.get1bit;
      if Z call $aacdec.stream_mux_config;
      // Corruption check 1: POSSIBLE_FRAME_CORRUPTION raised by $aacdec.stream_mux_config
      Null = M[r9 + $aac.mem.POSSIBLE_FRAME_CORRUPTION];
      if NZ jump frame_corrupted;

      call $aacdec.payload_length_info;
      M[r9 + $aac.mem.latm.MUX_SLOT_LENGTH_BYTES] = r4;

      // Corruption check 2: MUX_SLOT_LENGTH_BYTES too large
      Null = r4 - ($aacdec.MAX_AAC_FRAME_SIZE_IN_BYTES*2);
      if POS jump frame_corrupted;

      // Corruption check 3: if amount read too large fall through to corrupt frame
      r0 = M[r9 + $aac.mem.READ_BIT_COUNT];
      Null = r0 -  ($aacdec.MAX_AAC_FRAME_SIZE_IN_BYTES*8*2);
      if NEG jump skip_frame;

      // The decoding of current frame failed
   frame_corrupted:
      // Set validity to FALSE and exit
      r2 = M[io_struc_ptr_lc];
      r4 = M[r2 + $aac.dummy_decode.FRAME_DEC_STRUC];
      M[r4 + $aac.frame_decode_data.valid] = 0;
      jump exit2;

   skip_frame:
      // We are not going to parse the whole raw data block - we try to 
      // skip the frame; first check if we have enough data available.
      r5 = M[r9 + $aac.mem.latm.MUX_SLOT_LENGTH_BYTES];
      r4 = r5 LSHIFT 3;
      r1 = M[r9 + $aac.mem.READ_BIT_COUNT];
      r1 = r1 + r4;
      r0 = M[r9 + $aac.mem.FRAME_NUM_BITS_AVAIL];
      Null = r0 - r1;
      // If not enough data exit immediately.
      if NEG jump exit2;

      // byte_alignment - should be byte aligned anyway
      call $aacdec.byte_align;

      // Skip the rest of the frame by a combination of:
      // * moving the encoded stream read pointer ahead by full words & manually update READ_BIT_COUNT,
      // * use $aacdec.get1byte if MUX_SLOT_LENGTH_BYTES is odd ($aacdec.get1byte takes care of READ_BIT_COUNT).
      // We update READ_BIT_COUNT to simulate a full parsing of the encoded stream.
#ifdef DATAFORMAT_32
      // Convert octets to words (4 octets per word)
      r1 = r5 LSHIFT -2;
      // Calculate number of bits per word
      r2 = r1 LSHIFT 5;
#else
      // Convert octets to words (2 octets per word)
      r1 = r5 LSHIFT -1;
      // Calculate number of bits per word
      r2 = r1 LSHIFT 4;
#endif
      Words2Addr(r1);
      M0 = r1;
      r1 = M[I0, M0];
      r1 = M[r9 + $aac.mem.READ_BIT_COUNT];
      r1 = r1 + r2;
      M[r9 + $aac.mem.READ_BIT_COUNT] = r1;
#ifdef DATAFORMAT_32
      r0 = r5 AND 3;
      r0 = r0 LSHIFT 3;
      if NZ call $aacdec.getbits;
#else
      Null = r5 AND 1;
      if NZ call $aacdec.get1byte;
#endif

   successful_decode:
      // Increment frame counter and set decoded frame size
      r6 = M[io_struc_ptr_lc];
      r4 = CORE_AAC_FRAME_LENGTH;
      r3 = M[r6 + $aac.dummy_decode.FRAME_DEC_STRUC];
      r0 = M[r3 + $aac.frame_decode_data.nr_of_frames];
      r0 = r0 + 1;
      M[r3 + $aac.frame_decode_data.nr_of_frames] = r0;
      M[r3 + $aac.frame_decode_data.frame_samples] = r4;

   update_payload_left:
      // Update payload left and exit.
      // We don't attempt any reruns as we want the caller to timestamp each
      // frame (see B-236217 - dg 10705717 and B-243436 for more details).
      
      r1 = M[r9 + $aac.mem.READ_BIT_COUNT];
      r2 = M[r6 + $aac.dummy_decode.PAYLOAD_LEFT];
      r0 = r1 ASHIFT -3;
      // Update the frame_length == the amount of data consumed from
      // the input stream (this may cover more than one frame!)
      r4 = M[r3 + $aac.frame_decode_data.frame_length];
      r4 = r4 + r0;
      M[r3 + $aac.frame_decode_data.frame_length] = r4;

      r2 = r2 - r0;
      M[r6 + $aac.dummy_decode.PAYLOAD_LEFT] = r2;

   exit2:
      // Free cloned input cbuffer structure
      r6 = M[io_struc_ptr_lc];
      r0 = M[r6 + $aac.dummy_decode.CBUFF_CLONE];
      call $_pfree;
   exit1:

   POP_ALL_C
   rts;

.ENDMODULE;

