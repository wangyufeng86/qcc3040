/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.


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
#include <imageupgrade.h>

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
#include "upgrade_peer.h"

static uint8 is_validated = 0;

static void upgradeSmConfig_ResetValidation(void)
{
    PRINT(("upgradeSmConfig_ResetValidation()\n"));
    is_validated = 0;
}

/*
NAME
    IsValidatedToTrySwap - Ensures all data are validated before trying to swap image.

DESCRIPTION
    Uses an incremental flag to ensure that all parts of a DFU image have been
    copied and validated before trying to call the ImageUpgradeSwapTry() trap.
*/
static void IsValidatedToTrySwap(bool reset)
{
    if(reset)
    {
        upgradeSmConfig_ResetValidation();
        return;
    }
	
    PRINT(("IsValidatedToTrySwap() is_validated = %d\n", is_validated));
    switch(is_validated)
    {
    /* Last part of the DFU image has been copied and validated */
    case 0:
        PRINT(("All DFU images have been validated\n"));
        is_validated++;
        break;

    /* All part have been copied and validated */
    case 1:
        {
            PRINT(("IsValidatedToTrySwap(): Shutdown audio before calling ImageUpgradeSwapTry()\n"));

            /*
             * The audio needs to be shut down before calling the ImageUpgradeSwapTry trap.
             * This is applicable to audio SQIF or ROM, to avoid deadlocks in Apps P0, causing P0 to not stream audio data or process image swap request.
             */
            UpgradeApplyAudioShutDown();
        }
        break;

    default:
        return;
    }
}


/* This is the last state before reboot */
bool UpgradeSMHandleValidated(MessageId id, Message message)
{
    UpgradeCtx *ctx = UpgradeCtxGet();

    PRINT(("UpgradeSMHandleValidated(%d, %p)\n", id, message));

    switch(id)
    {
        
    case UPGRADE_INTERNAL_CONTINUE:

         /* Check if UPGRADE_HOST_IS_CSR_VALID_DONE_REQ message is received */
        if(ctx->isCsrValidDoneReqReceived)
        {
            /* If Peer DFU is supported, then start the DFU of Peer Device */
            if(UPGRADE_PEER_IS_SUPPORTED)
            {
                if(!UPGRADE_PEER_IS_STARTED)
                {
                    /*
                     * Setup Peer connection for DFU once image upgarde copy is
                     * completed successfully.
                     */
                    if(UpgradeCtxGet()->isImgUpgradeCopyDone)
                    {
                        if(UpgradeCtxGet()->ImgUpgradeCopyStatus)
                            UpgradeCtxSetPeerDFUStartStatus(UPGRADE_START_PEER_DFU);
                        else
                            break; /* No need to setup peer connection */
                    }
                    else
                    {
                        UpgradeCtxSetPeerDFUStartStatus(UPGRADE_DELAY_PEER_DFU);
                    }

                    if(UpgradePeerStartDfu()== FALSE)
                    {
                        /* TODO: An error has occured, fail the DFU */
                    }
                }
            }
            /* Send UPGRADE_HOST_TRANSFER_COMPLETE_IND once standalone upgrade
             * is done.
             */
            else
            {
                UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_TRANSFER_COMPLETE_IND);
            }
        }
        break;

    case UPGRADE_HOST_TRANSFER_COMPLETE_RES:
        {
            UPGRADE_HOST_TRANSFER_COMPLETE_RES_T *msg = (UPGRADE_HOST_TRANSFER_COMPLETE_RES_T *)message;
            PRINT(("UPGRADE_HOST_TRANSFER_COMPLETE_RES\n"));
            if(msg !=NULL && msg->action == 0)
            {
                /* Send msg to peer device to reboot first. if peer device is
                 * restarted already then move to next phase.
                 */
                if(UPGRADE_PEER_IS_SUPPORTED && !UPGRADE_PEER_IS_RESTARTED)
                {
                    UpgradePeerProcessHostMsg(UPGRADE_PEER_TRANSFER_COMPLETE_RES,
                                                UPGRADE_CONTINUE);
                }
                else
                {
                    UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_POST_REBOOT;
                    UpgradeSavePSKeys();
                    PRINT(("P&R: UPGRADE_RESUME_POINT_POST_REBOOT saved\n"));

                    UpgradeSendEndUpgradeDataInd(upgrade_end_state_complete);
                    /*Can consider disconnecting streams here*/

                    /* if we have permission, go ahead and call loader/reboot */
                    if (UpgradeSMHavePermissionToProceed(UPGRADE_APPLY_IND))
                    {
                        PRINT(("IsValidatedToTrySwap() in UPGRADE_HOST_TRANSFER_COMPLETE_RES\n"));
                        IsValidatedToTrySwap(FALSE);
                    }
                }
            }
            else
            {
                if(UPGRADE_PEER_IS_SUPPORTED && UPGRADE_PEER_IS_STARTED)
                {
                    /* Host has aborted th DFU, inform peer device as well */
                    UpgradePeerProcessHostMsg(UPGRADE_PEER_TRANSFER_COMPLETE_RES,
                                                UPGRADE_ABORT);
                }
                else
                {
                    UpgradeSendEndUpgradeDataInd(upgrade_end_state_abort);
                    IsValidatedToTrySwap(TRUE);
                    UpgradeSMMoveToState(UPGRADE_STATE_SYNC);
                }
            }
        }
        break;

    case UPGRADE_HOST_IS_CSR_VALID_DONE_REQ:
        {
            PRINT(("UPGRADE_HOST_IS_CSR_VALID_DONE_REQ\n"));

            /* If Peer DFU is supported, then start the DFU of Peer Device */
            if(UPGRADE_PEER_IS_SUPPORTED)
            {
                if(!UPGRADE_PEER_IS_STARTED)
                {
                    /*
                     * Setup Peer connection for DFU once image upgarde copy is
                     * completed successfully.
                     */
                    if(UpgradeCtxGet()->isImgUpgradeCopyDone)
                    {
                        if(UpgradeCtxGet()->ImgUpgradeCopyStatus)
                            UpgradeCtxSetPeerDFUStartStatus(UPGRADE_START_PEER_DFU);
                        else
                            break; /* No need to setup peer connection */
                    }
                    else
                    {
                        UpgradeCtxSetPeerDFUStartStatus(UPGRADE_DELAY_PEER_DFU);
                    }

                    if(UpgradePeerStartDfu()== FALSE)
                    {
                    /* TODO: An error has occured, failed the DFU */
                    }
                }
            }
            /* Send UPGRADE_HOST_TRANSFER_COMPLETE_IND later once upgrade is
             * done during standalone DFU.
             */
            else
            {
                UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_TRANSFER_COMPLETE_IND);
            }
        }
        break;

    case UPGRADE_HOST_TRANSFER_COMPLETE_IND:
        /* Receive the UPGRADE_HOST_TRANSFER_COMPLETE_IND message from Peer
         * device once the DFU data is successfully transferred and validated
         * in Peer device. Then, send UPGRADE_HOST_TRANSFER_COMPLETE_IND
         * to Host now.
         */
        {
            UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_TRANSFER_COMPLETE_IND);
            break;
        }

    /* application finally gave permission, warm reboot */
    case UPGRADE_INTERNAL_REBOOT:
        {
            PRINT(("IsValidatedToTrySwap() in UPGRADE_INTERNAL_REBOOT\n"));
            IsValidatedToTrySwap(FALSE);
        }
        break;

    case UPGRADE_VM_IMAGE_UPGRADE_COPY_SUCCESSFUL:
        PRINT(("UPGRADE_VM_IMAGE_UPGRADE_COPY_SUCCESSFUL\n"));
        /*
         * Try the images from the "other image bank" in all QSPI devices.
         * The apps p0 will initiate a warm reset.
         */
        PRINT(("IsValidatedToTrySwap() in UPGRADE_VM_IMAGE_UPGRADE_COPY_SUCCESSFUL\n"));
        IsValidatedToTrySwap(FALSE);
        break;

    case UPGRADE_VM_DFU_COPY_VALIDATION_SUCCESS:
        {
            PRINT(("ImageUpgradeSwapTry() in UPGRADE_VM_DFU_COPY_VALIDATION_SUCCESS\n"));
            ImageUpgradeSwapTry();
        }
        break;

    case UPGRADE_VM_AUDIO_DFU_FAILURE:
        UpgradeApplyAudioCopyFailed();
    case UPGRADE_VM_IMAGE_UPGRADE_COPY_FAILED:
        PRINT(("FAILED COPY\n"));
        UpgradeSMMoveToState(UPGRADE_STATE_SYNC);
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
    upgradeSmConfig_ResetValidation();

    /* If peer upgrade is supported then inform UpgradePeer lib as well */
    if(UPGRADE_PEER_IS_SUPPORTED && UPGRADE_PEER_IS_STARTED)
    {
        UpgradePeerProcessHostMsg(UPGRADE_PEER_ABORT_REQ, UPGRADE_ABORT);
    }
    /* if we have permission erase immediately and return to the SYNC state
     * to start again if required */
    if (UpgradeSMHavePermissionToProceed(UPGRADE_BLOCKING_IND))
    {
#ifdef MESSAGE_IMAGE_UPGRADE_COPY_STATUS
        /*
         * There may be non-blocking traps such as ImageUpgradeCopy in progress.
         * Call the ImageUpgradeAbortCommand() trap to abort any of those. It
         * will do no harm if there are no non-blocking traps in progress.
         */
        PRINT(("ImageUpgradeAbortCommand()\n"));
        ImageUpgradeAbortCommand();
#endif  /* MESSAGE_IMAGE_UPGRADE_COPY_STATUS */
        UpgradeSMErase();
        UpgradeSMSetState(UPGRADE_STATE_SYNC);
    }

    return TRUE;
}

uint16 UpgradeSMNewImageStatus(void)
{
    uint16 err = 0;
    bool result = ImageUpgradeSwapTryStatus();
    PRINT(("ImageUpgradeSwapTryStatus() returns %d\n", result));
    if (!result)
    {
        err = UPGRADE_HOST_ERROR_LOADER_ERROR;
    }
    return err;
}

/*
NAME
    UpgradeSMCheckEraseComplete

DESCRIPTION
    Indicate whether the erase has completed.
    Returns FALSE for CONFIG_HYDRACORE as UpgradePartitionsEraseAllManaged
    is non-blocking and completion is indicated by the
    MESSAGE_IMAGE_UPGRADE_ERASE_STATUS message.
*/
bool UpgradeSMCheckEraseComplete(void)
{
    if (UPGRADE_RESUME_POINT_ERASE != UpgradeCtxGetPSKeys()->upgrade_in_progress_key)
    {
        if(UpgradeCtxGet()->smState == UPGRADE_STATE_COMMIT)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return TRUE;
    }
}

/*
NAME
    UpgradeSMActionOnValidated

DESCRIPTION
    Calls ImageUpgradeCopy().
*/
void UpgradeSMActionOnValidated(void)
{
#ifdef MESSAGE_IMAGE_UPGRADE_COPY_STATUS
    PRINT(("ImageUpgradeCopy()\n"));
    ImageUpgradeCopy();
#endif  /* MESSAGE_IMAGE_UPGRADE_COPY_STATUS */
}

/*
NAME
    UpgradeSMHandleAudioDFU

DESCRIPTION
    Calls UpgradeSMHandleAudioImageCopy().
*/
void UpgradeSMHandleAudioDFU(void)
{
#ifdef MESSAGE_IMAGE_UPGRADE_AUDIO_STATUS
    PRINT(("ImageUpgradeAudio()\n"));
    ImageUpgradeAudio();
#endif
}

