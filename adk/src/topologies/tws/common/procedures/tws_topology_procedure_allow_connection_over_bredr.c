/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure for permiting legacy Bluetooth connections

            \note this permits the activities... but does not create them

            The procedure informs connection manager that such connections are
            permitted. They will only be created if connection manager has been
            asked to create them by
            \li allowing page scanning
            \li paging a device
            \li connecting a profile
*/

#include "tws_topology_procedure_allow_connection_over_bredr.h"
#include "tws_topology_procedures.h"

#include <connection_manager.h>

#include <logging.h>

#include <message.h>
#include <panic.h>

const ALLOW_CONNECTION_OVER_BREDR_T proc_allow_connections_over_bredr_enable = {.enable = TRUE };
const ALLOW_CONNECTION_OVER_BREDR_T proc_allow_connections_over_bredr_disable = {.enable = FALSE };


static void TwsTopology_ProcedureAllowConnectionOverBredrStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
static void TwsTopology_ProcedureAllowConnectionOverBredrCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_allow_connection_over_bredr_fns = {
    TwsTopology_ProcedureAllowConnectionOverBredrStart,
    TwsTopology_ProcedureAllowConnectionOverBredrCancel,
};

static void TwsTopology_ProcedureAllowConnectionOverBredrStart(Task result_task,
                                                 procedure_start_cfm_func_t proc_start_cfm_fn,
                                                 procedure_complete_func_t proc_complete_fn,
                                                 Message goal_data)
{
    ALLOW_CONNECTION_OVER_BREDR_T *params = (ALLOW_CONNECTION_OVER_BREDR_T *)goal_data;

    UNUSED(result_task);

    PanicNull(params);

    DEBUG_LOG("TwsTopology_ProcedureAllowConnectionOverBredrStart. Enable:%d", params->enable);

    ConManagerAllowConnection(cm_transport_bredr, params->enable);

    proc_start_cfm_fn(tws_topology_procedure_allow_connection_over_bredr, procedure_result_success);

    Procedures_DelayedCompleteCfmCallback(proc_complete_fn, tws_topology_procedure_allow_connection_over_bredr, procedure_result_success);
}

static void TwsTopology_ProcedureAllowConnectionOverBredrCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureAllowConnectionOverBredrCancel");

    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_allow_connection_over_bredr, procedure_result_success);
}

