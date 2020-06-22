/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Definition file for task data used by the device test service
*/

#ifndef __DEVICE_TEST_SERVICE_DATA_H__
#define __DEVICE_TEST_SERVICE_DATA_H__

#include <device_test_service.h>
#include <stream.h>
#include <spp_common.h>

/*! Structure for holding a 128-bit key

    \implementation Not using a typedef of array. C is permissive, 
                so felt too dangerous. 
    \implementation A union is used as a uint8 array, even in a structure
            has octet alignment and uint16 access is convenient.
 */
typedef union {
    /* The key array */
    uint8 key_u8[DTS_KEY_SIZE_OCTETS];
    /* A uint16 member used for alignment */
    uint16 aligned_u16[1];
} DTS_AUTH_KEY_T;



/*! Values indicating the status of authentication of the device test service */
typedef enum
{
        /*! Not authenticated, including authentication failed */
    device_test_service_not_authenticated,
        /*! Authentication is in progress */
    device_test_service_authentication_in_progress,
        /*! The service has completed an authentication sequence */
    device_test_service_authenticated,
        /*! No authentication is required */
    device_test_service_authentication_not_required,
} device_test_service_auth_state_t;


/*! Structure used for data specific to the device test service */
typedef struct
{
    /*! task used for internal messages, and most external messages */
    TaskData device_test_service_handler_task;
    /*! task used purely for messages from the SPP Server library */
    TaskData spps_handler_task;

    /*! Task passed in the call starting the device test service */
    Task app_task;

    /*! Whether the service has been authenticated */
    device_test_service_auth_state_t authenticated;

    /*! Randomised value sent in response to Authentication start */
    DTS_AUTH_KEY_T challenge;

    /*! The sink in use for communication 
        \todo This should probably be replaced with a transport mechanism */
    Sink rfc_sink;
    /*! The source in use for communication */
    Source rfc_source;

    /*! The SPP record provided when the SPP Server connected */
    SPP *spp;

    /*! The address of the connected device */
    bdaddr connected_address;

    /*! Whether the service is currently active */ 
    bool active:1;

    /*! Whether we are in the process of cleanly disconnecting */
    bool disconnecting:1;

    /*! Whether we have requested pairing */
    bool pairing_requested:1;

    /*! Whether the SPP record is registered */
    bool spp_service_registered:1;

    /*! Remember if bredr connections were allowed when the service was started */
    bool bredr_connections_allowed:1;

    /*! Remember if handset connections were allowed when the service was started */
    bool handset_connections_allowed:1;
} device_test_service_data_t;

extern device_test_service_data_t device_test_service;

#define DeviceTestServiceGetData() (&device_test_service)
#define DeviceTestServiceGetTask() (&device_test_service.device_test_service_handler_task)

#define DeviceTestServiceGetSppTask() (&device_test_service.spps_handler_task)

#define DeviceTestServiceIsActive() (device_test_service.active)

#define DeviceTestServiceIsAuthenticated() (   device_test_service.authenticated == device_test_service_authenticated \
                                            || device_test_service.authenticated == device_test_service_authentication_not_required)

#endif /* __DEVICE_TEST_SERVICE_DATA_H__ */
