/****************************************************************************
Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_peer.c

DESCRIPTION
    This file handles Upgrade Peer connection state machine and DFU file
    transfers.

NOTES

*/

#include "upgrade_peer_private.h"
#include "upgrade_msg_host.h"
#include "upgrade.h"
#include "upgrade_ctx.h"
#include "upgrade_sm.h"
#include <ps.h>
#include <logging.h>

#define SHORT_MSG_SIZE (sizeof(uint8) + sizeof(uint16))

/* PSKEYS are intentionally limited to 32 words to save stack. */
#define PSKEY_MAX_STORAGE_LENGTH    32

#define MAX_START_REQ_RETRAIL_NUM       (5)

#define HOST_MSG(x)     (x + UPGRADE_HOST_MSG_BASE)

UPGRADE_PEER_INFO_T *upgradePeerInfo = NULL;

/* This is used to store upgrade context when reboot happens */
typedef struct {
    uint16 length;
    uint16 ram_copy[PSKEY_MAX_STORAGE_LENGTH];
} FSTAB_PEER_COPY;

static void SendAbortReq(void);
static void SendConfirmationToPeer(upgrade_confirmation_type_t type,
                                   upgrade_action_status_t status);
static void SendTransferCompleteReq(upgrade_action_status_t status);
static void SendCommitCFM(upgrade_action_status_t status);
static void SendInProgressRes(upgrade_action_status_t status);
static void SendPeerData(uint8 *data, uint16 dataSize);
static void SendStartReq (void);
static void SendErrorConfirmation(void);
static void SendValidationDoneReq (void);
static void SendSyncReq (uint32 md5_checksum);
static void SendStartDataReq (void);
static UPGRADE_PEER_CTX_T *UpgradePeerCtxGet(void);
static void UpgradePeerCtxInit(void);
static void StopUpgrade(void);
static void SavePSKeys(void);

/**
 * Set resume point as provided by Secondary device.
 */
static void SetResumePoint(upgrade_peer_resume_point_t point)
{
    upgradePeerInfo->SmCtx->mResumePoint = point;
}

/**
 * Return the abort status of Peer DFU.
 */
bool UpgradePeerIsPeerDFUAborted(void)
{
    if(upgradePeerInfo != NULL)
    {
        return upgradePeerInfo ->is_dfu_aborted;
    }
    else
    {
        DEBUG_LOG("UpgradePeer: Invalid access of upgradePeerInfo ptr. Panic the app");
        Panic();
        /* To satisfy the compiler */
        return FALSE;
    }
}

/**
 * Abort Peer upgrade is case error occurs.
 */
static void AbortPeerDfu(void)
{
    DEBUG_LOG("UpgradePeer: Abort DFU\n");

    /* Once connection is establish, first send abort to peer device */
    if (upgradePeerInfo->SmCtx->isUpgrading)
    {
        SendAbortReq();
        upgradePeerInfo->SmCtx->isUpgrading = FALSE;
    }
    else
    {
        /* We are here because user has aborted upgrade and peer device
         * upgrade is not yet started. Send Link disconnetc request.
         */
        if(upgradePeerInfo->SmCtx->peerState == UPGRADE_PEER_STATE_SYNC)
        {
            StopUpgrade();
        }
    }
    /* Since the Peer DFU is aborted now, set this to TRUE*/
    upgradePeerInfo->is_dfu_aborted = TRUE;
}

/**
 * Primary device got confirmation from Host. Send this to secondary device
 * now.
 */
static void SendConfirmationToPeer(upgrade_confirmation_type_t type,
                                   upgrade_action_status_t status)
{
    DEBUG_LOG("UpgradePeer: Confirm to Peer %x, %x\n", type, status);

    switch (type)
    {
        case UPGRADE_TRANSFER_COMPLETE:
            if(status == UPGRADE_CONTINUE)
            {
                SendTransferCompleteReq(status);
            }
            else
            {
                AbortPeerDfu();
            }
            break;

        case UPGRADE_COMMIT:
            if(status == UPGRADE_CONTINUE)
            {
                UpgradePeerSetState(UPGRADE_PEER_STATE_COMMIT);
                SendCommitCFM(status);
            }
            else
            {
                AbortPeerDfu();
            }
            break;

        case UPGRADE_IN_PROGRESS:
            if(status == UPGRADE_CONTINUE)
            {
                SendInProgressRes(status);
            }
            else
            {
                AbortPeerDfu();
            }
            break;
        default:
            DEBUG_LOG("unhandled msg\n");
    }

    if (status == UPGRADE_ABORT)
    {
        AbortPeerDfu();
    }
}

/**
 * To continue the process this manager needs the listener to confirm it.
 *
 */
static void AskForConfirmation(upgrade_confirmation_type_t type)
{
    upgradePeerInfo->SmCtx->confirm_type = type;
    DEBUG_LOG("UpgradePeer: Ask Confirmation %x\n", type);

    switch (type)
    {
        case UPGRADE_TRANSFER_COMPLETE:
            /* Send message to UpgradeSm indicating TRANSFER_COMPLETE_IND */
            UpgradeHandleMsg(NULL, HOST_MSG(UPGRADE_PEER_TRANSFER_COMPLETE_IND), NULL);
            break;
        case UPGRADE_COMMIT:
            /* Send message to UpgradeSm */
            UpgradeCommitMsgFromUpgradePeer();
            break;
        case UPGRADE_IN_PROGRESS:
            /* Device is rebooted let inform Host to continue further.
             * Send message to UpgradeSm.
             */
            if(upgradePeerInfo->SmCtx->peerState ==
                               UPGRADE_PEER_STATE_RESTARTED_FOR_COMMIT)
            {
                UpgradePeerSetState(UPGRADE_PEER_STATE_COMMIT_HOST_CONTINUE);
                /* We can resume DFU only when primary device is rebooted */
                UpgradeHandleMsg(NULL, HOST_MSG(UPGRADE_PEER_SYNC_AFTER_REBOOT_REQ), NULL);
            }
            break;
        default:
            DEBUG_LOG("unhandled msg\n");
            break;
    }
}

/**
 * Destroy UpgradePeer context when upgrade process is aborted or completed.
 */
static void UpgradePeerCtxDestroy(void)
{
    /* Only free UpgradePeer SM context. This can be allocated again once
     * DFU process starts during same power cycle.
     * UpgradePeer Info is allocated only once during boot and never destroyed
     */
    if(upgradePeerInfo != NULL)
    {
        if(upgradePeerInfo->SmCtx != NULL)
        {
            upgradePeerInfo->SmCtx->confirm_type = UPGRADE_TRANSFER_COMPLETE;
            upgradePeerInfo->SmCtx->peerState = UPGRADE_PEER_STATE_SYNC;
            upgradePeerInfo->SmCtx->mResumePoint = UPGRADE_PEER_RESUME_POINT_START;
            if(upgradePeerInfo->SmCtx != NULL)
            {
                free(upgradePeerInfo->SmCtx);
                upgradePeerInfo->SmCtx = NULL;
            }
        }
    }
}

/**
 * To stop the upgrade process.
 */
static void StopUpgrade(void)
{
    upgradePeerInfo->SmCtx->isUpgrading = FALSE;
    MessageSend(upgradePeerInfo->appTask, UPGRADE_PEER_DISCONNECT_REQ, NULL);
    /* Clear Pskey to start next upgrade from fresh */
    memset(&upgradePeerInfo->UpgradePSKeys,0x0000,
               sizeof(&upgradePeerInfo->UpgradePSKeys) * sizeof(uint16));
    SavePSKeys();
    UpgradePeerCtxDestroy();
}

/**
 * To clean and set the upgradePeerInfo for next DFU
 */
static void CleanUpgradePeerCtx(void)
{
    upgradePeerInfo->SmCtx->isUpgrading = FALSE;
    /* Clear Pskey to start next upgrade from fresh */
    memset(&upgradePeerInfo->UpgradePSKeys,0x0000,
               sizeof(&upgradePeerInfo->UpgradePSKeys) * sizeof(uint16));
    SavePSKeys();
    UpgradePeerCtxDestroy();
}

/**
 * To reset the file transfer.
 */
static void UpgradePeerCtxSet(void)
{
    upgradePeerInfo->SmCtx->mStartAttempts = 0;
    upgradePeerInfo->SmCtx->mStartOffset = 0;
}

/**
 * Immediate answer to secondury device, data is the same as the received one.
 */
static void HandleErrorWarnRes(void)
{
    DEBUG_LOG("UpgradePeer: Handle Error Ind\n");
    SendErrorConfirmation();
}

/**
 * To send an UPGRADE_START_REQ message.
 */
static void SendStartReq (void)
{
    uint8* payload = NULL;
    uint16 byteIndex = 0;

    DEBUG_LOG("UpgradePeer: Start REQ\n");

    payload = PanicUnlessMalloc(UPGRADE_PEER_PACKET_HEADER);
    byteIndex += ByteUtilsSet1Byte(payload, byteIndex, UPGRADE_PEER_START_REQ);
    byteIndex += ByteUtilsSet2Bytes(payload, byteIndex, 0);

    SendPeerData(payload, byteIndex);
}

/****************************************************************************
NAME
    UpgradeHostIFClientSendData

DESCRIPTION
    Send a data packet to a connected upgrade client.

*/
static void UpgradePeerSendErrorMsg(upgrade_peer_status_t error)
{
    UpgradeErrorMsgFromUpgradePeer(error);
}

/**
 * This method is called when we received an UPGRADE_START_CFM message. This
 * method reads the message and starts the next step which is sending an
 * UPGRADE_START_DATA_REQ message, or aborts the upgrade depending on the
 * received message.
 */
static void ReceiveStartCFM(UPGRADE_PEER_START_CFM_T *data)
{
    DEBUG_LOG("UpgradePeer: Start CFM\n");

    // the packet has to have a content.
    if (data->common.length >= UPRAGE_HOST_START_CFM_DATA_LENGTH)
    {
        //noinspection IfCanBeSwitch
        if (data->status == UPGRADE_PEER_SUCCESS)
        {
            upgradePeerInfo->SmCtx->mStartAttempts = 0;
            UpgradePeerSetState(UPGRADE_PEER_STATE_DATA_READY);
            SendStartDataReq();
        }
        else if (data->status == UPGRADE_PEER_ERROR_APP_NOT_READY)
        {
            if (upgradePeerInfo->SmCtx->mStartAttempts < MAX_START_REQ_RETRAIL_NUM)
            {
                // device not ready we will ask it again.
                upgradePeerInfo->SmCtx->mStartAttempts++;
                MessageSendLater((Task)&upgradePeerInfo->myTask,
                                 INTERNAL_START_REQ_MSG, NULL, 2000);
            }
            else
            {
                upgradePeerInfo->SmCtx->mStartAttempts = 0;
                upgradePeerInfo->SmCtx->upgrade_status =
                                   UPGRADE_PEER_ERROR_IN_ERROR_STATE;
                UpgradePeerSendErrorMsg(upgradePeerInfo->SmCtx->upgrade_status);
            }
        }
        else
        {
            upgradePeerInfo->SmCtx->upgrade_status = UPGRADE_PEER_ERROR_IN_ERROR_STATE;
            UpgradePeerSendErrorMsg(upgradePeerInfo->SmCtx->upgrade_status);
        }
    }
    else {
        upgradePeerInfo->SmCtx->upgrade_status = UPGRADE_PEER_ERROR_IN_ERROR_STATE;
        UpgradePeerSendErrorMsg(upgradePeerInfo->SmCtx->upgrade_status);
    }
}

/**
 * To send a UPGRADE_SYNC_REQ message.
 */
static void SendSyncReq (uint32 md5_checksum)
{
    uint8* payload = NULL;
    uint16 byteIndex = 0;

    DEBUG_LOG("UpgradePeer: Sync REQ ox%x\n", md5_checksum);

    payload = PanicUnlessMalloc(UPGRADE_SYNC_CFM_DATA_LENGTH +
                                UPGRADE_PEER_PACKET_LENGTH);
    byteIndex += ByteUtilsSet1Byte(payload, byteIndex, UPGRADE_PEER_SYNC_REQ);
    byteIndex += ByteUtilsSet2Bytes(payload, byteIndex,
                                    UPGRADE_SYNC_CFM_DATA_LENGTH);
    byteIndex += ByteUtilsSet4Bytes(payload, byteIndex, md5_checksum);

    SendPeerData(payload, byteIndex);
}

/**
 * This method is called when we received an UPGRADE_SYNC_CFM message.
 * This method starts the next step which is sending an UPGRADE_START_REQ
 * message.
 */
static void ReceiveSyncCFM(UPGRADE_PEER_SYNC_CFM_T *update_cfm)
{
    DEBUG_LOG("UpgradePeer: Sync CFM\n");

    SetResumePoint(update_cfm->resume_point);
    UpgradePeerSetState(UPGRADE_PEER_STATE_READY);
    SendStartReq();
}

/**
 * To send an UPGRADE_DATA packet.
 */
static void SendDataToPeer (uint32 data_length, uint8 *data)
{
    uint16 byteIndex = 0;

    byteIndex += ByteUtilsSet1Byte(data, byteIndex, UPGRADE_PEER_DATA);
    byteIndex += ByteUtilsSet2Bytes(data, byteIndex, data_length +
                                    UPGRADE_DATA_MIN_DATA_LENGTH);
    byteIndex += ByteUtilsSet1Byte(data, byteIndex,
                                   upgradePeerInfo->SmCtx->wasLastPacket);

    SendPeerData(data, data_length + UPGRADE_PEER_PACKET_HEADER +
                                     UPGRADE_DATA_MIN_DATA_LENGTH);
}

static bool StartPeerData(uint32 dataLength, uint8 *packet, bool last_packet)
{
    PRINT(("UpgradePeer: Start Data %x, len %d\n", packet, dataLength));

    /* Set up parameters */
    if (packet == NULL)
    {
        upgradePeerInfo->SmCtx->upgrade_status = UPGRADE_PEER_ERROR_IN_ERROR_STATE;
        return FALSE;
    }

    upgradePeerInfo->SmCtx->wasLastPacket = last_packet;

    SendDataToPeer(dataLength, packet);

    if (upgradePeerInfo->SmCtx->wasLastPacket)
    {
        DEBUG_LOG("UpgradePeer: last packet\n");
        if (upgradePeerInfo->SmCtx->mResumePoint == UPGRADE_PEER_RESUME_POINT_START)
        {
            upgradePeerInfo->SmCtx->wasLastPacket = FALSE;
            SetResumePoint(UPGRADE_PEER_RESUME_POINT_PRE_VALIDATE);
            SendValidationDoneReq();
        }
    }
    return TRUE;
}

/**
 * This method is called when we received an UPGRADE_DATA_BYTES_REQ message.
 * We manage this packet and use it for the next step which is to upload the
 * file on the device using UPGRADE_DATA messages.
 */
static void ReceiveDataBytesREQ(UPGRADE_PEER_START_DATA_BYTES_REQ_T *data)
{
    uint8* payload = NULL;
    uint16 pkt_len;
    bool last_packet = FALSE;
    upgrade_peer_status_t error;
    /* For header and last packet information */
    uint8 dataPkt_hdr = UPGRADE_PEER_PACKET_HEADER +
                        UPGRADE_DATA_MIN_DATA_LENGTH;
    /* This will be updated as how much data device is sending */
    uint32 sentLength = 0;

    PRINT(("UpgradePeer: DATA Bytes Req\n"));

    /* Checking the data has the good length */
    if (data->common.length == UPGRADE_DATA_BYTES_REQ_DATA_LENGTH)
    {
        UpgradePeerSetState(UPGRADE_PEER_STATE_DATA_TRANSFER);

        pkt_len = BYTES_TO_WORDS(data->data_bytes + dataPkt_hdr);

        /* Allocate memory for data read from partition */
        payload = PanicUnlessMalloc(pkt_len*sizeof(uint16));
        error = UpgradePeerPartitionMoreData(&payload[dataPkt_hdr],
                                             &last_packet, data->data_bytes,
                                             &sentLength);

        /* Store data pointer for future purpose */
        upgradePeerInfo->SmCtx->file = payload;

        /* data has been read from partition, now send to peer device */
        if((error == UPGRADE_PEER_SUCCESS) &&
            StartPeerData(sentLength, payload, last_packet) == FALSE)
        {
            error = UPGRADE_PEER_ERROR_PARTITION_OPEN_FAILED;
        }
    }
    else
    {
        error = UPGRADE_PEER_ERROR_IN_ERROR_STATE;
    }

    if(error != UPGRADE_PEER_SUCCESS)
    {
        upgradePeerInfo->SmCtx->upgrade_status = error;
        UpgradePeerSendErrorMsg(upgradePeerInfo->SmCtx->upgrade_status);
    }
}

/**
 * To send an UPGRADE_IS_VALIDATION_DONE_REQ message.
 */
static void SendValidationDoneReq (void)
{
    uint8* payload = NULL;
    uint16 byteIndex = 0;

    DEBUG_LOG("UpgradePeer: Validation Done REQ\n");

    payload = PanicUnlessMalloc(UPGRADE_PEER_PACKET_HEADER);
    byteIndex += ByteUtilsSet1Byte(payload, byteIndex,
                                   UPGRADE_PEER_IS_VALIDATION_DONE_REQ);
    byteIndex += ByteUtilsSet2Bytes(payload, byteIndex, 0);

    SendPeerData(payload, byteIndex);
}

/**
 * This method is called when we received an UPGRADE_IS_VALIDATION_DONE_CFM
 * message. We manage this packet and use it for the next step which is to send
 * an UPGRADE_IS_VALIDATION_DONE_REQ.
 */
static void ReceiveValidationDoneCFM(UPGRADE_PEER_VERIFICATION_DONE_CFM_T *data)
{
    if ((data->common.length == UPGRADE_VAIDATION_DONE_CFM_DATA_LENGTH) &&
         data->delay_time > 0)
    {
        MessageSendLater((Task)&upgradePeerInfo->myTask,
                         INTERNAL_VALIDATION_DONE_MSG,
                         NULL, data->delay_time);
    }
    else
    {
        SendValidationDoneReq();
    }
}

/**
 * This method is called when we received an UPGRADE_TRANSFER_COMPLETE_IND
 * message. We manage this packet and use it for the next step which is to send
 * a validation to continue the process or to abort it temporally.
 * It will be done later.
 */
static void ReceiveTransferCompleteIND(void)
{
    DEBUG_LOG("UpgradePeer: Transfer Complete Ind\n");
    SetResumePoint(UPGRADE_PEER_RESUME_POINT_PRE_REBOOT);
    /* Send TRANSFER_COMPLETE_IND to host to get confirmation */
    AskForConfirmation(UPGRADE_TRANSFER_COMPLETE);
}

/**
 * This method is called when we received an UPGRADE_COMMIT_RES message.
 * We manage this packet and use it for the next step which is to send a
 * validation to continue the process or to abort it temporally.
 * It will be done later.
 */
static void ReceiveCommitREQ(void)
{
    DEBUG_LOG("UpgradePeer: Commit Req\n");
    SetResumePoint(UPGRADE_PEER_RESUME_POINT_COMMIT);
    AskForConfirmation(UPGRADE_COMMIT);
}

/**
 * This method is called when we received an UPGRADE_IN PROGRESS_IND message.
 */
static void ReceiveProgressIND(void)
{
    DEBUG_LOG("UpgradePeer: Progress Ind\n");
    AskForConfirmation(UPGRADE_IN_PROGRESS);
}

/**
 * This method is called when we received an UPGRADE_ABORT_CFM message after
 * we asked for an abort to the upgrade process.
 */
static void ReceiveAbortCFM(void)
{
    DEBUG_LOG("UpgradePeer: Abort CFM\n");
    StopUpgrade();
}

/****************************************************************************
NAME
    SendPeerData

DESCRIPTION
    Send a data packet to a connected upgrade client.

*/
static void SendPeerData(uint8 *data, uint16 dataSize)
{
    if(upgradePeerInfo->appTask != NULL)
    {
        UPGRADE_PEER_DATA_IND_T *dataInd;

        dataInd =(UPGRADE_PEER_DATA_IND_T *)PanicUnlessMalloc(
                                            sizeof(*dataInd) + dataSize - 1);

        PRINT(("UpgradePeer: Send Data %d\n", dataSize));

        ByteUtilsMemCpyToStream(dataInd->data, data, dataSize);

        free(data);

        dataInd->size_data = dataSize;
        dataInd->is_data_state = TRUE;
        MessageSend(upgradePeerInfo->appTask, UPGRADE_PEER_DATA_IND, dataInd);
    }
    else
    {
        free(data);
    }
}

/**
 * To send an UPGRADE_START_DATA_REQ message.
 */
static void SendStartDataReq (void)
{
    uint8* payload = NULL;
    uint16 byteIndex = 0;

    DEBUG_LOG("UpgradePeer: Start DATA Req\n");

    SetResumePoint(UPGRADE_PEER_RESUME_POINT_START);

    payload = PanicUnlessMalloc(UPGRADE_PEER_PACKET_HEADER);
    byteIndex += ByteUtilsSet1Byte(payload, byteIndex,
                                   UPGRADE_PEER_START_DATA_REQ);
    byteIndex += ByteUtilsSet2Bytes(payload, byteIndex, 0);

    SendPeerData(payload, byteIndex);
}

/**
 * To send an UPGRADE_TRANSFER_COMPLETE_RES packet.
 */
static void SendTransferCompleteReq(upgrade_action_status_t status)
{
    uint8* payload = NULL;
    uint16 byteIndex = 0;

    DEBUG_LOG("UpgradePeer: Transfer Complete RES\n");

    payload = PanicUnlessMalloc(UPGRADE_TRANSFER_COMPLETE_RES_DATA_LENGTH +
                                UPGRADE_PEER_PACKET_LENGTH);
    byteIndex += ByteUtilsSet1Byte(payload, byteIndex,
                                   UPGRADE_PEER_TRANSFER_COMPLETE_RES);
    byteIndex += ByteUtilsSet2Bytes(payload, byteIndex,
                                   UPGRADE_TRANSFER_COMPLETE_RES_DATA_LENGTH);
    byteIndex += ByteUtilsSet1Byte(payload, byteIndex, status);

    SendPeerData(payload, byteIndex);
}

/**
 * To send an UPGRADE_IN_PROGRESS_RES packet.
 */
static void SendInProgressRes(upgrade_action_status_t status)
{
    uint8* payload = NULL;
    uint16 byteIndex = 0;

    DEBUG_LOG("UpgradePeer: In Progress Res\n");

    payload = PanicUnlessMalloc(UPGRADE_IN_PROGRESS_DATA_LENGTH +
                                UPGRADE_PEER_PACKET_LENGTH);
    byteIndex += ByteUtilsSet1Byte(payload, byteIndex,
                                   UPGRADE_PEER_IN_PROGRESS_RES);
    byteIndex += ByteUtilsSet2Bytes(payload, byteIndex,
                                   UPGRADE_IN_PROGRESS_DATA_LENGTH);
    byteIndex += ByteUtilsSet1Byte(payload, byteIndex, status);

    SendPeerData(payload, byteIndex);
}

/**
 * To send an UPGRADE_COMMIT_CFM packet.
 */
static void SendCommitCFM(upgrade_action_status_t status)
{
    uint8* payload = NULL;
    uint16 byteIndex = 0;

    DEBUG_LOG("UpgradePeer: Commit CFM\n");

    payload = PanicUnlessMalloc(UPGRADE_COMMIT_CFM_DATA_LENGTH +
                                UPGRADE_PEER_PACKET_LENGTH);
    byteIndex += ByteUtilsSet1Byte(payload, byteIndex, UPGRADE_PEER_COMMIT_CFM);
    byteIndex += ByteUtilsSet2Bytes(payload, byteIndex,
                                    UPGRADE_COMMIT_CFM_DATA_LENGTH);
    byteIndex += ByteUtilsSet1Byte(payload, byteIndex, status);

    SendPeerData(payload, byteIndex);
}

/**
 * To send a message to abort the upgrade.
 */
static void SendAbortReq(void)
{
    uint8* payload = NULL;
    uint16 byteIndex = 0;

    DEBUG_LOG("UpgradePeer: Abort request\n");

    payload = PanicUnlessMalloc(UPGRADE_PEER_PACKET_HEADER);
    byteIndex += ByteUtilsSet1Byte(payload, byteIndex, UPGRADE_PEER_ABORT_REQ);
    byteIndex += ByteUtilsSet2Bytes(payload, byteIndex, 0);

    SendPeerData(payload, byteIndex);
}

/**
 * To send an UPGRADE_ERROR_WARN_RES packet.
 */
static void SendErrorConfirmation(void)
{
    uint8* payload = NULL;
    uint16 byteIndex = 0;

    DEBUG_LOG("UpgradePeer: Send Error IND\n");

    payload = PanicUnlessMalloc(UPGRADE_PEER_PACKET_HEADER);
    byteIndex += ByteUtilsSet1Byte(payload, byteIndex,
                                UPGRADE_PEER_ERROR_WARN_RES);
    byteIndex += ByteUtilsSet2Bytes(payload, byteIndex, 0);

    SendPeerData(payload, byteIndex);
}

static void HandlePeerAppMsg(uint8 *data)
{
    uint16 msgId;
    upgrade_peer_status_t error;
    UPGRADE_PEER_DATA_BYTES_REQ_T *msg;
    uint16 *scoFlag;
    msgId = ByteUtilsGet1ByteFromStream(data);

    PRINT(("UpgradePeer: Handle APP Msg 0x%x\n", msgId));

    switch(msgId)
    {
        case UPGRADE_PEER_SYNC_CFM:
            ReceiveSyncCFM((UPGRADE_PEER_SYNC_CFM_T *)data);
            break;

        case UPGRADE_PEER_START_CFM:
            ReceiveStartCFM((UPGRADE_PEER_START_CFM_T *)data);
            break;

        case UPGRADE_PEER_IS_VALIDATION_DONE_CFM:
            ReceiveValidationDoneCFM(
                            (UPGRADE_PEER_VERIFICATION_DONE_CFM_T *)data);
            break;

        case UPGRADE_PEER_ABORT_CFM:
            ReceiveAbortCFM();
            break;

        case UPGRADE_PEER_START_REQ:
            SendStartReq();
            break;

        case UPGRADE_PEER_DATA_BYTES_REQ:
#ifndef HOSTED_TEST_ENVIRONMENT
            scoFlag = UpgradeIsScoActive();
            if(*scoFlag)
            {
                DEBUG_LOG("Defer sending data as SCO IS ACTIVE in UPGRADE_PEER_DATA_BYTES_REQ");
                msg = (UPGRADE_PEER_DATA_BYTES_REQ_T *)PanicUnlessMalloc(sizeof(UPGRADE_PEER_DATA_BYTES_REQ_T));
                memcpy(msg, data, sizeof(UPGRADE_PEER_DATA_BYTES_REQ_T));
                MessageSendConditionally((Task)&upgradePeerInfo->myTask,INTERNAL_PEER_MSG, msg,UpgradeIsScoActive());
            }
            else
#endif
            {
                PRINT(("UPGRADE_PEER_DATA_BYTES_REQ"));
                ReceiveDataBytesREQ((UPGRADE_PEER_START_DATA_BYTES_REQ_T *)data);
            }
            break;

        case UPGRADE_PEER_COMMIT_REQ:
            UpgradePeerSetState(UPGRADE_PEER_STATE_COMMIT_CONFIRM);
            ReceiveCommitREQ();
            break;

        case UPGRADE_PEER_TRANSFER_COMPLETE_IND:
            UpgradePeerSetState(UPGRADE_PEER_STATE_VALIDATED);
            ReceiveTransferCompleteIND();
            break;

        case UPGRADE_PEER_COMPLETE_IND:
            /* Peer upgrade is finished let disconnect the peer connection */
            StopUpgrade();
            break;

        case UPGRADE_PEER_ERROR_WARN_IND:
            /* send Error message to Host */
            error = ByteUtilsGet1ByteFromStream(data +
                                                UPGRADE_PEER_PACKET_HEADER);
            UpgradePeerSendErrorMsg(error);
            break;

        case UPGRADE_PEER_IN_PROGRESS_IND:
            ReceiveProgressIND();
            break;

        default:
            DEBUG_LOG("unhandled msg\n");
    }
}

static void HandleLocalMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    PRINT(("UpgradePeer: Local Msg 0x%x\n", id));
    switch(id)
    {
        case INTERNAL_START_REQ_MSG:
            SendStartReq();
            break;

        case INTERNAL_VALIDATION_DONE_MSG:
            SendValidationDoneReq();
            break;

        case INTERNAL_PEER_MSG:
            HandlePeerAppMsg((uint8 *)message);
            break;

        default:
            DEBUG_LOG("unhandled msg\n");
    }
}

/****************************************************************************
NAME
    UpgradePeerApplicationReconnect

DESCRIPTION
    Check the upgrade status and decide if the application needs to consider
    restarting communication / UI so that it can connect to a host.

    If needed, builds and send an UPGRADE_PEER_RESTARTED_IND message and
    sends to the application task.

NOTE
    Considered implementing this as part of UpgradePeerInit() which also looks
    at the resume point information.
*/
static void UpgradePeerApplicationReconnect(void)
{
    DEBUG_LOG("UpgradePeer: Resume point after reboot 0x%x\n",
               upgradePeerInfo->UpgradePSKeys.upgradeResumePoint);

    switch(upgradePeerInfo->UpgradePSKeys.upgradeResumePoint)
    {
        case UPGRADE_PEER_RESUME_POINT_POST_REBOOT:

            /* We are here because reboot happens during upgrade process,
             * first reinit UpgradePeer Sm Context.
             */
            UpgradePeerCtxInit();
            upgradePeerInfo->SmCtx->mResumePoint =
                upgradePeerInfo->UpgradePSKeys.upgradeResumePoint;
            upgradePeerInfo->SmCtx->isUpgrading = TRUE;

            /* Primary device is rebooted, lets ask App to establish peer
             * connection as well.
             */
            UpgradePeerSetState(UPGRADE_PEER_STATE_RESTARTED_FOR_COMMIT);
            UpgradeSetPriRebootDone(FALSE);
            MessageSendConditionally(upgradePeerInfo->appTask,
                        UPGRADE_PEER_CONNECT_REQ, NULL, (uint16 *)UpgradeIsPriRebootDone());
        break;
        default:
            DEBUG_LOG("unhandled msg\n");
    }
}

/**
 * This method starts the upgrade process. It checks that there isn't already a
 * upgrade going on. It resets the manager to start the upgrade and sends the
 * UPGRADE_SYNC_REQ to start the process.
 * This method can dispatch an error object if the manager has not been able to
 * start the upgrade process. The possible errors are the following:
 * UPGRADE_IS_ALREADY_PROCESSING
 */
static bool StartUpgradePeerProcess(uint32 md5_checksum)
{
    UPGRADE_PEER_CTX_T *upgrade_peer = UpgradePeerCtxGet();
    DEBUG_LOG("UpgradePeer: Start Upgrade 0x%x\n", md5_checksum);
    if (!upgrade_peer->isUpgrading)
    {
        upgrade_peer->isUpgrading = TRUE;
        UpgradePeerCtxSet();
        SendSyncReq(md5_checksum);
        upgrade_peer->upgrade_status = UPGRADE_PEER_SUCCESS;
    }
    else
    {
        upgrade_peer->upgrade_status = UPGRADE_PEER_ERROR_UPDATE_FAILED;
    }

    return (upgrade_peer->upgrade_status == UPGRADE_PEER_SUCCESS);
}

/****************************************************************************
NAME
    UpgradePeerCtxGet

RETURNS
    Context of upgrade library
*/
static UPGRADE_PEER_CTX_T *UpgradePeerCtxGet(void)
{
    if(upgradePeerInfo->SmCtx == NULL)
    {
        DEBUG_LOG("UpgradeGetCtx: You shouldn't have done that\n");
        Panic();
    }

    return upgradePeerInfo->SmCtx;
}

/****************************************************************************
NAME
    UpgradePeerLoadPSStore  -  Load PSKEY on boot

DESCRIPTION
    Save the details of the PSKEY and offset that we were passed on
    initialisation, and retrieve the current values of the key.

    In the unlikely event of the storage not being found, we initialise
    our storage to 0x00 rather than panicking.
*/
static void UpgradePeerLoadPSStore(uint16 dataPskey,uint16 dataPskeyStart)
{
    union {
        uint16      keyCache[PSKEY_MAX_STORAGE_LENGTH];
        FSTAB_PEER_COPY  fstab;
        } stack_storage;
    uint16 lengthRead;

    upgradePeerInfo->upgrade_peer_pskey = dataPskey;
    upgradePeerInfo->upgrade_peer_pskeyoffset = dataPskeyStart;

    /* Worst case buffer is used, so confident we can read complete key
     * if it exists. If we limited to what it should be, then a longer key
     * would not be read due to insufficient buffer
     * Need to zero buffer used as the cache is on the stack.
     */
    memset(stack_storage.keyCache,0,sizeof(stack_storage.keyCache));
    lengthRead = PsRetrieve(dataPskey,stack_storage.keyCache,
                            PSKEY_MAX_STORAGE_LENGTH);
    if (lengthRead)
    {
        memcpy(&upgradePeerInfo->UpgradePSKeys,
               &stack_storage.keyCache[dataPskeyStart],
               UPGRADE_PEER_PSKEY_USAGE_LENGTH_WORDS*sizeof(uint16));
    }
    else
    {
        memset(&upgradePeerInfo->UpgradePSKeys,0x0000,
               sizeof(&upgradePeerInfo->UpgradePSKeys) * sizeof(uint16));
    }
}

static void SavePSKeys(void)
{
    uint16 keyCache[PSKEY_MAX_STORAGE_LENGTH];
    uint16 min_key_length = upgradePeerInfo->upgrade_peer_pskeyoffset +
                            UPGRADE_PEER_PSKEY_USAGE_LENGTH_WORDS;

    /* Find out how long the PSKEY is */
    uint16 actualLength = PsRetrieve(upgradePeerInfo->upgrade_peer_pskey,NULL,0);

    /* Clear the keyCache memory before it is used for reading and writing to
     * UPGRADE LIB PSKEY
     */
    memset(keyCache,0x0000,sizeof(keyCache));

    if (actualLength)
    {
        PsRetrieve(upgradePeerInfo->upgrade_peer_pskey,keyCache,actualLength);
    }
    else
    {
        if (upgradePeerInfo->upgrade_peer_pskeyoffset)
        {
            /* Initialise the portion of key before us */
            memset(keyCache,0x0000,sizeof(keyCache));
        }
        actualLength = min_key_length;
    }

    /* Correct for too short a key */
    if (actualLength < min_key_length)
    {
        actualLength = min_key_length;
    }

    memcpy(&keyCache[upgradePeerInfo->upgrade_peer_pskeyoffset],
           &upgradePeerInfo->UpgradePSKeys,
           UPGRADE_PEER_PSKEY_USAGE_LENGTH_WORDS*sizeof(uint16));
    PsStore(upgradePeerInfo->upgrade_peer_pskey,keyCache,actualLength);
}

/****************************************************************************
NAME
    UpgradePeerProcessHostMsg

DESCRIPTION
    Process message received from Host/UpgradeSm.

*/
void UpgradePeerProcessHostMsg(upgrade_peer_msg_t msgid,
                                 upgrade_action_status_t status)
{
    UPGRADE_PEER_CTX_T *upgrade_peer = upgradePeerInfo->SmCtx;
    DEBUG_LOG("UpgradePeer: Process Host Msg 0x%x\n", msgid);

    if(upgrade_peer == NULL)
    {
        /* This can happen only when Host sends DFU commands when process
         * is already aborted or primary device got rebooted.
         * Ideally this should not happen, and if it happens then inform
         * host that device is in error state.
         */
        DEBUG_LOG("UpgradePeer: Context is NULL, send error message \n");

        /* Establish l2cap connection with peer to initiate abort in local
         * device and send error message to Upgrade SM. Also, to initiate
         * state variables clearance in peer device for next DFU
         */
        MessageSend(upgradePeerInfo->appTask, UPGRADE_PEER_CONNECT_REQ, NULL);
        upgradePeerInfo->is_dfu_abort_trigerred = TRUE;
        /* Needed for initiating the abort after the l2cap connection is established */
        UpgradePeerCtxInit();
        return;
    }

    switch(msgid)
    {
        case UPGRADE_PEER_SYNC_REQ:
            SendSyncReq(upgradePeerInfo->md5_checksum);
            break;
        case UPGRADE_PEER_ERROR_WARN_RES:
            HandleErrorWarnRes();
            break;

        case UPGRADE_PEER_TRANSFER_COMPLETE_RES:
            SendConfirmationToPeer(upgrade_peer->confirm_type, status);
            break;

        case UPGRADE_PEER_IN_PROGRESS_RES:
            upgrade_peer->confirm_type = UPGRADE_IN_PROGRESS;
            UpgradePeerSetState(UPGRADE_PEER_STATE_COMMIT_VERIFICATION);
            SendConfirmationToPeer(upgrade_peer->confirm_type, status);
            break;

        case UPGRADE_PEER_COMMIT_CFM:
            SendConfirmationToPeer(upgrade_peer->confirm_type, status);
            break;
        case UPGRADE_PEER_ABORT_REQ:
            if(upgradePeerInfo->SmCtx->peerState == UPGRADE_PEER_STATE_ABORTING)
            {
                /* Peer Disconnection already occured. It's time to stop upgrade
                 */
                DEBUG_LOG("Upgrade_peer: StopUpgrade()");
                StopUpgrade();
            }
            else
            {
                /* Abort the DFU */
                AbortPeerDfu();
            }
            break;
        default:
            DEBUG_LOG("unhandled msg\n");
    }
}

/****************************************************************************
NAME
    UpgradePeerResumeUpgrade

DESCRIPTION
    resume the upgrade peer procedure.
    If an upgrade is processing, to resume it after a disconnection of the
    process, this method should be called to restart the existing running
    process.

*/
bool UpgradePeerResumeUpgrade(void)
{
    UPGRADE_PEER_CTX_T *upgrade_peer = UpgradePeerCtxGet();
    if (upgrade_peer->isUpgrading)
    {
        UpgradePeerCtxSet();
        SendSyncReq(upgradePeerInfo->md5_checksum);
    }

    return upgrade_peer->isUpgrading;
}

bool UpgradePeerIsSupported(void)
{
    return ((upgradePeerInfo != NULL) && upgradePeerInfo->is_primary_device);
}

bool UpgradePeerIsSecondary(void)
{
    return upgradePeerInfo && !upgradePeerInfo->is_primary_device;
}

static void UpgradePeerCtxInit(void)
{
    if(upgradePeerInfo != NULL)
    {
        UPGRADE_PEER_CTX_T *peerCtx;

        DEBUG_LOG("UpgradePeerCtx: Init\n");

        peerCtx = (UPGRADE_PEER_CTX_T *) PanicUnlessMalloc(sizeof(*peerCtx));
        memset(peerCtx, 0, sizeof(*peerCtx));

        upgradePeerInfo->SmCtx = peerCtx;

        UpgradePeerCtxSet();
        UpgradePeerParititonInit();
    }
}

/****************************************************************************
NAME
    UpgradePeerGetDFUInfo

DESCRIPTION
    DFU topology need to know DFU information for the device after reboot.
    Information needed are if the device was DFU primary or secondary and
    was device in DFU mode.
    This information might be required even before the upgrade peer context is created
    So creating the context just to read the information and then freeing it.
    If context is already created then just read the information from the context.
*/

void UpgradePeerGetDFUInfo(bool *is_primary_device, bool *is_dfu_mode)
{
    /* If upgradePeerInfo context is not yet created */
    if(upgradePeerInfo == NULL)
    {
        UPGRADE_PEER_INFO_T *peerInfo;
        peerInfo = (UPGRADE_PEER_INFO_T *) PanicUnlessMalloc(sizeof(*peerInfo));
        memset(peerInfo, 0, sizeof(*peerInfo));

        upgradePeerInfo = peerInfo;

        UpgradePeerLoadPSStore(EARBUD_UPGRADE_CONTEXT_KEY, EARBUD_UPGRADE_PEER_CONTEXT_OFFSET);

        if(is_primary_device != NULL)
        {
            *is_primary_device = !(upgradePeerInfo->UpgradePSKeys.is_secondary_device);
            DEBUG_LOG("UpgradePeer: UpgradePeerGetDFUInfo is_primary_device %d\n", *is_primary_device);
        }

        if(is_dfu_mode != NULL)
        {
            *is_dfu_mode = upgradePeerInfo->UpgradePSKeys.is_dfu_mode;
            DEBUG_LOG("UpgradePeer: UpgradePeerGetDFUInfo is_dfu_mode %d\n", *is_dfu_mode);
        }

        free(peerInfo);

        upgradePeerInfo = NULL;
    }
    else
    {
        if(is_primary_device != NULL)
        {
            *is_primary_device = !(upgradePeerInfo->UpgradePSKeys.is_secondary_device);
            DEBUG_LOG("UpgradePeer: UpgradePeerGetDFUInfo is_primary_device %d\n", *is_primary_device);
        }

        if(is_dfu_mode != NULL)
        {
            *is_dfu_mode = upgradePeerInfo->UpgradePSKeys.is_dfu_mode;
            DEBUG_LOG("UpgradePeer: UpgradePeerGetDFUInfo is_dfu_mode %d\n", *is_dfu_mode);
        }
    }
}

/****************************************************************************
 NAME
    UpgradePeerInit

 DESCRIPTION
    Perform initialisation for the upgrade library. This consists of fixed
    initialisation as well as taking account of the information provided
    by the application.
*/

void UpgradePeerInit(Task appTask, uint16 dataPskey,uint16 dataPskeyStart)
{
    UPGRADE_PEER_INFO_T *peerInfo;

    DEBUG_LOG("UpgradePeer: Init\n");

    peerInfo = (UPGRADE_PEER_INFO_T *) PanicUnlessMalloc(sizeof(*peerInfo));
    memset(peerInfo, 0, sizeof(*peerInfo));

    upgradePeerInfo = peerInfo;

    upgradePeerInfo->appTask = appTask;
    upgradePeerInfo->myTask.handler = HandleLocalMessage;

    upgradePeerInfo->is_primary_device = TRUE;

    UpgradePeerLoadPSStore(dataPskey,dataPskeyStart);

    MessageSend(upgradePeerInfo->appTask, UPGRADE_PEER_INIT_CFM, NULL);

    /* Device is  restarted in upgrade process, send connect request again */
    UpgradePeerApplicationReconnect();
}

/****************************************************************************
NAME
    UpgradePeerSetState

DESCRIPTION
    Set current state.
*/
void UpgradePeerSetState(upgrade_peer_state_t nextState)
{
    upgradePeerInfo->SmCtx->peerState = nextState;
}

/****************************************************************************
NAME
    UpgradePeerIsRestarted

DESCRIPTION
    After upgrade device is rebooted, check that is done or not.
*/
bool UpgradePeerIsRestarted(void)
{
    return ((upgradePeerInfo->SmCtx != NULL) &&
             (upgradePeerInfo->SmCtx->mResumePoint ==
                UPGRADE_PEER_RESUME_POINT_POST_REBOOT));
}

/****************************************************************************
NAME
    UpgradePeerIsCommited

DESCRIPTION
    Upgrade peer device has sent commit request or not.
*/
bool UpgradePeerIsCommited(void)
{
    return ((upgradePeerInfo->SmCtx != NULL) &&
             (upgradePeerInfo->SmCtx->peerState ==
                UPGRADE_PEER_STATE_COMMIT_CONFIRM));
}

/****************************************************************************
NAME
    UpgradePeerIsCommitContinue

DESCRIPTION
    Upgrade peer should be ready for commit after reboot.
*/
bool UpgradePeerIsCommitContinue(void)
{
    return (upgradePeerInfo->SmCtx->peerState ==
            UPGRADE_PEER_STATE_COMMIT_HOST_CONTINUE);
}

/****************************************************************************
NAME
    UpgradePeerIsStarted

DESCRIPTION
    Peer device upgrade procedure is started or not.
*/
bool UpgradePeerIsStarted(void)
{
    return (upgradePeerInfo->SmCtx != NULL);
}

/****************************************************************************
NAME
    UpgradePeerDeInit

DESCRIPTION
    Perform uninitialisation for the upgrade library once upgrade is done
*/
void UpgradePeerDeInit(void)
{
    DEBUG_LOG("UpgradePeer: DeInit\n");

    free(upgradePeerInfo->SmCtx);
    upgradePeerInfo->SmCtx = NULL;
}

void UpgradePeerStoreMd5(uint32 md5)
{
    if(upgradePeerInfo == NULL)
    {
        return;
    }

    upgradePeerInfo->md5_checksum = md5;
}

/****************************************************************************
NAME
    UpgradePeerStartDfu

DESCRIPTION
    Start peer device DFU procedure.

    If Upgrade Peer library has not been initialised do nothing.
*/
bool UpgradePeerStartDfu(void)
{
    if(upgradePeerInfo == NULL || upgradePeerInfo->appTask == NULL)
    {
        return FALSE;
    }

    DEBUG_LOG("UpgradePeer: DFU Started\n");

    /* Allocate UpgradePeer SM Context */
    UpgradePeerCtxInit();

    /* Peer DFU is starting,lets be in SYNC state */
    UpgradePeerSetState(UPGRADE_PEER_STATE_SYNC);

    /* Peer DFU is going to start, so it's not aborted yet*/
    upgradePeerInfo->is_dfu_aborted = FALSE;

    /* Peer DFU will start, once the image upgrade copy is completed */
    MessageSendConditionally(upgradePeerInfo->appTask, UPGRADE_PEER_CONNECT_REQ, NULL, (uint16 *)UpgradeCtxGetPeerDFUStartStatus());

    return TRUE;
}

bool UpgradePeerSetDeviceRolePrimary(bool is_primary)
{
    /* If upgradePeerInfo context is not yet created */
    if(upgradePeerInfo == NULL)
    {
        DEBUG_LOG("UpgradePeerSetDeviceRolePrimary upgradePeerInfo context is not yet created");
        return FALSE;
    }

    DEBUG_LOG("UpgradePeerSetDeviceRolePrimary is_primary %d\n", is_primary);
    upgradePeerInfo->is_primary_device = is_primary;
    upgradePeerInfo->UpgradePSKeys.is_secondary_device =
                                        !(upgradePeerInfo->is_primary_device);
    SavePSKeys();
    UpgradeSMHostRspSwap(is_primary);

    return TRUE;
}

void UpgradePeerSetDFUMode(bool is_dfu_mode)
{
    /* If upgradePeerInfo context is not yet created */
    if(upgradePeerInfo == NULL)
    {
        DEBUG_LOG("UpgradePeerSetDFUMode upgradePeerInfo context is not yet created, return");
        return;
    }

    DEBUG_LOG("UpgradePeerSetDFUMode is_dfu_mode %d", is_dfu_mode);
    upgradePeerInfo->UpgradePSKeys.is_dfu_mode = is_dfu_mode;
    SavePSKeys();
}

void UpgradePeerCancelDFU(void)
{
    MessageCancelAll(upgradePeerInfo->appTask, UPGRADE_PEER_CONNECT_REQ);
}

void UpgradePeerResetStateInfo(void)
{
    /* If upgradePeerInfo context is not yet created */
    if(upgradePeerInfo == NULL)
    {
        DEBUG_LOG("UpgradePeerSetDFUMode upgradePeerInfo context is not yet created, return");
        return;
    }

    upgradePeerInfo->is_dfu_aborted = FALSE;
    upgradePeerInfo->is_primary_device = FALSE;
}

/****************************************************************************
NAME
    UpgradePeerProcessDataRequest

DESCRIPTION
    Process a data packet from an Upgrade Peer client.

    If Upgrade Peer library has not been initialised do nothing.
*/
void UpgradePeerProcessDataRequest(upgrade_peer_app_msg_t Id, uint8 *data,
                                   uint16 dataSize)
{
    uint8 *peer_data = NULL;
    upgrade_peer_connect_state_t state;

    if (upgradePeerInfo->SmCtx == NULL)
        return;

    PRINT(("UpgradePeer: Process MsgId, Size 0x%x, %d\n", Id, dataSize));

    switch(Id)
    {
        case UPGRADE_PEER_CONNECT_CFM:
            state = ByteUtilsGet1ByteFromStream(data);
            DEBUG_LOG("Connect CFM state %d\n", state);
            if(state == UPGRADE_PEER_CONNECT_SUCCESS)
            {
                switch(upgradePeerInfo->SmCtx->peerState)
                {
                    case UPGRADE_PEER_STATE_SYNC:
                        StartUpgradePeerProcess(upgradePeerInfo->md5_checksum);
                    break;
                    case UPGRADE_PEER_STATE_CONNECT_FOR_REBOOT:
                    {
                        /* Create a msg payload of type UPGRADE_HOST_GENERIC_ACTION_T
                         * with action variable set as zero.
                         */
                        UPGRADE_HOST_GENERIC_ACTION_T msg = {0};

                        SetResumePoint(UPGRADE_PEER_RESUME_POINT_POST_REBOOT);
                        /* We are going to reboot save current state */
                        upgradePeerInfo->UpgradePSKeys.upgradeResumePoint =
                                        UPGRADE_PEER_RESUME_POINT_POST_REBOOT;
                        upgradePeerInfo->UpgradePSKeys.currentState =
                                               upgradePeerInfo->SmCtx->peerState;
                        SavePSKeys();

                        UpgradeHandleMsg(NULL,
                                         HOST_MSG(UPGRADE_PEER_TRANSFER_COMPLETE_RES),
                                         &msg);
                    }
                    break;
                    case UPGRADE_PEER_STATE_RESTARTED_FOR_COMMIT:
                        UpgradePeerSetState(
                                   UPGRADE_PEER_STATE_COMMIT_HOST_CONTINUE);
                        UpgradeHandleMsg(NULL,
                                         HOST_MSG(UPGRADE_PEER_SYNC_AFTER_REBOOT_REQ),
                                         NULL);
                    break;
                    default:
                    {
                        /* We are here because an l2cap connection is created
                         * due to either Primary or Secondary reset during
                         * Primary to Secondary DFU file transfer. Now abort
                         * the DFU and clear the state variables for both the 
                         * devices.
                         */
                        if(upgradePeerInfo->is_dfu_abort_trigerred)
                        {
                             DEBUG_LOG("UpgradePeer: SendErrorConfirmation\n");
                             /* Send Error confirmation to Peer */
                             SendErrorConfirmation();
                             /* Send Abort request to Upgrade SM*/
                             UpgradePeerSendErrorMsg(UPGRADE_PEER_ERROR_UPDATE_FAILED);
                             /* Since DFU abort is trigeered now, clear the flag */
                             upgradePeerInfo->is_dfu_abort_trigerred = FALSE;
                             /* Set the state to aborting */
                             UpgradePeerSetState(UPGRADE_PEER_STATE_ABORTING);
                             /* L2cap connection is no longer needed, so disconnect */
                             MessageSend(upgradePeerInfo->appTask, UPGRADE_PEER_DISCONNECT_REQ, NULL);
                        }
                        else
                            DEBUG_LOG("unhandled msg\n");
                    }
                }
            }
            else
            {
                upgradePeerInfo->SmCtx->upgrade_status =
                                      UPGRADE_PEER_ERROR_APP_NOT_READY;
                UpgradePeerSendErrorMsg(upgradePeerInfo->SmCtx->upgrade_status);
            }
            break;
        case UPGRADE_PEER_DISCONNECT_IND:
            if(upgradePeerInfo->SmCtx->peerState == UPGRADE_PEER_STATE_VALIDATED)
            {
                /* This might have come because of peer reboot after validation
                 * and connection lost. let APP knows to try connection again.
                 */
                MessageSend(upgradePeerInfo->appTask,
                            UPGRADE_PEER_CONNECT_REQ, NULL);
                UpgradePeerSetState(UPGRADE_PEER_STATE_CONNECT_FOR_REBOOT);
            }
            else if(upgradePeerInfo->SmCtx->peerState == UPGRADE_PEER_STATE_ABORTING)
            {
                /* Once the l2cap connection is disconnected, clear the 
                 * upgradePeerInfo flag as part of clearance process
                 */
                CleanUpgradePeerCtx();
            }
            else 
            {
                /* Peer device connection is lost and can't be recovered. Hence
                 * fail the upgrade process. Create the l2cap connection first,
                 * to send the DFU variables clearance to peer device.
                 */
                DEBUG_LOG("Create the l2cap connection to initiate DFU abort \n");
                /* Establish l2cap connection with peer to abort */
                MessageSend(upgradePeerInfo->appTask, UPGRADE_PEER_CONNECT_REQ, NULL);
                upgradePeerInfo->is_dfu_abort_trigerred = TRUE;
            }
            break;
        case UPGRADE_PEER_GENERIC_MSG:
            peer_data = (uint8 *) PanicUnlessMalloc(dataSize);
            ByteUtilsMemCpyToStream(peer_data, data, dataSize);
            MessageSend((Task)&upgradePeerInfo->myTask,
                         INTERNAL_PEER_MSG, peer_data);
            break;
        default:
            DEBUG_LOG("unhandled msg\n");
    }
}
