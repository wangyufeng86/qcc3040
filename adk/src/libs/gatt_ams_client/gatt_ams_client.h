/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_ams_client.h
@brief   Header file for the GATT AMS client library.

        This file provides documentation for the GATT AMS client library API (library name: gatt_ams_client).
*/

#ifndef GATT_AMS_CLIENT_H_
#define GATT_AMS_CLIENT_H_

#include <csrtypes.h>
#include <message.h>
#include <library.h>
#include <gatt.h>

/*!
 * Invalid Handle Value
 */
#define GATT_AMS_INVALID_HANDLE (0xFFFF)

typedef enum
{
    RemoteCommandIDPlay = 0,
    RemoteCommandIDPause = 1,
    RemoteCommandIDTogglePlayPause = 2,
    RemoteCommandIDNextTrack = 3,
    RemoteCommandIDPreviousTrack = 4,
    RemoteCommandIDVolumeUp = 5,
    RemoteCommandIDVolumeDown = 6,
    RemoteCommandIDAdvanceRepeatMode = 7,
    RemoteCommandIDAdvanceShuffleMode = 8,
    RemoteCommandIDSkipForward = 9,
    RemoteCommandIDSkipBackward = 10,
    RemoteCommandIDLikeTrack = 11,
    RemoteCommandIDDislikeTrack  = 12,
    RemoteCommandIDBookmarkTrack = 13
} gatt_ams_remote_command_id_t;

typedef enum
{
    EntityIDPlayer = 0, /* For its attributes see gatt_ams_player_attribute_id_t */
    EntityIDQueue  = 1, /* For its attributes see gatt_ams_queue_attribute_id_t */
    EntityIDTrack  = 2  /* For its attributes see gatt_ams_track_attribute_id_t */
} gatt_ams_entity_id_t;

/*! @brief Attributes for EntityIDPlayer
*/
typedef enum
{
    /* A string containing the localised name of the application */
    PlayerAttributeIDName = 0,
    /* A concatenation of three comma-separated values:
     *   PlaybackState: A string that represents the integer value of the playback state (see gatt_ams_playback_state_t),
     *   PlaybackRate:  A string that represents the floating point value of the playback rate.
     *   ElapsedTime:   A string that represents the floating point value of the elapsed time of the current track (in seconds) at the moment it was sent. */
    PlayerAttributeIDPlaybackInfo = 1,
    /* A string that represents the floating point value of the volume, ranging from 0 (silent) to 1 (full volume) */
    PlayerAttributeIDVolume = 2
} gatt_ams_player_attribute_id_t;

/*! @brief Attributes for EntityIDQueue
*/
typedef enum
{
    QueueAttributeIDIndex       = 0, /* A string containing the integer value of the queue index (zero-based) */
    QueueAttributeIDCount       = 1, /* A string containing the integer value of the total number of items in the queue */
    QueueAttributeIDShuffleMode = 2, /* A string containing the integer value of the shuffle mode (see gatt_ams_shuffle_mode_t) */
    QueueAttributeIDRepeatMode  = 3  /* A string containing the integer value value of the repeat mode (see gatt_ams_repeat_mode_t) */
} gatt_ams_queue_attribute_id_t;

/*! @brief Attributes for EntityIDTrack
*/
typedef enum
{
    TrackAttributeIDArtist   = 0, /* A string containing the name of the artist */
    TrackAttributeIDAlbum    = 1, /* A string containing the name of the album */
    TrackAttributeIDTitle    = 2, /* A string containing the title of the track */
    TrackAttributeIDDuration = 3  /* A string containing the floating point value of the total duration of the track in seconds */
} gatt_ams_track_attribute_id_t;

typedef enum
{
    PlaybackStatePaused = 0,
    PlaybackStatePlaying = 1,
    PlaybackStateRewinding = 2,
    PlaybackStateFastForwarding = 3
} gatt_ams_playback_state_t;

typedef enum
{
    ShuffleModeOff = 0,
    ShuffleModeOne = 1,
    ShuffleModeAll = 2
} gatt_ams_shuffle_mode_t;

typedef enum
{
    RepeatModeOff = 0,
    RepeatModeOne = 1,
    RepeatModeAll = 2
} gatt_ams_repeat_mode_t;

typedef enum
{
    EntityUpdateFlagTruncated = (1 << 0)
} gatt_ams_entity_update_flags_t;

/*! @brief Status code returned from the GATT AMS client library.
*/
typedef enum
{
    gatt_ams_status_failed,
    gatt_ams_status_success,
    gatt_ams_status_initiating,
    gatt_ams_status_invalid_parameter,
    gatt_ams_status_not_supported,
    gatt_ams_status_unknown_command,
    gatt_ams_status_invalid_command,
    gatt_ams_status_action_failed
} gatt_ams_status_t;

/*! @brief Enumeration of messages a client task may receive from the AMS client library.
 */
typedef enum
{
    GATT_AMS_CLIENT_INIT_CFM = GATT_AMS_CLIENT_MESSAGE_BASE,
    GATT_AMS_CLIENT_SET_REMOTE_COMMAND_NOTIFICATION_CFM,
    GATT_AMS_CLIENT_SET_ENTITY_UPDATE_NOTIFICATION_CFM,
    GATT_AMS_CLIENT_WRITE_REMOTE_COMMAND_CFM,
    GATT_AMS_CLIENT_WRITE_ENTITY_UPDATE_CFM,
    GATT_AMS_CLIENT_WRITE_ENTITY_ATTRIBUTE_CFM,
    GATT_AMS_CLIENT_READ_ENTITY_ATTRIBUTE_CFM,

    /* Library message id limit */
    GATT_AMS_CLIENT_MESSAGE_TOP
} gatt_ams_client_message_id_t;

/*!
   @brief The ams notification internal structure for the client role.
    This structure is visible to the application as it is responsible for managing resource to pass to the ams client library.
    The elements of this structure are ONLY modified by the ams client library.
    The application SHOULD NOT modify the values at any time as it could lead to undefined behaviour.
 */
typedef struct
{
    TaskData   lib_task;
    uint16     cid;
    uint16     remote_command_handle;
    /*! Client characteristic descriptor handle for Remote Command */
    uint16     remote_command_ccd;
    uint16     entity_attribute_handle;
    uint16     entity_update_handle;
    /*! Client characteristic descriptor handle for Entity Update */
    uint16     entity_update_ccd;
    uint16     pending_cmd;
    Task       task_pending_cfm;
    /*! Mask to report the order of characteristic received */
    uint8      char_report_mask;
    /*! Index to extract the order of characteristic received */
    uint8      char_report_mask_index;
} GAMS;

typedef struct
{
    const GAMS       *ams;
    gatt_ams_status_t status;
} GATT_AMS_CLIENT_INIT_CFM_T;

typedef struct
{
    const GAMS       *ams;
    uint16            cid;
    gatt_ams_status_t status;
    gatt_status_t     gatt_status;
} GATT_AMS_CLIENT_SET_REMOTE_COMMAND_NOTIFICATION_CFM_T;

typedef struct
{
    const GAMS       *ams;
    uint16            cid;
    gatt_ams_status_t status;
    gatt_status_t     gatt_status;
} GATT_AMS_CLIENT_SET_ENTITY_UPDATE_NOTIFICATION_CFM_T;

typedef struct
{
    const GAMS       *ams;
    uint16            cid;
    gatt_ams_status_t status;
    gatt_status_t     gatt_status;
} GATT_AMS_CLIENT_WRITE_REMOTE_COMMAND_CFM_T;

typedef struct
{
    const GAMS       *ams;
    uint16            cid;
    gatt_ams_status_t status;
    gatt_status_t     gatt_status;
} GATT_AMS_CLIENT_WRITE_ENTITY_UPDATE_CFM_T;

typedef struct
{
    const GAMS       *ams;
    uint16            cid;
    gatt_ams_status_t status;
    gatt_status_t     gatt_status;
} GATT_AMS_CLIENT_WRITE_ENTITY_ATTRIBUTE_CFM_T;

typedef struct
{
    const GAMS       *ams;
    uint16            cid;
    gatt_ams_status_t status;
    gatt_status_t     gatt_status;
    uint16            value_size;
    uint8             value[1];
} GATT_AMS_CLIENT_READ_ENTITY_ATTRIBUTE_CFM_T;

typedef void (*gatt_ams_ready_state_observer_t)(const GAMS *ams, bool is_ready);

/*!
    @brief After the VM application has used the GATT manager library to establish a connection to a discovered BLE device in the Client role,
    it can discover any supported services in which it has an interest. It should then register with the relevant client service library
    (passing the relevant CID and handles to the service). For the AMS client it will use this API. The GATT manager
    will then route notifications and indications to the correct instance of the client service library for the CID.

    @param task The Task that will receive the GATT_AMS_CLIENT_INIT_CFM message.
    @param ams A valid area of memory that the service library can use.
    @param cid The connection ID.
    @param start_handle The start handle of the AMS client instance.
    @param end_handle The end handle of the AMS client instance.

    @return The status result of calling the API.
*/
bool GattAmsInit(Task task, uint16 cid, uint16 start_handle, uint16 end_handle, void *ams_data_dynamic, void* ams_data_const);

/*!
    @brief When a GATT connection is removed, the application must remove all client service instances that were
    associated with the connection (using the CID value).
    This is the clean up routine as a result of calling the GattAmsInit API.

    @param ams The client instance to destroy.

    @return The status result of calling the API.
*/
bool GattAmsDestroy(void *ams_client_dysnamic_data);

/*!
    @brief Use to enable/disable notifications for Remote Command characteristic on the server.
    An error will be returned if the server does not support notifications.

    @param task The task to receive the GATT_AMS_CLIENT_SET_REMOTE_COMMAND_NOTIFICATION_CFM message.
    @param ams The client instance.
    @param notifications_enable Set to TRUE to enable notifications, FALSE to disable them.
*/
void GattAmsSetRemoteCommandNotificationEnableRequest(Task task, const GAMS *ams, bool notifications_enable);

/*!
    @brief Use to enable/disable notifications for Entity Update Characteristic on the server.
    An error will be returned if the server does not support notifications.

    @param task The task to receive the GATT_AMS_CLIENT_SET_ENTITY_UPDATE_NOTIFICATION_CFM message.
    @param ams The client instance.
    @param notifications_enable Set to TRUE to enable notifications, FALSE to disable them.
*/
void GattAmsSetEntityUpdateNotificationEnableRequest(Task task, const GAMS *ams, bool notifications_enable);

/*!
    @brief Use to write the Remote Command characteristic.

    @param task The task to receive the GATT_AMS_CLIENT_WRITE_REMOTE_COMMAND_CFM message.
    @param ams The client instance.
    @param remote_command_id The remote command value to write.
*/
void GattAmsWriteRemoteCommand(Task task, const GAMS *ams, gatt_ams_remote_command_id_t remote_command_id);

/*!
    @brief Use to write the Entity Update characteristic.

    @param task The task to receive the GATT_AMS_CLIENT_WRITE_ENTITY_UPDATE_CFM message.
    @param ams The client instance.
    @param entity_id The entity whose attributes we are interested in.
    @param attribute_list The list of attributes of that entity we want to enable updates for.
    @param size_attribute_list The size of the attribute_list.
*/
void GattAmsWriteEntityUpdate(Task task, const GAMS *ams, gatt_ams_entity_id_t entity_id, const uint8 *attribute_id_list, uint16 size_attribute_list);

/*!
    @brief Use to write the Entity Attribute characteristic.

    @param task The task to receive the GATT_AMS_CLIENT_WRITE_ENTITY_ATTRIBUTE_CFM message.
    @param ams The client instance.
    @param entity_id The entity whose attribute we are interested in.
    @param attribute_id The attribute whose extended value we want to retrieve.
*/
void GattAmsWriteEntityAttribute(Task task, const GAMS *ams, gatt_ams_entity_id_t entity_id, uint8 attribute_id);

/*!
    @brief Use to read the Entity Attribute characteristic.

    @param task The task to receive the GATT_AMS_CLIENT_READ_ENTITY_ATTRIBUTE_CFM message.
    @param ams The client instance.
*/
void GattAmsReadEntityAttribute(Task task, const GAMS *ams);

/*!
    @brief The function passed will be called when the ready state of a client instance changes.
    Ready means a client instance has just been fully initialised.
    Non-ready means a client instance is going to be destroyed immediately after this call.

    @param observer The function called when the ready state changes.

    @return TRUE when the observer has been successfully added, FALSE otherwise.
*/
bool GattAmsAddReadyStateObserver(gatt_ams_ready_state_observer_t observer);

/*!
    @brief Get the Remote Command characteristic handle.

    @param ams The client instance.

    @return The handle value on success, GATT_AMS_INVALID_HANDLE otherwise (Characteristic is not supported by the server, client initialisation is not completed).
*/
uint16 GattAmsGetRemoteCommandHandle(const GAMS *ams);

/*!
    @brief Get the Entity Update characteristic handle.

    @param ams The client instance.

    @return The handle value on success, GATT_AMS_INVALID_HANDLE otherwise (Characteristic is not supported by the server, client initialisation is not completed).
*/
uint16 GattAmsGetEntityUpdateHandle(const GAMS *ams);

/*!
    @brief Get the connection id used by the client.

    @param ams The client instance.

    @return The connection id (cid).
*/
uint16 GattAmsGetConnectionId(const GAMS *ams);

#endif /* GATT_AMS_CLIENT_H_ */
