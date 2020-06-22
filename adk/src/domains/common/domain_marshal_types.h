/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       domain_marshal_types.h
\brief      Types used by all handover application and all its clients.
*/
#ifndef DOMAIN_MARSHAL_TYPES_H
#define DOMAIN_MARSHAL_TYPES_H

#include "av_marshal_typedef.h"
#include "avrcp_marshal_typedef.h"
#include "a2dp_marshal_typedef.h"
#include "connection_manager_list_marshal_typedef.h"
#include "hfp_profile_marshal_typedef.h"
#include "bt_device_marshal_typedef.h"
#include "bt_device_handover_marshal_typedef.h"
#include "marshal_common.h"
#include <hydra_macros.h>

#define MARSHAL_TYPE_avrcp_play_status MARSHAL_TYPE_uint8
#define MARSHAL_TYPE_hfp_profile MARSHAL_TYPE_uint16
#define MARSHAL_TYPE_hfp_call_state MARSHAL_TYPE_uint8


#ifndef HOSTED_TEST_ENVIRONMENT
COMPILE_TIME_ASSERT(sizeof(avrcp_play_status) == sizeof(uint8), marshal_type_avrcp_play_status_invalid);
COMPILE_TIME_ASSERT(sizeof(hfp_profile) == sizeof(uint16), marshal_type_hfp_profile_invalid);
COMPILE_TIME_ASSERT(sizeof(hfp_call_state) == sizeof(uint8), MARSHAL_TYPE_hfp_call_state_invalid);
#endif

/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    LAST_COMMON_MARSHAL_TYPE = NUMBER_OF_COMMON_MARSHAL_OBJECT_TYPES -1, /* Subtracting 1 to keep the marshal types contiguous */
    AV_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    A2DP_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    AVRCP_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    CONNECTION_MANAGER_LIST_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    HFP_PROFILE_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    BT_DEVICE_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    BT_DEVICE_HANDOVER_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    NUMBER_OF_DOMAIN_MARSHAL_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION


#endif /* DOMAIN_MARSHAL_TYPES_H */
