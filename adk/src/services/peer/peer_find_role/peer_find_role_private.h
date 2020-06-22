/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Header file for the peer service finding peer using LE and selecting
            the role.
*/

#ifndef PEER_FIND_ROLE_PRIVATE_H_
#define PEER_FIND_ROLE_PRIVATE_H_

#include <message.h>

#include <task_list.h>
#include <gatt_role_selection_client.h>
#include <gatt_role_selection_server.h>

#include <le_scan_manager.h>
#include <le_advertising_manager.h>

#include "peer_find_role_scoring.h"
#include "peer_find_role_sm.h"

/*! Defines the  */
#define PEER_FIND_ROLE_REGISTERED_TASKS_LIST_INIT_CAPACITY 2


/*! Enumerated type (bit mask) to record scanning operations.
    These take time to cancel 
 */
typedef enum
{
        /*! We are still scanning */
    PEER_FIND_ROLE_ACTIVE_SCANNING        = (1 << 0),
} peerFindRoleScanActiveStatus_t;


/*! Enumerated type (bit mask) to record when media is busy, that
    may affect our operation. */
typedef enum
{
        /*! Audio streaming is active */
    PEER_FIND_ROLE_AUDIO_STREAMING      = (1 << 0),
        /*! Call is inactive */
    PEER_FIND_ROLE_CALL_ACTIVE          = (1 << 1),
        /*! Primary role has been issued */
    PEER_FIND_ROLE_CONFIRMING_PRIMARY   = (1 << 2),

        /*! Requested scan block  */
    PEER_FIND_ROLE_SCANNING_DISABLED    = (1 << 8),
} peerFindRoleMediaBusyStatus_t;

/* Enumerated type describing current LE scan mode */
typedef enum
{
    /*! When scan is active high duty scan is used */
    scan_state_high_duty,
    /*! When scan is active low duty scan is used */
    scan_state_low_duty
} scan_state_mode_t;

/*! Structure to hold information used by the peer find role service */
typedef struct 
{
    /*! Task for handling messages */
    TaskData                    _task;

    /*! List of tasks to send messages to */
    TASK_LIST_WITH_INITIAL_CAPACITY(PEER_FIND_ROLE_REGISTERED_TASKS_LIST_INIT_CAPACITY)  registered_tasks;

    /*! address of this device */
    bdaddr                      my_addr;
    /*! address of peer device (Public) */
    bdaddr                      primary_addr;

    /*! Peer device address (Public/Random) being used for connection */
    typed_bdaddr                peer_connection_typed_bdaddr;

    /*! Timeout, if any, to be used when attempting to find role */
    uint32                      role_timeout_ms;

    /*! Delay before starting adverts when entering a discovery phase */
    uint32                      advertising_backoff_ms;

    /*! The connection identifier of the gatt connection being used */
    uint16                      gatt_cid;

    /*! Role that has been selected for this device. Stashed temporarily
        while waiting for links to be cleared. Should not be set whilst
        peer_find_role isn't running */
    peer_find_role_message_t    selected_role;

    /*! The GATT connection has been encrypted */
    bool                        gatt_encrypted;

    /*! When find role times-out, stop all activity 
        \todo This is a temporary feature */
    bool                        timeout_means_timeout;

    /*! Details of the current LE advertising data set */
    le_adv_data_set_handle      advert_handle;

    /*! Store whether scanning should be enabled or disabled */
    bool                        scan_enable;
    /*! What scan operations are busy? Bitfield using peerFindRoleScanActiveStatus_t 

        This can be used in MessageSendConditional() */
    uint16                      scan_activity;

    /*! What operations are busy? Bitfield using peerFindRoleAdvertActiveStatus_t 

        This can be used in MessageSendConditional() */
    uint16                      advert_activity;

    /*! What media operations are active? Bitfield using
        peerFindRoleMediaBusyStatus_t

        This can be used in MessageSendConditional() */ 
    uint16                      media_busy;

    /*! The current internal state */
    PEER_FIND_ROLE_STATE        state;

    /*! Role selection server data. Always ready as a server */
    GATT_ROLE_SELECTION_SERVER  role_selection_server;

    /*! Role selection client data (if acting as a client) */
    GATT_ROLE_SELECTION_CLIENT  role_selection_client;

    /*! The selected role for the other peer */
    GattRoleSelectionServiceControlOpCode remote_role;

    /*! Information used by the scoring algorithms when calculating score */
    peer_find_role_scoring_t    scoring_info;

    /*! Prepare for role selection client. */
    task_list_t                 prepare_tasks;

    /*! Cached value of fixed role setting. */
    peer_find_role_fixed_role_t fixed_role;

    /*! Current LE scan mode, high or low duty. */
    scan_state_mode_t scan_mode;

} peerFindRoleTaskData;


/*! Global data for the find role service.

    Although global scope, the structure definition and variable are not visible
    outside of the peer find role service.
*/
extern peerFindRoleTaskData peer_find_role;

/*! Access the task data for the peer find role service */
#define PeerFindRoleGetTaskData()   (&peer_find_role)

/*! Access the task for the peer find role service */
#define PeerFindRoleGetTask()       (&peer_find_role._task)

/*! Access the list of tasks registered with the peer find role service */
#define PeerFindRoleGetTaskList()   (task_list_flexible_t *)(&peer_find_role.registered_tasks)

/*! Access the scoring information for the peer find role service */
#define PeerFindRoleGetScoring()    (&peer_find_role.scoring_info)

/*! Access the scan busy status for the peer find role service */
#define PeerFindRoleGetScanBusy()   (peer_find_role.scan_activity)

/*! Set the scan busy status for the peer find role service 

    \param new_value    Value to set
*/
#define PeerFindRoleSetScanBusy(new_value)  do { \
                                              peer_find_role.scan_activity = (new_value); \
                                            } while(0)

/*! Access the advertising busy status for the peer find role service */
#define PeerFindRoleGetAdvertBusy()   (peer_find_role.advert_activity)

/*! Set the advertising busy status for the peer find role service 

    \param new_value    Value to set
*/
#define PeerFindRoleSetAdvertBusy(new_value)  do { \
                                              peer_find_role.advert_activity = (new_value); \
                                            } while(0)



/*! Message identifiers used for internal messages */
typedef enum
{
        /*! Update the score and notify GATT server (if any) */
    PEER_FIND_ROLE_INTERNAL_UPDATE_SCORE,

        /*! Cancel a find role (if any) */
    PEER_FIND_ROLE_INTERNAL_CANCEL_FIND_ROLE,

        /*! Scanning and advertising stopped */
    PEER_FIND_ROLE_INTERNAL_CONNECT_TO_PEER,

        /*! Telephony is now busy */
    PEER_FIND_ROLE_INTERNAL_TELEPHONY_ACTIVE,

        /*! Telephony is no longer busy */
    PEER_FIND_ROLE_INTERNAL_TELEPHONY_IDLE,

        /*! Audio streaming is now busy */
    PEER_FIND_ROLE_INTERNAL_STREAMING_ACTIVE,

        /*! Audio streaming is no longer busy */
    PEER_FIND_ROLE_INTERNAL_STREAMING_IDLE,

        /*! Now able to enable scanning */
    PEER_FIND_ROLE_INTERNAL_ENABLE_SCANNING,

        /*! Failed to find peer within requested time */
    PEER_FIND_ROLE_INTERNAL_TIMEOUT_CONNECTION = 0x80,

        /*! Timer for starting advertising has finished */
    PEER_FIND_ROLE_INTERNAL_TIMEOUT_ADVERT_BACKOFF,

        /*! Timer for disconnecting link from server side, if not done */
    PEER_FIND_ROLE_INTERNAL_TIMEOUT_NOT_DISCONNECTED,

        /*! Timer for completing the server role selection if the client does
            not tell the server what role it should be in. */
    PEER_FIND_ROLE_INTERNAL_TIMEOUT_SERVER_ROLE_SELECTED,

        /*! Timer in case we don't receive the peer figure of merit */
    PEER_FIND_ROLE_INTERNAL_TIMEOUT_NO_FOM_RECEIVED,

        /*! App has responded to a "prepare for role selection" request */
    PEER_FIND_ROLE_INTERNAL_PREPARED,

} peer_find_role_internal_message_t;


/*! Is this device in the central role ?

    Function that checks whether our device is marked as the central one.

    \todo Since this doesn't change once set may be useful to calculate 
        as part of init. Not the boot init as in that case we may not know C/P.

    \return TRUE if central, FALSE if peripheral or error detected
 */
bool peer_find_role_is_central(void);


/*! Indicate whether peer find role is running.

    \return TRUE if running, FALSE otherwise
*/
bool peer_find_role_is_running(void);

/*! Record that we are advertising in the find role task 
*/
void peer_find_role_advertising_activity_set(void);


/*! Clear the fact that we are advertising in the find role task 

    This may cause a message to be sent
*/
void peer_find_role_advertising_activity_clear(void);


/*! Send the peer_find_role service a message when there are no longer 
    any active activities.

    \param id The message type identifier. 
    \param message The message data (if any).
*/
void peer_find_role_message_send_when_inactive(MessageId id, void *message);


/*! Cancel messages queued for the peer_find_role service if 
    there are still activities active

    \param id The message type identifier. 
*/
void peer_find_role_message_cancel_inactive(MessageId id);

/*! Override the score used in role selection.
 
    Setting a non-zero value will use that value rather than the
    calculated one for future role selection decisions.

    Setting a value of 0, will cancel the override and revert to
    using the calculated score.

    Note that use of this function will not initiate role selection,
    only influence *future* decisions.

    \param score Score to use for role selection decisions.
*/
void PeerFindRole_OverrideScore(grss_figure_of_merit_t score);

/*! Check if there are any blocking media actions active.

    \return FALSE if no media blocking, TRUE otherwise
*/
#define peer_find_role_media_active() (peer_find_role.media_busy != 0)


/*! Check if we are waiting for primary role to be confirmed

    \return TRUE if primary role has been reported previously
*/
#define peer_find_role_awaiting_primary_completion() (peer_find_role.media_busy & PEER_FIND_ROLE_CONFIRMING_PRIMARY)


/*! Check if a client has registered for PREPARE_FOR_ROLE_SELECTION requests.

    \return TRUE if a client is registered, FALSE otherwise.
*/
#define peer_find_role_prepare_client_registered() (TaskList_Size(&peer_find_role.prepare_tasks) != 0)


#endif /* PEER_FIND_ROLE_PRIVATE_H_ */
