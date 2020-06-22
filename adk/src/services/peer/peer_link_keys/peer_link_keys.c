/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_link_keys.c
\brief      Implementation of functions to transfer link keys
*/

#include <panic.h>
#include <cryptovm.h>
#include <stdio.h>
#include <stdlib.h>

#include "peer_link_keys.h"
#include "peer_link_keys_typedef.h"
#include "peer_link_keys_marshal_typedef.h"
#include "peer_signalling.h"
#include "logging.h"
#include "message.h"
#include "pairing.h"
#include "pairing_config.h"

static void peerLinkKeys_HandleMessage(Task task, MessageId id, Message message);

static const TaskData peer_link_keys_task = {peerLinkKeys_HandleMessage};

/*! \brief Accessor for key sync task. */
#define peerLinkKeys_GetTask()       ((Task)&peer_link_keys_task)


/*! Number of bytes in the 128-bit salt. */
#define SALT_SIZE       16
/*! Offset into SALT for start of peer earbud address NAP */
#define OFFSET_SALT_NAP 10
/*! Offset into SALT for start of peer earbud address UAP */
#define OFFSET_SALT_UAP 12
/*! Offset into SALT for start of peer earbud address LAP */
#define OFFSET_SALT_LAP 13


/*! @brief Zero and write the peer earbud BT address into last 6 bytes of the SALT.

    The salt is 0x00000000000000000000xxxxxxxxxxxx
    Where the 6 bytes are LAP (3 bytes big-endian), UAP (1 byte), NAP (2 bytes big-endian)
*/
static void peerLinkKeys_SetupSaltWithPeerAddress(uint8 *salt, const bdaddr *peer_addr)
{
    memset(salt, 0, SALT_SIZE);
    salt[OFFSET_SALT_NAP]   = (peer_addr->nap >> 8) & 0xFF;
    salt[OFFSET_SALT_NAP+1] = peer_addr->nap & 0xFF;
    salt[OFFSET_SALT_UAP]   = peer_addr->uap & 0xFF;
    salt[OFFSET_SALT_LAP]   = (peer_addr->lap >> 16) & 0xFF;
    salt[OFFSET_SALT_LAP+1] = (peer_addr->lap >> 8) & 0xFF;
    salt[OFFSET_SALT_LAP+2] = peer_addr->lap & 0xFF;
}


bool PeerLinkKeys_Init(Task init_task)
{
    UNUSED(init_task);
    DEBUG_LOG("PeerLinkKeys_Init");

    /* Register the marshalled channel with peer signalling */
    appPeerSigMarshalledMsgChannelTaskRegister(peerLinkKeys_GetTask(),
                                               PEER_SIG_MSG_CHANNEL_PEER_LINK_KEY,
                                               peer_link_keys_marshal_type_descriptors,
                                               NUMBER_OF_PEER_LINK_KEYS_MARSHAL_TYPES);

    return TRUE;
}


void PeerLinkKeys_GenerateKey(const bdaddr *bd_addr, const uint16 *lk_packed, uint32 key_id_in, uint16 *lk_derived)
{
    uint8 salt[SALT_SIZE];
    uint8 lk_unpacked[AES_CMAC_BLOCK_SIZE];
    uint8 key_h7[AES_CMAC_BLOCK_SIZE];
    uint8 key_h6[AES_CMAC_BLOCK_SIZE];
    uint32_t key_id = ((key_id_in & 0xFF000000UL) >> 24) +
                      ((key_id_in & 0x00FF0000UL) >> 8) +
                      ((key_id_in & 0x0000FF00UL) << 8) +
                      ((key_id_in & 0x000000FFUL) << 24);

    DEBUG_PRINT("PeerLinkKeys_GenerateKey");

    DEBUG_LOG("Key Id: %08lx, reversed %08lx", key_id_in, key_id);

    DEBUG_PRINT("LK In: ");
    for (int lk_i = 0; lk_i < 8; lk_i++)
        DEBUG_PRINT("%02x ", lk_packed[lk_i]);
    DEBUG_PRINT("\n");

    peerLinkKeys_SetupSaltWithPeerAddress(salt, bd_addr);
    DEBUG_PRINT("Salt: ");
    for (int salt_i = 0; salt_i < SALT_SIZE; salt_i++)
        DEBUG_PRINT("%02x ", salt[salt_i]);
    DEBUG_PRINT("\n");

    /* Unpack the link key: 0xCCDD 0xAABB -> 0xAA 0xBB 0xCC 0xDD */
    for (int lk_i = 0; lk_i < (AES_CMAC_BLOCK_SIZE / 2); lk_i++)
    {
        lk_unpacked[(lk_i * 2) + 0] = (lk_packed[7 - lk_i] >> 8) & 0xFF;
        lk_unpacked[(lk_i * 2) + 1] = (lk_packed[7 - lk_i] >> 0) & 0xFF;
    }
    DEBUG_PRINT("LK In: ");
    for (int lk_i = 0; lk_i < AES_CMAC_BLOCK_SIZE; lk_i++)
        DEBUG_PRINT("%02x ", lk_unpacked[lk_i]);
    DEBUG_PRINT("\n");

    CryptoVmH7(lk_unpacked, salt, key_h7);
    DEBUG_PRINT("H7: ");
    for (int h7_i = 0; h7_i < AES_CMAC_BLOCK_SIZE; h7_i++)
        DEBUG_PRINT("%02x ", key_h7[h7_i]);
    DEBUG_PRINT("\n");

    CryptoVmH6(key_h7, key_id, key_h6);
    DEBUG_PRINT("H6: ");
    for (int h6_i = 0; h6_i < AES_CMAC_BLOCK_SIZE; h6_i++)
        DEBUG_PRINT("%02x ", key_h6[h6_i]);
    DEBUG_PRINT("\n");

    /* Pack the link key: 0xAA 0xBB 0xCC 0xDD -> 0xCCDD 0xAABB */
    for (int lk_i = 0; lk_i < (AES_CMAC_BLOCK_SIZE / 2); lk_i++)
    {
        lk_derived[7 - lk_i] = (key_h6[(lk_i * 2) + 0] << 8) |
                               (key_h6[(lk_i * 2) + 1] << 0);
    }

    DEBUG_PRINT("LK Out: ");
    for (int lk_i = 0; lk_i < 8; lk_i++)
        DEBUG_PRINT("%02x ", lk_derived[lk_i]);
    DEBUG_PRINT("\n");
}

/*! \brief Handle incoming peer_link_key messages from the peer earbud. */
static void peerLinkKeys_HandlePeerSignallingMessageRxInd(const PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *ind)
{
    DEBUG_LOG("peerLinkKeys_HandlePeerSignallingMessageRxInd type %d", ind->type);

    switch (ind->type)
    {
    case MARSHAL_TYPE(peer_link_keys_add_key_req_t):
        {
            const peer_link_keys_add_key_req_t *req = (const peer_link_keys_add_key_req_t *)ind->msg;

            DEBUG_LOG("peerLinkKeys_HandlePeerSignallingMessageRxInd add_key_req key_type %d", req->key_type);

            PanicFalse(PEER_LINK_KEYS_KEY_TYPE_0 == req->key_type);

            Pairing_AddAuthDevice(&req->addr, (
                                  req->key_size_bytes / sizeof(uint16)),
                                  (uint16 *)req->key);
        }
        break;

    case MARSHAL_TYPE(peer_link_keys_add_key_cfm_t):
        {
            const peer_link_keys_add_key_cfm_t *cfm = (const peer_link_keys_add_key_cfm_t *)ind->msg;

            DEBUG_LOG("peerLinkKeys_HandlePeerSignallingMessageRxInd add_key_cfm synced %d", cfm->synced);

            if (cfm->synced)
            {
                /* update device manager that we have successfully sent
                 * handset link key to peer headset, clear the flag */
                BtDevice_SetHandsetLinkKeyTxReqd(&cfm->addr, FALSE);
            }
            else
            {
                DEBUG_LOG("peerLinkKeys_HandlePeerSignallingMessageRxInd Failed to send handset link key to peer earbud!");
            }
        }
        break;

    default:
        DEBUG_LOG("peerLinkKeys_HandlePeerSignallingMessageRxInd unhandled");
        break;
    }

    /* free unmarshalled msg */
    free(ind->msg);
}

static void peerLinkKeys_HandlePeerSignallingMessageTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *cfm)
{
    UNUSED(cfm);
}

static void peerLinkKeys_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            peerLinkKeys_HandlePeerSignallingMessageRxInd((const PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *)message);
            break;

        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            peerLinkKeys_HandlePeerSignallingMessageTxCfm((const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *)message);
            break;

        default:
            break;
    }

}

void PeerLinkKeys_SendKeyToPeer(const bdaddr* device_address, uint16 link_key_length, const uint16* link_key)
{
    bdaddr peer_addr;

    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        uint16 lk_out[8] ={0};

        PeerLinkKeys_GenerateKey(&peer_addr, link_key, appConfigTwsKeyId(), lk_out);

        peer_link_keys_add_key_req_t *req = PanicUnlessMalloc(sizeof(*req));
        req->addr = *device_address;
        req->key_type = PEER_LINK_KEYS_KEY_TYPE_0;
        req->key_size_bytes = (link_key_length * sizeof(uint16));
        memmove(&req->key, lk_out, req->key_size_bytes);

        appPeerSigMarshalledMsgChannelTx(peerLinkKeys_GetTask(),
                                         PEER_SIG_MSG_CHANNEL_PEER_LINK_KEY,
                                         req, MARSHAL_TYPE_peer_link_keys_add_key_req_t);
    }
}

void PeerLinkKeys_SendKeyResponseToPeer(const bdaddr *device_address, bool synced)
{
    peer_link_keys_add_key_cfm_t *cfm = PanicUnlessMalloc(sizeof(*cfm));
    cfm->addr = *device_address;
    cfm->synced = synced;

    /* send confirmation key received and stored back to peer. */
    appPeerSigMarshalledMsgChannelTx(peerLinkKeys_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_PEER_LINK_KEY,
                                     cfm, MARSHAL_TYPE_peer_link_keys_add_key_cfm_t);
}
