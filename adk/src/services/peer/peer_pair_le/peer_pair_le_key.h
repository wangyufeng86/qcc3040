/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\ingroup    le_peer_pairing_service
\brief      Header file for the Peer pairing secret key.

The Peer pairing secret key is a key that must be the same on both devices in order to pair.
The secret key is held in peer_pair_le_key which is compiled into the application.
*/

#ifndef __PEER_PAIR_LE_KEY_H__
#define __PEER_PAIR_LE_KEY_H__


#include <gatt_root_key_service.h>

extern const GRKS_KEY_T peer_pair_le_key;


#endif  /* __PEER_PAIR_LE_KEY_H__ */
