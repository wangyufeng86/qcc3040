/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application support for GATT clients
*/

#ifndef GATT_CLIENT_PROTECTED_H
#define GATT_CLIENT_PROTECTED_H

#include "gatt_client.h"

/* ----------------------------------------------------------------------------------------------------------------------------
Defines section
---------------------------------------------------------------------------------------------------------------------------- */
#define GATT_CLIENT_ID_ENCODING                         0x1234F0CC
#define CONTINUE_SERVICE_DISCOVERY                      FALSE
#define STOP_SERVICE_DISCOVERY                          TRUE

/* This macro creates 3 client data objects :
* 1. ROM object: THE gatt client instance
* 2. ROM object: A gatt client identifier (used to reference the client in the client base and to define client priorities)
* 3. RAM object: A gatt client status object (which gatt client base updates and which should NOT be modified directly from the concrete gatt clients)
*
* INIT_COMMON_GATT_CLIENT_INSTANCE(<client_name>, ....) creates a <client_name> object, a <client_name>_id object and a <client_name>_status object
* e.g.INIT_COMMON_GATT_CLIENT_INSTANCE(gatt_battery_client, ...) creates...
* gatt_battery_client (of type gatt_client_t) and
* gatt_battery_client_id (of type uint32)
* gatt_battery_client_status (of type gatt_client_status */
#define INIT_SIMPLE_GATT_CLIENT_INSTANCE(client_instance_name,\
                                        client_msg_handler,\
                                        client_service_type, client_service_0, client_service_1, client_service_2, client_service_3,\
                                        client_discovery_stop_req,\
                                        client_specific_lib_init_func,\
                                        client_specific_lib_deinit_func,\
                                        client_specific_data) \
static gatt_client_dynamic_data client_instance_name##_dynamic_data = {.status = gatt_client_status_invalid, .cid = 0};\
_Pragma("datasection gatt_client_const_section") static gatt_client_t client_instance_name = \
{\
    {\
        .allocateMemory = GattClient_Protected_AllocateMemory,\
        .deallocateMemory = GattClient_Protected_DeallocateMemory,\
        .getServiceForDiscovery = GattClient_Protected_GetServiceForDiscovery,\
        .getDiscoveryStopRequest = GattClient_Protected_GetDiscoveryStopRequest,\
        .initialise = GattClient_Protected_ClientLibInit,\
        .deinitialise = GattClient_Protected_ClientLibDeinit\
    },\
    {\
        .clientLibInit = client_specific_lib_init_func,\
        .clientLibDeinit = client_specific_lib_deinit_func,\
    },\
    {\
        .task = {.handler = client_msg_handler},\
        .service_for_discovery = {.uuid_type = client_service_type, .service_uuid = {client_service_0, client_service_1, client_service_2, client_service_3}},\
        .stop_discovery_after_finding_service = client_discovery_stop_req,\
        .dynamic_data = &client_instance_name##_dynamic_data,\
    },\
    {\
        .lib_data_dynamic = (void**)&client_specific_data,\
        .size_lib_data_dynamic = sizeof(*client_specific_data),\
        .lib_data_const = NULL\
    }\
};\
_Pragma("datasection gatt_client_const_section") const client_id_t client_instance_name##_id = (client_id_t)&client_instance_name + GATT_CLIENT_ID_ENCODING

#define INIT_CUSTOM_GATT_CLIENT_INSTANCE(client_instance_name, \
                                            alloc_client_mem_func, \
                                            destroy_client_mem_func, \
                                            get_service_for_discovery_func, \
                                            get_discovery_stop_req_func, \
                                            client_init_func, \
                                            client_deinit_func, \
                                            client_specific_lib_init_func,\
                                            client_specific_lib_deinit_func,\
                                            client_msg_handler, \
                                            client_service_type, client_service_0, client_service_1, client_service_2, client_service_3,\
                                            client_discovery_stop_req, \
                                            client_specific_data) \
static gatt_client_dynamic_data client_instance_name##_dynamic_data = {.status = gatt_client_status_invalid, .cid = 0};\
_Pragma("datasection gatt_client_const_section") static gatt_client_t client_instance_name = \
{\
    {\
        .allocateMemory = alloc_client_mem_func,\
        .deallocateMemory = destroy_client_mem_func,\
        .getServiceForDiscovery = get_service_for_discovery_func,\
        .getDiscoveryStopRequest = get_discovery_stop_req_func,\
        .initialise = client_init_func,\
        .deinitialise = client_deinit_func\
    },\
    {\
        .clientLibInit = client_specific_lib_init_func,\
        .clientLibDeinit = client_specific_lib_deinit_func,\
    },\
    {\
        .task = {.handler = client_msg_handler},\
        .service_for_discovery = {.uuid_type = client_service_type, .service_uuid = {client_service_0, client_service_1, client_service_2, client_service_3}},\
        .stop_discovery_after_finding_service = client_discovery_stop_req,\
        .dynamic_data = &client_instance_name##_dynamic_data,\
    },\
    {\
        .lib_data_dynamic = (void**)&client_specific_data,\
        .size_lib_data_dynamic = sizeof(*client_specific_data),\
        .lib_data_const = NULL,\
    }\
};\
_Pragma("datasection gatt_client_const_section") const client_id_t client_instance_name##_id = (const client_id_t)&client_instance_name + GATT_CLIENT_ID_ENCODING

/* ----------------------------------------------------------------------------------------------------------------------------
Types section
---------------------------------------------------------------------------------------------------------------------------- */
struct _gatt_client_t;
struct _gatt_client_common_if_t;

/* Functions to support client before service has been discovered */
typedef gatt_client_service_t(*GetServiceForDiscoveryFunc)(const struct _gatt_client_t* instance);
typedef bool(*GetDiscoveryStopRequestFunc)(const struct _gatt_client_t* instance);

/* Functions to create/destroy client after service has been discovered */
typedef bool(*AllocateMemoryFunc)(const struct _gatt_client_t* instance);
typedef void(*DeallocateMemoryFunc)(const struct _gatt_client_t* instance);
typedef bool(*InitialiseFunc)(const struct _gatt_client_t* instance, uint16 cid, uint16 start, uint16 end);
typedef bool (*DeinitialiseFunc)(const struct _gatt_client_t* instance);

/* Client specific functions */
typedef bool(*ClientLibInitFunc)(Task task, uint16 cid, uint16 start_handle, uint16 end_handle, void *client_data_dynamic, void* client_data_const);
typedef bool (*ClientLibDeinitFunc)(void *client_data_dynamic);

typedef enum
{
    gatt_client_status_invalid,
    gatt_client_status_service_attached,
    gatt_client_status_service_detached,
}gatt_client_status;

typedef struct
{
    gatt_client_status  status;
    uint16              cid;
}gatt_client_dynamic_data;

typedef struct _gatt_client_common_if_t
{
    TaskData                    task;
    gatt_client_service_t       service_for_discovery;
    bool                        stop_discovery_after_finding_service;
    gatt_client_dynamic_data    *dynamic_data;
}const gatt_client_common_data_t;

/* Client specific data separates out the client specific data, from the common data that all clients must support
* For gatt client instances, the data used to initialised its library is a client-specific struct. This data can be passed to its
* library as a void pointer and be cast as appropriate in the library, allowing for the common Gatt Client module to handle libary
* initialisation.
* This struct should be extended to handle any other required client-specific data */
typedef struct
{
    void **         lib_data_dynamic;
    uint16          size_lib_data_dynamic;
    void *          lib_data_const;
}const gatt_client_specific_data_t;


typedef struct
{
    ClientLibInitFunc   clientLibInit;  /* TODO - Lib init signatures have to be made consistent */
    ClientLibDeinitFunc clientLibDeinit;
}const gatt_client_specific_if_t;

typedef struct
{
    AllocateMemoryFunc          allocateMemory;
    DeallocateMemoryFunc        deallocateMemory;
    GetServiceForDiscoveryFunc  getServiceForDiscovery;
    GetDiscoveryStopRequestFunc getDiscoveryStopRequest;
    InitialiseFunc              initialise;
    DeinitialiseFunc            deinitialise;
}const gatt_client_common_if_t;

/* Interfaces are only exposed to the gatt client module */
typedef struct _gatt_client_t
{
    gatt_client_common_if_t     common_if;              /* All gatt clients must define this interface, which allows for base gatt client functions to be overridden */
    gatt_client_specific_if_t   client_specific_if;     /* Client specific fcuntions that can be called directly from the gatt client base module (should be extended with functions that require client-specific data handling) */
    gatt_client_common_data_t   common_data;            /* Data all gatt clients must support */
    gatt_client_specific_data_t client_specific_data;   /* Data that is client specific */
}const gatt_client_t;

/* ----------------------------------------------------------------------------------------------------------------------------
Protrcted overrideable API section (only gatt clients can use this API and they may, if needed override any function)
---------------------------------------------------------------------------------------------------------------------------- */
bool GattClient_Protected_AllocateMemory(gatt_client_t* instance);
void GattClient_Protected_DeallocateMemory(gatt_client_t* instance);
gatt_client_service_t GattClient_Protected_GetServiceForDiscovery(gatt_client_t *instance);
bool GattClient_Protected_GetDiscoveryStopRequest(gatt_client_t* instance);
bool GattClient_Protected_ClientLibInit(gatt_client_t* instance, uint16 cid, uint16 start_handle, uint16 end_handle);
bool GattClient_Protected_ClientLibDeinit(gatt_client_t*instance);

/* ----------------------------------------------------------------------------------------------------------------------------
Protrcted non-overrideable API section (only gatt clients can use this API, but functions are NOT override)
---------------------------------------------------------------------------------------------------------------------------- */
gatt_client_status GattClient_Protected_GetStatus(gatt_client_t*instance);
uint16 GattClient_Protected_GetConnectionID(gatt_client_t*instance);

#endif  /* GATT_CLIENT_PROTECTED_H */
