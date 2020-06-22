// *****************************************************************************
// Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.
// %%version
// *****************************************************************************

#include "kalsim.h"

// -- setup C runtime, and call main() --
.MODULE $M.crt0;
#if defined(CHIP_BASE_HYDRA)
   .CODESEGMENT EXT_DEFINED_PM;
#else
   .CODESEGMENT PM_RAM;
#endif
   .DATASEGMENT DM;

   $_crt0:
#if defined(CHIP_BASE_BC7)
   // NVMEM should be initialised
   call $flash.init_dmconst;
#endif /* CHIP_BASE_BC7 */

#if defined(CHIP_BASE_HYDRA)
   call $_pm_ram_initialise;

   call $_dm_initialise;
#endif   /* CHIP_BASE_HYDRA */

   call $_stack_initialise;
   // The compiler expects these registers set up this way.
   // See the ABI in CS-124812-UG.
   M0 = 0;
   M1 = ADDR_PER_WORD;
   M2 = -ADDR_PER_WORD;
   // The compiler also expects L0=L1=L4=L5=0.
   // Note: at this point this seems guaranteed by prior code.

   call $_main;
   jump $_terminate;
.ENDMODULE;


// The preserve block is a reserved space at the top of DM that doesn't get
// zeroed/initialised on boot.
.MODULE $M.preserve_block;
   .DATASEGMENT DM_PRESERVE_BLOCK;
   .VAR     $_preserve_block[PRESERVE_BLOCK_WORDS];

   // See preserve_block.h for more details on what this is for.
.ENDMODULE;

#if defined(CHIP_BASE_HYDRA)
.MODULE $M.dm_initialise;
   .CODESEGMENT EXT_DEFINED_PM;
   .DATASEGMENT DM;
   // $_pm_code_limit is used by the patch code to ensure that code patches
   // don't overwrite existing PM RAM contents. It gets initialised to the
   // first free address after the cache area, and adjusted if any more code
   // gets copied into PM RAM.
   .VAR $_pm_code_limit = PM_RAM_FREE_START;

// Crescendo derivatives uses kld based on binutils linker
// This allows better code placement than previous klink. The linker specified symbols
// can be used to determine start address and size of sections to be copied/initialised.
$_dm_initialise:

    // Initialise the .mem_guard region
    r10= $DM_INIT_MEM_GUARD_SIZE_DWORDS;
    I1 = $MEM_MAP_DM_INITC_MEM_GUARD_ROM_ADDR;
    I0 = $MEM_MAP_DM_GUARD_START;
    do initc_mem_guard_copy_loop;
        r1 = M[I1,4];
        M[I0,4] = r1;
    initc_mem_guard_copy_loop:

    // Initialise the .initc_dm2 region
    r10 = $DM2_INIT_SIZE_DWORDS;
    I1 =  $MEM_MAP_DM2_INITC_ROM_ADDR;
    I0 =  $MEM_MAP_DM2_INITC_START;
    do initc_dm2_copy_loop;
        r1 = M[I1,4];
        M[I0,4] = r1;
    initc_dm2_copy_loop:

    // Initialise the .initc_dm1 region
    r10 = $DM1_INIT_SIZE_DWORDS;
    I1 = $MEM_MAP_DM1_INITC_ROM_ADDR;
    I0 = $MEM_MAP_DM1_INITC_START;
    do initc_dm1_copy_loop;
        r1 = M[I1,4];
        M[I0,4] = r1;
    initc_dm1_copy_loop:

    // Common shared regions are initialised by P0 only.
    // Initialise the .initc_dm1_p0 region
    r10= $DM1_INIT_P0_SIZE_DWORDS;
    I1 = $MEM_MAP_DM1_INITC_P0_ROM_ADDR;
    I0 = $MEM_MAP_DM1_P0_INITC_START;
    do initc_dm1_p0_copy_loop;
        r1 = M[I1,4];
        M[I0,4] = r1;
    initc_dm1_p0_copy_loop:

    // Zero initialise the dm1 P0 .bss data region
    I0 = $MEM_MAP_DM1_P0_BSS_START;
    r10 = $MEM_MAP_DM1_P0_BSS_LENGTH_DWORDS;
    r2 = 0;
    do dm1_bss_p0_zero_loop;
        M[I0,4] = r2;
    dm1_bss_p0_zero_loop:

    // Store the correct limit of used PM RAM
    r1 = $PM_RAM_PATCH_CODE_START;
    // Linker symbol has MSB set for code, so mask it off to get the real address
    r1 = r1 AND 0x7FFFFFFF;
    M[$_pm_code_limit] = r1;

    // Zero initialise the dm1 .bss data region
    I0 = $MEM_MAP_DM1_BSS_START;
    r10 = $MEM_MAP_DM1_BSS_LENGTH_DWORDS;
    r2 = 0;
    do dm1_bss_zero_loop;
        M[I0,4] = r2;
    dm1_bss_zero_loop:

    // Zero initialise the dm2 .bss data region
    I0 = $MEM_MAP_DM2_BSS_START;
    r10 = $MEM_MAP_DM2_BSS_LENGTH_DWORDS;
    r2 = 0;
    do dm2_bss_zero_loop;
        M[I0,4] = r2;
    dm2_bss_zero_loop:

    // Zero initialise the dm1 pools and heap data region
    I0 = $MEM_MAP_HEAP_START;
    r10 = $MEM_MAP_HEAP_LENGTH_DWORDS;
    r2 = 0;
    do heap_zero_loop;
        M[I0,4] = r2;
    heap_zero_loop:


#if defined(SUPPORTS_MULTI_CORE)

    // Zero initialise the shared heap region
    I0 =  $MEM_MAP_SHARED_HEAP_START;
    r10 = $MEM_MAP_SHARED_HEAP_LENGTH_DWORDS;
    r2 = 0;
    do shared_heap_zero_loop;
        M[I0,4] = r2;
    shared_heap_zero_loop:

#endif // defined(SUPPORTS_MULTI_CORE)

#ifdef CHIP_HAS_SLOW_DM_RAM
    // Zero initilaise heap in Slow RAM

    I0 =  $MEM_MAP_SLOW_HEAP_START;
    r10 = $MEM_MAP_SLOW_HEAP_LENGTH_DWORDS;
    r2 = 0;
    do ext_heap_zero_loop;
        M[I0,4] = r2;
    ext_heap_zero_loop:
    
#endif // CHIP_HAS_SLOW_DM_RAM

   rts;
.ENDMODULE;
#endif /* CHIP_BASE_HYDRA */


// Zero all the PM RAM, to make sure the cache behaves itself

#if defined(CHIP_BASE_HYDRA)
.MODULE $M.pm_ram_initialise;
   .CODESEGMENT EXT_DEFINED_PM;
   .DATASEGMENT DM;

$_pm_ram_initialise:

   // Enable the windows so PM RAM is visible in DM space
   r1 = 1;
   // Store the old state
   r0 = M[$PMWIN_ENABLE];
   M[$PMWIN_ENABLE] = r1;

   L0 = 0;
   L1 = 0;

   // Clear all the PM RAM via the windows

    // Only P0 clear the PM RAM
   I0 = PM_RAM_WINDOW;
   r1 = 0;
   r10 = PM_RAM_SIZE_WORDS;

   do pm_zero_loop;
      M[I0,ADDR_PER_WORD] = r1;
   pm_zero_loop:

   // Copy code to be run from RAM
   r10 = $PM_INIT_SIZE_DWORDS;
   I1 = $MEM_MAP_PM_INIT_ROM_ADDR;
   I0 = $MEM_MAP_RAM_CODE_START;
   do init_pm_copy_loop;
       r1 = M[I1,4];
       M[I0,4] = r1;
   init_pm_copy_loop:

   // Enable external exceptions related to PM banks
   r1 = M[$EXT_EXCEPTION_EN];
   r1 = r1 OR $PM_BANK_EXCEPTION_EN_MASK;
   M[$EXT_EXCEPTION_EN] = r1;

   // Enable programmable exception regions for PM
   // Cache region start is defined at build time.
   r1 = PM_RAM_START_ADDRESS + PM_RAM_WINDOW_CACHE_START_ADDRESS_OFFSET;
   M[$PM_PROG_EXCEPTION_REGION_START_ADDR] = r1;
   r1 = PM_RAM_CACHE_BANK_SIZE_WORDS;
   r1 = r1 LSHIFT 2; // Size in bytes
   r1 = r1 + PM_RAM_START_ADDRESS + PM_RAM_WINDOW_CACHE_START_ADDRESS_OFFSET;
   r1 = r1 - 1; // Region start and end addresses are inclusive.
   M[$PM_PROG_EXCEPTION_REGION_END_ADDR] = r1;

   r1 =  M[$PROG_EXCEPTION_REGION_ENABLE];
   r1 = r1 OR $PM_PROG_EXCEPTION_REGION_ENABLE_MASK;
   M[$PROG_EXCEPTION_REGION_ENABLE] = r1;

   // Restore window state
   M[$PMWIN_ENABLE] = r0;

   rts;
.ENDMODULE;
#endif /* CHIP_BASE_HYDRA */

// -- abort, exit, terminate routine -- !!Quits with kalsim, loops on a real chip!!
.MODULE $M.abort_and_exit;
   .CODESEGMENT EXT_DEFINED_PM;

   $_abort:
   r0 = -1;

   $_exit:
   /* For calls to exit, r0 is set to exit code already */

   $_finish_no_error:
   $_terminate:
   TERMINATE
.ENDMODULE;
