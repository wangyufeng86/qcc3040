/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       fast_pair_session_data.h
\brief      Fast pairing module device database access.
*/

#ifndef FAST_PAIR_SESSION_DATA_H_
#define FAST_PAIR_SESSION_DATA_H_
#include "fast_pair_account_key_sync.h"



/*! \brief Get the Fast Pair Model ID

    This interface can be used to get Google provided fast pair Model ID

    \param None

    \return uint32 Model ID
 */
uint32 fastPair_GetModelId(void);

/*! \brief Get the Fast Pair anti spoofing private key

    This interface can be used to get Google provided ASPK
    
    \param uint8* aspk allocated buffer to copy anti-spoofing private key into.

    \return None
 */
void fastPair_GetAntiSpoofingPrivateKey(uint8* aspk);

/*! \brief Get the Fast Pair account keys

    This interface can be used to get fast pair account keys
    It's callers responsibility to free the account_keys after intended use.
    \param uint8** address of a pointer which will point to account keys buffer 

    \return uint16 Number of account keys in the buffer
 */
uint16 fastPair_GetAccountKeys(uint8** account_keys);

/*! \brief Get the number of Fast Pair account keys

    This interface can be used to get number of fast pair account keys

    \param None 

    \return uint16 Number of account keys
 */
uint16 fastPair_GetNumAccountKeys(void);

/*! \brief Store the Fast Pair account key

    This interface can be used to store a new fast pair account key

    \param uint8* Reference to account key buffer 

    \return bool TRUE if account key is stored else FALSE
 */
bool fastPair_StoreAccountKey(const uint8* account_key);

/*! \brief Store the Fast Pair account keys with the index values
 *
    This inteface can be used to store the complete fast pair account key info

    \param fast_pair_account_key_sync_req_t* reference to the fast pair account key info

    \return bool TRUE if account keys are stored else FALSE
 */
bool fastPair_StoreAllAccountKeys(fast_pair_account_key_sync_req_t* account_key_info);

/*! \brief Delete the Fast Pair account keys

    This interface can be used to delete all fast pair account keys in case of Factory Reset

    \param None 

    \return bool TRUE if all account keys are deleted else FALSE
 */
bool fastPair_DeleteAllAccountKeys(void);



#endif /* FAST_PAIR_SESSION_DATA_H_ */
