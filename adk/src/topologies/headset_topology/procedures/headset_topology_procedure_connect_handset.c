/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Procedure for headset to connect BR/EDR ACL to Handset.
*/

#include "headset_topology_procedure_connect_handset.h"
#include "headset_topology_config.h"
#include "core/headset_topology_rules.h"
#include "procedures.h"
#include "headset_topology_procedures.h"


#include <handset_service.h>
#include <connection_manager.h>
#include <logging.h>

#include <message.h>
#include <panic.h>

void HeadsetTopology_ProcedureConnectHandsetStart(Task result_task,
                                                  procedure_start_cfm_func_t proc_start_cfm_fn,
                                                  procedure_complete_func_t proc_complete_fn,
                                                  Message goal_data);
static void HeadsetTopology_ProcedureConnectHandsetCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);
static void headsetTopology_ProcConnectHandsetResetProc(void);

const procedure_fns_t hs_proc_connect_handset_fns = {
    HeadsetTopology_ProcedureConnectHandsetStart,
    HeadsetTopology_ProcedureConnectHandsetCancel,
};


typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
    uint8 profiles_status;
    bdaddr handset_addr;
} headsetTopProcConnectHandsetTaskData;

headsetTopProcConnectHandsetTaskData headsettop_proc_connect_handset;

#define HeadsetTopProcConnectHandsetGetTaskData()     (&headsettop_proc_connect_handset)
#define HeadsetTopProcConnectHandsetGetTask()     (&headsettop_proc_connect_handset.task)

/*! Internal messages use by this ConnectHandset procedure. */
typedef enum
{
    PROC_CONNECT_HANDSET_INTERNAL_ACL_CONNECT,
    PROC_CONNECT_HANDSET_INTERNAL_ACL_CONNECT_TIMEOUT,
} procConnetHandsetInternalMessages;

static void headsetTopology_ProcConnectHandsetHandleMessage(Task task, MessageId id, Message message);

headsetTopProcConnectHandsetTaskData headsettop_proc_connect_handset = {headsetTopology_ProcConnectHandsetHandleMessage};

void HeadsetTopology_ProcedureConnectHandsetStart(Task result_task,
                                                  procedure_start_cfm_func_t proc_start_cfm_fn,
                                                  procedure_complete_func_t proc_complete_fn,
                                                  Message goal_data)
{
    UNUSED(result_task);
    headsetTopProcConnectHandsetTaskData* td = HeadsetTopProcConnectHandsetGetTaskData();
    HSTOP_GOAL_CONNECT_HANDSET_T* chp = (HSTOP_GOAL_CONNECT_HANDSET_T*)goal_data;

    DEBUG_LOG("HeadsetTopology_ProcedureConnectHandsetStart profiles 0x%x", chp->profiles);

    td->complete_fn = proc_complete_fn;
    td->profiles_status = chp->profiles;
    BdaddrSetZero(&td->handset_addr);

    /* start the procedure */
    if (appDeviceGetHandsetBdAddr(&td->handset_addr))
    {
        HandsetService_ConnectAddressRequest(HeadsetTopProcConnectHandsetGetTask(), &td->handset_addr, chp->profiles);
        ConManagerRegisterTpConnectionsObserver(cm_transport_bredr, HeadsetTopProcConnectHandsetGetTask());
        proc_start_cfm_fn(hs_topology_procedure_connect_handset, procedure_result_success);
    }
    else
    {
        DEBUG_LOG("HeadsetTopology_ProcedureConnectHandsetStart shouldn't be called with no paired handset");
        Panic();
    }
}

void HeadsetTopology_ProcedureConnectHandsetCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    UNUSED(proc_cancel_cfm_fn);

    headsetTopology_ProcConnectHandsetResetProc();
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, hs_topology_procedure_connect_handset, procedure_result_success);
}

static void headsetTopology_ProcConnectHandsetResetProc(void)
{
    headsetTopProcConnectHandsetTaskData* td = HeadsetTopProcConnectHandsetGetTaskData();

    td->profiles_status = 0;
    td->complete_fn = NULL;
    BdaddrSetZero(&td->handset_addr);

    ConManagerUnregisterTpConnectionsObserver(cm_transport_bredr, HeadsetTopProcConnectHandsetGetTask());
}


static void headsetTopology_ProcConnectHandsetProfilesStatus(uint8 profile)
{
    headsetTopProcConnectHandsetTaskData* td = HeadsetTopProcConnectHandsetGetTaskData();

    /* clear the connected profile */
    td->profiles_status &= ~profile;

    /* report start complete if all done */
    if (!td->profiles_status)
    {
        td->complete_fn(hs_topology_procedure_connect_handset, procedure_result_success);
        headsetTopology_ProcConnectHandsetResetProc();
    }
}


static void headsetTopology_ProcConnectHandsetHandleHandsetConnectCfm(const HANDSET_SERVICE_CONNECT_CFM_T *cfm)
{
    headsetTopProcConnectHandsetTaskData* td = HeadsetTopProcConnectHandsetGetTaskData();
    DEBUG_LOG("headsetTopology_ProcConnectHandsetHandleHandsetConnectCfm status %d", cfm->status);

    if (cfm->status == handset_service_status_success)
    {
        device_t dev = BtDevice_GetDeviceForBdAddr(&cfm->addr);
        uint8 profiles_connected = BtDevice_GetConnectedProfiles(dev);
        headsetTopology_ProcConnectHandsetProfilesStatus(profiles_connected);
    }
    else if (cfm->status != handset_service_status_cancelled)
    {
        td->complete_fn(hs_topology_procedure_connect_handset, procedure_result_failed);
        headsetTopology_ProcConnectHandsetResetProc();
    }
    else
    {
        Panic();
    }
}


/*! Unregister as a conManager's observer.
    \param conn_ind The Connection manager indication
 */
static void headsetTopology_ProcConnectHandsetHandleConMgrConnInd(const CON_MANAGER_TP_CONNECT_IND_T *conn_ind)
{
    DEBUG_LOG("headsetTopology_ProcConnectHandsetHandleConMgrConnInd Transport:%d Type:%d, Tpaddr:x%06x Incoming connection:%d",
                    conn_ind->tpaddr.transport, conn_ind->tpaddr.taddr.type, conn_ind->tpaddr.taddr.addr.lap, conn_ind->incoming);

    if ((conn_ind->tpaddr.transport == TRANSPORT_BREDR_ACL)
        && appDeviceIsHandset(&conn_ind->tpaddr.taddr.addr))
    {
        ConManagerUnregisterTpConnectionsObserver(cm_transport_bredr, HeadsetTopProcConnectHandsetGetTask());
    }
}


static void headsetTopology_ProcConnectHandsetHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    headsetTopProcConnectHandsetTaskData* td = HeadsetTopProcConnectHandsetGetTaskData();

    /* ignore any delivered messages if no longer active */
    if (td->complete_fn == NULL)
    {
        return;
    }

    switch (id)
    {
        case HANDSET_SERVICE_CONNECT_CFM:
            headsetTopology_ProcConnectHandsetHandleHandsetConnectCfm((const HANDSET_SERVICE_CONNECT_CFM_T *)message);
            break;

        case CON_MANAGER_TP_CONNECT_IND:
            headsetTopology_ProcConnectHandsetHandleConMgrConnInd((const CON_MANAGER_TP_CONNECT_IND_T *)message);
            break;

        case CON_MANAGER_TP_DISCONNECT_IND:
            DEBUG_LOG("headsetTopology_ProcConnectHandsetHandleMessage: CON_MANAGER_TP_DISCONNECT_IND");
            break;

        default:
            DEBUG_LOG("headsetTopology_ProcConnectHandsetHandleMessage unhandled id 0x%x(%d)", id, id);
            break;
    }
}


