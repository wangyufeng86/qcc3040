#include "tws_topology_private.h"
#include "tws_topology_config.h"
#include "tws_topology.h"
#include "tws_topology_procedure_enable_le_connectable_handset.h"
#include "tws_topology_procedures.h"
#include "handset_service.h"
#include <bt_device.h>
#include <logging.h>
#include <message.h>
#include <panic.h>

/*! LE connectable procedure task data */
typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
    procedure_cancel_cfm_func_t cancel_fn;
} twsTopProcEnableLeConnectableHandsetTaskData;

/*! LE connectable procedure message handler */
static void twsTopology_ProcEnableLeConnectableHandsetHandleMessage(Task task, MessageId id, Message message);

twsTopProcEnableLeConnectableHandsetTaskData twstop_proc_enable_le_connectable_handset = 
                                {.task = twsTopology_ProcEnableLeConnectableHandsetHandleMessage};

#define TwsTopProcEnableLeConnectableHandsetGetTaskData()     (&twstop_proc_enable_le_connectable_handset)
#define TwsTopProcEnableLeConnectableHandsetGetTask()         (&twstop_proc_enable_le_connectable_handset.task)

const TWSTOP_PRIMARY_GOAL_ENABLE_LE_CONNECTABLE_HANDSET_T le_disable_connectable={FALSE};
const TWSTOP_PRIMARY_GOAL_ENABLE_LE_CONNECTABLE_HANDSET_T le_enable_connectable={TRUE};

void TwsTopology_ProcedureEnableLeConnectableHandsetStart(Task result_task,
                                                          procedure_start_cfm_func_t proc_start_cfm_fn,
                                                          procedure_complete_func_t proc_complete_fn,
                                                          Message goal_data);

void TwsTopology_ProcedureEnableLeConnectableHandsetCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_enable_le_connectable_handset_fns = {
    TwsTopology_ProcedureEnableLeConnectableHandsetStart,
    TwsTopology_ProcedureEnableLeConnectableHandsetCancel,
};

static void twsTopology_ProcEnableLeConnectableHandsetReset(void)
{
    twsTopProcEnableLeConnectableHandsetTaskData * td = TwsTopProcEnableLeConnectableHandsetGetTaskData();

    HandsetService_ClientUnregister(TwsTopProcEnableLeConnectableHandsetGetTask());

    td->complete_fn = NULL;
    td->cancel_fn = NULL;
}

static procedure_result_t twsTopology_ProcGetStatus(handset_service_status_t status)
{
    procedure_result_t result = procedure_result_success;

    DEBUG_LOG("twsTopology_ProcGetStatus() Status: %d", status);

    /* Handle return status from procedure */
    if(status != handset_service_status_success)
       result = procedure_result_failed;

    return result;
}

static void twsTopology_ProcEnableLeConnectableHandsetHandleMessage(Task task, MessageId id, Message message)
{
    twsTopProcEnableLeConnectableHandsetTaskData* td = TwsTopProcEnableLeConnectableHandsetGetTaskData();

    UNUSED(task);

    DEBUG_LOG("twsTopology_ProcLeConnectableHandleMessage() Recieved id: 0x%x(%d)", id, id);
    if (!td->complete_fn && !td->cancel_fn)
    {
        return;
    }

    switch (id)
    {
        case HANDSET_SERVICE_LE_CONNECTABLE_IND:
        {
            HANDSET_SERVICE_LE_CONNECTABLE_IND_T* msg = (HANDSET_SERVICE_LE_CONNECTABLE_IND_T*)message;

            DEBUG_LOG("twsTopology_ProcLeConnectableHandleMessage() LE Connectable: %d, Status: %d", msg->le_connectable, msg->status);

            if (td->complete_fn)
            {
                td->complete_fn(tws_topology_procedure_enable_le_connectable_handset, twsTopology_ProcGetStatus(msg->status));
            }
            else if (td->cancel_fn)
            {
                td->cancel_fn(tws_topology_procedure_enable_le_connectable_handset, procedure_result_success);
            }

            twsTopology_ProcEnableLeConnectableHandsetReset();
            break;
        }

        default:
            break;
    }
}

void TwsTopology_ProcedureEnableLeConnectableHandsetStart(Task result_task,
                                                  procedure_start_cfm_func_t proc_start_cfm_fn,
                                                  procedure_complete_func_t proc_complete_fn,
                                                  Message goal_data)
{
    twsTopProcEnableLeConnectableHandsetTaskData* td = TwsTopProcEnableLeConnectableHandsetGetTaskData();
    
    TWSTOP_PRIMARY_GOAL_ENABLE_LE_CONNECTABLE_HANDSET_T* params = (TWSTOP_PRIMARY_GOAL_ENABLE_LE_CONNECTABLE_HANDSET_T*)goal_data;
    DEBUG_LOG("TwsTopology_ProcedureEnableLeConnectableHandsetStart Enable %d ", params->enable);

    UNUSED(result_task);

    td->complete_fn = proc_complete_fn;

    /* Register to be able to receive HANDSET_SERVICE_BLE_CONNECTABLE_CFM */
    HandsetService_ClientRegister(TwsTopProcEnableLeConnectableHandsetGetTask());

    /* procedure started synchronously so indicate success */
    proc_start_cfm_fn(tws_topology_procedure_enable_le_connectable_handset, procedure_result_success);

    /* Start/Stop advertising */
    HandsetService_SetBleConnectable(params->enable);
} 


void TwsTopology_ProcedureEnableLeConnectableHandsetCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    twsTopProcEnableLeConnectableHandsetTaskData* td = TwsTopProcEnableLeConnectableHandsetGetTaskData();

    DEBUG_LOG("TwsTopology_ProcedureEnableLeConnectableHandsetCancel");

    /* There is no way to cancel the previous call to HandsetService_SetBleConnectable
       so we must wait for the next HANDSET_SERVICE_LE_CONNECTABLE_IND before calling the
       cancel cfm function. */
    td->complete_fn = NULL;
    td->cancel_fn = proc_cancel_cfm_fn;
}

