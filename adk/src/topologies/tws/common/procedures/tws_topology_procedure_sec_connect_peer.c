/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure for Secondary to connect BR/EDR ACL to Primary.
*/

#include "tws_topology_procedure_sec_connect_peer.h"
#include "tws_topology_procedures.h"
#include "tws_topology_config.h"

#include <connection_manager.h>
#include <bt_device.h>

#include <logging.h>

#include <message.h>

static void twsTopology_ProcSecConnectPeerHandleMessage(Task task, MessageId id, Message message);
void TwsTopology_ProcedureSecConnectPeerStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedureSecConnectPeerCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_sec_connect_peer_fns = {
    TwsTopology_ProcedureSecConnectPeerStart,
    TwsTopology_ProcedureSecConnectPeerCancel,
};

/*! Internal messages use by this SecConnectPeer procedure. */
typedef enum
{
    PROC_SEC_CONNECT_PEER_INTERNAL_ACL_CONNECT,
    PROC_SEC_CONNECT_PEER_INTERNAL_ACL_CONNECT_TIMEOUT,
} procSecConnetPeerInternalMessages;

typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
    uint16* connect_wait_lock;
} twsTopProcSecConnectPeerTaskData;

twsTopProcSecConnectPeerTaskData twstop_proc_sec_connect_peer = {twsTopology_ProcSecConnectPeerHandleMessage};

#define TwsTopProcSecConnectPeerGetTaskData()     (&twstop_proc_sec_connect_peer)
#define TwsTopProcSecConnectPeerGetTask()         (&twstop_proc_sec_connect_peer.task)

void TwsTopology_ProcedureSecConnectPeerStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcSecConnectPeerTaskData* td = TwsTopProcSecConnectPeerGetTaskData();
    bdaddr peer_addr;

    UNUSED(result_task);
    UNUSED(goal_data);

    DEBUG_LOG("TwsTopology_ProcedureSecConnectPeerStart");

    td->complete_fn = proc_complete_fn;

    /* start the procedure */
    if (appDeviceGetPrimaryBdAddr(&peer_addr))
    {
        /* attempt to create BR/EDR ACL to primary earbud and inform goal
         * engine that procedure has started */
        td->connect_wait_lock = ConManagerCreateAcl(&peer_addr);
        proc_start_cfm_fn(tws_topology_procedure_sec_connect_peer, procedure_result_success);

        if (!td->connect_wait_lock)
        {
            /* lock already clear, means ACL already exists */
            DEBUG_LOG("TwsTopology_ProcedureSecConnectPeerStart shouldn't be called when already have ACL to peer");
            Procedures_DelayedCompleteCfmCallback(proc_complete_fn, tws_topology_procedure_sec_connect_peer, procedure_result_success);
        }
        else
        {
            /* post message that will be delivered either when ACL is connected
             * or page timeout occured */
            MessageSendConditionally(TwsTopProcSecConnectPeerGetTask(),
                                     PROC_SEC_CONNECT_PEER_INTERNAL_ACL_CONNECT,
                                     NULL, td->connect_wait_lock);
            MessageSendLater(TwsTopProcSecConnectPeerGetTask(),
                        PROC_SEC_CONNECT_PEER_INTERNAL_ACL_CONNECT_TIMEOUT,
                        NULL, TwsTopologyConfig_SecondaryPeerConnectTimeoutMs());
        }
    }
    else
    {
        DEBUG_LOG("TwsTopology_ProcedureSecConnectPeerStart shouldn't be called with no peer");
        Panic();
    }
}

void TwsTopology_ProcedureSecConnectPeerCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureSecConnectPeerCancel");

    MessageCancelFirst(TwsTopProcSecConnectPeerGetTask(), PROC_SEC_CONNECT_PEER_INTERNAL_ACL_CONNECT);
    MessageCancelFirst(TwsTopProcSecConnectPeerGetTask(), PROC_SEC_CONNECT_PEER_INTERNAL_ACL_CONNECT_TIMEOUT);

    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_sec_connect_peer, procedure_result_success);
}

static void twsTopology_ProcSecConnectPeerCheckAclStatus(bool timeout)
{
    twsTopProcSecConnectPeerTaskData* td = TwsTopProcSecConnectPeerGetTaskData();
    bdaddr peer_addr;

    if (appDeviceGetPrimaryBdAddr(&peer_addr))
    {
        if (ConManagerIsConnected(&peer_addr))
        {
            td->complete_fn(tws_topology_procedure_sec_connect_peer, procedure_result_success);
        }
        else
        {
            td->complete_fn(tws_topology_procedure_sec_connect_peer,
                            timeout ? procedure_result_timeout : procedure_result_failed);
        }
    }
    else
    {
        Panic();
    }
}

static void twsTopology_ProcSecConnectPeerHandleInternalAclConnect(void)
{
    /* had a result from connection manager, cancel the timeout */
    MessageCancelFirst(TwsTopProcSecConnectPeerGetTask(), PROC_SEC_CONNECT_PEER_INTERNAL_ACL_CONNECT_TIMEOUT);
    /* check if connected to peer and return procedure success or failure */
    twsTopology_ProcSecConnectPeerCheckAclStatus(FALSE);
}

static void twsTopology_ProcSecConnectPeerHandleInternalAclConnectTimeout(void)
{
    /* timeout fired, cancel lock message that might race against the timeout if connection
     * happened whilst the timeout was on the message queue */
    MessageCancelFirst(TwsTopProcSecConnectPeerGetTask(), PROC_SEC_CONNECT_PEER_INTERNAL_ACL_CONNECT);
    /* check if connected to peer and return procedure success or timeout */
    twsTopology_ProcSecConnectPeerCheckAclStatus(TRUE);
}

static void twsTopology_ProcSecConnectPeerHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case PROC_SEC_CONNECT_PEER_INTERNAL_ACL_CONNECT:
            twsTopology_ProcSecConnectPeerHandleInternalAclConnect();
            break;
        case PROC_SEC_CONNECT_PEER_INTERNAL_ACL_CONNECT_TIMEOUT:
            twsTopology_ProcSecConnectPeerHandleInternalAclConnectTimeout();
            break;
        default:
            break;
    }
}
