/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup adaptor Adaptors to kymera interface
 *
 * \file adaptor.h
 * \ingroup adaptor
 *
 */

#ifndef _ADAPTOR_H_
#define _ADAPTOR_H_

#include "types.h"
#include "connection_id.h"
#if defined(SUPPORTS_MULTI_CORE)
#include "kip_msg_prim.h"
#endif

/****************************************************************************
Public Constant Declarations
*/

#define ADAPTOR_ANY_SIZE 1

/****************************************************************************
Public Macro Definitions
*/


/* Macros used to prepare lists received over transports that are 16-bit wide (such as
 * ACCMD & OBPM) for use in platform independent modules. On platforms where uint16 is
 * not same as unsigned, this involves allocating temporary array, which needs to be
 * freed after use.
 * Also provide protection against zero allocation and freeing of unallocated memory
 */

#if (ADDR_PER_WORD == 1)

typedef const uint16 *UINT16_LISTPTR;

#define ADAPTOR_UNPACK_UINT16_LIST_TO_UNSIGNED(p,c) (unsigned*)(p)
#define ADAPTOR_PACK_UNSIGNED_LIST_TO_UINT16(p, c) (UINT16_LISTPTR)(p)
#define ADAPTOR_FREE_LIST(p) (void)0

#else

typedef uint16 *UINT16_LISTPTR;

#define ADAPTOR_UNPACK_UINT16_LIST_TO_UNSIGNED(p,c) adaptor_unpack_list_to_unsigned((p),(c))
#define ADAPTOR_PACK_UNSIGNED_LIST_TO_UINT16(p, c) adaptor_pack_list_to_uint16(NULL, (p), (c))
#define ADAPTOR_FREE_LIST(p) pfree(p)

#endif

/****************************************************************************
Public Type Definitions
*/

typedef enum
{
    ADAPTOR_DEFAULT  = 0x00,
    ADAPTOR_STIBBONS = 0x01,
    ADAPTOR_OBPM     = 0x1F,
    ADAPTOR_INVALID  = 0xFF
} ADAPTOR_TARGET;

/**
 * Adapter Message IDs
 */
typedef enum
{
    AMSGID_FROM_OPERATOR        = 0x00000,
    AMSGID_FROM_FRAMEWORK       = 0x00001,
    AMSGID_PS_ENTRY_READ        = 0x00002,
    AMSGID_PS_ENTRY_WRITE       = 0x00003,
    AMSGID_PS_ENTRY_DELETE      = 0x00004,
    AMSGID_PS_SHUTDOWN_COMPLETE = 0x00005,
    AMSGID_FILE_DETAILS_QUERY   = 0x00006,
    AMSGID_KIP_START            = 0x10000,
    AMSGID_KIP_END              = 0x1ffff
} ADAPTOR_MSGID;

typedef struct
{
    uint16f ext_op_id;
    uint16f client_id;
    uint16f msg_id;
    uint16f length;
    uint16f message[ADAPTOR_ANY_SIZE];
} ADAPTOR_FROM_OPERATOR_MSG;
#define ADAPTOR_FROM_OPERATOR_MSG_MESSAGE_WORD_OFFSET (4)
#define ADAPTOR_FROM_OPERATOR_MSG_WORD_SIZE (5)

typedef struct
{
    uint16f key;
    uint16f message[ADAPTOR_ANY_SIZE];
} ADAPTOR_FROM_FRAMEWORK_MSG;
#define ADAPTOR_FROM_FRAMEWORK_MSG_MESSAGE_WORD_OFFSET (1)
#define ADAPTOR_FROM_FRAMEWORK_MSG_WORD_SIZE (2)

typedef struct
{
    uint16f key_id;
    uint16f offset;
} ADAPTOR_PS_ENTRY_READ_MSG;
#define ADAPTOR_PS_ENTRY_READ_MSG_WORD_SIZE (2)

typedef struct
{
    uint16f key_id;
    uint16f total_len;
    uint16f offset;
    uint16f data[ADAPTOR_ANY_SIZE];
} ADAPTOR_PS_ENTRY_WRITE_MSG;
#define ADAPTOR_PS_ENTRY_WRITE_MSG_DATA_WORD_OFFSET (3)
#define ADAPTOR_PS_ENTRY_WRITE_MSG_WORD_SIZE (4)

typedef struct
{
    uint16f key_id;
} ADAPTOR_PS_ENTRY_DELETE_MSG;
#define ADAPTOR_PS_ENTRY_DELETE_MSG_WORD_SIZE (1)

#define ADAPTOR_PS_SHUTDOWN_COMPLETE_MSG_WORD_SIZE (0)

typedef struct
{
    uint16f filename[ADAPTOR_ANY_SIZE]; /*!< Packed file name: 2 characters per word. */
} ADAPTOR_FILE_DETAILS_QUERY_MSG;
#define ADAPTOR_FILE_DETAILS_QUERY_MSG_DATA_WORD_OFFSET (0)
#define ADAPTOR_FILE_DETAILS_QUERY_MSG_WORD_SIZE (1)

/* The GNU compiler is better at dealing with types than the Kalimba
   compiler. So use the former to prevent the direct use of weak types
   which has been the source of several bugs and protocol violations. */
#ifdef __GNUC__
typedef union
{
    ADAPTOR_FROM_OPERATOR_MSG          *adaptor00;
    ADAPTOR_FROM_FRAMEWORK_MSG         *adaptor01;
    ADAPTOR_PS_ENTRY_READ_MSG          *adaptor02;
    ADAPTOR_PS_ENTRY_WRITE_MSG         *adaptor03;
    ADAPTOR_PS_ENTRY_DELETE_MSG        *adaptor04;
    ADAPTOR_FILE_DETAILS_QUERY_MSG     *adaptor05;
#if defined(SUPPORTS_MULTI_CORE)
    KIP_MSG_REQ_STRUC                  *kip00;
    KIP_MSG_MESSAGE_FROM_OPERATOR_REQ  *kip01;
    KIP_MSG_PS_WRITE_REQ               *kip02;
    KIP_MSG_PS_SHUTDOWN_REQ            *kip03;
    KIP_MSG_PS_SHUTDOWN_COMPLETE_REQ   *kip04;
    KIP_MSG_HANDLE_EOF_REQ             *kip05;
    KIP_MSG_PUBLISH_FAULT_REQ          *kip06;
    KIP_MSG_FILE_MGR_RELEASE_FILE_REQ  *kip07;
    KIP_MSG_PS_READ_RES                *kip08;
    KIP_MSG_PS_WRITE_RES               *kip09;
#endif /* defined(SUPPORTS_MULTI_CORE) */
} ADAPTOR_DATA;
#define ADAPTOR_NULL ((ADAPTOR_DATA)(ADAPTOR_FROM_OPERATOR_MSG*) NULL)
#else
typedef uint16f* ADAPTOR_DATA;
#define ADAPTOR_NULL NULL
#endif

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief  Returns the type of adaptor a connection link is
 *         associated with.
 *
 * \param  conn_id the connection ID in forward direction
 * 
 * \return the type of adaptor.
 */
extern ADAPTOR_TARGET adaptor_get_target(CONNECTION_LINK conn_id);

/**
 * \brief  Send message function.
 *
 * \param  conn_id the connection ID (sender and recipient ID codes)
 * \param  msg_id ID of the message
 * \param  msg_length length of payload
 * \param  msg_data pointer to the message payload
 *
 * \note   There can be at most 16 bits of useful data in each word
 *         of the array pointed by msg_data. KIP uses uint16 while
 *         other adaptors use uint16f.
 * 
 * \return success/fail (true/false).
 */
extern bool adaptor_send_message(CONNECTION_LINK conn_id, ADAPTOR_MSGID msg_id,
                                 unsigned msg_length, ADAPTOR_DATA msg_data);

/**
 * \brief Unpack a list of parameters in a message received over 16-bit wide transport
 * for use in a platform independent module
 *
 * \param pparam Pointer to the start of the parameter list within the message
 *
 * \param count  Number of elements in the list
 *
 * \return Unpacked list, which needs to be freed after use or NULL if count is 0
 */
extern uint16f *adaptor_unpack_list_to_unsigned(uint16* pparam, unsigned count);

/**
 * \brief converts an unsigned array into uint16 array.
 *  Allocates the destination uint16 array if required.
 *
 * \param pdest pointer to the destination uint16 array.
 *  Can be NULL in which case, the destination array
 *  is allocated on a non-zero count.
 *
 * \param psrc pointer to the source unsigned array.
 *
 * \param count number of elements in the source list.
 *
 * \return pointer to the destination uint16 array, if allocated.
 */
extern uint16* adaptor_pack_list_to_uint16(uint16* ppdu, const uint16f* psrc, unsigned count);

/**
 * \brief Check if a pointer is owned by an adaptor.
 *
 * \param context Pointer to be checked.
 *
 * \return TRUE if an adaptor owns the pointer.
 *
 * \note This function is useful to API that callback
 *       to functions for tasks belonging to different
 *       components and for which the pointer might
 *       become invalid before the function is called
 *       back.
 */
extern bool adaptor_does_context_exist(void *context);

#endif /* _ADAPTOR_H_ */
