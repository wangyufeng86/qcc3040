/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   unexpected_message Unexpected message handler
\brief      Unexpected message handler callbacks.
 
This component allows an application to register unexpected message handlers.
Those handlers are called from within adk code for debugging purposes.
 
*/

#ifndef UNEXPECTED_MESSAGE_H_
#define UNEXPECTED_MESSAGE_H_

#include <message.h>

/*@{*/

/*! \brief Register unexpected message handler.

    \param handler Function pointer to the message handler.
*/
void UnexpectedMessage_RegisterHandler(void (*handler)(MessageId id));

/*! \brief Register unexpected system message handler.

    \param handler Function pointer to the system message handler.
*/
void UnexpectedMessage_RegisterSysHandler(void (*handler)(MessageId id));

/*! \brief Call registered message handler.

    \param id Message id.
*/
void UnexpectedMessage_HandleMessage(MessageId id);

/*! \brief Call registered system message handler.

    \param id System message id.
*/
void UnexpectedMessage_HandleSysMessage(MessageId id);

/*@}*/

#endif /* UNEXPECTED_MESSAGE_H_ */
