#include "tws_topology_private.h"
#include "tws_topology_config.h"
#include "tws_topology.h"
#include "tws_topology_procedure_handover.h"
#include "tws_topology_procedures.h"
#include "handover_profile.h"
#include <bt_device.h>
#include <logging.h>
#include <message.h>



/*! handover return status */
typedef enum
{
    handover_success = 0,
    handover_failed,
    handover_timedout
}handover_result_t;

/*! handover procedure task data */
typedef struct
{
    TaskData task;
    uint16 handover_retry_count;
    procedure_complete_func_t complete_fn;
} twsTopProcHandoverTaskData;

/*! handover procedure message handler */
static void twsTopology_ProcHandoverHandleMessage(Task task, MessageId id, Message message);

twsTopProcHandoverTaskData twstop_proc_handover = {.task = twsTopology_ProcHandoverHandleMessage};

#define TwsTopProcHandoverGetTaskData()     (&twstop_proc_handover)
#define TwsTopProcHandoverGetTask()         (&twstop_proc_handover.task)

typedef enum
{
    /*! Internal message to retry the handover */
    TWS_TOP_PROC_HANDOVER_INTERNAL_RETRY,
} tws_top_proc_handover_internal_message_t;

void TwsTopology_ProcedureHandoverStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);

void TwsTopology_ProcedureHandoverCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_handover_fns = {
    TwsTopology_ProcedureHandoverStart,
    TwsTopology_ProcedureHandoverCancel,
};

static void twsTopology_ProcHandoverReset(void)
{
    twsTopProcHandoverTaskData* td = TwsTopProcHandoverGetTaskData();
    MessageCancelFirst(TwsTopProcHandoverGetTask(), TWS_TOP_PROC_HANDOVER_INTERNAL_RETRY);
    td->handover_retry_count = 0;
    HandoverProfile_ClientUnregister(TwsTopProcHandoverGetTask());
    memset(&TwsTopologyGetTaskData()->handover_info,0,sizeof(handover_data_t));
}

static handover_result_t twsTopology_ProcGetStatus(handover_profile_status_t status)
{
    handover_result_t result=handover_success;

    DEBUG_LOG("twsTopology_ProcGetStatus() Status: %d", status);

    /* Handle return status from procedure */
    switch (status)
    {
        case HANDOVER_PROFILE_STATUS_SUCCESS:
            /* Return success */
        break;

        case HANDOVER_PROFILE_STATUS_PEER_CONNECT_FAILED:
        case HANDOVER_PROFILE_STATUS_PEER_CONNECT_CANCELLED:
        case HANDOVER_PROFILE_STATUS_PEER_DISCONNECTED:
        case HANDOVER_PROFILE_STATUS_PEER_LINKLOSS:
        case HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE:
            result = handover_failed;
        break;

        case HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT:
        case HANDOVER_PROFILE_STATUS_HANDOVER_VETOED:
        {
            twsTopProcHandoverTaskData* td = TwsTopProcHandoverGetTaskData();
            td->handover_retry_count++;
            if(td->handover_retry_count >= TwsTopologyConfig_HandoverMaxRetryAttempts())
            {
                /* we have exhausted maximum number of handover retry attempts, flag failed event */
                td->handover_retry_count = 0;
                result = handover_failed;
            }
            else
            {
                result = handover_timedout;
            }
        }
        break;
        default:
        break;
    }

    return result;
}

static void twsTopology_ProcHandoverStart(void)
{
    twsTopProcHandoverTaskData* td = TwsTopProcHandoverGetTaskData();
    handover_profile_status_t handover_status = HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE;
    bdaddr address;
    tp_bdaddr vm_addr;

    if(appDeviceIsHandsetConnected() && appDeviceGetHandsetBdAddr(&address))
    {
        DEBUG_LOG("twsTopology_ProcHandoverStart() Started");
        BdaddrTpFromBredrBdaddr(&vm_addr, &address);
        handover_status = HandoverProfile_Handover(&vm_addr);
    }

    switch(twsTopology_ProcGetStatus(handover_status))
    {
        case handover_success:
        {
            DEBUG_LOG("twsTopology_ProcHandoverStart() Success ");
            twsTopology_ProcHandoverReset();
            Procedures_DelayedCompleteCfmCallback(td->complete_fn, tws_topology_procedure_handover,procedure_result_success);
        }
        break;

        case handover_failed:
        {
            DEBUG_LOG("twsTopology_ProcHandoverStart() Failed ");
            twsTopology_ProcHandoverReset();
            Procedures_DelayedCompleteCfmCallback(td->complete_fn, tws_topology_procedure_handover,procedure_result_failed);
        }
        break;

        case handover_timedout:
        {
            /* Retry handover */
            DEBUG_LOG("twsTopology_ProcHandoverStart() Timedout, retry handover ");
            MessageSendLater(TwsTopProcHandoverGetTask(), TWS_TOP_PROC_HANDOVER_INTERNAL_RETRY, NULL, TwsTopologyConfig_HandoverRetryTimeoutMs());
        }
        break;
    }
}

static void twsTopology_ProcHandoverHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    DEBUG_LOG("twsTopology_ProcHandoverHandleMessage() Recieved id: 0x%x(%d)", id, id);
    switch (id)
    {
        case TWS_TOP_PROC_HANDOVER_INTERNAL_RETRY:
        {
            twsTopology_ProcHandoverStart();
        }
        break;

        default:
        break;
    }
}


void TwsTopology_ProcedureHandoverStart(Task result_task,
                                                  procedure_start_cfm_func_t proc_start_cfm_fn,
                                                  procedure_complete_func_t proc_complete_fn,
                                                  Message goal_data)
{
    DEBUG_LOG("TwsTopology_ProcedureHandOverStart");
    UNUSED(result_task);
    UNUSED(goal_data);

    twsTopProcHandoverTaskData* td = TwsTopProcHandoverGetTaskData();
    td->complete_fn = proc_complete_fn;
    td->handover_retry_count = 0;

    /* procedure started synchronously so indicate success */
    proc_start_cfm_fn(tws_topology_procedure_handover, procedure_result_success);

    twsTopology_ProcHandoverStart();
}

void TwsTopology_ProcedureHandoverCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureHandOverCancel");

    twsTopology_ProcHandoverReset();
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_handover, procedure_result_success);
}
