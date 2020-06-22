/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure for permiting Bluetooth Low Energy connections

            \note this permits the activities... but does not create them

            The procedure informs connection manager that such connections are
            permitted. They will only be created if connectin manager has been
            asked to create them by
            \li allowing connectable advertising
            \li asked to create a connection, which will use scanning
*/

#include "tws_topology_procedure_allow_connection_over_le.h"
#include "tws_topology_procedures.h"

#include <connection_manager.h>

#include <logging.h>

#include <message.h>
#include <panic.h>

const ALLOW_CONNECTION_OVER_LE_T proc_allow_connections_over_le_enable = {.enable = TRUE };
const ALLOW_CONNECTION_OVER_LE_T proc_allow_connections_over_le_disable = {.enable = FALSE };


static void TwsTopology_ProcedureAllowConnectionOverLeStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
static void TwsTopology_ProcedureAllowConnectionOverLeCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_allow_connection_over_le_fns = {
    TwsTopology_ProcedureAllowConnectionOverLeStart,
    TwsTopology_ProcedureAllowConnectionOverLeCancel,
};

static void TwsTopology_ProcedureAllowConnectionOverLeStart(Task result_task,
                                                 procedure_start_cfm_func_t proc_start_cfm_fn,
                                                 procedure_complete_func_t proc_complete_fn,
                                                 Message goal_data)
{
    ALLOW_CONNECTION_OVER_LE_T *params = (ALLOW_CONNECTION_OVER_LE_T *)goal_data;

    UNUSED(result_task);

    PanicNull(params);

    DEBUG_LOG("TwsTopology_ProcedureAllowConnectionOverLeStart. Enable:%d", params->enable);

    ConManagerAllowConnection(cm_transport_ble, params->enable);

    proc_start_cfm_fn(tws_topology_procedure_allow_connection_over_le, procedure_result_success);

    Procedures_DelayedCompleteCfmCallback(proc_complete_fn, tws_topology_procedure_allow_connection_over_le, procedure_result_success);
}

static void TwsTopology_ProcedureAllowConnectionOverLeCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureAllowConnectionOverLeCancel");

    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_allow_connection_over_le, procedure_result_success);
}

