/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure to cleanly disconnect LE links.
*/

#include "tws_topology_procedure_disconnect_le_connections.h"
#include "tws_topology_procedures.h"

#include <connection_manager.h>

#include <logging.h>

#include <message.h>

static void twsTopology_ProcDisconnectLeConnectionsHandleMessage(Task task, MessageId id, Message message);
void TwsTopology_ProcedureDisconnectLeConnectionsStart(Task result_task,
                                                       procedure_start_cfm_func_t proc_start_cfm_fn,
                                                       procedure_complete_func_t proc_complete_fn,
                                                       Message goal_data);
void TwsTopology_ProcedureDisconnectLeConnectionsCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_disconnect_le_connections_fns = {
    TwsTopology_ProcedureDisconnectLeConnectionsStart,
    TwsTopology_ProcedureDisconnectLeConnectionsCancel,
};

typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
    bool active;
} twsTopProcDisconnectLeConnectionsTaskData;

twsTopProcDisconnectLeConnectionsTaskData twstop_proc_disconnect_le_connections = {twsTopology_ProcDisconnectLeConnectionsHandleMessage};

#define TwsTopProcDisconnectLeConnectionsGetTaskData()     (&twstop_proc_disconnect_le_connections)
#define TwsTopProcDisconnectLeConnectionsGetTask()         (&twstop_proc_disconnect_le_connections.task)

void TwsTopology_ProcedureDisconnectLeConnectionsStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcDisconnectLeConnectionsTaskData* td = TwsTopProcDisconnectLeConnectionsGetTaskData();

    UNUSED(result_task);
    UNUSED(goal_data);

    td->complete_fn = proc_complete_fn;
    td->active = TRUE;
    ConManagerDisconnectAllLeConnectionsRequest(TwsTopProcDisconnectLeConnectionsGetTask());

    /* procedure starts synchronously so return TRUE */
    proc_start_cfm_fn(tws_topology_procedure_disconnect_le_connections, procedure_result_success);

}

void TwsTopology_ProcedureDisconnectLeConnectionsCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    twsTopProcDisconnectLeConnectionsTaskData* td = TwsTopProcDisconnectLeConnectionsGetTaskData();

    DEBUG_LOG("TwsTopology_ProcedureDisconnectLeConnectionsCancel");

    td->active = FALSE;
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_disconnect_le_connections, procedure_result_success);
}

static void twsTopology_ProcDisconnectLeConnectionsHandleMessage(Task task, MessageId id, Message message)
{
    twsTopProcDisconnectLeConnectionsTaskData* td = TwsTopProcDisconnectLeConnectionsGetTaskData();

    UNUSED(task);
    UNUSED(message);

    if (!td->active)
    {
        return;
    }

    switch (id)
    {
        case CON_MANAGER_DISCONNECT_ALL_LE_CONNECTIONS_CFM:
            td->active = FALSE;
            Procedures_DelayedCompleteCfmCallback(td->complete_fn, tws_topology_procedure_disconnect_le_connections, procedure_result_success);
            break;

        default:
            DEBUG_LOG("twsTopology_ProcDisconnectLeConnectionsHandleMessage id %x", id);
    }
}
