/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file        fast_pair_bloom_filter.c
\brief      Handles Fast Pair Bloom Filter generation
*/

/*! Firmware and Library Headers */
#include <util.h>
#include <stdlib.h>
#include <message.h>
#include <logging.h>
#include <panic.h>
#include <stdio.h>
#include <connection_manager.h>
#include <string.h>
#include <cryptovm.h>

/*! Application Headers */
#include "fast_pair_bloom_filter.h"
#include "fast_pair_session_data.h"
#include "fast_pair.h"

/*! \brief Global data structure for fastpair bloom filter generation */
typedef struct
{
    uint8 bloom_filter_len;
    uint8 bloom_filter_ready_len;
    bool sha_hash_lock;
    uint8 *bloom_filter_generating;/*Buffer used to hold bloom filter while it is getting generated */
    uint8* bloom_filter_ready;/*Buffer used to hold ready bloom filter data*/
    uint16 num_account_keys;
    uint8 *account_keys;/*allocated everytime fastPair_GetAccountKeys() is called*/

    uint8 filter_size;/*Value S*/
    uint8 number_of_keys_processed;
    uint8 temp[SHA256_INPUT_ARRAY_LENGTH];

}fastpair_bloom_filter_data_t;

/*! Global Instance of fastpair bloom filter data */
fastpair_bloom_filter_data_t fastpair_bloom_filter;

/*! Module Constants */
#define NO_BLOOM_FILTER_LEN (0)
#define ACCOUNT_KEY_LEN (16)

/*! \brief Function to send SHA 256 hash data request to help with generation of bloom filter
*/
static void fastPair_HashData(uint8 input_array[SHA256_INPUT_ARRAY_LENGTH])
{
    fastPairTaskData *theFastPair;
    theFastPair = fastPair_GetTaskData();

    ConnectionEncryptBlockSha256(&theFastPair->task, (uint16 *)input_array, (SHA256_INPUT_ARRAY_LENGTH));
}


/*! @brief Helper function to generate bloom filter

      Calculates the key data length
 */
static uint8 fastPair_CalculateAccountKeyData(uint8 *acc_keys, uint8 *bloom_filter)
{
    uint8 size=0;
    uint8 num_keys = fastpair_bloom_filter.num_account_keys;

    DEBUG_LOG("fastPair_CalculateAccountKeyData");

    if (num_keys)
    {        
        fastpair_bloom_filter.filter_size = num_keys * 6 / 5 + 3;/* Size of bloom filter */
        size = fastpair_bloom_filter.filter_size + 4;/*Including Flag & Salt field*/

        if ((bloom_filter) && (acc_keys))
        {         
            /* 1 Byte Salt */
            uint8 salt = UtilRandom() & 0xFF;

            /*Reset number of account keys processed before starting the hash*/
            fastpair_bloom_filter.number_of_keys_processed = 0;

            memset(bloom_filter, 0, size);
            bloom_filter[0] = 0x00; /* Flags for furture use */
            bloom_filter[1] = (fastpair_bloom_filter.filter_size << 4) & 0xF0;
            bloom_filter[size - 2] = 0x11;
            bloom_filter[size - 1] = salt;

            fastpair_bloom_filter.temp[ACCOUNT_KEY_LEN] = salt;
            
            memmove(fastpair_bloom_filter.temp, acc_keys, ACCOUNT_KEY_LEN);
            fastpair_bloom_filter.sha_hash_lock = TRUE;
            fastPair_HashData(fastpair_bloom_filter.temp);
            
            fastpair_bloom_filter.number_of_keys_processed++;
        }
    }

    return size;
}

/*! @brief Helper function to release account key memory
 */
static void fastPairReleaseAccountKeys(void)
{
    if (fastpair_bloom_filter.account_keys)
    {
        DEBUG_LOG("fastPairReleaseAccountKeys\n");
        free(fastpair_bloom_filter.account_keys);
        fastpair_bloom_filter.account_keys=NULL;
        fastpair_bloom_filter.num_account_keys=0;
    }
}

/*! @brief Helper function to release bloom filter memory
 */
static void fastPairReleaseBloomFilter(void)
{
    if (fastpair_bloom_filter.bloom_filter_generating)
    {
        DEBUG_LOG("fastPairReleaseBloomFilter\n");
        free(fastpair_bloom_filter.bloom_filter_generating);
        fastpair_bloom_filter.bloom_filter_generating=NULL;
        fastpair_bloom_filter.bloom_filter_len=NO_BLOOM_FILTER_LEN;
    }
}


/*! \brief Function to initialise the fastpair bloom filter generation globals
*/
void fastPair_InitBloomFilter(void)
{
    memset(&fastpair_bloom_filter, 0, sizeof(fastpair_bloom_filter_data_t));
}

/*! @brief Function to generate bloom filter

      Account key filter or Bloom filter is used in adverts in unidentifable or BR/EDR non-discoverable mode only.
      
      The following are the trigger to generate bloom filter
      - During fastpair advertising init, to be ready with the data
      - During addition of new account key
      - During Adv Mgr callback to get Item, this is to ensure new bloom filter is ready with a new Salt.
      - Deletion of account keys is taken care through the session data API
      - Internal timer expiry to track Salt change in Account key Filter  
 */
void fastPair_GenerateBloomFilter(void)
{
    DEBUG_LOG("FP Bloom Filter: fastPair_GenerateBloomFilter: sha_hash_lock(%d)", fastpair_bloom_filter.sha_hash_lock);
    if(!fastpair_bloom_filter.sha_hash_lock)
    {
        DEBUG_LOG("fastPair_GenerateBloomFilter:");
        /*Housekeeping*/
        fastPairReleaseAccountKeys();
        fastPairReleaseBloomFilter();

        /*Start with checking the num account keys*/
        fastpair_bloom_filter.num_account_keys = fastPair_GetAccountKeys(&fastpair_bloom_filter.account_keys);
        
        if (fastpair_bloom_filter.num_account_keys)
        {

            fastpair_bloom_filter.bloom_filter_len = fastPair_CalculateAccountKeyData(fastpair_bloom_filter.account_keys, NULL);
            fastpair_bloom_filter.bloom_filter_generating = PanicUnlessMalloc(fastpair_bloom_filter.bloom_filter_len);

            if (fastpair_bloom_filter.bloom_filter_len)
            {
               fastPair_CalculateAccountKeyData(fastpair_bloom_filter.account_keys, fastpair_bloom_filter.bloom_filter_generating);
            }
        }
    }
}


/*! @brief Private API to return the generated bloom filter length
 */
uint8 fastPairGetBloomFilterLen(void)
{
    DEBUG_LOG("FP Bloom Filter Len: %d \n", fastpair_bloom_filter.bloom_filter_ready_len);
    return fastpair_bloom_filter.bloom_filter_ready_len;
}

/*! @brief Private API to return the generated bloom filter 
 */
uint8* fastPairGetBloomFilterData(void)
{
    return fastpair_bloom_filter.bloom_filter_ready;
}


/*! @brief Private API to handle CRYPTO_HASH_CFM for Bloom filter generation
 */
void fastPair_AdvHandleHashCfm(CL_CRYPTO_HASH_CFM_T *cfm)
{
    uint32 hash_value;
    uint32 hash_account_key;
    uint8 index;

    /* Bloom filter byte ordering is as below 
    | 0x00 |0b LLLL   TTTT   |0x--    0x--| ob LLLL   TTTT  |  0x   |
    |  FFU |   LACF   TYPEA  |   VLACF... |    LSF    TYPES.|..SALT |

    FFU --> For future use. Should be 0x00
    LACF --> Length of account key filter in bytes. Should be 0x00 if account key filter is empty
    TYPEA --> Type of account key filter. Should be 0x0
    VLACF --> Variable length account key filter
    LSF --> Length of salt filed in bytes i.e. 0x1
    TYPES --> Type of salt.Should be 0x1 if salt is used
    SALT --> Salt data */

    /* Allocated bloom filter buffer holds data from fields LACF to SALT.
    Generated bloom filter to be copied from field VLACF. Assign VLCAF index i.e. 1 to temporary place holder */

    if ((fastpair_bloom_filter.bloom_filter_generating) && (cfm->status==success))
    {
        uint8* key_filter_ptr = &fastpair_bloom_filter.bloom_filter_generating[2];
        for (index = 0; index < ACCOUNT_KEY_LEN; index += 2)
        {
            hash_value  = ((uint32) cfm->hash[index] & 0x00FF) << 24;
            hash_value |= ((uint32) cfm->hash[index] & 0xFF00) << 8;
            hash_value |= ((uint32) cfm->hash[index + 1] & 0x00FF) << 8;
            hash_value |= ((uint32) cfm->hash[index + 1] & 0xFF00) >> 8;

            hash_account_key = hash_value % (fastpair_bloom_filter.filter_size * 8);
            key_filter_ptr[hash_account_key/ 8] |= (1 << (hash_account_key % 8));
        }
        
         if (fastpair_bloom_filter.number_of_keys_processed == fastpair_bloom_filter.num_account_keys)
         {
            /*Account Key Filter generated successfully*/
            DEBUG_LOG("FP Bloom Filter Data: fastPair_AdvHandleHashCfm success \n");
            if(!fastpair_bloom_filter.bloom_filter_ready)
            {
                fastpair_bloom_filter.bloom_filter_ready = PanicUnlessMalloc(fastpair_bloom_filter.bloom_filter_len);
            }
            else
            {
                uint8* buf = fastpair_bloom_filter.bloom_filter_ready;
                fastpair_bloom_filter.bloom_filter_ready = PanicNull(realloc(buf, fastpair_bloom_filter.bloom_filter_len));
            }
            
            memcpy(fastpair_bloom_filter.bloom_filter_ready, fastpair_bloom_filter.bloom_filter_generating, fastpair_bloom_filter.bloom_filter_len);
            fastpair_bloom_filter.bloom_filter_ready_len = fastpair_bloom_filter.bloom_filter_len;
            fastPairReleaseAccountKeys();
            fastpair_bloom_filter.sha_hash_lock = FALSE;
         }  
          
         if (fastpair_bloom_filter.number_of_keys_processed < fastpair_bloom_filter.num_account_keys)
         {          
             memmove(fastpair_bloom_filter.temp, &fastpair_bloom_filter.account_keys[fastpair_bloom_filter.number_of_keys_processed*ACCOUNT_KEY_LEN], ACCOUNT_KEY_LEN);
             fastPair_HashData(fastpair_bloom_filter.temp);
             fastpair_bloom_filter.number_of_keys_processed++;
         }
    }
    else
    {
        DEBUG_LOG("FP Bloom Filter Data: fastPair_AdvHandleHashCfm failure\n");
        fastPairReleaseAccountKeys();
        fastPairReleaseBloomFilter();
        fastpair_bloom_filter.sha_hash_lock = FALSE;
    }
}


/*! @brief Private API to handle new account key addition
 */
void fastPair_AccountKeyAdded(void)
{
    /*Generate new bloom filter and keep it ready for advertisements in BR/EDR Connectable and non-discoverable mode*/
    DEBUG_LOG("fastPair_AccountKeyAdded: fastPair_GenerateBloomFilter\n");
    fastPair_GenerateBloomFilter();
}


