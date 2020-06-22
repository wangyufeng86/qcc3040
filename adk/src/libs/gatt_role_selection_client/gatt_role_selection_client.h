/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file    
@brief   Header file for the GATT root key client library.

        This file provides documentation for the GATT role selection client
        library API (library name: gatt_role_selection_client).

        The design intention is that the role selection service is fast for
        the normal use case. The normal case is to command the peer to change
        to primary or secondary role.

        If notifications are required then the required descriptor handle 
        will be discovered when required.

        \todo Its possible to find a handle by UUID. The GATT library does
        not expose this functionality, but function gatt_find_by_type_value_req
        MIGHT have the guts of what is needed.
*/

#ifndef GATT_ROLE_SELECTION_CLIENT_H_
#define GATT_ROLE_SELECTION_CLIENT_H_

#include <csrtypes.h>
#include <message.h>

#include <library.h>
#include <bdaddr.h>

/* Use common defines between server and client */
#include <gatt_role_selection_service.h>

/*! @brief Structure to contain role selection client Gatt handles.

*/
typedef struct
{
    uint16 handle_state;                /*!< Handle for reading our peers state */
    uint16 handle_state_config;         /*!< Handle for the configuration for the state
                                                characteristic */
    uint16 handle_state_end;            /*!< Maximum handle for finding the config characteristic */
    uint16 handle_figure_of_merit;      /*!< Handle for reading the figure of merit from our peer */
    uint16 handle_figure_of_merit_config;/*!< Handle for configuring the figure of merit
                                                characteristic */
    uint16 handle_figure_of_merit_end;  /*!< Maximum handle for finding the figure of merit 
                                                config characteristic */
    uint16 handle_role_control;         /*!< Handle for commanding a role */
} gatt_role_selection_handles_t;


/*! @brief The role_selection internal structure for the client role.

    This structure is visible to the application as it is responsible for managing resource to pass to the role_selection library.
    The elements of this structure are only modified by the role_selection library.
    The application SHOULD NOT modify the values at any time as it could lead to undefined behaviour.

 */
typedef struct
{
    TaskData lib_task;                  /*!< This task */
    Task app_task;                      /*!< Application task passed in GattRoleSelectionClientInit */

    gatt_role_selection_handles_t handles; /*! Handles for the various characteristics of this service */

    uint16 state;                       /*!< State of the client implementation. \ref gatt_role_selection_client_state.h */
                                        /*! Last received mirroring state of the peer */
    GattRoleSelectionServiceMirroringState peer_state;
    grss_figure_of_merit_t figure_of_merit;/*!< Last received figure of merit from the peer */

    bool init_response_needed;          /*!< Need to send a response for init */
    bool control_response_needed;       /*!< Need to send a response for writing to the role control endpoint */
    uint16  active_procedure;           /*!< Flag set when a procedure is active. Used for MessageSendConditional. */
} GATT_ROLE_SELECTION_CLIENT;


/*!
    @brief Status code returned from the GATT role_selection client library

    This status code indicates the outcome of the request.
*/
typedef enum
{
    gatt_role_selection_client_status_success,
    gatt_role_selection_client_status_not_allowed,
    gatt_role_selection_client_status_failed,
} gatt_role_selection_client_status_t;


/*!
    @brief Contents of the GATT_ROLE_SELECTION_CLIENT_INIT_CFM message that is 
    sent by the library, as a response to the initialisation request.
 */
typedef struct
{
    const GATT_ROLE_SELECTION_CLIENT *instance;
    gatt_role_selection_client_status_t  status;

} GATT_ROLE_SELECTION_CLIENT_INIT_CFM_T;


/*!
    @brief Contents of the GATT_ROLE_SELECTION_CLIENT_STATE_IND message that 
     is sent by the library when a notification of a state change by the peer
     is received..
 */
typedef struct
{
    const GATT_ROLE_SELECTION_CLIENT       *instance;
    GattRoleSelectionServiceMirroringState  state;

} GATT_ROLE_SELECTION_CLIENT_STATE_IND_T;


/*!
    @brief Contents of the GATT_ROLE_SELECTION_CLIENT_FIGURE_OF_MERIT_IND message 
     that is sent by the library when the peer notifies us of an updated figure
     of merit.
 */
typedef struct
{
    const GATT_ROLE_SELECTION_CLIENT       *instance;
    grss_figure_of_merit_t                  figure_of_merit;

} GATT_ROLE_SELECTION_CLIENT_FIGURE_OF_MERIT_IND_T;


/*!
    @brief Contents of the GATT_ROLE_SELECTION_CLIENT_COMMAND_CFM message that 
     is sent by the library following a command to change the peers state.

    The result indicates whether the command was successfully delivered.
 */
typedef struct
{
    const GATT_ROLE_SELECTION_CLIENT   *instance;
    gatt_status_t                       result;

} GATT_ROLE_SELECTION_CLIENT_COMMAND_CFM_T;


/*! @brief Enumeration of messages a client task may receive from the role_selection client library.
 */
typedef enum
{
    /* Client messages */
                    /*! Message sent when the gatt client is initialised */
    GATT_ROLE_SELECTION_CLIENT_INIT_CFM = GATT_ROLE_SELECTION_CLIENT_MESSAGE_BASE,
    GATT_ROLE_SELECTION_CLIENT_STATE_IND,
    GATT_ROLE_SELECTION_CLIENT_FIGURE_OF_MERIT_IND,
    GATT_ROLE_SELECTION_CLIENT_COMMAND_CFM,

    /* Library message limit */
    GATT_ROLE_SELECTION_CLIENT_MESSAGE_TOP
} gatt_role_selection_client_message_id_t;


/*!
    @brief After the VM application has used the GATT manager library to establish a connection to a discovered BLE device in the Client role,
    it can discover any supported services in which it has an interest. It should then register with the relevant client service library
    (passing the relevant CID and handles to the service). For the role_selection client it will use this API. The GATT manager 
    will then route notifications and indications to the correct instance of the client service library for the CID.
    If the characteristic handles are already known they can be passed to the library with the parameter 'cached_handles'. This
    will cause the libray to not retrieve the handles from the server. If the handles are not known set cached_handles to NULL.

    @param instance A valid area of memory that the service library can use.
    @param app_task The Task that will receive the messages sent from this role_selection client library.
    @param cached_handles Handles of characteristics if already known. Otherwise NULL.
    @param cid The connection ID.
    @param start_handle The start handle of the role_selection client instance.
    @param end_handle The end handle of the role_selection client instance.

    @return TRUE if successful, FALSE otherwise

*/
bool GattRoleSelectionClientInit(GATT_ROLE_SELECTION_CLIENT *instance,
                                 Task app_task,
                                 gatt_role_selection_handles_t *cached_handles,
                                 uint16 cid,
                                 uint16 start_handle,
                                 uint16 end_handle);

/*!
    @brief When a GATT connection is removed, the application must remove all client service instances that were
    associated with the connection (using the CID value).
    This is the clean up routine as a result of calling the GattRoleSelectionClientInit API. That is,
    the GattRoleSelectionClientInit API is called when a connection is made, and the GattRoleSelectionClientDestroy is called 
    when the connection is removed.

    @param instance The client instance that was passed into the GattRoleSelectionClientInit API.

    @return TRUE if successful, FALSE otherwise

*/
bool GattRoleSelectionClientDestroy(GATT_ROLE_SELECTION_CLIENT *instance);


/*! Request a read of the peers mirroring state

    This function sends a message requesting the state from the peer. 
    The application will be sent a GATT_ROLE_SELECTION_CLIENT_STATE_IND
    once the command is complete.

    \param[in]  instance        The gatt client instance memory

    \return TRUE if the client is able to request the read, FALSE
            if the client is not in a valid state.
*/
bool GattRoleSelectionClientReadPeerState(GATT_ROLE_SELECTION_CLIENT *instance);


/*! Enable notification of changes in the peers mirroring state

    This function enables the sending of autonomous state updates from 
    the remote device.

    \param[in]  instance    The gatt client instance memory

    \return TRUE if the client is able to process the request, FALSE
            if the client is not in a valid state.
*/
bool GattRoleSelectionClientEnablePeerStateNotifications(GATT_ROLE_SELECTION_CLIENT *instance);


/*! Request a read of the peers figure of merit

    This function sends a message requesting the figure of merit from 
    the peer. The application will be sent a 
    GATT_ROLE_SELECTION_CLIENT_FIGURE_OF_MERIT_IND once the command is complete.

    \param[in]  instance        The gatt client instance memory

    \return TRUE if the client is able to request the read, FALSE
            if the client is not in a valid state.
*/
bool GattRoleSelectionClientReadFigureOfMerit(GATT_ROLE_SELECTION_CLIENT *instance);


/*! Enable notification of changes in the peers figure of merit

    This function enables the sending of autonomous updates for the
    figure of merit from the remote device.

    \param[in]  instance    The gatt client instance memory

    \return TRUE if the client is able to process the request, FALSE
            if the client is not in a valid state.
*/
bool GattRoleSelectionClientEnablePeerFigureOfMeritNotifications(GATT_ROLE_SELECTION_CLIENT *instance);


/*! Command the peer to change their state to that requested

    This function sends a message commanding the peer to change to the requested
    state. The application will be sent a GATT_ROLE_SELECTION_CLIENT_COMMAND_CFM_T
    once the command is complete.

    \param[in]  instance        The gatt client instance memory
    \param[in]  state           The role to change the peer to

    \return TRUE if the client is able to start processing the challenge, FALSE
            if the client is not in a valid state.
*/
bool GattRoleSelectionClientChangePeerRole(GATT_ROLE_SELECTION_CLIENT *instance,
                                           GattRoleSelectionServiceControlOpCode state);

#endif /* GATT_ROLE_SELECTION_CLIENT_H_ */
