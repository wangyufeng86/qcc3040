/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Generic interface to TWS Topology procedure for enabling/disabling ble adverts.
*/

#ifndef TWS_TOPOLOGY_PROCEDURE_ENABLE_LE_CONNECTABLE_HANDSET_H
#define TWS_TOPOLOGY_PROCEDURE_ENABLE_LE_CONNECTABLE_HANDSET_H

#include "tws_topology_procedures.h"
#include "tws_topology_common_primary_rule_functions.h"
#include "script_engine.h"
#include <message.h>

typedef struct
{
    bool enable;
} TWSTOP_PRIMARY_GOAL_ENABLE_LE_CONNECTABLE_HANDSET_T;

extern const TWSTOP_PRIMARY_GOAL_ENABLE_LE_CONNECTABLE_HANDSET_T le_enable_connectable;
#define PROC_ENABLE_LE_CONNECTABLE_PARAMS_ENABLE  ((Message)&le_enable_connectable)

extern const TWSTOP_PRIMARY_GOAL_ENABLE_LE_CONNECTABLE_HANDSET_T le_disable_connectable;
#define PROC_ENABLE_LE_CONNECTABLE_PARAMS_DISABLE  ((Message)&le_disable_connectable)

extern const procedure_fns_t proc_enable_le_connectable_handset_fns;

#endif /* TWS_TOPOLOGY_PROCEDURE_ENABLE_LE_CONNECTABLE_HANDSET_H */

