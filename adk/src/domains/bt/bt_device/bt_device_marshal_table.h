/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Declares the marshal type table used by the Device Manager module.
*/

#ifndef BT_DEVICE_MARSHAL_TABLE_H
#define BT_DEVICE_MARSHAL_TABLE_H

#include "bt_device_typedef.h"

/* framework includes */
#include <marshal_common.h>

/* system includes */
#include <marshal.h>

/* Create base list of marshal types the Earbud SM will use. */
#define MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(bt_device_pdd_t)

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
extern const marshal_type_descriptor_t * const bt_device_marshal_type_descriptors[];

#endif /* BT_DEVICE_MARSHAL_TABLE_H */
