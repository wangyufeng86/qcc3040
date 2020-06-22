/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
 ************************************************************************//**
 * \file
 * header for the internals of the PM memory allocation system
 *
 * This file contains bits of the PM memory system that we don't want
 * users to see.
 */

#ifndef MALLOC_PM_PRIVATE_H
#define MALLOC_PM_PRIVATE_H

/****************************************************************************
Include Files
*/
#include "malloc_pm.h"
#include "types.h"
#include "platform/pl_trace.h"
#include "platform/pl_intrinsics.h"
#include "panic/panic.h"
#include "fault/fault.h"
#include "io_map.h"

#ifdef __KCC__
#ifdef KAL_ARCH4
#ifndef UNIT_DATA_SECTION_DEFINED
#define UNIT_DATA_SECTION_DEFINED
/* Keep all configuration user P0 for dual core*/
#pragma unitzeroinitdatasection DM_P0_RW_ZI
#pragma unitdatasection         DM_P0_RW
#endif
#endif /* KAL_ARCH4 */
#endif /* __KCC__ */

/****************************************************************************
Public Macro Declarations
*/

/****************************************************************************
Public Type Declarations
*/

/****************************************************************************
Global Variable Definitions
*/

/****************************************************************************
Public Function Prototypes
*/

/**
 * NAME
 *   heap_pm_init_start_offset
 *
 * \brief Initialise the offset used to calculate the heap start address
 *
 * \param[in] offset number of bytes (see notes for what that means) used for
 * other systems (e.g. patchpoints) from PM space that is shared with P0's heap
 *
 * \note The offset address can only be set before PM heap is initialised
 *
 * \note
 *   The offset is specified in units of the smallest addressable storage unit
 *   in PM on the processor used. This is 8 bits on KAL_ARCH4 and KAL_ARCH5 or 32 bits
 *   on KAL_ARCH3.
 *
 */
void heap_pm_init_start_offset(unsigned offset);

/**
 * NAME
 *   init_heap_pm
 *
 * \brief Initialise memory heap
 *
 */
void init_heap_pm(void);

#if defined(SUPPORTS_MULTI_CORE)
/**
 * NAME
 *   reconfigure_heap_pm
 *
 * \brief Reclaim P1's memory heap as P0's heap
 *
 */
void reconfigure_heap_pm(void);
#endif

/**
 * NAME
 *   heap_alloc_pm
 *
 * \brief Memory allocation using heap
 *
 * FUNCTION
 *   Allocate a chunk of memory from the heap.
 *   Returns a Null pointer if no suitable block is available.
 *   The memory is not initialised.
 *
 * \param[in] size number of addressable units required.
 * \param[in] preference_core core in which we want to allocate the memory
 *
 * \return pointer to the block of memory allocated
 *
 * \note
 *   The size is specified in units of the smallest addressable storage unit
 *   in PM on the processor used. This is 8 bits on KAL_ARCH4 and KAL_ARCH5 or 32 bits
 *   on KAL_ARCH3.
 *
 */
void_func_ptr heap_alloc_pm(unsigned size_byte, unsigned preference_core);


/**
 * NAME
 *   heap_free_pm
 *
 * \brief Free memory allocated from the PM heap
 *
 * \param[in] ptr pointer to the memory to be freed
 *
 * \return None
 *
 */
void heap_free_pm(void_func_ptr ptr);

/**
 * NAME
 *   heap_sizeof_pm
 *
 * \brief Get size of memory block allocated from the heap
 *
 * \param[in] ptr pointer to the memory
 *
 * \return Actual available size
 *
 */
unsigned heap_sizeof_pm(void_func_ptr ptr);

/**
 * NAME
 *   is_in_pm_heap
 *
 * \brief Returns whether provided address is within the boundaries
 * of the PM heap
 *
 * \param[in] ptr pointer to the memory
 *
 * \return True if it is. False otherwise.
 *
 */
bool is_in_pm_heap(void_func_ptr);

#endif /* MALLOC_PM_PRIVATE_H */


