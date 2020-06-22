/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "tws_topology_procedure_pair_peer.h"
#include "tws_topology_procedures.h"
#include "tws_topology_procedure_allow_connection_over_le.h"
#include "tws_topology_procedure_permit_bt.h"

#include <peer_pair_le.h>

#include <logging.h>

#include <message.h>
#include <panic.h>

#define MAKE_MESSAGE(TYPE)  TYPE##_T *message = PanicUnlessNew(TYPE##_T);

static void twsTopology_ProcPairPeerHandleMessage(Task task, MessageId id, Message message);
void TwsTopology_ProcedurePairPeerStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedurePairPeerCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_pair_peer_fns = {
    TwsTopology_ProcedurePairPeerStart,
    TwsTopology_ProcedurePairPeerCancel,
};

typedef struct
{
    TaskData task;
    Task result_task;
    procedure_complete_func_t complete_fn;
} twsTopProcPairPeerTaskData;

twsTopProcPairPeerTaskData twstop_proc_pair_peer = {twsTopology_ProcPairPeerHandleMessage};

#define TwsTopProcPairPeerGetTaskData()     (&twstop_proc_pair_peer)
#define TwsTopProcPairPeerGetTask()         (&twstop_proc_pair_peer.task)

void TwsTopology_ProcedurePairPeerStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    UNUSED(goal_data);

    DEBUG_LOG_INFO("TwsTopology_ProcedurePairPeerStart");

    TwsTopProcPairPeerGetTaskData()->complete_fn = proc_complete_fn;
    TwsTopProcPairPeerGetTaskData()->result_task = result_task;

    PeerPairLe_FindPeer(TwsTopProcPairPeerGetTask());

    /* procedure started synchronously, confirm start now */
    proc_start_cfm_fn(tws_topology_procedure_pair_peer, procedure_result_success);
}

void TwsTopology_ProcedurePairPeerCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedurePairPeerCancel NOT SUPPORTED");
    /* Peer pairing cannot be cancelled and there are currently no use cases to do so.
       Return failure to catch any attempt to do so.
       If we want to add cancellation, we'll need a cancel API in peer pair LE */
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_pair_peer, procedure_result_failed);
}

static void twsTopology_ProcPairPeerHandlePeerPairLePairCfm(const PEER_PAIR_LE_PAIR_CFM_T *cfm)
{
    MAKE_MESSAGE(PROC_PAIR_PEER_RESULT);

    DEBUG_LOG_INFO("twsTopology_ProcPairPeerHandlePeerPairLePairCfm %u", cfm->status);

    message->success = cfm->status == peer_pair_le_status_success;
    MessageSend(TwsTopProcPairPeerGetTaskData()->result_task, PROC_PAIR_PEER_RESULT, message);

    TwsTopProcPairPeerGetTaskData()->complete_fn(tws_topology_procedure_pair_peer, procedure_result_success);
}

static void twsTopology_ProcPairPeerHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case PEER_PAIR_LE_PAIR_CFM:
            twsTopology_ProcPairPeerHandlePeerPairLePairCfm((const PEER_PAIR_LE_PAIR_CFM_T *)message);
            break;

        default:
            break;
    }
}


#define PEER_PAIR_SCRIPT(ENTRY) \
    ENTRY(proc_allow_connection_over_le_fns, PROC_ALLOW_CONNECTION_OVER_LE_ENABLE), \
    ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_ENABLE), \
    ENTRY(proc_pair_peer_fns, NO_DATA),


/* Define the script pair_peer_script */
DEFINE_TOPOLOGY_SCRIPT(pair_peer,PEER_PAIR_SCRIPT);

