/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_sm_handover.h
\brief      Earbud application SM handover support.

*/

#include <marshal_common.h>
#include <service_marshal_types.h>
#include "earbud_handover_marshal_typedef.h"

/* Add Earbud SM marshal type enum to top of the marshal types enum hierarchy */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    LAST_SERVICE_MARSHAL_TYPE = NUMBER_OF_SERVICE_MARSHAL_OBJECT_TYPES -1,
    EARBUD_HANDOVER_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    NUMBER_OF_EARBUD_APP_MARSHAL_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION
