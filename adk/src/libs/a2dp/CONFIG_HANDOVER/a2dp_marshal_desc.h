/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    a2dp_marshal_desc.h

DESCRIPTION
    Creates tables of marshal type descriptors for A2DP data types

NOTES
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER
*/


#ifndef _A2DP_MARSHAL_DESC_H_
#define _A2DP_MARSHAL_DESC_H_

#include "marshal_common_desc.h"
#include "types.h"
#include "app/marshal/marshal_if.h"
#include "a2dp_private.h"

/* This structure is marshalled, its components are unpacked and copied/moved
   to the appropriate location when unmarshalling */
typedef struct
{
    /* The remote connection state */
    remote_device *remote_conn;

    /* The data related to the remote connection */
    data_block_header *data_blocks;

} a2dp_marshal_data;

#define A2DP_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(sep_info) \
    ENTRY(connection_setup) \
    ENTRY(media_connection_active_bitfields) \
    ENTRY(media_connection_active) \
    ENTRY(media_connection) \
    ENTRY(media_channel_status) \
    ENTRY(media_channel) \
    ENTRY(signalling_connection_bitfields) \
    ENTRY(signalling_connection_active) \
    ENTRY(signalling_connection) \
    ENTRY(signalling_channel_status) \
    ENTRY(signalling_channel) \
    ENTRY(remote_device) \
    ENTRY(remote_device_bitfields) \
    ENTRY(data_block_header) \
    ENTRY(a2dp_marshal_data) \


/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    A2DP_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    A2DP_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION

extern const marshal_type_descriptor_t * const  mtd_a2dp[];

#endif /* _A2DP_MARSHAL_DESC_H_ */

