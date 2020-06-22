/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup adaptor Adaptors to kymera interface
 *
 * \file adaptor_private.h
 * \ingroup adaptor
 *
 * This file contains private API for adaptor component.
 */

#ifndef ADAPTOR_PRIVATE_H
#define ADAPTOR_PRIVATE_H

#include "types.h"
#include "stream_prim.h"
#include "status_prim.h"
#include "system_keys_prim.h"
#include "adaptor/adaptor.h"

typedef struct
{
    uint32 heap_size;
    uint32 heap_current;
    uint32 heap_min;
    uint32 pool_size;
    uint32 pool_current;
    uint32 pool_min;
} ADAPTOR_GET_MEM_USAGE;

/* Generic adaptor callback. Status takes one of the values from status_prim.h */
typedef bool (*ADAPTOR_GENERIC_CALLBACK)(unsigned conidx, STATUS_KYMERA status, void *parameters);

/**
 * \brief  Get the value of a system key.
 *
 * \param  key  The name of the key
 *
 * \return The value of the key or 0 if there is no such key.
 */
uint32 adaptor_get_system_key(SYSTEM_KEYS_MSG key);

/**
 * \brief  Set the value of a system key.
 *
 * \param  key  The name of the key
 *
 * \return TRUE if the key was set properly, FALSE otherwise.
 */
bool adaptor_set_system_key(SYSTEM_KEYS_MSG key, uint32 value);

#if defined(SUPPORTS_MULTI_CORE)
/**
 * \brief  Helper function for sending variable length key-value system messages to P1
 *
 * \param  key_value_pairs An array of uint16 key-value pairs
 * \param  num_pairs       Number of key-value pairs in the KIP message
 *
 * \return                 Returns false if memory allocation for sending the
 *                         message fails or if called from the wrong processor.
 */
extern bool kip_adaptor_send_system_key_value_pairs(uint32* key_value_pairs,
                                                    uint16 num_pairs);
#endif /* defined(SUPPORTS_MULTI_CORE) */

#if defined(INSTALL_OBPM_ADAPTOR)
extern bool obpm_owns_pointer(void *context);
#else
#define obpm_owns_pointer(x) FALSE
#endif

#endif /* ADAPTOR_PRIVATE_H */
