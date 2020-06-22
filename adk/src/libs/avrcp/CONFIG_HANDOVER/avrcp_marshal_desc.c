/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    avrcp_marshal_desc.c

DESCRIPTION
    Creates marshal type descriptors for AVRCP data types

NOTES
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER
*/

#include "avrcp_marshal_desc.h"
#include "avrcp_private.h"
#include "avrcp_init.h"
#include <panic.h>

static uint32 dynArrElemsAvrcp(const void *parent,
                               const marshal_member_descriptor_t *member_descriptor,
                               uint32 array_element);

static const marshal_type_descriptor_t mtd_AvbpBitfields =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(AvbpBitfields);


static const marshal_member_descriptor_t mmd_AVBP[] =
{
 /* MAKE_MARSHAL_MEMBER(AVBP, TaskData, task), Constant, not required to be marshalled */
 /* MAKE_MARSHAL_MEMBER_POINTER(AVBP, AVRCP, avrcp_task), Circular reference, to be manually stitched */
    MAKE_MARSHAL_MEMBER(AVBP, L2capSink, avbp_sink), /* Gets converted to CID for marshalling */
 /* MAKE_MARSHAL_MEMBER(AVBP, uint16, avbp_sink_data), Should be invalid when marshalling */
    MAKE_MARSHAL_MEMBER(AVBP, uint16, avbp_mtu),
 /* MAKE_MARSHAL_MEMBER(AVBP, uint16, blocking_cmd), Should be invalid when marshalling */
    MAKE_MARSHAL_MEMBER(AVBP, AvbpBitfields, bitfields),
};

static const marshal_type_descriptor_t mtd_AVBP =
    MAKE_MARSHAL_TYPE_DEFINITION(AVBP, mmd_AVBP);


static const marshal_type_descriptor_t mtd_AvrcpBitfields =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(AvrcpBitfields);


static const marshal_member_descriptor_t mmd_AVRCP[] =
{
 /* MAKE_MARSHAL_MEMBER(AVRCP, TaskData, task), Constant, not required to be marshalled */
 /* MAKE_MARSHAL_MEMBER(AVRCP, Task, clientTask), Application to set it separately */
 /* MAKE_MARSHAL_MEMBER(AVRCP, Task, avbp_task), Stitched manually after unmarshalling */
 /* MAKE_MARSHAL_MEMBER(AVRCP, AvrcpCleanUpTask, dataFreeTask), Constant, not required to be marshalled */
    MAKE_MARSHAL_MEMBER(AVRCP, L2capSink, sink), /* Gets converted to CID for marshalling */
 /* MAKE_MARSHAL_MEMBER(AVRCP, Source, continuation_data), Should be invalid when marshalling */
 /* MAKE_MARSHAL_MEMBER(AVRCP, Source, data_app_ind), Should be invalid when marshalling */
 /* MAKE_MARSHAL_MEMBER(AVRCP, uint16, continuation_pdu), Should be invalid when marshalling */
    MAKE_MARSHAL_MEMBER(AVRCP, uint16, l2cap_mtu),
    MAKE_MARSHAL_MEMBER(AVRCP, uint16, tg_registered_events),
    MAKE_MARSHAL_MEMBER_ARRAY(AVRCP, uint8, tg_notify_transaction_label, AVRCP_MAX_NUM_EVENTS),
    MAKE_MARSHAL_MEMBER(AVRCP, uint16, ct_registered_events),
    MAKE_MARSHAL_MEMBER_ARRAY(AVRCP, uint8, ct_notify_transaction_label, AVRCP_MAX_NUM_EVENTS),
    MAKE_MARSHAL_MEMBER(AVRCP, uint16, av_msg_len), /* Size needs to be marshalled before pointer */
    MAKE_MARSHAL_MEMBER_POINTER(AVRCP, uint8_dyn_arr_t, av_msg), /* Invalid when marshalling */
 /* MAKE_MARSHAL_MEMBER(AVRCP, uint16, sdp_search_mode), Invalid when marshalling */
 /* MAKE_MARSHAL_MEMBER(AVRCP, uint16, pending), Invalid when marshalling */
    MAKE_MARSHAL_MEMBER(AVRCP, AvrcpBitfields, bitfields),
    MAKE_MARSHAL_MEMBER(AVRCP, uint16, av_max_data_size),
 /* MAKE_MARSHAL_MEMBER(AVRCP, uint8, avctp_packets_remaining), Invalid during marshalling */
 /* MAKE_MARSHAL_MEMBER(AVRCP, bdaddr, bd_addr), known by both devices, so marshalling not required */
};

static const marshal_type_descriptor_dynamic_t mtd_AVRCP =
    MAKE_MARSHAL_TYPE_DEFINITION_HAS_PTR_TO_DYNAMIC_ARRAY(AVRCP,
                                                          mmd_AVRCP,
                                                          dynArrElemsAvrcp);


static const marshal_member_descriptor_t mmd_AVRCP_AVBP_INIT[] =
{
    MAKE_MARSHAL_MEMBER(AVRCP_AVBP_INIT, AVRCP, avrcp),
    MAKE_MARSHAL_MEMBER(AVRCP_AVBP_INIT, AVBP, avbp),
};

static const marshal_type_descriptor_t mtd_AVRCP_AVBP_INIT =
    MAKE_MARSHAL_TYPE_DEFINITION(AVRCP_AVBP_INIT,
                                 mmd_AVRCP_AVBP_INIT);

/* Use xmacro to expand type table as array of type descriptors */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *) &mtd_##type,
const marshal_type_descriptor_t * const  mtd_avrcp[] =
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    AVRCP_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION


static uint32 dynArrElemsAvrcp(const void *parent,
                               const marshal_member_descriptor_t *member_descriptor,
                               uint32 array_element)
{
    const AVRCP *obj = parent;

    PanicFalse(obj && member_descriptor);
    PanicFalse(array_element == 0);
    PanicFalse(member_descriptor->offset == offsetof(AVRCP, av_msg));

    return obj->av_msg_len;
}
