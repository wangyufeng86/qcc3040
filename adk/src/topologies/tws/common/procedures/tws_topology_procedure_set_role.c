/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "tws_topology_procedure_set_role.h"
#include "tws_topology_procedures.h"
#include "tws_topology_private.h"
#include "tws_topology_client_msgs.h"

/* proc specific includes here */

#include <logging.h>

#include <message.h>

const SET_ROLE_TYPE_T proc_set_role_primary_role = {tws_topology_role_primary, FALSE};
const SET_ROLE_TYPE_T proc_set_role_acting_primary_role = {tws_topology_role_primary, TRUE};
const SET_ROLE_TYPE_T proc_set_role_secondary_role = {tws_topology_role_secondary, FALSE};
const SET_ROLE_TYPE_T proc_set_role_dfu_role = {tws_topology_role_dfu, FALSE};
const SET_ROLE_TYPE_T proc_set_role_none = {tws_topology_role_none, FALSE};

void TwsTopology_ProcedureSetRoleStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedureSetRoleCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_set_role_fns = {
    TwsTopology_ProcedureSetRoleStart,
    TwsTopology_ProcedureSetRoleCancel,
};

void TwsTopology_ProcedureSetRoleStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    SET_ROLE_TYPE_T* role_type = (SET_ROLE_TYPE_T*)goal_data;

    UNUSED(result_task);

    DEBUG_LOG("TwsTopology_ProcedureSetRoleStart %u role %d acting %d", role_type, role_type->role, role_type->acting_in_role);

    /* procedure started synchronously so indicate success */
    proc_start_cfm_fn(tws_topology_procedure_set_role, procedure_result_success);

    /* start the procedure 
     *  - update the role single point of truth here in topology
     */
    twsTopology_SetActingInRole(role_type->acting_in_role);
    twsTopology_SetRole(role_type->role);

    /* procedure completed synchronously so indicate completed already */
    Procedures_DelayedCompleteCfmCallback(proc_complete_fn, tws_topology_procedure_set_role, procedure_result_success);
}

void TwsTopology_ProcedureSetRoleCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureSetRoleCancel");
    /* nothing to cancel, just return success to keep goal engine happy */
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_set_role, procedure_result_success);
}
