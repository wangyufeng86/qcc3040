/*!
\copyright  Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       connection_manager_marshal_defs.c
\brief      Marshal type definitions for connection manager component.
*/

#include "connection_manager.h"

#include <marshal.h>
#include <marshal_common.h>

/*! CON_MANAGER_CONNECTION_IND message marshalling type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_CON_MANAGER_CONNECTION_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(sizeof(CON_MANAGER_CONNECTION_IND_T));


