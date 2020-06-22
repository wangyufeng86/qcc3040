// *****************************************************************************
// Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.        http://www.csr.com
// %%version
// *****************************************************************************




// *****************************************************************************
// NAME:
//    Memory operations
//
// DESCRIPTION:
//    This provides a set of functions that abstract certain memory operations on
//    the Bluecore platform.      
//       $mem.ext_window_unpack_to_ram
//       $mem.ext_window_copy_to_ram
//
// *****************************************************************************


// *****************************************************************************
// NAME:
//    Flash access Library
//
// DESCRIPTION:
//    This library provides an API to aid accessing data from the onchip flash
//    memory from kalimba.  Access to flash using BC3-MM is quite slow, it takes
//    approx 2us (64 DSP clock cycles) per word.  Using BC5-MM it is faster, but
//    the access time still depends on the speed of the flash part used and the
//    MCU's clock rate and workload.  Typically with a 70ns flash the DSP will
//    take 9 cycles (at 64MHz) to access a 16bit word from flash.
//
//    For BC5-MM this library also provides an API for initialising the hardware
//    to allow execution from flash program memory (PM).  Accessing flash
//    program memory is slower than RAM, again depending on the flash part
//    used and the MCU's clock rate and workload.  Typically with a 70ns flash
//    the DSP will take 17 cycles (at 64MHz) to access a 32bit instruction word
//    from flash.  There is a 64 word direct-mapped cache that is used when
//    accessing PM flash, and so if there is a cache hit the access time is just
//    a single clock cycle (same as accessing PM RAM).
//
//    For example use see the app in: apps/examples/kalimba_flash_access_example/
//
// *****************************************************************************

#include "stack.h"

// *****************************************************************************
// MODULE:
//    $mem.ext_window_unpack_to_ram
//
//    unsigned mem_ext_window_unpack_to_ram(char *src,unsigned size,unsigned *dest);
//
// DESCRIPTION:
//    Will copy data "FORMAT_PACKED16" to RAM.
//
// INPUTS:
//    - r0 = address of data block to be copied
//    - r1 = size of data block in destination RAM
//    - r2 = destination address
//
// OUTPUTS:
//    - r0 = result:  (r0==0) FAILED, else PASSED 
//
// TRASHED REGISTERS:
//    r0, r1, r2
//
// *****************************************************************************

.MODULE $M.mem.ext_window_unpack_to_ram;
   .CODESEGMENT EXT_DEFINED_PM;

$mem.ext_window_unpack_to_ram:
$_mem_ext_window_unpack_to_ram:

   pushm <r3, r4, r5, r6, r10, rLink>;
   pushm <I0, I1>;

   I0 = r2;
   r6 = r0;       // flash address
   r5 = r1 AND 1; // is odd
   r4 = r1 ASHIFT - 1;// r4 = (even part) / 2
   page_loop:
      // map appropriate flash page in
      r0 = r6;
      r1 = r4 * 3 (int);
      push r1;

      call $mem.ext_window_access_as_ram; 

      // make sure even number of data is read each time
      I1 = r0;
      r10 = r4;
      pop r0;
      Null = r0 - r1;
      if EQ jump read_values;
         r1 = r1 * 2 (int);
         r1 = r1 * 0.33333337306976 (frac);
         r10 = r1 ASHIFT -1;
      read_values:

      // calculate remaining pairs of data to read
      r4 = r4 - r10;

      // update flash address
      r0 = r10 * 3 (int);
      r6 = r6 + r0;

      // copy data from flash to dm for this flash page
      do loop;
         // read MSB 0 (0-15)
         r0 = M[I1,1];
         // read LSB 0 (8-15), MSB 1 (0-7)
         r1 = M[I1,1];

         // -- reassemble 1st 24bit word --
         r0 = r0 LSHIFT 8;
         r3 = r1 LSHIFT -8;

         // mask off possible sign extension of flash reads
         r3 = r3 AND 0xFF;
         r3 = r3 OR r0,
          r0 = M[I1,1]; // read LSB 1 (0-15)

         // -- reassemble 2nd 24bit word --
         // mask off possible sign extension of flash reads
         r0 = r0 AND 0xFFFF;
         r1 = r1 LSHIFT 16;
         r0 = r0 + r1,
          M[I0,1] = r3;       // store 1st 24bit word

         // store 2nd 24bit word
         M[I0,1] = r0;
      loop:
      // if we need more data from another flash page loop around again
      Null = r4;
   if NZ jump page_loop;

   // if number of samples to be read is odd, read the last sample separately
   Null = r5;
   if Z jump jp_unpack_done;
      r0 = r6;
      r1 = 2;
      call $mem.ext_window_access_as_ram;
      I1 = r0;
      r0 = M[I1,1];
      r1 = M[I1,1];
      r0 = r0 LSHIFT 8;
      r1 = r1 LSHIFT -8;
      r1 = r1 AND 0xFF;
      r0 = r1 OR r0;
      M[I0, 1] = r0;
   jp_unpack_done:

   popm <I0, I1>;
   popm <r3, r4, r5, r6, r10, rLink>;
   r0 = 1;
   rts;

.ENDMODULE;



// *****************************************************************************
// MODULE:
//    $mem.ext_window_copy_to_ram
//
//    unsigned mem_ext_window_copy_to_ram(char *src,unsigned size,unsigned *dest);
//
// DESCRIPTION:
//    Will copy data "FORMAT_16BIT_SIGN_EXT" to RAM.
//
//
// INPUTS:
//    - r0 = address of data block to be copied
//    - r1 = size of data block in destination RAM
//    - r2 = destination address
//
// OUTPUTS:
//    - r0 = result:  (r0==0) FAILED, else PASSED 
//
// TRASHED REGISTERS:
//    r0, r1, r2
//
// *****************************************************************************
.MODULE $M.mem.ext_window_copy_to_ram;
   .CODESEGMENT EXT_DEFINED_PM;

$mem.ext_window_copy_to_ram:
$_mem_ext_window_copy_to_ram:

   pushm <I0, I1>;
   pushm <r3, r4, r5, r10, rLink>;

   I0 = r2;
   r4 = r0;
   r5 = r1;
   page_loop:
      // map appropriate flash page in
      r0 = r4;
      r1 = r5;

      call $mem.ext_window_access_as_ram;
      // copy data from flash to dm for this flash page
      r10 = r1;
      I1 = r0;
      do loop;
         r0 = M[I1,1];
         M[I0,1] = r0;
      loop:
      // if we need more data from another flash page loop around again
      r4 = r4 + r1;
      r5 = r5 - r1;
   if NZ jump page_loop;

   popm <r3, r4, r5, r10, rLink>;
   popm <I0, I1>;
   r0 = 1;
   rts;

.ENDMODULE;





// *****************************************************************************
// MODULE:
//    $mem.ext_window_access_as_ram
//
// DESCRIPTION:
//    Given the address of a 'flash segment' variable this routine will map in
// the appropriate flash page required and return a pointer to the DM2 flash
// window to access it.
//
// The function also assumes that the size of the copy you wish to make from
// the flash page is passed in r1. If r1 is greater than the flash page size
// it will be set to the flash page size. If you don't require this
// behaviour, any value can be passed in r1 and the result can be ignored.
//
//
// INPUTS:
//    - r0 = address of variable in 'flash segment'
//    - r1 = optional size of variable in 'flash segment'
//
// OUTPUTS:
//    - r0 = an address in the DM2 flash window to read the variable from
//    - r1 = an adjusted value of the size so that reads are kept within
//           a single flash page
//
// TRASHED REGISTERS:
//    r2,r3
//
//
// *****************************************************************************
.MODULE $M.mem.ext_window_access_as_ram;
   .CODESEGMENT FLASH_MAP_PAGE_INTO_DM_PM;
   .DATASEGMENT DM;

   // base address in flash of the data - filled out by the VM's KalimbaLoad()
   .VAR/DM_STATIC $flash.windowed_data16.address;

$mem.ext_window_access_as_ram:

   r2 = M[$flash.windowed_data16.address];


   // In KAL_ARCH3, $FLASH_WINDOW3_START_ADDR is in units of 32 bits. However offset in
   // r0 is in 16 bits, so we need to transform it
   r3 = r0 ASHIFT -1;
   M[$FLASH_WINDOW3_START_ADDR] = r2 + r3;

   // if the address is odd we need to lose a word from the start of the window
   r3 = r0 AND 1;

   // limit r1 (requested amount to read) so that reads are kept within flash page
   r0 = $FLASHWIN3_SIZE - r3;
   r0 = r1 - r0;
   if POS r1 = r1 - r0;

   // address to read from is just the flashwin3 start address
   r0 = r3 + &$FLASHWIN3_START;

   rts;


.ENDMODULE;





// *****************************************************************************
// MODULE:
//    $flash.copy_to_dm_32_to_24
//
// DESCRIPTION:
//    Given the address of a 'flash segment' variable and its size this routine
// will copy the variable to a place in data memory ram.  It will automatically
// switch flash pages as needed if the variable overlaps a flash page boundary.
// each 24-bit data, 0xABCDEF, is stored in 2 words in flash:
//
//    word 1: 0xABCD
//    word 2: 0x--EF
//
// where the 8 MSBits of the second word are ignored.
//
//
// INPUTS:
//    - r0 = address of variable in 'flash segment'
//    - r1 = size of variable in 'flash segment'
//    - r2 = address to copy flash data to in DM
//
// OUTPUTS:
//    - r0 = last word read from flash (useful if you're just reading 1 word)
//    - I0 = end address of copied data + 1
//
// TRASHED REGISTERS:
//    r0, r1, r3, r4, r5, I1, I0, r10, DoLoop
//
// *****************************************************************************
.MODULE $M.flash.copy_to_dm_32_to_24;
   .CODESEGMENT FLASH_COPY_TO_DM_32_TO_24_PM;

   $flash.copy_to_dm_32_to_24:

   I0 = r2;

   // push rLink onto stack
   push rLink;

   r4 = r0;
   r5 = r1 * 2 (int);
   page_loop:
      // map appropriate flash page in
      r0 = r4;
      r1 = r5;
      call $mem.ext_window_access_as_ram;
      // copy data from flash to dm for this flash page
      r10 = r1 ASHIFT -1;
      r1 = r10 * 2 (int);
      I1 = r0;
      do loop;
         r0 = M[I1,1];
         r0 = r0 LSHIFT 8;
         r2 = M[I1,1];
         r2 = r2 AND 0xFF;
         r0 = r0 OR r2;
         M[I0,1] = r0;
      loop:
      // if we need more data from another flash page loop around again
      r4 = r4 + r1;
      r5 = r5 - r1;
   if NZ jump page_loop;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

