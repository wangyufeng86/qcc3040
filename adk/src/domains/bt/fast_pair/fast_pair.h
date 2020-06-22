/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair.h
\brief      Header file for the Fast Pair task
*/

#ifndef FAST_PAIR_H_
#define FAST_PAIR_H_

#include <connection.h>
#include <connection_manager.h>
#include <pairing.h>
#include <task_list.h>
#include <logging.h>
#include <panic.h>
#include <stdlib.h>
#include <util.h>
#include <byte_utils.h>

#include "domain_message.h"



#define PASSKEY_INVALID         0xFF000000UL


#define MAX_FAST_PAIR_ACCOUNT_KEYS              (5)

#define FAST_PAIR_ENCRYPTED_REQUEST_LEN         (16)
#define FAST_PAIR_ENCRYPTED_PASSKEY_BLOCK_LEN   (16)
#define FAST_PAIR_ENCRYPTED_ACCOUNT_KEY_LEN     (16)

#define FAST_PAIR_PUBLIC_KEY_LEN                (64)
#define FAST_PAIR_PRIVATE_KEY_LEN               (32)
#define FAST_PAIR_ACCOUNT_KEY_LEN               (16)
#define FAST_PAIR_AES_KEY_LEN                   (16)
#define FAST_PAIR_AES_BLOCK_SIZE                (16)


#define FAST_PAIR_PROVIDER_ADDRESS_OFFSET       (2)
#define FAST_PAIR_SEEKER_ADDRESS_OFFSET         (8)


/* FP state timeout in seconds*/
#define FAST_PAIR_STATE_TIMEOUT                 (10)
/* FP Seeker triggered discoverability timeout in seconds */
#define FAST_PAIR_DISCOVERABILITY_TIMEOUT       (10)
/* Quarantine timeout in seconds*/
#define FAST_PAIR_QUARANTINE_TIMEOUT            (300)
/* Maximum bumber of Failure attempts */
#define FAST_PAIR_MAX_FAIL_ATTEMPTS             (10)
/* Maximum BLE connections allowed */
#define MAX_BLE_CONNECTIONS    (2)

/*! \brief Type of passkey owners */
typedef enum
{
    fast_pair_passkey_seeker = 2,
    fast_pair_passkey_provider = 3
} fast_pair_passkey_owner_t;

/*! \brief Type of events passed to fast pair state machine */
typedef enum
{
     fast_pair_state_event_kbp_write = 0,              /*! Key based charactersitic write occured */
     fast_pair_state_event_passkey_write,              /*! Passkey charactersitic write occured */
     fast_pair_state_event_account_key_write,          /*! Account key charactersitic write occured */
     fast_pair_state_event_pairing_request,            /*! BR/EDR Pairing request recevived */
     fast_pair_state_event_provider_passkey,           /*! Provider Passkey recieved */
     fast_pair_state_event_connect,                    /*! ACL connection occured */
     fast_pair_state_event_disconnect,                 /*! ACL disconnection occured */
     fast_pair_state_event_handset_connect_allow,      /*! handset connection allowed */
     fast_pair_state_event_handset_connect_disallow,   /*! handset connection disallowed*/
     fast_pair_state_event_rpa_addr,                   /*! Read RPA Addr confirmation */
     fast_pair_state_event_crypto_shared_secret,       /*! Crypto ECDH Shared Secret confirmation */
     fast_pair_state_event_crypto_hash,                /*! Crypto Hash confirmation */
     fast_pair_state_event_crypto_encrypt,             /*! Crypto AES Encrypt confirmation */
     fast_pair_state_event_crypto_decrypt,             /*! Crypto AES Decrypt confirmation */
     fast_pair_state_event_timer_expire,               /*! FP Procedure Timer expired */
     fast_pair_state_event_power_off,                  /*! Power Off event by user */
     fast_pair_state_event_auth,                       /*! CL_SM_AUTHENTICATE_CFM received from application */
} fast_pair_state_event_id;


/*! \brief Event structure that is used to inject an event and any corresponding
    arguments into the state machine. */
typedef struct
{
    fast_pair_state_event_id id;
    void *args;
} fast_pair_state_event_t;

/*! \brief Event arguments for SM authentication event */
typedef struct
{
    CL_SM_AUTHENTICATE_CFM_T* auth_cfm;
} fast_pair_state_event_auth_args_t;

/*! \brief Fast Pair module state machine states */
typedef enum fast_pair_states
{
    FAST_PAIR_STATE_NULL,            /*!< Startup state */
    FAST_PAIR_STATE_IDLE,            /*!< No fast pairing happening */

    FAST_PAIR_STATE_WAIT_AES_KEY,    /*!< Processing Key Based Pairing Write */
    FAST_PAIR_STATE_WAIT_PAIRING_REQUEST,   /*!< Waiting for FP Seeker to send pairing request */

    FAST_PAIR_STATE_WAIT_PASSKEY,    /*!< Waiting for FP Seeker to write passkey */
    FAST_PAIR_STATE_WAIT_ACCOUNT_KEY,/*!< Wait for Account Key write by FP Seeker */
} fastPairState;



/*! \brief Stucture used to to store account keys and
    iterate through the list to calculate the right 'K'
 */
typedef struct
{
    uint8 num_keys;
    uint8 num_keys_processed;
    uint8* keys;
} fast_pair_account_keys_t;

/*! \brief Fast Pair Session Data structure */
typedef struct
{
    /*! Place holder for ASPK */
    uint16* private_key;
    /*! Place holder for public key provided by FP Seeker */
    uint16* public_key;
    /*! Place holder for encrypted data written by FP Seeker */
    uint16* encrypted_data;
    /*! Place holder for calculated AES key */
    uint16* aes_key;
    /*! Discoverbaility enabled by FP Seeker */
    bool    discoverability_flag;
    /*! Cached account keys read from PS */
    fast_pair_account_keys_t   account_key;

} fast_pair_session_data_t;

/*! \brief Fast Pair task structure */
typedef struct
{
    /*! The fast pairing module task */
    TaskData task;
    /*! The current fast pairing state */
    fastPairState state;
    /*! RPA address used to make LE connection with FP Seeker, 0 otherwise */
    bdaddr   rpa_bd_addr;
    /*! Failure Counter */
    uint16  failure_count;
    /*! Session Data. Will be cleaned when moving to Idle */
    fast_pair_session_data_t session_data;
    /*! Peer BD address used to make ACL connection */
    bdaddr peer_bd_addr[MAX_BLE_CONNECTIONS];
    /*! Random address used to make ACL connection */
    bdaddr own_random_address[MAX_BLE_CONNECTIONS];
} fastPairTaskData;


/*! Initialise the fast pair application module.

    This function is called to initilaze fast pairing module.
    If a message is processed then the function returns TRUE.

    \param  init_task       Initialization Task

    \returns TRUE if successfully processed
 */
bool FastPair_Init(Task init_task);

void FastPair_SetPrivateKey(const uint16 *key, unsigned size_of_key);

/*! Message Handler to handle CL message coming from application

    \param   id        Identifier of the message
    \param  message        The message content
    \param  already_handled        boolean variable to check if message is already handled or not

    \returns  already_handled
 */
bool FastPair_HandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled);


/*! Handler for all messages to fast pair Module

    This function is called to handle any messages sent to the fast pairing module.
    If a message is processed then the function returns TRUE.

    \note Some connection library messages can be sent directly as the
        request is able to specify a destination for the response.

    \param  task            Task to which this message was sent
    \param  id              Identifier of the message
    \param  message         The message content (if any)

    \returns None
 */
void FastPair_HandleMessage(Task task, MessageId id, Message message);


/*! \brief Initialize the Fast Pair Session Data Module

    \param None

    \return None
 */
void FastPair_RegisterPersistentDeviceDataUser(void);

/*! Handler for all events sent to fast pair state machine

    This function is called to handle any event sent to
    the FP state machine. If a message is processed then the function returns TRUE.

    \param  event           Identifier of the event

    \returns TRUE if the event has been processed, otherwise returns the
        value in already_handled
 */
bool fastPair_StateMachineHandleEvent(fast_pair_state_event_t event);

/*! Set state of Fast Pair procedure

    This function is called set the fast pair state.

    \param  theFastPair     Fast Pair Data context

    \param  state           Fast pair state
 */
void fastPair_SetState(fastPairTaskData *theFastPair, fastPairState state);

/*! Get state of Fast Pair procedure

    This function is called fetch the fast pair state.

    \param  theFastPair     Fast Pair Data context

    \returns fast pair state
 */
fastPairState fastPair_GetState(fastPairTaskData *theFastPair);

/*! Get pointer to Fast Pair data structure

    This function is called fetch the fast pair context.

    \param  None

    \returns fast pair data context
 */
fastPairTaskData* fastPair_GetTaskData(void);

/*! Start 10 seconds Timer for next step in Fast Pair procedure

    This function is Starts timer.

    \param  isQuarantine    If TRUE then 5 miinutes timer is set.
                            Else 10 seconds timer is started

 */
void fastPair_StartTimer(bool isQuarantine);

/*! Stop Timer and continue Fast Pair procedure

    This function is Stops timer.

 */
void fastPair_StopTimer(void);


/*! \brief Is idle */
#define fastPair_IsIdle() \
    (fastPair_GetTaskData()->state == FAST_PAIR_STATE_IDLE)

#endif /* FAST_PAIR_H_ */
