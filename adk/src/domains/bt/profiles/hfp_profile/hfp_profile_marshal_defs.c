/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       hfp_profile_marshal_defs.c
\brief      Marshal type definitions for HFP component messages.
*/

/* local includes */
#include "hfp_profile.h"

/* system includes */
#include <marshal.h>

const marshal_type_descriptor_t marshal_type_descriptor_APP_HFP_CONNECTED_IND_T = 
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(APP_HFP_CONNECTED_IND_T);
const marshal_type_descriptor_t marshal_type_descriptor_APP_HFP_DISCONNECTED_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(APP_HFP_DISCONNECTED_IND_T);
const marshal_type_descriptor_t marshal_type_descriptor_APP_HFP_SCO_CONNECTED_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(APP_HFP_SCO_CONNECTED_IND_T);
const marshal_type_descriptor_t marshal_type_descriptor_APP_HFP_SCO_DISCONNECTED_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(APP_HFP_SCO_DISCONNECTED_IND_T);


