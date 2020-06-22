/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file        fast_pair_account_key_sync.h
\brief      Header file of FP account key sync component
*/
#ifndef FAST_PAIR_ACCOUNT_KEY_SYNC_H
#define FAST_PAIR_ACCOUNT_KEY_SYNC_H

#include <marshal_common.h>
#include <marshal.h>
#include <message.h>
#include <task_list.h>
#include "fast_pair.h"

/*! Complete Account Keys Data Length in words(uint16) */
#define ACCOUNT_KEY_DATA_LENTH        (40)

/*! Account key sync task data. */
typedef struct
{
    TaskData task;
} fp_account_key_sync_task_data_t;

/*! Component level visibility of Account Key Sync Task Data */
extern fp_account_key_sync_task_data_t account_key_sync;

#define fpAccountKeySync_GetTaskData() (&account_key_sync)
#define fpAccountKeySync_GetTask() (&account_key_sync.task)

typedef struct fast_pair_account_key_sync_req
{
    uint16 account_key_index[MAX_FAST_PAIR_ACCOUNT_KEYS];
    uint16 account_keys[ACCOUNT_KEY_DATA_LENTH];
} fast_pair_account_key_sync_req_t;

typedef struct fast_pair_account_key_sync_cfm
{
    bool synced;
} fast_pair_account_key_sync_cfm_t;

/*! Create base list of marshal types the account key sync will use. */
#define MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(fast_pair_account_key_sync_req_t) \
    ENTRY(fast_pair_account_key_sync_cfm_t)

/*! X-Macro generate enumeration of all marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),

enum MARSHAL_TYPES
{
    /*! common types must be placed at the start of the enum */
    DUMMY = NUMBER_OF_COMMON_MARSHAL_OBJECT_TYPES-1,
    /*! now expand the marshal types specific to this component. */
    MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    NUMBER_OF_MARSHAL_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION

/*! Make the array of all message marshal descriptors available. */
extern const marshal_type_descriptor_t * const fp_account_key_sync_marshal_type_descriptors[];

/*! \brief Fast Pair Account Key Sync Initialization
    This is used to initialize fast pair account key sync interface.
 */
void fastPair_AccountKeySync_Init(void);

/*! \brief Fast Pair Account Key Synchronization API
    This is the API which triggers the synchronization between the peers.
 */
void fastPair_AccountKeySync_Sync(void);

#endif /*! FAST_PAIR_ACCOUNT_KEY_SYNC_H */
