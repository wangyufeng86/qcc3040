/*!
\copyright  Copyright (c) 2017 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       device_upgrade.c
\brief      Application upgrade management.

Over the air upgrade is managed from this file. Interaction
with the headset software is required as well as support from
the \b upgrade and \b gaia VM libraries.

This is a minimal implementation of upgrade.
*/

#ifdef INCLUDE_DFU

#include "device_upgrade.h"

#include "init.h"
#include "adk_log.h"
#include "phy_state.h"

#include <charger_monitor.h>

#include <vmal.h>
#include <panic.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <upgrade.h>
#include <ps.h>


/*!< Task information for UPGRADE support */
upgradeTaskData app_upgrade;

/*! Identifiers for messages used internally by the Upgrade module */
typedef enum device_upgrade_internal_messages
{
    AV_UPGRADE_INTERNAL_UNUSED = INTERNAL_MESSAGE_BASE + 0x80,
};

/* The factory-set upgrade version and PS config version.

   After a successful upgrade the values from the upgrade header
   will be written to the upgrade PS key and used in future.
*/
static const upgrade_version earbud_upgrade_init_version = { 1, 0 };  /* Values should come from config file */
static const uint16 earbud_upgrade_init_config_version = 1;

/* The upgrade libraries use of partitions is not relevant to the
   partitions as used on devices targetted by this application.

   As it is not possible to pass 0 partitions in the Init function
   use a simple entry */
static const UPGRADE_UPGRADABLE_PARTITION_T logicalPartitions[]
                    = {UPGRADE_PARTITION_SINGLE(0x1000,DFU)
                      };

/*! Maximum size of buffer used to hold the variant string
    supplied by the application. 6 chars, plus NULL terminator */
#define VARIANT_BUFFER_SIZE (7)

static void appUpgradeMessageHandler(Task task, MessageId id, Message message);


static void appUpgradeNotifyStartedNeedConfirm(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(UpgradeGetClientList()), APP_UPGRADE_REQUESTED_TO_CONFIRM);
}


static void appUpgradeNotifyStartedWithInProgress(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(UpgradeGetClientList()), APP_UPGRADE_REQUESTED_IN_PROGRESS);
}


static void appUpgradeNotifyActivity(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(UpgradeGetClientList()), APP_UPGRADE_ACTIVITY);
}


static void appUpgradeNotifyStart(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(UpgradeGetClientList()), APP_UPGRADE_STARTED);
}


static void appUpgradeNotifyCompleted(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(UpgradeGetClientList()), APP_UPGRADE_COMPLETED);
}

static void appUpgradeNotifyAbort(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(UpgradeGetClientList()), APP_UPGRADE_CLEANUP_ON_ABORT);
}

/*************************************************************************
    Provide the logical partition map.

    For earbuds this is initially hard coded, but may come from other
    storage in time.
*/
static void appUpgradeGetLogicalPartitions(const UPGRADE_UPGRADABLE_PARTITION_T **partitions, uint16 *count)
{
    uint16 num_partitions = sizeof(logicalPartitions)/sizeof(logicalPartitions[0]);
    *partitions = logicalPartitions;
    *count = num_partitions;
}

/*************************************************************************
    Get the variant Id from the firmware and convert it into a variant
    string that can be passed to UpgradeInit.

    This function allocates a buffer for the string which must be freed
    after the call to UpgradeInit.
*/
static void appUpgradeGetVariant(char *variant, size_t length)
{
    int i = 0;
    char chr;
    uint32 product_id;

    PanicFalse(length >= VARIANT_BUFFER_SIZE);

    product_id = VmalVmReadProductId();
    if (product_id == 0)
    {
        variant[0] = '\0';
        return;
    }

    /* The product Id is encoded as two ascii chars + 4 integers in BCD format. */

    /* The ascii chars may be undefined or invalid (e.g. '\0').
       If so, don't include them in the variant string. */
    chr = (char)((product_id >> 8) & 0xFF);
    if (isalnum(chr))
        variant[i++] = chr;

    chr = (char)(product_id & 0xFF);
    if (isalnum(chr))
        variant[i++] = chr;

    sprintf(&variant[i], "%04X", ((uint16)((product_id >> 16) & 0xFFFF)));
}


/********************  PUBLIC FUNCTIONS  **************************/


bool appUpgradeEarlyInit(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("appUpgradeEarlyInit");

    TaskList_InitialiseWithCapacity(UpgradeGetClientList(), THE_UPGRADE_CLIENT_LIST_INIT_CAPACITY);

    return TRUE;
}

/*! Initialisation point for the over the air support in the upgrade library.
 *
 */
bool appUpgradeInit(Task init_task)
{
    upgradeTaskData *the_upgrade=UpgradeGetTaskData();
    uint16 num_partitions;
    const UPGRADE_UPGRADABLE_PARTITION_T *logical_partitions;
    char variant[VARIANT_BUFFER_SIZE];

    the_upgrade->upgrade_task.handler = appUpgradeMessageHandler;

    appUpgradeGetVariant(variant, sizeof(variant));

    appUpgradeGetLogicalPartitions(&logical_partitions, &num_partitions);

    /* Allow storage of info at end of (SINK_UPGRADE_CONTEXT_KEY) */
    UpgradeInit(UpgradeGetTask(), EARBUD_UPGRADE_CONTEXT_KEY, EARBUD_UPGRADE_LIBRARY_CONTEXT_OFFSET,
            logical_partitions,
            num_partitions,
            UPGRADE_INIT_POWER_MANAGEMENT,
            variant,
            upgrade_perm_always_ask,
            &earbud_upgrade_init_version,
            earbud_upgrade_init_config_version);

    Init_SetInitTask(init_task);
    return TRUE;
}


bool appUpgradeHandleSystemMessages(MessageId id, Message message, bool already_handled)
{
    switch (id)
    {
        case MESSAGE_IMAGE_UPGRADE_ERASE_STATUS:
        case MESSAGE_IMAGE_UPGRADE_COPY_STATUS:
        case MESSAGE_IMAGE_UPGRADE_AUDIO_STATUS:
        case MESSAGE_IMAGE_UPGRADE_HASH_ALL_SECTIONS_UPDATE_STATUS:
        {
            Task upg = UpgradeGetTask();

            upg->handler(upg, id, message);
            return TRUE;
        }
    }
    return already_handled;
}

static void appUpgradeForwardInitCfm(const UPGRADE_INIT_CFM_T *cfm)
{
    UPGRADE_INIT_CFM_T *copy = PanicUnlessNew(UPGRADE_INIT_CFM_T);
    *copy = *cfm;

    MessageSend(Init_GetInitTask(), UPGRADE_INIT_CFM, copy);
}

static void appUpgradeHandleRestartedInd(const UPGRADE_RESTARTED_IND_T *restart)
{
    /* This needs to base its handling on the reason in the message,
       for instance upgrade_reconnect_not_required is a hint that errr,
       reconnect isn't a priority. */

    DEBUG_LOG("appUpgradeHandleRestartedInd 0x%x", restart->reason);
    switch (restart->reason)
    {
        case upgrade_reconnect_not_required:
            /* No need to reconnect, not even sure why we got this */
            break;

        case upgrade_reconnect_required_for_confirm:
            appUpgradeNotifyStartedNeedConfirm();
            break;

        case upgrade_reconnect_recommended_as_completed:
        case upgrade_reconnect_recommended_in_progress:
            appUpgradeNotifyStartedWithInProgress();
            break;
    }
}


static void appUpgradeHandleUpgradeStatusInd(const UPGRADE_STATUS_IND_T *sts)
{
    appUpgradeNotifyActivity();

    switch (sts->state)
    {
        case upgrade_state_idle:
            DEBUG_LOG("appUpgradeHandleUpgradeStatusInd. idle(%d)",sts->state);
            break;

        case upgrade_state_downloading:
            DEBUG_LOG("appUpgradeHandleUpgradeStatusInd. downloading(%d)",sts->state);
            break;

        case upgrade_state_commiting:
            DEBUG_LOG("appUpgradeHandleUpgradeStatusInd. commiting(%d)",sts->state);
            break;

        case upgrade_state_done:
            DEBUG_LOG("appUpgradeHandleUpgradeStatusInd. done(%d)",sts->state);
            appUpgradeNotifyCompleted();
            break;

        default:
            DEBUG_LOG("appUpgradeHandleUpgradeStatusInd. Unexpected state %d",sts->state);
            Panic();
            break;
    }
}

static void appUpgradeSwapImage(void)
{
    UpgradeImageSwap();
}

static void appUpgradeHandleUpgradeShutAudio(void)
{
    DEBUG_LOG("appUpgradeHandleUpgradeShutAudio");
    appUpgradeSwapImage();
}


static void appUpgradeHandleUpgradeCopyAudioImageOrSwap(void)
{
    DEBUG_LOG("appUpgradeHandleUpgradeCopyAudioImageOrSwap");
    appUpgradeSwapImage();
}

static void appUpgradeMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    DEBUG_LOG("appUpgradeMessageHandler. 0x%X (%d)",id,id);

    switch (id)
    {
            /* Message sent in response to UpgradeInit().
             * In this case we need to forward to the app to unblock initialisation.
             */
        case UPGRADE_INIT_CFM:
            {
                const UPGRADE_INIT_CFM_T *init_cfm = (const UPGRADE_INIT_CFM_T *)message;

                DEBUG_LOG("appUpgradeMessageHandler. UPGRADE_INIT_CFM %d (sts)",init_cfm->status);

                appUpgradeForwardInitCfm(init_cfm);
            }
            break;

            /* Message sent during initialisation of the upgrade library
                to let the VM application know that a restart has occurred
                and reconnection to a host may be required. */
        case UPGRADE_RESTARTED_IND:
            appUpgradeHandleRestartedInd((UPGRADE_RESTARTED_IND_T*)message);
            break;

            /* Message sent to application to request applying a downloaded upgrade.
                Note this may include a warm reboot of the device.
                Application must respond with UpgradeApplyResponse() */
        case UPGRADE_APPLY_IND:
            DEBUG_LOG("appUpgradeMessageHandler. UPGRADE_APPLY_IND saying now !");
            appUpgradeNotifyActivity();
            UpgradeApplyResponse(0);
            break;

            /* Message sent to application to request blocking the system for an extended
                period of time to erase serial flash partitions.
                Application must respond with UpgradeBlockingResponse() */
        case UPGRADE_BLOCKING_IND:
            DEBUG_LOG("appUpgradeMessageHandler. UPGRADE_BLOCKING_IND");
            appUpgradeNotifyActivity();
            UpgradeBlockingResponse(0);
            break;

            /* Message sent to application to indicate that blocking operation is finished */
        case UPGRADE_BLOCKING_IS_DONE_IND:
            DEBUG_LOG("appUpgradeMessageHandler. UPGRADE_BLOCKING_IS_DONE_IND");
            appUpgradeNotifyActivity();
            break;

            /* Message sent to application to inform of the current status of an upgrade. */
        case UPGRADE_STATUS_IND:
            appUpgradeHandleUpgradeStatusInd((const UPGRADE_STATUS_IND_T *)message);
            break;

            /* Message sent to application to request any audio to get shut */
        case UPGRADE_SHUT_AUDIO:
            appUpgradeHandleUpgradeShutAudio();
            break;

            /* Message sent to application set the audio busy flag and copy audio image */
        case UPRGADE_COPY_AUDIO_IMAGE_OR_SWAP:
            appUpgradeHandleUpgradeCopyAudioImageOrSwap();
            break;

            /* Message sent to application to reset the audio busy flag */
        case UPGRADE_AUDIO_COPY_FAILURE:
            DEBUG_LOG("appUpgradeMessageHandler. UPGRADE_AUDIO_COPY_FAILURE (not handled)");
            break;

            /* Message sent to application to inform that the actual upgrade has started */
        case UPGRADE_START_DATA_IND:
            DEBUG_LOG("appUpgradeMessageHandler. UPGRADE_START_DATA_IND");
            appUpgradeNotifyStart();
            break;

            /* Message sent to application to inform that the actual upgrade has ended */
        case UPGRADE_END_DATA_IND:
            DEBUG_LOG("appUpgradeMessageHandler. UPGRADE_END_DATA_IND %d (not handled)",
                        ((UPGRADE_END_DATA_IND_T*)message)->state);
            break;

            /* Message sent to application to inform for cleaning up DFU state variables on Abort */
        case UPGRADE_CLEANUP_ON_ABORT:
            DEBUG_LOG("appUpgradeMessageHandler. UPGRADE_CLEANUP_ON_ABORT");
            appUpgradeNotifyAbort();
            break;

        case MESSAGE_IMAGE_UPGRADE_ERASE_STATUS:
            DEBUG_LOG("appUpgradeMessageHandler. MESSAGE_IMAGE_UPGRADE_ERASE_STATUS");

            appUpgradeNotifyActivity();
            UpgradeEraseStatus(message);
            break;

        case MESSAGE_IMAGE_UPGRADE_COPY_STATUS:
            DEBUG_LOG("appUpgradeMessageHandler. MESSAGE_IMAGE_UPGRADE_COPY_STATUS");

            appUpgradeNotifyActivity();
            UpgradeCopyStatus(message);
            break;

        case MESSAGE_IMAGE_UPGRADE_HASH_ALL_SECTIONS_UPDATE_STATUS:
            DEBUG_LOG("appUpgradeMessageHandler. MESSAGE_IMAGE_UPGRADE_HASH_ALL_SECTIONS_UPDATE_STATUS");
            UpgradeHashAllSectionsUpdateStatus(message);
            break;
            /* Catch-all panic for unexpected messages */
        default:
            if (UPGRADE_UPSTREAM_MESSAGE_BASE <= id && id <  UPGRADE_UPSTREAM_MESSAGE_TOP)
            {
                DEBUG_LOG("appUpgradeMessageHandler. Unexpected upgrade library message 0x%x (%d)",id,id);
            }
            else
            {
                DEBUG_LOG("appUpgradeMessageHandler. Unexpected message 0x%x (%d)",id,id);
            }
            Panic();
            break;
    }

}


bool appUpgradeAllowUpgrades(bool allow)
{
    upgrade_status_t sts = (upgrade_status_t)-1;
    bool successful = FALSE;

    /* The Upgrade library API can panic very easily if UpgradeInit had
       not been called previously */
    if (Init_IsCompleted())
    {
         sts = UpgradePermit(allow ? upgrade_perm_assume_yes : upgrade_perm_no);
         successful = (sts == upgrade_status_success);
    }

    DEBUG_LOG("appUpgradeAllowUpgrades(%d) - success:%d (sts:%d)", allow, successful, sts);

    return successful;
}

void appUpgradeClientRegister(Task tsk)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(UpgradeGetClientList()), tsk);
}

void appUpgradeSetContext(app_upgrade_context_t context)
{
    uint16 actualLength = PsRetrieve(EARBUD_UPGRADE_CONTEXT_KEY, NULL, 0);
    if ((actualLength > 0) && (actualLength <= PSKEY_MAX_STORAGE_LENGTH))
    {
        uint16 keyCache[PSKEY_MAX_STORAGE_LENGTH];
        PsRetrieve(EARBUD_UPGRADE_CONTEXT_KEY, keyCache, actualLength);
        keyCache[APP_UPGRADE_CONTEXT_OFFSET] = (uint16) context;
        PsStore(EARBUD_UPGRADE_CONTEXT_KEY, keyCache, actualLength);
    }
}

app_upgrade_context_t appUpgradeGetContext(void)
{
    app_upgrade_context_t context = APP_UPGRADE_CONTEXT_UNUSED;
    uint16 actualLength = PsRetrieve(EARBUD_UPGRADE_CONTEXT_KEY, NULL, 0);
    if ((actualLength > 0) && (actualLength <= PSKEY_MAX_STORAGE_LENGTH))
    {
        uint16 keyCache[PSKEY_MAX_STORAGE_LENGTH];
        PsRetrieve(EARBUD_UPGRADE_CONTEXT_KEY, keyCache, actualLength);
        context = (app_upgrade_context_t) keyCache[APP_UPGRADE_CONTEXT_OFFSET];
    }

    return context;
}

/*! Abort the ongoing Upgrade if the device is disconnected from GAIA app */
void appUpgradeAbortDuringDeviceDisconnect(void)
{
    DEBUG_LOG("appUpgradeAbortDuringDeviceDisconnect()\n");
    UpgradeAbortDuringDeviceDisconnect();
}

bool appUpgradeIsDFUPrimary(void)
{
    /* While in DFU role, we need to know if EB is DFU primary or not.
     * Role can be found out from app state and also from DFU PSKEY.
     */
    bool is_primary_device;
    UpgradePeerGetDFUInfo(&is_primary_device, NULL);
    DEBUG_LOG("appUpgradeIsDFUPrimary: appUpgradeIsMainRolePrimary() %d, is_primary_device (read from PSKEY) %d",
                appUpgradeIsMainRolePrimary(), is_primary_device);
    return (appUpgradeIsMainRolePrimary() || is_primary_device);

}

bool appUpgradeIsOutofCaseDfu(void)
{
    /* For DFU, we need to know if it is out of case DFU so that primary/secondary
     * rules are also executed with DFU rules.
     * Primary Secondary rules are run if app state is not in-case DFU and
     * either phy state is out case or there is no in-case DFU pending.
     * No in case DFU pending condition is to make sure that in-case events
     * during out-of-case DFU also trigger these rules.
     */
    DEBUG_LOG("appUpgradeIsOutofCaseDfu: appUpgradeIsAppInCaseDFUState:%d, out-case:%d, InCase DFU Pending:%d",
               appUpgradeIsAppInCaseDFUState(), appPhyStateIsOutOfCase(), appUpgradeIsInCaseDfuPending());
    return (!appUpgradeIsAppInCaseDFUState() &&
            (appPhyStateIsOutOfCase() || !appUpgradeIsInCaseDfuPending()));
}

#endif /* INCLUDE_DFU */
