/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Definition of marshalled messages used by Key Sync.
*/

#include "key_sync.h"
#include "key_sync_marshal_defs.h"

#include <marshal_common.h>

#include <connection.h>

#include <marshal.h>

const marshal_type_descriptor_t marshal_type_descriptor_key_sync_req_bits_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(sizeof(key_sync_req_bits_t));

static uint32 key_sync_req_key_size(const void *parent,
                                    const marshal_member_descriptor_t *member_descriptor,
                                    uint32 array_element)
{
    const key_sync_req_t* req = parent;
    UNUSED(member_descriptor);
    UNUSED(array_element);
    return req->size_link_key;
}

const marshal_member_descriptor_t key_sync_req_member_descriptors[] = 
{
    MAKE_MARSHAL_MEMBER(key_sync_req_t, bdaddr, bd_addr),
    MAKE_MARSHAL_MEMBER(key_sync_req_t, key_sync_req_bits_t, bits),
    MAKE_MARSHAL_MEMBER(key_sync_req_t, cl_sm_link_key_type, link_key_type),
    MAKE_MARSHAL_MEMBER(key_sync_req_t, uint8, size_link_key),
    MAKE_MARSHAL_MEMBER_ARRAY(key_sync_req_t, uint8, link_key, 1),
};

const marshal_type_descriptor_dynamic_t marshal_type_descriptor_key_sync_req_t =
    MAKE_MARSHAL_TYPE_DEFINITION_HAS_DYNAMIC_ARRAY(key_sync_req_t,
                                                   key_sync_req_member_descriptors,
                                                   key_sync_req_key_size);

/*----------------------------------------------------------------------------*/

const marshal_member_descriptor_t key_sync_cfm_member_descriptors[] =
{
    MAKE_MARSHAL_MEMBER(key_sync_cfm_t, bdaddr, bd_addr),
    MAKE_MARSHAL_MEMBER(key_sync_cfm_t, uint32, synced),
};

const marshal_type_descriptor_t marshal_type_descriptor_key_sync_cfm_t =
    MAKE_MARSHAL_TYPE_DEFINITION(key_sync_cfm_t, key_sync_cfm_member_descriptors);

/*----------------------------------------------------------------------------*/

static uint32 key_sync_paired_device_req_data_size(const void *parent,
                                                   const marshal_member_descriptor_t *member_descriptor,
                                                   uint32 array_element)
{
    const key_sync_paired_device_req_t* req = parent;
    UNUSED(member_descriptor);
    UNUSED(array_element);
    return req->size_data;
}

const marshal_member_descriptor_t key_sync_paired_device_req_member_descriptors[] = 
{
    MAKE_MARSHAL_MEMBER(key_sync_paired_device_req_t, bdaddr, bd_addr),
    MAKE_MARSHAL_MEMBER(key_sync_paired_device_req_t, uint8, size_data),
    MAKE_MARSHAL_MEMBER_ARRAY(key_sync_paired_device_req_t, uint8, data, 1),
};

const marshal_type_descriptor_dynamic_t marshal_type_descriptor_key_sync_paired_device_req_t =
    MAKE_MARSHAL_TYPE_DEFINITION_HAS_DYNAMIC_ARRAY(key_sync_paired_device_req_t,
                                                   key_sync_paired_device_req_member_descriptors,
                                                   key_sync_paired_device_req_data_size);

/*----------------------------------------------------------------------------*/

/*! X-Macro generate key sync marshal type descriptor set that can be passed to a (un)marshaller
 *  to initialise it.
 *  */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *)&marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const key_sync_marshal_type_descriptors[NUMBER_OF_MARSHAL_OBJECT_TYPES] = {
    MARSHAL_COMMON_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION

