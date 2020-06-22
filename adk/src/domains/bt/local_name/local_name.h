/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       local_name.h
\brief      Header file for Bluetooth Local Name component

*/

#ifndef _DOMAINS_BT_LOCAL_NAME_
#define _DOMAINS_BT_LOCAL_NAME_

#include "domain_message.h"

/*! Messages sent by the Local Name module.  */
typedef enum
{
    /*! Message confirming that the Local Name module initialisation is complete */
    LOCAL_NAME_INIT_CFM = LOCAL_NAME_MESSAGE_BASE,
} local_name_message_t;

/*
    \brief Initialise Local Name module

    \param init_task
           Task to receive completion message

    \return TRUE to indicate successful initialisation,
            FALSE otherwise.
*/
bool LocalName_Init(Task init_task);

/*
    \brief Return the friendly name of the local Bluetooth device
*/
const uint8 *LocalName_GetName(uint16* name_len);

/*
    \brief Return the friendly name of the local Bluetooth device
           prefixed for use with Bluetooth Low Energy services
*/
const uint8 *LocalName_GetPrefixedName(uint16* name_len);

#endif /* _DOMAINS_BT_LOCAL_NAME_ */
