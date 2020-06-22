/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   connection_message_dispatcher Connection Message Dispatcher
\ingroup    bt_domain
\brief	    A component that dispatches the connection library messages to interested clients.

The connection library sends many messages to single task (the task passed into ConnectionInit).
The purpose of Connection Message Dispatcher is to redirect those messages to interested clients.

Instead of resending messages the client's task handler is called directly.

To limit number of client, clients register a task for a message group,
not an individual message.

The mapping of the messages to groups in hard-coded in this component.

*/

#ifndef CONNECTION_MESSAGE_DISPATCHER_H_
#define CONNECTION_MESSAGE_DISPATCHER_H_

#include <message.h>

/*\{*/

/*! \brief Initialise message dispatcher.
*/
void ConnectionMessageDispatcher_Init(void);

/*! \brief Gets message dispatcher handler.
    \return Handler.
*/
Task ConnectionMessageDispatcher_GetHandler(void);

/*! \brief Register a client task for inquiry connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterInquiryClient(Task task);

/*! \brief Register a client task for acl connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterAclClient(Task task);

/*! \brief Register a client task for init connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterInitClient(Task task);

/*! \brief Register a client task for crypto connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterCryptoClient(Task task);

/*! \brief Register a client task for csb connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterCsbClient(Task task);

/*! \brief Register a client task for le connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterLeClient(Task task);

/*! \brief Register a client task for tdl connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterTdlClient(Task task);

/*! \brief Register a client task for l2cap connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterL2capClient(Task task);

/*! \brief Register a client task for local device connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterLocalDeviceClient(Task task);

/*! \brief Register a client task for pairing connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterPairingClient(Task task);

/*! \brief Register a client task for link policy connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterLinkPolicyClient(Task task);

/*! \brief Register a client task for test connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterTestClient(Task task);

/*! \brief Register a client task for remote connection connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterRemoteConnectionClient(Task task);

/*! \brief Register a client task for RFCOMM connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterRfcommClient(Task task);

/*! \brief Register a client task for SCO connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterScoClient(Task task);

/*! \brief Register a client task for SDP connection messages.
    \param  task The client task.
    \return The current registered task, or NULL if no client was previously registered.
*/
Task ConnectionMessageDispatcher_RegisterSdpClient(Task task);


/*\}*/

#endif /* CONNECTION_MESSAGE_DISPATCHER_H_ */
