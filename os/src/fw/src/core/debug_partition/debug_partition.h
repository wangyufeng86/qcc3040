/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file
 * Interfaces to access Debug Partition.
 */

#ifndef DEBUG_PARTITION_H
#define DEBUG_PARTITION_H

#include <app/debug_partition/debug_partition_if.h>
#include <app/debug_partition/debug_partition_data_if.h>
#include <panic/panic.h>

/**
 * Callback function for erasing debug partition
 *
 * \param result Result code indicating success or error reason. */
typedef void (*DEBUG_PARTITION_ERASE_CB)(debug_partition_result result);

/**
 * Start erasing Debug Partition in the background
 *
 * \param callback Function to call on completion. */
extern void debug_partition_erase(DEBUG_PARTITION_ERASE_CB callback);

/**
 * Configure Debug Partition parameters
 *
 * \param key configuration key
 * \param value data for the key
 * \return Result code indicating success or error reason. */
extern debug_partition_result debug_partition_config(
        debug_partition_config_key key,
        uint32 value);

/**
 * Request Debug Partition info
 *
 * \param key information key
 * \param value location to return the requested data
 * \return Result code indicating success or error reason. */
extern debug_partition_result debug_partition_info(
        debug_partition_info_key key,
        uint32 *value);

/**
 * Store debug logs in Debug Partition.
 *
 * There is defined layout for storing each debug event.
 * Each debug event is stored sequentially.
 * Note: In Phase 1 only Apps P1 Panic logs support is going to be provided.
 *
 * \param deathbed_confession panic code
 * \param diatribe diatribe value for the panic code */
extern void debug_partition_handle_panic(panicid deathbed_confession,
                                         DIATRIBE_TYPE diatribe);

/**
 * Read block of data from debug partition into external buffer
 *
 * Debug partition stream use it to fetch data into an MMU buffer.
 * This function does not read beyond the free space offset in the Debug
 * Partition.
 *
 * \param offset offset inside debug partition.
 * \param len maximum size of data to read.
 * \param data location to store data read from the flash.
 * \return size of data actually copied. */
extern uint16 debug_partition_flash_read(uint32 offset, uint16 len, uint8 *data);

/**
 * Return size of data on Debug Partition
 *
 * \return size in bytes of data on the Debug Partition. */
extern uint32 debug_partition_data_size(void);

/**
 * Check whether Debug Partition is ready
 *
 * \return TRUE is ready or otherwise FALSE. */
extern bool debug_partition_ready(void);

#if INSTALL_DEBUG_PARTITION
#define is_debug_partition_supported() TRUE
#else
#define is_debug_partition_supported() FALSE
#endif

#endif /* DEBUG_PARTITION_H */

