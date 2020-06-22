/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Definition of HEADSET topology specific procedures.
*/

#ifndef HEADSET_TOPOLOGY_PROCEDURES_H
#define HEADSET_TOPOLOGY_PROCEDURES_H

#include "procedures.h"


/*! Definition of HEADSET Topology procedures. 

    A naming convention is followed for important procedures. 
    Following the convention and using in the recommended order
    reduces the possibility of unexpected behaviour.

    The convention applies to functions that enable or disable 
    activity.
    \li allow changes the response if an event happens.
    \li permit starts or stops an activity temporarily
    \li enable Starts or stops an activity permanently

    If several of these functions are called with DISABLE parameters
    it is recommended that they are called in the order 
    allow - permit - enable.
*/

typedef enum
{
    hs_topology_procedure_enable_connectable_handset = 1,

    hs_topology_procedure_allow_handset_connection,

    hs_topology_procedure_connect_handset,

    hs_topology_procedure_disconnect_handset,
} headset_topology_procedure;


#endif /* HEADSET_TOPOLOGY_PROCEDURES_H */
