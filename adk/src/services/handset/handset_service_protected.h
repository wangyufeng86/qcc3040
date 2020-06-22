/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Handset service types to be used within handset_service only
*/

#ifndef HANDSET_SERVICE_PROTECTED_H_
#define HANDSET_SERVICE_PROTECTED_H_

#include <bdaddr.h>
#include <logging.h>
#include <panic.h>
#include <task_list.h>
#include <le_advertising_manager.h>

#include "handset_service.h"
#include "handset_service_sm.h"

/*! \{
    Macros for diagnostic output that can be suppressed.
*/
#define HS_LOG         DEBUG_LOG
/*! \} */

/*! Code assertion that can be checked at run time. This will cause a panic. */
#define assert(x) PanicFalse(x)

/*! Client task list initial list */
#define HANDSET_SERVICE_CLIENT_LIST_INIT_CAPACITY 1

#define HANDSET_SERVICE_MAX_SM      3

/*! \brief Data type to specify the state of LE advertising data set select/release operation */
typedef enum
{
    handset_service_le_adv_data_set_state_not_selected = 0,
    handset_service_le_adv_data_set_state_selected,
    handset_service_le_adv_data_set_state_selecting,
    handset_service_le_adv_data_set_state_releasing,

} handset_service_le_adv_data_set_state_t;

/*! \brief The global data for the handset_service */
typedef struct
{
    /*! Handset Service task */
    TaskData task_data;

    /* Handset Service state machine */
    handset_service_state_machine_t state_machine[HANDSET_SERVICE_MAX_SM];

    /*! Client list for notifications */
    TASK_LIST_WITH_INITIAL_CAPACITY(HANDSET_SERVICE_CLIENT_LIST_INIT_CAPACITY) client_list;

    /* Flag to store if handset can be paired */
    bool pairing;
    
    /* Flag to store if handset has a BLE connection */
    bool ble_connected;
    
    /* Flag to indicate whether the device is BLE connectable */
    bool ble_connectable;
    
    /* State of LE advertising data set select/release */
    handset_service_le_adv_data_set_state_t ble_adv_state;

    /* Handle for LE advertising data set */
    le_adv_data_set_handle le_advert_handle;
    
    /* Selected LE advertising data set */
    le_adv_data_set_t le_advert_data_set;

} handset_service_data_t;

/*! \brief Internal messages for the handset_service */
typedef enum
{
    /*! Request to connect to a handset */
    HANDSET_SERVICE_INTERNAL_CONNECT_REQ = 0,

    /*! Request to disconnect a handset */
    HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ,

    /*! Delivered when an ACL connect request has completed. */
    HANDSET_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE,

    /*! Request to cancel any in-progress connect to handset. */
    HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ,

    /*! Request to re-try the ACL connection after a failure. */
    HANDSET_SERVICE_INTERNAL_CONNECT_ACL_RETRY_REQ,
} handset_service_internal_msg_t;

typedef struct
{
    /* Handset device to connect. */
    device_t device;

    /* Mask of profile(s) to connect. */
    uint8 profiles;
} HANDSET_SERVICE_INTERNAL_CONNECT_REQ_T;

typedef struct
{
    /* Address of handset device to disconnect. */
    bdaddr addr;
} HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ_T;

typedef struct
{
    /* Handset device to stop connect for */
    device_t device;
} HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ_T;



/*! \brief Send a HANDSET_SERVICE_CONNECTED_IND to registered clients.

    \param device Device that represents the handset
    \param status Status of the connection.
    \param profiles_connected Profiles currently connected to this handset.
*/
void HandsetService_SendConnectedIndNotification(device_t device,
    uint8 profiles_connected);

/*! \brief Send a HANDSET_SERVICE_DISCONNECTED_IND to registered clients.

    \param addr Address of the handset
    \param status Status of the connection.
*/
void HandsetService_SendDisconnectedIndNotification(const bdaddr *addr,
    handset_service_status_t status);

/*! Handset Service module data. */
extern handset_service_data_t handset_service;

/*! Get pointer to the Handset Service modules data structure */
#define HandsetService_Get() (&handset_service)

/*! Get the Task for the handset_service */
#define HandsetService_GetTask() (&HandsetService_Get()->task_data)

/*! Get the state machine for the handset service. */
#define HandsetService_GetSm() (HandsetService_Get()->state_machine)

/*! Get the client list for the handset service. */
#define HandsetService_GetClientList() (task_list_flexible_t *)(&HandsetService_Get()->client_list)

/*! Update advertising data */
bool handsetService_UpdateAdvertisingData(void);

/*! Check if a new handset connection is allowed */
bool handsetService_CheckHandsetCanConnect(const bdaddr *addr);

#endif /* HANDSET_SERVICE_PROTECTED_H_ */
