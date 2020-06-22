/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_gfps.h
\brief      Header file of GATT FP Server sub-module
*/

#ifndef GATT_FP_SERVER_H_
#define GATT_FP_SERVER_H_

/*! Library Headers */
#include <gatt_fast_pair_server.h>
#include <gatt_fast_pair_server_uuids.h>

/*! Firmware Headers */
#include <csrtypes.h>
#include <message.h>

/*! Client Characteristic Configuration Descriptor Notification and Indication Bit Mask as per BT Spec(3.3.3.3) */
#define GATT_CCCD_NOTI_BITMASK  (0x0001)
#define GATT_CCCD_INDI_BITMASK  (0x0002)

#define ENABLE_NOTIFICATION_INDICATION (GATT_CCCD_NOTI_BITMASK|GATT_CCCD_INDI_BITMASK)

#define IS_KBP_NOTIFICATION_INDICATION_ENABLED(conex)  ((conex.client_config.key_based_pairing) & ENABLE_NOTIFICATION_INDICATION)
#define IS_PASSKEY_NOTIFICATION_INDICATION_ENABLED(conex)   ((conex.client_config.passkey) & ENABLE_NOTIFICATION_INDICATION)

/*! This enum defines the common enums for FP Characteristics to be used between GATT FP Server interface and State Machine */
typedef enum 
{
    FAST_PAIR_KEY_BASED_PAIRING = UUID_KEYBASED_PAIRING,
    FAST_PAIR_PASSKEY,
    FAST_PAIR_ACCOUNT_KEY
} gatt_fp_characteristics_t;

/*! GATT FP Client Configuration attributes that need to be stored per remote client */
typedef struct __gatt_fp_ccd_attributes_t
{
    uint16 key_based_pairing;    
    uint16 passkey;         
}gatt_fp_config_values_t;

/*! Structure holding information for the application handling of GATT FP Server  */
typedef struct __gatt_fp_data_t
{
    /*! Instance of GATT FP server */
    GFPS gfps;
    
    /*! Connection ID */
    uint16 cid;
    /*! Client Service Configuration set by by the client */
    gatt_fp_config_values_t client_config; 
}gatt_fp_data_t;

/*! @brief Initialise the GATT FP Server.

    \param task - Fast Pair State Machine task.

    \returns TRUE if the Fast Pair server task was initialized, FALSE otherwise.
*/
bool fastPair_GattFPServerInitialize(Task task);

/*! @brief Send Notifications to GATT Fast Pair Service library.

    \param      fastpair_id         GATT characteristic for which Notification has to be sent
                value               The payload
*/
void fastPair_SendFPNotification(uint16 fp_characteristic_uuid, uint8 *value);

/*! @brief Handle messages from the GATT Fast Pair Service library.

    \param      task    The task the message is delivered to
                id      The ID for the GATT message
                message The message payload
*/
void fastPair_GattFPServerMsgHandler(Task task, MessageId id, Message message);

#endif /*! GATT_FP_SERVER_H_ */

