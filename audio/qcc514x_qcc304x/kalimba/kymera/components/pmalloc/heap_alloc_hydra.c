/****************************************************************************
 * Copyright (c) 2012 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file heap_alloc_hydra.c
 * \ingroup pl_malloc
 *
 * Hydra specific heap memory management.
 */

/****************************************************************************
Include Files
*/

#include "pl_malloc_private.h"

/****************************************************************************
Private Configuration Macro Declarations
*/
#define UNUSED(x)   ((void)(x))

#if defined(__KCC__)
#define LINKER_ADDRESS extern unsigned
#define LINKER_SIZE extern unsigned

/*
 * Directly accessing the linker provided symbol
 * as an address.
 */
#define GET_LINKER_ADDRESS(x) ((char*)(uintptr_t)&(x))

/*
 * Directly accessing the linker provided symbol
 * as an address and typecasting to integer to make
 * the compiler happy
 */
#define HEAP_SIZE(x) ((unsigned)(uintptr_t)&(x))

#else /* defined(__KCC__) */
#define LINKER_ADDRESS static uintptr_t
#define LINKER_SIZE static unsigned

#define GET_LINKER_ADDRESS(x) NULL
#define HEAP_SIZE(x) x

#endif /* defined(__KCC__) */

/*
 * If INSTALL_DYNAMIC_HEAP is not defined, the heap sizes are
 * initialised to the build time provided configuration of the fixed
 * heap size. INSTALL_DYNAMIC_HEAP allows to dynamically increase
 * the heap size in heap_config() API after reading the MIB keys
 *
 * while writing this, INSTALL_DYNAMIC_HEAP is supported  only
 * for hydra platforms.
 */

/**************************** heap configuration  ****************************/
/* heap is provided as linker symbols for start address and size. */
LINKER_ADDRESS _heap_start_addr;
LINKER_SIZE _heap_size;

#define HEAP_MAX_SIZE          HEAP_SIZE(_heap_size)

/* Use separate linker symbols provided for single mode. We only use the
   single mode start address. */
LINKER_ADDRESS _single_mode_heap_start_addr;
LINKER_SIZE _single_mode_heap_size;

static char *heap_single_mode = GET_LINKER_ADDRESS(_single_mode_heap_start_addr);
#define HEAP_MAX_SIZE_SINGLE_MODE  \
                                HEAP_SIZE(_single_mode_heap_size)

#define INVALID_HEAP_SIZE(x) ((x) > HEAP_MAX_SIZE)

#ifdef INSTALL_THREAD_OFFLOAD
LINKER_SIZE _thread_offload_heap_size;
#define HEAP_MAX_SIZE_THREAD_OFFLOAD  \
                                HEAP_SIZE(_thread_offload_heap_size)
#else
#define HEAP_MAX_SIZE_THREAD_OFFLOAD  HEAP_MAX_SIZE
#endif /* INSTALL_THREAD_OFFLOAD */

/************************ shared heap configuration **************************/
#if defined(SUPPORTS_MULTI_CORE) && defined(__KCC__)
/* heap is provided as linker symbols for start address and size  */
LINKER_ADDRESS _shared_heap_start_addr;
LINKER_SIZE _shared_heap_size;

/* Maximum shared heap as per linker symbols */
#define SHARED_HEAP_MAX_SIZE    HEAP_SIZE(_shared_heap_size)

/* Use separate linker symbols provided for single mode. We only use the
   single mode start address. */
LINKER_ADDRESS _single_mode_shared_heap_start_addr;
LINKER_SIZE _single_mode_shared_heap_size;

static char* shared_heap_single_mode = GET_LINKER_ADDRESS(_single_mode_shared_heap_start_addr);
#define SHARED_HEAP_MAX_SIZE_SINGLE_MODE  \
                                HEAP_SIZE(_single_mode_shared_heap_size)

#ifdef INSTALL_THREAD_OFFLOAD
LINKER_SIZE _thread_offload_shared_heap_size;
#define SHARED_HEAP_MAX_SIZE_THREAD_OFFLOAD  \
                                HEAP_SIZE(_thread_offload_shared_heap_size)
#else
#define SHARED_HEAP_MAX_SIZE_THREAD_OFFLOAD  0
#endif /* INSTALL_THREAD_OFFLOAD */

#else

/* Define the SHARED_HEAP to 0 for unit tests and non DUAL CORE configs. */
#define shared_heap_single_mode             ((char*)NULL)
#define SHARED_HEAP_MAX_SIZE                0
#define SHARED_HEAP_MAX_SIZE_SINGLE_MODE    0

#define SHARED_HEAP_MAX_SIZE_THREAD_OFFLOAD 0
#endif /* defined(SUPPORTS_MULTI_CORE) && defined(__KCC__) */


/************************ Extended DM1 configuration *************************/
#ifdef CHIP_HAS_SLOW_DM_RAM
/* heap is provided as linker symbols for start address and size  */
LINKER_ADDRESS _slow_heap_start_addr;
LINKER_SIZE _slow_heap_size;

static char *slow_heap = GET_LINKER_ADDRESS(_slow_heap_start_addr);

#define SLOW_HEAP_MAX_SIZE      HEAP_SIZE(_slow_heap_size)

#define HEAP_NEEDS_DM_BANK_PERMISSIONS(name) ((name == HEAP_MAIN) || (name == HEAP_SLOW))
#define INVALID_HEAP_SLOW_SIZE(x) ((x) > SLOW_HEAP_MAX_SIZE)
#else  /* CHIP_HAS_SLOW_DM_RAM */
#define slow_heap                   ((char*)NULL)
#define SLOW_HEAP_MAX_SIZE          0
#define INVALID_HEAP_SLOW_SIZE(x)   FALSE
#define HEAP_NEEDS_DM_BANK_PERMISSIONS(name) (name == HEAP_MAIN)
#endif /* CHIP_HAS_SLOW_DM_RAM */


/****************************************************************************
Private Macro Declarations
*/

#ifdef PMALLOC_DEBUG
#define HEAP_GUARD_WORD 0x987654ul
#define GUARD_SIZE (sizeof(unsigned int))
#else
#define GUARD_SIZE 0
#endif

#define MIN_SPARE 8

#define KIBYTE (1024)

#if defined(INSTALL_EXTERNAL_MEM)
#define ext_heap_is_disabled()  ((freelist[HEAP_EXT] == NULL))
#if defined(__KCC__)
#define SRAM_START_ADDR ((char*)(DM_NVMEM_WINDOW_START + (DM_NVMEM_WINDOW_SIZE * EXT_RAM_WRITE_WINDOW)))
#else
static char* ext_heap = NULL;
#define SRAM_START_ADDR &ext_heap[0]
#endif
#else
#define init_external_heap() FALSE
#define ext_heap_is_enabled() FALSE
#define heap_init_sram()     FALSE
#endif /* INSTALL_EXTERNAL_MEM */


/* If DM2 free heap is below this level, don't use it
 * as a prefered heap  unless request with DM2 preference
 */
#define DM2_RESERVE_HEAP_WATERMARK 0x1000

/* Calculate the address that is at the middle of the given DM bank */
#define DM_BANK_MIDPOINT_ADDR(bank) (uintptr_t)((bank * DM_RAM_BANK_SIZE) + DM_RAM_BANK_SIZE / 2)

/* Verify some assumptions and refactoring */
#if defined(__KCC__) && !defined(RUNNING_ON_KALSIM) && !defined(UNIT_TEST_BUILD)
STATIC_ASSERT(PROC_PROCESSOR_MAX==CHIP_NUM_CORES,PROC_PROCESSOR_MAX_equals_CHIP_NUM_CORES);
#endif

#if defined(INSTALL_THREAD_OFFLOAD) && !defined(UNIT_TEST_BUILD)
#define heap_alloc_thread_offload_is_configured()    audio_thread_offload_is_configured()
#else
#define heap_alloc_thread_offload_is_configured()    FALSE
#endif /* INSTALL_THREAD_OFFLOAD */

/****************************************************************************
Private Type Declarations
*/
typedef struct mem_node
{
    unsigned length;
    union
    {
        struct mem_node *next;
        unsigned magic;
    } u;
#ifdef PMALLOC_DEBUG
    const char *file;
    unsigned int line;
    unsigned int guard;
#endif
} mem_node;

/**
 * Type definition for heap allocation mib key values.
 */
typedef struct heap_allocation_mib_value
{
    uint8 p0_heap_size;
#ifdef CHIP_HAS_SLOW_DM_RAM
    uint8 p0_slow_heap_size;
#else
    uint8 p0_unused;
#endif
    uint8 p0_unused1;
    uint8 p1_heap_size;
#ifdef CHIP_HAS_SLOW_DM_RAM
    uint8 p1_slow_heap_size;
#else
    uint8 p1_unused;
#endif
    uint8 p1_unused1;
} heap_allocation_mib_value;

#if defined(INSTALL_EXTERNAL_MEM)
/**
 * Type definition for heap allocation mib key values.
 */
typedef struct ext_heap_allocation_mib_value
{
    uint8 ext_ram_size_msb;
    uint8 ext_ram_size_lsb;
    uint8 ext_p0_data_heap_size_msb;
    uint8 ext_p0_data_heap_size_lsb;
    uint8 ext_p1_data_heap_size_msb;
    uint8 ext_p1_data_heap_size_lsb;
    uint8 reserved_0;
    uint8 reserved_1;
    uint8 reserved_2;
    uint8 reserved_3;
    uint8 reserved_4;
    uint8 reserved_5;
    uint8 reserved_7;
    uint8 reserved_8;
    /* Currently we only have 1 heap per CPU in SRAM.
     * Ignoring reserved values*/
} ext_heap_allocation_mib_value;
#endif /* INSTALL_EXTERNAL_MEM */

/**
 * Heap sizes for a processor.
 */
typedef struct heap_sizes
{
    unsigned int heap_size[HEAP_ARRAY_SIZE];
} heap_sizes;

/**
 * Dynamic heap configuration for the chip.
 */
typedef struct heap_dyn_size_config
{
    /* Processor */
    heap_sizes p[PROC_PROCESSOR_MAX];
} heap_dyn_size_config;

/****************************************************************************
Private Function Declarations
*/
#ifdef __KCC__
#if defined(INSTALL_DYNAMIC_HEAP) && !defined(UNIT_TEST_BUILD)

#define SET_HEAP_MAIN_SIZE(c,v)     value->p[(c)].heap_size[HEAP_MAIN] = ((v) * KIBYTE)
#define SET_HEAP_SHARED_SIZE(c,v)   value->p[(c)].heap_size[HEAP_SHARED] = ((v) * KIBYTE)
#define GET_HEAP_MAIN_SIZE(c)       value->p[(c)].heap_size[HEAP_MAIN]
#define GET_HEAP_SHARED_SIZE(c)     value->p[(c)].heap_size[HEAP_SHARED]
#ifdef CHIP_HAS_SLOW_DM_RAM
#define SET_HEAP_SLOW_SIZE(c,v)     value->p[(c)].heap_size[HEAP_SLOW] = ((v) * KIBYTE)
#define GET_HEAP_SLOW_SIZE(c)       value->p[(c)].heap_size[HEAP_SLOW]
#else
#define SET_HEAP_SLOW_SIZE(c,v)
#define GET_HEAP_SLOW_SIZE(c)       (0)
#endif

static bool get_heap_allocation(heap_dyn_size_config* value);

#endif /* defined(INSTALL_DYNAMIC_HEAP) && !defined(UNIT_TEST_BUILD) */

static void heap_configure_and_align(heap_config *main_core,
                                     heap_config *second_core,
                                     heap_sizes *heap_max_sizes,
                                     bool have_dyn_config,
                                     heap_dyn_size_config *dyn_heap_config,
                                     mem_node **freelist);

static inline char* round_down_to_dm_bank_addr(char* address);
#endif /* __KCC__ */

static inline unsigned get_heap_size_node(char* heap);

/****************************************************************************
Private Variable Definitions
*/

/* Free list and minimum heap arrays
 * Minimum heap array will be initialised at pmalloc_init() by P0
 * pmalloc_config() will extend the heap beyond this array if free
 * memory availoable as per the memory map.
 * heap boundaries are externally visible only for the minimum heap sizes.
 * For the expanded heap boundaries after configuration needs to be calculated
 * using the configured sizes.
 */
static mem_node *freelist[HEAP_ARRAY_SIZE];

/* The heap info is shared between P0 and P1 if IPC installed */
static DM_SHARED_ZI heap_config processor_heap_info_list[PROC_PROCESSOR_BUILD];

/* local pointer to the processor copy of heap_config */
static heap_config *pheap_info;

#ifdef HEAP_DEBUG
/* Total free memory, number of blocks on the free list and minimum number of free
 * blocks (this intended for getting memory watermarks) */
unsigned heap_debug_free, heap_debug_freenodes, heap_debug_min_free;
/* Total allocated memory and number of allocated blocks */
unsigned heap_debug_alloc, heap_debug_allocnodes;
#endif

/****************************************************************************
Private Function Definitions
*/
#if defined(__KCC__)
static inline void *convert_to_dm1(void *addr)
{
    uintptr_t dm_addr = (uintptr_t) addr;

    if (dm_addr > DM2_RAM_BASE)
    {
        return (void *)(dm_addr - DM2_RAM_BASE);
    }

    return addr;
}

static inline void *convert_to_dm2(void *addr)
{
    uintptr_t dm_addr = (uintptr_t) addr;

    if (addr == NULL)
    {
        return NULL;
    }
    return (void *)(dm_addr + DM2_RAM_BASE);
}
#else
#define convert_to_dm1(x) x
#define convert_to_dm2(x) x
#endif /* !defined(__KCC__) */

#if defined(INSTALL_MIB) && !defined(UNIT_TEST_BUILD)
static inline void heap_alloc_configure_strictness(PROC_ID_NUM core)
{
    heap_config *pconfig = &processor_heap_info_list[core];
    uint16 value;

    if (core == PROC_PROCESSOR_0)
    {
        value = MALLOC_STRICTNESS_DM1_FALLBACK;
        (void) mibgetu16(RELAXMALLOCSTRICTNESS, &value);
    }
    else
    {
        /* Secondary processors use the same value as
           the primary processor. */
        value = pheap_info->relaxmallocstrictness;
    }
    pconfig->relaxmallocstrictness = (MALLOC_STRICTNESS) value;
}
#else
#define heap_alloc_configure_strictness(x)
#endif /* defined(INSTALL_MIB) && !defined(UNIT_TEST_BUILD) */

/**
 * \brief Allocate some memory on the host to emulate
 *        the area reserved by the linker for the target.
 */
#if !defined(__KCC__)
static void init_host_heap(void)
{
    if (heap_single_mode == NULL)
    {
        _single_mode_heap_size = NUMBER_DM_BANKS * DM_RAM_BANK_SIZE;
        heap_single_mode = calloc(_single_mode_heap_size, sizeof(char));
        PL_ASSERT(heap_single_mode != NULL);
        _single_mode_heap_start_addr = (uintptr_t) heap_single_mode;
        _heap_start_addr = _single_mode_heap_start_addr;
        _heap_size = _single_mode_heap_size;
    }
    if (ext_heap == NULL)
    {
        ext_heap = calloc(EXT_HEAP_SIZE, sizeof(char));
        PL_ASSERT(ext_heap != NULL);
    }
#if defined(CHIP_HAS_SLOW_DM_RAM)
    if (slow_heap == NULL)
    {
        _slow_heap_size = NUMBER_DM_BANKS * DM_RAM_BANK_SIZE;
        slow_heap = calloc(_slow_heap_size, sizeof(char));
        PL_ASSERT(slow_heap != NULL);
        _slow_heap_start_addr = (uintptr_t) slow_heap;
    }
#endif /* defined(CHIP_HAS_SLOW_DM_RAM) */
#if defined(INSTALL_THREAD_OFFLOAD)
    _thread_offload_heap_size = 0;
#endif
}
#else
#define init_host_heap()
#endif /* !defined(__KCC__) */

/**
 * \brief Returns which heap the pointer points to.
 *
 * \note  Panics if memory is not in the heap.
 */
heap_names get_heap_num(void *ptr)
{
    heap_info *heap_cfg = pheap_info->heap;
    heap_names heap_num;

    patch_fn_shared(heap);

    for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
    {
        if ((heap_cfg[heap_num].heap_start <= (char*) ptr) &&
            ((char*) ptr < heap_cfg[heap_num].heap_end))
        {
            return heap_num;
        }
    }
    PL_PRINT_P0(TR_PL_FREE, "Couldn't find anywhere\n");
    panic_diatribe(PANIC_AUDIO_FREE_INVALID, (DIATRIBE_TYPE)((uintptr_t)ptr));
#ifdef USE_DUMMY_PANIC
    /* panic_diatribe DoesNotReturn in standard builds, but it does for
     * some test builds where this is defined. */
    return HEAP_INVALID;
#endif
}

/**
 * \brief Set the heap information which consist of start, end address and size.
 */
static void set_heap_info(heap_info* heap,
                          char* start,
                          char* end,
                          unsigned size,
                          bool resizing)
{
    PL_ASSERT(start + size == end);

    heap->heap_start = start;
    heap->heap_size = size;
    heap->heap_end = end;

    if (!resizing)
    {
        /* set heap_free = 0, It will be initialised while initialising nodes */
        heap->heap_free = 0;
    }
}

/*
 * NAME
 *   get_heap_size_node
 *
 * \brief  Get the size of the heap node
 */
static inline unsigned get_heap_size_node(char* heap)
{
   return (heap != NULL)? (((mem_node*)heap)->length + sizeof(mem_node)):0;
}

/**
 * \brief Initialise the heap mem node.
 */
static mem_node* init_heap_node(char* heap, unsigned heap_size)
{
    unsigned freelength;
    heap_names heap_num;
    mem_node *node = (mem_node *) heap;

    if ((heap_size <= sizeof(mem_node)) || (heap == NULL))
    {
        return NULL;
    }

    freelength = heap_size - sizeof(mem_node);
    node->u.next = NULL;
    node->length = freelength;

    /* find out the location of the free node */
    heap_num = get_heap_num((void*) heap);
    pheap_info->heap[heap_num].heap_free += freelength - GUARD_SIZE;

#ifdef HEAP_DEBUG
    heap_debug_free += freelength - GUARD_SIZE;
    heap_debug_freenodes++;
#endif

    return node;
}

/**
 * \brief Find the mid point for the given heap
 */
static char* heap_midpoint(heap_config *pconfig, unsigned heap_num)
{
    return pconfig->heap[heap_num].heap_start +
           pconfig->heap[heap_num].heap_size / 2;
}

#if defined(SUPPORTS_MULTI_CORE) && defined(__KCC__)

#ifdef INSTALL_DM_BANK_ACCESS_CONTROL
/**
 * \brief For the given bank, find the heap that occupies it and return the
 *        preferred core and DM access method for setting arbiter permissions.
 */
static inline heap_names get_preferred_ownership_for_bank(unsigned bank,
                                                          PROC_ID_NUM *core,
                                                          hal_dm_bank *pref_dem)
{
    heap_names heap_num;
    unsigned core_index;

    *core = PROC_PROCESSOR_INVALID;
    *pref_dem = DM1;
    /* Loop through the number of cores */
    for (core_index = 0; core_index < PROC_PROCESSOR_BUILD; core_index++)
    {
        /* loop through each heap in the core */
        for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
        {
            heap_config *pconfig = &processor_heap_info_list[core_index];
            heap_info *info = &pconfig->heap[heap_num];

            /* Check if the mid point of the bank address falls within the heap */
            if (((uintptr_t)(info->heap_start) <= DM_BANK_MIDPOINT_ADDR(bank)) &&
                ((uintptr_t)(info->heap_end)    > DM_BANK_MIDPOINT_ADDR(bank))    )
            {
                /* If a core has only 1 bank in a given heap, the permissions
                 * will be set so that the core's DM1 access is the default
                 * owner.
                 *
                 * If a core has more than 1 bank, half of the total banks will
                 * be set to be owned by core's DM2 access and the remaining
                 * owned by core's DM1 access.
                 *
                 * If the end address of the DM bank is within the midpoint of
                 * the heap, use DM2 as preferred address space, since the lower
                 * allocations from the heap is usually allocated as DM2 blocks.
                 */
                *pref_dem = ((uintptr_t)heap_midpoint(pconfig, heap_num) >= (uintptr_t)(((bank+1) * DM_RAM_BANK_SIZE)-1))?
                        DM2: DM1;
                *core = (PROC_ID_NUM) core_index;
                return heap_num;
            }
        }
    }
    return HEAP_INVALID;
}
#endif /* INSTALL_DM_BANK_ACCESS_CONTROL */

/**
 * \brief Configure DM bank permissions based on configuration of the heaps.
 *
 *        Banks are divided between the 2 cores for use as various heaps, with
 *        each core getting at least 1 bank (except for shared heap). P1 will
 *        get the lower bank(s) and P0 will get the upper bank(s).
 *        If a core has only 1 bank in a given heap, the permissions will be
 *        set so that the core's DM1 access is the default owner.
 *        If a core has more than 1 bank, the lower half of the total banks
 *        will be set to be owned by core's DM2 access and the remaining upper
 *        half owned by core's DM1 access.
 *
 *        When setting the ownership for a core, all writes to the bank from
 *        the other core will be blocked. DM1/DM2 ownership for the same core
 *        only applies when there is simultaneous access from the same core
 *        on the 2 address spaces. This is not blocked, but the ownership will
 *        cause stalls on simultaneous access.
 *
 *        For shared heap and banks that do not fall in any heap (for example
 *        bank0 may be fully occupied by private and global vars), configure
 *        the default/reset value in the arbiter permissions. The default/reset
 *        value gives preferred owner as Core 0's DM1 access and other core or
 *        DM2 access are not blocked
 */
static void set_dm_memory_bank_permissions(void)
{
#ifdef INSTALL_DM_BANK_ACCESS_CONTROL
    unsigned bank;
    PROC_ID_NUM core;
    heap_names heap_num;
    hal_dm_bank pref_dm;

    patch_fn_shared(heap);

    for (bank=0; bank < NUMBER_DM_BANKS; bank++)
    {
        heap_num = get_preferred_ownership_for_bank(bank, &core, &pref_dm);
        /* Only need heap permissions for non-shared heaps located in DM banks */
        if (HEAP_NEEDS_DM_BANK_PERMISSIONS(heap_num))
        {
            if (proc_multiple_cores_running())
            {
                hal_multi_core_configure_arbiter(bank, core, pref_dm, FALSE);
            }
            else if (heap_alloc_thread_offload_is_configured())
            {
                /* By default, core specific banks exclude write access from the
                 * other core. If thread offload is enabled, allow access to P1 */
                hal_multi_core_configure_arbiter(bank, PROC_PROCESSOR_0, pref_dm, TRUE);
            }
            else
            {
                hal_multi_core_configure_arbiter(bank, PROC_PROCESSOR_0, pref_dm, FALSE);
            }
        }
        /* Non-main-heap (shared heap, globals, etc) */
        else
        {
            hal_multi_core_configure_arbiter(bank, PROC_PROCESSOR_0, DM1, TRUE);
        }
    }
#endif /* INSTALL_DM_BANK_ACCESS_CONTROL */
}
#else
#define set_dm_memory_bank_permissions()
#endif /* defined(SUPPORTS_MULTI_CORE) && defined(__KCC__) */

/**
 * \brief Internal call to allocate memory.
 */
static void *heap_alloc_internal(unsigned size,
                                 unsigned heap_num,
                                 bool top_down)
{
    mem_node **pbest = NULL;
    mem_node **pnode = &freelist[heap_num];
    unsigned bestsize = pheap_info->heap[heap_num].heap_size;

    /* Round up size to the nearest whole word */
    size = ROUND_UP_TO_WHOLE_WORDS(size) + GUARD_SIZE;

    /* Do all the list-traversal and update with interrupts blocked */
    LOCK_INTERRUPTS;

    /* Traverse the list looking for the best-fit free block
     * Best fit is the smallest one of at least the requested size
     * This will help to minimise wastage
     */
    while (*pnode != NULL)
    {
        unsigned nodesize = (*pnode)->length;
        if (top_down)
        {
            if ((char *)(*pnode) < heap_midpoint(pheap_info, heap_num))
            {
                nodesize = pheap_info->heap[heap_num].heap_size - 1;
            }
        }
        else
        {
            if ((char *)(*pnode) >= heap_midpoint(pheap_info, heap_num))
            {
                nodesize = pheap_info->heap[heap_num].heap_size - 1;
            }
        }
        if (((*pnode)->length >= size ) && (nodesize < bestsize))
        {
            pbest = pnode;
            bestsize = (*pnode)->length;
        }
        pnode = &(*pnode)->u.next;
    }
    if (pbest)
    {
        char *addr;
        mem_node *newnode;
        if ((*pbest)->length >= size + sizeof(mem_node) + MIN_SPARE)
        {
            /* There's enough space to allocate something else
             * so keep the existing free block and allocate the space at the top
             * In this case the allocation size is exactly what was requested
             */
            if (top_down)
            {
               addr = (char *)(*pbest) + (*pbest)->length - size;
               (*pbest)->length -= (size + sizeof(mem_node));
            }
            else
            {
                mem_node prev_node = **pbest;
                addr = (char *)(*pbest);
                *pbest = (mem_node *)(addr + size + sizeof(mem_node));
                **pbest = prev_node;
                (*pbest)->length -= (size + sizeof(mem_node));
            }
        }
        else
        {
            /* Not enough extra space to be useful
             * Replace the free block with an allocated one
             * The allocation size is the whole free block
             */
            addr = (char *)(*pbest);
            size = (*pbest)->length;
            *pbest = (*pbest)->u.next;

#ifdef HEAP_DEBUG
            heap_debug_freenodes--;
            /* This node gets reused. To simplify the logic, temporarily add its size
             * to the available space. It gets taken off again below.
             */
            heap_debug_free += sizeof(mem_node) + GUARD_SIZE;
#endif
            pheap_info->heap[heap_num].heap_free += sizeof(mem_node) + GUARD_SIZE;

        }
        /* Finally populate the header for the newly-allocated block */
        newnode = (mem_node *)addr;
        newnode->length = size - GUARD_SIZE;
        newnode->u.magic = MAGIC_WORD_WITH_OWNER();
#ifdef HEAP_DEBUG
        heap_debug_allocnodes++;
        heap_debug_alloc += newnode->length;
        heap_debug_free -= (size + sizeof(mem_node));
        if (heap_debug_min_free > heap_debug_free)
        {
            heap_debug_min_free = heap_debug_free;
        }
#endif
        pheap_info->heap[heap_num].heap_free -= (size + sizeof(mem_node));

        UNLOCK_INTERRUPTS;
        return addr + sizeof(mem_node);
    }
    /* No suitable block found */
    UNLOCK_INTERRUPTS;

    return NULL;
}

#if defined(__KCC__)
/**
 * \brief Replace the node representing current heap start in the freelist with
 *        the new heap start.
 */
static void update_freelist_with_new_start(mem_node **pfreelist,
                                           char* curr_start,
                                           char *new_start)
{
    if (new_start <= curr_start)
    {
        /* Nothing to do here if the new start is moving back or
           not moving at all. */
        return;
    }
    /* Do all the list-traversal and update with interrupts blocked. */
    LOCK_INTERRUPTS;

    /* Traverse the free list to find existing free block
       with the current start address. */
    while (*pfreelist != NULL)
    {
        mem_node* curr_node = *pfreelist;
        if ((char *) curr_node == curr_start)
        {
            /* Matching block found. Make the new start a node. */
            mem_node* new_node = (mem_node *)new_start;
            new_node->u.next = curr_node->u.next;
            new_node->length = curr_node->length - (new_start - curr_start);
            /* Finally, replace the new start in the free list. */
            *pfreelist = new_node;
            break;
        }
        pfreelist = &(curr_node->u.next);
    }

    /* Unlock interrupt and exit. */
    UNLOCK_INTERRUPTS;
}
#endif /* defined(__KCC__) */

/**
 * \brief Claim back the free memory.
 */
static void coalesce_free_mem(mem_node **pfreelist,
                              char *free_mem,
                              unsigned len)
{
    mem_node *curnode, **pnode;
    heap_names heap_num;

    /* Do all the list-traversal and update with interrupts blocked. */
    LOCK_INTERRUPTS;

    curnode = *pfreelist;

    /* Traverse the free list to see if we can coalesce an
       existing free block with this one. */
    while (curnode != NULL)
    {
        if ((char *) curnode + curnode->length +
            sizeof(mem_node) == (char *)free_mem)
        {
            /* Matching block found. */
            break;
        }
        curnode = curnode->u.next;
    }

    /* Find the associated heap number. */
    heap_num = get_heap_num(free_mem);

    if (curnode != NULL)
    {
        /* The immediately-previous block is free
           add the one now being freed to it. */
        curnode->length += len;
#ifdef HEAP_DEBUG
        heap_debug_free += len;
#endif
        /* update free heap */
        pheap_info->heap[heap_num].heap_free += len;

    }
    else
    {
        /* Previous block wasn't free so add the now-free block to the free
           list. Note the length is unchanged from when it was allocated (unless
           we have guard words, which get added to the free space). */
        curnode = init_heap_node(free_mem, len);

#ifdef PMALLOC_DEBUG
        if (curnode == NULL)
        {
            panic_diatribe(PANIC_AUDIO_FREE_INVALID,
                           (DIATRIBE_TYPE)((uintptr_t)free_mem));
        }
#endif
        /* It should not return NULL. */
        curnode->u.next = *pfreelist;
        *pfreelist = curnode;
    }

    /* Now check if there is a free block immediately after the found / new one */
    pnode = pfreelist;
    while (*pnode != NULL)
    {
        if ((char*)(*pnode) == (char*)curnode + curnode->length + sizeof(mem_node))
        {
            /* Matching block found. */
            break;
        }
        pnode = &(*pnode)->u.next;
    }
    if (*pnode != NULL)
    {
        /* The immediately-following block is free, add it to the current one
           and remove from the free list. */
        curnode->length += (*pnode)->length + sizeof(mem_node);
        *pnode = (*pnode)->u.next;
#ifdef HEAP_DEBUG
        heap_debug_freenodes--;
        heap_debug_free += sizeof(mem_node) + GUARD_SIZE;
#endif

        /* update free heap */
        pheap_info->heap[heap_num].heap_free += sizeof(mem_node) + GUARD_SIZE;

    }
    UNLOCK_INTERRUPTS;
}


#if defined(INSTALL_EXTERNAL_MEM)
/**
 * \brief Initialises external SRAM heap from MIB.
 */
static bool init_external_heap(void)
{
    uint32 ext_sram_size = 0;
    uint32 ext_p0_heap_size = 0;
    uint32 ext_p1_heap_size = 0;

    patch_fn_shared(heap);

#if defined(UNIT_TEST_BUILD)
    /* Stub values for unit test. */
    ext_sram_size = EXT_HEAP_SIZE;
    ext_p0_heap_size = EXT_HEAP_SIZE;
    ext_p1_heap_size = 0;
#elif defined(INSTALL_MIB) && defined(__KCC__) && !defined(RUNNING_ON_KALSIM)
    int16 len_read = 0;
    ext_heap_allocation_mib_value mib_value;

    memset(&mib_value, 0, sizeof(mib_value));
    len_read = mibgetreqstr(HEAPALLOCATIONSRAM,
                            (uint8*) &mib_value,
                            sizeof(mib_value));
    if (len_read != sizeof(mib_value))
    {
        return FALSE;
    }

    /* Size of external SRAM and heaps in kibioctets. */
    ext_sram_size = (uint32) (((mib_value.ext_ram_size_msb << 8) |
                                mib_value.ext_ram_size_lsb) * KIBYTE);

    /* Read P0 and P1 sizes. */
    ext_p0_heap_size = (uint32) (((mib_value.ext_p0_data_heap_size_msb << 8) |
                                   mib_value.ext_p0_data_heap_size_lsb) * KIBYTE);
    ext_p1_heap_size = (uint32) (((mib_value.ext_p1_data_heap_size_msb << 8) |
                                   mib_value.ext_p1_data_heap_size_lsb) * KIBYTE);
#endif  /* INSTALL_MIB */

    if (ext_sram_size == 0)
    {
        /* No external heap - Ignore init and return true. */
        return TRUE;
    }

    if (proc_single_core_running())
    {
        /* If p1 is disabled ignore mib and allocate p1 heap to p0. */
        ext_p0_heap_size += ext_p1_heap_size;
        ext_p1_heap_size = 0;
    }

    if (((ext_p0_heap_size + ext_p1_heap_size) == 0 ) ||
        ((ext_p0_heap_size + ext_p1_heap_size) > ext_sram_size))

    {
        /* If not, panic on fault return. */
        fault_diatribe(FAULT_AUDIO_INVALID_CONFIG, ext_sram_size);
        return FALSE;
    }

    /* If the number of external heaps increases, add a loop and replace
       EXT_HEAP with the loop counter. */

    if (ext_p0_heap_size > 0)
    {
        /* NOTE: when/if we have more heaps we can change this into a for loop. */
        set_heap_info(&pheap_info->heap[HEAP_EXT],
                      SRAM_START_ADDR,
                      SRAM_START_ADDR + ext_p0_heap_size,
                      ext_p0_heap_size,
                      FALSE);

        if (!ext_heap_enable(TRUE))
        {
            return FALSE;
        }
    }

#if defined(SUPPORTS_MULTI_CORE) && defined(__KCC__)
    /* If second processor is not active, all heap is with core0.
       Note: Only P0 will arrive here. */
    if (ext_p1_heap_size > 0)
    {
        /* initialising heap for P1 at a later point will initialise its
         * freelist
         */
        set_heap_info(&processor_heap_info_list[PROC_PROCESSOR_1].heap[HEAP_EXT],
                (char*) (pheap_info->heap[HEAP_EXT].heap_end),
                (char*) (pheap_info->heap[HEAP_EXT].heap_end) + ext_p1_heap_size,
                        ext_p1_heap_size, FALSE);
    }
#endif /* defined(SUPPORTS_MULTI_CORE) && defined(__KCC__) */

    PL_PRINT_P2(TR_PL_MALLOC, "UT: SRAM_START ADDR = %p, size = %u",
                pheap_info->heap[HEAP_EXT].heap_start,
                pheap_info->heap[HEAP_EXT].heap_size);
    return TRUE;
}
#endif /* INSTALL_EXTERNAL_MEM */

#ifdef __KCC__

/*************************** Set the memory bank size  ********************************/
/* RAM is not mapped to DM banks in all
 * platforms. For crescendo/aura it is
 * 32K. For the rest, it doesn't have any effect
 */
#ifndef DM_RAM_BANK_SIZE
#error "DM_RAM_BANK_SIZE is not defined for the platform"
#endif

/* Rounds down the address to the bank start address. In other words,
   it returns the start address of the bank that contains the address
   given as argument. */
static inline char* round_down_to_dm_bank_addr(char* address)
{
    uintptr_t retval = (uintptr_t)(address);
    retval = (retval / DM_RAM_BANK_SIZE) * DM_RAM_BANK_SIZE;
    return (char*) retval;
}

#if defined(INSTALL_DYNAMIC_HEAP) && !defined(UNIT_TEST_BUILD)
/**
 * \brief Read the heap allocation configuration from MIB.
 */
static bool get_heap_allocation(heap_dyn_size_config* value)
{
    bool ret_val = TRUE;
    int16 len_read;
    heap_allocation_mib_value mib_value;

    memset(&mib_value, 0, sizeof(mib_value));
    len_read = mibgetreqstr(HEAPALLOCATION, (uint8*)&mib_value, sizeof(mib_value));
    if (len_read != sizeof(mib_value))
    {
        /* Return straight away if MIB get fails. */
        return FALSE;
    }

    SET_HEAP_MAIN_SIZE(PROC_PROCESSOR_0,mib_value.p0_heap_size);
    /* Set the shared memory size to 0 to avoid any configuration later in
       heap_dynamic_config. */
    SET_HEAP_SHARED_SIZE(PROC_PROCESSOR_0,0);
    SET_HEAP_SLOW_SIZE(PROC_PROCESSOR_0, mib_value.p0_slow_heap_size);

    if (INVALID_HEAP_SIZE(GET_HEAP_MAIN_SIZE(PROC_PROCESSOR_0)) ||
        INVALID_HEAP_SLOW_SIZE(GET_HEAP_SLOW_SIZE(PROC_PROCESSOR_0)))
    {
        WARN_MEM_CFG_MSG("Heap config: Invalid p0 heap config from mib");
        ret_val = FALSE;
    }

#if defined(SUPPORTS_MULTI_CORE)
    SET_HEAP_MAIN_SIZE(PROC_PROCESSOR_1, mib_value.p1_heap_size);
    SET_HEAP_SHARED_SIZE(PROC_PROCESSOR_1, 0);
    SET_HEAP_SLOW_SIZE(PROC_PROCESSOR_1, mib_value.p1_slow_heap_size);

    if (INVALID_HEAP_SIZE(value->p[PROC_PROCESSOR_1].heap_size[HEAP_MAIN]) ||
        INVALID_HEAP_SLOW_SIZE(value->p[PROC_PROCESSOR_1].heap_size[HEAP_SLOW]))
    {
        WARN_MEM_CFG_MSG("Heap config: Invalid p1 heap config from mib");
        ret_val = FALSE;
    }

    if ((GET_HEAP_MAIN_SIZE(PROC_PROCESSOR_0) + GET_HEAP_MAIN_SIZE(PROC_PROCESSOR_1) > HEAP_MAX_SIZE) ||
        (GET_HEAP_SLOW_SIZE(PROC_PROCESSOR_0) + GET_HEAP_SLOW_SIZE(PROC_PROCESSOR_1) > SLOW_HEAP_MAX_SIZE))
    {
        WARN_MEM_CFG_MSG("Heap config: Invalid heap size config from mib");
        ret_val = FALSE;
    }
    /* Finally log read values and exit. */
#ifdef CHIP_HAS_SLOW_DM_RAM
    DBG_MEM_CFG_MSG2("Mib heap configuration for processor 1\n"
                     "heap HEAP size       = 0x%05x\n"
                     "heap HEAP_SLOW size  = 0x%05x\n",
                     mib_value.p1_heap_size * KIBYTE,
                     mib_value.p1_slow_heap_size * KIBYTE);
#else
    DBG_MEM_CFG_MSG1("\nMib heap configuration for processor 1\n"
                     "heap HEAP size  = 0x%05x\n",
                     mib_value.p1_heap_size * KIBYTE);
#endif /* defined(CHIP_HAS_SLOW_DM_RAM) */
#endif /* defined(SUPPORTS_MULTI_CORE) */

#ifdef CHIP_HAS_SLOW_DM_RAM
    DBG_MEM_CFG_MSG2("\nMib heap configuration for processor 0\n"
                     "heap HEAP size       = 0x%05x\n"
                     "heap HEAP_SLOW size  = 0x%05x\n",
                     mib_value.p0_heap_size * KIBYTE,
                     mib_value.p0_slow_heap_size * KIBYTE);
#else
    DBG_MEM_CFG_MSG1("\nMib heap configuration for processor 0\n"
                     "heap HEAP size  = 0x%05x\n",
                     mib_value.p0_heap_size * KIBYTE);
#endif /* CHIP_HAS_SLOW_DM_RAM */

    return ret_val;
}
#else /* defined(INSTALL_DYNAMIC_HEAP) && !defined(UNIT_TEST_BUILD) */
static bool get_heap_allocation(heap_dyn_size_config* value)
{
    NOT_USED(value);
    return FALSE;
}
#endif /* defined(INSTALL_DYNAMIC_HEAP) && !defined(UNIT_TEST_BUILD) */

/**
 * \brief Sets the maximum heap sizes.
 * \note  The maximum heap sizes are bigger in single core mode.
 */
static void set_heap_max_sizes(bool second_core_enabled,
                               heap_sizes* heap_max_sizes)
{
    if (heap_alloc_thread_offload_is_configured())
    {
        /*
         * HEAP_MAX_SIZE = HEAP_SIZE(_heap_size) (provided by linker)
         * HEAP_MAX_SIZE_THREAD_OFFLOAD = HEAP_SIZE(__thread_offload_heap_size)
         */
        DBG_MEM_CFG_MSG2("Heap config: Heap size: linker 0x%05x, thread offload 0x%05x",
                         HEAP_MAX_SIZE, HEAP_MAX_SIZE_THREAD_OFFLOAD);
        DBG_MEM_CFG_MSG2("Heap config: Shared Heap size: linker 0x%05x, thread offload 0x%05x",
                         SHARED_HEAP_MAX_SIZE, SHARED_HEAP_MAX_SIZE_THREAD_OFFLOAD);
#ifdef CHIP_HAS_SLOW_DM_RAM
        /* Slow heap sizes are same for linker, single mode and thread offload mode. */
        DBG_MEM_CFG_MSG2("Heap config: Slow Heap size: linker 0x%05x, thread offload 0x%05x",
                         SLOW_HEAP_MAX_SIZE, SLOW_HEAP_MAX_SIZE);
#endif
    }
    else
    {
        /*
         * HEAP_MAX_SIZE = HEAP_SIZE(_heap_size) (provided by linker)
         * HEAP_MAX_SIZE_SINGLE_MODE = HEAP_SIZE(_single_mode_heap2_size)
         */
        DBG_MEM_CFG_MSG2("Heap config: Heap size: linker 0x%05x, single mode 0x%05x",
                         HEAP_MAX_SIZE, HEAP_MAX_SIZE_SINGLE_MODE);
        DBG_MEM_CFG_MSG2("Heap config: Shared Heap size: linker 0x%05x, single mode 0x%05x",
                         SHARED_HEAP_MAX_SIZE, SHARED_HEAP_MAX_SIZE_SINGLE_MODE);
#ifdef CHIP_HAS_SLOW_DM_RAM
        /* Slow heap sizes are same for linker, single mode and thread offload mode */
        DBG_MEM_CFG_MSG2("Heap config: Slow Heap size: linker 0x%05x, single mode 0x%05x",
                         SLOW_HEAP_MAX_SIZE, SLOW_HEAP_MAX_SIZE);
#endif
    }

    if (second_core_enabled)
    {
        heap_max_sizes->heap_size[HEAP_MAIN]   = HEAP_MAX_SIZE;
        heap_max_sizes->heap_size[HEAP_SHARED] = SHARED_HEAP_MAX_SIZE;
    }
    else
    if (heap_alloc_thread_offload_is_configured())
    {
        heap_max_sizes->heap_size[HEAP_MAIN]   = HEAP_MAX_SIZE_THREAD_OFFLOAD;
        heap_max_sizes->heap_size[HEAP_SHARED] = SHARED_HEAP_MAX_SIZE_THREAD_OFFLOAD;
    }
    else
    {
        heap_max_sizes->heap_size[HEAP_MAIN]   = HEAP_MAX_SIZE_SINGLE_MODE;
        heap_max_sizes->heap_size[HEAP_SHARED] = SHARED_HEAP_MAX_SIZE_SINGLE_MODE;
    }
#ifdef CHIP_HAS_SLOW_DM_RAM
    heap_max_sizes->heap_size[HEAP_SLOW]   = SLOW_HEAP_MAX_SIZE;
#endif
}

#define VERIFY_NEW_HEAP_START(cur_start,new_start) \
        PL_ASSERT((cur_start + sizeof(mem_node) + ((mem_node *)cur_start)->length) > new_start)

/**
 * \brief Attempt to extend configured heap boundaries to a block boundary.
 */
static void heap_configure_and_align(heap_config *main_core,
                                     heap_config *second_core,
                                     heap_sizes *heap_max_sizes,
                                     bool have_dyn_config,
                                     heap_dyn_size_config *dyn_heap_config,
                                     mem_node **freelist)
{
    /* The main core (processor 0) configuration data. */
    heap_info* p0_size_info = NULL;
    char* p0_start;
    char* p0_end;
    unsigned p0_size;

#ifdef SUPPORTS_MULTI_CORE
    /* The second core (processor 1) configuration data. */
    heap_info* p1_size_info = NULL;
    char* p1_start = NULL;
    char* p1_end = NULL;
    unsigned p1_size = 0, p1_dyn_size= 0;
#endif

    /* Variables used in the for loops.  */
    mem_node** pfreelist;
    unsigned heap_original_size;
    unsigned current_heap_max_size;
    unsigned heap_num;

    /* Configure and align to memory bank boundary for all the heaps. */
    for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
    {
        /* Use this patchpoint if one heap need modifying. */
        patch_fn_shared(heap);

#if defined(INSTALL_EXTERNAL_MEM)
        if (heap_num == HEAP_EXT)
        {
            /* External heap already initialised with the correct values. */
            continue;
        }
#endif /* INSTALL_EXTERNAL_MEM */

        /* init_heap set the maximum heap sizes for p0 to enable memory
           allocation for some services, assuming single core mode. */
        p0_size_info = &main_core->heap[heap_num];

        /* Read the maximum size of the current heap. */
        current_heap_max_size = heap_max_sizes->heap_size[heap_num];
        /* Take note of the current heap size set for P0. */
        heap_original_size = p0_size_info->heap_size;

        /* P0 end address remains the same. Start with assumption of single core
           mode, i.e., P0 heap is maximum possible size and then adjust (make it
           smaller) according to P0/P1 dynamic requirements. */
        p0_end = p0_size_info->heap_end;

#ifdef SUPPORTS_MULTI_CORE
        /* Check if the second core is enabled. */
        if (second_core != NULL)
        {
            /* Read the size of the second core heap.*/
            p1_size_info = &second_core->heap[heap_num];
            /* Adjust the required heap size to the dynamic heap configuration
               values if present. Note that p0 size is calculated from p1 size.
               So only the dynamic size set for p1 is required. */
            if (have_dyn_config)
            {
                p1_dyn_size = dyn_heap_config->p[PROC_PROCESSOR_1].heap_size[heap_num];
            }
            p1_size = MIN(p1_dyn_size, current_heap_max_size);

            /* Also, each processor should have at least 1 bank worth of heap,
               except for shared heap, which is halved for each processor. */
            if (heap_num == HEAP_SHARED)
            {
                p1_size = ROUND_UP_TO_WHOLE_WORDS(current_heap_max_size / 2);
            }
            else
            {
                p1_size = MAX(p1_dyn_size, DM_RAM_BANK_SIZE);
            }

            /* P1's heap starts at the bottom and P0's heap follows on top. */
            p1_start = p0_end - current_heap_max_size;
            p1_end   = p1_start + p1_size;
            /* The shared heap boundary between p0 and p1 doesn't have to be
               in a memory bank boundary. */
            if (heap_num != HEAP_SHARED)
            {
                p1_end = round_down_to_dm_bank_addr(p1_end);
                p1_size =  p1_end - p1_start;
            }
            /* Now update P0's heap start and end. */
            p0_start = p1_end;
            p0_size = p0_end - p0_start;
        }
        else
#else
        UNUSED(second_core);
        UNUSED(have_dyn_config);
#endif /* SUPPORTS_MULTI_CORE */
        {
            /* Second core is not enabled, but there could be thread offload
               and max heap size will be different. Calculate the P0 heap
               start and size. */
            p0_start = p0_end - current_heap_max_size;
            p0_size =  p0_end - p0_start;
        }

        if (p0_start == p0_size_info->heap_start)
        {
            /* If second core is not enabled and there is no thread offload, P0
               gets all the memory, which is what was configured for anyway.
               So nothing to do here */
            continue;
        }

        /* At the end of it, P0's heap start is being moved ahead. Make sure
           it doesn't move past the first allocation. */
        VERIFY_NEW_HEAP_START(p0_size_info->heap_start, p0_start);

        /* Some basic defensive checks to make sure the sizes are still valid. */
        if (p0_size > current_heap_max_size)
        {
            WARN_MEM_CFG_MSG3("Heap %d configuration error while aligning\n"
                              "p0 heap to mem banks boundary!\n"
                              "p0 heap size after aligning: 0x%05x,\n"
                              "exceeds maximum heap size : 0x%05x, "
                              "this heap will be unusable",
                              heap_num, p0_size, current_heap_max_size);
            break;
        }

        /* Get P0's free list. */
        pfreelist = &freelist[heap_num];

        /* P0's heap free list needs an update with the new start node. */
        update_freelist_with_new_start(pfreelist,
                                       p0_size_info->heap_start,
                                       p0_start);

        /* Now save the heap configuration for both processors. */
        set_heap_info(p0_size_info, p0_start, p0_end, p0_size, TRUE);

#ifdef SUPPORTS_MULTI_CORE
        if (p1_size >= current_heap_max_size)
        {
            WARN_MEM_CFG_MSG3("Heap %d configuration error while aligning\n"
                              "p1 heap to mem banks boundary!\n"
                              "p1 heap size after aligning: 0x%05x,\n"
                              "exceeds maximum heap size : 0x%05x, "
                              "this heap will be unusable",
                              heap_num, p1_size, current_heap_max_size);
            break;
        }

        if (second_core != NULL)
        {
            set_heap_info(p1_size_info, p1_start, p1_end, p1_size, TRUE);
        }
#endif

        /* heap_min was the initial size of the heap set in init_heap. After
           config, P0's heap size could've been reduced. Adjust the free memory
           tracker and debug watermarks, if applicable. */
        if (p0_size_info->heap_size != heap_original_size)
        {
            int len = heap_original_size - p0_size_info->heap_size;
            p0_size_info->heap_free -= len;
#ifdef HEAP_DEBUG
            /* Adjust the minimum available heap after a reconfiguration. */
            heap_debug_min_free -= len;
            heap_debug_free -= len;
#endif
        }
    }
}
#endif /* __KCC__ */

#if defined(INSTALL_EXTERNAL_MEM)
#if defined(__KCC__) && !defined(RUNNING_ON_KALSIM) && defined(SUPPORTS_MULTI_CORE)
static void heap_init_sram(void)
{
    /* The P0 does not know if the chip has SRAM at this point.
       This will be configured after the mib keys are read. */
     if (PROC_SECONDARY_CONTEXT())
     {
         ext_heap_enable(TRUE);
     }
}
#else
static inline void heap_init_sram(void)
{
     /* For unit tests , initialise the external heap now. */
     if (!init_external_heap())
     {
        PL_PRINT_P0(TR_PL_MALLOC,"External SRAM heap initialisation failed");
     }
}
#endif  /* __KCC__*/
#endif /* INSTALL_EXTERNAL_MEM */

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Initialise the memory heap
 *
 * \param[in] config pointer to P0 heap_config object. This object holds
 *                   the pmalloc configuration for all processors. The
 *                   pmalloc configuration for all processors is done
 *                   by P0, and a pointer to it is an input to 'init_
 *                   heap' on other processors (P1,P2,P3).
 *                   This parameter is ignored on P0.
 */
void init_heap(heap_config *config)
{
    unsigned i;

    init_host_heap();

    /* Initialise the heap config. */
    if (PROC_SECONDARY_CONTEXT())
    {
        /* We should have been passed a pointer to config prepared
           on and by P0. */
        PL_ASSERT(config != NULL);

        /* Processor 1 init - More than 2 processors are not supported now. */
        pheap_info = &(config[PROC_PROCESSOR_1]);

        /* Processor 0 must have configured all heaps. so nothing to do here. */
    }
    else
    {
        /* Processor 0 init */
        pheap_info = &processor_heap_info_list[PROC_PROCESSOR_0];
        /* Set the heap information for p0:
           Main heap & shared heap starts off in single mode. All allocations
           are forced to be from the DM1, ie, top of heap. This will be
           realigned after configuration/MIB is available. */
        pheap_info->pref_bank = MALLOC_PREFERENCE_DM1;
        pheap_info->relaxmallocstrictness = MALLOC_STRICTNESS_DM1_FALLBACK;
        set_heap_info(&pheap_info->heap[HEAP_MAIN],
                      heap_single_mode,
                      heap_single_mode + HEAP_MAX_SIZE_SINGLE_MODE,
                      HEAP_MAX_SIZE_SINGLE_MODE,
                      FALSE);
        set_heap_info(&pheap_info->heap[HEAP_SHARED],
                      shared_heap_single_mode,
                      shared_heap_single_mode + SHARED_HEAP_MAX_SIZE_SINGLE_MODE,
                      SHARED_HEAP_MAX_SIZE_SINGLE_MODE,
                      FALSE);
#ifdef CHIP_HAS_SLOW_DM_RAM
        set_heap_info(&pheap_info->heap[HEAP_SLOW],
                      slow_heap,
                      slow_heap + SLOW_HEAP_MAX_SIZE,
                      SLOW_HEAP_MAX_SIZE,
                      FALSE);
#endif
    }

    /* A potential place to fix the config. */
    patch_fn_shared(heap);

    /* pheap_info is processor specific static global and it now points to the
       heap_config specific to the current processor. */
    for (i = 0; i < HEAP_ARRAY_SIZE; i++)
    {
#ifdef INSTALL_EXTERNAL_MEM
        if (i == HEAP_EXT)
        {
            heap_init_sram();
        }
        else
#endif /* INSTALL_EXTERNAL_MEM */
        {
            freelist[i] = init_heap_node(pheap_info->heap[i].heap_start,
                                         pheap_info->heap[i].heap_size);
        }
    }

#ifdef HEAP_DEBUG
    heap_debug_min_free = heap_debug_free;
    heap_debug_alloc = 0;
    heap_debug_allocnodes = 0;
#endif
}

/**
 * \brief Configure the memory heap.
 *
 * \note  This is called after booting up the subsystem to claim the remaining
 *        memory area in the memory map as heap. It also reads the MIB key to
 *        decide how much memory to be allocated to each processors. Currently
 *        only dual core is handled.
 */
#ifdef __KCC__
void config_heap(void)
{
    heap_config *second_core_config = NULL;
    heap_dyn_size_config dyn_heap_config;
    bool have_dyn_config = FALSE;
    heap_sizes heap_max_sizes;

    patch_fn_shared(heap);

    PROC_RETURN_IF_SECONDARY();

#if defined(INSTALL_EXTERNAL_MEM)
    if (!init_external_heap())
    {
        PL_PRINT_P0(TR_PL_MALLOC,"External SRAM heap initialisation failed");
    }
#endif /*INSTALL_EXTERNAL_MEM*/

#if defined(SUPPORTS_MULTI_CORE)
    /* Second processor could be disabled after boot via an ACCMD command. That
       would get us into this function for a second time. So while
       processor_heap_info_list is zero-initialised we need to make sure its
       irelevant parts are in fact zero. */
    memset(&processor_heap_info_list[PROC_PROCESSOR_1], 0, sizeof(heap_config));

    /* If second processor is not active, all heap is with core0. Note: Only
       P0 will arrive here. */
    if (proc_multiple_cores_running())
    {
        /* Currently only 2 cores are expected. */
        second_core_config = &processor_heap_info_list[PROC_PROCESSOR_1];
    }
#endif /* defined(SUPPORTS_MULTI_CORE) */

    /* Only P0 reaches here. Set the maximum heap sizes allowed. */
    /* Read any dynamic heap configuration */
    have_dyn_config = get_heap_allocation(&dyn_heap_config);

    /* Sets the maximum heap sizes. */
    set_heap_max_sizes(second_core_config != NULL ? TRUE : FALSE,
                       &heap_max_sizes);

    heap_configure_and_align(pheap_info,
                             second_core_config,
                             &heap_max_sizes,
                             have_dyn_config,
                             &dyn_heap_config,
                             freelist);

    heap_alloc_configure_strictness(PROC_PROCESSOR_0);

    /* update the bank access permissions */
    set_dm_memory_bank_permissions();
    if (second_core_config != NULL)
    {
        heap_alloc_configure_strictness(PROC_PROCESSOR_1);
    }

    /* core1 will populate its freelist while it boots up.
       Nothing to do here. */
}
#endif /* __KCC__ */

/**
 * \brief   Tell caller the address of the processor heap list
 *          On P0 this address can then be stored in the LUT,
 *          if multi-core solution. If not P0, return value
 *          can be ignored.
 *
 * \return  Pointer to (local) 'heap_config' object.
 */
heap_config *heap_get_heap_config(void)
{
    return processor_heap_info_list;
}

/**
 *  \brief Check whether the pointer is in heap or not.
 */
bool is_addr_in_heap(void* addr)
{
    unsigned heap_num;
    heap_info *heap_cfg = pheap_info->heap;

    for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
    {
        if ((heap_cfg[heap_num].heap_start <= (char*)addr) &&
            ((char*)addr < heap_cfg[heap_num].heap_end))
        {
            return TRUE;
        }
    }
    return FALSE;
}

#if defined(INSTALL_EXTERNAL_MEM)
/**
 * \brief Enable the external heap.
 */
bool ext_heap_enable(bool enable)
{
    bool result = TRUE;

    if (hal_get_sram_enabled() && enable)
    {
        char *heap = pheap_info->heap[HEAP_EXT].heap_start;
        unsigned heapsize =  pheap_info->heap[HEAP_EXT].heap_size;

        PL_PRINT_P0(TR_PL_MALLOC, "Enabling SRAM heap\n");

        if (ext_heap_is_disabled() && (heapsize > 0))
        {
            /* initilaise the freelist for P0 */
           freelist[HEAP_EXT] = init_heap_node(heap, heapsize);
        }

        /* validate the external, memory initialisation */
        if( ext_heap_is_disabled() ||
            get_heap_size_node(heap) != heapsize )
        {
            if(!ext_heap_is_disabled())
            {
                /* heap is not initialised properly. Panic */
                PL_PRINT_P0(TR_PL_MALLOC, "External heap is invalid\n");
                fault_diatribe(FAULT_AUDIO_MEMORY_ACCESS_FAILED, (uintptr_t)heap);
            }

            PL_PRINT_P0(TR_PL_MALLOC, "SRAM is still disabled\n");
            result = FALSE;
        }
    }
    else
    {
        PL_PRINT_P0(TR_PL_MALLOC, "Disabling SRAM heap\n");
        /* Disable the heap. */
        freelist[HEAP_EXT] = NULL;
    }

    return result;
}
#endif

/**
 * \brief Allocate a block of (at least) the requested size.
 */
#ifdef PMALLOC_DEBUG
void *heap_alloc_debug(unsigned size, unsigned preference,
                       const char *file, unsigned int line)
#else
void *heap_alloc(unsigned size, unsigned preference)
#endif
{
    void *addr = NULL;
    /* pref_dm1: If TRUE: Memory allocation mechanism tries to allocate
     * evenly between DM1/DM2 for MALLOC_PREFERENCE_NONE.
     *
     * There is no need for a separate fallback option. If you ask for DM1,
     * you will get DM1. If you ask for DM2, you will get DM2 address. All
     * requests will be honoured from heap/slow_heap/shared_heap - the order
     * depending on specific MALLOC_PREFERENCE - until we run out of heap.
     * Exception is if request is explicitly for shared and external memory,
     * which can be denied for lack of free blocks in the requested heaps.
     */
    bool pref_dm1 = TRUE;

#ifndef CHIP_HAS_SLOW_DM_RAM
    /* If chip has no slow RAM, request for FAST is treated as
     * No preference
     */
    if (preference == MALLOC_PREFERENCE_FAST)
    {
        preference = MALLOC_PREFERENCE_NONE;
    }
#endif

    switch (pheap_info->relaxmallocstrictness)
    {
        case MALLOC_STRICTNESS_DM1_FALLBACK:
        {
            /* DM1 as the preferred region for MALLOC_PREFERENCE_NONE
               option with no load balancing but fallback. */
            if (preference == MALLOC_PREFERENCE_NONE)
            {
                preference = MALLOC_PREFERENCE_DM1;
            }
            pref_dm1 = TRUE;
            break;
        }
        case MALLOC_STRICTNESS_DM2_FALLBACK:
        {
            /* DM2 as the preferred region for MALLOC_PREFERENCE_NONE
               option with no load balancing but fallback.*/
            if (preference == MALLOC_PREFERENCE_NONE)
            {
                preference = MALLOC_PREFERENCE_DM2;
            }
            pref_dm1 = FALSE;
            break;
        }
        case MALLOC_STRICTNESS_EVEN_FALLBACK:
        default:
        {
            /* Values 0, 1 (and unsupported values) are the same.
               Use load-balancing, i.e  swap between DM1 and DM2 for each
               allocation. This is really a crude attempt at load balancing
               since it swaps between DM1 and DM2 for all allocations on the
               core, be it shared/ext/slow/main.
               Update preferred bank for next time. */
            if (pheap_info->pref_bank == MALLOC_PREFERENCE_DM1)
            {
                pref_dm1 = TRUE;
                pheap_info->pref_bank = MALLOC_PREFERENCE_DM2;
            }
            else
            {
                pref_dm1 = FALSE;
                pheap_info->pref_bank = MALLOC_PREFERENCE_DM1;
            }
            break;
        }
    }

    /* Don't do anything if zero size requested. */
    if (size == 0)
    {
        return NULL;
    }
    PL_PRINT_P1(TR_PL_MALLOC, "Allocating size %d ", size);

    /* If last time DM1 alloc failed prefer doing DM2 first.
       Note that we specifically DON'T want to try to find the
       'best-fit' block across all heaps - in a mostly-unallocated
       state that would tend just fill up the one with less free space. */
    switch (preference)
    {
        case MALLOC_PREFERENCE_DM1:
        {
#ifdef CHIP_HAS_SLOW_DM_RAM
            addr = heap_alloc_internal(size, HEAP_SLOW, TRUE);
            if (addr != NULL)
            {
                break;
            }
#endif
            addr = heap_alloc_internal(size, HEAP_MAIN, TRUE);
            if (addr == NULL)
            {
                /* Failed to allocate memory. Use the shared heap as
                   the last fallback. */
                addr = heap_alloc_internal(size, HEAP_SHARED, TRUE);
            }
            break;
        }
        case MALLOC_PREFERENCE_DM2:
        {
#ifdef CHIP_HAS_SLOW_DM_RAM
            addr = heap_alloc_internal(size, HEAP_SLOW, FALSE);
            if (addr == NULL)
#endif
            {
                addr = heap_alloc_internal(size, HEAP_MAIN, FALSE);
                if (addr == NULL)
                {
                    /* Failed to allocate memory. Use the shared heap as
                       the last fallback. */
                    addr = heap_alloc_internal(size, HEAP_SHARED, FALSE);
                }
            }
            addr = convert_to_dm2(addr);
            break;
        }
        case MALLOC_PREFERENCE_FAST:
        {
            if (pref_dm1)
            {
                addr = heap_alloc_internal(size, HEAP_MAIN, TRUE);
                if (addr != NULL)
                {
                    break;
                }

                /* Fast is full. */
#ifdef CHIP_HAS_SLOW_DM_RAM
                addr = heap_alloc_internal(size, HEAP_SLOW, TRUE);
                if (addr != NULL)
                {
                    break;
                }
#endif
                /* Failed to allocate memory. Use the shared heap as
                   the last fallback. */
                addr = heap_alloc_internal(size, HEAP_SHARED, TRUE);
                if (addr != NULL)
                {
                    break;
                }
            }
            else
            {
                addr = heap_alloc_internal(size, HEAP_MAIN, FALSE);

                /* Fast is full. */
#ifdef CHIP_HAS_SLOW_DM_RAM
                if (addr == NULL)
                {
                    addr = heap_alloc_internal(size, HEAP_SLOW, FALSE);
                }
#endif
                if (addr == NULL)
                {
                    /* Failed to allocate memory. Use the shared heap as
                       the last fallback. */
                    addr = heap_alloc_internal(size, HEAP_SHARED, FALSE);
                }
                addr = convert_to_dm2(addr);
            }
            break;
        }
        case MALLOC_PREFERENCE_SHARED:
        {
            /* For shared heap, we could choose DM1 or DM2 address. For now,
               allocate as if DM1, because internally that is quicker. */
            addr = heap_alloc_internal(size, HEAP_SHARED, TRUE);
            break;
        }
#if defined(INSTALL_EXTERNAL_MEM)
        case MALLOC_PREFERENCE_EXTERNAL:
        {
            if (!ext_heap_is_disabled())
            {
                addr = heap_alloc_internal(size, HEAP_EXT, FALSE);
            }
            break;
        }
#endif
        case MALLOC_PREFERENCE_NONE:
        default:
        {
            if (pref_dm1)
            {
                /* DM1 slow ram heap first*/
#ifdef CHIP_HAS_SLOW_DM_RAM
                addr = heap_alloc_internal(size, HEAP_SLOW, TRUE);
                if (addr != NULL)
                {
                    break;
                }
#endif
                addr = heap_alloc_internal(size, HEAP_MAIN, TRUE);
                if (addr == NULL)
                {
                    /* Failed to allocate memory. Use the shared heap as
                       the last fallback. */
                    addr = heap_alloc_internal(size, HEAP_SHARED, TRUE);
                }
            }
            else
            {
#ifdef CHIP_HAS_SLOW_DM_RAM
                addr = heap_alloc_internal(size, HEAP_SLOW, FALSE);
                if (addr == NULL)
#endif
                {
                    addr = heap_alloc_internal(size, HEAP_MAIN, FALSE);
                    if (addr == NULL)
                    {
                        /* Failed to allocate memory. Use the shared heap as
                           the last fallback */
                        addr = heap_alloc_internal(size, HEAP_SHARED, FALSE);
                    }
                }
                addr = convert_to_dm2(addr);
            }
            break;
        }
    }

#ifdef PMALLOC_DEBUG
    if (addr != NULL)
    {
        /* Record where this block was allocated from */
        mem_node *node = (mem_node *)((char *)addr - sizeof(mem_node));

        node->file = file;
        node->line = line;
        node->guard = HEAP_GUARD_WORD;
        *((unsigned int *)((char *)addr + node->length)) = HEAP_GUARD_WORD;
    }
#endif
    PL_PRINT_P1(TR_PL_MALLOC,"Allocated address from heap = %p\n", addr);

    patch_fn_shared(heap);

    return addr;
}

/**
 * \brief Free a previously-allocated block.
 */
void heap_free(void *ptr)
{
    mem_node **pfreelist=NULL;
    mem_node *node;
    unsigned heap_num;

    if (ptr == NULL)
    {
        /* free(NULL) is a no-op  */
        return;
    }

    PL_PRINT_P1(TR_PL_FREE, "ptr to be freed %p..", ptr);
    /* Move DM2 addresses back into DM1 address range. */
    ptr = convert_to_dm1(ptr);
    node = (mem_node *)((char *)ptr - sizeof(mem_node));

    heap_num = get_heap_num(ptr);

#ifdef INSTALL_EXTERNAL_MEM
    if ((heap_num == HEAP_EXT) && !hal_get_sram_enabled())
    {
        /* Freed after disabling heap. Ignore. */
        PL_PRINT_P1(TR_PL_FREE, "free ptr %p on a disabled SRAM HEAP ..", ptr);
        return;
    }
#endif /* INSTALL_EXTERNAL_MEM */

    PL_PRINT_P1(TR_PL_FREE, "is in heap %u\n", heap_num);

    if (heap_num == HEAP_INVALID)
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID,
                       (DIATRIBE_TYPE)((uintptr_t)ptr));
    }

    pfreelist = &freelist[heap_num];

    /* Check that the address being freed looks sensible. */
    if (IS_NOT_MAGIC_WORD(node->u.magic))
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID,
                       (DIATRIBE_TYPE)((uintptr_t)ptr));
    }

    /* Check that the length seems plausible. Function will panic with
       PANIC_AUDIO_FREE_INVALID if memory is not in the heap. */
    get_heap_num((char *)ptr + node->length - 1);

#ifdef PMALLOC_DEBUG
    if (node->file == NULL)
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID,
                       (DIATRIBE_TYPE)((uintptr_t)node));
    }
    if (node->guard != HEAP_GUARD_WORD)
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION,
                       (DIATRIBE_TYPE)((uintptr_t)node));
    }
    if (*((unsigned int *)((char *)ptr + node->length)) != HEAP_GUARD_WORD)
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION,
                       (DIATRIBE_TYPE)((uintptr_t)node));
    }
    node->file = NULL;
    node->line = 0;
    node->guard = 0;
#endif
    node->u.magic = 0;

#ifdef HEAP_DEBUG
    heap_debug_allocnodes--;
    heap_debug_alloc -= node->length;
#endif

    /* Coalesce the freed block. */
    coalesce_free_mem(pfreelist, (char*) node,
                      node->length + sizeof(mem_node) + GUARD_SIZE);
}

/**
 * \brief Get the size of a previously-allocated block.
 */
unsigned heap_sizeof(void *ptr)
{
    mem_node *node = (mem_node *)((char *)ptr - sizeof(mem_node));

    if (ptr == NULL)
    {
        return 0;
    }

    /* Check that the address looks sensible. */
    if (IS_NOT_MAGIC_WORD(node->u.magic))
    {
        /* Might want a (debug-only?) panic here. */
        return 0;
    }

    return node->length;
}

/**
 * \brief Heap size in words.
 */
unsigned heap_size(void)
{
    unsigned i, size = 0;

    for (i = 0; i < HEAP_ARRAY_SIZE; i++)
    {
        size += pheap_info->heap[i].heap_size;
    }

    return (size >> LOG2_ADDR_PER_WORD);
}

#ifdef PMALLOC_DEBUG
/**
 * \brief Check that a previously-allocated block looks sensible.
 */
void heap_validate(void *ptr)
{
    mem_node *node;

    /* Move DM1 addresses back into DM2 address range */
    ptr = convert_to_dm1(ptr);
    if (ptr == NULL)
    {
        /* Shouldn't happen, but don't bother checking if NULL */
        return;
    }

    /* Check that the address looks sensible */
    node = (mem_node *)((char *)ptr - sizeof(mem_node));
    if (IS_NOT_MAGIC_WORD(node->u.magic))
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION,
                       (DIATRIBE_TYPE)((uintptr_t)ptr));
    }

    /* Check that the length seems plausible. get_heap_num panics if
       the pointer is not in any heap memory. */
    get_heap_num((char *)ptr + node->length - 1);
}
#endif /* PMALLOC_DEBUG */

#ifdef DESKTOP_TEST_BUILD
/**
 * \brief Test-only function to get total and largest free space.
 */
void heap_get_freestats(unsigned *maxfree,
                        unsigned *totfree,
                        unsigned *tracked_free)
{
    unsigned heap_num, tot_size = 0, max_size = 0, tracked = 0;
    mem_node *curnode;

    for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
    {
        tracked += pheap_info->heap[heap_num].heap_free;
        curnode = freelist[heap_num];
        while (curnode != NULL)
        {
            if (curnode->length - GUARD_SIZE > max_size)
            {
                max_size = curnode->length - GUARD_SIZE;
            }
            tot_size += curnode->length - GUARD_SIZE;
            curnode = curnode->u.next;
        }
    }

    if (maxfree != NULL)
    {
        *maxfree = max_size;
    }
    if (totfree != NULL)
    {
        *totfree = tot_size;
    }
    if (tracked_free != NULL)
    {
        *tracked_free = tracked;
    }
}

#endif /* DESKTOP_TEST_BUILD */

#ifdef INSTALL_DM_MEMORY_PROFILING
/**
 * NAME
 *   heap_tag_dm_memory
 *
 * \brief Function to 'tag' a block of allocated heap memory
 *
 * \param[in] ptr  Pointer to the allocated block to be tagged
 * \param[in] id   The owner id to tag the allocated block with
 *
 * \return Returns TRUE if  tagging succeeded,
 *                 FALSE if tagging failed (ptr is not a heap block).
 */
bool heap_tag_dm_memory(void *ptr, DMPROFILING_OWNER id)
{
    mem_node *node;

    node = (mem_node *)((unsigned char *)ptr - sizeof(mem_node));
    if (IS_NOT_MAGIC_WORD(node->u.magic))
    {
        return FALSE;
    }
    node->u.magic = MAGIC_WORD_WITH_ID(id);
    return TRUE;
}
#endif /* INSTALL_DM_MEMORY_PROFILING */

