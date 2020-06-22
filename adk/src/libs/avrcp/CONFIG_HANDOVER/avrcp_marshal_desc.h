/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    avrcp_marshal_desc.h

DESCRIPTION
    Creates tables of marshal type descriptors for AVRCP data types

NOTES
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER
*/


#ifndef _AVRCP_MARSHAL_DESC_H_
#define _AVRCP_MARSHAL_DESC_H_

#include "marshal_common_desc.h"
#include "types.h"
#include "app/marshal/marshal_if.h"


#define AVRCP_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(AvbpBitfields) \
    ENTRY(AVBP) \
    ENTRY(AvrcpBitfields) \
    ENTRY(AVRCP) \
    ENTRY(AVRCP_AVBP_INIT)


/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    AVRCP_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    AVRCP_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION

extern const marshal_type_descriptor_t * const  mtd_avrcp[];

#endif /* _AVRCP_MARSHAL_DESC_H_ */

