/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file

@brief  Header file for the GATT role selection server library.

        This file provides documentation for the GATT role selection library
        API (library name: gatt_role_selection_server).
*/

#ifndef GATT_ROLE_SELECTION_SERVER_H_
#define GATT_ROLE_SELECTION_SERVER_H_

#include <csrtypes.h>
#include <message.h>

#include <library.h>

#include "gatt_manager.h"
/* Common defines between the client and server */
#include "gatt_role_selection_service.h"


/*! Internal structure for the role_selection server (server)

    This structure is visible to the application as it is responsible 
    for managing resources.

    \note The elements of this structure are only modified by the 
        server library.
    \note The application SHOULD NOT modify the values at any time as it 
        could lead to undefined behaviour.
 */
typedef struct
{
    TaskData lib_task;              /*!< The task for an instance of the GATT server */
    Task app_task;                  /*!< Task for the app using the server */
    uint16 initialised;             /*!< Flag indicating initialised */
                                    /*!  Current mirroring state to be notified to clients */
    GattRoleSelectionServiceMirroringState mirror_state;
                                    /*!  Current figure of merit */
    grss_figure_of_merit_t figure_of_merit;
    bool mirror_state_notified;     /*!< Flag indicating if the mirror state needs to be notified */
    bool figure_of_merit_notified;  /*!< Flag indicating if the figure of merit needs to be notified */
    uint16 mirror_client_config;    /*!< Config for the state notifications */
    uint16 merit_client_config;     /*!< Config for the figure of merit notifications */
} GATT_ROLE_SELECTION_SERVER;


/*! Message sent to command a mirroring state change - select the role */
typedef struct
{
    GattRoleSelectionServiceControlOpCode  command;
} GATT_ROLE_SELECTION_SERVER_CHANGE_ROLE_IND_T;


/*! Enumeration of messages an application task can receive from the 
    root key server library.
 */
typedef enum
{
        /*! Client has commanded a role change */
    GATT_ROLE_SELECTION_SERVER_CHANGE_ROLE_IND = GATT_ROLE_SELECTION_SERVER_MESSAGE_BASE,
        /*! Library message limit */
    GATT_ROLE_SELECTION_SERVER_MESSAGE_TOP
} gatt_role_selection_server_message_id_t;


/*!
    @brief Initialises the Role Selection Library in the Server role.

    @param instance     pointer to the data for this instance of the server
    @param app_task     The Task that will receive messages sent from this library.
    @param start_handle start handle
    @param end_handle   end handle
    @param enable_fom_notification Enables notification of figure of merit changes without the client requesting it

    @return TRUE if successful, FALSE otherwise
*/
bool GattRoleSelectionServerInit(GATT_ROLE_SELECTION_SERVER *instance,
                                 Task app_task,
                                 uint16 start_handle,
                                 uint16 end_handle,
                                 bool enable_fom_notification);


/*!
    @brief Inform the server of the mirroring state

    @param instance     pointer to the data for this instance of the server
    @param mirror_state state that will be sent on reads, or notified to the client
    @param cid          connection identifier for this server
*/
void GattRoleSelectionServerSetMirrorState(GATT_ROLE_SELECTION_SERVER *instance,
                                           uint16 cid,
                                           GattRoleSelectionServiceMirroringState mirror_state);


/*!
    @brief Inform the server of the figure of merit

    @param instance     pointer to the data for this instance of the server
    @param figure_of_merit figure of merit that will be sent on reads, or notified 
                        to the client.
    @param cid          connection identifier for this server
    @param force_notify notify clients even when figure_of_merit is the same as last time.

    @return FALSE if unsuccessful. In this case should retry later.
*/
bool GattRoleSelectionServerSetFigureOfMerit(GATT_ROLE_SELECTION_SERVER *instance,
                                             uint16 cid,
                                             grss_figure_of_merit_t figure_of_merit,
                                             bool force_notify);

#endif /* GATT_ROLE_SELECTION_SERVER_H_ */
