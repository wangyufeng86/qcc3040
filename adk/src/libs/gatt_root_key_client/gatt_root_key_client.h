/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file    
@brief   Header file for the GATT root key client library.

        This file provides documentation for the GATT root key client library
        API (library name: gatt_root_key_client).
*/

#ifndef GATT_ROOT_KEY_CLIENT_H_
#define GATT_ROOT_KEY_CLIENT_H_

#include <csrtypes.h>
#include <message.h>

#include <library.h>
#include <bdaddr.h>

/* Use common defines between server and client */
#include <gatt_root_key_service.h>

/*! @brief The root_key internal structure for the client role.

    This structure is visible to the application as it is responsible for managing resource to pass to the root_key library.
    The elements of this structure are only modified by the root_key library.
    The application SHOULD NOT modify the values at any time as it could lead to undefined behaviour.

 */
typedef struct
{
    TaskData lib_task;                  /*!< This task */
    Task app_task;                      /*!< Application task passed in GattRootKeyClientInit */
    uint16 handle_features;             /*!< Handle for the Root Key Transfer service features characteristic */
    uint16 handle_status;               /*!< Handle for the Root Key Transfer service Status characteristic */
    uint16 handle_challenge_control;    /*!< Handle for the Root Key Transfer service 
                                                challenge control point characteristic */
    uint16 handle_challenge_control_end;/*!< Handle following the Root Key Transfer service 
                                                challenge control point characteristic. We keep this
                                                so we can enable indications (the handle needed is
                                                within this range) */
    uint16 handle_challenge_control_config;/*!< Handle for the configuration for the challenge control
                                                point characteristic */
    bool handle_challenge_control_config_found;/*!< Indicate if the config handle has been found */
    uint16 handle_keys_control;         /*!< Handle for the Root Key Transfer service 
                                                challenge key exchange characteristic */
    GRKS_KEY_T secret;                  /*!< The secret being used in the initial challenge*/
    bdaddr local_address;               /*!< Address of this device to be used in the challenge.
                                             Stored as can potentially be using random/public/resolvable */
    bdaddr remote_address;              /*!< Address of the remote device to be used in the challenge.
                                             Stored as can potentially be using random/public/resolvable */
    GRKS_KEY_T local_random;            /*!< Locally generated random number */
    GRKS_KEY_T remote_random;           /*!< Random number supplied by remote side */

    GRKS_KEY_T hashB_out;               /*!< Hash generated locally */
    GRKS_KEY_T hashA_in;                /*!< Hash received */

    GRKS_KEY_T ir;                      /*!< IR to transfer */
    GRKS_KEY_T er;                      /*!< ER to transfer */


    uint16 state;                       /*!< State of the client implementation. \ref gatt_root_key_client_state.h */
    bool init_response_needed;          /*!< Need to send a response for init */
    bool challenge_response_needed;     /*!< Need to send a response for challenge */
    bool writekeys_response_needed;     /*!< Need to send a response for write keys */
} GATT_ROOT_KEY_CLIENT;


/*!
    @brief Status code returned from the GATT root_key client library

    This status code indicates the outcome of the request.
*/
typedef enum
{
    gatt_root_key_client_status_success,
    gatt_root_key_client_status_not_allowed,
    gatt_root_key_client_status_failed,
} gatt_root_key_client_status_t;


/*!
    @brief Status code returned following a GattRootKeyClientWriteKey().

    The status is returned in a GATT_ROOT_KEY_CLIENT_WRITE_KEY_IND_T message.
*/
typedef enum
{
        /*! Indicates that the key transfer has completed successfully */
    gatt_root_key_client_write_keys_success,
        /*! Indicates that the key transfer has failed and cannot be repeated
            without a fresh challenge. */
    gatt_root_key_client_write_keys_fatal,
        /*! Cannot transfer keys as the service is disabled */
    gatt_root_key_client_write_keys_disabled,
        /*! Cannot transfer keys as the service is disabled */
    gatt_root_key_client_write_keys_parameter_error,
} gatt_root_key_client_write_key_status_t;



#define GRKC_KEY_SIZE_128BIT_OCTETS     (128/8)
#define GRKC_IR_KEY_SIZE_OCTETS         GRKS_KEY_SIZE_128BIT_OCTETS
#define GRKC_ER_KEY_SIZE_OCTETS         GRKS_KEY_SIZE_128BIT_OCTETS

/*!
    @brief Contents of the GATT_ROOT_KEY_CLIENT_INIT_CFM message that is sent by the library,
    as a response to the initialisation request.
 */
typedef struct
{
    const GATT_ROOT_KEY_CLIENT *instance;
    gatt_root_key_client_status_t  status;

} GATT_ROOT_KEY_CLIENT_INIT_CFM_T;


typedef struct
{
    const GATT_ROOT_KEY_CLIENT *instance;
    gatt_root_key_challenge_status_t  status;

} GATT_ROOT_KEY_CLIENT_CHALLENGE_PEER_IND_T;


typedef struct
{
    const GATT_ROOT_KEY_CLIENT *instance;
    gatt_root_key_client_write_key_status_t  status;

} GATT_ROOT_KEY_CLIENT_WRITE_KEY_IND_T;


/*! @brief Contents of the GATT_ROOT_KEY_CLIENT_SET_NOTIFICATION_ENABLE_CFM message that is sent by the library,
    as a result of setting notifications on the server.
 */
typedef struct
{
    const GATT_ROOT_KEY_CLIENT *instance;
    gatt_root_key_client_status_t status;
} GATT_ROOT_KEY_CLIENT_SET_INDICATION_ENABLE_CFM_T;

/*! @brief Enumeration of messages a client task may receive from the root_key client library.
 */
typedef enum
{
    /* Client messages */
                    /*! Message sent when the gatt client is initialised */
    GATT_ROOT_KEY_CLIENT_INIT_CFM = GATT_ROOT_KEY_CLIENT_MESSAGE_BASE,
    GATT_ROOT_KEY_CLIENT_CHALLENGE_PEER_IND,
    GATT_ROOT_KEY_CLIENT_WRITE_KEY_IND,
    GATT_ROOT_KEY_CLIENT_SET_INDICATION_ENABLE_CFM,

    /* Library message limit */
    GATT_ROOT_KEY_CLIENT_MESSAGE_TOP
} gatt_root_key_client_message_id_t;


/*!
    @brief After the VM application has used the GATT manager library to establish a connection to a discovered BLE device in the Client role,
    it can discover any supported services in which it has an interest. It should then register with the relevant client service library
    (passing the relevant CID and handles to the service). For the root_key client it will use this API. The GATT manager 
    will then route notifications and indications to the correct instance of the client service library for the CID.

    @param instance A valid area of memory that the service library can use.
    @param app_task The Task that will receive the messages sent from this root_key client library.
    @param cid The connection ID.
    @param start_handle The start handle of the root_key client instance.
    @param end_handle The end handle of the root_key client instance.

    @return TRUE if successful, FALSE otherwise

*/
bool GattRootKeyClientInit(GATT_ROOT_KEY_CLIENT *instance,
                           Task app_task,
                           uint16 cid,
                           uint16 start_handle,
                           uint16 end_handle);


/*!
    @brief When a GATT connection is removed, the application must remove all client service instances that were
    associated with the connection (using the CID value).
    This is the clean up routine as a result of calling the GattRootKeyClientInit API. That is,
    the GattRootKeyClientInit API is called when a connection is made, and the GattRootKeyClientDestroy is called 
    when the connection is removed.

    @param instance The client instance that was passed into the GattRootKeyClientInit API.

    @return TRUE if successful, FALSE otherwise

*/
bool GattRootKeyClientDestroy(GATT_ROOT_KEY_CLIENT *instance);


/*! Start the process of confirming the peer device is compatible

    This function starts an autonomous exchange with the peer, completing by
    sending an GATT_ROOT_KEY_CLIENT_CHALLENGE_PEER_IND message.

    \note The device bluetooth addresses used in the challenge are supplied in 
    this API, allowing the application to decide if verifying using public, 
    resolvable or random addresses.

    \param[in]  instance        The gatt client instance memory
    \param[in]  secret          The shared secret that is tested by the challenge
    \param[in]  local_address   The bluetooth address for this device to be verified
                                as part of the challenge.
    \param[in]  remote_address  The bluetooth address for this device to be verified
                                as part of the challenge.

    \return TRUE if the client is able to start processing the challenge, FALSE
            if the client is not in a valid state.
*/
bool GattRootKeyClientChallengePeer(GATT_ROOT_KEY_CLIENT *instance,
                                    const GRKS_KEY_T *secret,
                                    const bdaddr   *local_address,
                                    const bdaddr   *remote_address);


/*! Synchronise the keys between devices

    Transfer keys to the connected GATT server device. A 
    GATT_ROOT_KEY_CLIENT_WRITE_KEY_IND message is sent on completion
    or failure.

    This function can only be called once a challenge has been successful
    \see GattRootKeyClientChallengePeer.

    \param[in]  instance    The gatt client instance memory
    \param[in]  ir_key      The IR portion of the key to be synchronised
    \param[in]  er_key      The IR portion of the key to be synchronised

    \return TRUE if the client is able to start processing the key 
            update, FALSE otherwise. If FALSE is returned than a 
            GATT_ROOT_KEY_CLIENT_WRITE_KEY_IND message is sent.
*/
bool GattRootKeyClientWriteKeyPeer(GATT_ROOT_KEY_CLIENT *instance,
                               const GRKS_IR_KEY_T *ir_key,
                               const GRKS_ER_KEY_T *er_key);

#endif /* GATT_ROOT_KEY_CLIENT_H_ */
