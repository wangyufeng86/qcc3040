/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       message_broker.c
\brief      The Message Broker allows client Application Modules to register interest
            in System Messages by Message Group. The Message Broker sniffs Messages
            sent in the system, and if they belong to the interested Group, forwards
            a copy to the interested client component.
*/

#include "message_broker.h"

#include <panic.h>
#include <vmtypes.h>

#ifndef MESSAGE_BROKER_DEBUG_LIB
#define DISABLE_LOG
#endif
#include <logging.h>

/*! Message broker internal state */
typedef struct
{
    /*! Pointer to an array of registration structures. */
    const message_broker_group_registration_t *registrations;

    /*! The number of registrations in the array. */
    unsigned registrations_len;
} message_broker_state_t;

static message_broker_state_t message_broker_state;


void MessageBroker_Init(const message_broker_group_registration_t *registrations,
                        unsigned registrations_len)
{
    message_broker_state.registrations = registrations;
    message_broker_state.registrations_len = registrations_len;
}

void MessageBroker_RegisterInterestInMsgGroups(Task task, const message_group_t *msg_groups, unsigned num_groups)
{
    message_group_t msg_group_index;
    const message_broker_group_registration_t *registration;

    if (task == NULL || msg_groups == NULL)
    {
        Panic();
    }

    for (msg_group_index = 0; msg_group_index < num_groups; msg_group_index++)
    {
        message_group_t group = msg_groups[msg_group_index];
        bool registered = FALSE;

        for (registration = message_broker_state.registrations;
             registration < (message_broker_state.registrations + message_broker_state.registrations_len);
             registration++)
        {
            if (group == registration->message_group)
            {
                registration->MessageGroupRegister(task, group);
                registered = TRUE;

            }
        }

        if (!registered)
        {
            DEBUG_LOG("MessageBroker_RegisterInterestInMsgGroups, failed to register for group=%d", group);
            Panic();
        }
        else
        {
            DEBUG_LOG("MessageBroker_RegisterInterestInMsgGroups: group = %d", group);
        }
    }
}
