/****************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_ctx.h

DESCRIPTION
    Header file for the Upgrade context.
*/

#ifndef UPGRADE_CTX_H_
#define UPGRADE_CTX_H_

#include <message.h>

#include "imageupgrade.h"

#include "upgrade_partition_data_priv.h"
#include "upgrade_fw_if_priv.h"
#include "upgrade_sm.h"
#include "upgrade_private.h"
#include "upgrade.h"
#include "upgrade_psstore_priv.h"

/* Macros to determine whether to start the peer DFU or condiitonally delay it */
#define UPGRADE_DELAY_PEER_DFU 1

#define UPGRADE_START_PEER_DFU 0

typedef struct
{
    TaskData smTaskData;
    Task mainTask;
    Task clientTask;
    Task transportTask;
    /* An indication of whether the transport needs a UPGRADE_TRANSPORT_DATA_CFM
     * or not.
     */
    bool need_data_cfm;
    /* An indication of whether the transport can handle a request for multiple
     * blocks at a time, or not.
     */
    bool request_multiple_blocks;
    UpgradeState smState;
    UpgradePartitionDataCtx *partitionData;
    UpgradeFWIFCtx fwCtx;
    uint16 partitionValidationNextPartition;
    const UPGRADE_UPGRADABLE_PARTITION_T *upgradeLogicalPartitions;
    uint16 upgradeNumLogicalPartitions;
    /* Storage for PSKEY management.
     *
     * The library uses a (portion of) PSKEY supplied by the application
     * so needs to remember the details of the PSKEY.
     */
    uint16 upgrade_library_pskey;
    uint16 upgrade_library_pskeyoffset;
    uint32 partitionDataBlockSize;
    /*! Storage for upgrade library information maintained in PSKEYS.
     *
     * The values are only @b read from PSKEY on boot.
     * The local copy can be read/written at any time, but should be written
     * @b to storage as little as possible.
     *
     * Since this PSKEY storage structure has multiple uses be aware that
     * any value you write to the local copy may be committed at any time.
     *
     * The values will always be initialised, to 0x00 at worst.
     */
    UPGRADE_LIB_PSKEY UpgradePSKeys;

    /* Current level of permission granted the upgrade library by the VM
     * application */
    upgrade_permission_t perms;

    /*! The current power management mode (disabled/battery powered) */
    upgrade_power_management_t power_mode;

    /*! The current power management state informed by the VM app */
    upgrade_power_state_t power_state;

    /*! device variant */
    char dev_variant[UPGRADE_HOST_VARIANT_CFM_BYTE_SIZE+1];

    const upgrade_response_functions_t *funcs;

    /* P0 Hash context */
    hash_context_t vctx;

    /* CSR Valid message received flag */
    bool isCsrValidDoneReqReceived;

    /*! Force erase when a START_REQ is received */
    bool force_erase;

    /*! The flow control scheme using dfu_rx_flow_off and pendingDataReq
     *  is especially needed for DFU over LE where the upgrade data is
     *  packet oriented and the max MTU is 64.
     *  Being packet oriented and smaller MTU as compared to BR/EDR, leads to
     *  more short (complete or partial) messages queued for processing as the
     *  Source Buffer is drained.
     *  Since these messages are allocated from heap (pmalloc pool), its highly
     *  probable to run out of pmalloc pools. So a flow control scheme is
     *  required to keep the queued messages for processing within acceptable
     *  limits. In case of LE, the GATT Source stream size is 512 which can hold
     *  at max 8 upgrade data packets as max MTU is 64. So the acceptable
     *  queued message limit is 8.
     *  TODO: Upgrade library is commonly used both for Host<->Primary upgrade
     *        and also Primary<->Secondary upgrade.
     *        Primary<->Secondary upgrade is always using BR/EDR link and
     *        as such this flow control scheme is not required owing to larger
     *        MTU and upgrade data being non-packet oriented.
     *        Also Host<->Primary upgrade can be over BR/EDR link, in which case
     *        this flow control scheme is not required as reasoned above.
     *        But even if this flow control scheme is commonly applied for
     *        DFU over LE or BR/EDR, this won't radically impact the KPIs for
     *        DFU over BR/EDR.
     *        In future, if need arises to localize this flow control scheme to
     *        DFU over LE then logic needs to be added to distinguish
     *        Host<->Primary transport and whether Host<->Primary upgrade or
     *        Primary<->Secondary upgrade.
     */

    /*! Track whether processing of upgrade data from Source Buffer is flowed
     *  off or not.
     *  Used for DFU over LE flow control scheme.
     */
    bool dfu_rx_flow_off;

    /*! Outstanding upgrade data messages extracted from Source Buffer
     *  and queued to be processed (i.e. written to flash).
     *  Used for DFU over LE flow control scheme.
     */
    uint32 pendingDataReq;

     /*! Flag included to defer sending of data request
     *  during DFU if SCO is active.
     *  Used to pause DFU during incoming/outgoing active calls.
     *  Type: isScoActive is uint16 as used in MessageSendConditionally
     */
    uint16 isScoActive;

   /*! Status to indicate the start or conditionally delay the peer dfu for image upgrade copy
    */
   uint16 peerDfuStartStatus;

#ifdef HANDOVER_DFU_ABORT_WITHOUT_ERASE
    /*! Track upgrade abort states during Handover.*/
    handover_upgrade_abort_state_t handoverDfuAbortState;
#endif

   /*! Flag to indicate that Primary reboot phase of DFU is completed. */
   uint16 dfuPriRebootDone;


    /*! ImageUpgradeCopy() is completed
     *  i.e. received MESSAGE_IMAGE_UPGRADE_COPY_STATUS.
     */
    bool isImgUpgradeCopyDone:1;
    /*! Copy status is significant only when isImgUpgradeCopyDone is set.
     *  If set then copy is successful else failed.
     */
    bool ImgUpgradeCopyStatus:1;

    /*! ImageUpgradeErase() is completed
     *  i.e. received MESSAGE_IMAGE_UPGRADE_ERASE_STATUS.
     */
    uint16 isImgUpgradeEraseDone;

    upgrade_reconnect_recommendation_t reconnect_reason;

} UpgradeCtx;

void UpgradeCtxSet(UpgradeCtx *ctx);
UpgradeCtx *UpgradeCtxGet(void);

/*!
    @brief Set the partition data context in the upgrade context.
*/
void UpgradeCtxSetPartitionData(UpgradePartitionDataCtx *ctx);

/*! @brief Get the partition data context.

    @return Pointer to the partition data context. It may be
            NULL if it has not been set.
*/
UpgradePartitionDataCtx *UpgradeCtxGetPartitionData(void);

UpgradeFWIFCtx *UpgradeCtxGetFW(void);
UPGRADE_LIB_PSKEY *UpgradeCtxGetPSKeys(void);

uint16 *UpgradeCtxGetPeerDFUStartStatus(void);

void UpgradeCtxSetPeerDFUStartStatus(uint16 peer_dfu_start_status);

#endif /* UPGRADE_CTX_H_ */
