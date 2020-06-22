/****************************************************************************
 * Copyright (c) 2013 - 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup adaptor Definition of the Connection ID and macros to access its
 * internal information.
 *
 * \file connection_id.h
 * \ingroup adaptor
 *
 */

#ifndef CONNECTION_ID_H
#define CONNECTION_ID_H

#include "types.h"
#include "proc/proc.h"

/****************************************************************************
Public Constant Declarations
*/

/****************************************************************************
Public Type Declarations
*/
typedef uint16f CONNECTION_LINK;
typedef uint8   CONNECTION_PEER;

/****************************************************************************
Public Macro Definitions
*/

/* Connection id contains sender and receiver 8-bit ids as follows:
 *
 *  | 15 - 8 |  7 - 0  |
 *  --------------------
 *  | RCV ID | SEND ID |
 *
 *  These values are swapped in response messages.
 *
 *  When a command is first received from the client, only the SEND ID carries
 *  a value. The RCV ID was kept unused until the advent of Multicore Kymera.
 *  Currently the RCV ID field is used different purposes: in particular for
 *  the aggregate id, in commands sent to multiple operators, and for the
 *  processor id, in commands sent to secondary cores.
 *  We should be careful not to use the connection id as synonymous of
 *  the SEND ID, but extract this information from it (maybe stronger typing
 *  would help).
 */

/* Constants for accessing sender and receiver fields from connection ID.
 * The following are meant for private use to define public macros.
 */

/** The number of bits to shift the sender/receiver id in the connection id field */
#define RECEIVER_SENDER_ID_SHIFT    8
/** Mask for extracting the receiver id */
#define RECEIVER_ID_MASK            0xFF00
/** Mask for extracting the sender id */
#define SENDER_ID_MASK              0x00FF

/** Mask off upper 8 bits of routing information */
#define ROUTING_VALUE_MASK          SENDER_ID_MASK

/* Public MACROs to access and use the information available in the
 * connection id.
 */

/**
 * \brief Used for constructing the connection id from routing info
 *
 * \param send_id Carries information about the sender.
 * \param recv_id Carries information about the receiver.
 *
 * \return formatted connection id.
 */
static inline CONNECTION_LINK adaptor_pack_con_id(CONNECTION_PEER send_id,
                                                  CONNECTION_PEER recv_id)
{
    uint16 result;
    
    result = (uint16) recv_id;
    result &= ROUTING_VALUE_MASK;
    result <<= RECEIVER_SENDER_ID_SHIFT;

    result |= send_id & ROUTING_VALUE_MASK;

    return (CONNECTION_LINK) result;
}
#define PACK_CON_ID(send_id, recv_id) adaptor_pack_con_id(send_id, recv_id)

/**
 * \brief Used for extracting the sender id from a connection id.
 *
 * \param con_id Connection id.
 *
 * \return Sender id: information about the sender.
 */
static inline CONNECTION_PEER adaptor_get_send_id(CONNECTION_LINK con_id)
{
    return (CONNECTION_PEER) con_id & SENDER_ID_MASK;
}
#define GET_CON_ID_SEND_ID(con_id) adaptor_get_send_id(con_id)

/**
 * \brief Used for extracting the receiver id from a connection id.
 *
 * \param con_id Connection id.
 *
 * \return Receiver id: information about the receiver.
 */
static inline CONNECTION_PEER adaptor_get_recv_id(CONNECTION_LINK con_id)
{
    uint16 result;

    result = con_id & RECEIVER_ID_MASK;
    result >>= RECEIVER_SENDER_ID_SHIFT;

    return (CONNECTION_PEER) result;
}
#define GET_CON_ID_RECV_ID(con_id) adaptor_get_recv_id(con_id)

/**
 * \brief Used for extracting the client information (sender id) from a
 *        NOT reversed connection id.
 *
 * \param con_id Not-Reversed Connection id.
 *
 * \return Client Info: information about the client (sender).
 */
static inline CONNECTION_PEER adaptor_get_client_info(CONNECTION_LINK con_id)
{
    uint16 result;

    result = con_id & ROUTING_VALUE_MASK;

    return (CONNECTION_PEER) result;
}
#define GET_CON_ID_CLIENT_INFO(con_id) adaptor_get_client_info(con_id)

/**
 * \brief Reverse connection ID - Swap values of sender and reeiver.
 *
 * \param con_id Connection id.
 *
 * \return Reversed connection id.
 */
static inline CONNECTION_LINK adaptor_reverse_connection(CONNECTION_LINK con_id)
{
    uint16 result;

    result = (uint16) adaptor_get_send_id(con_id);
    result <<= RECEIVER_SENDER_ID_SHIFT;

    result |= adaptor_get_recv_id(con_id);

    return (CONNECTION_LINK) result;
}
#define REVERSE_CONNECTION_ID(con_id) adaptor_reverse_connection(con_id)

/* Value representing and invalid connection. N.B. This is a theoretically
 * valid value although impossible. */
#define INVALID_CON_ID  (CONNECTION_LINK)(SENDER_ID_MASK | RECEIVER_ID_MASK)

/*
 * Sender and receiver id contain processor and client id as follows:
 *
 *  |  7 - 5       |  4    |  3 - 0       |
 *  ---------------------------------------
 *  |  PROC ID     |Special| CLIENT INDEX |
 *  ---------------------------------------
 *                 |    CLIENT ID         |
 *  ---------------------------------------
 * Processor ID is top 3 bits of sneder or receiver id.
 * Client id is last 5 bits, where:
 * bit 4 distinguishes between an application client (i.e. ACCMD service instance,
 * value 0), and a special client (i.e. OBPM or internal operator client, value 1)
 * bottom 3 bits identify the client by a unique index.
 */

#define INVALID_CLIENT_ID (CONNECTION_PEER) ROUTING_VALUE_MASK

/* Constants for extracting processor and client IDs from sender or receiver IDs.
 * The following are meant for private use to define public macros.
 */
#define CONID_PROCESSOR_ID_SHIFT          5
#define CONID_PROCESSOR_ID_MASK           0x0007
#define CONID_CLIENT_ID_MASK              0x001F
#define CONID_PACKED_CLIENT_ID_MASK       0x1F1F
#define CONID_PACKED_RECV_PROC_ID_MASK    0xE000

/* Constants for extracting client index and the special client flag from
 * sender or receiver IDs.
 * The following are meant for private use to define public macros.
 */
#define CONID_SPECIAL_CLIENT_ID_MASK      0x0010
#define CONID_CLIENT_INDEX_MASK           0x000F


/* Public MACROs to access and use the information available in the
 * sender and receiver id.
 */

/**
 * \brief Used for building a sender OR receiver ID from client ID and processor ID.
 *
 * \param proc_id Processor id [PROC_ID_NUM]
 * \param client_id Client id, with the special flag information.
 *
 * \return Sender or Receiver id.
 */
static inline CONNECTION_PEER adaptor_pack_terminal_id(PROC_ID_NUM proc_id,
                                                       CONNECTION_PEER client_id)
{
    uint16 result;

    result = proc_id & CONID_PROCESSOR_ID_MASK;
    result <<= CONID_PROCESSOR_ID_SHIFT;
    result |= client_id & CONID_CLIENT_ID_MASK;
    
    return (CONNECTION_PEER) result;
}
#define PACK_SEND_RECV_ID(proc_id, client_id) adaptor_pack_terminal_id(proc_id, client_id)

/**
 * \brief Get client ID from sender OR receiver ID.
 *
 * \param id Sender or Receiver id.
 *
 * \return Client ID.
 */
#define GET_SEND_RECV_ID_CLIENT_ID(id) \
    ((id) & CONID_CLIENT_ID_MASK)

/**
 * \brief Get owner client ID from NOT reversed connection id.
 *
 * \param con_id Connection ID.
 *
 * \return Client ID.
 */
#define GET_CON_ID_OWNER_CLIENT_ID(id) \
    (GET_CON_ID_SEND_ID(id) & CONID_CLIENT_ID_MASK)

/**
 * \brief Extract the receiver client id from a connection id,
 *        without the information on the processor id.
 *
 * \param con_id Connection id.
 *
 * \return Receiver client ID.
 */
static inline CONNECTION_PEER adaptor_ext_con_id_recv_id(CONNECTION_LINK con_id)
{
    uint16 result;

    result = (uint16) (con_id >> RECEIVER_SENDER_ID_SHIFT);
    result &= CONID_CLIENT_ID_MASK;

    return (CONNECTION_PEER) result;
}
#define GET_EXT_CON_ID_RECV_ID(con_id) adaptor_ext_con_id_recv_id(con_id)

/**
 * \brief Extract the packed information about the receiver and sender client id
 *        from a connection id, without the information on the processor id.
 *
 * \param conid Connection id.
 *
 * \return Connection ID without processor information.
 */
static inline CONNECTION_LINK adaptor_strip_proc(CONNECTION_LINK con_id)
{
    uint16 result;

    result = con_id & CONID_PACKED_CLIENT_ID_MASK;
    
    return (CONNECTION_LINK) result;
}
#define GET_UNPACKED_CONID(conid) adaptor_strip_proc(conid)

/**
 * \brief Extract the packed information about the receiver and sender client id
 *        from a connection id and reverse it.
 *
 * \param conid Connection id.
 *
 * \return Reversed connection ID without processor information.
 */
#define UNPACK_REVERSE_CONID(conid) \
    REVERSE_CONNECTION_ID(GET_UNPACKED_CONID(conid))

#if defined(SUPPORTS_MULTI_CORE)

/**
 * \brief Extract the processor ID from the receiver information of a
 *        connection id.
 *
 * \param conid Connection id.
 *
 * \return Receiver processor ID.
 */
static inline PROC_ID_NUM adaptor_get_recv_proc_id(CONNECTION_LINK conid)
{
    uint16 id;
    id = GET_CON_ID_RECV_ID(conid);
    id >>= CONID_PROCESSOR_ID_SHIFT;
    id &= CONID_PROCESSOR_ID_MASK;
    return (PROC_ID_NUM) id;
}
#define GET_RECV_PROC_ID(conid) adaptor_get_recv_proc_id(conid)

/**
 * \brief Extract the processor ID from the sender information of a
 *        connection id.
 *
 * \param conid Connection id.
 *
 * \return Sender processor ID.
 */
static inline PROC_ID_NUM adaptor_get_send_proc_id(CONNECTION_LINK conid)
{
    uint16 id;
    id = GET_CON_ID_SEND_ID(conid);
    id >>= CONID_PROCESSOR_ID_SHIFT;
    id &= CONID_PROCESSOR_ID_MASK;
    return (PROC_ID_NUM) id;
}
#define GET_SEND_PROC_ID(conid) adaptor_get_send_proc_id(conid)

/**
 * \brief Add processor information to the connection id.
 *
 * \param conid Packed connection id.
 * \param procid Processor id.
 *
 * \return Packed connection ID with processor information.
 */
static inline CONNECTION_LINK adaptor_pack_proc_id(CONNECTION_LINK conid,
                                                   PROC_ID_NUM procid)
{
    uint16 id;
    uint16 recv_id;
    uint16 send_id;

    recv_id = (uint16) procid;
    recv_id &= CONID_PROCESSOR_ID_MASK;
    recv_id <<= RECEIVER_SENDER_ID_SHIFT + CONID_PROCESSOR_ID_SHIFT;

    send_id = (uint16) proc_get_processor_id();
    send_id &= CONID_PROCESSOR_ID_MASK;
    send_id <<= CONID_PROCESSOR_ID_SHIFT;

    id = (uint16) GET_UNPACKED_CONID(conid);
    id |= recv_id;
    id |= send_id;

    return (CONNECTION_LINK) id;
}
#define PACK_CONID_PROCID(conid, procid) adaptor_pack_proc_id(conid, procid)

#else

static inline PROC_ID_NUM adaptor_get_recv_proc_id(CONNECTION_LINK conid)
{
    return PROC_PROCESSOR_0;
}
#define GET_RECV_PROC_ID(conid) adaptor_get_recv_proc_id(conid)

static inline PROC_ID_NUM adaptor_get_send_proc_id(CONNECTION_LINK conid)
{
    return PROC_PROCESSOR_0;
}
#define GET_SEND_PROC_ID(conid) adaptor_get_send_proc_id(conid)

static inline CONNECTION_LINK adaptor_pack_proc_id(CONNECTION_LINK conid,
                                                   PROC_ID_NUM procid)
{
    uint16 id;
    uint16 recv_id;

    recv_id = (uint16) procid;
    recv_id &= CONID_PROCESSOR_ID_MASK;
    recv_id <<= RECEIVER_SENDER_ID_SHIFT + CONID_PROCESSOR_ID_SHIFT;

    id = (uint16) GET_UNPACKED_CONID(conid);
    id |= recv_id;

    return (CONNECTION_LINK) id;
}
#define PACK_CONID_PROCID(conid, procid) adaptor_pack_proc_id(conid, procid)
#endif /* defined(SUPPORTS_MULTI_CORE) */

/**
 * \brief Tells if the client is a special client (as opposed to an application
 *        client).
 *
 * \param id Sender or Receiver (client) id.
 *
 * \return TRUE if the client is a special client.
 */
#define GET_SEND_RECV_ID_IS_SPECIAL_CLIENT(id) \
            ( ((id) & CONID_SPECIAL_CLIENT_ID_MASK) \
            == CONID_SPECIAL_CLIENT_ID_MASK )

/**
 * \brief Get the client index from a Sender or Receiver (client) id.
 *
 * \param id Sender or Receiver (client) id.
 *
 * \return Client unique index.
 */
#define GET_SEND_RECV_ID_CLIENT_INDEX(id) \
            ( (id) & CONID_CLIENT_INDEX_MASK )


/**
 * \brief Makes a special client's id from a client index.
 *
 * \param index Client index.
 *
 * \return Client id.
 */
#define MAKE_SPECIAL_CLIENT_ID(index) \
            ((CONNECTION_PEER) ((index) | CONID_SPECIAL_CLIENT_ID_MASK))

/**
 * \brief Makes an application client's id from a client index.
 *
 * \param index Client index.
 *
 * \return Client id.
 */
#define MAKE_APPLICATION_CLIENT_ID(index) \
            ((CONNECTION_LINK) (index) )

/****************************************************************************
Public Type Definitions
*/


/****************************************************************************
Public Function Definitions
*/

#endif /* CONNECTION_ID_H */
