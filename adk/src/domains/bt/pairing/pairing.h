/*!
\copyright  Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       pairing.h
\brief      Header file for the Pairing task
*/

#ifndef PAIRING_H_
#define PAIRING_H_

#include <connection.h>

#include "domain_message.h"
#include <task_list.h>
#include <marshal.h>

/*!
    @startuml

    [*] -down-> NULL
    NULL -down-> INITIALISING : Pairing_Init()
    INITIALISING : Registering EIR data
    INITIALISING -down-> IDLE : EIR registration complete
    IDLE : Page and Inquiry scan disabled
    
    state DevicePairing {
        DevicePairing : Page scan enabled
        IDLE -down-> DISCOVERABLE : INTERNAL_PAIR_REQ
        IDLE -down-> PENDING_AUTHENTICATION : INTERNAL_PAIR_REQ(known addr)
        DISCOVERABLE : Inquiry scan enabled
        DISCOVERABLE : Awaiting device connection
        DISCOVERABLE -up-> IDLE : PAIR_CFM(timeout/cancelled)
        DISCOVERABLE -down-> PENDING_AUTHENTICATION : Start authentication
        PENDING_AUTHENTICATION : Pairing in progress
        PENDING_AUTHENTICATION --> IDLE : PAIR_CFM(success/failed)
    }

    footer Note that PAIRING_STATE_ prefix dropped from states and PAIRING_ prefix dropped from messages.

    @enduml
*/

/*! Defines the pairing client task list initial capacity */
#define PAIRING_CLIENT_TASK_LIST_INIT_CAPACITY 1
/*! Defines the pairing activity task list initial capacity */
#define PAIRING_ACTIVITY_LIST_INIT_CAPACITY 3


/*! \brief Pairing module state machine states */
typedef enum pairing_states
{
    PAIRING_STATE_NULL,                /*!< Startup state */
    PAIRING_STATE_INITIALISING,        /*!< Initialising state */
    PAIRING_STATE_IDLE,                /*!< No pairing happening */
    PAIRING_STATE_DISCOVERABLE,         /*!< Discoverable to the device */
    PAIRING_STATE_PENDING_AUTHENTICATION, /*!< Waiting to authenticate with device */
 } pairingState;

/*! \brief Pairing module UI Provider contexts */
typedef enum
{
    context_handset_pairing_idle,
    context_handset_pairing_active,

} pairing_provider_context_t;

/*! \brief Internal message IDs */
enum pairing_internal_message_ids
{
    PAIRING_INTERNAL_PAIR_REQ,                  /*!< Pair with handset/phone/AV source */
    PAIRING_INTERNAL_LE_PEER_PAIR_REQ,          /*!< Pair with le peer */
    PAIRING_INTERNAL_PAIR_LE_REQ,               /*!< Pair with le handset */
    PAIRING_INTERNAL_TIMEOUT_IND,               /*!< Pairing has timed out */
    PAIRING_INTERNAL_PAIR_STOP_REQ,             /*!< Stop in progress pairing */
    PAIRING_INTERNAL_DISABLE_SCAN               /*!< Delayed message to disable page and inquiry scan */
};

/*! \brief Definition of the #PAIRING_INTERNAL_PAIR_REQ message content */
typedef struct
{
    /*! The requester's task */
    Task client_task;
    /*! Address to pair */
    bdaddr addr;
    /*! If the request was user initiated (opposed to automatically initiated by software */
    bool is_user_initiated;
} PAIR_REQ_T;

/*! \brief Definition of the #PAIRING_INTERNAL_LE_PEER_PAIR_REQ message content */
typedef struct
{
    /*! The requester's task */
    Task client_task;
    /*! Address to pair */
    typed_bdaddr typed_addr;
    /*! le peer pairing works differently if it's a client or server */
    bool le_peer_server;
} PAIR_LE_PEER_REQ_T;

/*! \brief Definition of the #PAIRING_INTERNAL_PAIR_STOP_REQ_T message content */
typedef struct
{
    /*! The requester's task */
    Task client_task;
}PAIRING_INTERNAL_PAIR_STOP_REQ_T;

/*! Handling of pairing requests for a BLE device */
typedef enum
{
        /* only BLE connections to devices support secure connections
           will be permitted. These devices provide BLE pairing
           automatically when the handset pairs over BREDR. */
    pairingBleDisallowed,
        /* Pairing requests for a BLE link will be processed BUT when
           the simple pairing completes, if the public address does
           not match a paired handset - the link will be disconnected
           and pairing forgotten */
    pairingBleOnlyPairedHandsets,
        /* Pairing requests for a BLE link will be processed if a
           random address is used.
           When pairing completes, the device will be saved, unless it
           turns out that the address was not resolvable. In which case
           the link is disconnected and pairing forgotten */
    pairingBleAllowOnlyResolvable,
        /* All pairing requests for a BLE link will be processed.
           When pairing completes, the device will be saved, using
           the resolvable (public) address if available. */
    pairingBleAllowAll,
} pairingBlePermission;


/*! Pairing task structure */
typedef struct
{
    /*! The pairing module task */
    TaskData task;
    /*! The pairing module client's task */
    Task     client_task;
    /*! The pairing stop request task */
    Task     stop_task;
    /*! Client task for concurrent BLE handset pairing */
    Task     pair_le_task;
    /*! client list that the pairing module shall send indication messages to */
    TASK_LIST_WITH_INITIAL_CAPACITY(PAIRING_CLIENT_TASK_LIST_INIT_CAPACITY) client_list;
    /*! The current pairing state */
    pairingState state;
    /*! Set if the current pairing is user initiated */
    bool     is_user_initiated:1;
    /*! BT address of device if pairing request by peer, 0 otherwise */
    bdaddr   device_to_pair_with_bdaddr;
    /*! Ensure only 1 pairing operation can be running. */
    uint16   pairing_lock;
    /*! Number of unacknowledged peer signalling msgs */
    uint16   outstanding_peer_sig_req;
    /*! How to handle BLE pairing */
    pairingBlePermission    ble_permission;
    /*! The current BLE link pending pairing. This will be random address if used. */
    typed_bdaddr            pending_ble_address;

    TASK_LIST_WITH_INITIAL_CAPACITY(PAIRING_ACTIVITY_LIST_INIT_CAPACITY) pairing_activity;
} pairingTaskData;

/*! Pairing status codes */
typedef enum pairing_status
{
    pairingSuccess,
    pairingNotReady,
    pairingAuthenticationFailed,
    pairingtNoLinkKey,
    pairingTimeout,
    pairingUnknown,
    pairingStopped,
    pairingFailed,

    /* Activity statuses */
    pairingInProgress,
    pairingNotInProgress,
    pairingCompleteVersionChanged,
    pairingLinkKeyReceived,
} pairingStatus;

/*! \brief Message IDs from Pairing task to main application task */
enum pairing_messages
{
    /*! Message confirming pairing module initialisation is complete. */
    PAIRING_INIT_CFM = PAIRING_MESSAGE_BASE,
    /*! Message confirming pairing is complete. */
    PAIRING_PAIR_CFM,
    PAIRING_STOP_CFM,
    PAIRING_ACTIVITY,
    PAIRING_ACTIVE_USER_INITIATED,
    PAIRING_ACTIVE,
    PAIRING_INACTIVE_USER_INITIATED,
    PAIRING_INACTIVE,
    PAIRING_COMPLETE,
    PAIRING_FAILED
};

/*! \brief Definition of #PAIRING_PAIR_CFM message. */
typedef struct
{
    /*! The status result of the pairing */
    pairingStatus status;
    /*! The address of the paired device */
    bdaddr device_bd_addr;
} PAIRING_PAIR_CFM_T;

/*! \brief Message indicating pairing activity.
    For example pairing is in progress.
*/
typedef struct
{
    pairingStatus status;
    bdaddr device_addr;
} PAIRING_ACTIVITY_T;
/*! Marshalling type definition for PAIRING_ACTIVITY_T */
extern const marshal_type_descriptor_t marshal_type_descriptor_PAIRING_ACTIVITY_T;

/*!< App pairing task */
extern pairingTaskData pairing_task_data;

/*! Get pointer to Pairing data structure */
#define PairingGetTaskData()             (&pairing_task_data)

/*! Get pointer to Pairing client list */
#define PairingGetClientList()             (task_list_flexible_t *)(&pairing_task_data.client_list)

/*! Get pointer to Pairing's pairing activity */
#define PairingGetPairingActivity()             (task_list_flexible_t *)(&pairing_task_data.pairing_activity)

/*! \brief Initialise the pairing application module.
 */
bool Pairing_Init(Task init_task);

/*! \brief Pair with a device, where inquiry scanning is required.

    \param[in] client_task       Task to send #PAIRING_PAIR_CFM response message to.
    \param     is_user_initiated TRUE if this is a user initiated request.
 */
void Pairing_Pair(Task client_task, bool is_user_initiated);

/*! \brief Pair with a device where the address is already known.

    Used to pair with a device where the BT address is already known and inquiry 
    scanning is not required. Typically in response to receiving the address from 
    peer earbud via peer signalling channel.

    \param[in] client_task  Task to send #PAIRING_PAIR_CFM response message to.
    \param[in] handset_addr Pointer to BT address of handset.
 */
void Pairing_PairAddress(Task client_task, bdaddr* device_addr);


/*! \brief Stop a pairing.

    If successfully stopped the client_task passed when initiating pairing
    will receive a PAIRING_PAIR_CFM message with a status code of pairingStopped.

    The client_task passed to this function will receive the message 
    PAIRING_STOPPED_CFM once pairing has stopped. Note that if it is too
    late to stop the pairing then PAIRING_STOPPED_CFM will be sent when the pairing
    is complete. In such circumstances the client passed to the pairing function will receive
    a PAIRING_PAIR_CFM message with a status code indicating the result of the
    pairing operation.

    Calling this function a second time, before receiving PAIRING_STOP_CFM will
    result in a panic.

    \param[in] client_task  Task to send #PAIRING_STOP_CFM response message to.
 */
void Pairing_PairStop(Task client_task);


/*! Determine how BLE connections may pair

    Decide behaviour of the pairing code when a connection to an unpaired
    BLE device occurs.

    When BLE devices use resolvable random addresses the public address cannot
    be identified until the pairing is completed.

    See the definition for \ref pairingBlePermission to see what options are
    possible.

    \param permission The permission to use for any BLE pairing in future. This
        will not apply to any pairing that has already started.
*/
void Pairing_BlePermission(pairingBlePermission permission);


/*! Handler for all connection library messages not sent directly

    This function is called to handle any connection library messages sent to
    the application that the pairing module is interested in. If a message
    is processed then the function returns TRUE.

    \note Some connection library messages can be sent directly as the
        request is able to specify a destination for the response.

    \param  id              Identifier of the connection library message
    \param  message         The message content (if any)
    \param  already_handled Indication whether this message has been processed by
                            another module. The handler may choose to ignore certain
                            messages if they have already been handled.

    \returns TRUE if the message has been processed, otherwise returns the
        value in already_handled
 */
bool Pairing_HandleConnectionLibraryMessages(MessageId id,Message message, bool already_handled);

/*! Register to receive PAIRING_ACTIVITY messages.
    \param task Task to send the messages to.
*/
void Pairing_ActivityClientRegister(Task task);

/*! Add a device to the paired device list.

    This is used to add a device to the PDL

    \param  address         The address of the device
    \param  key_length      Length of link_key
    \param  link_key        Pointer to the link key
 */
void Pairing_AddAuthDevice(const bdaddr* address, const uint16 key_length, const uint16* link_key);

/*! \brief Pair with le peer device. This is a temporary function until
           discovery is removed from the pairing module.
 */
void Pairing_PairLePeer(Task client_task, typed_bdaddr* device_addr, bool server);


/*! \brief Pair with a BLE handset device.

    \param[in] client_task  Task to send #PAIRING_PAIR_CFM response message to.
    \param[in] handset_addr Pointer to BT address of handset.
*/
void Pairing_PairLeAddress(Task client_task, const typed_bdaddr* device_addr);


/*! \brief TEST FUNCTION to force link key TX to peer on reboot. */
void Pairing_SetLinkTxReqd(void);

/*! \brief TEST FUNCTION */
void Pairing_ClearLinkTxReqd(void);


/*! \brief Is idle */
#define appPairingIsIdle() \
    (PairingGetTaskData()->state == PAIRING_STATE_IDLE)

/*! \brief Callback for Remote IO capability */
typedef void (*pairing_remote_io_capability_callback_t)(const CL_SM_REMOTE_IO_CAPABILITY_IND_T* ind);

typedef struct
{
    cl_sm_io_capability io_capability;
    mitm_setting        mitm;
    bool                bonding;
    uint16              key_distribution;
    uint16              oob_data;
    uint8*              oob_hash_c;
    uint8*              oob_rand_r;
} pairing_io_capability_rsp_t;

/*! \brief Callback for IO capability request */
typedef pairing_io_capability_rsp_t (*pairing_io_capability_request_callback_t)(const CL_SM_IO_CAPABILITY_REQ_IND_T* ind);

typedef enum
{
    pairing_user_confirmation_reject,
    pairing_user_confirmation_accept,
    pairing_user_confirmation_wait,
} pairing_user_confirmation_rsp_t;

/*! \brief Callback for user confirmation */
typedef pairing_user_confirmation_rsp_t (*pairing_user_confirmation_callback_t)(const CL_SM_USER_CONFIRMATION_REQ_IND_T* ind);

/*! \brief Pairing plugin */
typedef struct
{
    pairing_remote_io_capability_callback_t     handle_remote_io_capability;
    pairing_io_capability_request_callback_t    handle_io_capability_req;
    pairing_user_confirmation_callback_t        handle_user_confirmation_req;
} pairing_plugin_t;

/*! 
    \brief Register pairing plugin callback

    \param plugin The plugin to register. This function will panic if
           called multiple times without calling Pairing_PluginUnregister
           inbetween
 */
void Pairing_PluginRegister(pairing_plugin_t plugin);

/*!
    \brief Retry user confirmation where pairing_user_confirmation_callback_t
           previously returned pairing_user_confirmation_wait

    \returns TRUE if user confirmation can be retried, FALSE if there is no
             pending user confirmation to retry. This may happen due to
             incorrect call sequence, pairing failure/timeout or shortage of
             memory to store the confirmation request.
 */
bool Pairing_PluginRetryUserConfirmation(void);

/*!
    \brief Unregister pairing plugin callback

    \param plugin The plugin to unregister. This function will panic if
           this does not match the last plugin passed to Pairing_PluginRegister
 */
void Pairing_PluginUnregister(pairing_plugin_t plugin);

#endif /* PAIRING_H_ */
