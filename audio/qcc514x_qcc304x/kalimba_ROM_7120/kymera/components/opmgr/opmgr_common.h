/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  opmgr_common.h
 * \ingroup opmgr
 *
 * Operator Manager public header file. <br>
 * This file contains public opmgr types and functions that are used also by operators. <br>
 *
 */

#ifndef OPMGR_COMMON_H
#define OPMGR_COMMON_H

#include "stream_prim.h"
#include "proc/proc.h"

/* Used to mark variable length arrays in structures. */
#define OPMGR_ANY_SIZE 1

#define INT_OP_ID unsigned
#define EXT_OP_ID unsigned

typedef struct OPERATOR_DATA OPERATOR_DATA;
typedef struct OP_UNSOLICITED_MSG OP_UNSOLICITED_MSG;
typedef union OPMSG_GET_CONFIG_RESULT OPMSG_GET_CONFIG_RESULT;

/* Helper macros for creation of opmsgs
 * These are necessary because the structure definitions and associated macros
 * in opmsg_prim are wrong for at least some platforms
 */
/* macro for reading a word from opmsg with an offset from a named field */
#define OPMSG_CREATION_FIELD_GET_FROM_OFFSET(msg, msgtype, field, offset) (((unsigned int *)msg)[msgtype##_##field##_WORD_OFFSET + offset])

/* macro for writing a word from opmsg with an offset from a named field */
#define OPMSG_CREATION_FIELD_SET_FROM_OFFSET(msg, msgtype, field, offset, value) ((((unsigned int *)msg)[msgtype##_##field##_WORD_OFFSET + offset]) = (value))

/* macro for retrieving a pointer to a word from opmsg with an offset from a named field */
#define OPMSG_CREATION_FIELD_POINTER_GET_FROM_OFFSET(msg, msgtype, field, offset) (&((unsigned int *)msg)[msgtype##_##field##_WORD_OFFSET + offset])

/* get a uint16 opmsg field */
#define OPMSG_CREATION_FIELD_GET(msg, msgtype, field) OPMSG_CREATION_FIELD_GET_FROM_OFFSET(msg, msgtype, field, 0)

/* Get a 32 bit value */
#define OPMSG_CREATION_FIELD_GET32(msg, msgtype, field) \
                (((uint32)OPMSG_CREATION_FIELD_GET_FROM_OFFSET(msg, msgtype, field,0)) | \
                ((uint32)OPMSG_CREATION_FIELD_GET_FROM_OFFSET(msg, msgtype, field,1) << 16))
/* set a uint16 opmsg field */
#define OPMSG_CREATION_FIELD_SET(msg, msgtype, field, value) OPMSG_CREATION_FIELD_SET_FROM_OFFSET(msg, msgtype, field, 0, value)

/* set a uint32 opmsg field */
#define OPMSG_CREATION_FIELD_SET32(msg, msgtype, field, value) \
           do {  \
               OPMSG_CREATION_FIELD_SET_FROM_OFFSET(msg, msgtype, field, 0, value & 0xffff ); \
               OPMSG_CREATION_FIELD_SET_FROM_OFFSET(msg, msgtype, field, 1, value >> 16 ); \
              } while(0)


/* get a pointer to an opmsg field */
#define OPMSG_CREATION_FIELD_POINTER_GET(msg, msgtype, field) OPMSG_CREATION_FIELD_POINTER_GET_FROM_OFFSET(msg, msgtype, field, 0)

/**
 * \brief Gets the operator id given a terminal endpoint of that operator.
 *
 * \param  opidep the endpoint id of an operator terminal endpoint.
 *
 * \return The id of the operator.
 */
extern unsigned int get_opid_from_opidep(unsigned int opidep);

/** Convert an external opid into an internal opid */
#define EXT_TO_INT_OPID(id) get_opid_from_opidep(id)

/** Convert an internal opid into an external opid */
#define INT_TO_EXT_OPID(id) (STREAM_EP_OP_BIT | (id << STREAM_EP_OPID_POSN))

/** Structure for capabilities to return the terminals they touched in */
typedef struct TOUCHED_TERMINALS
{
    /** Bitfield representing the source terminals that were written to and
     * should be kicked */
    unsigned sources;
    /** Bitfield representing the sink terminals that should be kicked as the
     * capability is starved. */
    unsigned sinks;
} TOUCHED_TERMINALS;

#define TERMINAL_NUM_MASK            0x003F

/* TODO- this for now defined to match current (temp?) convention in EP-to-terminal translation */
#define TERMINAL_SINK_MASK              (1 << 23)

/**
 * Enum which is used to signal which side of the operator doesn't want kick.
 * SOURCE_SIDE and SINK_SIDE can be used as mask to check if the kicks are disabled at
 * one side.
 */
typedef enum
{
    NEITHER_SIDE = 0,
    SOURCE_SIDE = 1,
    SINK_SIDE = 2,
    BOTH_SIDES = 3
} STOP_KICK;

typedef struct pendingContext pendingContext;

#endif /* OPMGR_COMMON_H */

