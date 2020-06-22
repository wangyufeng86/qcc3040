/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "tws_topology_procedure_find_role.h"
#include "tws_topology_procedures.h"
#include "tws_topology_config.h"
#include "tws_topology_procedure_allow_connection_over_le.h"
#include "tws_topology_procedure_permit_bt.h"

#include <peer_find_role.h>

#include <logging.h>

#include <message.h>

const FIND_ROLE_PARAMS_T proc_find_role_timeout = {TwsTopologyConfig_InitialPeerFindRoleTimeoutS()};
const FIND_ROLE_PARAMS_T proc_find_role_continuous = {0};

static void twsTopology_ProcFindRoleHandleMessage(Task task, MessageId id, Message message);
void TwsTopology_ProcedureFindRoleStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedureFindRoleCancel(procedure_cancel_cfm_func_t proc_cancel_fn);

const procedure_fns_t proc_find_role_fns = {
    TwsTopology_ProcedureFindRoleStart,
    TwsTopology_ProcedureFindRoleCancel,
};

typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
    procedure_cancel_cfm_func_t cancel_fn;
} twsTopProcFindRoleTaskData;

twsTopProcFindRoleTaskData twstop_proc_find_role = {twsTopology_ProcFindRoleHandleMessage};

#define TwsTopProcFindRoleGetTaskData()     (&twstop_proc_find_role)
#define TwsTopProcFindRoleGetTask()         (&twstop_proc_find_role.task)


static void twsTopology_ProcedureFindRoleReset(void)
{
    twsTopProcFindRoleTaskData* td = TwsTopProcFindRoleGetTaskData();
    PeerFindRole_UnregisterTask(TwsTopProcFindRoleGetTask());
    td->complete_fn = NULL;
    td->cancel_fn = NULL;
}

void TwsTopology_ProcedureFindRoleStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcFindRoleTaskData* td = TwsTopProcFindRoleGetTaskData();
    FIND_ROLE_PARAMS_T* params = (FIND_ROLE_PARAMS_T*)goal_data;

    UNUSED(result_task);

    DEBUG_LOG("TwsTopology_ProcedureFindRoleStart timeout %d", params->timeout);

    td->complete_fn = proc_complete_fn;    

    LeAdvertisingManager_ParametersSelect(0);

    PeerFindRole_RegisterTask(TwsTopProcFindRoleGetTask());
    PeerFindRole_FindRole(D_SEC(params->timeout));

    /* start is synchronous, use the callback to confirm now */
    proc_start_cfm_fn(tws_topology_procedure_find_role, procedure_result_success);

    /* if starting in continuous mode indicate the procedure is complete already,
     * this permits the procedure to be used in the middle of a script where we
     * want to continue processing subsequent procedures */
    if (goal_data == PROC_FIND_ROLE_TIMEOUT_DATA_CONTINUOUS)
    {
        twsTopology_ProcedureFindRoleReset();
        Procedures_DelayedCompleteCfmCallback(proc_complete_fn, tws_topology_procedure_find_role, procedure_result_success);
    }
}

void TwsTopology_ProcedureFindRoleCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    twsTopProcFindRoleTaskData* td = TwsTopProcFindRoleGetTaskData();

    DEBUG_LOG("TwsTopology_ProcedureFindRoleCancel");
    td->complete_fn = NULL;
    td->cancel_fn = proc_cancel_cfm_fn;
    PeerFindRole_FindRoleCancel();

    /* cancellation is asynchronous for this procedure, goal handling knows will have to wait for
     * the cancel cfm callback */
}

static void twsTopology_ProcFindRoleHandleMessage(Task task, MessageId id, Message message)
{
    twsTopProcFindRoleTaskData* td = TwsTopProcFindRoleGetTaskData();

    UNUSED(task);
    UNUSED(message);

    if (!td->complete_fn && !td->cancel_fn)
    {
        /* If neither callback is set this procedure is not active so ignore any messages */
        return;
    }

    switch (id)
    {
        /* These messages all indicate the procedure completed, so indicate such
         * to the topology client. The topology client is separately registered
         * to receive role notifications from the find role service, so no need
         * to send result. */
        case PEER_FIND_ROLE_NO_PEER:
        case PEER_FIND_ROLE_ACTING_PRIMARY:
        case PEER_FIND_ROLE_PRIMARY:
        case PEER_FIND_ROLE_SECONDARY:
            if (td->complete_fn)
            {
                td->complete_fn(tws_topology_procedure_find_role, procedure_result_success);
            }
            else if (td->cancel_fn)
            {
                /* It may be possible that peer_find_role completed before the
                   cancel was processed - in that situation we should call the
                   cancel_fn instead of the complete_fn. */
                td->cancel_fn(tws_topology_procedure_find_role, procedure_result_success);
            }

            twsTopology_ProcedureFindRoleReset();
            break;

        case PEER_FIND_ROLE_CANCELLED:
            td->cancel_fn(tws_topology_procedure_find_role, procedure_result_success);
            twsTopology_ProcedureFindRoleReset();
            break;

        default:
            break;
    }
}
