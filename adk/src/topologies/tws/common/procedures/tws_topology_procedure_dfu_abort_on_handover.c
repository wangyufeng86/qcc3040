/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Procedure to abort DFU when DFU is done out of case and Handover
            kicks in.
*/

#include "tws_topology_procedure_dfu_abort_on_handover.h"
#include "tws_topology_procedures.h"
#include "upgrade.h"
#include "device_upgrade.h"
#include <logging.h>
#ifdef HANDOVER_DFU_ABORT_WITHOUT_ERASE
/* ToDo: Check if the include dependency be burried under device_upgrade.h */
#include "upgrade_msg_host.h"
#endif

#ifndef HANDOVER_DFU_ABORT_WITHOUT_ERASE

void TwsTopology_ProcedureDfuAbortOnHandoverStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedureDfuAbortOnHandoverCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_abort_dfu_fns = {
    TwsTopology_ProcedureDfuAbortOnHandoverStart,
    TwsTopology_ProcedureDfuAbortOnHandoverCancel,
};

void TwsTopology_ProcedureDfuAbortOnHandoverStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    UNUSED(goal_data);
    UNUSED(result_task);

    DEBUG_LOG("TwsTopology_ProcedureDfuAbortOnHandoverStart");

    /* procedure started synchronously, confirm start now */
    proc_start_cfm_fn(tws_topology_procedure_dfu_abort_on_handover, procedure_result_success);

    /*Abort DFU when DFU is done out of case and Handover kicks in. */
    if (UpgradeIsInProgress())
    {
        DEBUG_LOG("TwsTopology_ProcedureDfuAbortOnHandoverStart Abort DFU.");
        /*
         * ToDo: Need to unify abort DFU handling so that it appropriately
         *       resets and cleanup the aborted upgrade seesion.
         *       Currently UpgradeHandleAbortDuringUpgrade() does error
         *       reporting and relies on the Host triggering the required
         *       cleanup procedure.
         */
        if(!UpgradeIsAborting())
            UpgradeHandleAbortDuringUpgrade();

        /*
         * This is a device triggered abort. Since its a defined abort,
         * it can switch to autonomous mode so that the graceful cleanup
         * especially erase on abort takes place.
         */
        UpgradePermit(upgrade_perm_assume_yes);
        /*
         * Since upgrade is in progess, peer upgrade may or may not have started
         * Based on the peer upgrade, the upgrade state machine appropriately
         * trigger DFU abort on the peer, as applicable.
         */
        UpgradeAbortDuringDeviceDisconnect();

        appUpgradeSetDfuAbortOnHandoverState(TRUE);
    }

    /* procedure completed synchronously so indicate completed already */
    Procedures_DelayedCompleteCfmCallback(proc_complete_fn, tws_topology_procedure_dfu_abort_on_handover, procedure_result_success);
}

void TwsTopology_ProcedureDfuAbortOnHandoverCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureDfuAbortOnHandoverCancel");

    /* nothing to cancel, just return success to keep goal engine happy */
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_dfu_abort_on_handover, procedure_result_success);
}

#else

/*! handover procedure task data */
typedef struct
{
    TaskData task;
    uint16 retry_count;
    procedure_complete_func_t complete_fn;
} twsTopProcDfuAbortOnHandoverTaskData;

/*! handover procedure message handler */
static void twsTopology_ProcDfuAbortOnHandoverHandleMessage(Task task, MessageId id, Message message);

twsTopProcDfuAbortOnHandoverTaskData twstop_proc_DfuAbortOnHandover = {.task = twsTopology_ProcDfuAbortOnHandoverHandleMessage};

#define TwsTopProcDfuAbortOnHandoverGetTaskData()     (&twstop_proc_DfuAbortOnHandover)
#define TwsTopProcDfuAbortOnHandoverGetTask()         (&twstop_proc_DfuAbortOnHandover.task)

/*! This is the poll interval to check if device initated abort is complete.
    Used in conjunction with TwsTopProcDfuAbortOnHandover_DFUAbortPollAttempts(). */
#define TwsTopProcDfuAbortOnHandover_DFUAbortPollPeriodMs()   (200)

/*! This is the poll attempts to check if device initated abort is complete.
    Used in conjunction with TwsTopProcDfuAbortOnHandover_DFUAbortPollPeriodMs(). */
#define TwsTopProcDfuAbortOnHandover_DFUAbortPollAttempts()   (5)

typedef enum
{
    /*! Internal message to await arrival of response to error notification to Host */
    TWS_TOP_PROC_DFU_ABORT_ON_HANDOVER_INTERNAL_RETRY,
} tws_top_proc_handover_internal_message_t;

void TwsTopology_ProcedureDfuAbortOnHandoverStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedureDfuAbortOnHandoverCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_abort_dfu_fns = {
    TwsTopology_ProcedureDfuAbortOnHandoverStart,
    TwsTopology_ProcedureDfuAbortOnHandoverCancel,
};

static void twsTopology_ProcDfuAbortOnHandoverReset(void)
{
    twsTopProcDfuAbortOnHandoverTaskData* td = TwsTopProcDfuAbortOnHandoverGetTaskData();
    MessageCancelFirst(TwsTopProcDfuAbortOnHandoverGetTask(),
                            TWS_TOP_PROC_DFU_ABORT_ON_HANDOVER_INTERNAL_RETRY);
    td->retry_count = 0;
    UpgradeSetHandoverDFUAbortState(handover_upgrade_abort_none);
}

void TwsTopology_ProcedureDfuAbortOnHandoverStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    UNUSED(goal_data);
    UNUSED(result_task);
    twsTopProcDfuAbortOnHandoverTaskData* td = TwsTopProcDfuAbortOnHandoverGetTaskData();
    td->complete_fn = proc_complete_fn;
    twsTopology_ProcDfuAbortOnHandoverReset();

    DEBUG_LOG("TwsTopology_ProcedureDfuAbortOnHandoverStart");

    /* procedure started synchronously, confirm start now */
    proc_start_cfm_fn(tws_topology_procedure_dfu_abort_on_handover, procedure_result_success);

    if (UpgradeIsInProgress() && !UpgradeIsAborting())
    {
        DEBUG_LOG("TwsTopology_ProcedureDfuAbortOnHandoverStart report to Host DFU is aborted.");

        /* Mark this abort as a Handover triggered DFU abort. */
        appUpgradeSetDfuAbortOnHandoverState(TRUE);

        /*
         * Error notify to Host and abort the Secondary if upgrade data was
         * being relayed from Primary to Secondary.
         */
        UpgradeSendDFUAbortErrInd(UPGRADE_HOST_ERROR_HANDOVER_DFU_ABORT);

        UpgradeSetHandoverDFUAbortState(handover_upgrade_abort_errorwarn_ind_sent);

        MessageSendLater(TwsTopProcDfuAbortOnHandoverGetTask(),
                        TWS_TOP_PROC_DFU_ABORT_ON_HANDOVER_INTERNAL_RETRY,
                        NULL, TwsTopProcDfuAbortOnHandover_DFUAbortPollPeriodMs());
    }
    else
    {
        /* Procedure completed synchronously so indicate completed already */
        Procedures_DelayedCompleteCfmCallback(proc_complete_fn,
                                tws_topology_procedure_dfu_abort_on_handover,
                                procedure_result_success);
    }
}

static void twsTopology_ProcDfuAbortOnHandoverHandleMessage(Task task, MessageId id, Message message)
{
    twsTopProcDfuAbortOnHandoverTaskData* td = TwsTopProcDfuAbortOnHandoverGetTaskData();
    UNUSED(task);
    UNUSED(message);

    DEBUG_LOG("twsTopology_ProcDfuAbortOnHandoverHandleMessage() Recieved id: 0x%x(%d)", id, id);
    switch (id)
    {
        case TWS_TOP_PROC_DFU_ABORT_ON_HANDOVER_INTERNAL_RETRY:
        {
            DEBUG_LOG("twsTopology_ProcDfuAbortOnHandoverHandleMessage() abort state: %d", UpgradeGetHandoverDFUAbortState());
            if(UpgradeIsHandoverDFUAbortComplete() ||
                td->retry_count++ >= TwsTopProcDfuAbortOnHandover_DFUAbortPollAttempts())
            {
                twsTopology_ProcDfuAbortOnHandoverReset();
                Procedures_DelayedCompleteCfmCallback(td->complete_fn,
                                tws_topology_procedure_dfu_abort_on_handover,
                                procedure_result_success);
            }
            else
            {
                DEBUG_LOG("twsTopology_ProcDfuAbortOnHandoverHandleMessage() retry count: %d", td->retry_count);
                MessageSendLater(TwsTopProcDfuAbortOnHandoverGetTask(),
                            TWS_TOP_PROC_DFU_ABORT_ON_HANDOVER_INTERNAL_RETRY,
                            NULL, TwsTopProcDfuAbortOnHandover_DFUAbortPollPeriodMs());
            }
        }
        break;

        default:
        break;
    }
}

void TwsTopology_ProcedureDfuAbortOnHandoverCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureDfuAbortOnHandoverCancel");

    /* Reset the procedure specific state information. */
    twsTopology_ProcDfuAbortOnHandoverReset();
    /* Nothing to cancel, just return success to keep goal engine happy */
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn,
                                tws_topology_procedure_dfu_abort_on_handover,
                                procedure_result_success);
}

#endif

