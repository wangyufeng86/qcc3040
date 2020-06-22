// *****************************************************************************
// Copyright (c) 2016 - 2018 Qualcomm Technologies International, Ltd.
// *****************************************************************************

                     //----Summary----//
// Cbuffer (_ex) library functions supporting octet access. This cbuffer library
// extension can track octets within a word.
// Not all cbuffer functions are extended.

                     //----What's supported?----//
// 16-bit unpacked MMU and Software buffers and 32-bit packed SW buffers are supported.
// Only supports 32-bit KAL_ARCH4 platform.
// AUX pointers are not supported.


                     //----Read pointers----//
// For Software read pointers, the number of octets read within a word are
// stashed in the 2 LSBs of the read pointer. There can only be one odd octet for 16-bit unpacked
// and 2 odd octets for 32-bit packed.
//
// The SW read pointer is not incremented until all octets in the word are read.
// The SW read pointer always points to an unread or a partially read word.

// For MMU read pointers (handles), the octet information is available in the
// BAC buffer offset and the read pointer (that points to an MMU handle) in the
// cbuffer structure is not touched.

// The _ex cbuffer and the standard cbuffer functions always a rounded down
// version of the read pointer.
// The _ex cbuffer functions additionally return octet information
// (in a separate register).

                     //----Write pointers----//
// For Software write pointers, the number of octets written into a word are
// stashed in the 2 LSBS of the write pointer. If a partial word is written
// the write pointer is immediately rounded up (incremented by ADDR_PER_WORD).
// Any further odd octets written to to the same word will keep the write
// pointer pointing to the same (rounded up) location. The SW write pointer
// always points to the next whole word to write to.

// For MMU write pointers (handles) the octet information is available in the
// BAC buffer offset.
// The internal BAC offsets are obviously not rounded up as these offsets can
// be used by entities outside the audio subsystem.

// Note that the _ex cbuffer functions always return the "truth" (*).
// For example, $cbuffer.get_write_address_ex)
// will return the "true" (*) (not rounded up) write pointer. Octet information
// is returned separately (for example, in a separate register).

// (*) "truth/true pointer" in this document/header and throughout cbuffer_ex
// are meant to differentiate from the plain/traditional cbuffer API that would
// "lie", i.e. return incremented/rounded-up write pointers for partially used
// words.
// Be aware that the pointers _are_ in reality rounded-up when an odd octet is
// written (if you look into the cbuffer structure - write_address)
// but the _ex API hides this from user (returns "true" pointer).

// For SW write pointers the standard cbuffer functions
// ($cbuffer.get_write_address_and_size_and_start_address)
// will return the rounded up version of the write pointer. Obviously no octet
// information is returned. This is so codecs such as SBC that use the standard
// cbuffer functions can see that there is enough data in their input. They may
// see some extra garbage octets too (maximum of 3 garbage octets in the 32-bit
// unpacked case, maximum of 1 garbage octet in the 16-bit unpacked case).
// These libaries are self-framing and they normally see whole packets.
// They are expected to start decode only when all data required for a frame is
// available. Therefore, seeing extra garbage octets due to a rounded up
// software write pointer is generally not an issue. Further we only tend to
// receive odd octets at the end of the stream.

// We normally expect the apps DMA engine to transfer data in multiples of 2
// (or possibly even powers of 2) chunks. Therefore we don't expect the problem
// of interpreting a garbage octet to hit us.


                  //----Misc----//
// The standard cbuffer functions will mask out the 2 LSBs from the Software
// read and the Software write pointers.
// It is important further changes to the standard cbuffer functions (that can
// operate on octet buffers) mask out the LSBs. Macros for these have been
// provided in cbuffer_asm.h (BUFFER_GET/MASK_READ/WRITE_ADDR)


                  //----Aberrations----//
// The standard cbuffer functions won't return rounded up write pointers for
// the MMU write handles. We don't expect MMU buffers to be connected to
// libraries directly so this is generally not an issue. This aberration
// is because of factors not under our control.

// For pure software buffers  (both read and write pointers are software)
// it is possible to maintain an invariant that the write and read pointers are
// at least one word apart so one can differentiate a buffer full and a buffer
// empty condition. For example a 16-bit unpacked buffer of 128 words can in
// theory hold 255 octets.
// It is possible to limit the maximum data in this buffer to 254 octets
// (by making calc_amount_space_ex return one word (2 octets) less than the
// actual space available). This will prevent the buffer holding 255 octets and
// will therefore prevent the write pointer being rounded (so the buffer won't
// appear empty when it is actually full).
// If the write pointer is an MMU handle written by an external entity it is
// not possible to maintain this invariant.
// The 16-bit unpacked buffer discussed above can hold 255 octets. This is what
// prevents the standard cbuffer functions returning a rounded up  write
// pointer when there is an underlying MMU handle. This also results in
// calc_amount_space_ex returning space - 1 (1 - 1 = 0) when the buffer has 255
// octets worth data rather than the usual space - 2 (which will otherwise be a
// negative number).


                  //----Functions callable from C and ASM ----//
// $M.cbuffer.get_read_address_ex(r0 = cbuffer address, r1 = read octet offset pointer)
// returns read-address (r0) and read octet offset written into address pointed to by offset

// $M.cbuffer.get_write_address_ex(r0 = cbuffer address, r1 = write octet offset pointer)
// returns true write-address (r0) and write octet offset written into address pointed to by offset

// $M.cbuffer.set_read_address_ex(r0 = cbuffer address, r1 = true read address, r2 = read octet offset)
// returns void

// $M.cbuffer.set_write_address_ex(r0 = cbuffer address, r1 = true write address, r2 = write octet offset)
// returns void

// $M.cbuffer.calc_amount_data_ex(r0 = cbuffer address)
// returns amount of data in octets

// $M.cbuffer.calc_amount_space_ex
// returns amount of space in octets - 2, clipped to 0

// $M.cbuffer_advance_write_ptr_ex(r0 = cbuffer address, r1 = octets to advance by)
// returns void

//$M.cbuffer_advance_read_ptr_ex(r0 = cbuffer address, r1 = octets to advance by)
// returns void

                  //----Functions callable only from ASM----//
//$M.cbuffer.get_read_address_ex(r0 = cbuffer address)
// returns true true read address (r0), read octet offset (r1), size in locations (r2), base (r3)

//$M.cbuffer.get_write_address_ex(r0 = cbuffer address)
// returns true write address (r0), write octet offset (r1), size in locations (r2), base (r3)

// *****************************************************************************

#ifdef INSTALL_CBUFFER_EX
#include "cbuffer_asm.h"
#include "io_defs.asm"
#include "patch/patch_asm_macros.h"

#define USE_16_BIT 2;
#define IS_OCTET_OFFSET_MASK 3;
#define IS_PACKED_MASK 2;
#define IS_ODD_OFFSET_MASK 1;   // check if octet is word or half-word aligned

.MODULE $M.cbuffer.get_read_address_ex;
    .CODESEGMENT BUFFER_PM;

// in r0 cbuffer address
// out r0 true read address word aligned
//     r1 octet offset
//     r2 size in addresses
//     r3 buffer start address
// trashed - none
$cbuffer.get_read_address_ex:
    LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($cbuffer.get_read_address_ex.PATCH_ID_0, r1)
    r1 = M[r0 + $cbuffer.DESCRIPTOR_FIELD];
    Null = r1 AND $cbuffer.BUFFER_TYPE_MASK;
    if Z jump its_a_sw_rd_ptr;
        Null = r1 AND $cbuffer.READ_PTR_TYPE_MASK;
    if Z jump its_a_sw_rd_ptr;
        jump $mmu.get_read_address_local_buff_ex;

    its_a_sw_rd_ptr:
    r3 = M[r0 + $cbuffer.START_ADDR_FIELD];
    BUFFER_GET_SIZE_IN_ADDRS_ASM(r2, r0);
    r0 = M[r0 + $cbuffer.READ_ADDR_FIELD];
    r1 = r0 AND BUFFER_EX_OFFSET_MASK;
    BUFFER_MASK_READ_ADDR(r0);
#ifdef INSTALL_EXTERNAL_MEM        
    // if the buffer is located in the external heap
    // and its size is >= 64KB return a buffer size of 0
    Null = r2 - 0xFFFF;
    if LE rts;
    pushm <rMAC, r0, rLink>;
    r0 = r3;
    call $_is_addr_in_ext_heap;
    Null = r0;
    if NZ r2 = 0;
    popm <rMAC, r0, rLink>;
#endif // INSTALL_EXTERNAL_MEM
    rts;

// and its C API (NOT returning size and start_address)
//extern unsigned int * cbuffer_get_read_address_ex(tCbuffer * cbuffer, unsigned * offset);
//in r0 cbuffer address
//in r1 pointer to octet offset
//out r0 read address
//trashed r2, r3
$_cbuffer_get_read_address_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($_cbuffer_get_read_address_ex.PATCH_ID_0)
    push rLink;
    push r1;
    call $cbuffer.get_read_address_ex;
    pop r2;
    M[r2] = r1;
    pop rLink;
    rts;

.ENDMODULE;


.MODULE $M.cbuffer.get_write_address_ex;
    .CODESEGMENT BUFFER_PM;

// in r0 cbuffer address
// out r0 true write address word aligned
// out r1 write octet offset
// out r2 size in locations
// out r3 base address
// trashed B0
$cbuffer.get_write_address_ex:
    LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($cbuffer.get_write_address_ex.PATCH_ID_0, r1)
    r1 = M[r0 + $cbuffer.DESCRIPTOR_FIELD];
    Null = r1 AND $cbuffer.BUFFER_TYPE_MASK;
    if Z jump its_a_sw_wr_ptr;
        // mmu only if it's not a SW pointer
        Null = r1 AND $cbuffer.WRITE_PTR_TYPE_MASK;
    if Z jump its_a_sw_wr_ptr;
        // write address is an MMU buffer
        jump $mmu.get_write_address_local_buff_ex;
    its_a_sw_wr_ptr:
    r3 = M[r0 + $cbuffer.START_ADDR_FIELD];
    BUFFER_GET_SIZE_IN_ADDRS_ASM(r2, r0);
    r0 = M[r0 + $cbuffer.WRITE_ADDR_FIELD];
    r1 = r0 AND BUFFER_EX_OFFSET_MASK;
    if Z jump test_buffer_size;
        BUFFER_MASK_WRITE_ADDR(r0);     // returns word aligned no offset
        r0 = r0 - ADDR_PER_WORD;        // go back one word
        Null = r0 - r3;
        if LT r0 = r0 + r2;             // wrap if we went past the base address
test_buffer_size:
#ifdef INSTALL_EXTERNAL_MEM        
        // if the buffer is located in the external heap
        // and its size is >= 64KB return a buffer size of 0
        Null = r2 - 0xFFFF;
        if LE rts;
        pushm <rMAC, r0, rLink>;
        r0 = r3;
        call $_is_addr_in_ext_heap;
        Null = r0;
        if NZ r2 = 0;
        popm <rMAC, r0, rLink>;
#endif // INSTALL_EXTERNAL_MEM
        rts;

// and its C API (NOT returning size and start_address)
//extern unsigned int * cbuffer_get_write_address_ex(tCbuffer * cbuffer, unsigned * offset);
//in r0 cbuffer address
//in r1 pointer to octet offset
//out r0 true write address
//trashed r2, r3, B0
$_cbuffer_get_write_address_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($_cbuffer_get_write_address_ex.PATCH_ID_0)
    push rLink;
    push r1;
    call $cbuffer.get_write_address_ex;
    pop r2;
    M[r2] = r1;
    pop rLink;
    rts;

.ENDMODULE;


//extern unsigned cbuffer_calc_amount_data_ex(tCbuffer * cb);
//in  r0 cbuffer address
//out r0 available data in octets
//trashed r1, r2, r3, B0
//only supports 16-bit unpacked/32-bit packed modes
.MODULE $M.cbuffer.calc_amount_data_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.calc_amount_data_ex:
$_cbuffer_calc_amount_data_ex:
    LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($cbuffer.calc_amount_data_ex.PATCH_ID_0, r1)
    pushm <r4, r5, r6, r7, rLink>;

    BUFFER_GET_USABLE_OCTETS(r6, r0);
    BUFFER_GET_SIZE_IN_ADDRS_ASM(r7, r0);

    push r0;
    call $cbuffer.get_write_address_ex;
    r4 = r0;
    r5 = r1;
    pop r0;
    call $cbuffer.get_read_address_ex;
    r2 = r7;
    r3 = r4 - r0;       // data = wrp - rdp
    if NEG r3 = r3 + r2;

    r4 = -1;
    Null = r6;
    if Z jump check_if_rd_eq_wr;
        r3 = r3 LSHIFT r4;

    check_if_rd_eq_wr:
    Null = r3;
    if Z jump rd_equals_wr;
        r0 = r3 - r1;   // subtract read octets
        r0 = r0 + r5;   // add written octets
        jump end;
    rd_equals_wr:
    r0 = r5 - r1;
    if POS jump end;
        // Buffer_full:
        // This is expected only if the buffer has an MMU write/read handle
        // For normal software buffers there will always be a gap of at least 2 octets.
        // This is because how much we allow to write into the buffer is completely
        //  under the control of SW running on the DSP.
        // For MMU buffers written to or read by other sub-systems this gap can be just 1 octet.
        Null = r6;
        if Z jump subtract_mmu_gap;
            r2 = r2 LSHIFT r4;  // divide by ADDR_PER_WORD - transform in usable_octets size
        subtract_mmu_gap:
        r0 = r2 + r4;
    end:
    popm <r4, r5, r6, r7, rLink>;
    rts;
.ENDMODULE;

//extern unsigned cbuffer_calc_amount_space_ex(tCbuffer * cb);
//in  r0 cbuffer address
//out r0 available space in octets -2/4, clipped to zero
//trashed r1, r2, r3, B0
// (still trashes r3 because of get_*_address_ex)
// only supports 16-bit unpacked/32-bit packed modes
.MODULE $M.cbuffer.calc_amount_space_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.calc_amount_space_ex:
$_cbuffer_calc_amount_space_ex:
    LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($cbuffer.calc_amount_space_ex.PATCH_ID_0, r1)
    pushm <r4, r5, r6, r7, rLink>;

    BUFFER_GET_USABLE_OCTETS(r6, r0);
    BUFFER_GET_SIZE_IN_ADDRS_ASM(r7, r0);
    
    push r0;
    call $cbuffer.get_write_address_ex;
    r4 = r0;
    r5 = r1;

    // check if the buffer is part of an in-place chain.
    pop r0;
    r1 = M[r0 + $cbuffer.DESCRIPTOR_FIELD];
    Null = r1 AND $cbuffer.IN_PLACE_MASK;
    if Z jump get_rd_addr;
        // its_in_place
        r2 = M[r0 + $cbuffer.AUX_ADDR_FIELD];
        if NZ r0 = r2;  // use the rdp of the head of the in-place chain
    get_rd_addr:
    call $cbuffer.get_read_address_ex;
    r2 = r7;
    r0 = r0 - r4;               // space = rdp - wrp
    if Z jump rd_equals_wr;
        if NEG r0 = r0 + r2;

        Null = r6;
        if Z jump account_for_offsets;
            r0 = r0 LSHIFT -1;
        account_for_offsets:
        r0 = r0 + r1;           // add read octets
        r0 = r0 - r5;           // subtract written octets
        jump end;

    rd_equals_wr:
        r0 = r1 - r5;
        if GT r0 = -1;          /* Buffer_empty:
            This is expected only if the buffer has an MMU write/read handle.
            For normal software buffers there will always be a gap of at least
            2 octets.
            This is because how much we allow to write into the buffer is
            completely under the control of SW running on the DSP.
            For MMU buffers written to or read by hardware this gap can be
            just 1 octet. */
        Null = r6;
        if Z jump add_octets;
            r2 = r2 LSHIFT -1;
        add_octets:
        r0 = r0 + r2;

    end:
    Null = r6;
    if Z r6 = ADDR_PER_WORD;
    // space in octets -2/4, clipped to zero
    r0 = r0 - r6;
    if NEG r0 = 0;
    // When the buffer has an MMU read or write handle
    // it is possible that at this point r0 is zero. See note
    // in cbuffer.calc_amount_data_ex. Therefore we need to clip
    // the return value to zero so we don't return a negative value, ie: -1.


    popm <r4, r5, r6, r7, rLink>;
    rts;
.ENDMODULE;

//extern void cbuffer_set_read_address_ex(tCbuffer * cbuffer, unsigned int * ra, unsigned ro);
//in r0 cbuffer address
//in r1 true read address
//in r2 read octet offset (caller responsibility to restrict to valid range, e.g. 0...3 for 32-bit)
//trashed r3, (r10 if mmu buffer)
.MODULE $M.cbuffer.set_read_address_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.set_read_address_ex:
$_cbuffer_set_read_address_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer.set_read_address_ex.PATCH_ID_0)
    Null = 3 - r2;
    if NEG call $error;     // within-word offset must be in range 0...3

    r3 = M[r0 + $cbuffer.DESCRIPTOR_FIELD];
    Null = r3 AND $cbuffer.BUFFER_TYPE_MASK;
    if Z jump its_a_sw_wr_ptr;
        Null = r3 AND $cbuffer.READ_PTR_TYPE_MASK;
        if Z jump its_a_sw_wr_ptr;
            r3 = M[r0 + $cbuffer.START_ADDR_FIELD];
            r1 = r1 - r3;
            r0 = M[r0 + $cbuffer.READ_ADDR_FIELD];
            jump $mmu.set_byte_offset_from_address_ex;

    its_a_sw_wr_ptr:
    r1 = r1 OR r2;
    M[r0 + $cbuffer.READ_ADDR_FIELD] = r1;
    rts;
.ENDMODULE;

//extern void cbuffer_set_write_address_ex(tCbuffer * cbuffer, unsigned int * wa, unsigned wo);
//in r0 cbuffer address
//in r1 true write address
//in r2 write octet offset (caller responsibility to restrict to valid range, e.g. 0...3 for 32-bit)
//trashed r3, B0, (r10 if mmu buffer)
.MODULE $M.cbuffer.set_write_address_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.set_write_address_ex:
$_cbuffer_set_write_address_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer.set_write_address_ex.PATCH_ID_0)
    Null = 3 - r2;
    if NEG call $error;     // within-word offset must be in range 0...3

    r3 = M[r0 + $cbuffer.DESCRIPTOR_FIELD];
    Null = r3 AND $cbuffer.BUFFER_TYPE_MASK;
    if Z jump its_a_sw_wr_ptr;
        Null = r3 AND $cbuffer.WRITE_PTR_TYPE_MASK;
        if Z jump its_a_sw_wr_ptr;
            r3 = M[r0 + $cbuffer.START_ADDR_FIELD];
            r1 = r1 - r3;
            r0 = M[r0 + $cbuffer.WRITE_ADDR_FIELD];
            jump $mmu.set_byte_offset_from_address_ex;

    its_a_sw_wr_ptr:
    Null = r2 AND BUFFER_EX_OFFSET_MASK;
    if Z jump no_round_up;
        push r4;
        BUFFER_GET_SIZE_IN_ADDRS_ASM(r4, r0);
        r3 = M[r0 + $cbuffer.START_ADDR_FIELD];
        r1 = r1 + ADDR_PER_WORD;
        r3 = r3 + r4;
        Null = r3 - r1;
        if LE r1 = r1 - r4;    // wrap if we went past the end of buffer
        r1 = r1 OR r2;
        pop r4;
    no_round_up:
    M[r0 + $cbuffer.WRITE_ADDR_FIELD] = r1;
    rts;
.ENDMODULE;

//extern void cbuffer_advance_write_ptr_ex(tCbuffer * cbuffer, unsigned num_octets);
//in r0 cbuffer address
//in r1 octets to advance by
//trashed r1, r2, r3, B0, (r10 if mmu buffer)
//only supports 16-bit unpacked/32-bit packed modes
.MODULE $M.cbuffer_advance_write_ptr_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.advance_write_ptr_ex:
$_cbuffer_advance_write_ptr_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer.advance_write_ptr_ex.PATCH_ID_0)
    push rLink;
    push M0;
    push r4;
    pushm <I0, L0>;
    pushm <r0, r1>;

    BUFFER_GET_USABLE_OCTETS(r4, r0);   // usable_octets
    if Z r4 = ADDR_PER_WORD;

    call $cbuffer.get_write_address_ex;
    L0 = r2;

    push r3;
    pop B0;
    I0 = r0;
    r2 = r1;            // existing octet offset
    popm <r0, r1>;      // get cbuffer address and octets to advance by
    r1 = r1 + r2;       // add existing octet offset and octets to advance by
    r3 = -1;
    r2 = r1 LSHIFT r3;  // get samples (for 16-bit unpacked)
    Null = ADDR_PER_WORD - r4;
    if NZ jump prepare_offset_index;
        r2 = r2 LSHIFT r3;      // further divide by 2 for 32-bit packed

    prepare_offset_index:
    r2 = r2 ASHIFT LOG2_ADDR_PER_WORD;  // *4 to restore the initial write ptr without offset
    M0 = r2;
    r4 = r4 + r3;    // mask for offset: max 1 for 16-bit and max 3 for 32-bit

    r2 = r1 AND r4; // get the offset
    r1 = M[I0, M0];

    r1 = I0;
    call $cbuffer.set_write_address_ex;
    popm <I0, L0>;
    pop r4;
    pop M0;
    pop rLink;
    rts;
.ENDMODULE;

//extern void cbuffer_advance_read_ptr_ex(tCbuffer * cbuffer, unsigned num_octets);
//in r0 cbuffer address
//in r1 octets to advance by
//trashed r1, r2, r3, (r10 if mmu buffer)
//only supports 16-bit unpacked/32-bit packed modes
.MODULE $M.cbuffer_advance_read_ptr_ex;
   .CODESEGMENT BUFFER_PM;
$cbuffer.advance_read_ptr_ex:
$_cbuffer_advance_read_ptr_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer.advance_read_ptr_ex.PATCH_ID_0)
    push rLink;
    push r4;
    push M0;
    pushm <I0, L0>;
    pushm <r0, r1>;

    BUFFER_GET_USABLE_OCTETS(r4, r0);   // usable_octets
    if Z r4 = ADDR_PER_WORD;

    call $cbuffer.get_read_address_ex;
    L0 = r2;

    push r3;
    pop B0;
    I0 = r0;
    r2 = r1;            // existing octet offset
    popm <r0, r1>;      // get cbuffer address and octets to advance by
    r1 = r1 + r2;       // add existing octet offset and octets to advance by
    r3 = -1;
    r2 = r4 + r3;
    r2 = r1 AND r2;

    r1 = r1 LSHIFT r3;
    Null = ADDR_PER_WORD - r4;
    if NZ jump set_mod_reg;
        r1 = r1 LSHIFT r3;

    set_mod_reg:
    r1 = r1 ASHIFT LOG2_ADDR_PER_WORD;
    M0 = r1;
    r1 = M[I0, M0];
    r1 = I0;
    call $cbuffer.set_read_address_ex;
    popm <I0, L0>;
    pop M0;
    pop r4;
    pop rLink;
    rts;
.ENDMODULE;

// --------- copy_aligned 16-bit ----------
//void cbuffer_copy_aligned_16bit_be_zero_shift_ex(tCbuffer * dst, tCbuffer *src, unsigned num_octets);
// in r0 destination cbuffer address
// in r1 source cbuffer address
// in r2 number of octets to copy (non-zero)
// trashed r3, r10, B0, B4
.MODULE $M.cbuffer_copy_aligned_16bit_be_zero_shift_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.copy_aligned_16bit_be_zero_shift_ex:
$_cbuffer_copy_aligned_16bit_be_zero_shift_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer.copy_aligned_16bit_be_zero_shift_ex.PATCH_ID_0)
    // save the input paramerters for later
    pushm <FP(=SP), r0, r1, r2, r5, rLink>;
    pushm <I0, I4, L0, L4>;

    // get dest buffer true write address and size
    call $cbuffer.get_write_address_ex;
    I4 = r0;
    L4 = r2;
    push r3;
    pop B4;
    r5 = r3;        // prepare for checking for in-place

    // get src buffer read address and size
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    // get the read address and size
    call $cbuffer.get_read_address_ex;
    I0 = r0;
    L0 = r2;

    // check if cbuffer base addresses are the same
    Null = r5 - r3;
    if NZ jump not_in_place_copy;
        // only advance pointers, no copy for in-place
        r5 = M[FP + 3*ADDR_PER_WORD];            // copy amount
        r0 = M[FP + 1*ADDR_PER_WORD];            // cbuffer_dest
        r1 = r5;
        call $cbuffer.advance_write_ptr_ex;

        r0 = M[FP + 2*ADDR_PER_WORD];            // cbuffer_src
        r1 = r5;
        call $cbuffer.advance_read_ptr_ex;

        jump cp_pop_and_exit;

    not_in_place_copy:
    // init base for src ahead of doloop
    push r3;
    pop B0;

    r5 =  M[FP + 3*ADDR_PER_WORD];               // copy amount
    Null = r1;                                   // r1 is the octet offset

    if Z jump no_offset;
        // read the second part of the word, combine it with the first one.
        r1 = M[I0, ADDR_PER_WORD];
        r1 = r1 AND 0xff;                        // The new octet is the LSB.
        r2 = M[I4, 0];                           // Read the msb
        r2 = r2 AND 0xff00;                      // Mask the lsb
        r1 = r1 + r2;                            // combine it
        M[I4, ADDR_PER_WORD] = r1;               // write it to the buffer
        r5 = r5 - 1;                             // one octet already written, decrement
                                                 // the copy amount
    no_offset:
    r10 = r5 LSHIFT -1;                          // convert the copy amount to words
    if Z jump copy_aligned_write_last_octet;     // could be that only one octet we need to copy.
        r10 = r10 - 1;                    // decrement the amount due to the initial read and last write
        r1 = M[I0,ADDR_PER_WORD];         // initial read
        do copy_aligned_loop;
            r1 = M[I0,ADDR_PER_WORD],
             M[I4,ADDR_PER_WORD] = r1;    // read and write
        copy_aligned_loop:
        M[I4,ADDR_PER_WORD] = r1;         // last write

    copy_aligned_write_last_octet:

    Null = r5 AND 0x1;
    if Z jump copy_aligned_done;
        //read last word.
        r1 = M[I0, 0];
        r1 = r1 AND 0xff00;
        r2 = M[I4,0];
        r2 = r2 AND 0xff;
        r1 = r1 + r2;
        M[I4,0] = r1;

    copy_aligned_done:
    // Update the write address
    r0 = M[FP + 1*ADDR_PER_WORD];                // cbuffer_dest
    r1 = I4;
    r2 = r5 AND IS_ODD_OFFSET_MASK;
    call $cbuffer.set_write_address_ex;

    // Update the read address
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    r1 = I0;
    call $cbuffer.set_read_address_ex;

    cp_pop_and_exit:
    // Restore index & length registers
    popm <I0, I4, L0, L4>;
    popm <FP, r0, r1, r2, r5, rLink>;
    rts;
.ENDMODULE;

// --------- copy_unaligned 16-bit ----------
//void cbuffer_copy_unaligned_16bit_be_zero_shift_ex(tCbuffer * dst, tCbuffer *src, unsigned num_octets);
// in r0 destination cbuffer address
// in r1 source cbuffer address
// in r2 number of octets to copy (non-zero TODO at the moment even r2=1 would panic)
// trashed r3, r10, B0, B4
.MODULE $M.cbuffer_copy_unaligned_16bit_be_zero_shift_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.copy_unaligned_16bit_be_zero_shift_ex:
$_cbuffer_copy_unaligned_16bit_be_zero_shift_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer.copy_unaligned_16bit_be_zero_shift_ex.PATCH_ID_0)
    // save the input paramerters for later
    pushm <FP(=SP), r0, r1, r2, r5, r6, r8, rLink>;
    pushm <I0, I4, L0, L4>;

    // get dest buffer true write address and size
    call $cbuffer.get_write_address_ex;
    I4 = r0;
    L4 = r2;
    r6 = r1;        // save write octet offset
    push r3;
    pop B4;
    r5 = r3;        // prepare for checking for in-place

    // get src buffer read address and size
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    // get the read address and size
    call $cbuffer.get_read_address_ex;
    I0 = r0;
    L0 = r2;

    // check if cbuffer base addresses are the same
    Null = r5 - r3;
    if NZ jump not_in_place_copy;
        // only advance pointers, no copy for in-place
        r5 = M[FP + 3*ADDR_PER_WORD];            // copy amount
        r0 = M[FP + 1*ADDR_PER_WORD];            // cbuffer_dest
        r1 = r5;
        call $cbuffer.advance_write_ptr_ex;

        r0 = M[FP + 2*ADDR_PER_WORD];            // cbuffer_src
        r1 = r5;
        call $cbuffer.advance_read_ptr_ex;

        jump cp_pop_and_exit;

    not_in_place_copy:
    // init base for src ahead of doloop
    push r3;
    pop B0;

    r3 = 0xFF;                         // mask for keeping LSByte
    r8 = 8;                            // shift value for faster access
    push r1;                           // save read octet offset
    r5 =  M[FP + 3*ADDR_PER_WORD];     // copy amount
    Null = r1;                         // read octet offset
    if Z jump check_wr_offset;
        // unaligned: rd offset
        Null = r6;                     // check write octet offset
        if NZ call $error;  // same_offset should be captured by the caller
                            // TODO grace fail, return octets_copied and make the caller check
        // unaligned, different offsets, rd offset (no wr offset)
        r1 = M[I0, ADDR_PER_WORD];      // read word to be copied
        r1 = r1 AND r3;                 // LSByte only to be mixed
        r5 = r5 - 2;                    // assume two octets will be written
        if NEG jump less_than_2;        // jump if only one octet to be copied
            r1 = r1 LSHIFT r8,               // on the MSByte position
             r0 = M[I0, ADDR_PER_WORD];      // read in advance next word to be copied
            r2 = r0 AND r3;                 // keep LSByte to be mixed to next word
            r0 = r0 LSHIFT -8;              // on the LSByte position
            r1 = r1 OR r0;                  // mix it
            r2 = r2 LSHIFT r8,              // value to be mixed next on the MSByte position
             M[I4, ADDR_PER_WORD] = r1,      // write a whole word back
             r0 = M[I0, ADDR_PER_WORD];      // read next word to be copied
            // keep in mind src buffer (rdp) runs ahead one word
            jump copy_unaligned_loop;

        less_than_2:
            // corner case: only one octet to copy
            r1 = r1 LSHIFT r8,               // on the MSByte position
             r0 = M[I4, 0];                   // read what's already there
            r0 = r0 AND r3;                  // keep only LSByte
            r1 = r1 OR r0;                   // mix it
            M[I4, 0] = r1;                   // write a whole word back, although only part changed (so no dst++)
            jump upd_ptrs;

    check_wr_offset:
        // no rd offset
        Null = r6;                           // write octet offset
        if Z call $error;   // same_offset should be captured by the caller TODO
            // unaligned, different offsets, no rd but wr
            r1 = M[I0, ADDR_PER_WORD],       // read word to be copied
             r0 = M[I4, 0];                   // read what's currently written
            r2 = r1 AND r3;                  // keep LSByte to be mixed to next word
            r0 = r0 AND 0xFF00;              // mask out the LSByte of what's there
            r1 = r1 LSHIFT -8;               // octet to be written on LSByte position
            r1 = r1 OR r0;                   // mix it
            r2 = r2 LSHIFT r8,               // value to be mixed next on the MSByte position
             M[I4, ADDR_PER_WORD] = r1,       // write it back
             r0 = M[I0, ADDR_PER_WORD];       // read next word to be copied
            r5 = r5 - 1;                     // one octet already written

        copy_unaligned_loop:
        r10 = r5 LSHIFT -1;              // convert the copy amount to words
        if Z jump write_last_octet;      // could be that only one octet we need to copy.

        // next octet to be mixed in r2 on MSByte position
        do cp_unld_loop;
            r1 = r0 AND r3;              // keep LSByte to be mixed to next word
            r0 = r0 LSHIFT -8;           // octet to be written on LSByte position
            r0 = r0 OR r2;               // mix with octet stored in previous iteration
            r2 = r1 LSHIFT r8;            // on the MSByte position
            M[I4, ADDR_PER_WORD] = r0,   // write a whole word back
             r0 = M[I0, ADDR_PER_WORD];  // read next word to be copied
        cp_unld_loop:

    write_last_octet:
        r1 = M[I0, -ADDR_PER_WORD]; // adjust back src buffer incremented in the last loop

    Null = r5 AND IS_ODD_OFFSET_MASK;         // check if doloop has copied even number of octets
    if Z jump adj_src_ptr;
        r0 = M[I4, 0];          // read what's currently written
        // mix with LSByte stored in last doloop iteration
        r0 = r0 AND r3;         // keep only LSByte of what's there
        r0 = r0 OR r2;          // mix
        M[I4,0] = r0;           // and write a whole word back
        jump upd_ptrs;

    adj_src_ptr:
        r1 = M[I0, -ADDR_PER_WORD];   // adjust back src buffer when rdp was running ahead

    upd_ptrs:
    // Update the write address
    r0 = M[FP + 1*ADDR_PER_WORD];                // cbuffer_dest
    r1 = I4;
    r5 = M[FP + 3*ADDR_PER_WORD];                // copy amount
    r2 = r5 XOR r6;     // new offset is a combination of old one and amt_copied
    r2 = r2 AND IS_ODD_OFFSET_MASK;   // play safe, only lsb
    call $cbuffer.set_write_address_ex;
    // r2 is not trashed

    // Update the read address
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    r1 = I0;
    pop r2;             // restore read offset
    r2 = r2 XOR r5;     // new offset is a combination of old one and amt_copied
    r2 = r2 AND IS_ODD_OFFSET_MASK;   // play safe, only lsb
    call $cbuffer.set_read_address_ex;

    cp_pop_and_exit:
    // Restore index & length registers
    popm <I0, I4, L0, L4>;
    popm <FP, r0, r1, r2, r5, r6, r8, rLink>;
    rts;

.ENDMODULE;

// --------- copy_aligned 32-bit ----------
//void cbuffer_copy_aligned_32bit_be_ex(tCbuffer * dst, tCbuffer *src, unsigned num_octets);
// in r0 destination cbuffer address
// in r1 source cbuffer address
// in r2 number of octets to copy (non-zero)
// trashed r3, r10, B0, B4
.MODULE $M.cbuffer_copy_aligned_32bit_be_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.copy_aligned_32bit_be_ex:
$_cbuffer_copy_aligned_32bit_be_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer.copy_aligned_32bit_be_ex.PATCH_ID_0)
    // save the input paramerters for later
    pushm <FP(=SP), r0, r1, r2, r5, r6, r7, r8, r9, rLink>;
    pushm <I0, I4, L0, L4>;

    BUFFER_GET_USABLE_OCTETS(r8, r0);
    if NZ call $error;      // usable octets must be 4

    r8 = ADDR_PER_WORD;     // usable_octets
    r9 = 0xFFFFFFFF;

    call $cbuffer.get_write_address_ex;
    I4 = r0;
    L4 = r2;
    push r3;
    pop B4;
    r5 = r3;        // prepare for checking for in-place

    // get src buffer read address and size
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    // get the read address and size
    call $cbuffer.get_read_address_ex;
    I0 = r0;
    L0 = r2;

    // check if cbuffer base addresses are the same
    Null = r5 - r3;
    if NZ jump not_in_place_copy;
        // only advance pointers, no copy for in-place
        r5 = M[FP + 3*ADDR_PER_WORD];            // copy amount
        r0 = M[FP + 1*ADDR_PER_WORD];            // cbuffer_dest
        r1 = r5;
        call $cbuffer.advance_write_ptr_ex;

        r0 = M[FP + 2*ADDR_PER_WORD];            // cbuffer_src
        r1 = r5;
        call $cbuffer.advance_read_ptr_ex;

        jump cp_pop_and_exit;

    not_in_place_copy:
    // init base for src ahead of doloop
    push r3;
    pop B0;

    r5 =  M[FP + 3*ADDR_PER_WORD];               // copy amount
    Null = r1;                                   // r1 is the octet offset

    if Z jump no_offset;
        r6 = r8 - r1;
        Null = r5 - r6;     // check if amount of data is smaller than the space in the current word
        if POS jump amount_data_greater;
            // less data to be copied than space
            r6 = r8 - r5;
            r6 = r6 LSHIFT 3;
            r6 = r9 LSHIFT r6;      // mask for source in the LSB

            r7 = r1 LSHIFT 3;
            r7 = -r7;
            r6 = r6 LSHIFT r7;      // mask source in the right position

            r2 = M[I0, 0];          // read the source word
            r2 = r2 AND r6;

            r3 = M[I4, 0];          // Read the dest
            r6 = r6 XOR r9;         // invert r6
            r3 = r3 AND r6;         // zero only the bits that need to be copied

            r2 = r3 + r2;

            M[I4, 0] = r2;

            r7 = r1 + r5;
            Null = ADDR_PER_WORD - r7;
            if NZ jump copy_aligned_done;
                r7 = 0;
                r3 = M[I4, ADDR_PER_WORD];
                r3 = M[I0, ADDR_PER_WORD];
                jump copy_aligned_done;

        amount_data_greater:
        r5 = r5 - r6;               // decrement the copy amount
        r6 = r1 LSHIFT 3;
        r6 = -r6;
        r6 = r9 LSHIFT r6;          // mask for source

        r7 = r8 - r1;
        r7 = r7 LSHIFT 3;
        r7 = r9 LSHIFT r7;          // mask for destination

        // read the second part of the word, combine it with the first one.
        r2 = M[I0, ADDR_PER_WORD];
        r2 = r2 AND r6;                        // The new octets are in the LSBs.

        r1 = M[I4, 0];                         // Read the msb
        r1 = r1 AND r7;                        // Mask the lsb
        r1 = r1 + r2;                          // combine it
        M[I4, ADDR_PER_WORD] = r1;             // write it to the buffer

    no_offset:
    r7 = r5 AND IS_OCTET_OFFSET_MASK;

    r10 = r5 LSHIFT -LOG2_ADDR_PER_WORD;         // convert the copy amount to words
    if Z jump copy_aligned_write_last_octet;     // maybe the copy amount is less than a word
        r10 = r10 - 1;                    // decrement the amount due to the initial read and last write
        r1 = M[I0, ADDR_PER_WORD];         // initial read
        do copy_aligned_loop;
            r1 = M[I0, ADDR_PER_WORD],
             M[I4, ADDR_PER_WORD] = r1;    // read and write
        copy_aligned_loop:
        M[I4, ADDR_PER_WORD] = r1;         // last write

    copy_aligned_write_last_octet:

    Null = r7;
    if Z jump copy_aligned_done;
        r6 = r8 - r7;           // us_octets - amount_data left
        r6 = r6 LSHIFT 3;
        r6 = r9 LSHIFT r6;      // mask for source

        r8 = r7 LSHIFT 3;
        r8 = -r8;
        r8 = r9 LSHIFT r8;      // mask for destination

        r1 = M[I0, 0];
        r1 = r1 AND r6;
        r2 = M[I4, 0];
        r2 = r2 AND r8;

        r1 = r1 + r2;
        M[I4, 0] = r1;

    copy_aligned_done:
    // Update the write address
    r0 = M[FP + 1*ADDR_PER_WORD];                // cbuffer_dest
    r1 = I4;
    r2 = r7;
    call $cbuffer.set_write_address_ex;

    // Update the read address
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    r1 = I0;
    call $cbuffer.set_read_address_ex;

    cp_pop_and_exit:
    // Restore index & length registers
    popm <I0, I4, L0, L4>;
    popm <FP, r0, r1, r2, r5, r6, r7, r8, r9, rLink>;
    rts;
.ENDMODULE;

// --------- copy_unaligned 32-bit ----------
//void cbuffer_copy_unaligned_32bit_be_ex(tCbuffer * dst, tCbuffer *src, unsigned num_octets);
// in r0 destination cbuffer address
// in r1 source cbuffer address
// in r2 number of octets to copy (non-zero)
// trashed r3, r10, B0, B4
.MODULE $M.cbuffer_copy_unaligned_32bit_be_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.copy_unaligned_32bit_be_ex:
$_cbuffer_copy_unaligned_32bit_be_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer.copy_unaligned_32bit_be_ex.PATCH_ID_0)
    // save the input paramerters for later
    pushm <FP(=SP), r0, r1, r2, r4, r5, r6, r7, r8, rLink>;
    pushm <I0, I4, L0, L4>;

    BUFFER_GET_USABLE_OCTETS(r4, r0);
    if NZ call $error;
    r4 = ADDR_PER_WORD;

    // get dest buffer true write address and size
    call $cbuffer.get_write_address_ex;
    I4 = r0;
    L4 = r2;
    r6 = r1;        // save write octet offset
    push r3;
    pop B4;
    r5 = r3;        // prepare for checking for in-place

    // get src buffer read address and size
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    // get the read address and size
    call $cbuffer.get_read_address_ex;
    I0 = r0;
    L0 = r2;

    // check if cbuffer base addresses are the same
    Null = r5 - r3;
    if NZ jump not_in_place_copy;
        // only advance pointers, no copy for in-place
        r5 = M[FP + 3*ADDR_PER_WORD];            // copy amount
        r0 = M[FP + 1*ADDR_PER_WORD];            // cbuffer_dest
        r1 = r5;
        call $cbuffer.advance_write_ptr_ex;

        r0 = M[FP + 2*ADDR_PER_WORD];            // cbuffer_src
        r1 = r5;
        call $cbuffer.advance_read_ptr_ex;

        jump cp_pop_and_exit;

    not_in_place_copy:
    // init base for src ahead of doloop
    push r3;
    pop B0;
    //push r1;                            // save read octet offset
    r7 = 0xFF; // mask for destination
    r8 = 0xFFFFFFFF;
    r10 = M[FP + 3*ADDR_PER_WORD];        // amount of data to be copied
    do copy_loop;

        r2 = r4 - r1;                     // source
        r2 = r2 - 1;
        r2 = r2 LSHIFT 3;
        r2 = r7 LSHIFT r2;
        r5 = M[I0, 0];
        r5 = r5 AND r2;                   // octet read

        r2 = r1 LSHIFT 3;
        r5 = r5 LSHIFT r2;                // move octet read into the LSB - shift right

        r3 = r6 LSHIFT 3;
        r3 = -r3;
        r5 = r5 LSHIFT r3;                //source octet in the right position for source

        r3 = r4 - r6;                     // us_octets dest
        r3 = r3 - 1;
        r3 = r3 LSHIFT 3;
        r3 = r7 LSHIFT r3;

        r3 = r3 XOR r8;                   // mask for source

        r2 = M[I4, 0];
        r2 = r2 AND r3;                   // protect the bits that aren't to be changed

        r2 = r2 + r5;

        M[I4, 0] = r2;

        r1 = r1 + 1;
        Null = r4 - r1;
        if NZ jump check_dest_offset;
            r1 = 0;
            r3 = M[I0, ADDR_PER_WORD];

        check_dest_offset:
        r6 = r6 + 1;
        Null = r4 - r6;
        if NZ jump loop_end;
            r6 = 0;
            r3 = M[I4, ADDR_PER_WORD];

        loop_end:
        nop;
    copy_loop:

    upd_ptrs:
    r5 = r1;    // save final offset source
    // Update the write address
    r0 = M[FP + 1*ADDR_PER_WORD];                // cbuffer_dest
    r2 = r6;
    r1 = I4;
    call $cbuffer.set_write_address_ex;
    // r2 is not trashed

    // Update the read address
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    r1 = I0;
    r2 = r5;
    call $cbuffer.set_read_address_ex;

    cp_pop_and_exit:
    // Restore index & length registers
    popm <I0, I4, L0, L4>;
    popm <FP, r0, r1, r2, r4, r5, r6, r7, r8, rLink>;
    rts;

.ENDMODULE;


// --------- pack_aligned_ex ----------
//void cbufer_pack_aligned_ex(tCbuffer *dst, tCbuffer *src, unsigned num_octets);
// in r0 destination cbuffer address
// in r1 source cbuffer address
// in r2 number of octets to copy (non-zero)
// trashed r3, r10, B0, B4
// NB. "aligned" for pack/unpack is less straight-forward to define given
//       the src and dst different data sizes; a better name would be
//       sync or "in-phase"
// TODO for the time being at least, we assume most use cases will be in-phase
//       the "unaligned" version( i.e. src and dst offsets are not in-phase)
//       is still slower, not taking advantage of a whole-word processing doloop
// NB. The name doesn't mention _be_ for big-endian as the simple copy _ex do;
//     The pack/unpack functions still implement BE as the only use case we care about at this time.
.MODULE $M.cbuffer_pack_aligned_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.pack_aligned_ex:
$_cbuffer_pack_aligned_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer.pack_aligned_ex.PATCH_ID_0)
    // save the input paramerters for later
    pushm <FP(=SP), r0, r1, r2, r4, r5, r6, rLink>;
    pushm <I0, I4, M0, L0, L4>;

    BUFFER_GET_USABLE_OCTETS(r3, r0);           // usable_octets destination
    if NZ call $error;

    BUFFER_GET_USABLE_OCTETS(r3, r1);           // usable_octets source
    Null = r3 - USE_16_BIT;
    if NZ call $error;

    M0 = ADDR_PER_WORD;

    // get dest buffer true write address and size
    call $cbuffer.get_write_address_ex;
    I4 = r0;
    L4 = r2;
    r6 = r1;            // preserve dst offset
    r4 = r1 AND IS_ODD_OFFSET_MASK;
    push r3;
    pop B4;
    r5 = r3;            // prepare for checking for in-place

    // get src buffer read address and size
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    // get the read address and size
    call $cbuffer.get_read_address_ex;
    I0 = r0;
    Null = r4 - r1;
    if NZ call $error;  // only in-phase should get here
    L0 = r2;

    // NB. within-word offsets in r4(src) and r6(dst) preserved to the end

    // check if cbuffer base addresses are the same
    Null = r5 - r3;
    if Z call $error;  // in-place not implemented

    // init base for src ahead of doloop
    push r3;
    pop B0;

    r5 = M[FP + 3*ADDR_PER_WORD];    // amount of data to copy in octets
    // pre-loaded masks and shifts
    r3 = 0xFFFF;
    r1 = 16;

    Null = r6 AND IS_ODD_OFFSET_MASK;
    if Z jump no_odd_offset;
        // aligned but with offset (src_off=1, dst_off = 1 or 3)
        r0 = M[I0, M0];      // read the next word to pack

        /* ARCH4 is little-endian
           what we need is big-endian (BE) so HW offsets need to be inverted
           HW offset: 3  2  1  0
           actual BE: 0  1  2  3
           Two cases here:
           - dst_off(r6)=1 : copy 3 octets pre-loop starting at HW offset 2 (towards offset 0)
           - dst_off(r6)=3 : copy 1 octet pre-loop starting at HW offset 0
         */
        r10 = r6 XOR IS_OCTET_OFFSET_MASK;      // dst offset inverted
        Null = r10 AND IS_PACKED_MASK;
        if Z jump copy_1_pre_loop;
            // dst_offset=1
            // assume most likely case: at least 3 octets to be copied
            r5 = r5 - 3;        // 3 octets written pre-loop
            if NEG jump less_than_3;    // corner case handled separately
            // octet orphan to go on LSB of the MS half-word
            r2 = r0 LSHIFT r1;
            r2 = r2 AND 0x00FF0000;
            push r4;
            r4 = M[I4, 0];      // read existing content
            r0 = r4 AND 0xFF000000; // mask for kept content
            pop r4;
            r2 = r2 OR r0;      // mix the existing content with the orphan
            // continue unpacking a half-word_orphan: will go on LS half-word
            r0 = M[I0, M0];     // read the next word to pack
            r0 = r0 AND r3;     // DO NOT assume MS halfw is zeroed
            r0 = r0 OR r2;      // mix them
            M[I4, M0] = r0;
            jump efficient_loop;

        copy_1_pre_loop:
            r2 = r0 AND 0xFF;
            r0 = M[I4, 0];      // read existing content
            r0 = r0 AND 0xFFFFFF00; // mask for kept content
            r2 = r2 OR r0;      // mix the existing content with the orphan
            M[I4, M0] = r2;
            r5 = r5 - 1;        // 1 octet written pre-loop
            if POS jump efficient_loop;
                call $error;    // octets to copy MUST be non-zero

        less_than_3:
            // nr octets to copy was decremented by 3
            r2 = M[I4, 0];      // read existing content
            r5 = r5 + 1;
            if NEG jump just_one;
                // 2 octets to copy
                r2 = r2 AND 0xFF0000FF; // mask for kept content
                r0 = r0 LSHIFT r1;      // first src half-word on MS
                r0 = r0 AND 0x00FF0000; // keep only 1 octet from first src word
                r2 = r2 OR r0,          // mix first octet
                 r0 = M[I0, 0];         // read the next word to pack, no src++
                r0 = r0 AND 0xFF00;     // keep only 1 octet from second src word
                jump lt3_mix_wr_back;
            just_one:
                // only 1 octet to copy
                r2 = r2 AND 0xFF00FFFF; // mask for kept content
                r0 = r0 LSHIFT r1;      // bring octet to be copied in position
                r0 = r0 AND 0x00FF0000;
            lt3_mix_wr_back:
                r2 = r2 OR r0;          // mix with data to copy
                M[I4, 0] = r2;          // pre-loop orphan, no dst index_reg++
                jump upd_ptrs;

        pack_incompl_halfw:
            r2 = r2 AND 0xFFFF00FF; // keep all except MS of the lower halfw
            r0 = r0 AND 0xFF00;     // src single octet
            r0 = r0 OR r2,          // mix them
             r2 = M[I0, -ADDR_PER_WORD]; // unwind src index reg
            M[I4, 0] = r0;          // and write back, incompl so no src++
            jump upd_ptrs;


    no_odd_offset:
    Null = r6;
    if Z jump efficient_loop;  // jump if dst_word_aligned
        // the previous write left the dst offset mid-word
        // we first get word aligned to set up an efficient packing loop
        r0 = M[I0, M0];         // read src to pack
        r2 = M[I4, 0];          // read what is already there at dst
        r5 = r5 - 2;            // mark 2 octets written pre-loop
        // ... but only if there are at least two octets to be written
        if NEG jump pack_incompl_halfw;
            r2 = r2 AND 0xFFFF0000; // keep only MS
            r0 = r0 AND r3;         // src half-word on LS, DO NOT rely on unpacked MS halfw always zeroed
            r0 = r0 OR r2;          // mix them
            M[I4, M0] = r0;         // and write back

    efficient_loop:
    // loop count is in dst words to pack
    r10 = r5 ASHIFT -LOG2_ADDR_PER_WORD;
    if NEG call $error;         // a doloop would take a loooong time

    // r3 = 0xFFFF; LS half-word mask
    // r1 = 16; shift left to get MS half-word
    r0 = M[I0, M0];             // read first unpacked half-word
    do pack_loop;
        r2 = r0 LSHIFT r1,
         r0 = M[I0, M0];        // read second unpacked half-word
        r0 = r0 AND r3;         // DO NOT assume MS halfw is zeroed (*)
        r0 = r0 OR r2;          // mix them
        M[I4, M0] = r0,         // write packed word
         r0 = M[I0, M0];        // and read next unpacked half-word
    pack_loop:
    r0 = M[I0, -ADDR_PER_WORD]; // unwind index reg that went too far in the optimised loop
    // (*) Re: "DO NOT..." we could make it faster by not doing this AND operation
    //      if we didn't care about WBS; see B-205518;
    //  also might reconsider for aura+ and future chips that do not suffer from this problem.

    // post-loop: if any leftovers pack it
    Null = r5 AND IS_OCTET_OFFSET_MASK;
    if Z jump upd_ptrs;
        r0 = M[I4, 0];         // read existing content
        Null = r5 AND IS_PACKED_MASK;
        if Z jump octet_orphan;
            // half-word_orphan:
            r2 = M[I0, M0];     // read the next word to pack
            r2 = r2 LSHIFT r1;  // src on MS half-word
            r10 = r0 AND r3;    // keep LS (dst)
            r0 = r2 OR r10;     // mix the existing content with the hw orphan
            Null = r5 AND IS_ODD_OFFSET_MASK;
            if Z jump wr_upd_ptrs;  // no more orphans
                // continue with an octet orphan on MS
                r2 = M[I0, 0];      // read the next word, pack to LS 16-bit
                                    // post-loop orphan, no src index_reg++
                r2 = r2 AND 0xFF00;
                r0 = r0 AND 0xFFFF00FF;   // mask for kept content
                jump mix_wr;
        octet_orphan:
                r2 = M[I0, 0];      // read the next word, pack to LS 16-bit
                                    // post-loop orphan, no src index_reg++
                r2 = r2 LSHIFT r1;  // on MS half-word
                r2 = r2 AND 0xFF000000;
                r0 = r0 AND 0x00FFFFFF;   // mask for kept content
            mix_wr:
                r0 = r0 OR r2;      // mix the existing content with the orphan
        wr_upd_ptrs:
            M[I4, 0] = r0;          // post-loop orphan, no dst index_reg++

    upd_ptrs:
    // Update the write address
    r5 = M[FP + 3*ADDR_PER_WORD];                // reload nr octets to process
    r0 = M[FP + 1*ADDR_PER_WORD];                // cbuffer_dest
    r1 = I4;
    // new offset is a combination of old one and amt_copied
    r2 = r5 + r6;
    r2 = r2 AND IS_OCTET_OFFSET_MASK;
    call $cbuffer.set_write_address_ex;

    // Update the read address
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    r1 = I0;
    r2 = r5 + r4;     // new offset is a combination of old one and amt_copied
    r2 = r2 AND IS_ODD_OFFSET_MASK; // src is unpacked, can't have offsets 2 and 3
    call $cbuffer.set_read_address_ex;

    // Restore index, modify & length registers
    popm <I0, I4, M0, L0, L4>;
    popm <FP, r0, r1, r2, r4, r5, r6, rLink>;
    rts;

.ENDMODULE;


// --------- pack_unaligned_ex ----------
//void cbufer_pack_unaligned_ex(tCbuffer *dst, tCbuffer *src, unsigned num_octets);
// in r0 destination cbuffer address
// in r1 source cbuffer address
// in r2 number of octets to copy (non-zero)
// trashed r3, r10, B0, B4
// NB. TODO this is only used for not "in-phase"
.MODULE $M.cbuffer_pack_unaligned_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.pack_unaligned_ex:
$_cbuffer_pack_unaligned_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer.pack_unaligned_ex.PATCH_ID_0)
    // save the input paramerters for later
    pushm <FP(=SP), r0, r1, r2, r4, r5, r6, r7, r8, r9, rLink>;
    pushm <I0, I4, L0, L4>;

    BUFFER_GET_USABLE_OCTETS(r9, r0);           // usable_octets destination
    if NZ call $error;
    r9 = ADDR_PER_WORD;

    BUFFER_GET_USABLE_OCTETS(r4, r1);           //source
    Null = r4 - USE_16_BIT;
    if NZ call $error;


    // get dest buffer true write address and size
    call $cbuffer.get_write_address_ex;
    I4 = r0;
    L4 = r2;
    r6 = r1;        // save write octet offset
    push r3;
    pop B4;
    r5 = r3;        // prepare for checking for in-place

    // get src buffer read address and size
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    // get the read address and size
    call $cbuffer.get_read_address_ex;
    r7 = r6 AND IS_ODD_OFFSET_MASK; // mask to keep only lsb of the packed side
    Null = r7 - r1;
    if Z call $error;   // should not be here for an aligned operation
    I0 = r0;
    L0 = r2;

    // check if cbuffer base addresses are the same
    Null = r5 - r3;
    if Z call $error;  // in-place not implemented

    // init base for src ahead of doloop
    push r3;
    pop B0;
    r7 = 0xFFFFFFFF;                // mask for destination
    r8 = r9 - r4;                   // us_octets_dest - us_octets_source
    r10 = M[FP + 3*ADDR_PER_WORD];  // amount of data to be copied
    do copy_loop;

        r2 = r4 - 1;           // source
        r2 = r2 - r1;
        r2 = r2 LSHIFT 3;
        r2 = 0xFF LSHIFT r2;
        r5 = M[I0, 0];
        r5 = r5 AND r2;        // octet read

        r2 = r8 + r1;
        r2 = r2 LSHIFT 3;
        r5 = r5 LSHIFT r2;     // move octet read into the LSB - shift left

        r3 = r6 LSHIFT 3;
        r3 = -r3;
        r5 = r5 LSHIFT r3;     //source octet in the right position for source


        r3 = r9 - r6;          // us_octets dest
        r3 = r3 - 1;
        r3 = r3 LSHIFT 3;
        r3 = 0xFF LSHIFT r3;   // mask for octets before

        r3 = r3 XOR r7;

        r2 = M[I4, 0];
        r2 = r2 AND r3;        // mask the source

        r2 = r2 + r5;          // combine octet read with the source word partially masked

        M[I4, 0] = r2;

        r1 = r1 + 1;
        Null = r4 - r1;
        if NZ jump check_dest_offset;
            r1 = 0;
            r3 = M[I0, ADDR_PER_WORD];

        check_dest_offset:
        r6 = r6 + 1;
        Null = r9 - r6;
        if NZ jump loop_end;
            r6 = 0;
            r3 = M[I4, ADDR_PER_WORD];

        loop_end:
        nop;
    copy_loop:

    upd_ptrs:
    r5 = r1;    // save final offset source
    // Update the write address
    r0 = M[FP + 1*ADDR_PER_WORD];                // cbuffer_dest
    r2 = r6;
    r1 = I4;
    call $cbuffer.set_write_address_ex;
    // r2 is not trashed

    // Update the read address
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    r1 = I0;
    r2 = r5;
    call $cbuffer.set_read_address_ex;

    cp_pop_and_exit:
    // Restore index & length registers
    popm <I0, I4, L0, L4>;
    popm <FP, r0, r1, r2, r4, r5, r6, r7, r8, r9, rLink>;
    rts;

.ENDMODULE;


// --------- unpack_aligned_ex ----------
//void cbufer_unpack_aligned_ex(tCbuffer *dst, tCbuffer *src, unsigned num_octets);
// in r0 destination cbuffer address
// in r1 source cbuffer address
// in r2 number of octets to copy (non-zero)
// trashed r3, r10, B0, B4
// NB. "aligned" for pack/unpack is less straight-forward to define given
//       the src and dst different data sizes; a better name would be
//       sync or "in-phase"
// TODO for the time being at least, we assume most use cases will be in-phase
//       the "unaligned" version( i.e. src and dst offsets are not in-phase)
//       is still slower, not taking advantage of a whole-word processing doloop
// NB. The name doesn't mention _be_ for big-endian as the simple copy _ex do;
//     The pack/unpack functions still implement BE as the only use case we care about at this time.
// Note. Unfortunately this function breaks with the common association of
//      src in I0 - dst in I4, here they are the other way around, for optimisation reasons.
.MODULE $M.cbuffer_unpack_aligned_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.unpack_aligned_ex:
$_cbuffer_unpack_aligned_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer.unpack_aligned_ex.PATCH_ID_0)
    // save the input paramerters for later
    pushm <FP(=SP), r0, r1, r2, r4, r5, r6, rLink>;
    pushm <I0, I4, M0, L0, L4>;

    BUFFER_GET_USABLE_OCTETS(r3, r0);           // destination
    Null = r3 - USE_16_BIT;
    if NZ call $error;

    BUFFER_GET_USABLE_OCTETS(r3, r1);           // usable_octets source
    if NZ call $error;

    M0 = ADDR_PER_WORD;

    // get dest buffer true write address and size
    call $cbuffer.get_write_address_ex;
    I0 = r0;
    L0 = r2;
    Null = r1 AND IS_PACKED_MASK;
    if NZ call $error;  // dst is packed
    r4 = r1;
    push r3;
    pop B0;
    r5 = r3;            // prepare for checking for in-place

    // get src buffer read address and size
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    // get the read address and size
    call $cbuffer.get_read_address_ex;
    I4 = r0;
    r6 = r1 AND IS_ODD_OFFSET_MASK;
    Null = r4 - r6;
    if NZ call $error;  // only in-phase should get here
    r6 = r1;            // preserve src offset
    L4 = r2;

    // check if cbuffer base addresses are the same
    Null = r5 - r3;
    if Z call $error;  // cannot do in-place, input data will be overwritten

    // init base for src ahead of doloop
    push r3;
    pop B4;

    r5 = M[FP + 3*ADDR_PER_WORD];    // amount of data to copy in octets
    // pre-loaded masks and shifts
    r3 = 0xFFFF;
    r1 = -16;

    Null = r6 AND IS_ODD_OFFSET_MASK;
    if Z jump no_odd_offset;
        // aligned but with offset (src_off=1 or 3, dst_off=1)
        r0 = M[I4, M0];      // read the next word, partial copy MSB only
        /* ARCH4 is little-endian
           what we need is big-endian (BE) so HW offsets need to be inverted
           HW offset: 3  2  1  0
           actual BE: 0  1  2  3
           Two cases here:
           - src_off(r6)=1 : copy 3 octets pre-loop, first an orphan at LSB then the half-word
           - src_off(r6)=3 : copy 1 octet pre-loop (at LSB)
         */
        r6 = r6 XOR IS_OCTET_OFFSET_MASK;      // src offset inverted
        // NB. r6 keeps a BE offset to update the src offset at the end (set_read_address_ex)
        Null = r6 AND IS_PACKED_MASK;
        if Z jump copy_1_pre_loop;
            // src_offset=1: copy 3 octets, first handle octet orphan
            r2 = r0 LSHIFT r1;  // on LS halfw
            r2 = r2 AND 0xFF;
            r4 = M[I0, 0];      // read existing content
            r4 = r4 AND 0xFF00; // mask for kept content
            r2 = r2 OR r4;      // mix the existing content with the orphan
            M[I0, M0] = r2,
             r5 = r5 - 1;        // first octet written pre-loop
            if Z jump pre_1_octet;
            // continue unpacking a half-word_orphan
            r5 = r5 - 2;        // (another) 2 octets written pre-loop
            // ... but only if there are at least two octets to be written
            if NEG jump incompl_halfw;
            r2 = r0 AND r3;
            M[I0, M0] = r2;
            jump efficient_loop;

            pre_1_octet:
                r2 = M[I4, -ADDR_PER_WORD]; // unwind src index reg
                jump upd_ptrs; // that was all (corner case pre-loop 1 octet)

            incompl_halfw:
                r2 = M[I4, -ADDR_PER_WORD]; // unwind src index reg
                r5 = r5 + 2;    // and adjust back nr octets written
                jump post_loop_octet_orphan;

        copy_1_pre_loop:
            r2 = r0 AND 0xFF;
            r0 = M[I0, 0];      // read existing content
            r0 = r0 AND 0xFF00; // mask for kept content
            r2 = r2 OR r0;      // mix the existing content with the orphan
            M[I0, M0] = r2,
             r5 = r5 - 1;        // 1 octet written pre-loop
            if POS jump efficient_loop;
                call $error;    // octets to copy MUST be non-zero

    // gets here with initial r6=src_offset (not XORed)
    no_odd_offset:
    Null = r6;
    if Z jump efficient_loop;  // jump if src_word_aligned
        // the previous read left the offset mid-word
        // src_halfword_aligned:
        // we first get word aligned to set up an efficient unpacking loop
        r0 = M[I4, M0];
        r0 = r0 AND r3;
        r5 = r5 - 2;            // 2 octets written pre-loop
        // ... but only if there are at least two octets to be written
        if NEG jump incompl_halfw;
        M[I0, M0] = r0;

    efficient_loop:
    // loop count is in words to unpack
    r10 = r5 ASHIFT -LOG2_ADDR_PER_WORD;
    if NEG call $error;         // a doloop would take a loooong time

    // r3 = 0xFFFF; LS half-word mask
    // r1 = -16; shift right to get LS half-word
    r2 = M[I4, M0];
    do unpack_loop;
        r0 = r2 LSHIFT r1;
        r0 = r2 AND r3,
         M[I0, M0] = r0;        // write first unpacked half-word
        M[I0, M0] = r0,         // write second unpacked half-word
         r2 = M[I4, M0];
    unpack_loop:
    r2 = M[I4, -ADDR_PER_WORD]; // unwind index reg that went too far in the optimised loop

    // post-loop: if any leftovers unpack it
    r6 = Null;              // assume no resulting dst offset (i.e. *all* 4 octets)
    Null = r5 AND IS_OCTET_OFFSET_MASK;
    if Z jump upd_ptrs;
        r0 = M[I4, 0];      // read the next word, unpack MS 16-bit
        Null = r5 AND IS_PACKED_MASK;
        if Z jump octet_orphan;
            // half-word_orphan:
            r2 = r0 LSHIFT r1;  // LS half-word
            r6 = 2,             // offset for packed 32-bit mid-word
             M[I0, M0] = r2;
            Null = r5 AND IS_ODD_OFFSET_MASK;
            if Z jump upd_ptrs; // no more orphans
        post_loop_octet_orphan:
                // continue with an octet after a half-word orphan
                r2 = r0 AND 0xFF00;
            wr_oct_orphan:
                r0 = M[I0, 0];      // read existing content
                r6 = r6 + 1;        // one more octet offset on dst
                r0 = r0 AND 0xFF;   // mask for kept content
                r2 = r2 OR r0;      // mix the existing content with the orphan
                M[I0, 0] = r2;

    upd_ptrs:
    // Update the write address
    r0 = M[FP + 1*ADDR_PER_WORD];                // cbuffer_dest
    r1 = I0;
    r2 = r5 AND IS_ODD_OFFSET_MASK;      // dst is unpacked, can't have offsets 2 and 3
    call $cbuffer.set_write_address_ex;

    // Update the read address
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    r1 = I4;
    r2 = r6;
    call $cbuffer.set_read_address_ex;

    // Restore index, modify & length registers
    popm <I0, I4, M0, L0, L4>;
    popm <FP, r0, r1, r2, r4, r5, r6, rLink>;
    rts;

    octet_orphan:
        // one octet only
        r2 = r0 LSHIFT r1;
        r2 = r2 AND 0xFF00;
        jump wr_oct_orphan;

.ENDMODULE;

// --------- unpack_unaligned_ex ----------
//void cbufer_unpack_unaligned_ex(tCbuffer *dst, tCbuffer *src, unsigned num_octets);
// in r0 destination cbuffer address
// in r1 source cbuffer address
// in r2 number of octets to copy (non-zero)
// trashed r3, r10, B0, B4
// NB. TODO this is only used for not "in-phase"
.MODULE $M.cbuffer_unpack_unaligned_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.unpack_unaligned_ex:
$_cbuffer_unpack_unaligned_ex:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer.unpack_unaligned_ex.PATCH_ID_0)
    // save the input paramerters for later
    pushm <FP(=SP), r0, r1, r2, r4, r5, r6, r7, r8, r9, rLink>;
    pushm <I0, I4, L0, L4>;

    BUFFER_GET_USABLE_OCTETS(r9, r0);           // destination
    Null = r9 - USE_16_BIT;
    if NZ call $error;

    BUFFER_GET_USABLE_OCTETS(r4, r1);           // usable_octets source
    if NZ call $error;
    r4 = ADDR_PER_WORD;

    // get dest buffer true write address and size
    call $cbuffer.get_write_address_ex;
    I4 = r0;
    L4 = r2;
    r6 = r1;        // save write octet offset
    push r3;
    pop B4;
    r5 = r3;        // prepare for checking for in-place

    // get src buffer read address and size
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    // get the read address and size
    call $cbuffer.get_read_address_ex;
    r7 = r1 AND IS_ODD_OFFSET_MASK; // mask to keep only lsb of the packed side
    Null = r6 - r7;
    if Z call $error;   // should not be here for an aligned operation
    I0 = r0;
    L0 = r2;

    // check if cbuffer base addresses are the same
    Null = r5 - r3;
    if Z call $error;  // cannot do in-place, input data will be overwritten

    // init base for src ahead of doloop
    push r3;
    pop B0;
    r7 = 0xFF;
    r8 = r9 - r4; // us_octets_dest - us_octets_source

    r10 = M[FP + 3*ADDR_PER_WORD];  // amount of data to be copied
    do copy_loop;
        r2 = ADDR_PER_WORD - r1;    // source
        r2 = r2 - 1;
        r2 = r2 LSHIFT 3;
        r2 = r7 LSHIFT r2;
        r5 = M[I0, 0];
        r5 = r5 AND r2;             // octet read

        r2 = r8 + r1;               // overwritten
        r2 = r2 LSHIFT 3;
        r5 = r5 LSHIFT r2;          // move octet read into the LSB - shift left

        r3 = r6 LSHIFT 3;
        r3 = -r3;
        r5 = r5 LSHIFT r3;          //source octet in the right position for source

        r3 = -r3;
        r3 = r7 LSHIFT r3;          // mask dest

        r2 = M[I4, 0];
        r2 = r2 AND r3;

        r2 = r2 + r5;

        M[I4, 0] = r2;

        r1 = r1 + 1;
        Null = r4 - r1;
        if NZ jump check_dest_offset;
            r1 = 0;
            r3 = M[I0, ADDR_PER_WORD];

        check_dest_offset:
        r6 = r6 + 1;
        Null = r9 - r6;
        if NZ jump loop_end;
            r6 = 0;
            r3 = M[I4, ADDR_PER_WORD];

        loop_end:
        nop;
    copy_loop:

    upd_ptrs:
    r5 = r1;    // save final offset source
    // Update the write address
    r0 = M[FP + 1*ADDR_PER_WORD];                // cbuffer_dest
    r2 = r6;
    r1 = I4;
    call $cbuffer.set_write_address_ex;
    // r2 is not trashed

    // Update the read address
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    r1 = I0;
    r2 = r5;
    call $cbuffer.set_read_address_ex;

    cp_pop_and_exit:
    // Restore index & length registers
    popm <I0, I4, L0, L4>;
    popm <FP, r0, r1, r2, r4, r5, r6, r7, r8, r9, rLink>;
    rts;

.ENDMODULE;


// --------- cbuffer_set_usable_octets ----------
// in r0 destination cbuffer address
// in r1 the value of usable_octets to be set
// trashed r1, r3
.MODULE $M.cbuffer_set_usable_octets_ex;
    .CODESEGMENT BUFFER_PM;
$cbuffer.set_usable_octets_ex:
$_cbuffer_set_usable_octets_ex:
   Null = r0;
   if Z call $error;
   r3 = M[r0 + $cbuffer.DESCRIPTOR_FIELD];
   r3 = r3 AND (~$cbuffer.USABLE_OCTETS_MASK);
   Null = r1 - 4;
   if Z r1 = 0;
   r1 = r1 LSHIFT $cbuffer.USABLE_OCTETS_SHIFT;
   r3 = r3 OR r1;
   M[r0 + $cbuffer.DESCRIPTOR_FIELD] = r3;
   rts;

.ENDMODULE;

// --------- cbuffer_copy_audio_to_packed ----------
//void cbuffer_copy_audio_to_packed(tCbuffer * dst, tCbuffer *src, unsigned num_words); -----------------------------------------------------
// in r0 destination cbuffer address
// in r1 source cbuffer address
// in r2 number of words to copy
// trashed r3, r10, B0, B4
//
//      |<----32bit----->|                 |<------------32bit------------->|
// src  +----------------+     dst         +--------------------------------+
//      | W0             |                 | W1(top 16bits)| W0(top 16bits) |
//      +----------------+                 +--------------------------------+
//      | W1             |                 | W3(top 16bits)| W2(top 16bits) |
//      +----------------+   ---------->   +--------------------------------+
//      | W2             |                 | ...                            |
//      +----------------+                 +--------------------------------+
//      | W3             |                 | ...                            |
//      +----------------+                 +--------------------------------+
//
// The bottom 16 bits of 32-bit sample are lost in the packing process

.MODULE $M.cbuffer_copy_audio_to_packed;
    .CODESEGMENT BUFFER_PM;
$_cbuffer_copy_audio_to_packed:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer_copy_audio_to_packed.PATCH_ID_0)
    // save the input paramerters for later
    pushm <FP(=SP), r0, r1, r2, r4, r5, r6, rLink>;
    pushm <I0, I4, M0, L0, L4>;

    // load a few constants
    M0 = ADDR_PER_WORD;
    r4 = -16;
    // get dest buffer true write address and size
    call $cbuffer.get_write_address_ex;
    I4 = r0;
    L4 = r2;
    r6 = r1;        // save write octet offset
    push r3;
    pop B4;
    r5 = r3;        // prepare for checking for in-place

    // get src buffer read address and size
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    // get the read address and size
    call $cbuffer.get_read_address_ex;
    I0 = r0;
    L0 = r2;

    // check if cbuffer base addresses are the same
    Null = r5 - r3;
    if NZ jump not_in_place_copy;
        // only advance pointers, no copy for in-place
        r5 = M[FP + 3*ADDR_PER_WORD];            // copy amount
        r0 = M[FP + 1*ADDR_PER_WORD];            // cbuffer_dest
        r1 = r5;
        call $cbuffer.advance_write_ptr_ex;

        r0 = M[FP + 2*ADDR_PER_WORD];            // cbuffer_src
        r1 = r5;
        call $cbuffer.advance_read_ptr_ex;

        jump cp_pop_and_exit;

    not_in_place_copy:
    
    // init base for src ahead of doloop
    push r3;
    pop B0;
    // get the amount of data to copy and set up a few masks
    r5 = M[FP + 3*ADDR_PER_WORD];
    r3 = 0xFFFF0000;
    r2 = 0x0000FFFF;

    Null = r6;
    if Z jump dst_word_aligned;
        // the previous write left the offset mid-word;
        // we first get word aligned to set up an efficient packing loop
        r1 = M[I4, 0];
        r1 = r1 AND r2, r0 = M[I0, M0];
        r0 = r0 AND r3;
        r1 = r1 OR r0;
        M[I4, M0] = r1;
        r5 = r5 - 1;
    dst_word_aligned:
    
    // loop count is half the number of words to pack
    r10 = r5 ASHIFT -1;
    r6 = 0;

    do copy_loop;
        r0 = M[I0, M0];
        r0 = r0 LSHIFT r4, r1 = M[I0, M0];
        r1 = r1 AND r3;
        r1 = r1 OR r0;
        M[I4,  M0] = r1;
    copy_loop:
    
    // if any leftover sample pack it here
    Null = r5 AND 0x1;
    if Z jump upd_ptrs;
        r1 = M[I4, 0];
        r0 = M[I0, M0];
        r1 = r1 AND r3;
        r0 = r0 LSHIFT r4;
        r1 = r1 OR r0;
        M[I4, 0] = r1;
        r6 = 2;

    upd_ptrs:
    // Update the write address
    r0 = M[FP + 1*ADDR_PER_WORD];                // cbuffer_dest
    r2 = r6;
    r1 = I4;
    call $cbuffer.set_write_address_ex;

    // Update the read address
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    r1 = I0;
    r2 = 0;
    call $cbuffer.set_read_address_ex;

    cp_pop_and_exit:
    // Restore index, modify & length registers
    popm <I0, I4, M0, L0, L4>;
    popm <FP, r0, r1, r2, r4, r5, r6, rLink>;
    rts;

.ENDMODULE;



// --------- cbuffer_copy_packed_to_audio ----------
//void cbuffer_copy_packed_to_audio(tCbuffer * dst, tCbuffer *src, unsigned num_words); -----------------------------------------------------
// in r0 destination cbuffer address
// in r1 source cbuffer address
// in r2 number of words to copy
// trashed r3, r10, B0, B4
//
//      |<----32bit----->|            |<------------32bit------------->|
// src  +----------------+     dest   +--------------------------------+
//      | W0             |            | W0(bottom 16bits)| 16 bits of 0|
//      +----------------+            +--------------------------------+
//      | W1             |            | W0(top 16bits)   | 16 bits of 0|
//      +----------------+   ----->   +--------------------------------+
//      | ...            |            | W1(bottom 16bits)| 16 bits of 0|
//      +----------------+            +--------------------------------+
//      | ...            |            | W1(top 16bits)   | 16 bits of 0|
//      +----------------+            +--------------------------------+

.MODULE $M.cbuffer_copy_packed_to_audio;
    .CODESEGMENT BUFFER_PM;
$_cbuffer_copy_packed_to_audio:
    LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($cbuffer_copy_packed_to_audio.PATCH_ID_0)
    // save the input paramerters for later
    pushm <FP(=SP), r0, r1, r2, r4, r5, r6, rLink>;
    pushm <I0, I4, M0, L0, L4>;

    // load a few constants
    M0 = ADDR_PER_WORD;
    // get dest buffer true write address and size
    call $cbuffer.get_write_address_ex;
    I0 = r0;
    L0 = r2;
    push r3;
    pop B0;
    r5 = r3;        // prepare for checking for in-place

    // get src buffer read address and size
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    // get the read address and size
    call $cbuffer.get_read_address_ex;
    I4 = r0;
    r6 = r1;        // save read octet offset
    L4 = r2;

    // check if cbuffer base addresses are the same
    Null = r5 - r3;
    if NZ jump not_in_place_copy;
        // only advance pointers, no copy for in-place
        r5 = M[FP + 3*ADDR_PER_WORD];            // copy amount
        r0 = M[FP + 1*ADDR_PER_WORD];            // cbuffer_dest
        r1 = r5;
        call $cbuffer.advance_write_ptr_ex;

        r0 = M[FP + 2*ADDR_PER_WORD];            // cbuffer_src
        r1 = r5;
        call $cbuffer.advance_read_ptr_ex;

        jump cp_pop_and_exit;

    not_in_place_copy:
    // init base for src ahead of doloop
    push r3;
    pop B4;

    // get the amount of data to copy and set up a few masks
    r5 = M[FP + 3*ADDR_PER_WORD];
    r3 = 0xFFFF0000;

    Null = r6;
    if Z jump src_word_aligned;
        // the previous read left the offset mid-word;
        // we first get word aligned to set up an efficient packing loop
        r0 = M[I4, M0];
        r0 = r0 AND r3;
        r5 = r5 - 1, M[I0, M0] = r0;
    src_word_aligned:

    // loop count is half the number of words to pack
    r10 = r5 ASHIFT -1;
    r6 = 0;

    do copy_loop;
        r2 = M[I4, M0];
        r0 = r2 LSHIFT 16;
        r1 = r2 AND r3, M[I0,  M0] = r0;
        M[I0,  M0] = r1;
    copy_loop:

    // if any leftover sample unpack it here
    Null = r5 AND 0x1;
    if Z jump upd_ptrs;
        r0 = M[I4, 0];
        r0 = r0 LSHIFT 16;
        M[I0, M0] = r0;
        r6 = 2;

    upd_ptrs:
    // Update the write address
    r0 = M[FP + 1*ADDR_PER_WORD];                // cbuffer_dest
    r2 = 0;
    r1 = I0;
    call $cbuffer.set_write_address_ex;

    // Update the read address
    r0 = M[FP + 2*ADDR_PER_WORD];                // cbuffer_src
    r1 = I4;
    r2 = r6;
    call $cbuffer.set_read_address_ex;

    cp_pop_and_exit:
    // Restore index, modify & length registers
    popm <I0, I4, M0, L0, L4>;
    popm <FP, r0, r1, r2, r4, r5, r6, rLink>;
    rts;

.ENDMODULE;

#endif /* INSTALL_CBUFFER_EX */
