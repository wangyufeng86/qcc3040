/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure for closing all remaining connections. This should normally be 
            called after requesting normal shutdowns of connections.
*/

#include "tws_topology_procedure_clean_connections.h"

#include <bt_device.h>
#include <connection_manager.h>
#include <timestamp_event.h>

#include <logging.h>

#include <message.h>
#include <panic.h>

typedef enum
{
    TWSTOP_PROC_INTERNAL_CLOSE_HANDSET_TIMEOUT,
};

typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
    bool active;
} twsTopProcCleanConnectionsTaskData;

static void twsTopology_ProcCleanConnectionsHandleMessage(Task task, MessageId id, Message message);

twsTopProcCleanConnectionsTaskData twstop_proc_clean_connections = {.task = twsTopology_ProcCleanConnectionsHandleMessage};

#define TwsTopProcCleanConnectionsGetTaskData()     (&twstop_proc_clean_connections)
#define TwsTopProcCleanConnectionsGetTask()         (&twstop_proc_clean_connections.task)



void TwsTopology_ProcedureCleanConnectionsStart(Task result_task,
                                                   procedure_start_cfm_func_t proc_start_cfm_fn,
                                                   procedure_complete_func_t proc_complete_fn,
                                                   Message goal_data);
void TwsTopology_ProcedureCleanConnectionsCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);


const procedure_fns_t proc_clean_connections_fns = {
    TwsTopology_ProcedureCleanConnectionsStart,
    TwsTopology_ProcedureCleanConnectionsCancel,
};

static void TwsTopology_ProcedureCleanConnectionsResetProc(void)
{
    twsTopProcCleanConnectionsTaskData* td = TwsTopProcCleanConnectionsGetTaskData();

    DEBUG_LOG("TwsTopology_ProcedureCleanConnectionsResetProc");

    TimestampEvent(TIMESTAMP_EVENT_CLEAN_CONNECTIONS_COMPLETED);

    MessageCancelAll(TwsTopProcCleanConnectionsGetTask(), TWSTOP_PROC_INTERNAL_CLOSE_HANDSET_TIMEOUT);
    td->active = FALSE;

    ConManagerUnregisterConnectionsClient(TwsTopProcCleanConnectionsGetTask());
}

void TwsTopology_ProcedureCleanConnectionsStart(Task result_task,
                                                procedure_start_cfm_func_t proc_start_cfm_fn,
                                                procedure_complete_func_t proc_complete_fn,
                                                Message goal_data)
{
    twsTopProcCleanConnectionsTaskData* td = TwsTopProcCleanConnectionsGetTaskData();
    bdaddr addr;

    UNUSED(result_task);
    UNUSED(goal_data);

    DEBUG_LOG("TwsTopology_ProcedureCleanConnectionsStart");

    td->complete_fn = proc_complete_fn;
    td->active = TRUE;

    TimestampEvent(TIMESTAMP_EVENT_CLEAN_CONNECTIONS_STARTED);

    ConManagerRegisterConnectionsClient(TwsTopProcCleanConnectionsGetTask());

    /* Disconnect the handset first, if it is connected */
    if (appDeviceGetHandsetBdAddr(&addr) && ConManagerIsConnected(&addr))
    {
        DEBUG_LOG("TwsTopology_ProcedureCleanConnectionsStart DISCONNECTING HANDSET");
        ConManagerSendCloseAclRequest(&addr, TRUE);
        MessageSendLater(TwsTopProcCleanConnectionsGetTask(), TWSTOP_PROC_INTERNAL_CLOSE_HANDSET_TIMEOUT, NULL,
                         TWSTOP_PROC_CLOSE_HANDSET_IND_TIMEOUT_MS);
    }
    else
    {
        /* Otherwise force disconnect any remaining ACLs, which may include the peer */
        DEBUG_LOG("TwsTopology_ProcedureCleanConnectionsStart DISCONNECT ALL");
        ConManagerTerminateAllAcls(TwsTopProcCleanConnectionsGetTask());
    }

    proc_start_cfm_fn(tws_topology_procedure_clean_connections, procedure_result_success);
}


void TwsTopology_ProcedureCleanConnectionsCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureCleanConnectionsCancel");

    TwsTopology_ProcedureCleanConnectionsResetProc();
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_clean_connections, procedure_result_success);
}


static void twsTopology_ProcCleanConnectionsHandleConManagerConnectionInd(const CON_MANAGER_CONNECTION_IND_T* ind)
{
    DEBUG_LOG("twsTopology_ProcCleanConnectionsHandleConManagerConnectionInd Conn:%u BLE:%u %04x,%02x,%06lx", ind->connected,
                                                                                          ind->ble,
                                                                                          ind->bd_addr.nap,
                                                                                          ind->bd_addr.uap,
                                                                                          ind->bd_addr.lap);

    if (   (!ind->connected)
        && (appDeviceIsHandset(&ind->bd_addr)
        && (!ind->ble)))
    {
        /* Force disconnect any remaining ACLs, which may include the peer */
        DEBUG_LOG("twsTopology_ProcCleanConnectionsHandleConManagerConnectionInd handset disconnected, now DISCONNECT ALL");
        MessageCancelAll(TwsTopProcCleanConnectionsGetTask(), TWSTOP_PROC_INTERNAL_CLOSE_HANDSET_TIMEOUT);
        ConManagerTerminateAllAcls(TwsTopProcCleanConnectionsGetTask());
    }
}


static void twsTopology_ProcCleanConnectionsHandleCloseAllCfm(void)
{
    DEBUG_LOG("twsTopology_ProcCleanConnectionsHandleCloseAllCfm");

    if (TwsTopProcCleanConnectionsGetTaskData()->active)
    {
        TwsTopology_ProcedureCleanConnectionsResetProc();
        TwsTopProcCleanConnectionsGetTaskData()->complete_fn(tws_topology_procedure_clean_connections, 
                                                             procedure_result_success);
    }
}


static void twsTopology_ProcCleanConnectionsHandleMessage(Task task, MessageId id, Message message)
{
    twsTopProcCleanConnectionsTaskData* td = TwsTopProcCleanConnectionsGetTaskData();

    UNUSED(task);
    UNUSED(message);

    if (!td->active)
    {
        return;
    }

    switch (id)
    {
    case TWSTOP_PROC_INTERNAL_CLOSE_HANDSET_TIMEOUT:
        {
            DEBUG_LOG("twsTopology_ProcCleanConnectionsHandleMessage ******************************** TIMEOUT waiting on CON_MANAGER_CONNECTION_IND");

            /* Force disconnect any remaining ACLs, which may include the peer */
            ConManagerTerminateAllAcls(TwsTopProcCleanConnectionsGetTask());

            bdaddr addr;
            if (appDeviceGetHandsetBdAddr(&addr) && ConManagerIsConnected(&addr))
            {
                /* Log the case where the timeout triggered because it is
                   too short, i.e. the handset is still connected. */
                DEBUG_LOG("twsTopology_ProcCleanConnectionsHandleMessage Handset still connected");
            }
        }
        break;

    case CON_MANAGER_CLOSE_ALL_CFM:
        twsTopology_ProcCleanConnectionsHandleCloseAllCfm();
        break;

    case CON_MANAGER_CONNECTION_IND:
        DEBUG_LOG("twsTopology_ProcCleanConnectionsHandleConManagerConnectionInd CON_MANAGER_CONNECTION_IND");
        twsTopology_ProcCleanConnectionsHandleConManagerConnectionInd((const CON_MANAGER_CONNECTION_IND_T*)message);
        break;

    default:
        break;
    }
}
