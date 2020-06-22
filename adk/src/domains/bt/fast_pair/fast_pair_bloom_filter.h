/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file        fast_pair_bloom_filter.h
\brief      Handles Fast Pair Bloom Filter generation
*/

#ifndef FAST_PAIR_BLOOM_FILTER_H_
#define FAST_PAIR_BLOOM_FILTER_H_

#include <connection.h>
#include <connection_no_ble.h>
#include "domain_message.h"


#define SHA256_INPUT_ARRAY_LENGTH 17 /*Here 0-15 Bytes are used to store account key data and remaining 1 Byte for salt*/

/*! @brief Private API to initialise the bloom filter data structure

    This will be initialised under fastpair advertisement module as the bloom filter data goes into the adverts

    \param  none

    \returns none
 */
void fastPair_InitBloomFilter(void);


/*! @brief Function to generate bloom filter

      Account key filter or Bloom filter is used in adverts in unidentifiable or BR/EDR non-discoverable mode only.
      
      The following are the trigger to generate bloom filter
      - During fastpair advertising init, to be ready with the data
      - During addition of new account key
      - During Adv Mgr callback to get Item, this is to ensure new bloom filter is ready with a new Salt.
      - Deletion of account keys is taken care through the session data API
      - Internal timer expiry to track Salt change in Account key Filter  
 */
void fastPair_GenerateBloomFilter(void);


/*! @brief Private API to get the precalculated Bloom Filter data length

    This will go into the fastpair advertisement data

    \param  none

    \returns bloom filter data length
 */
uint8 fastPairGetBloomFilterLen(void);


/*! @brief Private API to get the precalculated Bloom Filter data

    This will go into the fastpair advertisement data

    \param  void

    \returns pointer to bloom filter data
 */
uint8* fastPairGetBloomFilterData(void);



/*! @brief Private API to handle CRYPTO_HASH_CFM for Bloom filter generation

    Called from Fast pair state manager to inform FP Adv module on hash confirmation

    \param  cfm     Confirmation from Crypto hash module while generating bloom filters

    \returns none
 */
void fastPair_AdvHandleHashCfm(CL_CRYPTO_HASH_CFM_T *cfm);


/*! @brief Private API to handle new account key addition

    Called from Fast pair state manager when a new account key is added on a successful fast pairing

    FP Adv module to generate bloom filters with the new data for adverts in BR/EDR non-discoverable mode

 */
void fastPair_AccountKeyAdded(void);


#endif /* FAST_PAIR_BLOOM_FILTER_H_ */
