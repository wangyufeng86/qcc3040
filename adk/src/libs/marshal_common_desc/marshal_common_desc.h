/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    marshal_common_desc.h

DESCRIPTION
    Creates tables of marshal type descriptors for common data types

*/

#ifndef MARSHAL_COMMON_DESC_H_
#define MARSHAL_COMMON_DESC_H_

#include "types.h"
#include "app/marshal/marshal_if.h"


#define COMMON_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(uint8) \
    ENTRY(uint16) \
    ENTRY(uint32) \
    ENTRY(bdaddr) \
    ENTRY(typed_bdaddr) \
    ENTRY(TRANSPORT_T) \
    ENTRY(tp_bdaddr) \
    ENTRY(L2capSink)


/* Dummy type to describe the dynamic array of uint8_t */
typedef struct uint8_dyn_arr
{
    uint8 array[1];
} uint8_dyn_arr_t;


#define COMMON_DYN_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(uint8_dyn_arr_t)


/* Use xmacro to expand type table as declaration for marshal type descriptors */
#define EXPAND_AS_EXTERN_DECL(type) extern const marshal_type_descriptor_t mtd_##type;
COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_EXTERN_DECL)
#undef EXPAND_AS_EXTERN_DECL


/* Use xmacro to expand type table as declaration for marshal type descriptors for dynamic structures */
#define EXPAND_AS_EXTERN_DECL(type) extern const marshal_type_descriptor_dynamic_t mtd_##type;
COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_EXTERN_DECL)
#undef EXPAND_AS_EXTERN_DECL

/* Convert L2CAP CID to Sink */
void convertL2capCidToSink(Sink * sink);

#endif /* MARSHAL_COMMON_DESC_H_ */
