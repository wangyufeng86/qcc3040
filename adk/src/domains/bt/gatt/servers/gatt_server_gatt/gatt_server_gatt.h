/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Header file for the GATT Server module.

Component that deals with initialising the gatt_server library, 
and deals with messages sent from this library.

*/

#ifndef GATT_SERVER_GATT_H_
#define GATT_SERVER_GATT_H_


#include <gatt_handler.h>

#include <gatt_server.h>


/*! The number of GATT clients that are able to simultaneously connect with one GATT server. */
#define NUMBER_GATT_CLIENTS APP_GATT_SERVER_INSTANCES

/*! Structure holding the client data */
typedef struct
{
    /*! Client connection ID */
    uint16  cid;
    /*! Client Service Configuration set by by the client */
    uint16 config;
} gattServerGattClientData;

/*! Structure holding information for the application handling of GATT Server */
typedef struct
{
    /*! Task for handling GATT related messages */
    TaskData                        gatt_task;
    /*! Client data associated with the server */
    gattServerGattClientData client_data[NUMBER_GATT_CLIENTS];
    /*! GATT server library data */
    GGATTS  ggatts;
} gattServerGattData;

/*!< App GATT component task */
extern gattServerGattData gatt_server_gatt;

/*! Get pointer to the main GATT server Task */
#define GetGattServerGattTask() (&gatt_server_gatt.gatt_task)

/*! Get pointer to the GATT server data passed to the library */
#define GetGattServerGattGgatts() (&gatt_server_gatt.ggatts)


/*! \brief Initialise the GATT Server.

    \param init_task    Task to send init completion message to

    \returns TRUE
*/
bool GattServerGatt_Init(Task init_task);


/*! \brief Called when the local GATT DB has changed (eg. after software upgrade)

    If GATT services have changed on the local device,
    (eg. after a software upgrade), then it is necessary
    to inform any interested bonded clients of this.
    Clients can use the client configuration of the Service Changed
    characteristic to register for indications of such changes.
    This API will send indications to any connected clients that have
    registered and are bonded.
*/
void GattServerGatt_SetGattDbChanged(void);

#endif /* GATT_SERVER_GATT_H_ */
