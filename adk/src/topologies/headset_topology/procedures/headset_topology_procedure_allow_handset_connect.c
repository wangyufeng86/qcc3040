/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure for allowing or disallowing handset connections

            \note this allows the connection if one arrives, but does not 
            make any changes to allow a connection
*/

#include "headset_topology_procedure_allow_handset_connect.h"

#include <connection_manager.h>

#include <logging.h>

#include <message.h>
#include <panic.h>

const ALLOW_HANDSET_CONNECT_PARAMS_T hs_proc_allow_handset_connect_enable =  { .enable = TRUE };
const ALLOW_HANDSET_CONNECT_PARAMS_T hs_proc_allow_handset_connect_disable = { .enable = FALSE };


static void HeadsetTopology_ProcedureAllowHandsetConnectStart(Task result_task,
                                                              procedure_start_cfm_func_t proc_start_cfm_fn,
                                                              procedure_complete_func_t proc_complete_fn,
                                                              Message goal_data);
static void HeadsetTopology_ProcedureAllowHandsetConnectCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t hs_proc_allow_handset_connect_fns = {
    HeadsetTopology_ProcedureAllowHandsetConnectStart,
    HeadsetTopology_ProcedureAllowHandsetConnectCancel,
};

static void HeadsetTopology_ProcedureAllowHandsetConnectStart(Task result_task,
                                                              procedure_start_cfm_func_t proc_start_cfm_fn,
                                                              procedure_complete_func_t proc_complete_fn,
                                                              Message goal_data)
{
    const ALLOW_HANDSET_CONNECT_PARAMS_T *param = (const ALLOW_HANDSET_CONNECT_PARAMS_T *)goal_data;
    UNUSED(result_task);

    DEBUG_LOG("HeadsetTopology_ProcedureAllowHandsetConnectStart allow %d", param->enable);
    ConManagerAllowHandsetConnect(param->enable);
    proc_start_cfm_fn(hs_topology_procedure_allow_handset_connection, procedure_result_success);
    Procedures_DelayedCompleteCfmCallback(proc_complete_fn, hs_topology_procedure_allow_handset_connection, procedure_result_success);
}


static void HeadsetTopology_ProcedureAllowHandsetConnectCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("HeadsetTopology_ProcedureAllowHandsetConnectCancel");

    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn,
                                         hs_topology_procedure_allow_handset_connection,
                                         procedure_result_success);
}

