/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_peer_private.h

DESCRIPTION

NOTES

*/
#include <string.h>
#include <stdlib.h>

#include <boot.h>
#include <message.h>
#include <byte_utils.h>
#include <print.h>
#include <panic.h>
#include "upgrade_peer.h"

#ifndef UPGRADE_PEER_PRIVATE_H_
#define UPGRADE_PEER_PRIVATE_H_
/**
 * This file allows building of a packet for the VM upgrade as defined in the 
 * VM upgrade documentation. The VM upgrade packet is composed as follows:
 *      0 bytes   1        2         3         4        length+3
 *      +---------+---------+---------+ +---------+---------+
 *      | OPCODE* |      LENGTH*      | |      DATA...      |
 *      +---------+---------+---------+ +---------+---------+
 */

#define UPGRADE_PEER_PACKET_HEADER                                  (3)
#define UPGRADE_PEER_PACKET_LENGTH                                  (2)

/**
 * Structure of UPGRADE_IS_VALIDATION_DONE_CFM message.
 *      0 bytes  1        2
 *      +--------+--------+
 *      |   WAITING TIME  |
 *      +--------+--------+
 */
#define UPGRADE_VAIDATION_DONE_CFM_DATA_LENGTH                      (2)

/**
 * Structure of UPGRADE_SYNC_REQ message.
 *      0 bytes  1        2        3        4
 *      +--------+--------+--------+--------+
 *      |      IN PROGRESS IDENTIFIER       |
 *      +--------+--------+--------+--------+
 */
#define UPGRADE_SYNC_REQ_DATA_LENGTH                                (4)

/**
 * Structure of UPGRADE_DATA_BYTES_REQ message.
 *      0 bytes  1        2        3        4        5        6        7        8
 *      +--------+--------+--------+--------+--------+--------+--------+--------+
 *      |     NUMBER OF BYTES REQUESTED     |         FILE START OFFSET         |
 *      +--------+--------+--------+--------+--------+--------+--------+--------+
 */
#define UPGRADE_DATA_BYTES_REQ_DATA_LENGTH                          (8)

/**
 * Structure of UPGRADE_START_CFM UPGRADE_START_CFM message.
 *      0 bytes  1        2        3
 *      +--------+--------+--------+
 *      | STATUS |  BATTERY LEVEL  |
 *      +--------+--------+--------+
 */
#define UPRAGE_HOST_START_CFM_DATA_LENGTH                           (3)

/**
 * Structure of UPGRADE_SYNC_CFM UPGRADE_SYNC_CFM message.
 *      0 bytes            1        2        3        4        5                  6
 *      +------------------+--------+--------+--------+--------+------------------+
 *      |   RESUME POINT   |       IN PROGRESS IDENTIFIER      | PROTOCOL VERSION |
 *      +------------------+--------+--------+--------+--------+------------------+
 */
#define UPGRADE_SYNC_CFM_DATA_LENGTH                                (6)

/**
 * Structure of UPGRADE_TRANSFER_COMPLETE_RES message.
 *      0 bytes    1
 *      +----------+
 *      |  ACTION  |
 *      +----------+
 */
#define UPGRADE_TRANSFER_COMPLETE_RES_DATA_LENGTH                   (1)

/**
 * Structure of UPGRADE_IN_PROGRESS_RES message.
 * <blockquote><pre>
 *      0 bytes    1
 *      +----------+
 *      |  ACTION  |
 *      +----------+
 * </pre></blockquote>
 */
#define UPGRADE_IN_PROGRESS_DATA_LENGTH                             (1)

/**
 * Structure of UPGRADE_DATA UPGRADE_DATA message.
 * .</p>
 * <blockquote><pre>
 *      0 bytes       1       ...       n
 *      +-------------+--------+--------+
 *      | LAST PACKET |    DATA...
 *      +-------------+--------+--------+
 * </pre></blockquote>
 */
#define UPGRADE_DATA_MIN_DATA_LENGTH                                (1)

/**
 * Structure of UPGRADE_COMMIT_CFM message.
 * <blockquote><pre>
 *      0 bytes    1
 *      +----------+
 *      |  ACTION  |
 *      +----------+
 * </pre></blockquote>
 */
#define UPGRADE_COMMIT_CFM_DATA_LENGTH                              (1)

/**
 * Structure of UPGRADE_ERROR_IND message.
 *      0 bytes  1        2
 *      +--------+--------+
 *      |   EROR          |
 *      +--------+--------+
 */
#define UPGRADE_ERROR_IND_DATA_LENGTH                      (2)

#define BYTES_TO_WORDS(_b_)       ((_b_+1) >> 1)

typedef enum  {
    /**
     * When the primary device receives the UPGRADE_TRANSFER_COMPLETE_IND
     * message, the board is asking for a confirmation to CONTINUE
     * or ABORT the process.
     */
    UPGRADE_TRANSFER_COMPLETE,
    /**
     * When the primary device receives the UPGRADE_COMMIT_REQ message, the
     * board is asking for a confirmation to CONTINUE
     * or ABORT the process.
     */
    UPGRADE_COMMIT,
    /**
     * When the primary device receives resume point IN_PROGRESS IN_PROGRESS is
     * reached, the board is expecting to receive a confirmation to CONTINUE or
     * ABORT the process.
     */
    UPGRADE_IN_PROGRESS,
    /**
     * When the primary device receives UPGRADE_WARNING_FILE_IS_DIFFERENT,
     * the listener has to ask if the upgrade should continue or not.
     */
    UPGRADE_WARNING_FILE_IS_DIFFERENT,
    /**
     * When the primary device receives ERROR_BATTERY_LOW ERROR_BATTERY_LOW,the
     * listener has to ask if the upgrade should continue or not.
     */
    UPGRADE_BATTERY_LOW_ON_DEVICE
} upgrade_confirmation_type_t;

typedef enum {
   /*! 
     * This is the resume point "0", that means the upgrade will start from the
     * beginning, the UPGRADE_START_DATA_REQ request.
     */
    UPGRADE_PEER_RESUME_POINT_START,
    /*! This is the 1st resume point, that means the upgrade should resume from
     * the UPGRADE_IS_CSR_VALID_DONE_REQ request.
     */
    UPGRADE_PEER_RESUME_POINT_PRE_VALIDATE,
    /*! This is the 2nd resume point, that means the upgrade should resume from
     * the UPGRADE_TRANSFER_COMPLETE_RES request.
     */
    UPGRADE_PEER_RESUME_POINT_PRE_REBOOT, 
    /*! This is the 3rd resume point, that means the upgrade should resume from
     * the UPGRADE_IN_PROGRESS_RES request.
     */
    UPGRADE_PEER_RESUME_POINT_POST_REBOOT, 
    /*! This is the 4th resume point, that means the upgrade should resume from
     * the UPGRADE_COMMIT_CFM confirmation request.
     */
    UPGRADE_PEER_RESUME_POINT_COMMIT,
    /*! Final stage of an upgrade, partition erase still required */
    UPGRADE_PEER_RESUME_POINT_ERASE,

    /*! Resume in error handling, before reset unhandled error message have been sent */
    UPGRADE_PEER_RESUME_POINT_ERROR
} upgrade_peer_resume_point_t;

/*!
    @brief Enumeration of the states in the machine.
 */
typedef enum {
    /*! TODO and all the other items too... */
    UPGRADE_PEER_STATE_CHECK_STATUS,
    UPGRADE_PEER_STATE_SYNC,
    UPGRADE_PEER_STATE_READY,
    UPGRADE_PEER_STATE_ABORTING,
    
    UPGRADE_PEER_STATE_DATA_READY,
    UPGRADE_PEER_STATE_DATA_TRANSFER,
    UPGRADE_PEER_STATE_DATA_TRANSFER_SUSPENDED,
    UPGRADE_PEER_STATE_DATA_TRANSFER_DONE,

    UPGRADE_PEER_STATE_VALIDATED,
    UPGRADE_PEER_STATE_CONNECT_FOR_REBOOT,

    UPGRADE_PEER_STATE_RESTARTED_FOR_COMMIT,
    UPGRADE_PEER_STATE_COMMIT_HOST_CONTINUE,
    UPGRADE_PEER_STATE_COMMIT_VERIFICATION,
    UPGRADE_PEER_STATE_COMMIT_CONFIRM,
    UPGRADE_PEER_STATE_COMMIT,

    UPGRADE_PEER_STATE_PS_JOURNAL,
    UPGRADE_PEER_STATE_REBOOT_TO_RESUME  /*! Unable to continue until a reboot */
} upgrade_peer_state_t;

typedef enum
{
    INTERNAL_START_REQ_MSG,
    INTERNAL_VALIDATION_DONE_MSG,
    INTERNAL_PEER_MSG
} upgrade_peer_internal_msg_t;

/* Structure used internally to save information about the upgrade across
 * boots in a PSKEY.
 *
 * See upgrade_psstore.h for functions to access / update the values in this
 * structure.

 * IMPROTANT NOTE: If any new elements is added in the PSKEY, add it from the top. For
                   more details, refer to upgrade.h for the PKSEY parameters alignment.
 */
typedef struct
{
    /* Upgrade status information, stored from offset 26 in PSKEY */
    upgrade_peer_resume_point_t upgradeResumePoint;
    upgrade_peer_state_t currentState;
    bool is_secondary_device;
    /* While DFU is happening on primary, secondary doesn't know it so store
       the DFU mode info in PSKEY. So if secondary is reset during that time, it should
       come back in DFU mode.
       */
    bool is_dfu_mode;

    /* If adding any more fields here, refer to EARBUD_UPGRADE_PEER_CONTEXT_OFFSET
       to know if space is available. */
} UPGRADE_PEER_LIB_PSKEY;

/* Use sizeof to make sure we get the right size under Windows and final build */
#define UPGRADE_PEER_PSKEY_USAGE_LENGTH_WORDS  (sizeof(UPGRADE_PEER_LIB_PSKEY)/sizeof(uint16))

#ifndef HOSTED_TEST_ENVIRONMENT
/* Added a COMPILE TIME ASSERT to make sure the PSKEY is not used to stored data
   more than allowed length of 32 words.
 */
COMPILE_TIME_ASSERT(EARBUD_UPGRADE_PEER_CONTEXT_OFFSET ==
                    (PSKEY_MAX_STORAGE_LENGTH - UPGRADE_PEER_PSKEY_USAGE_LENGTH_WORDS),
                    earbud_upgrade_context_pskey_overflows_pskey_max_length);
#endif

typedef struct
{
    /**
     * To know if the upgrade process is currently running.
     */
    bool isUpgrading;
    /**
     * To know how many times we try to start the upgrade.
     */
    unsigned mStartAttempts;
    /**
     * The file to upload on the device.
     */
    uint8 *file;
    /**
     * To know if the packet with the operation code "UPGRADE_DATA" which was
     * sent was the last packet to send.
     */
    bool wasLastPacket;
    /**
     * The value of the actual resume point to display to the user.
     */
    upgrade_peer_resume_point_t mResumePoint;

    upgrade_peer_status_t upgrade_status;

    upgrade_confirmation_type_t confirm_type;

    upgrade_peer_state_t peerState;

    /**
     * The offset to use to upload data on the device.
     */
    unsigned mStartOffset;
} UPGRADE_PEER_CTX_T;

typedef struct
{
    TaskData myTask;

    Task appTask;

    bool is_primary_device;

    uint16 upgrade_peer_pskey;

    uint16 upgrade_peer_pskeyoffset;

    UPGRADE_PEER_LIB_PSKEY UpgradePSKeys;

    uint32 md5_checksum;

    UPGRADE_PEER_CTX_T *SmCtx;

    uint8 upgrade_dummy;

    bool is_dfu_aborted;

    bool is_dfu_abort_trigerred;
} UPGRADE_PEER_INFO_T;

/*!
    @brief Get Upgrade Peer Context
    @param none
    
    Returns Context of UpgradePeer.
*/
UPGRADE_PEER_CTX_T *upgradePeerCtxGet(void);

/*!
    @brief Read data from parititon
    @param dataPtr where partition data is copied.
    @param last_packet TRUE if partition data is last packet.
    @param mBytesReq Amount of parition data is requested by peer.
    @param mBytesSent Amount of data is read from partition.

    Returns status of partition data (SUCCESS or FAILURE).
*/
upgrade_peer_status_t UpgradePeerPartitionMoreData(uint8 *dataPtr,
                                                   bool *last_packet,
                                                   uint32 mBytesReq,
                                                   uint32 *mBytesSent);

/*!
    @brief Set Upgrade peer state on context.
    @param nextState State of Upgrade Peer.

    Returns none.
*/
void UpgradePeerSetState(upgrade_peer_state_t nextState);

/*!
    @brief Init Upgrade peer partition.

    Returns none.
*/
void UpgradePeerParititonInit(void);

#endif /* UPGRADE_PEER_PRIVATE_H_ */
