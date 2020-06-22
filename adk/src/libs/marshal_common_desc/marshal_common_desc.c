/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    common_marshal_desc.c

DESCRIPTION
    Defines marshal type descriptors for common data types

*/

#include <app/bluestack/l2cap_prim.h>
#include <bdaddr.h>
#include <sink.h>
#include <stream.h>
#include <panic.h>
#include <string.h>
#include "marshal_common_desc.h"


/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    COMMON_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION


static void *copyCid(void *dest, const void *src, size_t n);
static void *convertSinkToL2capCid(void *dest, const void *src, size_t n);


const marshal_type_descriptor_t mtd_uint8 = MAKE_MARSHAL_TYPE_DEFINITION_BASIC(uint8);
const marshal_type_descriptor_t mtd_uint16 = MAKE_MARSHAL_TYPE_DEFINITION_BASIC(uint16);
const marshal_type_descriptor_t mtd_uint32 = MAKE_MARSHAL_TYPE_DEFINITION_BASIC(uint32);

/* Member descriptors for the dynamic array */
static const marshal_member_descriptor_t mmd_uint8_dyn_arr_t[] =
{
    MAKE_MARSHAL_MEMBER_ARRAY(uint8_dyn_arr_t, uint8, array, 1),
};

/* Since uint8_dyn_arr_t doesn't know the length of its array (only its parent
   knows) a callback is not set. The parent's array_elements callback
   will be called instead. */
const marshal_type_descriptor_dynamic_t mtd_uint8_dyn_arr_t = MAKE_MARSHAL_TYPE_DEFINITION_HAS_DYNAMIC_ARRAY(uint8_dyn_arr_t,
                                                                                                             mmd_uint8_dyn_arr_t,
                                                                                                             NULL);

static const marshal_member_descriptor_t mmd_bdaddr[] =
{
    MAKE_MARSHAL_MEMBER(bdaddr, uint32, lap),
    MAKE_MARSHAL_MEMBER(bdaddr, uint8, uap),
    MAKE_MARSHAL_MEMBER(bdaddr, uint16, nap),
};

const marshal_type_descriptor_t mtd_bdaddr = MAKE_MARSHAL_TYPE_DEFINITION(bdaddr,
                                                                          mmd_bdaddr);

static const marshal_member_descriptor_t mmd_typed_bdaddr[] =
{
    MAKE_MARSHAL_MEMBER(typed_bdaddr, uint8, type),
    MAKE_MARSHAL_MEMBER(typed_bdaddr, bdaddr, addr),
};

const marshal_type_descriptor_t mtd_typed_bdaddr = MAKE_MARSHAL_TYPE_DEFINITION(typed_bdaddr,
                                                                                mmd_typed_bdaddr);

const marshal_type_descriptor_t mtd_TRANSPORT_T = MAKE_MARSHAL_TYPE_DEFINITION_BASIC(TRANSPORT_T);

static const marshal_member_descriptor_t mmd_tp_bdaddr[] =
{
    MAKE_MARSHAL_MEMBER(tp_bdaddr, typed_bdaddr, taddr),
    MAKE_MARSHAL_MEMBER(tp_bdaddr, TRANSPORT_T, transport),
};

const marshal_type_descriptor_t mtd_tp_bdaddr = MAKE_MARSHAL_TYPE_DEFINITION(tp_bdaddr,
                                                                             mmd_tp_bdaddr);

/* Read/Write callbacks for marshalling L2CAP Sink */
static const marshal_custom_copy_cbs read_write_cb_L2capSink =
{
    convertSinkToL2capCid,
    copyCid,
};

const marshal_type_descriptor_t mtd_L2capSink =
{
    {.custom_copy_cbs = &read_write_cb_L2capSink},          /* Callbacks */
    sizeof(Sink),                                           /* Although CID (uint16) would be marshalled,
                                                               allocation at the unmarshaller needs to be
                                                               able to hold Sink */
    0,                                                      /* No members */
    FALSE,                                                  /* Not a union */
    FALSE,                                                  /* Not a variable length variable */
    0                                                       /* Not a dynamic type */
};

/* Copies CID during unmarshalling. Till now P0 data is not unmarshalled, so
 * we cannot convert CID to Sink. This will be done during commit
 */
static void * copyCid(void *dest, const void *src, size_t n)
{
    PanicFalse(n = sizeof(Sink));
    return memcpy(dest, src, sizeof(uint16));
}

/* Converts L2CAP Sink to CID */
static void *convertSinkToL2capCid(void *dest, const void *src, size_t n)
{
    const Sink *sink = src;
    uint16 cid = L2CA_CID_INVALID;

    PanicFalse(n == sizeof(*sink));

    if (SinkIsValid(*sink))
    {
        cid = SinkGetL2capCid(*sink);
    }

    return memcpy(dest, &cid, sizeof(cid));
}

/* Convert L2CAP CID to Sink */
void convertL2capCidToSink(Sink * sink)
{
    uint16 cid;
    memcpy(&cid, sink, sizeof(uint16));
    if (cid != L2CA_CID_INVALID)
    {
        *sink = StreamL2capSink(cid);
    }
}
