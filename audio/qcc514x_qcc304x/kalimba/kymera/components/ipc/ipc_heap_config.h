/*****************************************************************************
*
* Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
*
*****************************************************************************/
/** \file
 *
 *  This is the main public project header for the \c ipc heap config library.
 *
 */
/****************************************************************************
Include Files
*/

#ifndef IPC_HEAP_CONFIG_H
#define IPC_HEAP_CONFIG_H

/* To get the definition of 'heap_config' */
#include "pmalloc/pl_malloc.h"

/****************************************************************************
Public Type Declarations
*/

/****************************************************************************
Public Constant and macros
*/

/****************************************************************************
Public Variable Declarations
*/

/****************************************************************************
Public Function Declarations
*/

/**
 * \brief  On P1: return pointer to heap configuration of P0,
 *         on P0: return NULL.
 *         Uses IPC LUT entry IPC_LUT_ID_HEAP_CONFIG_SHARED_ADDR
 *
 * \return Pointer to 'heap_config' object.
 */
extern heap_config *ipc_lut_get_heap_config(void);

/**
 * \brief  On P1: do nothing,
 *         on P0: store pointer to heap configuration of P0.
 *         Uses IPC LUT entry IPC_LUT_ID_HEAP_CONFIG_SHARED_ADDR
 *
 * \return (none)
 */
extern void ipc_lut_set_heap_config(void);

#endif /* IPC_HEAP_CONFIG_H */

