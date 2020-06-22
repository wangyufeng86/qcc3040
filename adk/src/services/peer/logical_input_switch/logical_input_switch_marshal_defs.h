/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Definition of Logical Input messages that can be sent between Earbud application entities.
*/

#ifndef LOGICAL_INPUT_SWITCH_MARSHAL_DEFS_H
#define LOGICAL_INPUT_SWITCH_MARSHAL_DEFS_H

/* framework includes */
#include <marshal_common.h>
#include <ui.h>

/* system includes */
#include <marshal.h>

typedef struct logical_input_ind
{
    uint16 logical_input;
    ui_input_t passthrough_ui_input;
} logical_input_ind_t;

/* Create base list of marshal types the Earbud SM will use. */
#define MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(logical_input_ind_t) 

/* X-Macro generate enumeration of all marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum MARSHAL_TYPES
{
    /* expand the marshal types specific to this component. */
    MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    NUMBER_OF_MARSHAL_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION

/* Make the array of all message marshal descriptors available. */
extern const marshal_type_descriptor_t * const logical_input_switch_marshal_type_descriptors[];

#endif /* LOGICAL_INPUT_SWITCH_MARSHAL_DEFS_H */
