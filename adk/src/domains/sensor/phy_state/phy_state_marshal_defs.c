/*!
\copyright  Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       phy_state_marshal_defs.c
\brief      Marshal type definitions for phy state component.
*/

#include "phy_state.h"

#include <marshal.h>
#include <marshal_common.h>

/*! PHY_STATE_CHANGED_IND message marshalling member descriptor. */
const marshal_member_descriptor_t PHY_STATE_CHANGED_IND_member_descriptors[] = 
{
    MAKE_MARSHAL_MEMBER(PHY_STATE_CHANGED_IND_T, phyState, new_state),
    MAKE_MARSHAL_MEMBER(PHY_STATE_CHANGED_IND_T, phy_state_event, event),
};
/*! PHY_STATE_CHANGED_IND message marshalling type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_PHY_STATE_CHANGED_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION(PHY_STATE_CHANGED_IND_T, PHY_STATE_CHANGED_IND_member_descriptors);

