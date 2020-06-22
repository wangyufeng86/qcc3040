/**
 * COMMERCIAL IN CONFIDENCE
 * Copyright (c) 2009 - 2018 Qualcomm Technologies International, Ltd.
 *
 *
 ****************************************************************************
 * \file pl_malloc.c
 * \ingroup pl_malloc
 *
 *
 ****************************************************************************/


/****************************************************************************
Include Files
*/
#include "pl_malloc_private.h"


/****************************************************************************
Private Macro Declarations
*/

/**
 * Macro for the size of the header information.
 * This is a bit ugly - we define the block sizes in "words", but to keep
 * our tests working on 64-bit machines a "word" is actually a uintptr_t
 * This is the same size as a pointer - 24 bits on Kalimba, 32 bits on x86 and
 * 64 bits on 64-bit machines. This ensures that a structure that fits in a
 * given size pool in Kalimba builds will also fit in the same (or smaller)
 * pool in unit test builds.
 *
 * The -1 / +1 in the macro allow for the header size not being an exact
 * multiple of the "word" size.
 */

#define PL_MEM_BLOCK_OVERHEAD_WORDS ((sizeof(tPlMemoryBlockHeader) - 1)/sizeof(uintptr_t) + 1)

/**
 * Macro to calculate the number of words needed for a single block in a
 * pool, given the block size in words. Adds space for the block header
 */
#ifdef PMALLOC_DEBUG
/* Add extra guard words at the end of each block to check for corruption */
#define PL_CALC_MEM_POOL_WORDS(PoolSizeWords) ((PoolSizeWords) + PL_MEM_BLOCK_OVERHEAD_WORDS + 1)
#define PMALLOC_DEBUG_GUARD_0 0xFEDCBAul
#define PMALLOC_DEBUG_GUARD_1 0x012345ul
#else
#define PL_CALC_MEM_POOL_WORDS(PoolSizeWords) ((PoolSizeWords) + PL_MEM_BLOCK_OVERHEAD_WORDS)
#endif

/****************************************************************************
Global Variable Definitions
*/

/****************************************************************************
Private Type Declarations
*/

/**
 * tPlMemoryBlockHeader - header at the start of each memory block
 *
 * Contains either a pointer to the next block (for blocks that are free)
 * or the index of the pool which the block came from (for blocks that
 * are allocated)
 *
 * If PMALLOC_DEBUG is defined, the header also contains the file / line
 * of the pmalloc caller, to assist debugging of memory leaks etc.
 * This is conditional as it imposes a significant memory overhead.
 */
typedef struct tPlMemoryBlockHeaderTag
{
    union
    {
        struct tPlMemoryBlockHeaderTag *pNextBlock; /**< For free blocks */
        unsigned int poolIndex; /**< For alloced blocks */
    } u;
#ifdef PMALLOC_DEBUG
    const char *file;
    unsigned int line;
#endif
} tPlMemoryBlockHeader;

/**
 * tPlMemoryPoolControlStruct - Control structure for memory management
 *
 * We have one of these structures for each memory pool.
 */
typedef struct
{
    /**
     * Size (in words) of the blocks in this pool.
     * Excludes the size of the header
     */
    const int blockSizeWords;
     /** Number of free blocks in pool */
    int numBlocksFree;
     /**
      * Minimum number of free blocks.
      * This is intended to help with post-mortem debugging (coredumps etc)
      * and hence is always enabled and not protected by PMALLOC_DEBUG
      */
    int minBlocksFree;
    /** Pointer to the first free block in pool */
    tPlMemoryBlockHeader *pFirstFreeBlock;
} tPlMemoryPoolControlStruct;


/* 
 * Definitions that apply with either static or dynamically-allocated pool arrays
 */

#define PL_NUM_DM1_MEM_POOLS 3
#define PL_NUM_DM2_MEM_POOLS 3

/* Total number of pools */
#define PL_NUM_MEM_POOLS (PL_NUM_DM1_MEM_POOLS + PL_NUM_DM2_MEM_POOLS)
#define DM2_POOL(pool) (pool + PL_NUM_DM1_MEM_POOLS)

#define MEM_POOL_0_BLOCK_SIZE_WORDS 4
#ifdef INSTALL_METADATA
#define MEM_POOL_1_BLOCK_SIZE_WORDS 7
#else
#define MEM_POOL_1_BLOCK_SIZE_WORDS 6
#endif
#define MEM_POOL_2_BLOCK_SIZE_WORDS 12


#if defined (UNIT_TEST_BUILD) && defined (PMALLOC_DYNAMIC_POOLS)

/* Define some small fixed block counts that can be allocated from the default heap */
#define MEM_POOL_0_NUM_BLOCKS 8
#define MEM_POOL_0_NUM_RESERVED_BLOCKS 4

#define MEM_POOL_1_NUM_BLOCKS 8
#define MEM_POOL_1_NUM_RESERVED_BLOCKS 4

#define MEM_POOL_2_NUM_BLOCKS 8
#define MEM_POOL_2_NUM_RESERVED_BLOCKS 4

#define DM2_MEM_POOL_0_NUM_BLOCKS 8
#define DM2_MEM_POOL_0_NUM_RESERVED_BLOCKS 0

#define DM2_MEM_POOL_1_NUM_BLOCKS 8
#define DM2_MEM_POOL_1_NUM_RESERVED_BLOCKS 0

#define DM2_MEM_POOL_2_NUM_BLOCKS 6
#define DM2_MEM_POOL_2_NUM_RESERVED_BLOCKS 0

#else /* defined (UNIT_TEST_BUILD) && defined (PMALLOC_DYNAMIC_POOLS) */

/**
 * \name malloc_pool_size_macros
 * Macros for the number of pools and the size and number of blocks in each pool
 *
 * See B-201725 for some rationale behind these particular numbers.
 */
/**@{*/
/* The number of blocks is calculated based on the total available RAM on the
 * device in Kwords according to:
 * POOL_NUM_BLOCKS = BASE_NUM_BLOCKS + (NUM_BLOCKS_COEFF * DEVICE_RAM)
 *
 * This number of blocks is then split between the RAM instances
 */

#define BASE_NUM_BLOCKS_POOL_0 60
#define BASE_NUM_BLOCKS_POOL_1 90
#define BASE_NUM_BLOCKS_POOL_2 80
#define POOL_0_NUM_BLOCKS_COEFF 0.75
#define POOL_1_NUM_BLOCKS_COEFF 1.125
#define POOL_2_NUM_BLOCKS_COEFF 1

/** Macro to determine the number of blocks for a given pool for a given RAM
 * instance. */
#ifdef NUM_POOL_BLOCKS_PER_INSTANCE_0
#define NUM_POOL_BLOCKS_PER_INSTANCE(pool) NUM_POOL_BLOCKS_PER_INSTANCE_##pool
#else
#define NUM_POOL_BLOCKS_PER_INSTANCE(pool) (((BASE_NUM_BLOCKS_POOL_##pool) + \
                                 ((unsigned)(POOL_##pool##_NUM_BLOCKS_COEFF * \
                                 MAX_POOL_BLOCKS_PER_INSTANCE))) /2)
#endif /* NUM_POOL_BLOCKS_PER_INSTANCE_0 */

#define MEM_POOL_0_NUM_BLOCKS NUM_POOL_BLOCKS_PER_INSTANCE(0)
#define MEM_POOL_0_NUM_RESERVED_BLOCKS 4

#define MEM_POOL_1_NUM_BLOCKS NUM_POOL_BLOCKS_PER_INSTANCE(1)
#define MEM_POOL_1_NUM_RESERVED_BLOCKS 4

#define MEM_POOL_2_NUM_BLOCKS NUM_POOL_BLOCKS_PER_INSTANCE(2)
#define MEM_POOL_2_NUM_RESERVED_BLOCKS 4

#ifndef DM2_EXTRA_POOL0_BLOCKS
#define  DM2_EXTRA_POOL0_BLOCKS 0
#endif

#ifndef DM2_EXTRA_POOL1_BLOCKS
#define  DM2_EXTRA_POOL1_BLOCKS 0
#endif

#ifndef DM2_EXTRA_POOL2_BLOCKS
#define  DM2_EXTRA_POOL2_BLOCKS 0
#endif

/* For DM2 pools, only define block counts. Sizes must match DM1 */
#define DM2_MEM_POOL_0_NUM_BLOCKS (NUM_POOL_BLOCKS_PER_INSTANCE(0) + DM2_EXTRA_POOL0_BLOCKS)
#define DM2_MEM_POOL_0_NUM_RESERVED_BLOCKS 0

#define DM2_MEM_POOL_1_NUM_BLOCKS (NUM_POOL_BLOCKS_PER_INSTANCE(1) + DM2_EXTRA_POOL1_BLOCKS)
#define DM2_MEM_POOL_1_NUM_RESERVED_BLOCKS 0

#define DM2_MEM_POOL_2_NUM_BLOCKS (NUM_POOL_BLOCKS_PER_INSTANCE(2) + DM2_EXTRA_POOL2_BLOCKS)
#define DM2_MEM_POOL_2_NUM_RESERVED_BLOCKS 0

#endif /* defined (UNIT_TEST_BUILD) && defined (PMALLOC_DYNAMIC_POOLS) */

#define PMALLOC_PRIVATE_MEM_GUARD 0xDEAD

#ifdef PMALLOC_DYNAMIC_POOLS

#ifndef PMALLOC_DEBUG

#define INIT_MEM_POOL_CTRL_DYN(i, p, mem) \
    do { \
        aMemoryPoolControl[i].pFirstFreeBlock = (tPlMemoryBlockHeader *)(mem); \
        aMemoryPoolControl[i].numBlocksFree = pool_block_counts[i]; \
        aMemoryPoolControl[i].minBlocksFree = pool_block_counts[i]; \
    } while (0)

#else /* !PMALLOC_DEBUG */

#define INIT_MEM_POOL_CTRL_DYN(i, p, mem) \
    do { \
        aMemoryPoolControl[i].pFirstFreeBlock = (tPlMemoryBlockHeader *)(mem); \
        aMemoryPoolControl[i].numBlocksFree = pool_block_counts[i]; \
        aMemoryPoolControl[i].minBlocksFree = pool_block_counts[i]; \
        memory_pool_limits[i].pool_start = (mem); \
        memory_pool_limits[i].pool_end = (mem) + PL_CALC_MEM_POOL_WORDS(MEM_POOL_##p##_BLOCK_SIZE_WORDS) * pool_block_counts[i]; \
    } while (0)
#endif /* !PMALLOC_DEBUG */

#else /* PMALLOC_DYNAMIC_POOLS */

#ifndef PMALLOC_DEBUG

#define INIT_MEM_POOL_CTRL(i, p, d, mem) \
        aMemoryPoolControl[i].pFirstFreeBlock = (tPlMemoryBlockHeader *)(mem)

#else /* !PMALLOC_DEBUG */

#define INIT_MEM_POOL_CTRL(i, p, d, mem) \
    do { \
        aMemoryPoolControl[i].pFirstFreeBlock = (tPlMemoryBlockHeader *)(mem); \
        memory_pool_limits[i].pool_start = (mem); \
        memory_pool_limits[i].pool_end = (mem) + \
                 PL_CALC_MEM_POOL_WORDS(MEM_POOL_##p##_BLOCK_SIZE_WORDS)\
                 * d##MEM_POOL_##p##_NUM_BLOCKS; \
    } while (0)
#endif /* !PMALLOC_DEBUG */

#endif /* PMALLOC_DYNAMIC_POOLS */

#define INIT_MEM1_POOL_CTRL(core, pool) \
          INIT_MEM_POOL_CTRL(pool,pool, ,P##core##MemPool##pool)

#define INIT_MEM2_POOL_CTRL(core, pool) \
          INIT_MEM_POOL_CTRL(PL_NUM_DM1_MEM_POOLS+ pool, pool , \
                             DM2_, P##core##Mem2Pool##pool)

#define INIT_P0_MEM_POOL( dm, pool ) INIT_MEM##dm##_POOL_CTRL( 0, pool )
#define INIT_P1_MEM_POOL( dm, pool ) INIT_MEM##dm##_POOL_CTRL( 1, pool )

#define UNUSED_MEM2_POOL(core, pool) (tPlMemoryBlockHeader *) P##core##Mem2Pool##pool
#define UNUSED_MEM1_POOL(core, pool) (tPlMemoryBlockHeader *) P##core##MemPool##pool

#define UNUSED_P0_MEM_POOL(dm, pool) UNUSED_MEM##dm##_POOL( 0, pool )
#define UNUSED_P1_MEM_POOL(dm, pool) UNUSED_MEM##dm##_POOL( 1, pool )

#if defined(SUPPORTS_MULTI_CORE)
/* Touch and validate the unused pools using a temp variable 't' while initilising
 * the right pool to avoid compiler ignoring it (it is a bit ugly to add
 * this dummy statement to force the compiler to keep the unused pool. Do not
 * keep this validation under PMALLOC_DEBUG until we have a better way
 * of keeping unused pool without writing into the unused pool
 * locations).
 * Both pools must be linked to calculate the free ram available
 * for the heap size.
 */
#define INIT_P0_DM1_MEM_POOL(pool) do {  \
                                    tPlMemoryBlockHeader* t = UNUSED_P1_MEM_POOL(1, pool);\
                                    if((GET_POOLINDEX(t->u.poolIndex) != pool) && \
                                       (t->u.pNextBlock != NULL ) && \
                                       (t->u.pNextBlock < t )) \
                                        panic(PANIC_AUDIO_INVALID_POOL_INFO);\
                                    INIT_P0_MEM_POOL(1, pool); \
                                   } while(0)
#define INIT_P0_DM2_MEM_POOL(pool) do {  \
                                    tPlMemoryBlockHeader *t = UNUSED_P1_MEM_POOL(2, pool); \
                                    if((GET_POOLINDEX(t->u.poolIndex) != (pool + PL_NUM_DM1_MEM_POOLS)) && \
                                       (t->u.pNextBlock != NULL ) && \
                                       (t->u.pNextBlock < t )) \
                                        panic(PANIC_AUDIO_INVALID_POOL_INFO);\
                                    INIT_P0_MEM_POOL(2, pool); \
                                    } while(0)
#define INIT_P1_DM1_MEM_POOL(pool) do {  \
                                    tPlMemoryBlockHeader *t = UNUSED_P0_MEM_POOL(1, pool); \
                                    if((GET_POOLINDEX(t->u.poolIndex) != pool) && \
                                       (t->u.pNextBlock != NULL ) && \
                                       (t->u.pNextBlock < t )) \
                                        panic(PANIC_AUDIO_INVALID_POOL_INFO);\
                                    INIT_P1_MEM_POOL(1, pool); \
                                    } while(0)
#define INIT_P1_DM2_MEM_POOL(pool) do {  \
                                    tPlMemoryBlockHeader *t =  UNUSED_P0_MEM_POOL(2, pool); \
                                    if((GET_POOLINDEX(t->u.poolIndex) != (pool + PL_NUM_DM1_MEM_POOLS)) && \
                                       (t->u.pNextBlock != NULL ) && \
                                       (t->u.pNextBlock < t )) \
                                        panic(PANIC_AUDIO_INVALID_POOL_INFO);\
                                    INIT_P1_MEM_POOL(2, pool); \
                                    } while(0)
#else
#define INIT_P0_DM1_MEM_POOL(pool) do {  \
                                    INIT_P0_MEM_POOL(1, pool); \
                                   } while(0)
#define INIT_P0_DM2_MEM_POOL(pool) do {  \
                                    INIT_P0_MEM_POOL(2, pool); \
                                    } while(0)
#define INIT_P1_DM1_MEM_POOL(pool)
#define INIT_P1_DM2_MEM_POOL(pool)
#endif /* defined(SUPPORTS_MULTI_CORE) */

/**@}*/


/****************************************************************************
Private Constant Declarations
*/

/****************************************************************************
Private Variable Definitions
*/

/* The sections are valid on all KAL_ARCH4 platforms that use kld.
 * Other platforms support a subset, with unsupported sections mapped to defaults.
 */

#if defined(KAL_ARCH4)
DM_MEM_GUARD unsigned private_mem_guard = PMALLOC_PRIVATE_MEM_GUARD;
#endif

#ifdef PMALLOC_DYNAMIC_POOLS

/* For dynamic pools, just pointers for this core */
static uintptr_t *memPool0, *memPool1, *memPool2, *memPoolEnd;
static uintptr_t *mem2Pool0, *mem2Pool1, *mem2Pool2, *mem2PoolEnd;

/* Fill in default values for block counts. These might change later */
static unsigned pool_block_counts[PL_NUM_MEM_POOLS] = 
{
    MEM_POOL_0_NUM_BLOCKS,
    MEM_POOL_1_NUM_BLOCKS,
    MEM_POOL_2_NUM_BLOCKS,

    DM2_MEM_POOL_0_NUM_BLOCKS,
    DM2_MEM_POOL_1_NUM_BLOCKS,
    DM2_MEM_POOL_2_NUM_BLOCKS
};

#if defined(INSTALL_MIB) && !defined(UNIT_TEST_BUILD)
#if defined(SUPPORTS_MULTI_CORE)
/* Fill in default values for block counts. These might change later */
DM_SHARED static unsigned pool_block_counts_p1[PL_NUM_MEM_POOLS] = 
{
    MEM_POOL_0_NUM_BLOCKS,
    MEM_POOL_1_NUM_BLOCKS,
    MEM_POOL_2_NUM_BLOCKS,

    DM2_MEM_POOL_0_NUM_BLOCKS,
    DM2_MEM_POOL_1_NUM_BLOCKS,
    DM2_MEM_POOL_2_NUM_BLOCKS
};
#else
#define pool_block_counts_p1 pool_block_counts
#endif /* defined(SUPPORTS_MULTI_CORE) */
#endif /* defined(INSTALL_MIB) && !defined(UNIT_TEST_BUILD) */

#define PL_CALC_DM1_DYN_POOL_WORDS(n) PL_CALC_MEM_POOL_WORDS(MEM_POOL_##n##_BLOCK_SIZE_WORDS) * pool_block_counts[n]
#define PL_CALC_DM2_DYN_POOL_WORDS(n) PL_CALC_MEM_POOL_WORDS(MEM_POOL_##n##_BLOCK_SIZE_WORDS) * pool_block_counts[PL_NUM_DM1_MEM_POOLS + n]

static tPlMemoryPoolControlStruct aMemoryPoolControl[PL_NUM_MEM_POOLS] =
{
    { MEM_POOL_0_BLOCK_SIZE_WORDS, 0, 0, NULL },
    { MEM_POOL_1_BLOCK_SIZE_WORDS, 0, 0, NULL },
    { MEM_POOL_2_BLOCK_SIZE_WORDS, 0, 0, NULL },

    { MEM_POOL_0_BLOCK_SIZE_WORDS, 0, 0, NULL },
    { MEM_POOL_1_BLOCK_SIZE_WORDS, 0, 0, NULL },
    { MEM_POOL_2_BLOCK_SIZE_WORDS, 0, 0, NULL }
};

/* At boot all allocations will use heap */
static unsigned heap_threshold = 0;

#else /* PMALLOC_DYNAMIC_POOLS */

/* Pools for P0 */
static DM1_POOLS0 uintptr_t P0MemPool0[PL_CALC_MEM_POOL_WORDS(MEM_POOL_0_BLOCK_SIZE_WORDS) * MEM_POOL_0_NUM_BLOCKS];
static DM1_POOLS0 uintptr_t P0MemPool1[PL_CALC_MEM_POOL_WORDS(MEM_POOL_1_BLOCK_SIZE_WORDS) * MEM_POOL_1_NUM_BLOCKS];
static DM1_POOLS0 uintptr_t P0MemPool2[PL_CALC_MEM_POOL_WORDS(MEM_POOL_2_BLOCK_SIZE_WORDS) * MEM_POOL_2_NUM_BLOCKS];

static DM2_POOLS0 uintptr_t P0Mem2Pool0[PL_CALC_MEM_POOL_WORDS(MEM_POOL_0_BLOCK_SIZE_WORDS) * DM2_MEM_POOL_0_NUM_BLOCKS];
static DM2_POOLS0 uintptr_t P0Mem2Pool1[PL_CALC_MEM_POOL_WORDS(MEM_POOL_1_BLOCK_SIZE_WORDS) * DM2_MEM_POOL_1_NUM_BLOCKS];
static DM2_POOLS0 uintptr_t P0Mem2Pool2[PL_CALC_MEM_POOL_WORDS(MEM_POOL_2_BLOCK_SIZE_WORDS) * DM2_MEM_POOL_2_NUM_BLOCKS];

#if defined(SUPPORTS_MULTI_CORE)
/* Pools for P1 */
static DM1_POOLS1 uintptr_t P1MemPool0[PL_CALC_MEM_POOL_WORDS(MEM_POOL_0_BLOCK_SIZE_WORDS) * MEM_POOL_0_NUM_BLOCKS];
static DM1_POOLS1 uintptr_t P1MemPool1[PL_CALC_MEM_POOL_WORDS(MEM_POOL_1_BLOCK_SIZE_WORDS) * MEM_POOL_1_NUM_BLOCKS];
static DM1_POOLS1 uintptr_t P1MemPool2[PL_CALC_MEM_POOL_WORDS(MEM_POOL_2_BLOCK_SIZE_WORDS) * MEM_POOL_2_NUM_BLOCKS];

static DM2_POOLS1 uintptr_t P1Mem2Pool0[PL_CALC_MEM_POOL_WORDS(MEM_POOL_0_BLOCK_SIZE_WORDS) * DM2_MEM_POOL_0_NUM_BLOCKS];
static DM2_POOLS1 uintptr_t P1Mem2Pool1[PL_CALC_MEM_POOL_WORDS(MEM_POOL_1_BLOCK_SIZE_WORDS) * DM2_MEM_POOL_1_NUM_BLOCKS];
static DM2_POOLS1 uintptr_t P1Mem2Pool2[PL_CALC_MEM_POOL_WORDS(MEM_POOL_2_BLOCK_SIZE_WORDS) * DM2_MEM_POOL_2_NUM_BLOCKS];
#endif /* defined(SUPPORTS_MULTI_CORE) */

static tPlMemoryPoolControlStruct aMemoryPoolControl[PL_NUM_MEM_POOLS] =
{
    { MEM_POOL_0_BLOCK_SIZE_WORDS, MEM_POOL_0_NUM_BLOCKS, MEM_POOL_0_NUM_BLOCKS, NULL },
    { MEM_POOL_1_BLOCK_SIZE_WORDS, MEM_POOL_1_NUM_BLOCKS, MEM_POOL_1_NUM_BLOCKS, NULL },
    { MEM_POOL_2_BLOCK_SIZE_WORDS, MEM_POOL_2_NUM_BLOCKS, MEM_POOL_2_NUM_BLOCKS, NULL },

    { MEM_POOL_0_BLOCK_SIZE_WORDS, DM2_MEM_POOL_0_NUM_BLOCKS, DM2_MEM_POOL_0_NUM_BLOCKS, NULL },
    { MEM_POOL_1_BLOCK_SIZE_WORDS, DM2_MEM_POOL_1_NUM_BLOCKS, DM2_MEM_POOL_1_NUM_BLOCKS, NULL },
    { MEM_POOL_2_BLOCK_SIZE_WORDS, DM2_MEM_POOL_2_NUM_BLOCKS, DM2_MEM_POOL_2_NUM_BLOCKS, NULL }
};

static unsigned heap_threshold = HEAP_THRESHOLD;

#endif /* PMALLOC_DYNAMIC_POOLS */

#ifdef PMALLOC_DEBUG
static struct
{
    uintptr_t *pool_start;
    uintptr_t *pool_end;
} memory_pool_limits[PL_NUM_MEM_POOLS];
#endif /* PMALLOC_DEBUG */

#ifdef PMALLOC_DYNAMIC_POOLS

/* TODO is it still useful to have reserved blocks ? */
static const int reserved_block_count[PL_NUM_MEM_POOLS] =
{
    0, 0, 0, 
    0, 0, 0
};

#if defined(INSTALL_MIB) && !defined(UNIT_TEST_BUILD)
static const unsigned pool_block_counts_min[PL_NUM_MEM_POOLS] = 
{
    MEM_POOL_0_NUM_BLOCKS / 2,
    MEM_POOL_1_NUM_BLOCKS / 2,
    MEM_POOL_2_NUM_BLOCKS / 2,

    DM2_MEM_POOL_0_NUM_BLOCKS / 2,
    DM2_MEM_POOL_1_NUM_BLOCKS / 2,
    DM2_MEM_POOL_2_NUM_BLOCKS / 2
};
#endif /* defined(INSTALL_MIB) && !defined(UNIT_TEST_BUILD) */

#else /* PMALLOC_DYNAMIC_POOLS */


#define TOTAL_POOL_SIZE \
    (MEM_POOL_0_BLOCK_SIZE_WORDS * MEM_POOL_0_NUM_BLOCKS) + \
    (MEM_POOL_1_BLOCK_SIZE_WORDS * MEM_POOL_1_NUM_BLOCKS) + \
    (MEM_POOL_2_BLOCK_SIZE_WORDS * MEM_POOL_2_NUM_BLOCKS) + \
    (MEM_POOL_0_BLOCK_SIZE_WORDS * DM2_MEM_POOL_0_NUM_BLOCKS) + \
    (MEM_POOL_1_BLOCK_SIZE_WORDS * DM2_MEM_POOL_1_NUM_BLOCKS) + \
    (MEM_POOL_2_BLOCK_SIZE_WORDS * DM2_MEM_POOL_2_NUM_BLOCKS)

/* Array of reserved block counts
 * Separate from the control block array so it can be const
 * Also signed to force correct condition evaluation
 */
static const int reserved_block_count[PL_NUM_MEM_POOLS] =
{
    MEM_POOL_0_NUM_RESERVED_BLOCKS,
    MEM_POOL_1_NUM_RESERVED_BLOCKS,
    MEM_POOL_2_NUM_RESERVED_BLOCKS,

    DM2_MEM_POOL_0_NUM_RESERVED_BLOCKS,
    DM2_MEM_POOL_1_NUM_RESERVED_BLOCKS,
    DM2_MEM_POOL_2_NUM_RESERVED_BLOCKS
};

#endif /* PMALLOC_DYNAMIC_POOLS */

static volatile unsigned oversize_allocation_count;

static pmalloc_cached_report_handler cached_report_handler = NULL;

/****************************************************************************
Private Function Prototypes
*/

/****************************************************************************
Private Function Definitions
*/

#ifdef PMALLOC_DYNAMIC_POOLS
/* When dynamic pools are used, all the pool addresses are also in the heap
 * so the check for calling heap functions is modified to "address not in pools"
 */
static bool is_addr_in_pool(void *addr)
{
    return ((((uintptr_t *)addr >= memPool0) && ((uintptr_t *)addr < memPoolEnd)) ||
            (((uintptr_t *)addr >= mem2Pool0) && ((uintptr_t *)addr < mem2PoolEnd)));
}
#else /* PMALLOC_DYNAMIC_POOLS */
/* Without dynamic pools, heap and pool are separate address ranges */
static inline bool is_addr_in_pool(void *addr)
{
    return !is_addr_in_heap(addr);
}
#endif /* PMALLOC_DYNAMIC_POOLS */

#ifdef PMALLOC_DEBUG
/**
 * \brief debug function to check that a dynamically-allocated block looks
 *        sensible
 * \param[in] pMemory pointer to the block
 * \param[in] id Type of error to report if the pointer was not found.
 * \note If the pointer was found but the associated header was corrupted,
         then PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION is used to panic.
 */
static void pvalidate_internal(void *pMemory, panicid id)
{
    unsigned i;
    unsigned int poolIndex;
    tPlMemoryBlockHeader *pThisBlock;
    uintptr_t *pBlock;
    unsigned int guard;

    if (pMemory == NULL)
    {
        return;
    }

    if (!is_addr_in_pool(pMemory))
    {
        heap_validate(pMemory);
        return;
    }

    /* The pointer does not belong to the heap. If it is valid, then it must
     * be in the pools.
     * Adjust pointer to point to header in this block. Pointer arithmentic
     * means the -1 decrements the pointer by size of the header */
    pThisBlock = ((tPlMemoryBlockHeader *) pMemory) - 1;

    /* At this point, it is unknown whether the pointer is valid or not,
       so scan through all the pool to validate it without dereferencing it. */
    for (i=0; i<PL_NUM_MEM_POOLS; i++)
    {
        if ((uintptr_t *)(pThisBlock) >= memory_pool_limits[i].pool_start &&
            (uintptr_t *)(pThisBlock) < memory_pool_limits[i].pool_end)
        {
            /* The pointer is within one of the pool boundaries so it is safe
               to dereference it. */
            if (i != GET_POOLINDEX(pThisBlock->u.poolIndex))
            {
                /* The pointer is not in the right pool. */
                panic_diatribe(id,
                               (DIATRIBE_TYPE)((uintptr_t)pMemory));
            }
            break;
        }
    }
    if (i == PL_NUM_MEM_POOLS)
    {
        /* The pointer to free is neither in the heaps nor the pools. */
        panic_diatribe(id,
                       (DIATRIBE_TYPE)((uintptr_t)pMemory));
    }
    poolIndex = GET_POOLINDEX(pThisBlock->u.poolIndex);

    /* Check the guard data */
    pBlock = (uintptr_t *)(pThisBlock + 1);
    guard = aMemoryPoolControl[poolIndex].blockSizeWords;
    while ((pBlock[guard] == PMALLOC_DEBUG_GUARD_1) && (guard > 0))
    {
        guard--;
    }
    if ((guard == 0) || (pBlock[guard] != PMALLOC_DEBUG_GUARD_0))
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION,
                       (DIATRIBE_TYPE)((uintptr_t)pMemory));
    }

    /* Check the block really was allocated */
    if (pThisBlock->file == NULL)
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION,
                       (DIATRIBE_TYPE)((uintptr_t)pThisBlock));
    }
}
#endif /* PMALLOC_DEBUG */

/**
 * \brief Try to find a free block in the preferred pool. If preferred pool is not present,
 *  it will fallback
 *
 * \param[in] pool Pool number to look in (ignoring DM1/DM2 split)
 * \param[in] preference choice of DM1, DM2 or don't care
 * \param[out] found_pool actual index of pool (allowing for DM1/DM2 split)
 *
 * \return TRUE if a block was found
 */
/* Try to find a free block in the preferred pool */
static bool is_block_free(int pool, int preference, int *found_pool)
{
    /* Select the DM1 pool as a starting point */
    *found_pool = pool;
    switch (preference)
    {
    case MALLOC_PREFERENCE_DM1:
        /* Just check the default DM1 pool */
        break;
    case MALLOC_PREFERENCE_DM2:
        if (pool < PL_NUM_DM2_MEM_POOLS)
        {
            /* Look in the DM2 pool */
            *found_pool = DM2_POOL(pool);
        }
        else
        {
            /* No DM2 pool, can't allocate anything */
            return FALSE;
        }
        break;
    case MALLOC_PREFERENCE_SYSTEM:
        /* Pick the DM2 pool if it has more free blocks */
        if ((pool < PL_NUM_DM2_MEM_POOLS) &&
            (aMemoryPoolControl[DM2_POOL(pool)].numBlocksFree >
             aMemoryPoolControl[pool].numBlocksFree))
        {
            *found_pool = DM2_POOL(pool);
        }
        return (aMemoryPoolControl[*found_pool].numBlocksFree > 0);

    case MALLOC_PREFERENCE_NONE:
    default:
        /* Pick the DM2 pool if it has more free blocks */
        if ((pool < PL_NUM_DM2_MEM_POOLS) &&
            (aMemoryPoolControl[DM2_POOL(pool)].numBlocksFree - reserved_block_count[DM2_POOL(pool)] >
             aMemoryPoolControl[pool].numBlocksFree - reserved_block_count[pool]))
        {
            *found_pool = DM2_POOL(pool);
        }
        break;
    }

    return (aMemoryPoolControl[*found_pool].numBlocksFree - reserved_block_count[*found_pool] > 0);
}

#ifndef PMALLOC_DYNAMIC_POOLS

/**
 * \brief Initialise the memory control block for static pools.
 *        For compatibility with dual core platforms, this happens while 
 *        initialising pmalloc rather than being statically initialised, 
 *        even on single-core platforms.
 *
 * Note: Do not replace the hardcoded values, there are macro concatenated values.
 */
static void init_mem_pool_ctrl(void)
{
#if defined(KAL_ARCH4)
    /* read from private mem guard. This is placed on top of the
     * private memory and if it is corrupted. panic
     */
    if( private_mem_guard != PMALLOC_PRIVATE_MEM_GUARD )
    {
        panic(PANIC_AUDIO_PRIVATE_MEMORY_CORRUPTED);
    }
#endif

    /* Memory pools must not be initilised at this point. If it was
     * already assigned to the pool control table, then init was
     * called twice, Panic.
     */
    if(aMemoryPoolControl[0].pFirstFreeBlock != NULL)
    {
        panic(PANIC_AUDIO_INVALID_POOL_INFO);
    }

    /* Initialise based on processor id at run time */
    if (PROC_PRIMARY_CONTEXT())
    {
         /* pool = pool num */
        INIT_P0_DM1_MEM_POOL(0);
        INIT_P0_DM1_MEM_POOL(1);
        INIT_P0_DM1_MEM_POOL(2);

        INIT_P0_DM2_MEM_POOL(0);
        INIT_P0_DM2_MEM_POOL(1);
        INIT_P0_DM2_MEM_POOL(2);
    }
    else
    {
         /* pool = pool num */
        INIT_P1_DM1_MEM_POOL(0);
        INIT_P1_DM1_MEM_POOL(1);
        INIT_P1_DM1_MEM_POOL(2);

        INIT_P1_DM2_MEM_POOL(0);
        INIT_P1_DM2_MEM_POOL(1);
        INIT_P1_DM2_MEM_POOL(2);
    }
}

#endif /* PMALLOC_DYNAMIC_POOLS */

/**
 * \brief Initialise the memory pool blocks
 *
 * For each pool, this populates a linked list of free blocks 
 * which is initially all of the blocks, in order.
 *
 */
static void init_mem_pool_blocks(void)
{
    int poolCount;

    for (poolCount = 0; poolCount < PL_NUM_MEM_POOLS; poolCount++)
    {
        tPlMemoryBlockHeader *pCurrentBlock;
        int blockCount, blockSizeWords;
        tPlMemoryPoolControlStruct *pCurrentPool;

        pCurrentPool = &aMemoryPoolControl[poolCount];

        /* Check that the control information for this pool is valid */
        if ((pCurrentPool->blockSizeWords <= 0) || (pCurrentPool->numBlocksFree <= 0) || (pCurrentPool->pFirstFreeBlock == (tPlMemoryBlockHeader *) NULL))
        {
            panic(PANIC_AUDIO_INVALID_POOL_INFO);
        }

#ifdef PMALLOC_DEBUG
        /* If we know what the pool limits are, check that the first block address looks valid */
        if (pCurrentPool->pFirstFreeBlock != (tPlMemoryBlockHeader *)(memory_pool_limits[poolCount].pool_start))
        {
            panic(PANIC_AUDIO_INVALID_POOL_INFO);
        }
#endif

        /* Go through each block in this pool, except the last block,  setting
         * the next free block pointer. Last block has its pointer set to NULL */
        pCurrentBlock = pCurrentPool->pFirstFreeBlock;
        blockSizeWords = PL_CALC_MEM_POOL_WORDS(pCurrentPool->blockSizeWords);
        for (blockCount = 0; blockCount < pCurrentPool->numBlocksFree-1; blockCount++)
        {
            tPlMemoryBlockHeader *pNextBlock;

#ifdef PMALLOC_DEBUG
            /* Initialise the file and line members. */
            pCurrentBlock->file = NULL;
            pCurrentBlock->line = 0;
            /* Initialise the guard word */
            *((uintptr_t *)(pCurrentBlock) + blockSizeWords - 1) = PMALLOC_DEBUG_GUARD_0;
#endif

            /* Calc the address of the next free block. Note careful casting is
             * needed to ensure the pointer arithmetic works correctly */
            pNextBlock = (tPlMemoryBlockHeader *) ((uintptr_t *) pCurrentBlock + blockSizeWords);

            /* Set the header of the curent block to point to the next and
             * update the current pointer for the next iteration */
            pCurrentBlock->u.pNextBlock = pNextBlock;
            pCurrentBlock = pNextBlock;
        }

        /* Set the last pointer to NULL */
        pCurrentBlock->u.pNextBlock = (tPlMemoryBlockHeader *)  NULL;
#ifdef PMALLOC_DEBUG
        /* Initialise the guard word */
        *((uintptr_t *)(pCurrentBlock) + blockSizeWords - 1) = PMALLOC_DEBUG_GUARD_0;
#endif
    }
}

#ifdef PMALLOC_DYNAMIC_POOLS

/**
 * \brief Initialise the memory pool control for dynamic pools
 *
 * This allocates the pool arrays from the heap, then fills in the 
 * control structure with the allocated pointers and block counts.
 *
 */
static void init_mem_pool_ctrl_dynamic(void)
{
    unsigned dm1_size_words = PL_CALC_DM1_DYN_POOL_WORDS(0) +
                              PL_CALC_DM1_DYN_POOL_WORDS(1) + 
                              PL_CALC_DM1_DYN_POOL_WORDS(2);
    unsigned dm2_size_words = PL_CALC_DM2_DYN_POOL_WORDS(0) +
                              PL_CALC_DM2_DYN_POOL_WORDS(1) + 
                              PL_CALC_DM2_DYN_POOL_WORDS(2);

    /* read from private mem guard. This is placed on top of the
     * private memory and if it is corrupted. panic
     */
    if( private_mem_guard != PMALLOC_PRIVATE_MEM_GUARD )
    {
        panic(PANIC_AUDIO_PRIVATE_MEMORY_CORRUPTED);
    }

    /* Allocate heap memory for the dynamic pools.
     * Panic if this fails, because it happens during initialisation
     * and there's no other way of reporting a failure
     */
    memPool0 = heap_alloc(dm1_size_words * sizeof(uintptr_t), MALLOC_PREFERENCE_DM1);
    if (memPool0 == NULL) 
    {
        panic(PANIC_AUDIO_INVALID_POOL_INFO);
    }

    mem2Pool0 = heap_alloc(dm2_size_words * sizeof(uintptr_t), MALLOC_PREFERENCE_DM2);
    if (mem2Pool0 == NULL) 
    {
        panic(PANIC_AUDIO_INVALID_POOL_INFO);
    }

    /* Remaining pointers are just at fixed offsets from the allocated base */
    memPool1 = memPool0 + PL_CALC_DM1_DYN_POOL_WORDS(0);
    memPool2 = memPool1 + PL_CALC_DM1_DYN_POOL_WORDS(1);
    memPoolEnd = memPool2 + PL_CALC_DM1_DYN_POOL_WORDS(2);

    mem2Pool1 = mem2Pool0 + PL_CALC_DM2_DYN_POOL_WORDS(0);
    mem2Pool2 = mem2Pool1 + PL_CALC_DM2_DYN_POOL_WORDS(1);
    mem2PoolEnd = mem2Pool2 + PL_CALC_DM2_DYN_POOL_WORDS(2);

    /* Memory pools must not be initilised at this point. If it was
     * already assigned to the pool control table, then init was
     * called twice, Panic.
     */
    if(aMemoryPoolControl[0].pFirstFreeBlock != NULL)
    {
        panic(PANIC_AUDIO_INVALID_POOL_INFO);
    }

    INIT_MEM_POOL_CTRL_DYN(0, 0, memPool0);
    INIT_MEM_POOL_CTRL_DYN(1, 1, memPool1);
    INIT_MEM_POOL_CTRL_DYN(2, 2, memPool2);

    INIT_MEM_POOL_CTRL_DYN(3, 0, mem2Pool0);
    INIT_MEM_POOL_CTRL_DYN(4, 1, mem2Pool1);
    INIT_MEM_POOL_CTRL_DYN(5, 2, mem2Pool2);
}

#if defined(INSTALL_MIB) && !defined(UNIT_TEST_BUILD)

/* 16-bit values from MIB octet string need endianness swap */
static unsigned swap_16b(uint16 data)
{
    return ((data & 0xff) << 8) + ((data & 0xff00) >> 8);
}

/* Check if new count looks sensible, TRUE if OK, FALSE otherwise */
static bool validate_block_count(int poolnum, unsigned count)
{
    /* Don't reduce below half the static pool value */
    if (count >= pool_block_counts_min[poolnum])
    {
        return TRUE;
    }
    else
    {
        L2_DBG_MSG3("Pool block counts poolnum %d value %u ignored, leaving at %u", 
            poolnum, count, pool_block_counts[poolnum]);
        return FALSE;
    }
}

#endif /* defined(INSTALL_MIB) && !defined(UNIT_TEST_BUILD) */

/**
 * \brief Memory initialisation for dynamic pools
 *
 * Read and validate block counts from MIB, then initialise
 * control structure and pool arrays.
 *
 * Note : this must be called after heap configuration
 */
static void init_pmalloc_dynamic_pools(void)
{
    if (heap_threshold == HEAP_THRESHOLD)
    {
        /* Do not re-initialise pools if heap_threshold is already set since it
         * will overwrite existing allocations.
         */
        return;
    }
#if defined(INSTALL_MIB) && !defined(UNIT_TEST_BUILD)
    if (PROC_PRIMARY_CONTEXT())
    {
        /* For dual-core builds, only read the MIB values on P0 */
        uint16 mib_data[PL_NUM_MEM_POOLS];
        unsigned len_read;
        PROC_ID_NUM core;
        unsigned *pool_block_counts_array[PROC_PROCESSOR_MAX];
        unsigned *counts;
        const mibkey keys[] = { MALLOCPOOLBLOCKCOUNTS, MALLOCP1POOLBLOCKCOUNTS };

        pool_block_counts_array[PROC_PROCESSOR_0] = pool_block_counts;
        pool_block_counts_array[PROC_PROCESSOR_1] = pool_block_counts_p1;        

        for (core=PROC_PROCESSOR_0; core<PROC_PROCESSOR_BUILD; core++)
        {
            counts = pool_block_counts_array[core];
            len_read = mibgetreqstr(keys[core], (uint8 *)mib_data, sizeof(mib_data));
            if (len_read == sizeof(mib_data))
            {
                unsigned poolnum;
                /* Check the MIB values, replace static counts if they look OK */
                for (poolnum = 0; poolnum < PL_NUM_MEM_POOLS; poolnum++)
                {
                    unsigned blockcount = swap_16b(mib_data[poolnum]);
                    if (validate_block_count(poolnum, blockcount))
                    {
                        counts[poolnum] = blockcount;
                    }
                }
            }
            L2_DBG_MSG4("PMALLOC_DYNAMIC_POOLS P%u DM1 block counts: %u %u %u",
                        core, counts[0], counts[1], counts[2]);
            L2_DBG_MSG4("PMALLOC_DYNAMIC_POOLS P%u DM2 block counts: %u %u %u",
                        core, counts[3], counts[4], counts[5]);
        }
    }
    else
    {
        /* Populate P1's block counts with the values initialised by P0 */
        unsigned poolnum;
        for (poolnum = 0; poolnum < PL_NUM_MEM_POOLS; poolnum++)
        {
            pool_block_counts[poolnum] = pool_block_counts_p1[poolnum];
        }
    }
#endif /* defined(INSTALL_MIB) && !defined(UNIT_TEST_BUILD) */

    init_mem_pool_ctrl_dynamic();

    init_mem_pool_blocks();

    /* Now start using the pool blocks for sizes below HEAP_THRESHOLD */
    heap_threshold = HEAP_THRESHOLD;
}

#else /* PMALLOC_DYNAMIC_POOLS */

/**
 * \brief Memory initialisation for static pools
 *
 * This initialises and populates the control structure 
 * and pool arrays.
 *
 * Note : this must be called early in the boot sequence 
 * (before any code can allocate memory)
 */
static void init_pmalloc_static_pools(void)
{
    /* Initialise the Memory pool control block */
    init_mem_pool_ctrl();

    init_mem_pool_blocks();
}

#endif /* PMALLOC_DYNAMIC_POOLS */


/****************************************************************************
Public Function Definitions
*/

/**
 * NAME
 *   init_pmalloc
 *
 * \brief   Initiallise the memory pools
 *
 * \param[in] config  
 *          pointer to P0 heap_config object. This object holds
 *          the pmalloc configuration for all processors. The
 *          pmalloc configuration for all processors is done
 *          by P0, and a pointer to it is an input to 'init_
 *          heap' on other processors (P1,P2,P3).
 *          This parameter is ignored on P0.
 *
 * Note: the behaviour is different depending on whether or not dynamically-
 * allocated pools are in use (PMALLOC_DYNAMIC_POOLS).
 * 
 * In both cases initial heap configuration happens here.
 * 
 * With static pools the pool control setup follows.
 * 
 * For dynamic pools on P0, pool setup is deferred until the heap 
 * reconfiguration which happens in config_pmalloc. Dynamic pool config 
 * for P1 happens here, because in that case init_heap does the full heap 
 * configuration.
 *
 */
void init_pmalloc(heap_config *config)
{
    init_heap(config);

#if defined(PMALLOC_DYNAMIC_POOLS)
    if (PROC_SECONDARY_CONTEXT())
    {
        /* For P1, heap init also configures, so we can set up
           the dynamic pools here. */
        init_pmalloc_dynamic_pools();
    }
#else /* PMALLOC_DYNAMIC_POOLS */  
    init_pmalloc_static_pools();
#endif /* PMALLOC_DYNAMIC_POOLS */  

#if defined(RUNNING_ON_KALSIM) && \
    defined(CHIP_BASE_CRESCENDO) && \
    !defined(DESKTOP_TEST_BUILD)
    /*
     * For Hydra system with subsystems (and unit testing) this is done now,
     * because it won't be done later.
     * (We do it here so that unit tests automatically pick it up.
     * We assume there's no MIB or equivalent in any Kalsim builds that
     * we need to initialise first.)
     */
    config_pmalloc();
#endif /* RUNNING_ON_KALSIM && CHIP_BASE_CRESCENDO */

#if defined(__GNUC__) && defined(UNIT_TEST_BUILD) && defined(PMALLOC_DYNAMIC_POOLS)
    init_pmalloc_dynamic_pools();
#endif
}


#ifndef __GNUC__
/**
 * NAME
 *   config_pmalloc
 *
 * \brief configure the memory pools and heap
 *        This will be called only after completely booting up
 *        the audio subsystem
 */
void config_pmalloc(void)
{
    /* First configure the heap */
    config_heap();

#ifdef PMALLOC_DYNAMIC_POOLS 
    /* Dynamic pool setup happens after configuring the heap */
    init_pmalloc_dynamic_pools();
#endif /* PMALLOC_DYNAMIC_POOLS */
}

#endif /* !__GNUC__  */

/**
 * NAME
 *   xppmalloc
 *
 * \brief Memory allocation, based on memory pools (does not panic, does not zero)
 *
 * FUNCTION
 *   Allocate a chunk of memory from one of the memory pools pointed to from
 *   aMemoryPoolControl. A pointer to the smallest availaible block is
 *   returned. Returns a Null pointer if no suitable block is available.
 *   The memory is not initialised.
 *
 * \param[in] numBytes number of bytes required, as returned by sizeof. See NOTES
 * for details of what a byte is (its not necessarily 8 bits!)
 * \param[in] preference choice of DM1, DM2 or don't care
 *
 * \return pointer to the block of memory allocated
 *
 * \note
 *   Here a "byte" is the smallest addressable storage unit on the processor
 *   used. This is 8 bits on a pentium or 24 bits on Kalimba. The code is written
 *   so that a call xpmalloc(sizeof(SomeStruct)) will work as expected
 *
 */
#ifdef PMALLOC_DEBUG
void *xppmalloc_debug(unsigned int numBytes, unsigned int preference, const char *file, unsigned int line)
#else
void *xppmalloc(unsigned int numBytes, unsigned int preference)
#endif
{
    int poolCount, found_pool;
    int numWords;
    bool poolFull = FALSE;

    /* xpmalloc must return Null if requested to allocate Zero bytes
     * External malloc must be allocated using ext_malloc interface 
     */
    if((numBytes <= 0) || (preference == MALLOC_PREFERENCE_EXTERNAL))
    {
        return(NULL);
    }

    /* Shared and fast mem preference always go with heap */
    if ((numBytes > heap_threshold * sizeof(uintptr_t)) ||
        (preference == MALLOC_PREFERENCE_FAST) ||
        (preference == MALLOC_PREFERENCE_SHARED) )
    {
        void *heap_mem;
#ifdef PMALLOC_DEBUG
        heap_mem = heap_alloc_debug(numBytes, preference, file, line);
#else
        heap_mem = heap_alloc(numBytes, preference);
#endif
        return heap_mem;
    }

    numWords = (numBytes - 1)/sizeof(uintptr_t) + 1;

    /* Go through all the pools, till we find one big enough and with free pools */
    for(poolCount = 0; poolCount < PL_NUM_DM1_MEM_POOLS; poolCount++)
    {
        if (aMemoryPoolControl[poolCount].blockSizeWords >= numWords)
        {
            LOCK_INTERRUPTS;
            if (is_block_free(poolCount, preference, &found_pool))
            {
                tPlMemoryBlockHeader *pThisBlock, *pNextBlock;
                void *pPointer;

                /* There are available blocks in this pool. Allocate one,
                   unlock interrupts and break out of the for loop */
                aMemoryPoolControl[found_pool].numBlocksFree--;

                /* Update the minimum-free tracking */
                if (aMemoryPoolControl[found_pool].numBlocksFree < aMemoryPoolControl[found_pool].minBlocksFree)
                {
                    aMemoryPoolControl[found_pool].minBlocksFree = aMemoryPoolControl[found_pool].numBlocksFree;
                }

                pThisBlock = aMemoryPoolControl[found_pool].pFirstFreeBlock;

#ifdef PMALLOC_DEBUG
                /* Check that the block points into the right pool */
                if ((uintptr_t *)(pThisBlock) < memory_pool_limits[found_pool].pool_start ||
                    (uintptr_t *)(pThisBlock) >= memory_pool_limits[found_pool].pool_end)
                {
                    panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION, (DIATRIBE_TYPE)((uintptr_t)pThisBlock));
                }
#endif

                /* Update the head pointer for this pool to point to the next block */
                pNextBlock = pThisBlock->u.pNextBlock;
                aMemoryPoolControl[found_pool].pFirstFreeBlock = pNextBlock;

                /*Update the header for this block to indicate which pool the block came from */
                pThisBlock->u.poolIndex = SET_POOLINDEX(found_pool);

                /* Do we need to report an oversize allocation ? */
                if (poolFull)
                {
                    oversize_allocation_count++;
                }

#ifdef PMALLOC_DEBUG
                /* Check that the block looks free */
                if (pThisBlock->file != NULL)
                {
                    panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION, (DIATRIBE_TYPE)((uintptr_t)pThisBlock));
                }

                pThisBlock->file = file;
                pThisBlock->line = line;

                /* Check the guard word */
                if (*((uintptr_t *)(pThisBlock + 1) +
                    aMemoryPoolControl[found_pool].blockSizeWords) != PMALLOC_DEBUG_GUARD_0)
                {
                    panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION, (DIATRIBE_TYPE)((uintptr_t)pThisBlock));
                }
#endif

                /* Set output pointer to point at the memory after the block header.
                 * Pointer arithmetic means +1 increments pThisBlock pointer by size of header*/
                pPointer = (void  *) (pThisBlock + 1);

                UNLOCK_INTERRUPTS;

#ifdef PMALLOC_DEBUG
                /* Fill in the unused part of the block with some known data
                 * so we can see if any of it got overwritten
                 */
                {
                    uintptr_t *pBlock = (uintptr_t *)pPointer;
                    unsigned int guard;

                    /* There is always at least one guard word, hence the +1 below */
                    for (guard = numWords+1; guard < aMemoryPoolControl[found_pool].blockSizeWords+1; guard++)
                    {
                        pBlock[guard] = PMALLOC_DEBUG_GUARD_1;
                    }
                    pBlock[numWords] = PMALLOC_DEBUG_GUARD_0;
                }
#endif

                PL_PRINT_P3(TR_PL_MALLOC, "PL Malloc for %i 'bytes' - pointer %p allocated from pool %u\n",
                                           numBytes, pPointer, found_pool);
                #ifdef PL_DEBUG_MEM_ON_HOST
                printf("[MEM] 0x%x alloc of %i bytes ",pPointer, numBytes);
                #endif

                return(pPointer);
            }
            /* If we get here then the current memory pool has blocks big enough,
             * but they are all used. Need to unlock IRQs and try next pool */
            UNLOCK_INTERRUPTS;
            PL_PRINT_P2(TR_PL_MALLOC_POOL_EMPTY, "PL Malloc for %i 'bytes'. Pool %u has blocks big enough, but none left\n",
                                       numBytes, poolCount);
            poolFull = TRUE;

        }
    }

    patch_fn(xppmalloc);

#ifdef POOL_OVERFLOW_TO_HEAP
    if (preference == MALLOC_PREFERENCE_SYSTEM)
    {
        /* See if we can allocate a block from the heap */
#ifdef PMALLOC_DEBUG
        return heap_alloc_debug(numBytes, preference, file, line);
#else
        return heap_alloc(numBytes, preference);
#endif
    }
#endif

    /* If we get here, no block has been allocated. Return NULL  */
    PL_PRINT_P1(TR_PL_MALLOC_FAIL,"PL Malloc for %i 'bytes' failed\n",numBytes);
    return(NULL);
}


/**
 * NAME
 *   ppmalloc
 *
 * \brief Memory allocation (panics if no memory)
 *
 * FUNCTION
 *   As xppmalloc, except that it panics if it does not have enough memory to allocate.
 *
 * \param[in] numBytes number of bytes required, as returned by sizeof.
 * See xppmalloc for details of what a "byte" is.
 * \param[in] preference choice of DM1, DM2 or don't care
 *
 * \return pointer to the block of memory allocated
 *
 */
#ifdef PMALLOC_DEBUG
void *ppmalloc_debug(unsigned int numBytes, unsigned int preference, const char *file, unsigned int line)
#else
void *ppmalloc(unsigned int numBytes, unsigned int preference)
#endif
{
    /* [p]pmalloc must panic if requested to allocate Zero bytes */
    if(numBytes == 0)
    {
        panic(PANIC_AUDIO_REQ_ZERO_MEMORY);
    }

    /* If a version of malloc which can panic is called with no DM1/DM2 preference
     * assume it's a "system" allocation that can eat into the reserved blocks
     */
    if (preference == MALLOC_PREFERENCE_NONE)
    {
        preference = MALLOC_PREFERENCE_SYSTEM;
    }

#ifdef PMALLOC_DEBUG
    void *ptr = xppmalloc_debug(numBytes, preference, file, line);
#else
    void *ptr = xppmalloc(numBytes, preference);
#endif

    /* panic out of memory */
    if(ptr == NULL)
    {
        panic_diatribe(PANIC_AUDIO_HEAP_EXHAUSTION, numBytes);
    }

    return ptr;
}




/**
 * NAME
 *   xzppmalloc
 *
 * \brief Memory allocation and zeroing (returns Null if no memory)
 *
 * FUNCTION
 *   As xppmalloc, except that the allocated memory is zeroed
 *
 * \param[in] numBytes number of bytes required, as returned by sizeof.
 * See xppmalloc for details of what a "byte" is.
 * \param[in] preference choice of DM1, DM2 or don't care
 *
 * \return pointer to the block of memory allocated
 *
 */
#ifdef PMALLOC_DEBUG
void *xzppmalloc_debug(unsigned int numBytes, unsigned int preference, const char *file, unsigned int line)
#else
void *xzppmalloc(unsigned int numBytes, unsigned int preference)
#endif
{

    /* xz[p]pmalloc must return Null if requested to allocate Zero bytes */
    if(numBytes <= 0)
    {
        return(NULL);
    }

#ifdef PMALLOC_DEBUG
    void *ptr = xppmalloc_debug(numBytes, preference, file, line);
#else
    void *ptr = xppmalloc(numBytes, preference);
#endif

    if (ptr)
    {
        memset(ptr, 0, numBytes);
    }

    return ptr;
}



/**
 * NAME
 *   zppmalloc
 *
 * \brief Memory allocation and zeroing (panics if no memory)
 *
 * FUNCTION
 *   As ppmalloc, except that the allocated memory is zeroed
 *
 * \param[in] numBytes number of bytes required, as returned by sizeof.
 * See xppmalloc for details of what a "byte" is.
 * \param[in] preference choice of DM1, DM2 or don't care
 *
 * \return pointer to the block of memory allocated
 *
 */
#ifdef PMALLOC_DEBUG
void *zppmalloc_debug(unsigned int numBytes, unsigned int preference, const char *file, unsigned int line)
#else
void *zppmalloc(unsigned int numBytes, unsigned int preference)
#endif
{
    /* Allocate the memory in pmalloc but we will zero it here */
#ifdef PMALLOC_DEBUG
    void *ptr = ppmalloc_debug(numBytes, preference, file, line);
#else
    void *ptr = ppmalloc(numBytes, preference);
#endif

    if (ptr)
    {
        memset(ptr, 0, numBytes);
    }

    return ptr;
}

/**
 * NAME
 *   pfree
 *
 * \brief free memory allocated using versions of malloc (does not panic if passed Null)
 *
 * FUNCTION
 *   Looks up which memory pool this block cames from, and returns the memory
 *   to the pool. NOTE requires that the memory just above the top of the
 *   buffer pointed to by pMemory contains the block header information, with
 *   the index of pool from which this block came. This will be the case
 *   assuming the memory was allocated using xpmalloc and there has been no
 *   memory corruption
 *
 * \param[in] pMemory pointer to the memory to be freed
 *
 */
void pfree(void *pMemory)
{
    unsigned int poolIndex;
    tPlMemoryBlockHeader *pThisBlock;
    tPlMemoryPoolControlStruct *pThisPool;

    if (pMemory == NULL)
    {
        return;
    }

#ifdef PMALLOC_DEBUG
    pvalidate_internal(pMemory, PANIC_AUDIO_FREE_INVALID);
#endif
    PL_PRINT_P1(TR_PL_FREE,"freeing memory %p\n", pMemory);

    if (!is_addr_in_pool(pMemory))
    {
        PL_PRINT_P0(TR_PL_FREE,"freeing memory from heap\n");
        heap_free(pMemory);
        return;
    }

    PL_PRINT_P0(TR_PL_FREE,"freeing memory from pools\n");

    /* Adjust pointer to point to header in this block. Pointer arithmentic
     * means the -1 decrements the pointer by size of the header */
    pThisBlock = ((tPlMemoryBlockHeader *) pMemory) - 1;

    /* Look up which pool this block came from */
    poolIndex = GET_POOLINDEX(pThisBlock->u.poolIndex);
    if (poolIndex >= PL_NUM_MEM_POOLS)
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID, (DIATRIBE_TYPE)((uintptr_t) pMemory));
    }
    pThisPool = &aMemoryPoolControl[poolIndex];

#ifdef PMALLOC_DEBUG
    {
        uintptr_t *pBlock = (uintptr_t *)pMemory;
        /* Reinstate the end-of-block guard word */
        pBlock[pThisPool->blockSizeWords] = PMALLOC_DEBUG_GUARD_0;
        pThisBlock->file = NULL;
        pThisBlock->line = 0;
    }
#endif

    PL_PRINT_P2(TR_PL_FREE, "Free of pointer %p from pool %u\n", pMemory, poolIndex);

    /* Put this block back into the linked list for this pool */
    LOCK_INTERRUPTS;
    pThisPool->numBlocksFree++;
    pThisBlock->u.pNextBlock = pThisPool->pFirstFreeBlock;
    pThisPool->pFirstFreeBlock = pThisBlock;
    UNLOCK_INTERRUPTS;
}

/**
 * NAME
 *   psizeof
 *
 * \brief get the actual size of a dynamically-allocated block
 *
 * FUNCTION
 *   Looks up which memory pool this block cames from, and returns
 *   the block size for the pool.
 *   NOTE requires that the memory just above the top of the
 *   buffer pointed to by pMemory contains the block header information, with
 *   the index of pool from which this block came. This will be the case
 *   assuming the memory was allocated using xpmalloc and there has been no
 *   memory corruption
 *
 * \param[in] pMemory pointer to the block to get the size of
 *
 */
int psizeof(void *pMemory)
{
    unsigned int poolIndex;
    tPlMemoryBlockHeader *pThisBlock;

    if (pMemory == NULL)
    {
        return 0;
    }

#ifdef PMALLOC_DEBUG
    pvalidate(pMemory);
#endif

    if (!is_addr_in_pool(pMemory))
    {
        return heap_sizeof(pMemory);
    }

    /* Adjust pointer to point to header in this block. */
    pThisBlock = ((tPlMemoryBlockHeader *) pMemory) - 1;

    /* Look up which pool this block came from */
    poolIndex = GET_POOLINDEX(pThisBlock->u.poolIndex);

    if (poolIndex >= PL_NUM_MEM_POOLS)
    {
        /* Probably not a valid dynamically-allocated block.
         * Could panic here, but for now just get out
         */
        return 0;
    }

#ifdef PMALLOC_DEBUG
    /* Adjust guard words, because anyone calling this
     * might use more memory than they originally asked for
     */
    ((uintptr_t *)pMemory)[aMemoryPoolControl[poolIndex].blockSizeWords] = PMALLOC_DEBUG_GUARD_0;
#endif

    return aMemoryPoolControl[poolIndex].blockSizeWords * sizeof(uintptr_t);
}

/**
 * NAME
 *   pool_cur
 *
 * \brief Function for getting the currently available pools size.
 *
 * FUNCTION
 *   Looks up the currently available memory pools, and returns the sum of all the
 *   available memory pool size.
 *
 * \return Returns the size of the currently available pool memory in words.
 */

unsigned pool_cur(void)
{
    unsigned poolCount;
    unsigned total_cur_size = 0;
    /* Calculate the total pool size. */
    /* Go through all the pools to calculate the total pool size. */
    for(poolCount = 0; poolCount < PL_NUM_MEM_POOLS; poolCount++)
    {
        total_cur_size += aMemoryPoolControl[poolCount].blockSizeWords * aMemoryPoolControl[poolCount].numBlocksFree;
    }
    /* Add in any cached data structures */
    if (cached_report_handler != NULL)
    {
        total_cur_size += (cached_report_handler() / sizeof(unsigned));
    }
    return total_cur_size;
}

/**
 * NAME
 *   pool_min
 *
 * \brief Function for getting the minimum available pools size.
 *
 * FUNCTION
 *   Looks up the minimum available memory pools, and returns the sum of all the
 *   minimum available memory pool size.
 *
 * \return Returns the size of the minimum available pool memory in words.
 */
unsigned pool_min(void)
{
    unsigned poolCount;
    unsigned total_min_size = 0;
    /* Calculate the total pool size. */
    /* Go through all the pools to calculate the total pool size. */
    for(poolCount = 0; poolCount < PL_NUM_MEM_POOLS; poolCount++)
    {
        total_min_size += aMemoryPoolControl[poolCount].blockSizeWords * aMemoryPoolControl[poolCount].minBlocksFree;
    }
    return total_min_size;
}

/**
 * NAME
 *   pool_size_total
 *
 * \brief Function for getting the total (used and free) pools size.
 *
 * FUNCTION
 *   This is currently a fixed constant unless PMALLOC_DYNAMIC_POOLS
 *   in which case it's calculated from the configured pool counts.
 *
 * \return Returns total size of the pool memory in words.
 */
unsigned pool_size_total(void)
{
#ifdef PMALLOC_DYNAMIC_POOLS
    return PL_CALC_DM1_DYN_POOL_WORDS(0) +
           PL_CALC_DM1_DYN_POOL_WORDS(1) + 
           PL_CALC_DM1_DYN_POOL_WORDS(2) + 
           PL_CALC_DM2_DYN_POOL_WORDS(0) +
           PL_CALC_DM2_DYN_POOL_WORDS(1) + 
           PL_CALC_DM2_DYN_POOL_WORDS(2);
#else
    return TOTAL_POOL_SIZE;
#endif
}

/**
 * NAME
 *   pool_clear_watermarks
 *
 * \brief Function for clearing the minimum available pools size (watermarks).
 */
void pool_clear_watermarks(void)
{
    unsigned poolCount;
    /* Calculate the total pool size. */
    LOCK_INTERRUPTS;
    /* Go through all the pools to calculate the total pool size. */
    for(poolCount = 0; poolCount < PL_NUM_MEM_POOLS; poolCount++)
    {
        aMemoryPoolControl[poolCount].minBlocksFree = aMemoryPoolControl[poolCount].numBlocksFree;
    }
    UNLOCK_INTERRUPTS;
}

/*
 *   pmalloc_cached_report
 */
pmalloc_cached_report_handler pmalloc_cached_report(pmalloc_cached_report_handler new_handler)
{
    pmalloc_cached_report_handler old_handler = cached_report_handler;
    cached_report_handler = new_handler;
    /* Pass the previous handler back to the caller to save */
    return old_handler;
}

#ifdef PMALLOC_DEBUG
/**
 * NAME
 *   pvalidate
 *
 * \brief debug function to check that a dynamically-allocated block looks sensible
 *
 * \param[in] pMemory pointer to the block
 *
 */
void pvalidate(void *pMemory)
{
    pvalidate_internal(pMemory, PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION);
}
#endif /* PMALLOC_DEBUG */

#ifdef INSTALL_DM_MEMORY_PROFILING
/**
 * NAME
 *   pool_tag_dm_memory
 *
 * \brief Function to 'tag' a block of allocated pool memory
 *
 * \param[in] ptr  Pointer to the allocated block to be tagged
 * \param[in] id   The owner id to tag the allocated block with
 *
 * \return Returns TRUE if  tagging succeeded,
 *                 FALSE if tagging failed (ptr is not a pool block).
 */
static bool pool_tag_dm_memory(void *ptr, DMPROFILING_OWNER id)
{
    tPlMemoryBlockHeader *pThisBlock;
    unsigned int poolIndex;

    /* Adjust pointer to point to header in this block. Pointer arithmentic
     * means the -1 decrements the pointer by size of the header */
    pThisBlock = ((tPlMemoryBlockHeader *) ptr) - 1;

    /* Look up which pool this block came from */
    poolIndex = GET_POOLINDEX(pThisBlock->u.poolIndex);
    if (poolIndex >= PL_NUM_MEM_POOLS)
    {
        return FALSE;
    }
    pThisBlock->u.poolIndex = SET_POOLINDEX_WITH_ID(poolIndex, id);
    return TRUE;
}

/**
 * NAME
 *   pmalloc_tag_dm_memory
 *
 * \brief Function to 'tag' a block of allocated heap or pool memory
 *        with its 'owner id'. The 'owner id' is usually derived from
 *        'get_current_id', but in a handful of cases where the owner
 *        is not the current task (as, for example, in 'create_task'),
 *        this function allows an allocated block to be assigned to
 *        a non-current-task owner.
 *
 * \param[in] ptr  Pointer to the allocated block to be tagged
 * \param[in] id   The owner id to tag the allocated block with
 *
 * \return Returns TRUE if  tagging succeeded,
 *                 FALSE if tagging failed.
 *         FALSE would have been caused by a pointer not pointing
 *         to an allocated heap or pool block.
 */
bool pmalloc_tag_dm_memory(void *ptr, DMPROFILING_OWNER id)
{
    if (is_addr_in_pool(ptr))
    {
        return pool_tag_dm_memory(ptr, id);
    }
    else
    {
        return heap_tag_dm_memory(ptr, id);
    }
}
#endif

/****************************************************************************
Module Test
*/

#ifdef PL_MEM_POOL_TEST
/* Code needed only for testing of the real code in this file */

/**
 * Global data used by the test harness to know how many blocks each pool has
 */
int aPlMemPoolNumBlocks[PL_NUM_MEM_POOLS];
int aPlMem1PoolNumBlocks[PL_NUM_DM1_MEM_POOLS];
int aPlMem2PoolNumBlocks[PL_NUM_DM2_MEM_POOLS];

int aPlMemPoolNumReservedBlocks[PL_NUM_MEM_POOLS];
int aPlMem1PoolNumReservedBlocks[PL_NUM_DM1_MEM_POOLS];
int aPlMem2PoolNumReservedBlocks[PL_NUM_DM2_MEM_POOLS];

/**
 * Global data used by the test harness to know how big the blocks are
 */
int aPlMemPoolBlockSizes[PL_NUM_MEM_POOLS];

int numPools, numDM1Pools, numDM2Pools;

/**
 * NAME
 *   PlMemPoolTest
 *
 *   sets up number of pools and test arrays above
 *
 *   Prototype here to avoid contamination public pl_malloc.h
 *
 * \note   USED FOR MODULE TESTING ONLY
 */
void PlMemPoolTest(void)
{
    int n;

    init_pmalloc(heap_get_heap_config());
    for(n=0; n<PL_NUM_DM1_MEM_POOLS; n++)
    {
        aPlMemPoolBlockSizes[n] = aMemoryPoolControl[n].blockSizeWords;
        aPlMemPoolNumBlocks[n] = aMemoryPoolControl[n].numBlocksFree;
        aPlMem1PoolNumBlocks[n] = aMemoryPoolControl[n].numBlocksFree;
        aPlMemPoolNumReservedBlocks[n] = reserved_block_count[n];
        aPlMem1PoolNumReservedBlocks[n] = reserved_block_count[n];
        if (n < PL_NUM_DM2_MEM_POOLS)
        {
            aPlMemPoolNumBlocks[n] += aMemoryPoolControl[DM2_POOL(n)].numBlocksFree;
            aPlMemPoolNumReservedBlocks[n] += reserved_block_count[DM2_POOL(n)];
        }
    }

    for(n=0; n<PL_NUM_DM2_MEM_POOLS; n++)
    {
        aPlMem2PoolNumBlocks[n] = aMemoryPoolControl[DM2_POOL(n)].numBlocksFree;
        aPlMem2PoolNumReservedBlocks[n] = reserved_block_count[DM2_POOL(n)];
    }

    numPools = PL_NUM_DM1_MEM_POOLS;
    numDM1Pools = PL_NUM_DM1_MEM_POOLS;
    numDM2Pools = PL_NUM_DM2_MEM_POOLS;
}
#endif /* PL_MEM_POOL_TEST */
