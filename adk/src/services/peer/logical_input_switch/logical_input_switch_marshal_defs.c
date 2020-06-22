/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Marshal type definitions for logical input switch.
*/

/* component includes */
#include "logical_input_switch_marshal_defs.h"

/* framework includes */
#include <marshal_common.h>

/* system includes */
#include <marshal.h>

/*! logical_input_ind message marshalling type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_logical_input_ind_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(logical_input_ind_t);

/*! X-Macro generate logical input switch marshal type descriptor set that can be passed to a (un)marshaller
 *  to initialise it.
 *  */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *)&marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const logical_input_switch_marshal_type_descriptors[NUMBER_OF_MARSHAL_OBJECT_TYPES] = {
    MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
