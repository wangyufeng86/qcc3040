/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_peer.h
    
DESCRIPTION
    Header file for the Upgrade Peer library.
*/
/*!
@file   upgrade_peer.h

*/
#ifndef UPGRADE_PEER_H_
#define UPGRADE_PEER_H_

#include <library.h>
#include <message.h>
#include "upgrade_msg_host.h"
#include "upgrade.h"

#define UPGRADE_PEER_IS_SUPPORTED           UpgradePeerIsSupported()
#define UPGRADE_PEER_IS_COMMITED            UpgradePeerIsCommited()
#define UPGRADE_PEER_IS_RESTARTED           UpgradePeerIsRestarted()
#define UPGRADE_PEER_START_DFU              UpgradePeerStartDfu()
#define UPGRADE_PEER_IS_STARTED             UpgradePeerIsStarted()
#define UPGRDAE_PEER_IS_COMMIT_CONTINUE     UpgradePeerIsCommitContinue()
#define UPGRADE_PEER_IS_ENDED               !UpgradePeerIsStarted()

/*!
    @brief Time wait before polling again for peer upgrade completion.
 */
#define UPGRADE_PEER_POLL_INTERVAL 300

typedef enum
{
    UPGRADE_PEER_START_REQ                                        = 0x01,
    UPGRADE_PEER_START_CFM                                        = 0x02,
    UPGRADE_PEER_DATA_BYTES_REQ                                   = 0x03,
    UPGRADE_PEER_DATA                                             = 0x04,
    UPGRADE_PEER_SUSPEND_IND                                      = 0x05, /* Not Supported */
    UPGRADE_PEER_RESUME_IND                                       = 0x06, /* Not Supported */
    UPGRADE_PEER_ABORT_REQ                                        = 0x07,
    UPGRADE_PEER_ABORT_CFM                                        = 0x08,
    UPGRADE_PEER_PROGRESS_REQ                                     = 0x09, /* Not Supported */
    UPGRADE_PEER_PROGRESS_CFM                                     = 0x0A, /* Not Supported */
    UPGRADE_PEER_TRANSFER_COMPLETE_IND                            = 0x0B,
    UPGRADE_PEER_TRANSFER_COMPLETE_RES                            = 0x0C,
    UPGRADE_PEER_IN_PROGRESS_IND                                  = 0x0D, /* Not Supported */
    UPGRADE_PEER_IN_PROGRESS_RES                                  = 0x0E, /* Not Used */
    UPGRADE_PEER_COMMIT_REQ                                       = 0x0F,
    UPGRADE_PEER_COMMIT_CFM                                       = 0x10,
    UPGRADE_PEER_ERROR_WARN_IND                                   = 0x11,
    UPGRADE_PEER_COMPLETE_IND                                     = 0x12,
    UPGRADE_PEER_SYNC_REQ                                         = 0x13,
    UPGRADE_PEER_SYNC_CFM                                         = 0x14,
    UPGRADE_PEER_START_DATA_REQ                                   = 0x15,
    UPGRADE_PEER_IS_VALIDATION_DONE_REQ                           = 0x16,
    UPGRADE_PEER_IS_VALIDATION_DONE_CFM                           = 0x17,
    UPGRADE_PEER_SYNC_AFTER_REBOOT_REQ                            = 0x18, /* Not Supported */
    UPGRADE_PEER_VERSION_REQ                                      = 0x19,
    UPGRADE_PEER_VERSION_CFM                                      = 0x1A,
    UPGRADE_PEER_VARIANT_REQ                                      = 0x1B,
    UPGRADE_PEER_VARIANT_CFM                                      = 0x1C,
    UPGRADE_PEER_ERASE_SQIF_REQ                                   = 0x1D,
    UPGRADE_PEER_ERASE_SQIF_CFM                                   = 0x1E,
    UPGRADE_PEER_ERROR_WARN_RES                                   = 0x1F
} upgrade_peer_msg_t;

/* Error codes based on Upgrade library */
typedef enum
{
    UPGRADE_PEER_SUCCESS = UPGRADE_HOST_SUCCESS,
    UPGRADE_PEER_OEM_VALIDATION_SUCCESS,

    UPGRADE_PEER_ERROR_INTERNAL_ERROR_DEPRECATED = UPGRADE_HOST_ERROR_INTERNAL_ERROR_DEPRECATED,
    UPGRADE_PEER_ERROR_UNKNOWN_ID,
    UPGRADE_PEER_ERROR_BAD_LENGTH_DEPRECATED,
    UPGRADE_PEER_ERROR_WRONG_VARIANT,
    UPGRADE_PEER_ERROR_WRONG_PARTITION_NUMBER,

    UPGRADE_PEER_ERROR_PARTITION_SIZE_MISMATCH,
    UPGRADE_PEER_ERROR_PARTITION_TYPE_NOT_FOUND_DEPRECATED,
    UPGRADE_PEER_ERROR_PARTITION_OPEN_FAILED,
    UPGRADE_PEER_ERROR_PARTITION_WRITE_FAILED_DEPRECATED,
    UPGRADE_PEER_ERROR_PARTITION_CLOSE_FAILED_DEPRECATED,

    UPGRADE_PEER_ERROR_SFS_VALIDATION_FAILED,
    UPGRADE_PEER_ERROR_OEM_VALIDATION_FAILED_DEPRECATED,
    UPGRADE_PEER_ERROR_UPDATE_FAILED,
    UPGRADE_PEER_ERROR_APP_NOT_READY,

    UPGRADE_PEER_ERROR_LOADER_ERROR,
    UPGRADE_PEER_ERROR_UNEXPECTED_LOADER_MSG,
    UPGRADE_PEER_ERROR_MISSING_LOADER_MSG,

    UPGRADE_PEER_ERROR_BATTERY_LOW,
    UPGRADE_PEER_ERROR_INVALID_SYNC_ID,
    UPGRADE_PEER_ERROR_IN_ERROR_STATE,
    UPGRADE_PEER_ERROR_NO_MEMORY,
    UPGRADE_PEER_ERROR_SQIF_ERASE,
    UPGRADE_PEER_ERROR_SQIF_COPY,
    UPGRADE_PEER_ERROR_AUDIO_SQIF_COPY,

    /* The remaining errors are grouped, each section starting at a fixed
     * offset */
    UPGRADE_PEER_ERROR_BAD_LENGTH_PARTITION_PARSE = UPGRADE_HOST_ERROR_BAD_LENGTH_PARTITION_PARSE,
    UPGRADE_PEER_ERROR_BAD_LENGTH_TOO_SHORT,
    UPGRADE_PEER_ERROR_BAD_LENGTH_UPGRADE_HEADER,
    UPGRADE_PEER_ERROR_BAD_LENGTH_PARTITION_HEADER,
    UPGRADE_PEER_ERROR_BAD_LENGTH_SIGNATURE,
    UPGRADE_PEER_ERROR_BAD_LENGTH_DATAHDR_RESUME,

    UPGRADE_PEER_ERROR_OEM_VALIDATION_FAILED_HEADERS = UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_HEADERS,
    UPGRADE_PEER_ERROR_OEM_VALIDATION_FAILED_UPGRADE_HEADER,
    UPGRADE_PEER_ERROR_OEM_VALIDATION_FAILED_PARTITION_HEADER1,
    UPGRADE_PEER_ERROR_OEM_VALIDATION_FAILED_PARTITION_HEADER2,
    UPGRADE_PEER_ERROR_OEM_VALIDATION_FAILED_PARTITION_DATA,
    UPGRADE_PEER_ERROR_OEM_VALIDATION_FAILED_FOOTER,
    UPGRADE_PEER_ERROR_OEM_VALIDATION_FAILED_MEMORY,

    UPGRADE_PEER_ERROR_PARTITION_CLOSE_FAILED = UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED,
    UPGRADE_PEER_ERROR_PARTITION_CLOSE_FAILED_HEADER,
    /*! When sent, the error indicates that an upgrade could not be completed 
     * due to concerns about space in Persistent Store.  No other upgrade
     * activities will be possible until the device restarts.
     * This error requires a UPGRADE_PEER_ERRORWARN_RES response (following
     * which the library will cause a restart, if the VM application permits)
     */
    UPGRADE_PEER_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE,

    UPGRADE_PEER_ERROR_PARTITION_TYPE_NOT_MATCHING = UPGRADE_HOST_ERROR_PARTITION_TYPE_NOT_MATCHING,
    UPGRADE_PEER_ERROR_PARTITION_TYPE_TWO_DFU,

    UPGRADE_PEER_ERROR_PARTITION_WRITE_FAILED_HEADER = UPGRADE_HOST_ERROR_PARTITION_WRITE_FAILED_HEADER,
    UPGRADE_PEER_ERROR_PARTITION_WRITE_FAILED_DATA,

    UPGRADE_PEER_ERROR_FILE_TOO_SMALL = UPGRADE_HOST_ERROR_FILE_TOO_SMALL,
    UPGRADE_PEER_ERROR_FILE_TOO_BIG,

    UPGRADE_PEER_ERROR_INTERNAL_ERROR_1 = UPGRADE_HOST_ERROR_INTERNAL_ERROR_1,
    UPGRADE_PEER_ERROR_INTERNAL_ERROR_2,
    UPGRADE_PEER_ERROR_INTERNAL_ERROR_3,
    UPGRADE_PEER_ERROR_INTERNAL_ERROR_4,
    UPGRADE_PEER_ERROR_INTERNAL_ERROR_5,
    UPGRADE_PEER_ERROR_INTERNAL_ERROR_6,
    UPGRADE_PEER_ERROR_INTERNAL_ERROR_7,

    UPGRADE_PEER_WARN_APP_CONFIG_VERSION_INCOMPATIBLE = UPGRADE_HOST_WARN_APP_CONFIG_VERSION_INCOMPATIBLE,
    UPGRADE_PEER_WARN_SYNC_ID_IS_DIFFERENT
} upgrade_peer_status_t;

typedef enum
{
    /**
     * Used by the application to confirm that the upgrade should continue.
     */
    UPGRADE_CONTINUE,

    /**
     * Used by the application to confirm that the upgrade should abort.
     */
    UPGRADE_ABORT
}upgrade_action_status_t;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Command Packet Common Fields and Return packet
 *
 *---------------------------------------------------------------------------*/
typedef struct
{
    uint8    op_code;             /* op code of command */
    uint16   length;              /* parameter total length */
} UPGRADE_PEER_COMMON_CMD_T;


typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
    uint32                      upgrade_file_id;
} UPGRADE_PEER_SYNC_REQ_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
    uint8                       resume_point;
    uint32                      upgrade_file_id;
    uint8                       protocol_id;
} UPGRADE_PEER_SYNC_CFM_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
} UPGRADE_PEER_START_REQ_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
    uint8                       status;
    uint16                      battery_level;
} UPGRADE_PEER_START_CFM_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
} UPGRADE_PEER_START_DATA_REQ_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
    uint32                      data_bytes;
    uint32                      start_offset;
} UPGRADE_PEER_START_DATA_BYTES_REQ_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
    uint8                       more_data;
    uint8                       image_data;
} UPGRADE_PEER_DATA_REQ_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
    uint32                      data_bytes;
    uint32                      start_offset;
} UPGRADE_PEER_DATA_BYTES_REQ_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
} UPGRADE_PEER_VERIFICATION_DONE_REQ_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
    uint16                      delay_time;
} UPGRADE_PEER_VERIFICATION_DONE_CFM_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
} UPGRADE_PEER_TRANSFER_COMPLETE_IND_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
    uint8                       action;
} UPGRADE_PEER_TRANSFER_COMPLETE_RES_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
    uint8                       action;
} UPGRADE_PEER_PROCEED_TO_COMMIT_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
} UPGRADE_PEER_COMMIT_REQ_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
    uint8                       action;
} UPGRADE_PEER_COMMIT_CFM_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
} UPGRADE_PEER_UPGRADE_COMPLETE_IND_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
    uint16                       error;
} UPGRADE_PEER_UPGRADE_ERROR_IND_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
    uint16                       error;
} UPGRADE_PEER_UPGRADE_ERROR_RES_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
} UPGRADE_PEER_ABORT_REQ_T;

typedef struct {
    UPGRADE_PEER_COMMON_CMD_T    common;
} UPGRADE_PEER_ABORT_CFM_T;

/*!
    @brief Check if Upgrade peer is in commit confirm state.
    Returns TRUE if peer device has requested COMMIT_REQ.
*/
bool UpgradePeerIsCommited(void);

/*!
    @brief Check is UpgradePeer module is restarted after upgrade is done.
    Returns TRUE if UpgradePeer is in Post Reboot state.
*/
bool UpgradePeerIsRestarted(void);

/*!
    @brief Handle message received from UpgradeSm (i.e received from Host)

    @param msgid This indictaes message id.
    @param status Continue or Abort
    Returns None
*/
void UpgradePeerProcessHostMsg(upgrade_peer_msg_t msgid,
                               upgrade_action_status_t status);

/*!
    @brief Start peer device upgrade procedure.
        
    Returns TRUE if peer device upgrade procedure is started.
*/
bool UpgradePeerStartDfu(void);

/*!
    @brief Check is peer device upgrade is started.
        
    Returns TRUE if peer device upgrade is started.
*/
bool UpgradePeerIsStarted(void);

/*!
    @brief Request to resume the peer device upgrade.
        
    Returns TRUE if peer device upgrade is resumed.
*/
bool UpgradePeerResumeUpgrade(void);

/*!
    @brief Check if reboot is done and peer connection is up for commit
           continue.
        
    Returns TRUE if peer device is ready for commit continue.
*/
bool UpgradePeerIsCommitContinue(void);

#endif /* UPGRADE_PEER_H_ */
