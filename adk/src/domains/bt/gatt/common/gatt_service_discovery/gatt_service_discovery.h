/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for Gatt Service Discovery
*/

#ifndef GATT_SERVICE_DISCOVERY_H
#define GATT_SERVICE_DISCOVERY_H

#include "gatt_client.h"

/*! Enumerated type for service discovery status */
typedef enum
{
    gsd_uninitialised,   /*! Uninitialised state */
    gsd_in_idle,         /*! Idle State - Ready to start dicovering */
    gsd_in_progress,     /*! Service discovery in progress */
    gsd_complete_failure /*! Service discovery failed */
}service_discovery_status;

/*! \brief Initialise the GATT Dervice Discovery component
    \param[in] init_task - unused
    \return bool TRUE
 */
bool GattServiceDiscovery_Init(Task init_task);

/*! Function to initiate the GATT service procedure.

    \param connection id in which the discovery procedure has to be started.

    \return TRUE if service discovery started successfully, FALSE otherwise.
*/
bool GattServiceDiscovery_StartDiscovery(uint16 cid);

/*! Function to remove the clients.

    \param cid Connection in which the Service discovery should start.

    \return TRUE if all the clients destroyed successfully, FALSE otherwise.
*/
bool GattServiceDiscovery_DestroyClients(uint16 cid);

/*! Function to get the GATT Service discovery component status.

    \param none.

    \return Gatt service discovery component's present status.
*/
service_discovery_status GattServiceDiscovery_GetStatus(void);

#endif
