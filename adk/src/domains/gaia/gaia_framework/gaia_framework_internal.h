/*!
\copyright  Copyright (c) 2017 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version
\file
\brief      Header file for handling the GAIA transport interface
*/

#ifndef GAIA_FRAMEWORK_INTERNAL_H_
#define GAIA_FRAMEWORK_INTERNAL_H_

#ifdef INCLUDE_DFU


#include <gaia.h>
#include <task_list.h>

#include "domain_message.h"
#include "gatt_connect.h"


/*! Minor version number used for the GAIA interface */
#define GAIA_API_MINOR_VERSION 5

/*! Defines the gaia client task list initial capacity*/
#define GAIA_CLIENT_TASK_LIST_INIT_CAPACITY 1

/*! Messages that are sent by the gaia_handler module */
typedef enum {
    APP_GAIA_INIT_CFM = AV_GAIA_MESSAGE_BASE,   /*!< Application GAIA module has initialised */
    APP_GAIA_CONNECTED,                         /*!< A GAIA connection has been made */
    APP_GAIA_DISCONNECTED,                      /*!< A GAIA connection has been lost */
    APP_GAIA_UPGRADE_CONNECTED,                 /*!< An upgrade protocol has connected through GAIA */
    APP_GAIA_UPGRADE_DISCONNECTED,              /*!< An upgrade protocol has disconnected through GAIA */
} av_headet_gaia_messages;

/*! Data used by the GAIA module */
typedef struct
{
        /*! Function to send response once GAIA has tidied up due to a disconnect. */
    gatt_connect_disconnect_req_response response;
        /*! The cid of the connection that is being disconnected. */
    uint16 cid;
} gaiaDisconnectData;

/*! Data used by the GAIA module */
typedef struct
{
        /*! Task for handling messaging from upgrade library */
    TaskData        gaia_task;
        /*! The current transport (if any) for GAIA */
    GAIA_TRANSPORT *transport;
        /*! Whether a GAIA connection is allowed, or will be rejected immediately */
    bool            connections_allowed;
        /*! List of tasks to notify of GAIA activity. */
    TASK_LIST_WITH_INITIAL_CAPACITY(GAIA_CLIENT_TASK_LIST_INIT_CAPACITY) client_list;
        /*! Data associated with a disconnection. */
    gaiaDisconnectData disconnect;

} gaiaTaskData;

/*!< Task information for GAIA support */
extern gaiaTaskData    app_gaia;

/*! Get the info for the applications Gaia support */
#define GaiaGetTaskData()                (&app_gaia)

/*! Get the Task info for the applications Gaia task */
#define GaiaGetTask()            (&app_gaia.gaia_task)

/*! Get the transport for the current GAIA connection */
#define GaiaGetTransport()           (GaiaGetTaskData()->transport)

/*! Set the transport for the current GAIA connection */
#define GaiaSetTransport(_transport)  do { \
                                            GaiaGetTaskData()->transport = (_transport);\
                                           } while(0)

/*! Get the client list for the applications Gaia task */
#define GaiaGetClientList()            (task_list_flexible_t *)(&app_gaia.client_list)

bool GaiaFrameworkInternal_Init(Task init_task);

/*! Add a client to the GAIA module

    Messages from #av_headet_gaia_messages will be sent to any task
    registered through this API

    \param task Task to register as a client
 */
extern void GaiaFrameworkInternal_ClientRegister(Task task);


/*! \brief Disconnect any active gaia connection
 */
extern void gaiaFrameworkInternal_GaiaDisconnect(void);


/*! \brief Let GAIA know whether to allow any connections

    \param  allow A new gaia connection is allowed
 */
extern void gaiaFrameworkInternal_AllowNewConnections(bool allow);


/*! \brief Initialise the GATT Gaia Server.

    \param  init_task    Task to send init completion message to
 */
extern bool gaiaFrameworkInternal_GattServerInit(Task init_task);


#endif /* INCLUDE_DFU */
#endif /* GAIA_FRAMEWORK_INTERNAL_H_ */
