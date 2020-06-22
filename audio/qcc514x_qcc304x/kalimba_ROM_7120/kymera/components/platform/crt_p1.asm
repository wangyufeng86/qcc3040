// *****************************************************************************
// Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.
// %%version
// *****************************************************************************

// Setup C run time environment for P1 before calling main
// It initalises Stack, PM RAM and DM RAM for P1. 
.MODULE $M.crt_p1;
#ifdef CHIP_BASE_HYDRA
   .CODESEGMENT EXT_DEFINED_PM;
#else
   .CODESEGMENT PM_RAM;
#endif
   .DATASEGMENT DM;

   $_crt_p1:
   call $_pm_ram_initialise_p1;

   call $_dm_initialise_p1_part1;

   call $_stack_initialise;

   // The compiler expects these registers set up this way.
   // See the ABI in CS-124812-UG.
   M0 = 0;
   M1 = ADDR_PER_WORD;
   M2 = -ADDR_PER_WORD;
   // The compiler also expects L0=L1=L4=L5=0.
   // FIXME is that guaranteed by prior code?

#ifdef INSTALL_THREAD_OFFLOAD
   /* Check this is audio thread */
   call $_audio_thread_offload_is_enabled;
   Null = r0;
   if NZ jump $p1_thread_offload_main;
#endif /* INSTALL_THREAD_OFFLOAD */

   call $_dm_initialise_p1_part2;

   /* Standard Kymera 'main' */
   call $_main;
   jump $_terminate;

#ifdef INSTALL_THREAD_OFFLOAD
$p1_thread_offload_main:
   /* Thread Offload 'main' */
   call $_audio_thread_main;
   jump $_terminate;
#endif /* INSTALL_THREAD_OFFLOAD */

.ENDMODULE;

// The preserve block is a reserved space at the top of DM that doesn't get
// zeroed/initialised on boot.
.MODULE $M.preserve_block;
   .DATASEGMENT DM_P1_PRESERVE_BLOCK;
   .VAR     $_preserve_block_p1[PRESERVE_BLOCK_WORDS];

   // See preserve_block.h for more details on what this is for.
.ENDMODULE;

.MODULE $M.dm_initialise_p1_part1;
   .CODESEGMENT EXT_DEFINED_PM;
   .DATASEGMENT DM;

// Crescendo derivatives uses kld based on binutils linker
// This allows better code placement than previous klink. The linker specified symbols
// can be used to determine start address and size of sections to be copied/initialised.

$_dm_initialise_p1_part1:

    // Initialise the .mem_guard region
    r10= $DM_INIT_MEM_GUARD_SIZE_DWORDS;
    I1 = $MEM_MAP_DM_INITC_MEM_GUARD_ROM_ADDR;
    I0 = $MEM_MAP_DM_GUARD_START;
    do initc_mem_guard_copy_loop;
        r1 = M[I1,4];
        M[I0,4] = r1;
    initc_mem_guard_copy_loop:

    // Crescendo derivatives supports private memory, The DM init values goes
    // to the private memory locations of P1 even though it to the same mapped
    // location. The addresses 0x0000000 - 0x00001000 is aliased to
    // 0x0001000 - 0x00002000.
    // Crescendo d00 does not supprt running P0 and P1 from a single image
    // For seperate images , the linking symbols are different to P0 and
    // it is not expected to conflict.

    // Initialise the .initc_dm2 region
    r10 = $DM2_INIT_SIZE_DWORDS;
    I1 = $MEM_MAP_DM2_INITC_ROM_ADDR;
    I0 = $MEM_MAP_DM2_INITC_START;
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

    // Zero initialise the dm2 P1 .bss data region
    // debugBuffer1, error_stack_exception.stack_p1
    I0 = $MEM_MAP_DM2_P1_BSS_START;
    r10 = $MEM_MAP_DM2_P1_BSS_LENGTH_DWORDS;
    r2 = 0;
    do dm2_bss_p1_zero_loop;
        M[I0,4] = r2;
    dm2_bss_p1_zero_loop:

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

    // P0 will initialise all other regions.
    // Since P1 doesn't know the exact start location
    // of heap, do not attempt to zero those locations.
   rts;
.ENDMODULE;


.MODULE $M.dm_initialise_p1_part2;
   .CODESEGMENT EXT_DEFINED_PM;
   .DATASEGMENT DM;

$_dm_initialise_p1_part2:

    // Common shared regions are initialised by P0 only.
    // Initialise the .initc_dm2_p1 region
    r10= $DM1_INIT_P1_SIZE_DWORDS;
    I1 = $MEM_MAP_DM2_INITC_P1_ROM_ADDR;
    I0 = $MEM_MAP_DM2_P1_INITC_START;
    do initc_dm2_p1_copy_loop;
        r1 = M[I1,4];
        M[I0,4] = r1;
    initc_dm2_p1_copy_loop:

    rts;
.ENDMODULE;


// Do any P1 specific PM RAM initialisation
.MODULE $M.pm_ram_initialise_p1;
   .CODESEGMENT EXT_DEFINED_PM;
   .DATASEGMENT DM;

$_pm_ram_initialise_p1:

   // Enable external exceptions related to PM banks
   r1 = M[$EXT_EXCEPTION_EN];
   r1 = r1 OR $PM_BANK_EXCEPTION_EN_MASK;
   M[$EXT_EXCEPTION_EN] = r1;

   // Enable programmable exception regions for PM
   // Cache region start is defined at build time.
   r1 = PM_RAM_P1_CACHE_START_ADDRESS;
   M[$PM_PROG_EXCEPTION_REGION_START_ADDR] = r1;

   r1 = PM_RAM_CACHE_BANK_SIZE_WORDS;
   r1 = r1 LSHIFT 2; // Size in bytes
   r1 = r1 + PM_RAM_P1_CACHE_START_ADDRESS;
   r1 = r1 - 1; // Region start and end addresses are inclusive.
   M[$PM_PROG_EXCEPTION_REGION_END_ADDR] = r1;

   r1 =  M[$PROG_EXCEPTION_REGION_ENABLE];
   r1 = r1 OR $PM_PROG_EXCEPTION_REGION_ENABLE_MASK;
   M[$PROG_EXCEPTION_REGION_ENABLE] = r1;

   rts;
.ENDMODULE;

