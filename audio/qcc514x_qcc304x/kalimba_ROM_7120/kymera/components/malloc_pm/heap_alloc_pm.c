/****************************************************************************
 * Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
 ****************************************************************************
 * \file heap_alloc_pm.c
 * Memory allocation/free functionality for program memory (PM)
 *
 * MODULE : malloc_pm
 *
 * \ingroup malloc_pm
 *
 ****************************************************************************/


/****************************************************************************
Include Files
*/
#include "malloc_pm_private.h"
#if defined(SUPPORTS_MULTI_CORE)
#include "hal/hal.h"
#include "hal/hal_multi_core.h"
#endif
#include "patch.h"
#include "platform/pl_assert.h"
#include "mib/mib.h"

#ifdef INSTALL_THREAD_OFFLOAD
#include "thread_offload.h"
#endif

#include "audio_log/audio_log.h"

/****************************************************************************
Private Macro Declarations
*/

/* PM allocation from core 0 PM heap */
#define HEAP_PM_CORE_0_ALLOC(size)  \
            heap_alloc_internal_pm(size, PM_HEAP_SIZE, &freelist_pm)

#if defined(SUPPORTS_MULTI_CORE)
/* PM allocation from core 1 PM heap */
#define HEAP_PM_CORE_1_ALLOC(size)  \
            heap_alloc_internal_pm(size, PM_P1_HEAP_BEST_SIZE, &freelist_pm_p1)
#else
#define HEAP_PM_CORE_1_ALLOC(size) HEAP_PM_CORE_0_ALLOC(size)
#endif

#if !defined(KAL_ARCH4)
#define READ_NODE_FROM_PM(addr32, node) \
        do { \
        (node)->raw_memory[0] = (((*(volatile uint16*)((volatile uint16*)PM_RAM_MS_WINDOW + (addr32))) & 0xFF) << 16); \
        (node)->raw_memory[0] |= ((*(volatile uint16*)((volatile uint16*)PM_RAM_LS_WINDOW + (addr32))) & 0xFFFF); \
        (node)->raw_memory[1] = (((*(volatile uint16*)((volatile uint16*)PM_RAM_MS_WINDOW + ((addr32) + 1))) & 0xFF) << 16); \
        (node)->raw_memory[1] |= ((*(volatile uint16*)((volatile uint16*)PM_RAM_LS_WINDOW + ((addr32) + 1))) & 0xFFFF); \
        } while (0)

#define WRITE_NODE_TO_PM(addr32, node) \
        do { \
        *(volatile unsigned*)((volatile unsigned*)PM_RAM_MS_WINDOW + (addr32)) = (uint16)((((node)->raw_memory[0])>>16) & 0xFF); \
        *(volatile unsigned*)((volatile unsigned*)PM_RAM_LS_WINDOW + (addr32)) = (uint16)((node)->raw_memory[0] & 0xFFFF); \
        *(volatile unsigned*)((volatile unsigned*)PM_RAM_MS_WINDOW + (addr32 + 1)) = (uint16)((((node)->raw_memory[1])>>16) & 0xFF); \
        *(volatile unsigned*)((volatile unsigned*)PM_RAM_LS_WINDOW + (addr32 + 1)) = (uint16)((node)->raw_memory[1] & 0xFFFF); \
        } while (0)
#else
#define READ_NODE_FROM_PM(addr32, node) \
        do { \
        (node)->raw_memory[0] = (*(volatile uint32*)((volatile uint32*)(PM_RAM_WINDOW - PM_RAM_START_ADDRESS) + (addr32))); \
        (node)->raw_memory[1] = (*(volatile uint32*)((volatile uint32*)(PM_RAM_WINDOW - PM_RAM_START_ADDRESS) + ((addr32) + 1))); \
        } while (0)

#define WRITE_NODE_TO_PM(addr32, node) \
        do { \
        *(volatile unsigned*)((volatile unsigned*)(PM_RAM_WINDOW - PM_RAM_START_ADDRESS) + (addr32)) = (uint32)((node)->raw_memory[0]); \
        *(volatile unsigned*)((volatile unsigned*)(PM_RAM_WINDOW - PM_RAM_START_ADDRESS) + (addr32) + 1)  = (uint32)((node)->raw_memory[1]); \
        } while (0)
#endif

/* Util Macros */
#define HEAP_NODE(x)             ((void_func_ptr)((uintptr_t)(x)))
#define HEAP_SIZE(start,end)    (((uintptr_t)(end) - (uintptr_t)(start)) / PC_PER_INSTRUCTION)

#define PM_HEAP_SIZE            HEAP_SIZE(heap_pm_start, heap_pm_end)

#define IS_IN_PM_P0_HEAP(addr) (((addr) >= heap_pm_start) && ((addr) <= heap_pm_end))

#if defined(SUPPORTS_MULTI_CORE)
#define PM_P1_HEAP_SIZE         HEAP_SIZE(heap_pm_p1_start, heap_pm_p1_end)

#ifdef HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK

#define P1_PM_CACHE_BANK_USABLE_START_ADDR  (PM_RAM_P1_CACHE_START_ADDRESS+(PM_RAM_CACHE_BANK_SIZE_WORDS*ADDR_PER_WORD))
#define P1_PM_CACHE_BANK_USABLE_END_ADDR    PM_RAM_END_ADDRESS
#define P1_PM_CACHE_BANK_USABLE_SIZE        HEAP_SIZE(addnl_heap_pm_p1_start, addnl_heap_pm_p1_end)

#define HEAP_NODE_IN_CACHE_START            HEAP_NODE(P1_PM_CACHE_BANK_USABLE_START_ADDR)
#define HEAP_NODE_IN_CACHE_END              HEAP_NODE(P1_PM_CACHE_BANK_USABLE_END_ADDR)

#define PM_P1_HEAP_BEST_SIZE    MAX(PM_P1_HEAP_SIZE, P1_PM_CACHE_BANK_USABLE_SIZE)

#define IS_IN_PM_P1_HEAP(addr)  ((((addr) >= heap_pm_p1_start) && ((addr) <= heap_pm_p1_end)) \
                   || (((addr) >= HEAP_NODE_IN_CACHE_START) && ((addr) <= HEAP_NODE_IN_CACHE_END)))

#else
#define PM_P1_HEAP_BEST_SIZE    PM_P1_HEAP_SIZE

#define IS_IN_PM_P1_HEAP(addr)  (((addr) >= heap_pm_p1_start) && ((addr) <= heap_pm_p1_end))
#endif /* HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK */

#define IS_IN_PM_HEAP(addr) ((IS_IN_PM_P0_HEAP(addr))||(IS_IN_PM_P1_HEAP(addr)))

#else

#define IS_IN_PM_HEAP(addr) (IS_IN_PM_P0_HEAP(addr))
#define IS_IN_PM_P1_HEAP(addr) FALSE

#endif /* defined(SUPPORTS_MULTI_CORE) */

#define GET_NODE_ADDR32(heap_pm) \
        (((uintptr_t)(heap_pm)) / PC_PER_INSTRUCTION)

/* This macro gets the size of a variable in DM, and normalises it into platform words
 * It's assumed that a word that fits in DM, will fit in PM (e.g. 24 or 32 bit DM words will
 * fit in 32-bit PM words) */
#define SIZE_OF_DM_VAR_IN_PM_32(x) (sizeof(x)/ADDR_PER_WORD)
#define BYTES_INTO_PM_32(x) (x/PC_PER_INSTRUCTION)

#define MAGIC_WORD 0xabcd01ul
#define MIN_SPARE_32 BYTES_INTO_PM_32(32)

/****************************************************************************
Private Type Declarations
*/

typedef union
{
    struct struct_mem_node
    {
        unsigned length_32;
        union
        {
            void_func_ptr next;
            unsigned magic;
        } u;
    } struct_mem_node;
    unsigned raw_memory[2];
} mem_node_pm;

/****************************************************************************
Private Variable Definitions
*/
static unsigned int pm_reserved_size = 0;

static void_func_ptr freelist_pm = NULL;
#if defined(SUPPORTS_MULTI_CORE)
static void_func_ptr freelist_pm_p1 = NULL;
#else
#define freelist_pm_p1 freelist_pm
#endif

/* We might need to correct the address (i.e. Minim addresses set the bottom bit to 1) */
static void_func_ptr heap_pm_start = NULL;
static void_func_ptr heap_pm_end = NULL;

#if defined(SUPPORTS_MULTI_CORE)
static void_func_ptr heap_pm_p1_start = NULL;
static void_func_ptr heap_pm_p1_end = NULL;

#ifdef HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK
static void_func_ptr addnl_heap_pm_p1_start = NULL;
static void_func_ptr addnl_heap_pm_p1_end = NULL;
#endif

#endif /* defined(SUPPORTS_MULTI_CORE) */

/****************************************************************************
Private Function Definitions
*/
/* This is where the PM heap starts */
extern unsigned _pm_heap_start_addr;
#define PM_HEAP_START_ADDR  ((unsigned)((uintptr_t)&(_pm_heap_start_addr)))

static void_func_ptr init_and_adjust_pm_heap(unsigned pm_heap_size, void_func_ptr heap_addr, void_func_ptr next_addr)
{
    patch_fn_shared(heap_alloc_pm_init);
    unsigned int pm_win_value;
    void_func_ptr adjusted_heap_pm;
    /* pm_heap_size is in 32-bit words */
    unsigned freelength = pm_heap_size - SIZE_OF_DM_VAR_IN_PM_32(mem_node_pm);
    mem_node_pm heap_node;

    LOCK_INTERRUPTS;
    /* Save initial state */
    pm_win_value = PMWIN_ENABLE;
    /* Enable access to PM through DM window */
    PMWIN_ENABLE = 1;

    heap_node.struct_mem_node.u.next= next_addr;
    heap_node.struct_mem_node.length_32 = freelength;
    adjusted_heap_pm = heap_addr;
    #if PC_PER_INSTRUCTION == 4
    /* 8-bit addressable architecture, make sure the MiniMode bit is cleared */
    adjusted_heap_pm = (void_func_ptr)(((uintptr_t)adjusted_heap_pm) & ~1);
    #endif
    WRITE_NODE_TO_PM(GET_NODE_ADDR32(adjusted_heap_pm), &heap_node);

    /* Restore initial state */
    PMWIN_ENABLE = pm_win_value;
    UNLOCK_INTERRUPTS;
    return adjusted_heap_pm;
}

#if defined(SUPPORTS_MULTI_CORE)
static inline void configure_pm_heap_multi_core(void)
{
    uint16 num_p1_banks;

    /* Start with P1 heap start set to P1 cache start address and work downwards
     * Use an unsigned to make the calculations simpler */
    unsigned p1_heap_start = PM_RAM_P1_CACHE_START_ADDRESS;

    /* P1 heap always ends at the cache start */
    heap_pm_p1_end = HEAP_NODE(PM_RAM_P1_CACHE_START_ADDRESS);

    if(!mibgetu16(P1PMHEAPALLOCATION, &num_p1_banks) || num_p1_banks > NUMBER_PM_BANKS)
    {
        fault_diatribe(FAULT_AUDIO_INVALID_PM_HEAP_ALLOCATION, num_p1_banks);
        /* Meanwhile use some default values             */
        num_p1_banks = 1;
    }
    if (num_p1_banks)
    {
        /* For non zero P1 PM heap bank values, adjust p1 heap start */
        p1_heap_start -= (PM_BANK_SIZE*num_p1_banks);
        /* Make sure that the P1 heap does not start before the actual
         * P0 heap start.
         */
        if (p1_heap_start < (PM_HEAP_START_ADDR + pm_reserved_size))
        {
            p1_heap_start = (PM_HEAP_START_ADDR + pm_reserved_size);
        }
    }

    /* P1 heap starts at bank boundaries and ends at P1 cache start.
     * So depending on the cache configuration, the calculated heap start
     * might not start at bank boundaries. */
    if (p1_heap_start & (PM_BANK_SIZE-1))
    {
        /* Adjusted P1 heap start is not at bank boundary. Adjust it
         * upwards to the next bank boundary if num_p1_banks was non zero.
         * Otherwise to the start of the bank with the cache tags */
        p1_heap_start = (p1_heap_start & ~(PM_BANK_SIZE-1)) + (num_p1_banks?PM_BANK_SIZE:0);
    }

    /* Setting P1 priority for the banks. This will set the correct access
     * permissions for all of P1's PM banks including the cache banks */
    hal_multi_core_configure_pm_bank_access_control(1, p1_heap_start);

    /* Set P1 PM heap start */
    heap_pm_p1_start = HEAP_NODE(p1_heap_start);

    /* P0 PM ends wherever P1 PM starts */
    heap_pm_end = heap_pm_p1_start;

    if(PM_HEAP_SIZE)
    {
        /* Initialise P0 PM heap, but only if heap size is non-zero. The
         * freelist start will remain NULL if all the heap is given away to
         * P1 PM heap.
         * If P0 has a PM heap, heap_pm_start needs to be adjusted because
         * Minim might set the lowest bit.         */
        heap_pm_start = init_and_adjust_pm_heap(PM_HEAP_SIZE, heap_pm_start, NULL);
        freelist_pm = heap_pm_start;
    }

    if (PM_P1_HEAP_SIZE)
    {
        /* Similarly, initialise P1 PM heap, but only if heap size is non-zero. */
        heap_pm_p1_start = init_and_adjust_pm_heap(
                                    PM_P1_HEAP_SIZE, heap_pm_p1_start, NULL);
        freelist_pm_p1 = heap_pm_p1_start;
    }

#ifdef HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK
    /* P1 could use section of PM RAM cache bank as P1 PM heap. Create the new
     * section and add it to the list as head.
     * The following check is a dummy check to protect against CONFIG getting
     * values wrong. Compiler should optimise this away for the right values */
    if (P1_PM_CACHE_BANK_USABLE_START_ADDR < PM_RAM_END_ADDRESS)
    {
        addnl_heap_pm_p1_start = HEAP_NODE_IN_CACHE_START;
        addnl_heap_pm_p1_end = HEAP_NODE_IN_CACHE_END;
        freelist_pm_p1 = init_and_adjust_pm_heap(
                P1_PM_CACHE_BANK_USABLE_SIZE, addnl_heap_pm_p1_start, freelist_pm_p1);
    }
#endif /* HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK */
}
#else
#define configure_pm_heap_multi_core()
#endif /* defined(SUPPORTS_MULTI_CORE) */

#if defined(INSTALL_THREAD_OFFLOAD)
#if defined(CHIP_AURA)
static inline void configure_pm_heap_thread_offload(void)
{
#if !defined(INSTALL_CACHE)
    /* Banks 5,6,7,8 used for Core 0. Bank 9 is for P1 */
    heap_pm_end = HEAP_NODE(PM_RAM_END_ADDRESS);
    heap_pm_start = init_and_adjust_pm_heap(PM_HEAP_SIZE, heap_pm_start, NULL);
    freelist_pm = heap_pm_start;
    L2_DBG_MSG2("NO CACHE: PM heap: start 0x%x end 0x%x", heap_pm_start, heap_pm_end);
#elif defined(INSTALL_2WAY_CACHE)
    /* Banks 5,6,7,8,9 used for P0|P1 Cache */
    heap_pm_end = HEAP_NODE(PM_RAM_END_ADDRESS-5*PM_BANK_SIZE);
    heap_pm_start = init_and_adjust_pm_heap(PM_HEAP_SIZE, heap_pm_start, NULL);
    freelist_pm = heap_pm_start;
    L2_DBG_MSG2("INSTALL_2WAY_CACHE: PM heap: start 0x%x end 0x%x", heap_pm_start, heap_pm_end);
#elif defined(INSTALL_2WAYHALF_CACHE)
    /* Banks 7,8,9 used for P0|P0 Cache, Bank 5,6 is for P0 */
    heap_pm_end = HEAP_NODE(PM_RAM_END_ADDRESS-3*PM_BANK_SIZE);
    heap_pm_start = init_and_adjust_pm_heap(PM_HEAP_SIZE, heap_pm_start, NULL);
    freelist_pm = heap_pm_start;
    L2_DBG_MSG2("INSTALL_2WAYHALF_CACHE: PM heap: start 0x%x end 0x%x", heap_pm_start, heap_pm_end);
#else /* Direct cache */
    /* Banks 5,6,7,8 used for Core 0. Bank 9 is for Cache */
    /*         -------------------- 0x14000
     * BANK 9   Non-contiguous PM
     *         -------------------- 0x12800
     *          Cache
     *         -------------------- 0x12000
     * BANK 8   Contiguous PM
     *         ...
     * BANK0    Contiguous PM
     *         -------------------- 0x00000
     */
    heap_pm_end = HEAP_NODE(PM_RAM_END_ADDRESS-PM_BANK_SIZE);
    heap_pm_start = init_and_adjust_pm_heap(PM_HEAP_SIZE, heap_pm_start, NULL);
    freelist_pm = heap_pm_start;
    L2_DBG_MSG2("Direct cache: PM heap: start 0x%x end 0x%x", heap_pm_start, heap_pm_end);

#ifdef HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK
    /* P0 could use section of PM RAM cache bank as P0 PM heap. Create the new
     * section and add it to the list as head.
     * The following check is a dummy check to protect against CONFIG getting
     * values wrong. Compiler should optimise this away for the right values */
    if (P1_PM_CACHE_BANK_USABLE_START_ADDR < PM_RAM_END_ADDRESS)
    {
        heap_pm_end = HEAP_NODE_IN_CACHE_END;
        heap_pm_start = HEAP_NODE_IN_CACHE_START;
        heap_pm_start = init_and_adjust_pm_heap(PM_HEAP_SIZE, heap_pm_start, freelist_pm);
        freelist_pm = heap_pm_start;
        L2_DBG_MSG2("                  and: start 0x%x end 0x%x", heap_pm_start, heap_pm_end);
    }
#endif /* HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK */

#endif /* THREAD_OFFLOAD_P1_CACHE */

    /*
     * Setting P1 priority for the banks. This will set the correct access
     * permissions for all of P1's PM banks including the cache banks
     */
    hal_thread_offload_configure_pm_bank_access_control();
}
#else
#if defined(CHIP_STREPLUS)
static inline void configure_pm_heap_thread_offload(void)
{
#if !defined(INSTALL_CACHE)
    /* Banks 5 to 13 used for Core 0. */
    heap_pm_end = HEAP_NODE(PM_RAM_END_ADDRESS);
    heap_pm_start = init_and_adjust_pm_heap(PM_HEAP_SIZE, heap_pm_start, NULL);
    freelist_pm = heap_pm_start;
    L2_DBG_MSG2("NO CACHE: PM heap: start 0x%x end 0x%x", heap_pm_start, heap_pm_end);
#elif defined(INSTALL_2WAY_CACHE)
    /* Banks 9,10,11,12,13 used for P0|P1 Cache */
    heap_pm_end = HEAP_NODE(PM_RAM_END_ADDRESS-5*PM_BANK_SIZE);
    heap_pm_start = init_and_adjust_pm_heap(PM_HEAP_SIZE, heap_pm_start, NULL);
    freelist_pm = heap_pm_start;
    L2_DBG_MSG2("INSTALL_2WAY_CACHE: PM heap: start 0x%x end 0x%x", heap_pm_start, heap_pm_end);
#elif defined(INSTALL_2WAYHALF_CACHE)
    /* Banks 11,12,13 used for P0|P0 Cache, Bank 9,10 is for P0 */
    heap_pm_end = HEAP_NODE(PM_RAM_END_ADDRESS-3*PM_BANK_SIZE);
    heap_pm_start = init_and_adjust_pm_heap(PM_HEAP_SIZE, heap_pm_start, NULL);
    freelist_pm = heap_pm_start;
    L2_DBG_MSG2("INSTALL_2WAYHALF_CACHE: PM heap: start 0x%x end 0x%x", heap_pm_start, heap_pm_end);
#else /* Direct cache */
    /* Banks 9,10,11,12 used for Core 0. Bank 13 is for P0|P1 Cache */
    /*         -------------------- 0x1C000
     * BANK 13  Cache
     *         -------------------- 0x1B800
     * BANK 13  Contiguous PM
     *         -------------------- 0x1A000
     * BANK 12  Contiguous PM
     *         ...
     * BANK 0   Contiguous PM
     *         -------------------- 0x00000
     */
    heap_pm_end = HEAP_NODE(PM_RAM_P1_CACHE_START_ADDRESS);
    heap_pm_start = init_and_adjust_pm_heap(PM_HEAP_SIZE, heap_pm_start, NULL);
    freelist_pm = heap_pm_start;
    L2_DBG_MSG2("Direct cache: PM heap: start 0x%x end 0x%x", heap_pm_start, heap_pm_end);
#endif /* THREAD_OFFLOAD_P1_CACHE */

    /*
     * Setting P1 priority for the banks. This will set the correct access
     * permissions for all of P1's PM banks including the cache banks
     */
    hal_thread_offload_configure_pm_bank_access_control();
}
#else
    #error Thread Offload only implemented for CHIP_AURA and CHIP_STREPLUS at time of writing
#endif /* CHIP_STREPLUS */
#endif /* CHIP_AURA */
#endif /* INSTALL_THREAD_OFFLOAD */

/****************************************************************************
Public Function Definitions
*/

/**
 * NAME
 *   heap_pm_init_start_address
 *
 * \brief Initialise the offset used to calculate the heap start address
 *
 * \note The offset address has to be set before PM heap is initialised
 *
 */
void heap_pm_init_start_offset(unsigned offset)
{
    /* Should never be called after heap PM is initialised */
    PL_ASSERT(NULL == heap_pm_start);
    pm_reserved_size = offset;
    return;
}

/**
 * NAME
 *   init_heap_pm
 *
 * \brief Initialise the memory heap
 *
 */
void init_heap_pm(void)
{
    patch_fn_shared(heap_alloc_pm_init);
    /* Initialise the PM heap
     * Create a single free block of maximum length
     * and point the free list at it.
     */

    /* P0 heap always starts at an offset from the designated heap_pm linker symbol */
    heap_pm_start = HEAP_NODE((PM_HEAP_START_ADDR + pm_reserved_size));

    /* if second processor is not active, all heap is with core0. Note: Only
     * P0 will arrive here. */
    if (proc_multiple_cores_running())
    {
        configure_pm_heap_multi_core();
    }
    else
#if defined(INSTALL_THREAD_OFFLOAD)
    if (audio_thread_offload_is_configured())
    {
        patch_fn_shared(heap_alloc_pm_init);
        configure_pm_heap_thread_offload();
    }
    else
#endif /* INSTALL_THREAD_OFFLOAD */
    {
        /* Use the entirety of PM RAM as P0 PM heap */
        heap_pm_end = HEAP_NODE(PM_RAM_END_ADDRESS);
        heap_pm_start = init_and_adjust_pm_heap(PM_HEAP_SIZE, heap_pm_start, NULL);
        freelist_pm = heap_pm_start;
        L2_DBG_MSG2("PM heap: start 0x%x end 0x%x", heap_pm_start, heap_pm_end);
    }
}

#if defined(SUPPORTS_MULTI_CORE)
/**
 * NAME
 *   reconfigure_heap_pm
 *
 * \brief Reclaim P1's memory heap as P0's heap
 *
 */
void reconfigure_heap_pm(void)
{
    void_func_ptr p1_node;
    /* Trying to reconfigure PM heap, but P1 doesn't have any PM heap */
    PL_ASSERT(heap_pm_p1_start!=NULL);

    /* P1 heap is unused. First make it one giant block from heap_pm_p1_start
     * all the way to PM_RAM_END_ADDRESS (using up all of the banks including
     * P1's cache banks). Also make it look like a previously allocated block */
    heap_pm_p1_end = HEAP_NODE(PM_RAM_END_ADDRESS);
    p1_node = init_and_adjust_pm_heap(
                                PM_P1_HEAP_SIZE, heap_pm_p1_start,
                                (void_func_ptr)MAGIC_WORD);

    /* Now, update P0's PM heap to end at PM_RAM_END_ADDRESS */
    heap_pm_end = HEAP_NODE(PM_RAM_END_ADDRESS);

    /* Set P1 start and end to be the same, so that there is no P1 heap anymore */
    heap_pm_p1_start = NULL;
    heap_pm_p1_end = NULL;
#ifdef HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK
    addnl_heap_pm_p1_start = NULL;
    addnl_heap_pm_p1_end = NULL;
#endif
    freelist_pm_p1 = NULL;

    /* Finally free the P1 heap block. Since P0's heap end address has changed,
     * this block should be returned to P0's free list after coalescing other
     * free blocks, resulting in a giant P0 heap block.
     */
    heap_free_pm((void_func_ptr) ((uintptr_t)p1_node + SIZE_OF_DM_VAR_IN_PM_32(mem_node_pm)*PC_PER_INSTRUCTION));
}
#endif

/* *
 * Declare heap_alloc_internal_pm as a static function (unsigned, unsigned, pointer to pointer to function (void) returning void)
 * returning a pointer to function (void) returning void
 * */
static void_func_ptr heap_alloc_internal_pm(unsigned size_byte, unsigned heapsize_32, void_func_ptr *pfreelist)
{
    patch_fn(heap_alloc_pm_heap_alloc_internal_pm);
    unsigned int pm_win_value;
    mem_node_pm node, node2;
    /* The following two pointers can use the same memory as we don't need to have
     * them loaded at the same time */
    mem_node_pm *curnode = &node;
    mem_node_pm *bestnode = &node;
    /* This pointer needs separate memory */
    mem_node_pm *tempnode = &node2;
    unsigned bestsize_32 = heapsize_32;
    void_func_ptr bestnode_pm_addr = NULL;
    void_func_ptr curnode_pm_addr;
    void_func_ptr prevnode_pm_addr = NULL;
    void_func_ptr bestnode_prev_pm_addr = NULL;

    curnode_pm_addr = *pfreelist;

    /* Do all the list-traversal and update with interrupts blocked */
    LOCK_INTERRUPTS;
    /* Save initial state */
    pm_win_value = PMWIN_ENABLE;
    /* Enable access to PM through DM window */
    PMWIN_ENABLE = 1;

    /* Traverse the list looking for the best-fit free block
     * Best fit is the smallest one of at least the requested size
     * This will help to minimise wastage
     */
    while (curnode_pm_addr != NULL)
    {
        READ_NODE_FROM_PM(GET_NODE_ADDR32(curnode_pm_addr), curnode);
        if (((curnode)->struct_mem_node.length_32 >= BYTES_INTO_PM_32(size_byte)) &&
                ((curnode)->struct_mem_node.length_32 < bestsize_32))
        {
            bestnode_pm_addr = curnode_pm_addr;
            /* Save the pointer that pointed to the best node, if it was the first one in DM, this would be NULL */
            bestnode_prev_pm_addr = prevnode_pm_addr;
            bestsize_32 = curnode->struct_mem_node.length_32;
        }
        /* Save the node (PM Address) that will point to next one */
        prevnode_pm_addr = curnode_pm_addr;
        curnode_pm_addr = (curnode)->struct_mem_node.u.next;
    }

    if (bestnode_pm_addr)
    {
        void_func_ptr addr;
        READ_NODE_FROM_PM(GET_NODE_ADDR32(bestnode_pm_addr), bestnode);
        if (bestsize_32 >= (BYTES_INTO_PM_32(size_byte)) + SIZE_OF_DM_VAR_IN_PM_32(mem_node_pm) + MIN_SPARE_32)
        {
            /* There's enough space to allocate something else
             * so keep the existing free block and allocate the space at the top
             * In this case the allocation size is exactly what was requested
             */
            addr = (void_func_ptr) ((uintptr_t)(bestnode_pm_addr) +
                    (bestnode->struct_mem_node.length_32 - BYTES_INTO_PM_32(size_byte))*PC_PER_INSTRUCTION);
            bestnode->struct_mem_node.length_32 -= (BYTES_INTO_PM_32(size_byte) + SIZE_OF_DM_VAR_IN_PM_32(mem_node_pm));
            WRITE_NODE_TO_PM(GET_NODE_ADDR32(bestnode_pm_addr), bestnode);
        }
        else
        {
            /* Not enough extra space to be useful
             * Replace the free block with an allocated one
             * The allocation size is the whole free block
             */
            addr = bestnode_pm_addr;
            size_byte = (bestnode->struct_mem_node.length_32)*PC_PER_INSTRUCTION;

            /* Update pointer that pointed to the best node */
            if (bestnode_prev_pm_addr != NULL)
            {
                READ_NODE_FROM_PM(GET_NODE_ADDR32(bestnode_prev_pm_addr), tempnode);
                tempnode->struct_mem_node.u.next = bestnode->struct_mem_node.u.next;
                WRITE_NODE_TO_PM(GET_NODE_ADDR32(bestnode_prev_pm_addr), tempnode);
            }
            else
            {
                /* This node was pointed to by an address in DM, update it */
                *pfreelist = bestnode->struct_mem_node.u.next;
            }
        }
        /* Finally populate the header for the newly-allocated block */
        bestnode->struct_mem_node.length_32 = BYTES_INTO_PM_32(size_byte);
        bestnode->struct_mem_node.u.magic = MAGIC_WORD;
        WRITE_NODE_TO_PM(GET_NODE_ADDR32(addr), bestnode);
        /* Restore initial state */
        PMWIN_ENABLE = pm_win_value;
        UNLOCK_INTERRUPTS;
        return (void_func_ptr) ((uintptr_t)addr + SIZE_OF_DM_VAR_IN_PM_32(mem_node_pm)*PC_PER_INSTRUCTION);
    }
    /* No suitable block found */
    /* Restore initial state */
    PMWIN_ENABLE = pm_win_value;
    UNLOCK_INTERRUPTS;
    return NULL;
}

/**
 * NAME
 *   heap_alloc_pm
 *
 * \brief Allocate a block of (at least) the requested size
 *
 */
void_func_ptr heap_alloc_pm(unsigned size_byte, unsigned preference_core)
{
    patch_fn_shared(heap_alloc_pm);
    void_func_ptr addr;

    /* Don't do anything if zero size requested */
    if (size_byte == 0)
    {
        return NULL;
    }
    /* Make sure we always try to allocate a 32bit-aligned amount of
     * PM memory, even on 8-bit addressable PMs */
    else if ((size_byte % PC_PER_INSTRUCTION) != 0)
    {
        size_byte += PC_PER_INSTRUCTION - (size_byte % PC_PER_INSTRUCTION);
    }
    switch(preference_core)
    {
        case MALLOC_PM_PREFERENCE_CORE_0:
            addr = HEAP_PM_CORE_0_ALLOC(size_byte);
            break;
        case MALLOC_PM_PREFERENCE_CORE_1:
            addr = HEAP_PM_CORE_1_ALLOC(size_byte);
            break;
        default:
            addr = HEAP_PM_CORE_0_ALLOC(size_byte);
            break;
    }

    return addr;
}

/**
 * NAME
 *   heap_free_pm
 *
 * \brief Free a previously-allocated block
 *
 */
void heap_free_pm(void_func_ptr ptr)
{
    patch_fn(heap_alloc_pm_heap_free_pm);
    unsigned int pm_win_value;
    void_func_ptr *pfreelist = NULL;

    mem_node_pm node_cur;
    mem_node_pm *curnode = &node_cur;
    void_func_ptr curnode_pm_addr = NULL;

    mem_node_pm node_;
    mem_node_pm *node = &node_;
    void_func_ptr node_pm_addr = NULL;

    mem_node_pm prev_node_;
    mem_node_pm *prev_node = &prev_node_;
    void_func_ptr prev_node_pm_addr = NULL;

    LOCK_INTERRUPTS;
    /* Save initial state */
    pm_win_value = PMWIN_ENABLE;
    /* Enable access to PM through DM window */
    PMWIN_ENABLE = 1;

    if (ptr == NULL)
    {
        /* free(NULL) is a no-op  */
        return;
    }
    PL_PRINT_P1(TR_PL_FREE, "PM ptr to be freed %lx..", (uintptr_t)ptr);

    if (IS_IN_PM_P0_HEAP(ptr))
    {
        PL_PRINT_P0(TR_PL_FREE, "is in main PM heap\n");
        pfreelist = &freelist_pm;
    }
    else if (IS_IN_PM_P1_HEAP(ptr))
    {
        PL_PRINT_P0(TR_PL_FREE, "is in PM P1 heap\n");
        pfreelist = &freelist_pm_p1;
    }
    else
    {
        PL_PRINT_P0(TR_PL_FREE, "Couldn't find in any PM heap\n");
        panic_diatribe(PANIC_AUDIO_FREE_INVALID, (uintptr_t)ptr);
    }

    node_pm_addr = (void_func_ptr) ((uintptr_t)ptr - SIZE_OF_DM_VAR_IN_PM_32(mem_node_pm)*PC_PER_INSTRUCTION);
    READ_NODE_FROM_PM(GET_NODE_ADDR32(node_pm_addr), node);

    /* Check that the address being freed looks sensible */
    if (node->struct_mem_node.u.magic != MAGIC_WORD)
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID, (uintptr_t)ptr);
    }

    /* Check that the length seems plausible */
    if (!IS_IN_PM_HEAP((void_func_ptr)((uintptr_t)ptr + (node->struct_mem_node.length_32 - 1)*PC_PER_INSTRUCTION)))
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID, (uintptr_t)ptr);
    }

    node->struct_mem_node.u.magic = 0;
    WRITE_NODE_TO_PM(GET_NODE_ADDR32(node_pm_addr), node);

    curnode_pm_addr = *pfreelist;

    /* Traverse the free list to see if we can coalesce an existing free block with this one */
    while (curnode_pm_addr != NULL)
    {
        READ_NODE_FROM_PM(GET_NODE_ADDR32(curnode_pm_addr), curnode);
        if ((void_func_ptr)((uintptr_t)curnode_pm_addr + (curnode->struct_mem_node.length_32 + SIZE_OF_DM_VAR_IN_PM_32(mem_node_pm))*PC_PER_INSTRUCTION) == node_pm_addr)
        {
            /* Matching block found */
            break;
        }
        curnode_pm_addr = curnode->struct_mem_node.u.next;
    }

    if (curnode_pm_addr != NULL)
    {
        /* The immediately-previous block is free
         * add the one now being freed to it
         */
        curnode->struct_mem_node.length_32 += node->struct_mem_node.length_32 + SIZE_OF_DM_VAR_IN_PM_32(mem_node_pm);
        WRITE_NODE_TO_PM(GET_NODE_ADDR32(curnode_pm_addr), curnode);
    }
    else
    {
        /* Previous block wasn't free
         * so add the now-free block to the free list
         * Note length is unchanged from when it was allocated
         */
        curnode_pm_addr = node_pm_addr;
        READ_NODE_FROM_PM(GET_NODE_ADDR32(curnode_pm_addr), curnode);

        curnode->struct_mem_node.u.next = *pfreelist;
        WRITE_NODE_TO_PM(GET_NODE_ADDR32(curnode_pm_addr), curnode);

        *pfreelist = curnode_pm_addr;
    }
    /* Now check if there is a free block immediately after the found / new one */
    node_pm_addr = *pfreelist;
    while (node_pm_addr != NULL)
    {
        READ_NODE_FROM_PM(GET_NODE_ADDR32(node_pm_addr), node);
        if ( node_pm_addr == (void_func_ptr)((uintptr_t)curnode_pm_addr +
                (curnode->struct_mem_node.length_32 + SIZE_OF_DM_VAR_IN_PM_32(mem_node_pm))*PC_PER_INSTRUCTION))
        {
            /* Matching block found */
            break;
        }
        prev_node_pm_addr = node_pm_addr;
        node_pm_addr = node->struct_mem_node.u.next;
    }
    if (node_pm_addr != NULL)
    {
        /* The immediately-following block is free
         * add it to the current one and remove from the free list
         */
        curnode->struct_mem_node.length_32 += node->struct_mem_node.length_32 + SIZE_OF_DM_VAR_IN_PM_32(mem_node_pm);
        WRITE_NODE_TO_PM(GET_NODE_ADDR32(curnode_pm_addr), curnode);
        if (prev_node_pm_addr == NULL)
        {
            /* The found block is head of the list */
            *pfreelist = node->struct_mem_node.u.next;
        }
        else
        {
            READ_NODE_FROM_PM(GET_NODE_ADDR32(prev_node_pm_addr), prev_node);
            prev_node->struct_mem_node.u.next = node->struct_mem_node.u.next;
            WRITE_NODE_TO_PM(GET_NODE_ADDR32(prev_node_pm_addr), prev_node);
        }
    }
    /* Restore initial state */
    PMWIN_ENABLE = pm_win_value;
    UNLOCK_INTERRUPTS;
}


/**
 * NAME
 *   heap_sizeof_pm
 *
 * \brief Get the size of a previously-allocated block
 *
 */
unsigned heap_sizeof_pm(void_func_ptr ptr)
{
    patch_fn_shared(heap_alloc_pm);
    unsigned int pm_win_value;
    void_func_ptr node_pm_addr;
    mem_node_pm node;

    if (ptr == NULL)
    {
        return 0;
    }
    node_pm_addr = (void_func_ptr) ((uintptr_t)ptr - SIZE_OF_DM_VAR_IN_PM_32(mem_node_pm)*PC_PER_INSTRUCTION);
    LOCK_INTERRUPTS;
    /* Save initial state */
    pm_win_value = PMWIN_ENABLE;
    /* Enable access to PM through DM window */
    PMWIN_ENABLE = 1;
    READ_NODE_FROM_PM(GET_NODE_ADDR32(node_pm_addr), &node);
    /* Restore initial state */
    PMWIN_ENABLE = pm_win_value;
    UNLOCK_INTERRUPTS;

    /* Check that the address looks sensible */
    if (node.struct_mem_node.u.magic != MAGIC_WORD)
    {
        /* Might want a (debug-only?) panic here */
        return 0;
    }
    return (node.struct_mem_node.length_32 * PC_PER_INSTRUCTION);
}

bool is_in_pm_heap(void_func_ptr ptr)
{
    patch_fn_shared(heap_alloc_pm);
    return IS_IN_PM_HEAP(ptr);
}
