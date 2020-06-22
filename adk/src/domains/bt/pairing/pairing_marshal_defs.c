/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       pairing_marshal_defs.c
\brief      Marshal type definitions for pairing component messages.
*/

/* local includes */
#include "pairing.h"

/* system includes */
#include <marshal.h>

const marshal_type_descriptor_t marshal_type_descriptor_PAIRING_ACTIVITY_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(sizeof(PAIRING_ACTIVITY_T));
