/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       ama_rfcomm.h
\brief  File consists of function decalration for Amazon Voice Service's RFCOMM transport.
*/
#ifndef AMA_RFCOMM_H
#define AMA_RFCOMM_H

#include "ama.h"
#include "ama_protocol.h"
#include "bdaddr.h"


Task AmaRfcomm_GetTask(void);

/*\{*/
/*! Definition of messages that AMA TWS can send/rcv. */
typedef enum
{   /*! */
    AMA_RFCOMM_LOCAL_DISCONNECT_REQ_IND = 0,
    AMA_RFCOMM_LOCAL_ALLOW_CONNECTIONS_IND
    /*!
    AMA_RFCOMM_LOCAL_DISCONNECTED_IND
    */
} ama_rfcomm_local_message_t;

/*! \brief Initialize the AMA RFCOMM module.
*/
void AmaRfcomm_Init(void);

/*! \brief Send AMA protocol data to the handset using RFCOMM.

    \param data Pointer to uint8 data to send
    \param length Number of octets to send
    \return TRUE if the data was successfully sent, otherwise FALSE.
*/
bool AmaRfcomm_SendData(uint8* data, uint16 length);

#endif /* AMA_RFCOMM_H*/

