/*!
    \copyright Copyright (c) 2019 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file 
    \brief Defines the marshal type table used by the Device Manager module.
*/
#include <marshal_common.h>
#include <app/marshal/marshal_if.h>
#include <bt_device_marshal_typedef.h>
#include <bt_device_marshal_table.h>

/*! X-Macro generate logical input switch marshal type descriptor set that can be passed to a (un)marshaller
     *  to initialise it.
     *  */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *)&marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const bt_device_marshal_type_descriptors[NUMBER_OF_MARSHAL_OBJECT_TYPES] = {
    MARSHAL_COMMON_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
