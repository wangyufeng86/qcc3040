/* Copyright (c) 2018-2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
/****************************************************************************
FILE
    acl_if.h

CONTAINS
    Definitions used by Acl and Mirroring trapsets.

*/

#ifndef __APP_ACL_IF_H__
#define __APP_ACL_IF_H__

/*! @brief Status of handover prepare state as returned by the AclHandoverPrepared()
    trap.
*/
typedef enum
{
    /*! Handover preparation in progress. */
    ACL_HANDOVER_PREPARE_IN_PROGRESS,

    /*! Handover preparation completed. */
    ACL_HANDOVER_PREPARE_COMPLETE,

    /*! Handover preparation failed. */
    ACL_HANDOVER_PREPARE_FAILED

} acl_handover_prepare_status;

/*! @brief Inbound data processed status as returned by the AclReceivedDataProcessed()
    trap.
*/
typedef enum
{
    /*! All received ACL data processed and consumed successfully. */
    ACL_RECEIVE_DATA_PROCESSED_COMPLETE,

    /*! All received ACL data not processed within the configured timeout. */
    ACL_RECEIVE_DATA_PROCESSED_TIMEOUT,

    /*! Received ACL data processing failed. */
    ACL_RECEIVE_DATA_PROCESSED_FAILED

} acl_rx_processed_status;

/*! @brief Structure used to capture debug statistics for reliable ACL mirroring.
 *  These statistics are enabled and reset to zero when reliable ACL mirroring is
 *  enabled.
 */
typedef struct acl_reliable_mirror_debug_statistics
{
    /*! Number of ACL packets received from remote device. */
    uint32 rx_packet_count;
    /*! Number of ACL packets relayed to secondary device. */
    uint32 relay_packet_count;
    /*! Number of instances of flushing ACL packets. */
    uint32 flush_flag_count;
    /*! Number of instances of buffer not being available to 
     *  receive ACL packets.
     */
    uint32 out_of_buffers_count;
    /*! Number of instances of flushing ACL packets due to
     *  buffer overflows.
     */
    uint32 overflow_count;
    /*! Number of instances of flushing ACL packets due to
     *  generic errors.
     */
    uint32 general_error_count;
} acl_reliable_mirror_debug_statistics_t;

#endif /* __APP_ACL_IF_H__ */
