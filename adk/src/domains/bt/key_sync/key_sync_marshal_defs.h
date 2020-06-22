/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Definition of messages that can be sent between key sync components.
*/

#ifndef KEY_SYNC_MARSHAL_DEFS_H
#define KEY_SYNC_MARSHAL_DEFS_H

#include <marshal_common.h>

#include <connection.h>

#include <marshal.h>

typedef struct
{
    unsigned trusted:1;
} key_sync_req_bits_t;

typedef struct key_sync_req
{
    bdaddr              bd_addr;
    key_sync_req_bits_t bits;
    cl_sm_link_key_type link_key_type;
    uint8               size_link_key;
    uint8               link_key[1];
} key_sync_req_t;
#define MARSHAL_TYPE_cl_sm_link_key_type MARSHAL_TYPE_uint8

typedef struct key_sync_cfm
{
    bdaddr bd_addr;
    uint32 synced;
} key_sync_cfm_t;

typedef struct key_sync_paired_device_req
{
    bdaddr bd_addr;
    uint8  size_data;
    uint8  data[1];
} key_sync_paired_device_req_t;

/* Create base list of marshal types the key sync will use. */
#define MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(key_sync_req_t) \
    ENTRY(key_sync_cfm_t) \
    ENTRY(key_sync_req_bits_t) \
    ENTRY(key_sync_paired_device_req_t)

/* X-Macro generate enumeration of all marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum MARSHAL_TYPES
{
    /* common types must be placed at the start of the enum */
    DUMMY = NUMBER_OF_COMMON_MARSHAL_OBJECT_TYPES-1,
    /* now expand the marshal types specific to this component. */
    MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    NUMBER_OF_MARSHAL_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION

/* Make the array of all message marshal descriptors available. */
extern const marshal_type_descriptor_t * const key_sync_marshal_type_descriptors[];

#endif /* KEY_SYNC_MARSHAL_DEFS_H */
