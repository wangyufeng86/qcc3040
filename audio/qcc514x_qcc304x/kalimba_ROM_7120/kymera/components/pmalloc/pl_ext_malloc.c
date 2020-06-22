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

/**
 * NAME
 *   ext_malloc
 *
 * \brief Memory allocation in the external memory (SRAM)
 *
 * \param[in] numBytes number of bytes required, as returned by sizeof.
 * See xppmalloc for details of what a "byte" is.
 *
 * \return pointer to the block of memory allocated
 *
 */
#ifdef PMALLOC_DEBUG
void *ext_malloc_debug(unsigned int numBytes, const char *file, unsigned int line)
#else
void *ext_malloc(unsigned int numBytes)
#endif
{
    void *ext_heap_mem;

    /* xpmalloc must return Null if requested to allocate Zero bytes */
    if(!hal_get_sram_enabled())
    {
        return(NULL);
    }

#ifdef PMALLOC_DEBUG
        ext_heap_mem = heap_alloc_debug(numBytes, MALLOC_PREFERENCE_EXTERNAL, file, line);
#else
        ext_heap_mem = heap_alloc(numBytes, MALLOC_PREFERENCE_EXTERNAL);
#endif

        /* panic out of memory */
        if(ext_heap_mem == NULL)
        {
            panic_diatribe(PANIC_AUDIO_HEAP_EXHAUSTION, numBytes);
        }

    return ext_heap_mem;
}


/**
 * NAME
 *   ext_malloc_enable
 *
 * \brief  enable or disable the external malloc
 *
 * \param[in]  enable : TRUE to enable and FALSE to disable
 *
 * \return bool - TRUE on success
 */
#ifdef RUNNING_ON_KALSIM
bool ext_malloc_enable(bool enable)
{
   return  FALSE;
}
#else
bool ext_malloc_enable(bool enable)
{
   return  ext_heap_enable(enable);
}
#endif /* RUNNING_ON_KALSIM */


/**
 * NAME
 *   is_addr_in_ext_heap
 *
 * \brief  Determine if an address in the the external heap (SRAM)
 *
 * \param[in]  ptr: pointer to the memory
 *
 * \return bool - TRUE if address in external heap
 */
#ifdef RUNNING_ON_KALSIM
bool is_addr_in_ext_heap(void *ptr)
{
   return  FALSE;
}
#else
bool is_addr_in_ext_heap(void *ptr)
{
   return  (get_heap_num(ptr) == HEAP_EXT);
}
#endif /* RUNNING_ON_KALSIM */
