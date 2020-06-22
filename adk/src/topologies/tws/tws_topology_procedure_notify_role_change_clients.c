/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       tws_topology_procedure_notify_role_change_clients.c
\brief      Procedure to notify registered clients of the intention to force/propose/cancel a role change.
*/

#include "tws_topology_procedure_notify_role_change_clients.h"
#include "tws_topology_procedures.h"
#include "tws_topology_private.h"
#include "tws_topology_role_change_client_notifier.h"


#include <logging.h>

#include <message.h>
#include <panic.h>

#define TWSTOP_PROC_CLIENT_ACCEPTANCE_TIMEOUT_TIMEOUT_MS        (1000)
#define TWSTOP_PROC_CLIENT_FORCE_RESPONSE_TIMEOUT_TIMEOUT_MS    (500)

typedef enum
{
    TWSTOP_PROC_INTERNAL_PROPOSAL_TIMEOUT,
    TWSTOP_PROC_INTERNAL_FORCE_TIMEOUT,
};

/*! Parameter definition for force notification */
const NOTIFY_ROLE_CHANGE_CLIENTS_PARAMS_T proc_notify_role_change_clients_force_params = { .notification = tws_notify_role_change_force_notification };

/*! Parameter definition for proposal notification */
const NOTIFY_ROLE_CHANGE_CLIENTS_PARAMS_T proc_notify_role_change_clients_propose_params = { .notification = tws_notify_role_change_proposal_notification };

/*! Parameter definition for cancel notification */
const NOTIFY_ROLE_CHANGE_CLIENTS_PARAMS_T proc_notify_role_change_clients_cancel_params = { .notification = tws_notify_role_change_cancel_notification };

typedef struct
{
    TaskData task;
    NOTIFY_ROLE_CHANGE_CLIENTS_PARAMS_T params;
    role_change_notification_t notication_in_progress;
    procedure_complete_func_t complete_fn;
    procedure_cancel_cfm_func_t cancel_fn;
    
} twsTopProcNotifyRoleChangeClientsTaskData;

static void twsTopology_ProcNotifyRoleChangeClientsHandleMessage(Task task, MessageId id, Message message);

void TwsTopology_ProcedureNotifyRoleChangeClientsStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);

void TwsTopology_ProcedureNotifyRoleChangeClientsCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

twsTopProcNotifyRoleChangeClientsTaskData twstop_proc_notify_role_change_clients = {twsTopology_ProcNotifyRoleChangeClientsHandleMessage};

#define TwsTopProcNotifyRoleChangeClientsGetTaskData()     (&twstop_proc_notify_role_change_clients)
#define TwsTopProcNotifyRoleChangeClientsGetTask()         (&twstop_proc_notify_role_change_clients.task)



static void TwsTopology_ProcedureNotifyRoleChangeClientsReset(void)
{
    twsTopProcNotifyRoleChangeClientsTaskData* td = TwsTopProcNotifyRoleChangeClientsGetTaskData();
    
    MessageCancelFirst(TwsTopProcNotifyRoleChangeClientsGetTask(), TWSTOP_PROC_INTERNAL_PROPOSAL_TIMEOUT);
    MessageCancelFirst(TwsTopProcNotifyRoleChangeClientsGetTask(), TWSTOP_PROC_INTERNAL_FORCE_TIMEOUT);
    td->complete_fn = NULL;
    td->notication_in_progress = tws_notify_role_change_nothing_to_notify;
}

procedure_fns_t proc_notify_role_change_clients_fns = {
    TwsTopology_ProcedureNotifyRoleChangeClientsStart,
    TwsTopology_ProcedureNotifyRoleChangeClientsCancel,
};


void TwsTopology_ProcedureNotifyRoleChangeClientsStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcNotifyRoleChangeClientsTaskData* td = TwsTopProcNotifyRoleChangeClientsGetTaskData();
    NOTIFY_ROLE_CHANGE_CLIENTS_PARAMS_T* rc_params = (NOTIFY_ROLE_CHANGE_CLIENTS_PARAMS_T*)goal_data;

    UNUSED(result_task);
   
    td->params = *rc_params;
    td->complete_fn = proc_complete_fn;
    td->notication_in_progress = tws_notify_role_change_nothing_to_notify;
    /* start the procedure */
    switch(td->params.notification)
    {
        case tws_notify_role_change_force_notification:    
        {
            DEBUG_LOG("TwsTopology_ProcedureNotifyRoleChangeClientsStart tws_notify_role_change_force_notification");
            td->notication_in_progress = tws_notify_role_change_force_notification;
            TwsTopology_NotifyListenersOfForcedRoleChange(TwsTopProcNotifyRoleChangeClientsGetTask());
            MessageSendLater(TwsTopProcNotifyRoleChangeClientsGetTask(), TWSTOP_PROC_INTERNAL_FORCE_TIMEOUT, NULL, TWSTOP_PROC_CLIENT_FORCE_RESPONSE_TIMEOUT_TIMEOUT_MS);                
    }
        break;
        case tws_notify_role_change_proposal_notification:
        {
            DEBUG_LOG("TwsTopology_ProcedureNotifyRoleChangeClientsStart tws_notify_role_change_proposal_notification");
            /* REMOVE this Panic() when role change clients are known to support role change rejection*/
            Panic(); 
            td->notication_in_progress = tws_notify_role_change_proposal_notification;
            TwsTopology_NotifyListenersOfRoleChangeProposal(TwsTopProcNotifyRoleChangeClientsGetTask());
            MessageSendLater(TwsTopProcNotifyRoleChangeClientsGetTask(), TWSTOP_PROC_INTERNAL_PROPOSAL_TIMEOUT, NULL, TWSTOP_PROC_CLIENT_ACCEPTANCE_TIMEOUT_TIMEOUT_MS);
        }
        break;
        case tws_notify_role_change_cancel_notification:
        {
            DEBUG_LOG("TwsTopology_ProcedureNotifyRoleChangeClientsStart tws_notify_role_change_cancel_notification");
            td->notication_in_progress = tws_notify_role_change_cancel_notification;
            TwsTopology_NotifyListenersOfRoleChangeCancellation();
        }
        break;
        case tws_notify_role_change_nothing_to_notify:
        {
            DEBUG_LOG("TwsTopology_ProcedureNotifyRoleChangeClientsStart tws_notify_role_change_nothing_to_notify");
            Panic();
        }
        break;
        
        default: 
        break;
    }
    proc_start_cfm_fn(tws_topology_procedure_notify_role_change_clients, procedure_result_success); 
}

void TwsTopology_ProcedureNotifyRoleChangeClientsCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    twsTopProcNotifyRoleChangeClientsTaskData* td = TwsTopProcNotifyRoleChangeClientsGetTaskData();

    
    switch(td->notication_in_progress)
    {
        case tws_notify_role_change_force_notification:  
        case tws_notify_role_change_proposal_notification:
        {
            DEBUG_LOG("TwsTopology_ProcedureNotifyRoleChangeClientsCancel force/proposal notification");

            td->notication_in_progress = tws_notify_role_change_nothing_to_notify; 
            TwsTopology_NotifyListenersOfRoleChangeCancellation();
        }
        break;

        case tws_notify_role_change_cancel_notification:
        {
            DEBUG_LOG("TwsTopology_ProcedureNotifyRoleChangeClientsCancel cancel_notification does nothing.");
            td->notication_in_progress = tws_notify_role_change_nothing_to_notify;
            /* Nothing to achieve by cancelling cancel */ 
        }
        break;
        
        case tws_notify_role_change_nothing_to_notify:
        DEBUG_LOG("TwsTopology_ProcedureNotifyRoleChangeClientsCancel nothing_to_notify.");
        Panic();
        break;
        
        default: 
        break;
    }
    
    /* must use delayed cfm callback to prevent script engine recursion */
    Procedures_DelayedCompleteCfmCallback(proc_cancel_cfm_fn,
                                         tws_topology_procedure_notify_role_change_clients,
                                         procedure_result_success);
    TwsTopology_ProcedureNotifyRoleChangeClientsReset();
}

static void twsTopology_ProcNotifyRoleChangeClientsHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);
    twsTopProcNotifyRoleChangeClientsTaskData* td = TwsTopProcNotifyRoleChangeClientsGetTaskData();
    if (!td->complete_fn)
    {
        /* If callback is not set this procedure is not active so ignore any messages */
        return;
    }
    
    switch (id)
    {
        case TWSTOP_INTERNAL_ROLE_CHANGE_CLIENT_REJECTION:
        {    
            DEBUG_LOG("twsTopology_ProcNotifyRoleChangeClientsHandleMessage TWSTOP_INTERNAL_ROLE_CHANGE_CLIENT_REJECTION");
            Procedures_DelayedCompleteCfmCallback(td->complete_fn,
                                                       tws_topology_procedure_notify_role_change_clients,
                                                       procedure_result_timeout);
            TwsTopology_ProcedureNotifyRoleChangeClientsReset();
        }
        break;

        case TWSTOP_INTERNAL_ALL_ROLE_CHANGE_CLIENTS_PREPARED:
        {         
            DEBUG_LOG("twsTopology_ProcNotifyRoleChangeClientsHandleMessage TWSTOP_INTERNAL_ALL_ROLE_CHANGE_CLIENTS_PREPARED");
            Procedures_DelayedCompleteCfmCallback(td->complete_fn,
                                                       tws_topology_procedure_notify_role_change_clients,
                                                       procedure_result_success);
            TwsTopology_ProcedureNotifyRoleChangeClientsReset();
        }
        break;
        
        case TWSTOP_PROC_INTERNAL_PROPOSAL_TIMEOUT:
        {       
            DEBUG_LOG("twsTopology_ProcNotifyRoleChangeClientsHandleMessage TWSTOP_PROC_INTERNAL_PROPOSAL_TIMEOUT");
            Procedures_DelayedCompleteCfmCallback(td->complete_fn,
                                                       tws_topology_procedure_notify_role_change_clients,
                                                       procedure_result_success); 
                /* Note procedure_result_success is returned above deliberately as we do not want to
                 *  prevent a topology role change from occuring*/
            TwsTopology_ProcedureNotifyRoleChangeClientsReset();
        }
        break;
        
        case TWSTOP_PROC_INTERNAL_FORCE_TIMEOUT:
        {  
            DEBUG_LOG("twsTopology_ProcNotifyRoleChangeClientsHandleMessage TWSTOP_PROC_INTERNAL_FORCE_TIMEOUT");
            Procedures_DelayedCompleteCfmCallback(td->complete_fn,
                                                       tws_topology_procedure_notify_role_change_clients,
                                                       procedure_result_success);
                /* Note procedure_result_success is returned above deliberately as we do not want to
                 *  prevent a topology role change from occuring*/
            TwsTopology_ProcedureNotifyRoleChangeClientsReset();
        }
        break;
        
        default: 
        break;
    } 
}
