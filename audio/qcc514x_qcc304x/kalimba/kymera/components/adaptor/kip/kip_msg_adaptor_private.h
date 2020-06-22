/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup kip adaptor
 * \ingroup adaptor
 *
 * \file kip_msg_adaptor_private.h
 * \ingroup kip
 *
 * This file contains data types and API private to the kip adaptor component.
 *
 */

#ifndef _KIP_MSG_ADAPTOR_PRIVATE_H_
#define _KIP_MSG_ADAPTOR_PRIVATE_H_

#include "kip_msg_adaptor.h"

#define KIP_CONID_SEND_CLIENT_MASK    0xF800
#define KIP_CONID_RECV_CLIENT_MASK    0x00F8

static inline bool kip_msg_is_request(KIP_MSG_ID msg_id)
{
    return KIP_MSG_ID_IS_REQ(msg_id);
}

static inline KIP_MSG_ID kip_msg_to_request(KIP_MSG_ID msg_id)
{
    uint16 id;
    id = msg_id & ~KIP_MSG_ID_RESP_MASK;
    return (KIP_MSG_ID) id;
}

extern void kip_adaptor_init(void);

/**
 * \brief    Register KIP REQ information to be used when KIP RESP comes back.
 *
 * \param    sender_id  The sender part of connection id in the request
 * \param    msg_id     The request message id
 * \param    seq_nr     The sequence number
 * \param    context    Context ( it can be a callback or data )
 */
extern bool kip_record_in_progress_req(CONNECTION_LINK con_id,
                                       KIP_MSG_ID msg_id,
                                       uint16 seq_nr,
                                       void *context);

/**
 * \brief    Retrieve the request information when the response comes back.
 *           We may only pass the receiver part of con_id,to be then matched up with
 *           receiver part of RESP con_id along with the msg_id
 *
 * \param    sender_id  The sender part of connection id in the request
 * \param    msg_id     The request message id
 * \param    *seq_nr    The sequence number to return
 *
 * \return void*       Context
 */

extern void* kip_retrieve_in_progress_req(CONNECTION_LINK con_id,
                                          KIP_MSG_ID msg_id,
                                          uint16* seq_nr);

#endif /* _KIP_MSG_ADAPTOR_PRIVATE_H_ */

