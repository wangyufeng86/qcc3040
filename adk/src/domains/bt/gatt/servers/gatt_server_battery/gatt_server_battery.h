/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       gatt_server_battery.h
\brief      Header file for the GATT Battery Server module.

Component that deals with initialising the gatt_battery_server library, 
and deals with messages sent from this library.

There can be multiple battery servers, so this component initialises and handles each server.
Also each server could have multiple battery clients connected,
if supported by the application, where each client can store and retrieve its own configuration.

*/

#ifndef GATT_SERVER_BATTERY_H_
#define GATT_SERVER_BATTERY_H_


#include "le_advertising_manager.h"

#include <gatt_handler.h>

#include <gatt_battery_server.h>


/*! Enumerated type that specifies the type of battery instances that are handled by the app */
typedef enum
{
    gatt_server_battery_type_left,       /*!< Left battery */
    gatt_server_battery_type_right,      /*!< Right battery */
    gatt_server_battery_type_max         /*!< Always the last entry in the enumeration, used as the max index */
} gatt_server_battery_type;

/*! The number of battery servers that are initialised. */
#define NUMBER_BATTERY_SERVERS gatt_server_battery_type_max

/*! The number of battery clients that are able to simultaneously connect with one battery server. */
#define NUMBER_BATTERY_CLIENTS APP_GATT_SERVER_INSTANCES

/*! Structure holding the client data */
typedef struct
{
    /*! Client connection ID */
    uint16  cid;
    /*! Client Service Configuration set by by the client */
    uint16 config;
    /*! Last Battery Level sent to the client */
    uint16 sent_battery_level;
} gattServerBatteryClientData;

/*! Structure holding information for the application handling of GATT Battery Server */
typedef struct
{
    /*! Client data associated with the server */
    gattServerBatteryClientData client_data[NUMBER_BATTERY_CLIENTS];
    /*! Battery server library data */
    GBASS  gbass;
} gattServerBatteryInstanceInfo;

/*! Structure holding information for the application handling of GATT Battery Server */
typedef struct
{
    /*! Task for handling battery related messages */
    TaskData                        gatt_battery_task;
    /*! Instances of the battery servers */
    gattServerBatteryInstanceInfo   instance[NUMBER_BATTERY_SERVERS];
} gattServerBatteryData;

/*!< App GATT component task */
extern gattServerBatteryData gatt_server_battery;

/*! Get pointer to the main battery Task */
#define GetGattServerBatteryTask() (&gatt_server_battery.gatt_battery_task)

/*! Get pointer to the battery server instance */
#define GetGattServerBatteryInstance(battery_type) (&gatt_server_battery.instance[battery_type])

/*! Get pointer to the battery server data passed to the library */
#define GetGattServerBatteryGbass(battery_type) (&gatt_server_battery.instance[battery_type].gbass)


/*! @brief Initialise the GATT Battery Server.

    \param init_task    Task to send init completion message to

    \returns TRUE
*/
bool GattServerBattery_Init(Task init_task);


#endif /* GATT_SERVER_BATTERY_H_ */
