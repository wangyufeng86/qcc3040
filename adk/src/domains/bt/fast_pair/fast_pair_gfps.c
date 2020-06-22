/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_gfps.c
\defgroup   fast_pair
\brief      This sub-module provides the GATT Server interface for Fast Pair under CAA

*/

/*! Firmware Headers */
#include <message.h>
#include <util.h>
#include <stdlib.h>
#include <byte_utils.h>
#include <logging.h>
#include <panic.h>

/*! Application Headers */
#include "fast_pair_events.h"
#include "fast_pair_gfps.h"
#include "gatt_handler.h"
#include "gatt_handler_db_if.h"

/*! Global Instance of GATT Server FP Data */
gatt_fp_data_t gatt_server_fp;

#define FAST_PAIR_GATT_SERVER gatt_server_fp

/***********************************************************************/
bool fastPair_GattFPServerInitialize(Task task)
{
    bool ret = FALSE;

#ifdef INCLUDE_FAST_PAIR
    if (GattFastPairServerInit(
                &FAST_PAIR_GATT_SERVER.gfps,
                task,
                HANDLE_FAST_PAIR_SERVICE,
                HANDLE_FAST_PAIR_SERVICE_END
                ))
    {
        DEBUG_LOG(" fastPair_GattServerInitialize. GATT Fast Pair Server initialised");
        ret = TRUE;
    }
    else
    {
        DEBUG_LOG("fastPair_GattServerInitialize. GATT Fast Pair Server initialisation failed");
    }
#else
    /*! In case, INCLUDE_FAST_PAIR is not defined, task remains unused and it throws build error */
    UNUSED(task);
#endif

    return ret;
}

/***********************************************************************/
void fastPair_SendFPNotification(uint16 fp_characteristic_uuid, uint8 *value)
{
    switch(fp_characteristic_uuid)
    {
        case FAST_PAIR_KEY_BASED_PAIRING:
            DEBUG_LOG("fastPair_SendFPNotification. FAST_PAIR_KEY_BASED_PAIRING");
            /*! Check to ensure if notifications are enabled */
            if(IS_KBP_NOTIFICATION_INDICATION_ENABLED(FAST_PAIR_GATT_SERVER))
            {
                /*! Send KbP Notification */
                GattFastPairServerKeybasedPairingNotification(&FAST_PAIR_GATT_SERVER.gfps, FAST_PAIR_GATT_SERVER.cid, value);
                DEBUG_LOG("fastPair_SendFPNotification. KbP notification sent successfully");
            }
            break;

        case FAST_PAIR_PASSKEY:
            DEBUG_LOG("fastPair_SendFPNotification. FAST_PAIR_PASSKEY");
            /*! Check to ensure if notifications are enabled */
            if(IS_PASSKEY_NOTIFICATION_INDICATION_ENABLED(FAST_PAIR_GATT_SERVER))
            {
                /*! Send Passkey Notification */
                GattFastPairServerPasskeyNotification(&FAST_PAIR_GATT_SERVER.gfps, FAST_PAIR_GATT_SERVER.cid, value);
                DEBUG_LOG("fastPair_SendFPNotification. Passkey notification sent successfully");
            }
            break;

        default:
            break;
    }

}

/***************************************************************************/
void fastPair_GattFPServerMsgHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    switch(id)
    {
        case GATT_FAST_PAIR_SERVER_WRITE_KEYBASED_PAIRING_IND:
        {
            GATT_FAST_PAIR_SERVER_WRITE_KEYBASED_PAIRING_IND_T *kbp_write =
                (GATT_FAST_PAIR_SERVER_WRITE_KEYBASED_PAIRING_IND_T *)message;

            FAST_PAIR_GATT_SERVER.cid = kbp_write->cid;

            /*! Check the data, if correct then write KbP if not, then send the failure response to library */
            if(kbp_write->size_value == (FAST_PAIR_ENCRYPTED_REQUEST_LEN+FAST_PAIR_PUBLIC_KEY_LEN)
                || kbp_write->size_value == FAST_PAIR_ENCRYPTED_REQUEST_LEN )
            {
                DEBUG_LOG("fastPair_GattFPServerMsgHandler. Write Key-Based Pairing");

                fastPair_KeyBasedPairingWrite(kbp_write->cid, kbp_write->value, kbp_write->size_value);

                GattFastPairServerWriteKeybasedPairingResponse(
                        kbp_write->fast_pair_server,
                        kbp_write->cid,
                        gatt_status_success
                        );
            }
            else
            {
                DEBUG_LOG("fastPair_GattFPServerMsgHandler. Invalid data for Key-Based Pairing");
                GattFastPairServerWriteKeybasedPairingResponse(
                        kbp_write->fast_pair_server,
                        kbp_write->cid,
                        gatt_status_invalid_cid
                        );
            }
        }
        break;

        case GATT_FAST_PAIR_SERVER_READ_KEYBASED_PAIRING_CONFIG_IND:
        {
            GATT_FAST_PAIR_SERVER_READ_KEYBASED_PAIRING_CONFIG_IND_T *kbp_config_read =
                (GATT_FAST_PAIR_SERVER_READ_KEYBASED_PAIRING_CONFIG_IND_T *)message;

            DEBUG_LOG("fastPair_GattFPServerMsgHandler. Read Key-Based Pairing Client Config Descriptor");

            GattFastPairServerReadKeybasedPairingConfigResponse(
                    kbp_config_read->fast_pair_server,
                    kbp_config_read->cid,
                    FAST_PAIR_GATT_SERVER.client_config.key_based_pairing
                    );
        }
        break;

        case GATT_FAST_PAIR_SERVER_WRITE_KEYBASED_PAIRING_CONFIG_IND:
        {
            GATT_FAST_PAIR_SERVER_WRITE_KEYBASED_PAIRING_CONFIG_IND_T *kbp_config_write =
                (GATT_FAST_PAIR_SERVER_WRITE_KEYBASED_PAIRING_CONFIG_IND_T *)message;

            DEBUG_LOG("fastPair_GattFPServerMsgHandler. Write Key-Based Pairing Client Config Descriptor 0x%04x",
                    (int)kbp_config_write->config_value
                    );

            /*! Only last notification or indication written by client will be used */
            FAST_PAIR_GATT_SERVER.client_config.key_based_pairing = (int)kbp_config_write->config_value;
            GattFastPairServerWriteKeybasedPairingConfigResponse(
                    kbp_config_write->fast_pair_server,
                    kbp_config_write->cid,
                    gatt_status_success
                    );
        }
        break;

        case GATT_FAST_PAIR_SERVER_WRITE_PASSKEY_IND:
        {
            GATT_FAST_PAIR_SERVER_WRITE_PASSKEY_IND_T *passkey_write =
                (GATT_FAST_PAIR_SERVER_WRITE_PASSKEY_IND_T *)message;

            /*! check the cid, if correct then write Passkey
                if not, then send the failure response to library */
            if(passkey_write->cid == FAST_PAIR_GATT_SERVER.cid)
            {
                DEBUG_LOG("fastPair_GattFPServerMsgHandler. Write Passkey");

                fastPair_PasskeyWrite(passkey_write->cid, passkey_write->value, FAST_PAIR_VALUE_SIZE);

                GattFastPairServerWritePasskeyResponse(
                        passkey_write->fast_pair_server,
                        passkey_write->cid,
                        gatt_status_success
                        );
            }
            else
            {
                DEBUG_LOG("fastPair_GattFPServerMsgHandler. Invalid CID");
                GattFastPairServerWritePasskeyResponse(
                        passkey_write->fast_pair_server,
                        passkey_write->cid,
                        gatt_status_invalid_cid
                        );
            }
        }
        break;

        case GATT_FAST_PAIR_SERVER_READ_PASSKEY_CONFIG_IND:
        {
            GATT_FAST_PAIR_SERVER_READ_PASSKEY_CONFIG_IND_T *passkey_config_read =
                (GATT_FAST_PAIR_SERVER_READ_PASSKEY_CONFIG_IND_T *)message;
            DEBUG_LOG("fastPair_GattFPServerMsgHandler. Read Passkey Client Config Descriptor" );

            GattFastPairServerReadPasskeyConfigResponse(
                    passkey_config_read->fast_pair_server,
                    passkey_config_read->cid,
                    FAST_PAIR_GATT_SERVER.client_config.passkey
                    );
        }
        break;

        case GATT_FAST_PAIR_SERVER_WRITE_PASSKEY_CONFIG_IND:
        {
            GATT_FAST_PAIR_SERVER_WRITE_PASSKEY_CONFIG_IND_T *passkey_config_write =
                (GATT_FAST_PAIR_SERVER_WRITE_PASSKEY_CONFIG_IND_T *)message;
            DEBUG_LOG(
                    "fastPair_GattFPServerMsgHandler. Write Passkey Client Config Descriptor 0x%04x",
                    (int)passkey_config_write->config_value
                    );

            /*! Only last notification or indication written by client will be used */
            FAST_PAIR_GATT_SERVER.client_config.passkey = (int)passkey_config_write->config_value;
            GattFastPairServerWritePasskeyConfigResponse(
                    passkey_config_write->fast_pair_server,
                    passkey_config_write->cid,
                    gatt_status_success
                    );
        }
        break;

        case GATT_FAST_PAIR_SERVER_WRITE_ACCOUNT_KEY_IND:
        {
            GATT_FAST_PAIR_SERVER_WRITE_ACCOUNT_KEY_IND_T *account_key_write =
                (GATT_FAST_PAIR_SERVER_WRITE_ACCOUNT_KEY_IND_T *)message;

            /*! check the cid, if it matches then write Account Key
                if not, then send the failure response to library */
            if(account_key_write->cid == FAST_PAIR_GATT_SERVER.cid)
            {
                DEBUG_LOG("fastPair_GattFPServerMsgHandler. Write Account Key");

                fastPair_AccountkeyWrite(account_key_write->cid, account_key_write->value, FAST_PAIR_VALUE_SIZE);

                GattFastPairServerWriteAccountKeyResponse(
                        account_key_write->fast_pair_server,
                        account_key_write->cid,
                        gatt_status_success
                        );
            }
            else
            {
                DEBUG_LOG("fastPair_GattFPServerMsgHandler. Invalid CID");
                GattFastPairServerWriteAccountKeyResponse(
                        account_key_write->fast_pair_server,
                        account_key_write->cid,
                        gatt_status_invalid_cid
                        );
            }
        }
        break;

        case GATT_FAST_PAIR_SERVER_KEYBASED_PAIRING_NOTIFICATION_CFM:
        {
            DEBUG_LOG(
                    "fastPair_GattFPServerMsgHandler. Key-based Pairing Notify Cfm"
                    );
        }
        break;

        case GATT_FAST_PAIR_SERVER_PASSKEY_NOTIFICATION_CFM:
        {
            DEBUG_LOG(
                    "fastPair_GattFPServerMsgHandler. Passkey Notify Cfm"
                    );
        }
        break;

        default:
        {
            /* Un-handled messages */
            DEBUG_LOG("fastPair_GattFPServerMsgHandler. GATT Fast Pair Unhandled msg id[%x]", id);
        }
        break;

    }
}
