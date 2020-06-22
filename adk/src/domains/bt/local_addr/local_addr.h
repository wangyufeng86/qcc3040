/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Header file for Bluetooth Local Address component

*/

#ifndef _DOMAINS_BT_LOCAL_ADDR_
#define _DOMAINS_BT_LOCAL_ADDR_

#include "domain_message.h"

/*! Local address status */
typedef enum
{
    /*! Success */
    local_addr_success,
    /*! Failure */
    local_addr_failure
} local_addr_status_t;

/*! Messages sent by the Local Addr module. */
typedef enum
{
    /*! Message confirming that the Local Addr module initialisation is complete */
    LOCAL_ADDR_CONFIGURE_BLE_GENERATION_CFM = LOCAL_ADDR_MESSAGE_BASE,
} local_addr_message_t;

/*! Message payload for LOCAL_ADDR_CONFIGURE_BLE_GENERATION_CFM */
typedef struct
{
    /*! Success or failure */
    local_addr_status_t status;
} LOCAL_ADDR_CONFIGURE_BLE_GENERATION_CFM_T;

/*! Host address generation options */
typedef enum
{
    /*! None, use public address */
    local_addr_host_gen_none,
    /*! Host generates static random address */
    local_addr_host_gen_static,
    /*! Host generates resolvable private address */
    local_addr_host_gen_resolvable,
    /*! Host generates non-resolvable private address */
    local_addr_host_gen_non_resolvable
} local_addr_host_gen_t;

/*! Controller address generation options */
typedef enum
{
    /*! No controller generation, always use host address */
    local_addr_controller_gen_none,
    /*! Controller generates RPA where a local IRK is available */
    local_addr_controller_gen_rpa
} local_addr_controller_gen_t;

/*
    \brief Initialise Local Address module

    \param init_task Unused

    \return TRUE to indicate successful initialisation,
            FALSE otherwise.
*/
bool LocalAddr_Init(Task init_task);

/*
    \brief Get Own Address Type to use when configuring
           LE scan, advertise or connect

    \return Type to use in the own_address_type field
*/
uint8 LocalAddr_GetBleType(void);

/*
    \brief Configure BLE address generation. Note that the configuration
           will only take effect once own_address_type is set using the
           value returned by LocalAddr_GetBleType
    
    \param task The Task to send confirmation message to
    \param host The method for the host to use to generate random addresses
    \param controller The method for the controller to use to generate 
           random addresses

    \return Will send a LOCAL_ADDR_CONFIGURE_BLE_GENERATION_CFM
            message to the client's task.
*/
void LocalAddr_ConfigureBleGeneration(Task task, local_addr_host_gen_t host, local_addr_controller_gen_t controller);

/*
    \brief Release BLE address generation configuration. Note that the configuration
           will only return to public address once own_address_type is set using the
           value returned by LocalAddr_GetBleType
    
    \param task The Task used to configure BLE address generation

    \return TRUE if successful, FALSE otherwise
*/
bool LocalAddr_ReleaseBleGeneration(Task task);

/*
    \brief Handle connection library messages
    
    \param id The message ID
    \param message The message payload
    \param already_handled Indicates if another handler has already processed this message

    \return TRUE if message was handled in local_addr, otherwise already_handled
*/
bool LocalAddr_HandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled);

/*
    \brief Is the local device using its public address

    \return TRUE if local address is public, otherwise FALSE
*/
bool LocalAddr_IsPublic(void);

#endif /* _DOMAINS_BT_LOCAL_ADDR_ */
