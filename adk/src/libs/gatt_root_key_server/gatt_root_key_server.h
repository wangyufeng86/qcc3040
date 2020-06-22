/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file

@brief  Header file for the GATT root key service server library.

        This file provides documentation for the GATT root key server library
        API (library name: gatt_root_key_server).
*/

#ifndef GATT_ROOT_KEY_SERVER_H_
#define GATT_ROOT_KEY_SERVER_H_

#include <csrtypes.h>
#include <message.h>

#include <library.h>

#include "gatt_manager.h"
/* Common defines between the client and server */
#include "gatt_root_key_service.h"


/*! Internal structure for the root key server service (server)

    This structure is visible to the application as it is responsible 
    for managing resources.

    \note The elements of this structure are only modified by the root key 
        server library.
    \note The application SHOULD NOT modify the values at any time as it 
        could lead to undefined behaviour.
 */
typedef struct
{
    TaskData lib_task;              /*!< The task for an instance of the GATT server */
    Task app_task;                  /*!< Task for the app using the server */
    uint16 state;                   /*!< Internal state of the server */
                                    /*! Features supported by the server */
    GattRootKeyServiceFeatures  features; 
    GattRootKeyServiceStatus    status;
                                    /*! The client configuration for the mirror challenge
                                        control point. */
    uint16                      mirror_client_config;
    GRKS_KEY_T secret;              /*!< The secret being used in the initial challenge*/
    bdaddr local_address;           /*!< Address of this device to be used in the challenge.
                                         Stored as can potentially be using random/public/resolvable */
    bdaddr remote_address;          /*!< Address of the remote device to be used in the challenge.
                                         Stored as can potentially be using random/public/resolvable */
    GRKS_KEY_T local_random;        /*!< Random number generated locally */
    GRKS_KEY_T remote_random;       /*!< Random number received from remote */
    GRKS_KEY_T hashA_out;           /*!< Hash generated locally */
    GRKS_KEY_T hashB_in;            /*!< Hash received from remote */
    GRKS_IR_KEY_T ir_key;           /*!< Received IR key */
    GRKS_IR_KEY_T er_key;           /*!< Received ER key */
    bool challenge_response_needed; /*!< Is a message required for the outcome of 
                                            GattRootKeyServerReadyForChallenge() */
    uint16 commit_cid;              /*!< The cid used when responding to a key update */
} GATT_ROOT_KEY_SERVER;



typedef struct
{
    const GATT_ROOT_KEY_SERVER *instance;
    gatt_root_key_challenge_status_t  status;

} GATT_ROOT_KEY_SERVER_CHALLENGE_IND_T;


typedef struct
{
    const GATT_ROOT_KEY_SERVER *instance;
    GRKS_IR_KEY_T               ir;
    GRKS_ER_KEY_T               er;

} GATT_ROOT_KEY_SERVER_KEY_UPDATE_IND_T;


/*! Enumeration of messages an application task can receive from the 
    root key server library.
 */
typedef enum
{
        /*! A root key service challenge has completed */
    GATT_ROOT_KEY_SERVER_CHALLENGE_IND = GATT_ROOT_KEY_SERVER_MESSAGE_BASE,
        /*! The client has requested that the following keys be updated */
    GATT_ROOT_KEY_SERVER_KEY_UPDATE_IND,

        /*! Library message limit */
    GATT_ROOT_KEY_SERVER_MESSAGE_TOP
} gatt_root_key_server_message_id_t;

/*!
    Optional parameters used by the Initialisation API

    Parameters that can define how the root key server library is initialised.
 */
typedef struct
{
   bool enable_notifications;       /*!< Flag that can be used to enable or disable notifications */
                                    /*! Initial value for features */
   GattRootKeyServiceFeatures features;
} gatt_root_key_server_init_params_t;


/*!
    @brief Initialises the Root Key Service Library in the Server role.

    @param instance     pointer to the data for this instance of the server
    @param app_task     The Task that will receive messages sent from this library.
    @param init_params  pointer to initialization parameters (mandatory)
    @param start_handle start handle
    @param end_handle   end handle

    @return TRUE if successful, FALSE otherwise

*/
bool GattRootKeyServerInit(GATT_ROOT_KEY_SERVER *instance,
                           Task app_task,
                           const gatt_root_key_server_init_params_t *init_params,
                           uint16 start_handle,
                           uint16 end_handle);


/*! Allow the server to autonomously handle a challenge.

    This API should be used when the GATT server is connected to a target
    peer device and allows the server to autonomously respond to challenges.
    On completeion, or failure, a GATT_ROOT_KEY_SERVER_CHALLENGE_IND message.
    will be sent.

    \note The device bluetooth addresses used in the challenge are supplied in 
    this API, allowing the application to decide if verifying using public, 
    resolvable or random addresses.

    \param[in]  instance        The gatt server instance memory
    \param[in]  secret          The shared secret that is tested by the challenge
    \param[in]  local_address   The bluetooth address for this device to be verified
                                as part of the challenge.
    \param[in]  remote_address  The bluetooth address for this device to be verified
                                as part of the challenge.

    \return TRUE if the server is able to start processing challenges, FALSE
            if not in a valid state. If FALSE is returned then a 
            GATT_ROOT_KEY_SERVER_CHALLENGE_IND will also be sent with a more
            detailed error status.
 */
bool GattRootKeyServerReadyForChallenge(GATT_ROOT_KEY_SERVER *instance,
                                        const GRKS_KEY_T *secret,
                                        const bdaddr   *local_address,
                                        const bdaddr   *remote_address);


/*! Must be called after receiving a GATT_ROOT_KEY_SERVER_KEY_UPDATE_IND message.

    This API should be used to respond to the remote side, to indicate that it has finished 
    processing the GATT_ROOT_KEY_SERVER_KEY_UPDATE_IND message.

    @param instance     pointer to the data for this instance of the server

    @return TRUE if successful, FALSE otherwise

*/
bool GattRootKeyServerKeyUpdateResponse(const GATT_ROOT_KEY_SERVER *instance);


#endif /* GATT_ROOT_KEY_SERVER_H_ */
