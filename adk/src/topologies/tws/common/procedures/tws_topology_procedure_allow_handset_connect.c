/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure for allowing or disallowing handset connections

            \note this allows the connection if one arrives, but does not 
            make any changes to allow a connection
*/

#include "tws_topology_procedure_allow_handset_connect.h"

#include <connection_manager.h>

#include <logging.h>

#include <message.h>
#include <panic.h>


const ALLOW_HANDSET_CONNECT_PARAMS_T proc_allow_handset_connect_enable =  { .enable = TRUE };
const ALLOW_HANDSET_CONNECT_PARAMS_T proc_allow_handset_connect_disable = { .enable = FALSE };


void TwsTopology_ProcedureAllowHandsetConnectStart(Task result_task,
                                                   procedure_start_cfm_func_t proc_start_cfm_fn,
                                                   procedure_complete_func_t proc_complete_fn,
                                                   Message goal_data);
void TwsTopology_ProcedureAllowHandsetConnectCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_allow_handset_connect_fns = {
    TwsTopology_ProcedureAllowHandsetConnectStart,
    TwsTopology_ProcedureAllowHandsetConnectCancel,
};


void TwsTopology_ProcedureAllowHandsetConnectStart(Task result_task,
                                                   procedure_start_cfm_func_t proc_start_cfm_fn,
                                                   procedure_complete_func_t proc_complete_fn,
                                                   Message goal_data)
{
    const ALLOW_HANDSET_CONNECT_PARAMS_T *param = (const ALLOW_HANDSET_CONNECT_PARAMS_T *)goal_data;

    UNUSED(result_task);

    DEBUG_LOG("TwsTopology_ProcedureAllowHandsetConnectStart allow %d", param->enable);

    ConManagerAllowHandsetConnect(param->enable);

    proc_start_cfm_fn(tws_topology_procedure_allow_handset_connection, procedure_result_success);
    Procedures_DelayedCompleteCfmCallback(proc_complete_fn, tws_topology_procedure_allow_handset_connection, procedure_result_success);
}

void TwsTopology_ProcedureAllowHandsetConnectCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureAllowHandsetConnectCancel");

    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_allow_handset_connection, procedure_result_success);
}

