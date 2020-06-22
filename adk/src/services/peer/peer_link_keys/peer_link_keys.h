/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_link_keys.h
\brief      Header file to send transfer peer link keys
*/
#ifndef PEER_LINK_KEYS_H
#define PEER_LINK_KEYS_H

#include <bdaddr.h>

/*! \brief Initialise the pairing application module.
 */
bool PeerLinkKeys_Init(Task init_task);

/*! \brief Send link keys to the peer device.

    Pass the link keys to the peer device

    \param[in] device_address  Address of the paired device
    \param[in] link_key_length  The length of link_key
    \param[in] link_key Pointer to the link key
 */
void PeerLinkKeys_SendKeyToPeer(const bdaddr* device_address, uint16 link_key_length, const uint16* link_key);

/*! \brief Send acknowledgement that key has been set to peer.

    Tell the peer device that the link key has been set.

    \param[in] device_address Address of the paired device
    \param[in] status TRUE if the link key was set successfully, FALSE otherwise
*/
void PeerLinkKeys_SendKeyResponseToPeer(const bdaddr *device_address, bool status);

/*! \brief Use link key for attached device to derive key for peer earbud.

    \param[in] device_address  Address of the paired device
    \param[in] lk_packed  Link for device bd_addr
    \param[in] key_id_in Key ID for peer link-key derivation 
    \param[out] lk_derived Pointer to location to store generate link key.
 */
void PeerLinkKeys_GenerateKey(const bdaddr *bd_addr, const uint16 *lk_packed, uint32 key_id_in, uint16 *lk_derived);

#endif /* PEER_LINK_KEYS_H */

