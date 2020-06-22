/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*   %%version */

#ifndef RA_PARTITION_IF_H
#define RA_PARTITION_IF_H

/** RA Partition handle. */
typedef uint8 raPartition_handle;

/** Partition information retrieved on successful RAPartitionOpen(). */
typedef struct _raPartition_info
{
    /** Relation between Partition-Block-Pages
     *  Partition contains Blocks and Block contains Pages
     *  All sizes in bytes. */
    uint32 page_size;
    uint32 partition_size;
    uint32 block_size;
} raPartition_info;

/** Result of operations on RA partition. */
typedef enum _raPartition_result
{
    /** Generic success. */
    RA_PARTITION_RESULT_SUCCESS,
    /** Generic failure. */
    RA_PARTITION_RESULT_FAIL,
    /** Error in operation parameters/arguments.
     *  e.g. invalid handle OR inappropriate offset to erase/write/read
     *  operation. */
    RA_PARTITION_RESULT_INVALID_PARAM,
    /** Error when open operation is already done. i.e. Partition in use.*/
    RA_PARTITION_RESULT_IN_USE
} raPartition_result;

#endif /* RA_PARTITION_IF_H */
