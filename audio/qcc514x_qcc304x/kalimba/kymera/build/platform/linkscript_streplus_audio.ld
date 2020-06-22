/*
 * Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 * Notifications and licenses are retained for attribution purposes only
 */
/*
 * Common link script for running P0 and P1 fron common image or from
 * separate images.
 * All Memory sizes in the link script are in Bytes.
 */
OUTPUT_ARCH(QCC514x_audio)

ENTRY($reset)

/*
 * Generate a simple crc32 of the contents of all code and data sections
 * in the final linked image.
 * The global symbol __devtools_image_checksum is added to the symbol table
 * with the crc32 value in
 * section dm. For more info see https://ukbugdb/B-198383
 */
CHECKSUM_DATA_SECTION(.const);

/* Description of the physical characteristics of the chip: data memory */
$PHYSICAL_DMRAM_INSTANCE_SIZE = 0x8000;    /* 32 KiB */
$DATA_BANK_NUMBER             = 14;

/* Description of the physical characteristics of the chip: program memory */
$PHYSICAL_PMRAM_INSTANCE_SIZE = 0x2000;  /* 8 KiB */
$PM_BANK_NUMBER               = 10;

/* Description of the physical characteristics of the chip: cache */
$DIRECT_CACHE_SIZE                = 0x0800; /* 2 KiB */
$DIRECT_CACHE_SIZE_IN_BANKS       = 1;
$TWO_WAY_CACHE_SIZE               = 0x8800; /* 34 KiB */
$TWO_WAY_CACHE_SIZE_IN_BANKS      = 5;
$TWO_WAY_HALF_CACHE_SIZE          = 0x4400; /* 17 KiB */
$TWO_WAY_HALF_CACHE_SIZE_IN_BANKS = 3;


/* Base Addresses */
$DM1_RAM_BASE_ADDRESS         = 0x00000000; /* Start of DM RAM accessed by DM1 bus */
$DM2_RAM_BASE_ADDRESS         = 0xFFF00000; /* Start of DM RAM accessed by DM2 bus */
$PM_RAM_START_ADDRESS         = 0x04000000; /* Start of PM RAM accessed by PM bus */
$DM_WINDOW_TO_PM_BASE_ADDRESS = 0x00100000; /* Start of PM RAM accessed by DM bus */
$DM_NVRAM0_START_ADDRESS      = 0xF8000000; /* Start of ROM or SQIF accessed by DM bus */

/* Maximum DM1 Address */
$DM1_RAM_MAX_ADDRESS          = $DM1_RAM_BASE_ADDRESS + $DATA_BANK_NUMBER * $PHYSICAL_DMRAM_INSTANCE_SIZE;

$PM_RAM_END_ADDRESS           = $PM_RAM_START_ADDRESS +
                                $PM_BANK_NUMBER * $PHYSICAL_PMRAM_INSTANCE_SIZE;

$DM_MEM_BLOCK_SIZE           = 0x1000;    /* 4 KiB = private memory size*/

/* Private memory on DM1*/
$DM1_P0_PRIVATE_MEMORY_END_ADDRESS = $DM1_RAM_BASE_ADDRESS + $DM_MEM_BLOCK_SIZE;
$DM1_P1_PRIVATE_MEMORY_END_ADDRESS = $DM1_P0_PRIVATE_MEMORY_END_ADDRESS + $DM_MEM_BLOCK_SIZE;
$DM1_PRIVATE_MEMORY_END_ADDRESS    = DEFINED(P1_STANDALONE) ?
                                     $DM1_P1_PRIVATE_MEMORY_END_ADDRESS :
                                     $DM1_P0_PRIVATE_MEMORY_END_ADDRESS;

/* If dual core is not enabled keep all statics together, so give space */
$DM1_PUBLIC_MEM_START_ADDRESS  = $DM1_P1_PRIVATE_MEMORY_END_ADDRESS;
$DM1_PRIVATE_GUARD_END_ADDRESS = $DM1_PRIVATE_MEMORY_END_ADDRESS;
$DM1_PRIVATE_GUARD_ADDRESS     = $DM1_PRIVATE_GUARD_END_ADDRESS - 4;

/* Handle the defined symbols */
$CODE_ROM_BASE                 = 0x00000000;
$MEM_MAP_P1_CODE_START_ADDRESS = DEFINED(P1_CODE_START_ADDRESS) ?
                                 P1_CODE_START_ADDRESS : 0;
$CODE_P0_BASE                  = $CODE_ROM_BASE;
$CODE_BASE                     = DEFINED(P1_STANDALONE) ?
                                 $MEM_MAP_P1_CODE_START_ADDRESS :
                                 $CODE_P0_BASE;

/* The caches can work in 3 different modes:
   - Direct cache consume 2 KiB in the first and last Program Memory (PM) bank.
   It is useful when running from ROM as it gives good enough performance
   and leave plenty of space for Downloadable capabilities.
   - 2-Way cache consume an additional 32 KiB per processor in the following
   banks at the bottom and the top. It is useful when running from SQIF as
   it mitigates the performance loss.
   - 2-Way Half cache is similar to the previous but consumes only 16 KiB
   per processor. It is useful when running from SQIF on a chip with little
   PM available.

   Bank 0 is a special case. It contains both some cache and some function needed
   to activate and desactive the caches. The banks not used for cache are split
   between the 2 processors either evenly or according to the
   "PM_RAM_CODE_P0_END_BANK" configuration variable. */

/* On Aura, PM banks are 8 KiB so:
   - 2-Way cache uses banks 0 to 4 for P0 and banks 5 to 9 for P1. Leaving
   no space for downloadable capabilities.
   - 2-Way Half cache uses banks 0 to 2 for P0 and banks 7 to 9 for P1. Sadly
   there seem to be a bug in hardware that makes it unusable. */
$CACHE_SIZE_IN_BANKS           = DEFINED(USE_2WAYHALF_CACHE) ?
                                 $TWO_WAY_HALF_CACHE_SIZE_IN_BANKS :
                                 $DIRECT_CACHE_SIZE_IN_BANKS;
$CACHE_SIZE_IN_BANKS           = DEFINED(USE_2WAY_CACHE) ?
                                 $TWO_WAY_CACHE_SIZE_IN_BANKS :
                                 $CACHE_SIZE_IN_BANKS;

/* Give one extra bank for patches only if 2 WAY CACHE is not defined */
$PM_PATCH_NUM_BANKS            = DEFINED(USE_2WAY_CACHE) ? 0 : 1;


$PM_P0_LAST_BANK               = DEFINED(PM_RAM_CODE_P0_END_BANK) ?
                                 PM_RAM_CODE_P0_END_BANK :
                                 ($PM_BANK_NUMBER / 2) + $PM_PATCH_NUM_BANKS;

ASSERT(($PM_P0_LAST_BANK >= $CACHE_SIZE_IN_BANKS) &&
       ($PM_P0_LAST_BANK <= ($PM_BANK_NUMBER - $CACHE_SIZE_IN_BANKS)),
       "$PM_P0_LAST_BANK must be outside of the area reserved for the caches.")
$PM_RAM_CODE_P0_END_ADDRESS    = $PM_RAM_START_ADDRESS +
                                 $PM_P0_LAST_BANK * $PHYSICAL_PMRAM_INSTANCE_SIZE;

/* The PM bank used either by the direct cache of P0 or the tags of the 2-Way cache
   and 2-Way half cache also contains functions to enable and disable the cache.
   The functions are called once so this will not degrade performance. */
$PM_CACHE_SIZE_OCTETS          = DEFINED(USE_2WAY_CACHE) ? $TWO_WAY_CACHE_SIZE : $DIRECT_CACHE_SIZE;
$PM_CACHE_SIZE_OCTETS          = DEFINED(USE_2WAYHALF_CACHE) ? $TWO_WAY_HALF_CACHE_SIZE : $PM_CACHE_SIZE_OCTETS;
$PM_CACHE_P0_END               = $PM_RAM_START_ADDRESS + $PM_CACHE_SIZE_OCTETS;

$DM1_RAM_START_ADDRESS         = DEFINED(P1_STANDALONE) ?
                                 $DM1_P0_PRIVATE_MEMORY_END_ADDRESS :
                                 $DM1_RAM_BASE_ADDRESS;
$DATA_START_ADDRESS            = $DM_NVRAM0_START_ADDRESS;
$CONST_START_BASE              = $DM_NVRAM0_START_ADDRESS + $CODE_BASE;
$PM_DATA_START_BASE            = $DM_NVRAM0_START_ADDRESS;

/* Stack, preserved areas and heaps are defined relative to anchors
   that define the start of specific banks and end DM addresses */
$ANCHOR_DM1_BANK1_START_ADDRESS = $DM1_RAM_BASE_ADDRESS +
                                  1 * $PHYSICAL_DMRAM_INSTANCE_SIZE;
$ANCHOR_DM2_BANK1_START_ADDRESS = $DM2_RAM_BASE_ADDRESS +
                                  1 * $PHYSICAL_DMRAM_INSTANCE_SIZE;

/* P1 stack, if P1 is enabled, is anchored to the start of DM bank 1 */
$ANCHOR_P1_STACK_START = $ANCHOR_DM2_BANK1_START_ADDRESS;

$DM1_RESERVED_PATCH_SIZE = PATCH_RESERVED_RAM;

/* This is carefully sized to be just about big enough for the current
 * contents of the preserve block, to save memory. (Units are octets.) */
$SPACE_FOR_PRESERVE_BLOCK = 28;

/* Preserve block and stack. */
ASSERT(FORCE_STACK_SIZE_WORDS != 0, "Absolute stack size must be specified with FORCE_STACK_SIZE_WORDS")
$SPACE_FOR_STACK_AND_PRESERVE = FORCE_STACK_SIZE_WORDS*4 + $SPACE_FOR_PRESERVE_BLOCK;

/* P0 stack hangs off the top end of the DM space */
$P0_STACK_START_ADDRESS    = $DM1_RAM_MAX_ADDRESS - $SPACE_FOR_STACK_AND_PRESERVE;

/* P1 stack starts at the boundary of DM bank 1 */
$P1_STACK_START_ADDRESS    = $ANCHOR_P1_STACK_START;

/* Symbol lookup table (SLT) offset from start of ROM */
$AUDIO_SLT_OFFSET = 0x20;

/* This magic layout offset for $CODE_MAGIC$ tells the linker that this is
 * in the program address space rather than the data address space */
$CODE_MAGIC$      = 0x80000000;


SECTIONS
{
    /* Code starts at ROM address zero (aka LMA - Load Memory Address) */
    __CODE_load_address = $CODE_BASE;

    . = ALIGN($CODE_MAGIC$ + __CODE_load_address, 0x4);
    /* Kalimba objects create a unique section name for each function/symbol
     * These are of the form <CODESEGMENT_NAME>?<function/symbol_name>
     * Hence always match the '?' and trailing chars
     *
     * Reset code is usually maxim and then it changes to minim. So we need
     * different alignment for the different sections     */
    .text_reset : AT (__CODE_load_address)
    {
        $MEM_MAP_CODE_START = ABSOLUTE(.);
        *(RESET_PM\?*)
    }

    __CODE_reset_size = SIZEOF(.text_reset);

    /* This is a duplicate copy for P1 but the code should look in the
     * P0 location
     */
    . = ALIGN($CODE_MAGIC$ + __CODE_load_address + $AUDIO_SLT_OFFSET, 0x4);
    __CONST_SLT_rom_address = __CODE_load_address + $AUDIO_SLT_OFFSET;
    .const_slt : AT (__CONST_SLT_rom_address)
    {
        /* Put the symbol look up table between reset code and rest of code */
        KEEP(*(CONST_SLT\?*))
    }

    __CONST_SLT_size = SIZEOF(.const_slt);

    .text_minim :
    {
        KEEP(*.o(PM_KEEP\?*__minim))
        *.o(*pm*\?*__minim)
        *.o(PM_FLASH\?*__minim)
        *.o(*PM\?*__minim)
    }

    .text_minim_private ALIGN(4) :
    {
        /* Separate code section for scrambled libraries */
        KEEP(*.po(PM_KEEP\?*__minim))
        *.po(*pm*\?*__minim)
        *.po(PM_FLASH\?*__minim)
        *.po(*PM\?*__minim)
    }

    .text_maxim ALIGN(4) :
    {
        /* we want to include all the sections except the PM_RAM ones in
         * .text_maxim. PM_RAM needs to after all this, so that the
         * rest of the build process can pick it up and do things with
         * it (dm_append, etc). Thats why we dont use the only and only rule
         * *(*__maxim)
         */
        KEEP(*.o(PM_KEEP\?*__maxim))
        *.o(PM_MAXIM\?*__maxim)
        *.o(*PM_FLASH\?*__maxim)
        *.o(*PM\?*__maxim)
        *.o(*pm*\?*__maxim)
    }
    .text_maxim_private ALIGN(4) :
    {
        /* Separate code section for scrambled libraries*/
        KEEP(*.po(PM_KEEP\?*__maxim))
        *.po(*PM_FLASH\?*)
        *.po(*PM\?*)
        $MEM_MAP_CODE_END = ABSOLUTE(.);
    }

    /* Next ROM address (LMA) immediately follows on from last LMA of .text */
    __CODE_ROM_SIZE = $MEM_MAP_CODE_END - $MEM_MAP_CODE_START;

    /* Next section is Const section, which immediately follows on from last
     * LMA of .text.  */
    __CONST_rom_address = ALIGN(__CODE_load_address + __CODE_ROM_SIZE, 4);


    /* Const values are accessed via the NVMEM window in DM space.
     * This is a window in the data space that maps onto the ROM
     * so we advance into it the same amount as the offset in the
     * program memory.
     */
    . = $CONST_START_BASE + __CODE_ROM_SIZE;

    .const ALIGN(4) : AT (__CONST_rom_address)
    {
        $MEM_MAP_CONST_START = ABSOLUTE(.);
        *.o(DMCONST*)
        *.o(dmconst*)
        *.o(const*)
        *.o(CONST*)
    }
    .const_private ALIGN(4) :
    {
        *.po(DMCONST*)
        *.po(dmconst*)
        *.po(const*)
        *.po(CONST*)
        $MEM_MAP_CONST_END = ABSOLUTE(.);
    }

    /* Next ROM address (LMA) immediately follows on from last LMA of .const
     * Next sections (.*_ram*) are statically linked code which needs to be
       copied from ROM/SQIF into PM RAM at startup */
    __CODE_RAM_address = __CONST_rom_address +
                         ALIGN(SIZEOF(.const), 4) +
                         ALIGN(SIZEOF(.const_private), 4);

    . = ALIGN($CODE_MAGIC$ + $PM_CACHE_P0_END, 4);
    .text_ram : AT (__CODE_RAM_address)
    {
        $PM_RAM_P0_CODE_START = ABSOLUTE(.) - $CODE_MAGIC$;
        *.o(PM_RAM\?*__maxim)
    }
    $PM_RAM_STATIC_CODE_SIZE_1 = SIZEOF(.text_ram);
    .text_ram_private ALIGN(4) : AT (__CODE_RAM_address + ALIGN($PM_RAM_STATIC_CODE_SIZE_1,4))
    {
        *.po(PM_RAM\?*__maxim)
    }
    $PM_RAM_STATIC_CODE_SIZE_2 = $PM_RAM_STATIC_CODE_SIZE_1 + ALIGN(SIZEOF(.text_ram_private),4);
    .text_ram_minim ALIGN(4) : AT (__CODE_RAM_address + ALIGN($PM_RAM_STATIC_CODE_SIZE_2,4))
    {
        *.o(PM_RAM\?*__minim)
    }
    $PM_RAM_STATIC_CODE_SIZE_3 = $PM_RAM_STATIC_CODE_SIZE_2 + ALIGN(SIZEOF(.text_ram_minim),4);
    .text_ram_minim_private ALIGN(4) : AT (__CODE_RAM_address + ALIGN($PM_RAM_STATIC_CODE_SIZE_3,4))
    {
        *.po(PM_RAM\?*__minim)
    }
    $PM_RAM_STATIC_CODE_SIZE = $PM_RAM_STATIC_CODE_SIZE_3 + ALIGN(SIZEOF(.text_ram_minim_private),4);

    /* Length of code which crt0 (P0) has to copy from NV to PM RAM */
    __CODE_RAM_SIZE = $PM_RAM_STATIC_CODE_SIZE;

    /* Next ROM address (LMA) immediately follows on from LMA of RAM code sections */
    __DM2_rom_address = ALIGN(__CODE_RAM_address + __CODE_RAM_SIZE, 4);

    /* But the VMA is different: the init data is copied to RAM so the VMA
     * doesn't depend on the LMA */
    . = $DM1_RAM_START_ADDRESS + $DM2_RAM_BASE_ADDRESS;

    /* OBPM requires info_block_ptr (marked as DM_SPI_COMMS) to be at start of RAM */
    .initc_dm2_private : AT (__DM2_rom_address)
    {
        $MEM_MAP_DM2_INITC_START = ABSOLUTE(.);
        *(*DM_SPI_COMMS*)
        *.po(*DM2\?*)
        *.po(*dm2\?*)
    }
    .initc_dm2 ALIGN(4) :
    {
        *.o(*DM2\?*)
        *.o(*dm2\?*)
    }

    /* zeroinit DM2 */
    .zeroinit_dm2 ALIGN(4) (NOLOAD) :
    {
        $MEM_MAP_DM2_BSS_START = ABSOLUTE(.);
        *.o(dm2zi*)
        *.o(DM2ZI*)
    }
    .zeroinit_dm2_private ALIGN(4) (NOLOAD) :
    {
        *.po(dm2zi*)
        *.po(DM2ZI*)
        $MEM_MAP_DM2_BSS_END = ABSOLUTE(.);
    }

    __DM1_rom_address = __DM2_rom_address +
                        ALIGN(SIZEOF(.initc_dm2_private), 4) +
                        ALIGN(SIZEOF(.initc_dm2), 4);

    . = $DM1_RAM_START_ADDRESS +
        ALIGN(SIZEOF(.initc_dm2_private), 4) + ALIGN(SIZEOF(.initc_dm2), 4) +
        ALIGN(SIZEOF(.zeroinit_dm2), 4) + ALIGN(SIZEOF(.zeroinit_dm2_private), 4);

    .initc_dm1 : AT (__DM1_rom_address)
    {
        $MEM_MAP_DM1_INITC_START = ABSOLUTE(.);
        *.o(*DM1\?*)
        *.o(*dm1\?*)
        *.o(DM\?*)
        *.o(dm\?*)
    }
    .initc_dm1_private ALIGN(4) :
    {
        *.po(*DM1\?*)
        *.po(*dm1\?*)
        *.po(DM\?*)
        *.po(dm\?*)
        $MEM_MAP_DM1_INITC_END = ABSOLUTE(.);
    }

    /* zeroinit DM1 */
    .zeroinit_dm1 ALIGN(4) (NOLOAD) :
    {
        $MEM_MAP_DM1_BSS_START = ABSOLUTE(.);
        *.o(dmzi*)
        *.o(DMZI*)
    }
    .zeroinit_dm1_private ALIGN(4) (NOLOAD) :
    {
        *.po(dmzi*)
        *.po(DMZI*)
        $MEM_MAP_DM1_BSS_END = ABSOLUTE(.);
    }

    /* Make sure that the end of zeroinit DM1 in private RAM has not gone past the private memory guard
     * Note that ASSERT does not require a semicolon at the end */
    ASSERT(($MEM_MAP_DM1_BSS_END<=$DM1_PRIVATE_GUARD_ADDRESS),"DM1 zero init extends past private memory guard")

    __DM1_guard_rom_address = __DM1_rom_address + ALIGN(SIZEOF(.initc_dm1),4) + ALIGN(SIZEOF(.initc_dm1_private), 4);
    . = $DM1_PRIVATE_GUARD_ADDRESS;

    /* Add a private window guard to ensure it doesn't overlap with any other
     * region
     **/
    .initc_mem_guard : AT (__DM1_guard_rom_address)
    {
        $MEM_MAP_DM_GUARD_START = ABSOLUTE(.);
        *(*DM_MEM_GUARD\?*)
        $MEM_MAP_DM_GUARD_END = ABSOLUTE(.);
    }

    /* Area for IPC_LUT and P1 only static, P0 never access this area */
    __DM1_P0_rom_address = ALIGN(__DM1_guard_rom_address + SIZEOF(.initc_mem_guard), 4);

    . = ALIGN($DM1_PUBLIC_MEM_START_ADDRESS, 4);
    /* Running P0 and P1 from separate image would require DM_IPC_LUT
     * to be located at a fixed address. This could be achieved by adding an
     * extra section here exclusively for DM_IPC_LUT as the symbol is private
     * in release builds but public in development builds.
     * See B-221174 for more information. */
    .initc_p0 : AT (__DM1_P0_rom_address)
    {
        $MEM_MAP_DM1_P0_INITC_START = ABSOLUTE(.);
        *.o(*DM_SHARED\?*)
        *.o(*DM1_P0_RW\?*)
        *.o(*DM_P0_RW\?*)
        *.o(*DM_IPC_LUT*)
    }

    .initc_p0_private ALIGN(4) :
    {
        *.po(*DM_SHARED\?*)
        *.po(*DM1_P0_RW\?*)
        *.po(*DM_P0_RW\?*)
        *.po(*DM_IPC_LUT*)
    }

    .zeroinit_p0 ALIGN(4) (NOLOAD) :
    {
        $MEM_MAP_DM1_P0_BSS_START = ABSOLUTE(.);
        *.o(*DM_SHARED_ZI*)
        *.o(*DM1_P0_RW_ZI*)
        *.o(*DM_P0_RW_ZI*)
    }
    .zeroinit_p0_private ALIGN(4) (NOLOAD) :
    {
        *.po(*DM_SHARED_ZI*)
        *.po(*DM1_P0_RW_ZI*)
        *.po(*DM_P0_RW_ZI*)
        $MEM_MAP_DM1_P0_BSS_END_ACTUAL = ABSOLUTE(.);
        . = ALIGN(.,4);
        $PATCH_RESERVED_DM_START = .;
        . = . + $DM1_RESERVED_PATCH_SIZE;
        $PATCH_RESERVED_DM_END = .;
        $MEM_MAP_DM1_P0_BSS_END = ABSOLUTE(.);
    }

    __DM2_P1_rom_address = ALIGN(__DM1_P0_rom_address + SIZEOF(.initc_p0), 4);
    $DM2_P1_INITC_START_ADDRESS  = $DM2_RAM_BASE_ADDRESS + $MEM_MAP_DM1_P0_BSS_END;
    . = ALIGN($DM2_P1_INITC_START_ADDRESS, 4);

    .initc_p1 : AT (__DM2_P1_rom_address)
    {
        $MEM_MAP_DM2_P1_INITC_START = ABSOLUTE(.);
        *(*DM2_P1_RW\?*)
        *(*DM_P1_RW\?*)
        $MEM_MAP_DM2_P1_INITC_END = ABSOLUTE(.);
    }

    __initc_p1_address = ALIGN(__DM2_P1_rom_address + SIZEOF(.initc_p1), 4);
    .zeroinit_p1 (NOLOAD) : AT (__initc_p1_address)
    {
        $MEM_MAP_DM2_P1_BSS_START = ABSOLUTE(.);
        *.o(*DM2_P1_RW_ZI*)
        *.o(*DM_P1_RW_ZI\?*)
    }
    .zeroinit_p1_private (NOLOAD) :
    {
        *.po(*DM2_P1_RW_ZI*)
        *.po(*DM_P1_RW_ZI\?*)
        $MEM_MAP_DM2_P1_BSS_END = ABSOLUTE(.);
    }

    /*******************************************
      Stack and heap Area:
      DM1_RAM_MAX_ADDRESS
      - P0 preserved block sits at the top of DM1 space
      - P0 stack
      Everything below P0 stack is part of the heap until
      - P1 stack (but in DM2 space)
      - P1 preserve block (but in DM2 space)
      ANCHOR_DM2_BANK1_START_ADDRESS
      Everything below the bank 1 start address is shared heap until
      MEM_MAP_DM2_P1_BSS_END

      Stack and Preserve blocks are sections that contain variable
      definitions. The heap boundaries are exported as linker symbols.

      Do P0 stack before P1 stack since P1 stack is in DM2 space and
      hence the addresses will be greater.
    *******************************************/

    . = $P0_STACK_START_ADDRESS;
    .stack (NOLOAD) : AT (__initc_p1_address)
    {
        $MEM_MAP_P0_STACK_START = ABSOLUTE(.);
        *(STACK_DM*)
        $MEM_MAP_P0_STACK_END = ALIGN(.,4);
    }

    .preserve (NOLOAD) :
    {
        $MEM_P0_PRESERVE_START = ABSOLUTE(.);
        *(DM_PRESERVE_BLOCK*)
        $MEM_P0_PRESERVE_END = ALIGN(.,4);
    }

    . = $P1_STACK_START_ADDRESS;
    .p1_preserve (NOLOAD) : AT (__initc_p1_address)
    {
        $MEM_P1_PRESERVE_START = ABSOLUTE(.);
        *(DM_P1_PRESERVE_BLOCK*)
        $MEM_P1_PRESERVE_END = ALIGN(.,4);
    }

    .p1_stack (NOLOAD) :
    {
        $MEM_MAP_P1_STACK_START = ABSOLUTE(.);
        *(STACK_P1_DM*)
        $MEM_MAP_P1_STACK_END = ALIGN(.,4);
    }

    ASSERT(($MEM_P0_PRESERVE_END <= $DM1_RAM_MAX_ADDRESS), "P0 stack and preserve exceeds past DM addressable space.")
    /* P1 stack is in DM2 space. Calculate all symbols in DM1 space */
    $DUAL_MODE_HEAP_START = $MEM_MAP_P1_STACK_END - $DM2_RAM_BASE_ADDRESS;
    $MEM_MAP_HEAP_END   = $MEM_MAP_P0_STACK_START;
    $HEAP_SIZE  = $MEM_MAP_HEAP_END - $DUAL_MODE_HEAP_START;

    /* The maximum heap size in single core mode is bigger. */
    $SINGLE_MODE_HEAP_START = $ANCHOR_DM1_BANK1_START_ADDRESS;
    $SINGLE_MODE_HEAP_SIZE = $MEM_MAP_HEAP_END - $SINGLE_MODE_HEAP_START;

    /* For ThreadOffload keep the P1 stack and preserved reserved for P1, which is same as original size */
    $THREAD_OFFLOAD_HEAP_SIZE = $HEAP_SIZE;

    /* Similarly define symbols for shared heap */
    $DUAL_MODE_SHARED_HEAP_START = $MEM_MAP_DM2_P1_BSS_END - $DM2_RAM_BASE_ADDRESS;
    $MEM_MAP_SHARED_HEAP_END     = $MEM_P1_PRESERVE_START - $DM2_RAM_BASE_ADDRESS;
    $SHARED_HEAP_SIZE            = $MEM_MAP_SHARED_HEAP_END - $DUAL_MODE_SHARED_HEAP_START;

    $SINGLE_MODE_SHARED_HEAP_START = $MEM_MAP_DM1_P0_BSS_END;
    $SINGLE_MODE_SHARED_HEAP_SIZE  = $MEM_MAP_SHARED_HEAP_END - $SINGLE_MODE_SHARED_HEAP_START;

    /* For ThreadOffload same as original size */
    $THREAD_OFFLOAD_SHARED_HEAP_SIZE = $SHARED_HEAP_SIZE;

    /* Debug strings should be present in the elf file but not loaded onto the
     * chip. Therefore where we put them in the image doesn't matter,
     * so we choose an address out of the way of real code so that the linker
     * doesn't complain about overlapped sections and the value is not as
     * likely to crop up by accident. NOTE: This is different for downloadable
     * capabilities to make easier to distinguish between downloadable and
     * normal logs.
     */
    . = 0x13500000;
    debug_strings (INFO) :
    {
        KEEP(*(GLOBAL_DEBUG_STRINGS*))
        *(DEBUG_TRACE_STRINGS*)
    }

    /* Create some useful symbols for the crt code to use */
    /* Size of each regions */
    $DM2_INIT_SIZE_DWORDS = (SIZEOF (.initc_dm2) + 3) / 4 + (SIZEOF (.initc_dm2_private) + 3) / 4;
    $DM1_INIT_SIZE_DWORDS = (SIZEOF (.initc_dm1) + 3) / 4 + (SIZEOF (.initc_dm1_private) + 3) / 4;
    $DM1_INIT_P0_SIZE_DWORDS = (SIZEOF (.initc_p0) + 3) / 4 + (SIZEOF (.initc_p0_private) + 3) / 4;
    $DM1_INIT_P1_SIZE_DWORDS = (SIZEOF (.initc_p1) + 3) / 4;
    $DM_INIT_MEM_GUARD_SIZE_DWORDS = (SIZEOF (.initc_mem_guard) + 3) / 4;

    /* ROM address on each init regions */

    $MEM_MAP_DM1_INITC_ROM_ADDR = $DATA_START_ADDRESS + ABSOLUTE(__DM1_rom_address);
    $MEM_MAP_DM2_INITC_ROM_ADDR = $DATA_START_ADDRESS + ABSOLUTE(__DM2_rom_address);
    $MEM_MAP_DM1_INITC_P0_ROM_ADDR = $DATA_START_ADDRESS + ABSOLUTE(__DM1_P0_rom_address);
    $MEM_MAP_DM2_INITC_P1_ROM_ADDR = $DATA_START_ADDRESS + ABSOLUTE(__DM2_P1_rom_address);
    $MEM_MAP_DM_INITC_MEM_GUARD_ROM_ADDR = $DATA_START_ADDRESS + ABSOLUTE(__DM1_guard_rom_address);

    /* BSS Symbols */
    $MEM_MAP_DM1_BSS_LENGTH_DWORDS = (ALIGN(SIZEOF(.zeroinit_dm1),4) + ALIGN(SIZEOF(.zeroinit_dm1_private),4)) / 4;
    $MEM_MAP_DM2_BSS_LENGTH_DWORDS = (ALIGN(SIZEOF(.zeroinit_dm2),4) + ALIGN(SIZEOF(.zeroinit_dm2_private),4)) / 4;
    $MEM_MAP_DM1_P0_BSS_LENGTH_DWORDS = (ALIGN(SIZEOF(.zeroinit_p0),4) + ALIGN(SIZEOF(.zeroinit_p0_private),4)) / 4;
    $MEM_MAP_DM2_P1_BSS_LENGTH_DWORDS = (ALIGN(SIZEOF(.zeroinit_p1),4) + ALIGN(SIZEOF(.zeroinit_p1_private),4)) / 4;

    /* BSS Free symbols: These define the free DM sections start addresses and their sizes.
     * These symbols are mainly used to zero initialise the sections that can be used as heap & pools
     * and as such do not need to differentiate between DM1/DM2 or between P0/P1. There are 3 possible
     * sections - Heap, Shared Heap and "Slow Heap" (for chips that have DM RAMs with half and full speed) */
    $MEM_MAP_HEAP_START = $SINGLE_MODE_HEAP_START;
    $MEM_MAP_HEAP_LENGTH_DWORDS = ($SINGLE_MODE_HEAP_SIZE + 3) / 4;

    $MEM_MAP_SHARED_HEAP_START = $SINGLE_MODE_SHARED_HEAP_START;
    $MEM_MAP_SHARED_HEAP_LENGTH_DWORDS = ($SINGLE_MODE_SHARED_HEAP_SIZE + 3) / 4;

    /* PM RAM symbols */
    $PM_INIT_SIZE_DWORDS = (__CODE_RAM_SIZE + 3) / 4;
    $MEM_MAP_PM_INIT_ROM_ADDR = $PM_DATA_START_BASE + ABSOLUTE(__CODE_RAM_address);
    $MEM_MAP_RAM_CODE_START = ($PM_RAM_P0_CODE_START - $PM_RAM_START_ADDRESS) + $DM_WINDOW_TO_PM_BASE_ADDRESS;

    /* DM1 Max address */
    $__dm1_max_address = $DM1_RAM_MAX_ADDRESS;

    /* Heap symbols for start address and size used in the code */
    $__heap_start_addr                    = $DUAL_MODE_HEAP_START;
    $__heap_size                          = $HEAP_SIZE;

    $__single_mode_heap_start_addr        = $SINGLE_MODE_HEAP_START;
    $__single_mode_heap_size              = $SINGLE_MODE_HEAP_SIZE;

    $__shared_heap_start_addr             = $DUAL_MODE_SHARED_HEAP_START;
    $__shared_heap_size                   = $SHARED_HEAP_SIZE;

    $__single_mode_shared_heap_start_addr = $SINGLE_MODE_SHARED_HEAP_START;
    $__single_mode_shared_heap_size       = $SINGLE_MODE_SHARED_HEAP_SIZE;


    /* For ThreadOffload keep the P1 stack and preserved reserved for P1, which is same as original size */
    $__thread_offload_heap_start_addr     = $DUAL_MODE_HEAP_START;
    $__thread_offload_heap_size           = $HEAP_SIZE;

    $__thread_offload_shared_heap_start_addr = $DUAL_MODE_SHARED_HEAP_START;
    $__thread_offload_shared_heap_size       = $SHARED_HEAP_SIZE;

    /* Export the PM heap and patch code start address as linker symbols*/
    $__pm_heap_start_addr = ALIGN($PM_RAM_P0_CODE_START + __CODE_RAM_SIZE, 4);

    $PM_RAM_PATCH_CODE_START = ALIGN($CODE_MAGIC$ + $__pm_heap_start_addr, 4);
    $__pm_patch_code_start_addr = $PM_RAM_PATCH_CODE_START - $CODE_MAGIC$;

    /* Expose P1 start addr to the C code */
    $__mem_map_p1_code_start_address = $MEM_MAP_P1_CODE_START_ADDRESS;

    /* Export the checksum symbol so it can be referenced in C and included in the SLT */
    $__devtools_image_checksum = __devtools_image_checksum;

    /* Used by AudioPatchMaker. */
    $DATA_ROM_WINDOW = $DM_NVRAM0_START_ADDRESS;

    /* Used by ACAT. */
    $PM_RAM_DM_WINDOW_BASE_ADDRESS = $DM_WINDOW_TO_PM_BASE_ADDRESS;
}
