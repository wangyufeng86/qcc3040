/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application support for GATT clients
*/

#ifndef GATT_CLIENT_H
#define GATT_CLIENT_H

#include <gatt.h>
 
/*! Helper macro for Gatt client iniitalisation */
#define GattClientInit(table)   GattClient_Init(table, sizeof(table)/sizeof(table[0]))

#ifndef CONST_CLIENT_ID_TYPE
/* Refer to test code */
typedef uint32 client_id_t;
#endif

/*! client required service information */
typedef struct
{
    gatt_uuid_type_t	uuid_type;
    gatt_uuid_t         service_uuid[4];
}gatt_client_service_t;


/*! @brief MUST be called on application startup with a list of gatt client identifiers

    \param gatt_client_id A list of pointers to client identifiers
    \param num_elements the number of elements within the list
*/
void GattClient_Init(const client_id_t* gatt_client_id[], uint8 num_elements);

/*! @brief May be used to invalidate ALL the gatt clients */
void GattClient_Deinit(void);

/*! @brief Attaches a client to the service the client has requested

    \param client_id    The client id
    \param cid          The connection id
    \param start_handle The start handle of the service
    \param end_handle   The end handle of the service

    \returns TRUE if client was successfully attached to service. FALSE otherwise.
*/
bool GattClient_AttachClient(client_id_t client_id, uint16 cid, uint16 start_handle, uint16 end_handle);

/*! @brief Detaches a client from a service (called on rediscovery, link loss etc)

    \param client_id    The client id

    \returns TRUE if client was successfully detached from the service. FALSE otherwise.
*/
bool GattClient_DetachClient(client_id_t client_id, uint16 cid);

/*! @brief Gets the clients associated service for discovery

    \param client_id    The client id

    \returns The service information associated with the client.
             Note: If the client is in a state which invalidates its need to discover a service, then the uuid type
             returned must be set to gatt_uuid_none
*/
gatt_client_service_t GattClient_GetServiceForDiscovery(client_id_t client_id);

/*! @brief Asks the client if service discovery should terminate after finding service

    \param client_id    The client id

    \returns TRUE if service discovery should terminate if service is discovered. FALSE otherwise.
*/
bool GattClient_GetDiscoveryStopRequest(client_id_t client_id);

#endif
