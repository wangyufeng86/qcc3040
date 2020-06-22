/****************************************************************************
Copyright (c) 2014 - 2015, 2020 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_sm.c

DESCRIPTION

NOTES

*/

#include <stdlib.h>

#include <print.h>
#include <boot.h>
#include <loader.h>
#include <ps.h>
#include <panic.h>

#include <upgrade.h>

#include "upgrade_ctx.h"
#include "upgrade_host_if_data.h"
#include "upgrade_partition_data.h"
#include "upgrade_psstore.h"
#include "upgrade_partitions.h"
#include "upgrade_partition_validation.h"
#include "upgrade_partitions.h"

#include "upgrade_sm.h"
#include "upgrade_msg_vm.h"
#include "upgrade_msg_host.h"
#include "upgrade_msg_fw.h"
#include "upgrade_msg_internal.h"
#include "upgrade_fw_if.h"
#include <logging.h">

/*
NAME
    UpgradeSMCallLoaderOrReboot

DESCRIPTION
    Process the required actions from UpgradeSMHandleValidated.

    Depending on the type of upgrade being performed either call
    the loader to execute a DFU from SQIF, or warm reboot to change
    the partitions in SQIF that are mounted.
    
    Called either immediately or once we receive permission from
    the VM application.
*/
void UpgradeSMCallLoaderOrReboot(void)
{
    if (UpgradePartitionDataIsDfuUpdate())
    {
        uint16 partition = UpgradePartitionDataGetDfuPartition();

        /* This will reboot into the loader to apply the dfu */
        LoaderPerformDfuFromSqif(partition);

        /* If we get here then the dfu file failed the initial
           validation checks and the device was not rebooted by
           the firmware. We still need to reboot to reconnect to
           the host though. */
        UpgradeCtxGetPSKeys()->loader_msg = UPGRADE_LOADER_MSG_INVALID;
        UpgradeSavePSKeys();
    }
    
    BootSetMode(BootGetMode());
}

/* This is the last state before reboot */
bool UpgradeSMHandleValidated(MessageId id, Message message)
{
    switch(id)
    {
    case UPGRADE_HOST_TRANSFER_COMPLETE_RES:
        {
            UPGRADE_HOST_TRANSFER_COMPLETE_RES_T *msg = (UPGRADE_HOST_TRANSFER_COMPLETE_RES_T *)message;
            if(msg->action == 0)
            {
                UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_POST_REBOOT;
                UpgradeSavePSKeys();
                PRINT(("P&R: UPGRADE_RESUME_POINT_POST_REBOOT saved\n"));

                DEBUG_LOG("UpgradeSMHandleValidated, move to completed state. id:%d action:%d", id, msg->action);
                UpgradeSendEndUpgradeDataInd(upgrade_end_state_complete);

                /*Can consider disconnecting streams here*/

                UpgradeSetToTryUpgrades();

                /* if we have permission, go ahead and call loader/reboot */
                if (UpgradeSMHavePermissionToProceed(UPGRADE_APPLY_IND))
                {
                    UpgradeSMCallLoaderOrReboot();
                }
            }
            else
            {
                DEBUG_LOG("UpgradeSMHandleValidated, move to abort state. id:%d action:%d", id, msg->action);
                UpgradeSendEndUpgradeDataInd(upgrade_end_state_abort);
                UpgradeSMMoveToState(UPGRADE_STATE_SYNC);
            }
        }
        break;

    case UPGRADE_HOST_IS_CSR_VALID_DONE_REQ:
        UpgradeHostIFDataSendShortMsg(UPGRADE_HOST_TRANSFER_COMPLETE_IND);
        break;

    /* application finally gave permission, warm reboot */
    case UPGRADE_INTERNAL_REBOOT:
        {
            UpgradeSMCallLoaderOrReboot();
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/*
NAME
    UpgradeSMAbort - Clean everything and go to the sync state.

DESCRIPTION
    Common handler for clearing up an upgrade after an abort
    and going back to a state that is ready for a new upgrade.
*/
bool UpgradeSMAbort(void)
{
    /* if we have permission erase immediately and return to the SYNC state
     * to start again if required */
    if (UpgradeSMHavePermissionToProceed(UPGRADE_BLOCKING_IND))
    {
        UpgradeSMErase();
        UpgradeSMSetState(UPGRADE_STATE_SYNC);
    }

    return FALSE;
}

uint16 UpgradeSMNewImageStatus(void)
{
    uint16 err = 0;
    if(UpgradePartitionDataIsDfuUpdate())
    {
        switch(UpgradeCtxGetPSKeys()->loader_msg)
        {
        case UPGRADE_LOADER_MSG_SUCCESS:
            break;

        case UPGRADE_LOADER_MSG_ERROR:
        case UPGRADE_LOADER_MSG_INVALID:
            err = UPGRADE_HOST_ERROR_LOADER_ERROR;
            break;

        default:
            err = UPGRADE_HOST_ERROR_MISSING_LOADER_MSG;
        }
    }
    else if(UpgradeCtxGetPSKeys()->loader_msg != UPGRADE_LOADER_MSG_NONE)
    {
        err = UPGRADE_HOST_ERROR_UNEXPECTED_LOADER_MSG;
    }
    return err;
}

/*
NAME
    UpgradeSMCheckEraseComplete

DESCRIPTION
    Indicate whether the erase has completed.
    Always returns TRUE for standard config as UpgradePartitionsEraseAllManaged
    is blocking in BlueCore.
*/
bool UpgradeSMCheckEraseComplete(void)
{
    return TRUE;
}

/*
NAME
    UpgradeSMActionOnValidated

DESCRIPTION
    Dummy function for bluecore
*/
void UpgradeSMActionOnValidated(void)
{
    return;
}

/*
NAME
    UpgradeSMHandleAudioDFU

DESCRIPTION
    Dummy function for bluecore, set PSKEY to  UPGRADE_RESUME_POINT_POST_REBOOT and proceed
*/
void UpgradeSMHandleAudioDFU(void)
{
    return;
}

