/****************************************************************************
 * Copyright (c) 2013 - 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file kip_msg_adaptor.h
 * \ingroup adaptor
 *
 * Public definitions of functions to send messages over the
 * Kymera Inter Processor interface which allow communication between
 * processor cores in the audio subsystem.
 */

#ifndef KIP_MSG_ADAPTOR_H
#define KIP_MSG_ADAPTOR_H

#include "types.h"
#include "proc/proc.h"
#include "adaptor/connection_id.h"
#include "kip_msg_prim.h"

/****************************************************************************
Public Functions
*/

/**
 * \brief Send a message over the Kymera Inter Processor interface.
 *
 * \param conn_id  As defined in connection_id.h with processor field
 * \param msg_id   Identify the type of message transmitted
 * \param length   Number of words of msg_data
 * \param msg_data Pointer to the message data
 * \param context  Pointer that will be passed to the function in
 *                 charge of handling the response.
 *
 * \return True in case of success
 *
 * \note It is recommended to use adaptor_send_message instead of
 *       kip_adaptor_send_message when context is NULL.
 */
extern bool kip_adaptor_send_message(CONNECTION_LINK conn_id,
                                     KIP_MSG_ID msg_id,
                                     unsigned length,
                                     uint16 *msg_data,
                                     void* context);

#if defined(SUPPORTS_MULTI_CORE)
/**
 * \brief  Helper function for dispatching licensing queries to P0.
 *
 * \param  length    Length of buffer that 'payload' points to (in words).
 *         payload   Pointer to license query contents
 *
 * \return TRUE if message sent successfully (P1->P0), FALSE if not
 */
bool kip_send_msg_lic_mgr_query(unsigned length, unsigned* payload);

bool kip_reply_if_kip_request_pending(unsigned length, unsigned* payload);
void kip_configure_pending_query(void);
#else
#define kip_send_msg_lic_mgr_query(x, y) FALSE
#endif

#endif /* KIP_MSG_ADAPTOR_H */
