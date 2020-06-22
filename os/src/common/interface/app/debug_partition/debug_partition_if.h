    /* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
/****************************************************************************
FILE
    debug_partition_if.h

CONTAINS
    Definitions for Debug Partition.

DESCRIPTION
    This file is seen by the firmware and VM applications, and
    contains things that are common between them.
*/

#ifndef DEBUG_PARTITION_IF_H
#define DEBUG_PARTITION_IF_H

/** Debug Partition Configuration Keys */
typedef enum
{
    /** Clear configuration keys.
     *
     * Clear all previously configured keys, meaning nothing is dumped. */
    DP_CONFIG_CLEAR_ALL = 0,

    /** Dump minimal set of Apps P1 registers.
     *
     * The following Apps P1 registers are dumped:
     * REGFILE_(PC, RMAC2-RMAC0, RMAC24, R0-R10, RLINK)
     * STACK_START_ADDR, STACK_END_ADDR, STACK_POINTER, FRAME_POINTER,
     * INT_SAVE_INFO */
    DP_CONFIG_DUMP_P1_BASIC_REGS = 0x10,

    /** Dump Apps P1 application stack.
     *
     * Location of the stack is determined by looking into Apps P1 registers:
     * [STACK_START_ADDR:STACK_END_ADDR]. */
    DP_CONFIG_DUMP_P1_STACK = 0x20,

    /** Dump Apps P1 Hydra log data
     *
     * Dump block of Apps P1 RAM with Hydra log starting at offset 0x10004.
     * Block of data consists of the log buffer followed by 4 bytes of log
     * position. Application can either provide log buffer size in the
     * upper 2 octets of the trap argument or set them to "0" to use the
     * default size:
     *
     * // custom log buffer size:
     * DebugPartitionConfig(DP_CONFIG_DUMP_P1_HYDRA_LOG,
     *                      (custom_log_size << 16) | 1);
     * // default log buffer size:
     * DebugPartitionConfig(DP_CONFIG_DUMP_P1_HYDRA_LOG, 1);
     *
     * Default size is platform dependent. If application changes default
     * log buffer size then the new size has to be provided in the trap
     * argument. */
    DP_CONFIG_DUMP_P1_HYDRA_LOG = 0x30,

    /** Dump custom range of Apps P1 registers.
     *
     * The range is defined relative to 0xFFFF8000 in the Apps P1 memory space.
     * Start offset and length are encoded in the 32-bit parameter value as follows:
     * (((start & 0xffff) << 16) | (length_bytes & 0xffff)).
     * Application can use supplied macro DEBUG_PARTITION_PACK(offset, size).
     *
     * For example to dump three consecutive registers
     * STACK_START_ADDR(0xffff8018), STACK_END_ADDR(0xffff801c) and
     * STACK_POINTER(0xffff8020), parameter value has to be:
     * DEBUG_PARTITION_PACK(0x18, 0xc) == 0x0018000C.
     *
     * An arbitrary number of custom ranges can be configured by calling
     * the trap multiple times. */
    DP_CONFIG_DUMP_P1_REG_RANGE = 0x40,

    /** Dump custom range of Apps P1 RAM addresses.
     *
     * The range is defined relative to 0x10000 which is the start of
     * Apps P1 RAM window. Start offset and length are encoded in
     * the 32-bit parameter value as follows:
     * (((start & 0xffff) << 16) | (length_bytes & 0xffff)).
     * Application can use supplied macro DEBUG_PARTITION_PACK(offset, size).
     *
     * For example to dump 0x400 bytes of Apps P1 Hydra log that starts
     * at 0x10004 and the following 4 bytes of log position, parameter
     * value has to be: DEBUG_PARTITION_PACK(0x4, 0x404) == 0x00040404.
     *
     * An arbitrary number of custom ranges can be configured by calling
     * the trap multiple times. */
    DP_CONFIG_DUMP_P1_RAM_RANGE = 0x50,

    /** Maximum value for the config key */
    DP_CONFIG_MAX = 0xffff
} debug_partition_config_key;

/** Result codes for Debug Partition traps. */
typedef enum {
    /** No error, operation completed as expected. */
    DP_SUCCESS,
    /** Debug Partition does not present in the image header. */
    DP_NOT_CONFIGURED,
    /** Operation can't be completed because resource is busy. For example
     * erase can't be started if there is an open Debug Partition
     * source stream. */
    DP_BUSY,
    /** Configuration/info key not supported by this firmware. */
    DP_KEY_NOT_SUPPORTED,
    /** Configuration key parameter is either outside the allowed range
     * or not supported. */
    DP_INVALID_PARAMETER,
    /** Debug Partition does not have enough free space for a dump with
     * this configuration. */
    DP_NOT_ENOUGH_SPACE
} debug_partition_result;

/** Macro to pack offset and size into the uint32 argument. */
#define DEBUG_PARTITION_PACK(offset, size) (uint32)(((offset) << 16) | \
                                             ((size) & 0xffff))

/** Macro to extract size from uint32 argument. */
#define DEBUG_PARTITION_GET_SIZE(value) (uint16)(value)
/** Macro to extract offset from uint32 argument. */
#define DEBUG_PARTITION_GET_OFFSET(value) (uint16)((value) >> 16)

/** Debug Partition Information Keys */
typedef enum
{
    /** Debug Partition size.
     *
     * Returns size of Debug Partition in bytes. This is different from
     * the size of data on the partition that can be queried with the
     * DP_INFO_DATA_SIZE key. */
    DP_INFO_PARTITION_SIZE = 0,

    /** Debug Partition data size.
     *
     * Returns size of data on the Debug Partition in bytes. */
    DP_INFO_DATA_SIZE = 1,

    /** Maximum value for the information key */
    DP_INFO_MAX = 0xffff
} debug_partition_info_key;

#endif /* DEBUG_PARTITION_IF_H */
