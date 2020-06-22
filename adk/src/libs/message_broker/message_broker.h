/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       message_broker.h
\brief      Interface to the Message Broker.

            The Message Broker allows client modules to register interest
            in System Messages by Message Group. The Message Broker registers
            with the message group owner on behalf of the client module. The
            message group owner subsequently messages the client module directly.
            The message group owner's messge group ID and registration function
            is provides to message broker in MessageBroker_init().

            An example message sequence chart for providing an LED indication:

            @startuml
            skinparam roundcorner 20
            hide footbox
            participant "App Init" as init
            participant "Message Broker" as mb
            participant "<<UI_Provider>>\nTelephony" as tel #Thistle
            participant "TaskList" as tl
            participant "Message Loop" as ml
            participant "<<UI_Indicator>>\nLEDs" as led #LightBlue
            group Initialisation
                init -> mb: MessageBroker_Init(registrations[])
                ...
                led -> mb: MessageBroker_RegisterInterestInMsgGroups(led_task, led_grps, num_grps)
                mb  -> tel: message_group_register(led_task)
                note over mb
                    message_group_register is part of
                    message broker init registrations structure
                    (one structure per message group)
                end note
                ...
            end group
            ...
            tel -> tl: TaskList_SendMessage(led_task, "Telephony Connected")
            tl -> ml: MessageSend(led_task, "Telephony Connected")
            ml -> led: "Telephony Connected"
            note over led
                Flash LED to indicate connection
            end note
            @enduml

*/
#ifndef MESSAGE_BROKER_H
#define MESSAGE_BROKER_H

#include <csrtypes.h>

/*! A type for message groups */
typedef uint16 message_group_t;

/*! \brief Each message group in the system shall create an instance of
    this structure defining functions to allow the message broker to register /
    unregister for messages in that group.
*/
typedef struct
{
    /*! The message group associated with the register/unregister functions */
    message_group_t message_group;

    /*! A function that will register a task for messages. This pointer
    must not be NULL. */
    void (*MessageGroupRegister)(Task, message_group_t);

    /*! A function that will unregister a task for messages. This pointer
    may be NULL. */
    void (*MessageGroupUnregister)(Task, message_group_t);

} message_broker_group_registration_t;

/*! \brief MessageBroker_Init
    \param registrations Pointer to an array of registration structures.
    \param registrations_len The number of registrations in the array.
*/
void MessageBroker_Init(const message_broker_group_registration_t *registrations,
                        unsigned registrations_len);

/*! \brief MessageBroker_RegisterInterestInMsgGroups
    \param task the client's message handler, i.e. where the message broker shall send the interested messages to
    \param msg_groups pointer to the array of message groups the client is registering interest in
    \param num_groups the number of message groups the client is registering interest in
*/
void MessageBroker_RegisterInterestInMsgGroups(Task task, const message_group_t* msg_groups, unsigned num_groups);

#endif /* MESSAGE_BROKER_H */

