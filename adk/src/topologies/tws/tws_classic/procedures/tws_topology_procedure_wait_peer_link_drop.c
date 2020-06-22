/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure for a Secondary to wait for peer link to drop.
*/

#include "tws_topology_procedure_wait_peer_link_drop.h"
#include "tws_topology_procedures.h"

#include <connection_manager.h>
#include <bt_device.h>

#include <logging.h>
#include <message.h>
#include <panic.h>

typedef enum
{
    TWSTOP_PROC_WAIT_PEER_LINK_DROP_INTERNAL_TIMEOUT,
};

const WAIT_PEER_LINK_DROP_TYPE_T wait_peer_link_drop_default_timeout = {TWSTOP_PROC_WAIT_PEER_LINK_DROP_DEFAULT_TIMEOUT_MS};

static void twsTopology_ProcWaitPeerLinkDropHandleMessage(Task task, MessageId id, Message message);
void TwsTopology_ProcedureWaitPeerLinkDropStart(Task result_task,
                                                procedure_start_cfm_func_t proc_start_cfm_fn,
                                                procedure_complete_func_t proc_complete_fn,
                                                Message goal_data);
void TwsTopology_ProcedureWaitPeerLinkDropCancel(procedure_cancel_cfm_func_t proc_cancel_fn);

const procedure_fns_t proc_wait_peer_link_drop_fns = {
    TwsTopology_ProcedureWaitPeerLinkDropStart,
    TwsTopology_ProcedureWaitPeerLinkDropCancel,
};

typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
} twsTopProcWaitPeerLinkDropTaskData;

twsTopProcWaitPeerLinkDropTaskData twstop_proc_wait_peer_link_drop = {twsTopology_ProcWaitPeerLinkDropHandleMessage};

#define TwsTopProcWaitPeerLinkDropGetTaskData()     (&twstop_proc_wait_peer_link_drop)
#define TwsTopProcWaitPeerLinkDropGetTask()         (&twstop_proc_wait_peer_link_drop.task)

static void twsTopology_ProcWaitPeerLinkDropCleanup(void)
{
    twsTopProcWaitPeerLinkDropTaskData* td = TwsTopProcWaitPeerLinkDropGetTaskData();

    ConManagerUnregisterTpConnectionsObserver(cm_transport_bredr, TwsTopProcWaitPeerLinkDropGetTask());

    td->complete_fn = NULL;
    MessageCancelAll(TwsTopProcWaitPeerLinkDropGetTask(), 
                     TWSTOP_PROC_WAIT_PEER_LINK_DROP_INTERNAL_TIMEOUT);
}


static void twsTopology_ProcWaitPeerLinkDropCompleteWithStatus(procedure_result_t status)
{
    twsTopProcWaitPeerLinkDropTaskData* td = TwsTopProcWaitPeerLinkDropGetTaskData();

    Procedures_DelayedCompleteCfmCallback(td->complete_fn,
                                           tws_topology_procedure_wait_peer_link_drop,
                                           status);
    twsTopology_ProcWaitPeerLinkDropCleanup();
}

void TwsTopology_ProcedureWaitPeerLinkDropStart(Task result_task,
                                                procedure_start_cfm_func_t proc_start_cfm_fn,
                                                procedure_complete_func_t proc_complete_fn,
                                                Message goal_data)
{
    twsTopProcWaitPeerLinkDropTaskData* td = TwsTopProcWaitPeerLinkDropGetTaskData();
    WAIT_PEER_LINK_DROP_TYPE_T *procedure_params = (WAIT_PEER_LINK_DROP_TYPE_T *)goal_data;
    uint32 timeout;
    bdaddr peer_addr;

    UNUSED(result_task);

    DEBUG_LOG("TwsTopology_ProcedureWaitPeerLinkDropStart");

    PanicNull(procedure_params);

    timeout = procedure_params->timeout_ms;
    PanicZero(timeout);

    /* start is synchronous, use the callback to confirm now */
    proc_start_cfm_fn(tws_topology_procedure_wait_peer_link_drop, procedure_result_success);

    /* remember the completion callback and we wait for indication from connection
     * manager */
    td->complete_fn = proc_complete_fn;

    /* register to get notification of peer disconnect.
       This is done now (before checking synchronously) to avoid message race */
    ConManagerRegisterTpConnectionsObserver(cm_transport_bredr, TwsTopProcWaitPeerLinkDropGetTask());

    if (!appDeviceGetPeerBdAddr(&peer_addr))
    {
        /* no peer address, shouldn't happen, but would be a hard fail */
        twsTopology_ProcWaitPeerLinkDropCompleteWithStatus(procedure_result_failed);
    }
    else
    {
        if (!ConManagerIsConnected(&peer_addr))
        {
            twsTopology_ProcWaitPeerLinkDropCompleteWithStatus(procedure_result_success);
        }
        else
        {
            DEBUG_LOG("TwsTopology_ProcedureWaitPeerLinkDropStart waiting for peer link to drop");

            MessageSendLater(TwsTopProcWaitPeerLinkDropGetTask(),
                             TWSTOP_PROC_WAIT_PEER_LINK_DROP_INTERNAL_TIMEOUT,
                             NULL,
                             timeout);
        }
    }

}

void TwsTopology_ProcedureWaitPeerLinkDropCancel(procedure_cancel_cfm_func_t proc_cancel_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureWaitPeerLinkDropCancel");

    Procedures_DelayedCancelCfmCallback(proc_cancel_fn, tws_topology_procedure_wait_peer_link_drop, procedure_result_success);
    twsTopology_ProcWaitPeerLinkDropCleanup();
}

static void twsTopology_ProcWaitPeerLinkDropHandleConManTpDisconnectInd(const CON_MANAGER_TP_DISCONNECT_IND_T* ind)
{
    DEBUG_LOG("twsTopology_ProcWaitPeerLinkDropHandleConManTpDisconnectInd %04x,%02x,%06lx",
                ind->tpaddr.taddr.addr.nap, ind->tpaddr.taddr.addr.uap, ind->tpaddr.taddr.addr.lap);

    if (appDeviceIsPeer(&ind->tpaddr.taddr.addr))
    {
        twsTopology_ProcWaitPeerLinkDropCompleteWithStatus(procedure_result_success);
    }
}


static void twsTopology_ProcWaitPeerLinkDropHandleTimeout(void)
{
    twsTopology_ProcWaitPeerLinkDropCompleteWithStatus(procedure_result_timeout);
}


static void twsTopology_ProcWaitPeerLinkDropHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    if (TwsTopProcWaitPeerLinkDropGetTaskData()->complete_fn)
    {
        switch (id)
        {
            case CON_MANAGER_TP_DISCONNECT_IND:
                twsTopology_ProcWaitPeerLinkDropHandleConManTpDisconnectInd((CON_MANAGER_TP_DISCONNECT_IND_T*)message);
                break;

            case TWSTOP_PROC_WAIT_PEER_LINK_DROP_INTERNAL_TIMEOUT:
                twsTopology_ProcWaitPeerLinkDropHandleTimeout();
                break;

            default:
                DEBUG_LOG("twsTopology_ProcWaitPeerLinkDropHandleMessage unhandled id 0x%x", id);
                break;
        }
    }
}

