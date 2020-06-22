/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   key_sync Key Sync
\ingroup    bt_domain
\brief      Key synchronisation component.
*/

#ifndef KEY_SYNC_H
#define KEY_SYNC_H

#include <domain_message.h>

#include <message.h>

/*\{*/

/*! \brief Message that may be sent by this component. */
enum key_sync_messages
{
    /* Key synchronisation is complete.
     * NOT IMPLEMENTED YET */
    KEY_SYNC_COMPLETE = KEY_SYNC_MESSAGE_BASE,
};

/*! \brief Initialise the key sync component. */
bool KeySync_Init(Task init_task);

/*! \brief Synchronise link keys with peer. */
void KeySync_Sync(void);

/*\}*/

#endif /* KEY_SYNC_H */
