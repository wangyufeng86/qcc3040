/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       profile_manager.c
\brief      This is profile manager module which provides APIs to different applications
            to manage profiles connections with different devices.

    These apis are used by application to register different profile connection
    handlers with the profile manager as well as invoking them when application
    wants to connect all supported profiles for that device.Profile manager will
    handle connection crossover as well as is able to connect profiles in any order
    application wants.
*/

#include "profile_manager.h"

#include <logging.h>
#include <panic.h>

#include <av.h>
#include <device_list.h>
#include <device_properties.h>
#include <hfp_profile.h>
#include <task_list.h>
#include <timestamp_event.h>

#define PROFILE_MANAGER_LOG(...)   DEBUG_LOG(__VA_ARGS__)

profile_manager_task_data profile_manager;

typedef enum
{
    profile_manager_connect,
    profile_manager_disconnect
} profile_manager_request_type_t;

typedef enum {
   request_succeeded,
   request_failed
} profile_request_status_t;

typedef struct
{
    profile_manager_registered_connect_request_t connect_req_fp;
    profile_manager_registered_disconnect_request_t disconnect_req_fp;
} profile_manager_registered_profile_interface_callbacks_t;

/*! \brief array of function pointers used for making profile connection/disconnection requests. */
profile_manager_registered_profile_interface_callbacks_t profile_manager_interface_callbacks[profile_manager_max_number_of_profiles];

static profile_t ordered_profiles_list[profile_manager_max_number_of_profiles] =
{
    profile_manager_hfp_profile,
    profile_manager_a2dp_profile,
    profile_manager_avrcp_profile
};

typedef struct
{
    device_t device;
    uint8 profile;
    profile_manager_request_type_t type;
} profile_manager_connect_next_on_ind_params;

/*******************************************************************************************************/

static void profileManager_SendConnectedInd(device_t device, unsigned profile)
{
    MESSAGE_MAKE(msg, CONNECTED_PROFILE_IND_T);
    msg->device = device;
    msg->profile = profile;
    TaskList_MessageSendWithSize(TaskList_GetFlexibleBaseTaskList(ProfileManager_GetClientTasks()), CONNECTED_PROFILE_IND, msg, sizeof(*msg));
}

static void profileManager_SendDisconnectedInd(device_t device, unsigned profile, profile_manager_disconnected_ind_reason_t reason)
{
    MESSAGE_MAKE(msg, DISCONNECTED_PROFILE_IND_T);
    msg->device = device;
    msg->profile = profile;
    msg->reason = reason;
    TaskList_MessageSendWithSize(TaskList_GetFlexibleBaseTaskList(ProfileManager_GetClientTasks()), DISCONNECTED_PROFILE_IND, msg, sizeof(*msg));
}

static task_list_t* profileManager_GetRequestTaskList(profile_manager_request_type_t type)
{
    profile_manager_task_data * pm = ProfileManager_GetTaskData();
    task_list_with_data_t * request_client = (type == profile_manager_connect) ? &pm->pending_connect_reqs : &pm->pending_disconnect_reqs;
    return TaskList_GetBaseTaskList(request_client);
}

static uint8 profileManager_FindIndexOfProfileInOrderedList(profile_t profile)
{
    int index = 0;
    for(index = 0; index<profile_manager_max_number_of_profiles; index++)
    {
       if(ordered_profiles_list[index] == profile)
       {
           break;
       }
    }
    PanicFalse(index != profile_manager_max_number_of_profiles);
    return index;
}

static uint8 profileManager_GetNextProfileRequestIndex(device_t device)
{
    uint8 profile_connect_index = 0;
    if (Device_GetPropertyU8(device, device_property_profile_request_index, &profile_connect_index))
    {
        profile_connect_index++;
    }
    Device_SetPropertyU8(device, device_property_profile_request_index, profile_connect_index);

    DEBUG_LOG("profileManager_GetNextProfileRequestIndex index=%d", profile_connect_index);

    return profile_connect_index;
}

static bool profileManager_GetProfileRequestOrder(device_t device,
                                                  profile_manager_request_type_t type,
                                                  profile_t** profiles_connecting_order,
                                                  uint8 *profiles_order_list_size)
{
    bool is_order_valid = FALSE;
    void * profiles_order = NULL;
    size_t property_size;
    device_property_t order_property = (type == profile_manager_connect) ?
                device_property_profiles_connect_order : device_property_profiles_disconnect_order;

    is_order_valid = Device_GetProperty(device, order_property, &profiles_order, &property_size);

    *profiles_order_list_size = property_size;
    *profiles_connecting_order = (profile_t*) profiles_order;
    return is_order_valid;
}

static bool profileManager_IsAlreadyConnected(uint8 connected_profiles, profile_t requested_profile)
{
    bool already_connected = FALSE;
    switch (requested_profile)
    {
    case profile_manager_hfp_profile:
        if (DEVICE_PROFILE_HFP & connected_profiles)
            already_connected = TRUE;
        break;
    case profile_manager_a2dp_profile:
        if (DEVICE_PROFILE_A2DP & connected_profiles)
            already_connected = TRUE;
        break;
    case profile_manager_avrcp_profile:
        if (DEVICE_PROFILE_AVRCP & connected_profiles)
            already_connected = TRUE;
        break;
    default:
        break;
    }
    return already_connected;
}

static profile_manager_disconnected_ind_reason_t profileManager_ConvertHfpDisconnectReason(appHfpDisconnectReason hfp_reason)
{
    profile_manager_disconnected_ind_reason_t reason;
    switch (hfp_reason)
    {
    case APP_HFP_DISCONNECT_NORMAL:
        reason = profile_manager_disconnected_normal;
        break;
    case APP_HFP_DISCONNECT_LINKLOSS:
        reason = profile_manager_disconnected_link_loss;
        break;
    case APP_HFP_DISCONNECT_ERROR:
    default:
        reason = profile_manager_disconnected_error;
        break;
    }
    return reason;
}

static profile_manager_disconnected_ind_reason_t profileManager_ConvertA2dpDisconnectReason(avA2dpDisconnectReason hfp_reason)
{
    profile_manager_disconnected_ind_reason_t reason;
    switch (hfp_reason)
    {
    case AV_A2DP_DISCONNECT_NORMAL:
        reason = profile_manager_disconnected_normal;
        break;
    case AV_A2DP_DISCONNECT_LINKLOSS:
        reason = profile_manager_disconnected_link_loss;
        break;
    case AV_A2DP_DISCONNECT_ERROR:
    default:
        reason = profile_manager_disconnected_error;
        break;
    }
    return reason;
}

static profile_t profileManager_GetNextProfile(device_t device, profile_manager_request_type_t type)
{
    profile_t * profiles_order = NULL;
    uint8 profiles_order_list_size = 0;
    profile_t profile = profile_manager_bad_profile;

    if (profileManager_GetProfileRequestOrder(device, type, &profiles_order, &profiles_order_list_size))
    {
        uint8 profile_request_index = profileManager_GetNextProfileRequestIndex(device);
        PanicFalse(profile_request_index != profiles_order_list_size);

        profile = profiles_order[profile_request_index];
    }

    DEBUG_LOG("profileManager_GetNextProfile profile=%d, disconnect=%d", profile, type);

    return profile;
}

static profile_t profileManager_GetCurrentProfile(device_t device, profile_manager_request_type_t type, uint8 profile_request_index)
{
    profile_t * profiles_order = NULL;
    uint8 profiles_order_list_size = 0;
    profile_t profile = profile_manager_bad_profile;

    if (profileManager_GetProfileRequestOrder(device, type, &profiles_order, &profiles_order_list_size))
    {
        PanicFalse(profile_request_index != profiles_order_list_size);

        profile = profiles_order[profile_request_index];
    }

    DEBUG_LOG("profileManager_GetCurrentProfile profile=%d, disconnect=%d", profile, type);

    return profile;
}

static bool profileManager_FindClientSendConnectCfm(Task task, task_list_data_t *data, void *arg)
{
    bool found_client_task  = FALSE;
    profile_manager_send_client_cfm_params * params = (profile_manager_send_client_cfm_params *)arg;

    if (data->ptr == params->device)
    {
        found_client_task = TRUE;
        MESSAGE_MAKE(msg, CONNECT_PROFILES_CFM_T);
        msg->device = params->device;
        msg->result = params->result;

        DEBUG_LOG("profileManager_FindClientSendConnectCfm toTask=%x result=%d", task, params->result);
        MessageSend(task, CONNECT_PROFILES_CFM, msg);

        profile_manager_task_data * pm = ProfileManager_GetTaskData();
        TaskList_RemoveTask(TaskList_GetBaseTaskList(&pm->pending_connect_reqs), task);
    }
    return !found_client_task;
}

static bool profileManager_FindClientSendDisconnectCfm(Task task, task_list_data_t *data, void *arg)
{
    bool found_client_task  = FALSE;
    profile_manager_send_client_cfm_params * params = (profile_manager_send_client_cfm_params *)arg;

    if (data->ptr == params->device)
    {
        found_client_task = TRUE;
        MESSAGE_MAKE(msg, DISCONNECT_PROFILES_CFM_T);
        msg->device = params->device;
        msg->result = params->result;

        DEBUG_LOG("profileManager_FindClientSendDisconnectCfm toTask=%x result=%d", task, params->result);
        MessageSend(task, DISCONNECT_PROFILES_CFM, msg);

        profile_manager_task_data * pm = ProfileManager_GetTaskData();
        TaskList_RemoveTask(TaskList_GetBaseTaskList(&pm->pending_disconnect_reqs), task);
    }
    return !found_client_task;
}

static void profileManager_SendConfirm(device_t device, profile_manager_request_type_t type, profile_manager_request_cfm_result_t result)
{
    task_list_t * req_task_list = profileManager_GetRequestTaskList(type);

    profile_manager_send_client_cfm_params params = {0};
    params.device = device;
    params.result = result;

    if (type == profile_manager_connect)
    {
        TaskList_IterateWithDataRawFunction(req_task_list, profileManager_FindClientSendConnectCfm, &params);
    }
    else
    {
        TaskList_IterateWithDataRawFunction(req_task_list, profileManager_FindClientSendDisconnectCfm, &params);
    }
    Device_RemoveProperty(device, device_property_profile_request_index);
}

static inline bool profileManager_IsRequestComplete(profile_t profile)
{
    return (profile >= profile_manager_max_number_of_profiles);
}

static void profileManager_IssueNextProfileRequest(device_t device, profile_t profile, profile_manager_request_type_t type)
{
    profile_manager_task_data * pm = ProfileManager_GetTaskData();
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);

    DEBUG_LOG("profileManager_IssueNextProfileRequest bdaddr nap=%x uap=%x lap=%x, profile=%d, disconnect=%d",
              bd_addr.nap, bd_addr.uap, bd_addr.lap, profile, type );

    /* profile must always be a valid value otherwise the array de-reference
       below be past the end of the array */
    PanicFalse(profile < profile_manager_max_number_of_profiles);

    if (type == profile_manager_connect)
    {
        profile_manager_interface_callbacks[profile].connect_req_fp(&pm->task, &bd_addr);
    }
    else
    {
        profile_manager_interface_callbacks[profile].disconnect_req_fp(&pm->task, &bd_addr);
    }
}

static profile_t profileManager_ConsumeRequestsForProfilesAlreadyInRequestedState( device_t device, profile_manager_request_type_t type)
{
    bool requested_profile_is_connected = (type == profile_manager_connect) ? TRUE : FALSE;
    uint8 connected_profiles = BtDevice_GetConnectedProfiles(device);
    profile_t profile = profileManager_GetNextProfile(device, type);

    while ((profileManager_IsAlreadyConnected(connected_profiles, profile) == requested_profile_is_connected) && (profile < profile_manager_max_number_of_profiles))
    {
        DEBUG_LOG("profileManager_ConsumeRequestsForProfilesAlreadyInRequestedState profile=%d consumed state=%d",
                  profile, requested_profile_is_connected);
        profile = profileManager_GetNextProfile(device, type);
    }
    return profile;
}

static bool profileManager_NextProfile(device_t device, profile_manager_request_type_t type)
{
    bool is_success = FALSE;
    profile_t profile = profileManager_ConsumeRequestsForProfilesAlreadyInRequestedState(device, type);

    if (profile == profile_manager_bad_profile)
        return is_success;

    if (profileManager_IsRequestComplete(profile))
    {
        profileManager_SendConfirm(device, type, profile_manager_success);
    }
    else
    {
        profileManager_IssueNextProfileRequest(device, profile, type);
    }
    is_success = TRUE;

    return is_success;
}

static bool profileManager_IndWasExpectedProfile(uint8 profile, profile_t last_requested_profile)
{
    profile_t ind_profile;
    switch(profile)
    {
    case DEVICE_PROFILE_HFP:
        ind_profile = profile_manager_hfp_profile;
        break;
    case DEVICE_PROFILE_A2DP:
        ind_profile = profile_manager_a2dp_profile;
        break;
    case DEVICE_PROFILE_AVRCP:
        ind_profile = profile_manager_avrcp_profile;
        break;
    case DEVICE_PROFILE_SCOFWD:
    case DEVICE_PROFILE_PEERSIG:
    default:
        ind_profile = profile_manager_bad_profile;
        break;
    }
    DEBUG_LOG("profileManager_IndWasExpectedProfile ind_profile=%d,last_requested_profile=%d", ind_profile, last_requested_profile);
    return (ind_profile == last_requested_profile);
}

static profile_t profileManager_GetPendingRequestProfile(device_t device, profile_manager_request_type_t type)
{
    profile_t * profiles_request_order = NULL;
    uint8 profiles_order_list_size = 0;
    profile_t profile = profile_manager_bad_profile;

    if (profileManager_GetProfileRequestOrder(device, type, &profiles_request_order, &profiles_order_list_size))
    {
        uint8 profile_request_index = 0;
        if (Device_GetPropertyU8(device, device_property_profile_request_index, &profile_request_index))
        {
            DEBUG_LOG("profileManager_GetPendingRequestProfile profile_request_index=%d", profile_request_index);
            PanicFalse(profile_request_index != profiles_order_list_size);
            profile = profiles_request_order[profile_request_index];
        }
    }
    return profile;
}

static bool profileManager_RequestNextProfileOnIndication(Task task, task_list_data_t *data, void *arg)
{
    bool found_a_client_req_pending = FALSE;
    profile_manager_connect_next_on_ind_params *params = (profile_manager_connect_next_on_ind_params *)arg;

    UNUSED(task);

    if (data->ptr == params->device)
    {
        profile_t last_profile_requested = profileManager_GetPendingRequestProfile(params->device, params->type);
        if (profileManager_IndWasExpectedProfile(params->profile, last_profile_requested))
        {
            found_a_client_req_pending = TRUE;
            profileManager_NextProfile(params->device, params->type);
        }
    }
    return !found_a_client_req_pending;
}

static void profileManager_UpdateProfileRequestIndexOnFailure(device_t device)
{
    uint8 profile_request_index = 0;
    if (Device_GetPropertyU8(device, device_property_profile_request_index, &profile_request_index))
    {
        DEBUG_LOG("profileManager_UpdateProfileRequestIndexOnFailure profile_request_index %u", profile_request_index);

        if (profile_request_index != 0)
        {
            profile_request_index--;
            Device_SetPropertyU8(device, device_property_profile_request_index, profile_request_index);
        }
        else
        {
            Device_RemoveProperty(device, device_property_profile_request_index);
        }
    }
}

static void profileManager_UpdateConnectedProfilesProperty(device_t device, unsigned profile, profile_manager_request_type_t type)
{
    uint8 connected_mask = BtDevice_GetConnectedProfiles(device);
    (type == profile_manager_connect) ? (connected_mask |= profile) : (connected_mask &= ~profile);
    BtDevice_SetConnectedProfiles(device, connected_mask);

    switch (profile)
    {
        case DEVICE_PROFILE_A2DP:
            TimestampEvent(type == profile_manager_connect ? TIMESTAMP_EVENT_PROFILE_CONNECTED_A2DP :
                                                             TIMESTAMP_EVENT_PROFILE_DISCONNECTED_A2DP);
            break;
        case DEVICE_PROFILE_AVRCP:
            TimestampEvent(type == profile_manager_connect ? TIMESTAMP_EVENT_PROFILE_CONNECTED_AVRCP :
                                                             TIMESTAMP_EVENT_PROFILE_DISCONNECTED_AVRCP);
            break;
        case DEVICE_PROFILE_HFP:
            TimestampEvent(type == profile_manager_connect ? TIMESTAMP_EVENT_PROFILE_CONNECTED_HFP :
                                                             TIMESTAMP_EVENT_PROFILE_DISCONNECTED_HFP);
            break;
        default:
            DEBUG_LOG("profileManager_UpdateConnectedProfilesProperty unrecorded profile %u timestmap", profile);
            break;
    }

    DEBUG_LOG("profileManager_UpdateConnectedProfilesProperty type=%d connected_mask=%x", type, connected_mask );
}

static void profileManager_UpdateProfileRequestStatus(device_t device, uint8 profile, profile_request_status_t status, profile_manager_request_type_t type)
{
    task_list_t * req_task_list = profileManager_GetRequestTaskList(type);

    PanicNull(device);

    if (status == request_succeeded)
    {
        DEBUG_LOG("profileManager_HandleProfileRequestStatus Ok");
        profile_manager_connect_next_on_ind_params params = {0};
        params.device = device;
        params.profile = profile;
        params.type = type;

        /* If there is a pending request for this device, then if this was the profile last requested, then continue to the next profile */
        TaskList_IterateWithDataRawFunction(req_task_list, profileManager_RequestNextProfileOnIndication, &params);
    }
    else
    {
        profile_manager_send_client_cfm_params params = {0};
        params.device = device;
        params.result = profile_manager_failed;

        profileManager_UpdateProfileRequestIndexOnFailure(device);

        if (type == profile_manager_connect)
        {
            TaskList_IterateWithDataRawFunction(req_task_list, profileManager_FindClientSendConnectCfm, &params);
        }
        else
        {
            TaskList_IterateWithDataRawFunction(req_task_list, profileManager_FindClientSendDisconnectCfm, &params);
        }
    }
}

static void profileManager_HandleProfileStatusChange(device_t device, unsigned profile, profile_manager_request_type_t type, profile_request_status_t status)
{
    DEBUG_LOG("profileManager_HandleProfileStatusChange device %p profile 0x%x type %u status %d",
              device, profile, type, status);

    if (status == request_succeeded)
    {
        profileManager_UpdateConnectedProfilesProperty(device, profile, type);
    }

    profileManager_UpdateProfileRequestStatus(device, profile, status, type);
}

static void profileManager_HandleConnectedProfileInd(bdaddr* bd_addr, unsigned profile, profile_request_status_t status)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);

    DEBUG_LOG("profileManager_HandleConnectedProfileInd bdaddr nap=%x uap=%x lap=%x, profile=%x, status=%d",
              bd_addr->nap, bd_addr->uap, bd_addr->lap, profile, status);

    profileManager_HandleProfileStatusChange(device, profile, profile_manager_connect, status);

    /*
     * Send CONNECTED_PROFILE_IND only in case of successful connection else this
     * would put the handset service state machine in a bad state.
     */
    if(status == request_succeeded)
    {
        profileManager_SendConnectedInd(device, profile);
    }
}

static void profileManager_HandleDisconnectedProfileInd(bdaddr* bd_addr, unsigned profile, profile_request_status_t status, profile_manager_disconnected_ind_reason_t reason)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);

    DEBUG_LOG("profileManager_HandleDisconnectedProfileInd bdaddr nap=%x uap=%x lap=%x, profile=0x%x",
              bd_addr->nap, bd_addr->uap, bd_addr->lap, profile );

    profileManager_HandleProfileStatusChange(device, profile, profile_manager_disconnect, status);
    profileManager_SendDisconnectedInd(device, profile, reason);
}

static void profileManager_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG("profileManager_HandleMessage id=%d", id);

    switch (id)
    {
    case APP_HFP_CONNECT_CFM:
    {
        APP_HFP_CONNECT_CFM_T *msg = (APP_HFP_CONNECT_CFM_T *)message;
        profileManager_HandleProfileStatusChange(msg->device, DEVICE_PROFILE_HFP, profile_manager_connect, msg->successful ? request_succeeded : request_failed );
        break;
    }
    case AV_A2DP_CONNECT_CFM:
    {
        AV_A2DP_CONNECT_CFM_T *msg = (AV_A2DP_CONNECT_CFM_T *)message;
        profileManager_HandleProfileStatusChange(msg->device, DEVICE_PROFILE_A2DP, profile_manager_connect, msg->successful ? request_succeeded : request_failed );
        break;
    }
    case AV_AVRCP_CONNECT_CFM_PROFILE_MANAGER:
    {
        AV_AVRCP_CONNECT_CFM_PROFILE_MANAGER_T *msg = (AV_AVRCP_CONNECT_CFM_PROFILE_MANAGER_T *)message;
        profileManager_HandleProfileStatusChange(msg->device, DEVICE_PROFILE_AVRCP, profile_manager_connect, msg->successful ? request_succeeded : request_failed );
        break;
    }
    case APP_HFP_CONNECTED_IND:
    {
        APP_HFP_CONNECTED_IND_T *msg = (APP_HFP_CONNECTED_IND_T *)message;
        profileManager_HandleConnectedProfileInd(&msg->bd_addr, DEVICE_PROFILE_HFP, request_succeeded);
        break;
    }
    case APP_HFP_DISCONNECT_CFM:
    {
        APP_HFP_DISCONNECT_CFM_T *msg = (APP_HFP_DISCONNECT_CFM_T *)message;
        profileManager_HandleProfileStatusChange(msg->device, DEVICE_PROFILE_HFP, profile_manager_disconnect, msg->successful ? request_succeeded : request_failed );
        break;
    }
    case AV_A2DP_DISCONNECT_CFM:
    {
        AV_A2DP_DISCONNECT_CFM_T *msg = (AV_A2DP_DISCONNECT_CFM_T *)message;
        profileManager_HandleProfileStatusChange(msg->device, DEVICE_PROFILE_A2DP, profile_manager_disconnect, msg->successful ? request_succeeded : request_failed );
        break;
    }
    case AV_AVRCP_DISCONNECT_CFM:
    {
        AV_AVRCP_DISCONNECT_CFM_T *msg = (AV_AVRCP_DISCONNECT_CFM_T *)message;
        profileManager_HandleProfileStatusChange(msg->device, DEVICE_PROFILE_AVRCP, profile_manager_disconnect, msg->successful ? request_succeeded : request_failed );
        break;
    }
    case APP_HFP_DISCONNECTED_IND:
    {
        APP_HFP_DISCONNECTED_IND_T *msg = (APP_HFP_DISCONNECTED_IND_T *)message;
        if (msg->reason == APP_HFP_CONNECT_FAILED)
        {
            profileManager_HandleConnectedProfileInd(&msg->bd_addr, DEVICE_PROFILE_HFP, request_failed);
        }
        else
        {
            profile_manager_disconnected_ind_reason_t reason = profileManager_ConvertHfpDisconnectReason(msg->reason);

            /* Treat link loss and error conditions as successful disconnections if a disconnect request had been sent. */
            profileManager_HandleDisconnectedProfileInd(&msg->bd_addr, DEVICE_PROFILE_HFP, request_succeeded, reason );
        }
        break;
    }
    case AV_A2DP_CONNECTED_IND:
    {
        AV_A2DP_CONNECTED_IND_T *msg = (AV_A2DP_CONNECTED_IND_T *)message;
        profileManager_HandleConnectedProfileInd(&msg->bd_addr, DEVICE_PROFILE_A2DP, request_succeeded);
        break;
    }
    case AV_A2DP_DISCONNECTED_IND:
    {
        AV_A2DP_DISCONNECTED_IND_T *msg = (AV_A2DP_DISCONNECTED_IND_T *)message;
        profile_manager_disconnected_ind_reason_t reason = profileManager_ConvertA2dpDisconnectReason(msg->reason);
        profileManager_HandleDisconnectedProfileInd(&msg->bd_addr, DEVICE_PROFILE_A2DP, request_succeeded, reason);
        break;
    }
    case AV_AVRCP_CONNECTED_IND:
    {
        AV_AVRCP_CONNECTED_IND_T *msg = (AV_AVRCP_CONNECTED_IND_T *)message;
        profileManager_HandleConnectedProfileInd(&msg->bd_addr, DEVICE_PROFILE_AVRCP, request_succeeded);
        break;
    }
    case AV_AVRCP_DISCONNECTED_IND:
    {
        AV_AVRCP_DISCONNECTED_IND_T *msg = (AV_AVRCP_DISCONNECTED_IND_T *)message;
        profileManager_HandleDisconnectedProfileInd(&msg->bd_addr, DEVICE_PROFILE_AVRCP, request_succeeded, profile_manager_disconnected_normal);
        break;
    }
    case AV_AVRCP_CONNECT_IND:
    {
        /* Needed to allow AVRCP to get setup properly */
        profile_manager_task_data * pm = ProfileManager_GetTaskData();
        AV_AVRCP_CONNECT_IND_T *msg = (AV_AVRCP_CONNECT_IND_T *)message;
        appAvAvrcpConnectResponse(&pm->task, &pm->task,
                                  &msg->bd_addr, msg->connection_id,
                                  msg->signal_id, AV_AVRCP_REJECT);
        break;
    }
    default:
        break;
    }
}

static void profileManager_checkForAndCancelPendingProfileRequest(device_t device, profile_manager_request_type_t type)
{
    uint8 request_index = 0;
    if (Device_GetPropertyU8(device, device_property_profile_request_index, &request_index))
    {
        /* We are in the middle of a pending profile request that needs to be cancelled */
        profile_manager_request_type_t request_to_cancel_type = (type == profile_manager_connect) ?
                    profile_manager_disconnect : profile_manager_connect;
        device_property_t order_property_to_cancel = (request_to_cancel_type == profile_manager_connect) ?
                    device_property_profiles_connect_order : device_property_profiles_disconnect_order;

        /* Cancel the last issued request by calling it's opposite request type API. */
        profile_t profile_to_cancel = profileManager_GetCurrentProfile(device, request_to_cancel_type, request_index);

        DEBUG_LOG("profileManager_checkForAndCancelPendingProfileRequest profile_to_cancel=%d, connect=%d", profile_to_cancel, type);

        /* Only cancel if there is a valid profile to cancel */
        if (profile_to_cancel != profile_manager_bad_profile)
        {
            profileManager_IssueNextProfileRequest(device, profile_to_cancel, type);
        }

        /* Clear the previous request order at current index to prevent any further profiles being requested. */
        Device_RemoveProperty(device, device_property_profile_request_index);
        Device_RemoveProperty(device, order_property_to_cancel);

        /* Send request confirmation cancelled to client task */
        profileManager_SendConfirm(device, request_to_cancel_type, profile_manager_cancelled);
    }
}

static bool profileManager_HandleRequest(Task client, device_t device, profile_manager_request_type_t type)
{
    bool request_created = FALSE;
    task_list_t * req_task_list = profileManager_GetRequestTaskList(type);

    PanicNull(device);

    DEBUG_LOG("profileManager_HandleRequest type=%d, device=%x", type, device);

    if (TaskList_IsTaskOnList(req_task_list, client))
    {
        /* This client still has an outstanding request in progress - ignore the new request. */
        DEBUG_LOG_WARN("profileManager_HandleRequest Previous request from client %p still in progress", client);
    }
    else
    {
        profileManager_checkForAndCancelPendingProfileRequest(device, type);

        /* Store device and client for the pending request */
        task_list_data_t device_used = {0};
        device_used.ptr = device;
        TaskList_AddTaskWithData(req_task_list, client, &device_used);

        request_created = profileManager_NextProfile(device, type);
    }
    return request_created;
}

void ProfileManager_RegisterProfile(profile_t profile,
                                    profile_manager_registered_connect_request_t connect,
                                    profile_manager_registered_disconnect_request_t disconnect)
{
    if(profile < profile_manager_max_number_of_profiles)
    {
        uint8 index = profileManager_FindIndexOfProfileInOrderedList(profile);
        profile_manager_interface_callbacks[index].connect_req_fp = connect;
        profile_manager_interface_callbacks[index].disconnect_req_fp = disconnect;
    }
}

bool ProfileManager_ConnectProfilesRequest(Task client, device_t device)
{
    return profileManager_HandleRequest(client, device, profile_manager_connect);
}

bool ProfileManager_DisconnectProfilesRequest(Task client, device_t device)
{
    return profileManager_HandleRequest(client, device, profile_manager_disconnect);
}

void ProfileManager_AddRequestToTaskList(task_list_t *list, device_t device, Task client_task)
{
    /* Store device and client for the pending connection */
    task_list_data_t device_used = {0};
    device_used.ptr = device;
    TaskList_AddTaskWithData(list, client_task, &device_used);
}

void ProfileManager_SendConnectCfmToTaskList(task_list_t *list,
                                             bdaddr* bd_addr,
                                             profile_manager_request_cfm_result_t result,
                                             TaskListIterateWithDataRawHandler handler)
{
    profile_manager_send_client_cfm_params params = {0};
    DeviceProperties_SanitiseBdAddr(bd_addr);
    device_t device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_bdaddr, bd_addr, sizeof(bdaddr));
    PanicNull(device);
    params.device = device;
    params.result = result;
    TaskList_IterateWithDataRawFunction(list, handler, &params);
}

bool ProfileManager_Init(Task init_task)
{
    profile_manager_task_data * pm = ProfileManager_GetTaskData();

    DEBUG_LOG("ProfileManager_Init");

    pm->task.handler = profileManager_HandleMessage;
    TaskList_InitialiseWithCapacity(ProfileManager_GetClientTasks(), PROFILE_MANAGER_CLIENT_LIST_INIT_CAPACITY);
    TaskList_WithDataInitialise(&pm->pending_connect_reqs);
    TaskList_WithDataInitialise(&pm->pending_disconnect_reqs);

    /* Register to receive notifications of profile connections/disconnections */
    appAvAvrcpClientRegister(&pm->task, 0);
    appAvStatusClientRegister(&pm->task);
    appHfpStatusClientRegister(&pm->task);

    UNUSED(init_task);
    return TRUE;
}

void ProfileManager_ClientRegister(Task client_task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(ProfileManager_GetClientTasks()), client_task);
}

void ProfileManager_ClientUnregister(Task client_task)
{
    TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(ProfileManager_GetClientTasks()), client_task);
}
