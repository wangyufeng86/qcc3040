    /* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
/****************************************************************************
FILE
    debug_partition_data_if.h

CONTAINS
    Describes data on the Debug Partition.

DESCRIPTION
    This file is seen by the firmware and VM applications, and
    contains things that are common between them.
*/

#ifndef DEBUG_PARTITION_DATA_IF_H
#define DEBUG_PARTITION_DATA_IF_H

/* layout versioning */
#define EVENT_LAYOUT_VERSION 1
#define LOG_LAYOUT_VERSION 1

#define FW_APPSP1 1

/* debug event header */
typedef struct debug_event_header_t
{
    unsigned int layout_ver_no:8;
    unsigned int trigger_type:8;
    unsigned int chip_version:16;
    unsigned int event_log_length;
} debug_event_header_t;

/* susbys log header */
typedef struct subsys_log_header_t
{
    unsigned int subsys_id:16;
    unsigned int log_ver_no:16;
    unsigned int build_id;
    unsigned int subsys_log_length;
} subsys_log_header_t;

/* log format */
typedef struct subsys_log_t
{
    unsigned int key;
    unsigned int log_length;
} subsys_log_t;

/**
 * Type definition for debug event trigger type
 */
typedef enum debug_event_trigger_type
{
    DEBUG_EVENT_PANIC /**< Logging event trigger is Panic */
} debug_event_trigger_type;

#endif /* DEBUG_PARTITION_DATA_IF_H */
