/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       av.c
\brief      Marshal type definitions for AV messages.
*/

/* local includes */
#include "av.h"

/* system includes */
#include <marshal.h>

const marshal_type_descriptor_t marshal_type_descriptor_AV_A2DP_CONNECTED_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(AV_A2DP_CONNECTED_IND_T);
const marshal_type_descriptor_t marshal_type_descriptor_AV_A2DP_DISCONNECTED_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(AV_A2DP_DISCONNECTED_IND_T);
const marshal_type_descriptor_t marshal_type_descriptor_AV_AVRCP_CONNECTED_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(AV_AVRCP_CONNECTED_IND_T);
const marshal_type_descriptor_t marshal_type_descriptor_AV_AVRCP_DISCONNECTED_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(AV_AVRCP_DISCONNECTED_IND_T);
const marshal_type_descriptor_t marshal_type_descriptor_AV_STREAMING_ACTIVE_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(AV_STREAMING_ACTIVE_IND_T);
const marshal_type_descriptor_t marshal_type_descriptor_AV_STREAMING_INACTIVE_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(AV_STREAMING_INACTIVE_IND_T);

