/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       battery_monitor_marshal_defs.c
\brief      Marshal definitions of battery monitoring messages.
*/

/* local includes */
#include "battery_monitor.h"

/* system includes */
#include <marshal.h>
#include <marshal_common.h>

const marshal_type_descriptor_t marshal_type_descriptor_MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(sizeof(MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE));

const marshal_member_descriptor_t MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T_member_descriptors[] =
{
    MAKE_MARSHAL_MEMBER(MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T, battery_level_state, state),
};
const marshal_type_descriptor_t marshal_type_descriptor_MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T =
    MAKE_MARSHAL_TYPE_DEFINITION(MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T, MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T_member_descriptors);


