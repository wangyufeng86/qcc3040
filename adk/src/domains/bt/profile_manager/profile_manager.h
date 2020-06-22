/*!
\copyright  Copyright (c) 2018 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       profile_manager.h
\brief     Interface of the profile manager module.

    These apis are used by application to register different profile connection
    handlers with the profile manager as well as invoking them when application
    wants to connect all supported profiles for that device.Profile manager will
    handle connection crossover as well as is able to connect profiles in any order
    application wants.
*/

#ifndef PROFILE_MANAGER_H_
#define PROFILE_MANAGER_H_


#include <bdaddr.h>
#include <csrtypes.h>
#include <device.h>
#include <domain_message.h>
#include <task_list.h>


#define PROFILE_MANAGER_CLIENT_LIST_INIT_CAPACITY 1

typedef enum
{
    profile_manager_success,
    profile_manager_failed,
    profile_manager_cancelled
} profile_manager_request_cfm_result_t;

typedef enum
{
    profile_manager_disconnected_normal,
    profile_manager_disconnected_link_loss,
    profile_manager_disconnected_error
} profile_manager_disconnected_ind_reason_t;

enum profile_manager_messages
{
    CONNECT_PROFILES_CFM = PROFILE_MANAGER_MESSAGE_BASE,
    DISCONNECT_PROFILES_CFM,

    CONNECTED_PROFILE_IND,
    DISCONNECTED_PROFILE_IND
};

typedef struct
{
    device_t device;
    profile_manager_request_cfm_result_t result;
} CONNECT_PROFILES_CFM_T;

typedef struct
{
    device_t device;
    unsigned profile;
} CONNECTED_PROFILE_IND_T;

typedef struct
{
    device_t device;
    profile_manager_request_cfm_result_t result;
} DISCONNECT_PROFILES_CFM_T;

typedef struct
{
    device_t device;
    unsigned profile;
    profile_manager_disconnected_ind_reason_t reason;
} DISCONNECTED_PROFILE_IND_T;

typedef struct
{
    device_t device;
    profile_manager_request_cfm_result_t result;
} profile_manager_send_client_cfm_params;

/*! \brief Profile manager task data. */
typedef struct
{
    TaskData                task;                      /*!< Profile Manager task */
    TASK_LIST_WITH_INITIAL_CAPACITY(PROFILE_MANAGER_CLIENT_LIST_INIT_CAPACITY)             client_tasks;              /*!< List of tasks interested in Profile Manager indications */
    task_list_with_data_t   pending_connect_reqs;      /*!< List of tasks that are pending connection requests */
    task_list_with_data_t   pending_disconnect_reqs;   /*!< List of tasks that are pending discconnection requests */

} profile_manager_task_data;

/*!< Profile manager task */
extern profile_manager_task_data profile_manager;

/*! Get pointer to Device Management data structure */
#define ProfileManager_GetTaskData()  (&profile_manager)

/*! Get pointer to Device Management client tasks */
#define ProfileManager_GetClientTasks()  (task_list_flexible_t *)(&profile_manager.client_tasks)

/*! \brief This contains address of the connect handler of the profile
           being registred which application wants to connect.Internally
           these connect handlers will invoke corresponding library calls
           to initiate connection procedure.This connect handler needs to
           pass only the bt address of the device with which application
           is initiating profile connection.This connect handler will be
           invoked during ProfileManager_ConnectProfiles() call.

    \param bd_addr - Bluetooth address of the connecting device.

    \return void
*/
typedef void (*profile_manager_registered_connect_request_t)(const Task client_task, bdaddr* bd_addr);
typedef void (*profile_manager_registered_disconnect_request_t)(const Task client_task, bdaddr* bd_addr);

 /*! \brief supported profiles list. */
typedef enum {
    profile_manager_hfp_profile,
    profile_manager_a2dp_profile,
    profile_manager_avrcp_profile,
    profile_manager_max_number_of_profiles,
    profile_manager_bad_profile
} profile_t;

/*! \brief Request to connect profiles for the given device.

     This API connects the BlueTooth profiles specified in the device property
     device_property_profiles_connect_order for the specified device. The profiles
     will be conected in the order in which they occur in the property, i.e. index 0 will
     be connected first. Profiles are connected sequentially.

    \param client the client requesting the connection, whom shall recieve the confirmation
    \param device for which profiles are to be connected

    \return TRUE : if a request was created , FALSE : If the request was not accepted
*/
bool ProfileManager_ConnectProfilesRequest(Task client, device_t device);

/*! \brief Request to disconnect profiles for the given device.

     This API disconnects the BlueTooth profiles specified in the device property
     device_property_profiles_disconnect_order for the specified device. The profiles
     will be disconected in the order in which they occur in the property, i.e. index 0 will
     be disconnected first. Profiles are disconnected sequentially.

    \param client the client requesting the disconnection, whom shall recieve the confirmation
    \param device for whch profiles are to be disconnected

    \return TRUE : if a request was created , FALSE : If the request was not accepted
*/
bool ProfileManager_DisconnectProfilesRequest(Task client, device_t device);

/*! \brief Register profile connect handlers with the profile manager.

    This api is called to register connect functions of different profiles
    and then these registered functions will be invoked by profile manager
    when any connect request will arrive from client.

    \param name - profile whose interface is being registered
    \param profile_if - pointer to profile interface struct.

    \return void
*/
void ProfileManager_RegisterProfile(profile_t name,
                                    profile_manager_registered_connect_request_t connect_fp,
                                    profile_manager_registered_disconnect_request_t disconnect_fp);

/*! \brief Helper function to add a Request to a task list with data. */
void ProfileManager_AddRequestToTaskList(task_list_t *list, device_t device, Task client_task);

/*! \brief Helper function to send confirmations to a Request using a task list with data. */
void ProfileManager_SendConnectCfmToTaskList(task_list_t *list, bdaddr* bd_addr, profile_manager_request_cfm_result_t result, TaskListIterateWithDataRawHandler handler);

/*! \brief Initialse the Profile Manager module. */
bool ProfileManager_Init(Task init_task);

/*! \brief Register a Task to receive notifications from profile_manager.

    Once registered, #client_task will receive IND type messages from #profile_manager_messages.

    \param client_task Task to register to receive profile_manager notifications.
*/
void ProfileManager_ClientRegister(Task client_task);

/*! \brief Un-register a Task that is receiving notifications from profile_manager.

    If the task is not currently registered then nothing will be changed.

    \param client_task Task to un-register from profile_manager notifications.
*/
void ProfileManager_ClientUnregister(Task client_task);

#endif /* PROFILE_MANAGER_H_*/
