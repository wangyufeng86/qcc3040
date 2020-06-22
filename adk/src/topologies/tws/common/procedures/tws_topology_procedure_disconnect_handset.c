/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief
*/

#include "tws_topology_procedure_disconnect_handset.h"
#include "tws_topology_procedures.h"
#include "tws_topology_client_msgs.h"

#include <bt_device.h>
#include <device_list.h>
#include <device_properties.h>
#include <handset_service.h>
#include <connection_manager.h>

#include <logging.h>

#include <message.h>

static void twsTopology_ProcDisconnectHandsetHandleMessage(Task task, MessageId id, Message message);
void TwsTopology_ProcedureDisconnectHandsetStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedureDisconnectHandsetCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_disconnect_handset_fns = {
    TwsTopology_ProcedureDisconnectHandsetStart,
    TwsTopology_ProcedureDisconnectHandsetCancel,
};

typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
    bool active;
} twsTopProcDisconnectHandsetTaskData;

twsTopProcDisconnectHandsetTaskData twstop_proc_disconnect_handset = {twsTopology_ProcDisconnectHandsetHandleMessage};

#define TwsTopProcDisconnectHandsetGetTaskData()     (&twstop_proc_disconnect_handset)
#define TwsTopProcDisconnectHandsetGetTask()         (&twstop_proc_disconnect_handset.task)

static void twsTopology_ProcDisconnectHandsetResetProc(void)
{
    twsTopProcDisconnectHandsetTaskData* td = TwsTopProcDisconnectHandsetGetTaskData();
    td->active = FALSE;
}

void TwsTopology_ProcedureDisconnectHandsetStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcDisconnectHandsetTaskData* td = TwsTopProcDisconnectHandsetGetTaskData();
    bdaddr handset_addr;

    UNUSED(result_task);
    UNUSED(goal_data);

    DEBUG_LOG("TwsTopology_ProcedureDisconnectHandsetStart");

    appDeviceGetHandsetBdAddr(&handset_addr);

    /* Request to Handset Services to disonnect Handset even it is disonnected,
    Handset Services sends HANDSET_SERVICE_DISCONNECT_CFM if nothing to do, this
    message is used by topology to send TWS_TOPOLOGY_HANDSET_DISCONNECTED_IND to
    apps sm. When earbud been requested to enter into User Pairing Mode,apps state
    machine makes decision of entering into pairing mode after reception of
    TWS_TOPOLOGY_HANDSET_DISCONNECTED_IND. */
    HandsetService_DisconnectRequest(TwsTopProcDisconnectHandsetGetTask(), &handset_addr);

    /* start the procedure */
    td->complete_fn = proc_complete_fn;
    td->active = TRUE;

    proc_start_cfm_fn(tws_topology_procedure_disconnect_handset, procedure_result_success);
}

void TwsTopology_ProcedureDisconnectHandsetCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureDisconnectHandsetCancel");

    twsTopology_ProcDisconnectHandsetResetProc();
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn,
                                         tws_topology_procedure_disconnect_handset,
                                         procedure_result_success);
}

static void twsTopology_ProcDisconnectHandsetHandleHandsetConnectCfm(const HANDSET_SERVICE_CONNECT_CFM_T *cfm)
{
    DEBUG_LOG("twsTopology_ProcDisconnectHandsetHandleHandsetConnectCfm status %d", cfm->status);

    UNUSED(cfm);
}

static void twsTopology_ProcDisconnectHandsetHandleHandsetDisconnectCfm(const HANDSET_SERVICE_DISCONNECT_CFM_T *cfm)
{
    twsTopProcDisconnectHandsetTaskData* td = TwsTopProcDisconnectHandsetGetTaskData();
    tp_bdaddr le_handset_tpaddr;
    bool le_handset = HandsetService_GetConnectedLeHandsetTpAddress(&le_handset_tpaddr);

    DEBUG_LOG("twsTopology_ProcDisconnectHandsetHandleHandsetDisconnectCfm status %d", cfm->status);

    if (le_handset)
    {
        HandsetService_DisconnectTpAddrRequest(TwsTopProcDisconnectHandsetGetTask(), &le_handset_tpaddr);
    }
    else
    {
        twsTopology_ProcDisconnectHandsetResetProc();
        td->complete_fn(tws_topology_procedure_disconnect_handset, procedure_result_success);

        TwsTopology_SendHandsetDisconnectedIndication();
    }
}

static void twsTopology_ProcDisconnectHandsetHandleMessage(Task task, MessageId id, Message message)
{
    twsTopProcDisconnectHandsetTaskData* td = TwsTopProcDisconnectHandsetGetTaskData();

    UNUSED(task);

    if (!td->active)
    {
        return;
    }

    switch (id)
    {
    case HANDSET_SERVICE_CONNECT_CFM:
        twsTopology_ProcDisconnectHandsetHandleHandsetConnectCfm((const HANDSET_SERVICE_CONNECT_CFM_T *)message);
        break;

    case HANDSET_SERVICE_DISCONNECT_CFM:
        twsTopology_ProcDisconnectHandsetHandleHandsetDisconnectCfm((const HANDSET_SERVICE_DISCONNECT_CFM_T *)message);
        break;

    default:
        break;
    }
}
