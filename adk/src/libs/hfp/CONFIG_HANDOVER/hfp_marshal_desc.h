/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    hfp_marshal_desc.h

DESCRIPTION
    Creates tables of marshal type descriptors for HFP data types
    
NOTES
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER
*/


#ifndef _HFP_MARSHAL_DESC_H_
#define _HFP_MARSHAL_DESC_H_

#include "marshal_common_desc.h"
#include "types.h"
#include "app/marshal/marshal_if.h"
#include "hfp_private.h"

/* This single object is marshalled by HFP */
typedef struct
{
    uint8 channel;
    hfp_link_data *link;
    hfp_task_data_bitfields bitfields;
} hfp_marshalled_obj;


#define HFP_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(hfp_service_data_alias) \
    ENTRY(hfp_task_data_bitfields) \
    ENTRY(hfp_link_data_bitfields) \
    ENTRY(hfp_csr_features) \
    ENTRY(hfp_indicators) \
    ENTRY(hfp_indicator_indexes) \
    ENTRY(hfp_hf_indicators) \
    ENTRY(hfp_audio_params) \
    ENTRY(sync_pkt_type) \
    ENTRY(hfp_link_data) \
    ENTRY(hfp_marshalled_obj)


/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    HFP_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    HFP_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION

extern const marshal_type_descriptor_t * const  mtd_hfp[];

#endif /* _HFP_MARSHAL_DESC_H_ */

