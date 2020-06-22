/****************************************************************************
 * COMMERCIAL IN CONFIDENCE
 * Copyright (c) 2008 - 2017 Qualcomm Technologies International, Ltd.
 *
 ***************************************************************************
 * \defgroup pl_ext_malloc External  Memory allocation functionality
 *
 * \file pl_ext_malloc.h
 * \ingroup pl_ext_malloc
 *
 * Interface for ext memory allocation
 *
 ****************************************************************************/

#ifndef _PL_EXT_MALLOC_H_
#define _PL_EXT_MALLOC_H_

/****************************************************************************
Include Files
*/
#include "types.h"

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
extern void *ext_malloc_debug(unsigned int numBytes, const char *file, unsigned int line);
#define ext_malloc(numBytes) ext_malloc_debug(numBytes, __FILE__, __LINE__)
#else
extern void *ext_malloc(unsigned int numBytes);
#endif


/**
 * NAME
 *   ext_malloc_enable
 *
 * \brief  enable or disable the external malloc
 *
 * \param[in]  enable : TRUE to enable and FALSE to disable
 *
 * \return bool
 */
extern bool ext_malloc_enable(bool enable);

/**
 * NAME
 *   is_addr_in_ext_heap
 *
 * \brief  Determine if an address in the the external heap (SRAM)
 *
 * \param[in]  ptr: pointer to the memory
 *
 * \return bool
 */
extern bool is_addr_in_ext_heap(void *ptr);

#endif /*_PL_EXT_MALLOC_H_*/

