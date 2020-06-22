/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Interfaace to Topology procedure to set the BT address to Primary or Secondary. 
*/

#ifndef TWS_TOPOLOGY_PROC_SET_ADDRESS_H
#define TWS_TOPOLOGY_PROC_SET_ADDRESS_H

#include "script_engine.h"
#include "tws_topology_procedures.h"

/*! Set Address procedure functions interface. */
extern const procedure_fns_t proc_set_address_fns;

/*! Procedure configuration parameter, used to specify Primary or Secondary address.
    Not typically used directly, but instead via the constants and macros below.
*/
typedef struct
{
    bool primary;
} SET_ADDRESS_TYPE_T;

/*! Primary address parameter. */
extern const SET_ADDRESS_TYPE_T proc_set_address_primary;
/*! Macro to access primary address parameter as a goal_data parameter. */
#define PROC_SET_ADDRESS_TYPE_DATA_PRIMARY  ((Message)&proc_set_address_primary)

/*! Secondary address parameter. */
extern const SET_ADDRESS_TYPE_T proc_set_address_secondary;
/*! Macro to access secondary address parameter as a goal_data parameter. */
#define PROC_SET_ADDRESS_TYPE_DATA_SECONDARY  ((Message)&proc_set_address_secondary)

/* Make script available that will set primary address (including BT protection). */
extern const procedure_script_t set_primary_address_script;

#endif /* TWS_TOPOLOGY_PROC_SET_ADDRESS_H */
