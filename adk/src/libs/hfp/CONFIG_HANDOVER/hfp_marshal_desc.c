/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    hfp_marshal_desc.c

DESCRIPTION
    Creates marshal type descriptors for HFP data types

NOTES
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER
*/

#include "hfp_marshal_desc.h"
#include "hfp_private.h"
#include "hfp_init.h"
#include "hfp_service_manager.h"

#include <panic.h>


#define RFC_INVALID_SERV_CHANNEL   0x00


/* hfp_link_data contains an alias of hfp_service_data which is not be marshalled.
 * Instead mapping of hfp_link_data to its hfp_service_data needs to be marshalled.
 * This is done by treating hfp_service_data alias member in hfp_link_data as
 * a separate type by itself and creating converter based marshal type descriptors
 * for it. It marshal type descriptors convert it into server_channel, which is
 * consistent across earbuds, for marshalling. While unmarshalling, the server_channel
 * is used to find appropriate hfp_service_data to relate to hfp_link_data. */
typedef hfp_service_data *hfp_service_data_alias;


static uint32 dynArrElemsHfpIndicatorIndexes(const void *parent,
                                             const marshal_member_descriptor_t *member_descriptor,
                                             uint32 array_element);
static void *convertChannelToService(void *dest, const void *src, size_t n);
static void *convertServiceToChannel(void *dest, const void *src, size_t n);


static const marshal_type_descriptor_t mtd_hfp_task_data_bitfields =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(hfp_task_data_bitfields);


static const marshal_type_descriptor_t mtd_hfp_link_data_bitfields =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(hfp_link_data_bitfields);


static const marshal_type_descriptor_t mtd_hfp_csr_features =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(hfp_csr_features);


static const marshal_type_descriptor_t mtd_hfp_indicators =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(hfp_indicators);


static const marshal_member_descriptor_t mmd_hfp_indicator_indexes[] =
{
    MAKE_MARSHAL_MEMBER(hfp_indicator_indexes, hfp_indicators, indicator_idxs),
    MAKE_MARSHAL_MEMBER(hfp_indicator_indexes, uint16, num_extra_indicator_idxs),
    MAKE_MARSHAL_MEMBER_POINTER(hfp_indicator_indexes, uint8_dyn_arr_t, extra_indicator_idxs),
    MAKE_MARSHAL_MEMBER(hfp_indicator_indexes, uint16, num_indicators),
};

static const marshal_type_descriptor_dynamic_t mtd_hfp_indicator_indexes =
    MAKE_MARSHAL_TYPE_DEFINITION_HAS_PTR_TO_DYNAMIC_ARRAY(hfp_indicator_indexes,
                                                          mmd_hfp_indicator_indexes,
                                                          dynArrElemsHfpIndicatorIndexes);


static const marshal_type_descriptor_t mtd_hfp_hf_indicators =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(hfp_hf_indicators);


static const marshal_type_descriptor_t mtd_hfp_audio_params =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(hfp_audio_params);


static const marshal_type_descriptor_t mtd_sync_pkt_type =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(sync_pkt_type);


/* Read/Write callbacks for marshalling hfp_service_data_alias */
static const marshal_custom_copy_cbs read_write_cb_hfp_service_data_alias =
{
    convertServiceToChannel,
    convertChannelToService
};

const marshal_type_descriptor_t mtd_hfp_service_data_alias =
{
    {.custom_copy_cbs = &read_write_cb_hfp_service_data_alias}, /* Callbacks */
    sizeof(hfp_service_data_alias),                             /* Although server channel (uint8) would be marshalled,
                                                                   allocation at the unmarshaller needs to be
                                                                   able to hold pointer to hfp_service_data */
    0,                                                          /* No members */
    FALSE,                                                      /* Not a union */
    FALSE,                                                      /* Not a variable length variable */
    0                                                           /* Not a dynamic type */
};


static const marshal_member_descriptor_t mmd_hfp_link_data[] =
{
 /* MAKE_MARSHAL_MEMBER(hfp_link_data, hfp_link_identifier, identifier), To marshalled with separately before this structure */
    MAKE_MARSHAL_MEMBER(hfp_link_data, hfp_service_data_alias, service),
    MAKE_MARSHAL_MEMBER(hfp_link_data, hfp_link_data_bitfields, bitfields),
    MAKE_MARSHAL_MEMBER(hfp_link_data, hfp_csr_features, ag_csr_features),
    MAKE_MARSHAL_MEMBER(hfp_link_data, uint16, ag_supported_features),
    MAKE_MARSHAL_MEMBER(hfp_link_data, hfp_indicator_indexes, ag_supported_indicators),
    MAKE_MARSHAL_MEMBER(hfp_link_data, hfp_hf_indicators, ag_supported_hf_indicators),
 /* MAKE_MARSHAL_MEMBER(hfp_link_data, ScoSink, audio_sink), To be set manually by a higher layer */
    MAKE_MARSHAL_MEMBER(hfp_link_data, hfp_audio_params, audio_params),
    MAKE_MARSHAL_MEMBER(hfp_link_data, sync_pkt_type, audio_packet_type),
    MAKE_MARSHAL_MEMBER(hfp_link_data, sync_pkt_type, audio_packet_type_to_try),
    MAKE_MARSHAL_MEMBER(hfp_link_data, uint16, ag_codec_modes),
    MAKE_MARSHAL_MEMBER(hfp_link_data, uint16, qce_codec_mode_id),    
};

static const marshal_type_descriptor_t mtd_hfp_link_data =
    MAKE_MARSHAL_TYPE_DEFINITION(hfp_link_data,
                                 mmd_hfp_link_data);


static const marshal_member_descriptor_t mmd_hfp_marshalled_obj[] =
{
    MAKE_MARSHAL_MEMBER(hfp_marshalled_obj, uint8, channel),
    MAKE_MARSHAL_MEMBER_POINTER(hfp_marshalled_obj, hfp_link_data, link),
    MAKE_MARSHAL_MEMBER(hfp_marshalled_obj, hfp_task_data_bitfields, bitfields),
};

static const marshal_type_descriptor_t mtd_hfp_marshalled_obj =
    MAKE_MARSHAL_TYPE_DEFINITION(hfp_marshalled_obj,
                                 mmd_hfp_marshalled_obj);

/* Use xmacro to expand type table as array of type descriptors */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *) &mtd_##type,
const marshal_type_descriptor_t * const  mtd_hfp[] =
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    HFP_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION



static uint32 dynArrElemsHfpIndicatorIndexes(const void *parent,
                                             const marshal_member_descriptor_t *member_descriptor,
                                             uint32 array_element)
{
    const hfp_indicator_indexes *obj = parent;

    PanicFalse(obj && member_descriptor);
    PanicFalse(array_element == 0);
    PanicFalse(member_descriptor->offset == offsetof(hfp_indicator_indexes, extra_indicator_idxs));

    return (obj->num_extra_indicator_idxs * sizeof(*obj->extra_indicator_idxs));
}

/* Converts server channel to hfp_service_channel pointer */
static void *convertChannelToService(void *dest, const void *src, size_t n)
{
    hfp_service_data_alias *p_service = dest;
    const uint8 *channel = src;

    PanicFalse(n == sizeof(*p_service));

    if (*channel != RFC_INVALID_SERV_CHANNEL)
    {
        *p_service = hfpGetServiceFromChannel(*channel);
    }
    else
    {
        *p_service = NULL;
    }

    return dest;
}

/* Converts hfp_service_channel pointer to server channel */
static void *convertServiceToChannel(void *dest, const void *src, size_t n)
{
    const hfp_service_data_alias *p_service = src;
    uint8 *channel = dest;

    PanicFalse(n == sizeof(*p_service));

    if (*p_service)
    {
        *channel = (*p_service)->bitfields.rfc_server_channel;
    }
    else
    {
        *channel = RFC_INVALID_SERV_CHANNEL;
    }

    return dest;
}

