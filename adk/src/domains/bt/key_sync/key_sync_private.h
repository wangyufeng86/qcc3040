/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Key Sync component private header.
*/

#ifndef KEY_SYNC_PRIVATE_H
#define KEY_SYNC_PRIVATE_H

#include <message.h>

/*! \brief Key sync task data. */
typedef struct
{
    TaskData task;
} key_sync_task_data_t;

/*! \brief Component level visibility of the key sync task data. */
extern key_sync_task_data_t key_sync;

/*! \brief Accessor for key sync task data. */
#define keySync_GetTaskData()   (&key_sync)

/*! \brief Accessor for key sync task. */
#define keySync_GetTask()       (&key_sync.task)

#endif /* KEY_SYNC_PRIVATE_H */
