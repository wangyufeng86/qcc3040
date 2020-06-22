/****************************************************************************
 * COMMERCIAL IN CONFIDENCE
 * Copyright (c) 2012 - 2018 Qualcomm Technologies International, Ltd.
 ****************************************************************************
 * \file heap_alloc.c
 * \ingroup pl_malloc
 *
 * Memory allocation/free functionality
 *
 ****************************************************************************/
/****************************************************************************
Include Files
*/

#include "pl_malloc_private.h"

/****************************************************************************
Private Configuration Macro Declarations
*/
#define UNUSED(x)   ((void)(x))

/*
 * Directly accessing the linker provided symbol
 * as an address and typecasting to integer to make
 * the compiler happy
 */
#define HEAP_SIZE(x) ((unsigned)((uintptr_t)&(x)))
#define HEAP_ADDR(x) HEAP_SIZE(x)

/*
 * Directly accessing the linker provided symbol
 * as an address.
 */
#define GET_LINKER_ADDRESS(x) ((char*)(uintptr_t)&(x))

/******************************* dm1 heap configuration  ******************************/
#ifdef HEAP_SIZE_DM1
/* heap on DM1. */
DM1_HEAP_LOCATION char          heap1[HEAP_SIZE_DM1];
/* Linker provides the calculated heapsize after
 * calculating the available free RAM.*/
extern unsigned                 _heap1_size;
#define HEAP1_MAX_SIZE          HEAP_SIZE(_heap1_size)
extern unsigned                 _single_mode_heap1_size;
#define HEAP1_MAX_SIZE_SINGLE_MODE  \
                                HEAP_SIZE(_single_mode_heap1_size)

#define INVALID_HEAP_DM1_SIZE(x) ((x) < HEAP_SIZE_DM1)

#else  /* HEAP_SIZE_DM1*/
#define heap1                   ((char*)NULL)
#define HEAP_SIZE_DM1           0
#define HEAP1_MAX_SIZE          0
#define HEAP1_MAX_SIZE_SINGLE_MODE 0
#define INVALID_HEAP_DM1_SIZE(x) FALSE

#endif /* HEAP_SIZE_DM1*/

#define DM1_ADDRESS_MASK (uintptr_t)0x000FFFFFu
#if defined(__KCC__) && defined(HEAP_MAX_BOUNDARY_GUARD)
extern uintptr_t                 _dm1_max_address;
#define HEAP_BOUNDARY_GUARD_SIZE(end)  (((uintptr_t)(end) & DM1_ADDRESS_MASK) + HEAP_MAX_BOUNDARY_GUARD) > (HEAP_ADDR(_dm1_max_address))?\
                                       (((uintptr_t)(end) & DM1_ADDRESS_MASK) + HEAP_MAX_BOUNDARY_GUARD - (HEAP_ADDR(_dm1_max_address))) : 0
#else
#define HEAP_BOUNDARY_GUARD_SIZE(end) 0
#endif


/******************************** dm2 heap configuration  *****************************/
#ifdef HEAP_SIZE_DM2
DM2_HEAP_LOCATION char          heap2[HEAP_SIZE_DM2];
/* Linker provides the calculated heapsize after
 * calculating the available free RAM.*/
extern unsigned                 _heap2_size;
#define HEAP2_MAX_SIZE          HEAP_SIZE(_heap2_size)
extern unsigned                 _single_mode_heap2_size;
#define HEAP2_MAX_SIZE_SINGLE_MODE  \
                                HEAP_SIZE(_single_mode_heap2_size)
extern unsigned                 _thread_offload_heap2_size;
#define HEAP2_MAX_SIZE_THREAD_OFFLOAD  \
                                HEAP_SIZE(_thread_offload_heap2_size)

#define INVALID_HEAP_DM2_SIZE(x) ((x) < HEAP_SIZE_DM2)

#else  /* HEAP_SIZE_DM2 */
#define heap2                   ((char*)NULL)
#define HEAP_SIZE_DM2           0
#define HEAP2_MAX_SIZE          0
#define HEAP2_MAX_SIZE_SINGLE_MODE 0
#define INVALID_HEAP_DM2_SIZE(x)  FALSE

#endif /* HEAP_SIZE_DM2 */


/**************************** shared heap configuration *******************************/
/* Define the HEAP_SIZE_SHARED to 0 for unit tests and non DUAL CORE configs if not defined,
 * if defined, redefined. */
#ifdef HEAP_SIZE_SHARED
#undef HEAP_SIZE_SHARED
#endif
#define HEAP_SIZE_SHARED        0

#define heap3                   ((char*)NULL)
#define HEAP3_MAX_SIZE          0
#define HEAP3_MAX_SIZE_SINGLE_MODE 0


/****************************************************************************
Private Macro Declarations
*/

#define MAGIC_WORD 0xabcd01ul

#ifdef PMALLOC_DEBUG
#define HEAP_GUARD_WORD 0x987654ul
#define GUARD_SIZE (sizeof(unsigned int))
#else
#define GUARD_SIZE 0
#endif

#define MIN_SPARE 8

#define KIBYTE (1024)

/* If DM2 free heap is below this level, don't use it
 * as a prefered heap  unless request with DM2 preference
 */
#define DM2_RESERVE_HEAP_WATERMARK 0x1000

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

/****************************************************************************
Private Function Declarations
*/

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
static DM_SHARED_ZI heap_config processor_heap_info_list[CHIP_NUM_CORES];

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

/**
 * NAME
 *   get_heap_num
 *
 * \brief Returns which heap the pointer points to. Panics if memory is not in the heap.
 *
 */
heap_names get_heap_num(void *ptr)
{
    heap_info *heap_cfg = pheap_info->heap;
    heap_names heap_num;

    patch_fn_shared(heap);
    for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
    {
        if ((heap_cfg[heap_num].heap_start <= (char*)ptr) && ((char*)ptr < heap_cfg[heap_num].heap_end))
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
 * NAME
 *   is_dm2_heap_loaded
 *
 * \brief Returns TRUE if DM2 heap has more heap space that DM1 heap
 *
 */
static inline bool is_dm2_heap_loaded(void)
{

    /* If DM2 space - reserved space is less than DM1 space
     * it returns TRUE suggesting that DM2 is loaded more
     * This reserved space is kept to satisfy any requests
     * for DM2 specific requests
     */

    return (( pheap_info->heap[HEAP_DM2].heap_free <
               DM2_RESERVE_HEAP_WATERMARK ))? TRUE: FALSE;

}


/**
 * NAME
 *   set_heap_info
 *
 * \brief Set the heap information which consist of start, end address and size.
 *
 */
static void set_heap_info(heap_info* heap, char* start, char* end, unsigned size, bool resizing)
{
    unsigned boundary_size = HEAP_BOUNDARY_GUARD_SIZE(end);

    PL_ASSERT(start + size == end);
    PL_ASSERT(boundary_size <= size);
    heap->heap_start = start;
    heap->heap_size = size - boundary_size;
    heap->heap_end = end - boundary_size;

    if (!resizing)
    {
        /* set heap_free = 0, It will be initialised while initialising nodes */
        heap->heap_free = 0;
    }
}

/**
 * NAME
 *   init_heap_node
 *
 * \brief Initialise the heap mem node
 *
 */
static mem_node* init_heap_node(char* heap, unsigned heap_size)
{
    unsigned freelength;
    heap_names heap_num;
    mem_node *node = (mem_node *)heap;

    if (( heap_size <= sizeof(mem_node) ) || heap == NULL)
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
 * NAME
 *   allocate memory internally
 *
 * \brief internal call to allocate memory
 *
 */
static void *heap_alloc_internal(unsigned size, unsigned heap_num)
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
        if (((*pnode)->length >= size ) && ((*pnode)->length < bestsize))
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
        if (bestsize >= size + sizeof(mem_node) + MIN_SPARE)
        {
            /* There's enough space to allocate something else
             * so keep the existing free block and allocate the space at the top
             * In this case the allocation size is exactly what was requested
             */
            addr = (char *)(*pbest) + (*pbest)->length - size;
            (*pbest)->length -= (size + sizeof(mem_node));
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
        newnode->u.magic = MAGIC_WORD;
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

/**
 * NAME
 *   coalesce_free_mem
 *
 * \brief claim back the free memory
 *
 */
static void  coalesce_free_mem(mem_node **pfreelist, char *free_mem, unsigned len)
{
    mem_node *curnode, **pnode;
    heap_names heap_num;

    /* Do all the list-traversal and update with interrupts blocked */
    LOCK_INTERRUPTS;

    curnode = *pfreelist;

    /* Traverse the free list to see if we can coalesce an
     * existing free block with this one */
    while (curnode != NULL)
    {
        if ( (char *) curnode + curnode->length +
              sizeof(mem_node) == (char *)free_mem)
        {
            /* Matching block found */
            break;
        }
        curnode = curnode->u.next;
    }

    /* find the associated heap number */
    heap_num = get_heap_num(free_mem);

    if (curnode != NULL)
    {
        /* The immediately-previous block is free
         * add the one now being freed to it
         */
        curnode->length += len;
#ifdef HEAP_DEBUG
        heap_debug_free += len;
#endif
        /* update free heap */
        pheap_info->heap[heap_num].heap_free += len;

    }
    else
    {
        /* Previous block wasn't free
         * so add the now-free block to the free list
         * Note length is unchanged from when it was allocated
         * (unless we have guard words, which get added to the free space)
         */
        curnode = init_heap_node( free_mem, len );

#ifdef PMALLOC_DEBUG
        if (curnode == NULL)
        {
            panic_diatribe(PANIC_AUDIO_FREE_INVALID,
                          (DIATRIBE_TYPE)((uintptr_t)free_mem));
        }
#endif
        /* it should not return NULL */
        curnode->u.next = *pfreelist;
        *pfreelist = curnode;
    }

    /* Now check if there is a free block immediately after the found / new one */
    pnode = pfreelist;
    while (*pnode != NULL)
    {
        if ((char*)(*pnode) == (char*)curnode + curnode->length + sizeof(mem_node))
        {
            /* Matching block found */
            break;
        }
        pnode = &(*pnode)->u.next;
    }
    if (*pnode != NULL)
    {
        /* The immediately-following block is free
         * add it to the current one and remove from the free list
         */
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

/****************************************************************************
Public Function Definitions
*/

/**
 * NAME
 *   init_heap
 *
 * \brief Initialise the memory heap
 *
 * \param[in] config
 *        pointer to P0 heap_config object. This object holds
 *        the pmalloc configuration for all processors. The
 *        pmalloc configuration for all processors is done
 *        by P0, and a pointer to it is an input to 'init_
 *        heap' on other processors (P1,P2,P3).
 *        This parameter is ignored on P0.
 */
void init_heap(heap_config *config)
{
    UNUSED(config);
    /* processor 0 init */
    pheap_info = &processor_heap_info_list[0];
    /* Set the heap information for p0 */
    set_heap_info(&pheap_info->heap[HEAP_DM1],      heap1, heap1 + HEAP_SIZE_DM1,     HEAP_SIZE_DM1, FALSE);
    set_heap_info(&pheap_info->heap[HEAP_DM2],      heap2, heap2 + HEAP_SIZE_DM2,     HEAP_SIZE_DM2, FALSE);

    /* A potential place to fix the config. */
    patch_fn_shared(heap);

    {
        unsigned i;
        /* pheap_info is processor specific static global and it
         * now points to the heap_config specific to the current
         * processor
         */
        for (i = 0; i < HEAP_ARRAY_SIZE; i++)
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
 * NAME
 *   config_heap
 *
 * \brief Configure the memory heap. This is called after booting up
 *        the subsystem to claim the remaining memory area in the memory
 *        map as heap.
 *
 *        This is not used on bluecore, but included for completeness
 */
#ifdef __KCC__
void config_heap(void)
{
    patch_fn_shared(heap);

    PROC_RETURN_IF_SECONDARY();

#if defined(INSTALL_EXTERNAL_MEM)
    if(!init_external_heap())
    {
        PL_PRINT_P0(TR_PL_MALLOC,"External SRAM heap initialisation failed");
    }
#endif /*INSTALL_EXTERNAL_MEM*/
}
#endif /* __KCC__ */

/**
 * NAME
 *   heap_get_heap_config
 *
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
 * NAME
 *   is_addr_in_heap
 *
 *  \brief check whether the pointer is in heap or not
 */
bool is_addr_in_heap( void* addr)
{
    unsigned heap_num;
    heap_info *heap_cfg = pheap_info->heap;

    for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
    {
        if ((heap_cfg[heap_num].heap_start <= (char*)addr) && ((char*)addr < heap_cfg[heap_num].heap_end))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * NAME
 *   heap_alloc
 *
 * \brief Allocate a block of (at least) the requested size
 *
 */
#ifdef PMALLOC_DEBUG
void *heap_alloc_debug( unsigned size, unsigned preference,
                        const char *file, unsigned int line)
#else
void *heap_alloc(unsigned size, unsigned preference)
#endif
{
    void *addr=NULL;
    bool pref_dm1;
    bool fallback = FALSE;

    switch (pheap_info->relaxmallocstrictness)
    {
    case 2:
        /* Value 2: DM1 preference */
        pref_dm1 = TRUE;
        fallback = TRUE;
        break;
    case 3:
        /* Value 3: DM2 preference */
        pref_dm1 = FALSE;
        fallback = TRUE;
        break;
    case 1:
        fallback = TRUE;
        /* fall through is deliberate */
    default:
        /* Values 0, 1 (and unsupported values) use load-balancing. */
        pref_dm1 = is_dm2_heap_loaded();
        break;
    }

    /* If chip has no slow RAM, request for FAST is treated as
     * No preference
     */
    if( preference == MALLOC_PREFERENCE_FAST)
    {
        preference =  MALLOC_PREFERENCE_NONE;
    }

    /* Don't do anything if zero size requested */
    if (size == 0)
    {
        return NULL;
    }
    PL_PRINT_P1(TR_PL_MALLOC, "Allocating size %d ", size);

    /* If last time DM1 alloc failed prefer doing DM2 first
     * Note that we specifically DON'T want to try to find the
     * 'best-fit' block across all heaps - in a mostly-unallocated
     * state that would tend just fill up the one with less free space.
     */
    switch(preference)
    {
    case MALLOC_PREFERENCE_DM1:
        addr = heap_alloc_internal(size, HEAP_DM1);
        if ((addr == NULL) && (fallback))
        {
            addr = heap_alloc_internal(size, HEAP_DM2);
        }
        break;

    case MALLOC_PREFERENCE_DM2:
        if ((addr = heap_alloc_internal(size, HEAP_DM2)) != NULL)
        {
            break;
        }
        addr = heap_alloc_internal(size, HEAP_DM1);
        break;

    case MALLOC_PREFERENCE_NONE:
    default:
        if (pref_dm1)
        {
            /* DM1 slow ram heap first*/
            if ((addr = heap_alloc_internal(size, HEAP_DM1)) != NULL)
            {
                break;
            }

            /* DM1 heap is full */
            addr = heap_alloc_internal(size, HEAP_DM2);
        }
        else
        {
            /* get from DM2 fast ram */
            if ((addr = heap_alloc_internal(size, HEAP_DM2)) != NULL)
            {
                break;
            }

            /* DM2 heap is full */
            addr = heap_alloc_internal(size, HEAP_DM1);
        }
        break;
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
    PL_PRINT_P1(TR_PL_MALLOC,"Allocated address from heap = %lx\n", (uintptr_t)addr);

    patch_fn_shared(heap);

    return addr;
}

/**
 * NAME
 *   heap_free
 *
 * \brief Free a previously-allocated block
 *
 */
void heap_free(void *ptr)
{
    mem_node **pfreelist=NULL;
    mem_node *node;
    heap_names heap_num;

    if (ptr == NULL)
    {
        /* free(NULL) is a no-op  */
        return;
    }

    PL_PRINT_P1(TR_PL_FREE, "ptr to be freed %lx..", (uintptr_t)ptr);

    node = (mem_node *)((char *)ptr - sizeof(mem_node));

    heap_num = get_heap_num(ptr);

    PL_PRINT_P1(TR_PL_FREE, "is in heap %d\n",heap_num);

    if( heap_num == HEAP_INVALID)
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID, (DIATRIBE_TYPE)((uintptr_t)ptr));
    }


    pfreelist = &freelist[heap_num];

    /* Check that the address being freed looks sensible */
    if (node->u.magic != MAGIC_WORD)
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID, (DIATRIBE_TYPE)((uintptr_t)ptr));
    }

    /* Check that the length seems plausible. Function will panic with
     * PANIC_AUDIO_FREE_INVALID if memory is not in the heap. */
    get_heap_num((char *)ptr + node->length - 1);

#ifdef PMALLOC_DEBUG
    if (node->file == NULL)
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID, (DIATRIBE_TYPE)((uintptr_t)node));
    }
    if (node->guard != HEAP_GUARD_WORD)
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION, (DIATRIBE_TYPE)((uintptr_t)node));
    }
    if (*((unsigned int *)((char *)ptr + node->length)) != HEAP_GUARD_WORD)
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION, (DIATRIBE_TYPE)((uintptr_t)node));
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

    /* coalsce the freed block */
    coalesce_free_mem( pfreelist,(char*) node,
                      node->length + sizeof(mem_node)+ GUARD_SIZE);

}


/**
 * NAME
 *   heap_sizeof
 *
 * \brief Get the size of a previously-allocated block
 *
 */
unsigned heap_sizeof(void *ptr)
{
    mem_node *node = (mem_node *)((char *)ptr - sizeof(mem_node));

    if (ptr == NULL)
    {
        return 0;
    }

    /* Check that the address looks sensible */
    if (node->u.magic != MAGIC_WORD)
    {
        /* Might want a (debug-only?) panic here */
        return 0;
    }

    return node->length;
}

/**
 * NAME
 *   heap_size
 *
 * \brief Heap size in words
 *
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
 * NAME
 *   heap_validate
 *
 * \brief Check that a previously-allocated block looks sensible
 *
 */
void heap_validate(void *ptr)
{
    mem_node *node;
    node = (mem_node *)((char *)ptr - sizeof(mem_node));

    if (ptr == NULL)
    {
        /* Shouldn't happen, but don't bother checking if NULL */
        return;
    }

    /* Check that the address looks sensible */
    if (node->u.magic != MAGIC_WORD)
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION, (DIATRIBE_TYPE)((uintptr_t)ptr));
    }

    /* Check that the length seems plausible. get_heap_num panics if the pointer is not
     * in any heap memory. */
    get_heap_num((char *)ptr + node->length - 1);
}
#endif /* PMALLOC_DEBUG */


#ifdef DESKTOP_TEST_BUILD
/**
 * NAME
 *   heap_get_freestats
 *
 * \brief Test-only function to get total and largest free space
 *
 */
void heap_get_freestats(unsigned *maxfree, unsigned *totfree, unsigned *tracked_free)
{
    unsigned heap_num, tot_size = 0, max_size = 0;
    mem_node *curnode;

    *tracked_free = 0;

    for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
    {
        *tracked_free += pheap_info->heap[heap_num].heap_free;
        curnode = freelist[heap_num];
        while (curnode != NULL)
        {
            if (curnode->length -GUARD_SIZE > max_size)
            {
                max_size = curnode->length - GUARD_SIZE;
            }
            tot_size += curnode->length - GUARD_SIZE;
            curnode = curnode->u.next;
        }
    }

    *maxfree = max_size;
    *totfree = tot_size;
}

#endif /* DESKTOP_TEST_BUILD */

