/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "tws_topology_procedure_cancel_find_role.h"
#include "tws_topology_procedures.h"

#include <peer_find_role.h>

#include <logging.h>

#include <message.h>

static void twsTopology_ProcCancelFindRoleHandleMessage(Task task, MessageId id, Message message);
void TwsTopology_ProcedureCancelFindRoleStart(Task result_task,
                                              procedure_start_cfm_func_t proc_start_cfm_fn,
                                              procedure_complete_func_t proc_complete_fn,
                                              Message goal_data);
void TwsTopology_ProcedureCancelFindRoleCancel(procedure_cancel_cfm_func_t proc_cancel_fn);

const procedure_fns_t proc_cancel_find_role_fns = {
    TwsTopology_ProcedureCancelFindRoleStart,
    TwsTopology_ProcedureCancelFindRoleCancel,
};

typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
    bool active;
} twsTopProcCancelFindRoleTaskData;

twsTopProcCancelFindRoleTaskData twstop_proc_cancel_find_role = {twsTopology_ProcCancelFindRoleHandleMessage};

#define TwsTopProcCancelFindRoleGetTaskData()     (&twstop_proc_cancel_find_role)
#define TwsTopProcCancelFindRoleGetTask()         (&twstop_proc_cancel_find_role.task)

static void twsTopology_ProcedureCancelFindRoleReset(void)
{
    twsTopProcCancelFindRoleTaskData* td = TwsTopProcCancelFindRoleGetTaskData();
    PeerFindRole_UnregisterTask(TwsTopProcCancelFindRoleGetTask());
    td->active = FALSE;
}

void TwsTopology_ProcedureCancelFindRoleStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcCancelFindRoleTaskData* td = TwsTopProcCancelFindRoleGetTaskData();

    UNUSED(goal_data);
    UNUSED(result_task);

    DEBUG_LOG("TwsTopology_ProcedureCancelFindRoleStart");

    td->complete_fn = proc_complete_fn;
    td->active = TRUE;

    PeerFindRole_RegisterTask(TwsTopProcCancelFindRoleGetTask());

    PeerFindRole_FindRoleCancel();

    /* start is synchronous, use the callback to confirm now */
    proc_start_cfm_fn(tws_topology_procedure_cancel_find_role, procedure_result_success);
}

void TwsTopology_ProcedureCancelFindRoleCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureCancelFindRoleCancel");

    /* nothing to do for cancel, except ignore any pending messages */
    twsTopology_ProcedureCancelFindRoleReset();
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_cancel_find_role, procedure_result_success);
}

static void twsTopology_ProcCancelFindRoleHandleMessage(Task task, MessageId id, Message message)
{
    twsTopProcCancelFindRoleTaskData* td = TwsTopProcCancelFindRoleGetTaskData();

    UNUSED(task);
    UNUSED(message);

    if (!td->active)
    {
        return;
    }

    switch (id)
    {
        case PEER_FIND_ROLE_CANCELLED:
            twsTopology_ProcedureCancelFindRoleReset();
            td->complete_fn(tws_topology_procedure_cancel_find_role, procedure_result_success);
            break;

        default:
            break;
    }
}
