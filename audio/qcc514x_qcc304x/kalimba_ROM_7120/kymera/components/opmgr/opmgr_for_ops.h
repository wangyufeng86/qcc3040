/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  opmgr_for_ops.h
 * \ingroup opmgr
 *
 * Operator Manager public header file. <br>
 * This file contains public opmgr functions and types that are used also by operators. <br>
 *
 */

#ifndef OPMGR_FOR_OPS_H
#define OPMGR_FOR_OPS_H

#include "types.h"
#include "status_prim.h"
#include "cap_id_prim.h"
#include "opmgr/opmgr_common.h"
#include "pl_timers/pl_timers.h"
#include "buffer/buffer.h"
#include "rate/rate_types.h"
#include "stream/stream_common.h"

typedef struct CBOPS_PARAMETERS CBOPS_PARAMETERS;
typedef void* OVERRIDE_EP_HANDLE;

/* Changes the terminal direction. */
#define SWAP_TERMINAL_DIRECTION(TERMINAL)   ((TERMINAL)^(TERMINAL_SINK_MASK))

/* Changes the channel number. */
#define CHANGE_TERMINAL_CHANNEL(TERMINAL, CHANNEL)   ( ((TERMINAL)&(~TERMINAL_NUM_MASK))|(CHANNEL) )

/** Bitfield definitions for returning the terminals that an operator touched */
#define TOUCHED_NOTHING (0)

#define TOUCHED_SINK_0 (1)
#define TOUCHED_SINK_1 (1 << 1)
#define TOUCHED_SINK_2 (1 << 2)
#define TOUCHED_SINK_3 (1 << 3)
#define TOUCHED_SINK_4 (1 << 4)
#define TOUCHED_SINK_5 (1 << 5)
#define TOUCHED_SINK_6 (1 << 6)
#define TOUCHED_SINK_7 (1 << 7)
#define TOUCHED_SINK_8 (1 << 8)
#define TOUCHED_SINK_9 (1 << 9)
#define TOUCHED_SINK_10 (1 << 10)
#define TOUCHED_SINK_11 (1 << 11)
#define TOUCHED_SINK_12 (1 << 12)
#define TOUCHED_SINK_13 (1 << 13)
#define TOUCHED_SINK_14 (1 << 14)
#define TOUCHED_SINK_15 (1 << 15)
#define TOUCHED_SINK_16 (1 << 16)
#define TOUCHED_SINK_17 (1 << 17)
#define TOUCHED_SINK_18 (1 << 18)
#define TOUCHED_SINK_19 (1 << 19)
#define TOUCHED_SINK_20 (1 << 20)
#define TOUCHED_SINK_21 (1 << 21)
#define TOUCHED_SINK_22 (1 << 22)
#define TOUCHED_SINK_23 (1 << 23)
#if (DAWTH == 32)
#define TOUCHED_SINK_24 (1 << 24)
#define TOUCHED_SINK_25 (1 << 25)
#define TOUCHED_SINK_26 (1 << 26)
#define TOUCHED_SINK_27 (1 << 27)
#define TOUCHED_SINK_28 (1 << 28)
#define TOUCHED_SINK_29 (1 << 29)
#define TOUCHED_SINK_30 (1 << 30)
#define TOUCHED_SINK_31 (1 << 31)
#endif /* DAWTH == 32 */

#define TOUCHED_SOURCE_0 (1)
#define TOUCHED_SOURCE_1 (1 << 1)
#define TOUCHED_SOURCE_2 (1 << 2)
#define TOUCHED_SOURCE_3 (1 << 3)
#define TOUCHED_SOURCE_4 (1 << 4)
#define TOUCHED_SOURCE_5 (1 << 5)
#define TOUCHED_SOURCE_6 (1 << 6)
#define TOUCHED_SOURCE_7 (1 << 7)
#define TOUCHED_SOURCE_8 (1 << 8)
#define TOUCHED_SOURCE_9 (1 << 9)
#define TOUCHED_SOURCE_10 (1 << 10)
#define TOUCHED_SOURCE_11 (1 << 11)
#define TOUCHED_SOURCE_12 (1 << 12)
#define TOUCHED_SOURCE_13 (1 << 13)
#define TOUCHED_SOURCE_14 (1 << 14)
#define TOUCHED_SOURCE_15 (1 << 15)
#define TOUCHED_SOURCE_16 (1 << 16)
#define TOUCHED_SOURCE_17 (1 << 17)
#define TOUCHED_SOURCE_18 (1 << 18)
#define TOUCHED_SOURCE_19 (1 << 19)
#define TOUCHED_SOURCE_20 (1 << 20)
#define TOUCHED_SOURCE_21 (1 << 21)
#define TOUCHED_SOURCE_22 (1 << 22)
#define TOUCHED_SOURCE_23 (1 << 23)
#if (DAWTH == 32)
#define TOUCHED_SOURCE_24 (1 << 24)
#define TOUCHED_SOURCE_25 (1 << 25)
#define TOUCHED_SOURCE_26 (1 << 26)
#define TOUCHED_SOURCE_27 (1 << 27)
#define TOUCHED_SOURCE_28 (1 << 28)
#define TOUCHED_SOURCE_29 (1 << 29)
#define TOUCHED_SOURCE_30 (1 << 30)
#define TOUCHED_SOURCE_31 (1 << 31)
#endif /* DAWTH == 32 */

/****************************************************************************
Type Declarations
*/

/** Function pointer prototype of message handler functions. */
typedef bool (*handler_function)(struct OPERATOR_DATA *cap_data, void *message_body,
              unsigned *response_id, void **response_data);

#ifdef INSTALL_OPERATOR_CREATE_PENDING
/* Support for asynchronous operator functions.
   This is not currently intended for customer usage */
typedef enum
{
    HANDLER_FAILED = FALSE,
    HANDLER_COMPLETE = TRUE,
    HANDLER_INCOMPLETE
} PENDABLE_OP_HANDLER_RETURN;

/* Callback function that may be returned by operator function in response_data */
typedef void (*pending_operator_cb)(struct OPERATOR_DATA *cap_data,
                                    uint16 msgId, void *msg,
                                    pendingContext *routing,
                                    unsigned supplied_id);

/* Function pointer prototype for message handler functions that can pend. */
typedef PENDABLE_OP_HANDLER_RETURN (*handler_function_with_cb)
                    (struct OPERATOR_DATA *cap_data, void *message_body,
                     unsigned *response_id, void **response_data);
#endif /* INSTALL_OPERATOR_CREATE_PENDING */

/** Struct used by capabilities to communicate their entry points to opmgr. */
/* Do not forget to update enum OPCMD_ID after modifying this structure. */
typedef struct
{
    handler_function op_create;
    handler_function op_destroy;
    handler_function op_start;
    handler_function op_stop;
    handler_function op_reset;
    handler_function op_connect;
    handler_function op_disconnect;
    handler_function op_buffer_details;
    handler_function op_data_format;
    handler_function op_get_sched_info;
} handler_lookup_struct;

/**
 * Message IDs
 */
/* The values in the enum must match the order of the fields in the
 * previous structure. There should be no gaps.
 * Except for 2 special values that should be kept at the end of the
 * range, the last value should always be OPCMD_LAST_ID. */
typedef enum
{
    OPCMD_CREATE = 0x0000,
    OPCMD_DESTROY = 0x0001,
    OPCMD_START = 0x0002,
    OPCMD_STOP = 0x0003,
    OPCMD_RESET = 0x0004,
    OPCMD_CONNECT = 0x0005,
    OPCMD_DISCONNECT = 0x0006,
    OPCMD_BUFFER_DETAILS = 0x0007,
    OPCMD_DATA_FORMAT = 0x0008,
    OPCMD_GET_SCHED_INFO = 0x0009,
    OPCMD_TEST = 0x000A,
    OPCMD_ARRAY_SIZE = 0x000B, /* Do not forget to update this value. */

    OPCMD_MESSAGE = 0xFFFD,
    OPCMD_FROM_OPERATOR = 0xFFFE,
    OPCMD_INVALID = 0xFFFF,
} OPCMD_ID;

/* The array is used by opmgr to access the member functions quickly when
   given OPCMD_ID */
typedef union
{
    handler_function      by_index[OPCMD_ARRAY_SIZE];
    handler_lookup_struct by_member;
} handler_lookup_table_union;

/** Forward declaration of OP_OPMSG_RSP_PAYLOAD. */
typedef struct op_opmsg_rsp_payload OP_OPMSG_RSP_PAYLOAD;

/** Function pointer prototype of operator message handler functions. */
typedef bool (*opmsg_handler_function)(struct OPERATOR_DATA *op_data, void *message_body,
        unsigned *response_length, OP_OPMSG_RSP_PAYLOAD **response_data);

/** Structure of an operator message function table entry */
typedef struct
{
    /** Opmsg ID associated with the handler */
    unsigned id;

    /** Function pointer of the function that is to be used to service the operator message */
    opmsg_handler_function handler;
} opmsg_handler_lookup_table_entry;


/** Static capability data */
typedef struct CAPABILITY_DATA
{
    /** Capability ID */
    CAP_ID id;

    /** Version information - hi and lo parts */
    unsigned version_msw;
    unsigned version_lsw;

    /** Max number of sinks/inputs and sources/outputs */
    unsigned max_sinks;
    unsigned max_sources;

    /** Pointer to message handler function table */
    const handler_lookup_struct *handler_table;

    /** Pointer to operator message handler function table */
    const opmsg_handler_lookup_table_entry *opmsg_handler_table;

    /** Pointer to data processing function */
    void (*process_data)(struct OPERATOR_DATA*, TOUCHED_TERMINALS*);

    /** Reserved (previously intended for processing time, but never used) */
    unsigned cap_data_reserved;

    /** Size of capability-specific per-instance data */
    unsigned instance_data_size;

} CAPABILITY_DATA;


/** Standard response message structure */
typedef struct
{
    unsigned op_id;
    unsigned status; /* Serialized STATUS_KYMERA */
    union{
        unsigned data;
        unsigned err_code;
    }resp_data;
}OP_STD_RSP;


/** Scheduler information response message structure */
typedef struct
{
    unsigned op_id;
    unsigned status; /* Serialized STATUS_KYMERA */
    unsigned block_size;
    unsigned run_period;
    bool locally_clocked;
}OP_SCHED_INFO_RSP;


/** Buffer details response message structure */
typedef struct
{
    /** The ID of the operator the response is from */
    unsigned op_id;
    /** Whether the request was successful or encountered an error */
    unsigned status;

    /** Flag indicating if the operator terminal wants to supply the buffer for the connection */
    bool supplies_buffer:1;
    /** Flag indicating if the operator terminal can be overridden by the endpoint
     * being connected to. */
    bool can_override:1;
    /** Flag indicating if the operator terminal needs to override the endpoint
     * being connected to */
    bool needs_override:1;
    /** Flag indicating if the operator terminal can run in-place re-using the
     * same buffer at a corresponding input/output terminals */
    bool runs_in_place:1;
#ifdef INSTALL_METADATA
    bool supports_metadata:1;
    tCbuffer *metadata_buffer;
#endif /* INSTALL_METADATA */

    /** This union is discriminated by the flags the combination of flags that have been set */
    union buffer_op_info_union{
        /** The minimum size of buffer being requested to form the connection */
        unsigned buffer_size;
        /** The buffer supplied when supplies_buffer is TRUE */
        tCbuffer *buffer;

        /** The buffer parameters being requested when runs_in_place is TRUE */
        struct{
            /**
             * The in place terminal shows where the operator would run in pace on for
             * the asked endpoint. */
            unsigned int in_place_terminal;
            /** The minimum buffer size in words needed for the operator. */
            unsigned int size;
            /** A pointer to the buffer that the in_place_terminal is connected to.
             * NULL if the terminal is not yet connected. */
            tCbuffer *buffer;
        }in_place_buff_params;
    }b;
}OP_BUF_DETAILS_RSP;

/** Get configuration message response structure */
typedef struct
{
    unsigned op_id;
    unsigned status;
    unsigned length;
    uint32 value;
}OP_GET_CONFIG_RSP;

/** Operator connect header structure */
typedef struct
{
    unsigned terminal_id;
    tCbuffer *buffer;
} OP_CONNECT_HEADER;

#define OPMGR_GET_OP_CONNECT_TERMINAL_ID(msg) (((OP_CONNECT_HEADER *)(msg))->terminal_id)
#define OPMGR_GET_OP_CONNECT_BUFFER(msg) ((((OP_CONNECT_HEADER *)(msg)))->buffer)

/** Operator disconnect header structure */
typedef struct
{
    unsigned terminal_id;
} OP_DISCONNECT_HEADER;

#define OPMGR_GET_OP_DISCONNECT_TERMINAL_ID(msg) (((OP_DISCONNECT_HEADER *)(msg))->terminal_id)

/** Operator buffer_details header structure */
typedef struct
{
    unsigned terminal_id;
} OP_BUF_DETAILS_HEADER;

#define OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(msg) (((OP_BUF_DETAILS_HEADER *)(msg))->terminal_id)

/** Operator sched_info header structure */
typedef struct
{
    unsigned terminal_id;
} OP_SCHED_INFO_HEADER;

#define OPMGR_GET_OP_SCHED_INFO_TERMINAL_ID(msg) (((OP_SCHED_INFO_HEADER *)(msg))->terminal_id)

/** Operator data_format header structure */
typedef struct
{
    unsigned terminal_id;
} OP_DATA_FORMAT_HEADER;

#define OPMGR_GET_OP_DATA_FORMAT_TERMINAL_ID(msg) (((OP_DATA_FORMAT_HEADER *)(msg))->terminal_id)

/** Header structure for operator command message (OPCMD_MESSAGE). */
typedef struct
{
    /**client_id - To allow operators to determine the message source,*/
    unsigned client_id;
    /** length of the remaining content */
    unsigned length;
}OPCMD_MSG_HEADER;

/** Operator message header structure. */
typedef struct
{
    OPCMD_MSG_HEADER cmd_header;
    /** operator message ID */
    unsigned msg_id;
}OPMSG_HEADER;

/* Macros for accessing the header fields for the operator command message.*/
#define OPMGR_GET_OPCMD_MESSAGE_CLIENT_ID(x)  ((x)->cmd_header.client_id)
#define OPMGR_GET_OPCMD_MESSAGE_LENGTH(x)     ((x)->cmd_header.length)
#define OPMGR_GET_OPCMD_MESSAGE_MSG_ID(x)     ((x)->msg_id)

/** Get config result for key OPMSG_OP_TERMINAL_RATEMATCH_MEASUREMENT */
typedef struct
{
    /** Approximate (averaged) sample period deviation */
    int sp_deviation;

    /** Measurement triple in rate library format */
    RATE_RELATIVE_RATE measurement;
} OPMSG_GET_CONFIG_RM_MEASUREMENT;

/** Get config result, forward declared in opmgr_common.h */
union OPMSG_GET_CONFIG_RESULT
{
    /** Default result is one word */
    uint32 value;

    /** Result for OPMSG_OP_TERMINAL_RATEMATCH_MEASUREMENT */
    OPMSG_GET_CONFIG_RM_MEASUREMENT rm_measurement;
};

/** Get configuration message structure */
typedef struct
{
    OPMSG_HEADER header;
    /** Key- specify the type of the get configuration. */
    unsigned key;
    /** Pointer which will be populated with the asked configuration value. */
    OPMSG_GET_CONFIG_RESULT* result;
}OPMSG_GET_CONFIG;

/** Configure parameters for key OPMSG_OP_TERMINAL_RATEMATCH_REFERENCE */
typedef struct
{
    /** Legacy adjustment */
    int32 ratio;

    /** Optional reference triple */
    RATE_RELATIVE_RATE ref;
}
OPMSG_CONFIGURE_RM_REFERENCE;

/** Configure message structure */
typedef struct
{
    OPMSG_HEADER header;
    /** Key- specify the type of the configure. */
    unsigned key;
    /** The configure value which will be applied to the operator endpoint.
     * For some keys, this is a pointer to a struct. */
    uint32 value;
}OPMSG_CONFIGURE;

/** Generic opmsg REQ structure - header followed by (any) payload */
typedef struct
{
    OPMSG_HEADER header;

    /** Some payload */
    unsigned payload[];
}OP_MSG_REQ;

/* Macros for accessing the header fields for the operator message.*/
#define OPMGR_GET_OPMSG_MSG_ID(x)             ((x)->header.msg_id)
/* TODO: This should be handled in the common part of capabilities. */
#define OPMGR_GET_OPMSG_CLIENT_ID(x)          ((x)->header.cmd_header.client_id)
#define OPMGR_GET_OPMSG_LENGTH(x)             ((x)->header.cmd_header.length)


/**
 * 'Set control' message data block
 */
typedef struct
{
    unsigned control_id;
    unsigned msw;
    unsigned lsw;
}OPMSG_SET_CONTROL_BLOCK;

/**
 * 'Set control' message structre
 */
typedef struct
{
    OPMSG_HEADER header;
    unsigned num_blocks;
    OPMSG_SET_CONTROL_BLOCK   data[];
}OPMSG_SET_CONTROL_MSG;

/**
 * 'Get parameters' message data block
 */
typedef struct
{
    unsigned param_id;
    unsigned num_params;
}OPMSG_PARAM_BLOCK;

/**
 * 'Get parameters' message structure definiton
 */
typedef struct
{
    OPMSG_HEADER header;
    unsigned num_blocks;
    OPMSG_PARAM_BLOCK data_block[];
}OPMSG_PARAM_MSG;

/**
 * 'Set parameters' message data block
 */
typedef struct
{
    unsigned param_id;
    unsigned num_params;
    unsigned *values;       /* Pointer to the 3:2 encoded parameter values */
}OPMSG_SET_PARAM_BLOCK;

/**
 * 'Set parameters' message structure definition
 */
typedef struct
{
    OPMSG_HEADER header;
    unsigned num_blocks;
    OPMSG_SET_PARAM_BLOCK data_block[];
}OPMSG_SET_PARAM_MSG;

/**
 * 'Get status' message structure definition
 */
typedef struct
{
    OPMSG_HEADER header;
    unsigned stat_config;
}OPMSG_GET_STATUS_MSG;

/**
 * 'Set UCID' message structure definition
 */
typedef struct
{
    OPMSG_HEADER header;
    unsigned ucid;
}OPMSG_SET_UCID_MSG;

/**
 * 'Get LOGICAL PS ID' message structure definition
 */
typedef struct
{
    OPMSG_HEADER header;
    unsigned sid;
}OPMSG_GET_LOGICAL_PS_ID_MSG;

/** Common message IDs for any operators that use the described functionality. We don't
 * call these "standard" IDs because not every operator uses the related features.
 */
typedef enum
{
    MSG_FROM_OP_FADEOUT_DONE = 0x2000, /* it is matched with the historic value */
    MSG_FROM_OP_XXX_YYY /* TODO - whatever other messages */
} COMMON_MSG_FROM_OP_ID;


/** Operator messages - response structure */
/* NOTE: length is 1 or more -  at least an op. msg ID / key ID is present in the payload */
typedef struct
{
    unsigned op_id;
    unsigned status;
    unsigned length;
    unsigned payload[];
} OP_OPMSG_RSP;

/** Variable length opmsg response payload (an opmsg ID, a
 *  capability- & opmsg-specific status word, followed by optional generic payload data.
 */
struct op_opmsg_rsp_payload
{
    unsigned msg_id;
    union
    {
        /* Only the CPS response messages use the status word */
        struct
        {
            unsigned status; /* Serialized OPMSG_RESULT_STATES */
            unsigned data[];
        } cps;
        unsigned raw_data[1];
    } u;
};

/* Payload size in words, with just message ID */
#define OPMSG_RSP_PAYLOAD_SIZE_RAW_DATA(x) ((x) + 1)

/* Payload size in words, with message ID and status */
#define OPMSG_RSP_PAYLOAD_SIZE_CPS_DATA(x) ((x) + 2)



/****************************************************************************
Macro Declarations
*/

/* Helper macros for opmsg field lookup
 * These are necessary because the structure definitions and associated macros
 * in opmsg_prim are wrong for at least some platforms
 */
#define OPCMD_MSG_HEADER_SIZE (sizeof(OPCMD_MSG_HEADER)/sizeof(unsigned int))

/* macro for reading a word from opmsg with an offset from a named field */
#define OPMSG_FIELD_GET_FROM_OFFSET(msg, msgtype, field, offset) ((uint16)(((unsigned int *)msg)[msgtype##_##field##_WORD_OFFSET + OPCMD_MSG_HEADER_SIZE + offset]))

/* macro for retrieving a pointer to a word from opmsg with an offset from a named field */
#define OPMSG_FIELD_POINTER_GET_FROM_OFFSET(msg, msgtype, field, offset) (&((unsigned int *)msg)[msgtype##_##field##_WORD_OFFSET + OPCMD_MSG_HEADER_SIZE + offset])

/* get a uint16 opmsg field */
#define OPMSG_FIELD_GET(msg, msgtype, field) OPMSG_FIELD_GET_FROM_OFFSET(msg, msgtype, field, 0)

/* get a pointer to an opmsg field */
#define OPMSG_FIELD_POINTER_GET(msg, msgtype, field) OPMSG_FIELD_POINTER_GET_FROM_OFFSET(msg, msgtype, field, 0)

/* get a uint32 opmsg filed */
#define OPMSG_FIELD_GET32(msg, msgtype, field) (((uint32)OPMSG_FIELD_GET_FROM_OFFSET(msg, msgtype, field, 0)) + \
                                                (((uint32)OPMSG_FIELD_GET_FROM_OFFSET(msg, msgtype, field, 1))<<16))

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Helper function for finding out where an operator is running
 * or not.
 *
 * \param op_data The data structure of the operator to query
 */
extern bool opmgr_op_is_running(OPERATOR_DATA *op_data);

/**
 * \brief Helper function for kicking the operator in the background from an
 * abnormal source. e.g. some special interrupt the operator setup
 *
 * \param op_data The data structure of the operator to kick
 */
extern void opmgr_kick_operator(OPERATOR_DATA *op_data);

/**
 * \brief Helper function for issue kicks from an operator
 *
 * \param op_data       The data structure of the operator to kick
          source_kicks  Mask of sources to kick
          sink_kicks    Mask of sinks to kick
 */
extern void opmgr_kick_from_operator(OPERATOR_DATA *op_data,
                                     unsigned source_kicks,
                                     unsigned sink_kicks);

extern OPERATOR_DATA* get_op_data_from_id(unsigned int id);

/**
 * \brief Helper for synchronisation between command/message handlers and data
 *        processing. Call this from command/message handlers before modifying
 *        complex data shared between command/message handlers and data
 *        processing.
 * \param op_data       The data structure of the operator
 */
extern void opmgr_op_suspend_processing(OPERATOR_DATA *op_data);

/**
 * \brief Helper for synchronisation between command/message handlers and data
 *        processing. Call this from command/message handlers after modifying
 *        complex data shared between command/message handlers and data
 *        processing.
 * \param op_data       The data structure of the operator
 */
extern void opmgr_op_resume_processing(OPERATOR_DATA *op_data);

/**
 * \brief Helper for synchronisation between command/message handlers and data
 *        processing. Call from command/message handlers while processing is
 *        suspended, to force data processing to run after
 *        opmgr_op_resume_processing. Without this call, data processing will
 *        only be triggered by opmgr_op_resume_processing if the operator
 *        was kicked while processing was suspended.
 * \param op_data       The data structure of the operator
 */
extern void opmgr_op_process_after_resume(OPERATOR_DATA *op_data);

/**
 * \brief Helper for synchronisation between command/message handlers and data
 *        processing. Intended for asserts and similar checks that code
 *        accessing shared data is run only when processing is suspended.
 * \param op_data       The data structure of the operator
 * \return True if processing is suspended
 */
extern bool opmgr_op_is_processing_suspended(OPERATOR_DATA *op_data);

/**
 * \brief Helper for synchronisation between command/message handlers and
 *        strict timer driven data processing. Call this from command/message
 *        handlers before modifying complex data shared between
 *        command/message handlers and strict timer callback.
 * \param op_data       The data structure of the operator
 */
extern void opmgr_op_suspend_processing_strict(OPERATOR_DATA *op_data);

/**
 * \brief Helper for synchronisation between command/message handlers and
 *        strict timer driven data processing. Call this from command/message
 *        handlers after modifying complex data shared between
 *        command/message handlers and strict timer callback.
 * \param op_data       The data structure of the operator
 * \param timer_id      Pointer to the timer id field
 * \param timer_fn      Strict timer callback function
 * \param timer_data    Strict timer callback data
 */
extern void opmgr_op_resume_processing_strict(OPERATOR_DATA *op_data,
                                              tTimerId* timer_id,
                                              tTimerEventFunction timer_fn,
                                              void* timer_data);

/**
 * \brief Helper for synchronisation between command/message handlers and
 *        strict timer driven data processing. Call from command/message
 *        handlers while processing is suspended, to force the timer
 *        handler to be run after opmgr_op_resume_processing. Without this
 *        call, opmgr_op_resume_processing_strict will only trigger
 *        the timer callback if opmgr_op_is_processing_suspended_strict
 *        was called while processing was suspended.
 * \param op_data       The data structure of the operator
 */
extern void opmgr_op_process_after_resume_strict(OPERATOR_DATA *op_data);

/**
 * \brief Helper for synchronisation between command/message handlers and
 *        strict timer driven data processing. Call this at the start of the
 *        strict timer callback. If it returns true, quit before accessing
 *        shared data. The timer callback will then be triggered again from
 *        opmgr_op_resume_processing_strict.
 * \param op_data       The data structure of the operator
 * \return True if processing is suspended
 */
extern bool opmgr_op_is_processing_suspended_strict(OPERATOR_DATA *op_data);

/**
 * \brief  Count the number of operators with matching capability.
 *
 * \param  capid The capability ID to be searched for.
 *               If CAP_ID_NONE, all operators get counted.
 *
 * \return Number of instances of operator with matching capability or
 *         count of all operators.
 */
extern unsigned int opmgr_get_ops_count(CAP_ID capid);

/**
 * \brief  Function to set the endpoint handle of the endpoint associated with
 *         an operator terminal.
 *
 * \param  op_data     Pointer to the structure describing the operator.
 * \param  terminal_id The terminal ID of the operator which is connected to
 *                     the operator endpoint which overrides the real endpoint.
 *
 * \return endpoint handle of the operator terminal's endpoint
 */
extern OVERRIDE_EP_HANDLE opmgr_override_get_endpoint(OPERATOR_DATA *op_data,
                                                      unsigned int terminal_id);

#ifdef INSTALL_DELEGATE_RATE_ADJUST_SUPPORT
/**
 * \brief  Function to get the rate_adjust operator if available for the
 *         overridden endpoint.
 *
 * \param  ep_hdl Endpoint handle.
 * \param  op_id  Pointer to hold the identifier for available standalone
 *                rate adjust operator.
 *
 * \return True on success.
 */
extern bool opmgr_override_get_rate_adjust_op(OVERRIDE_EP_HANDLE ep_hdl,
                                              EXT_OP_ID* op_id);
#endif /* INSTALL_DELEGATE_RATE_ADJUST_SUPPORT */

/**
 * \brief  Function to check if the overridden endpoint is clocked locally or
 *         remotely.
 *
 * \param  ep_hdl Endpoint handle.
 *
 * \return TRUE if the overridden endpoint is locally clocked.
 */
extern bool opmgr_override_is_locally_clocked(OVERRIDE_EP_HANDLE ep_hdl);

/**
 * \brief  Function to tests whether two overridden endpoints use the same
 *         clock source.
 *
 * \param  ep_hdl1 The handle for the operator endpoint which overrides a real
 *                 endpoint.
 * \param  ep_hdl2 The second handle for the operator endpoint which overrides
 *                 another real endpoint.
 *
 * \return TRUE if the two overridden endpoints have same clock source.
 */
extern bool opmgr_override_have_same_clock_source(OVERRIDE_EP_HANDLE ep_hdl1,
                                                  OVERRIDE_EP_HANDLE ep_hdl2);
/**
 * \brief  Function to tests whether there is a sidetone path enabled between two
 * overridden endpoints.
 *
 * \param  mic_hdl The handle for the operator endpoint which overrides a real
 *                 mic endpoint.
 * \param  spkr_hdl The second handle for the operator endpoint which overrides
 *                 a real speaker endpoint.
 *
 * \return TRUE if there is sidetone path between two overridden endpoints else FALSE.
 */
extern bool opmgr_override_have_sidetone_route(OVERRIDE_EP_HANDLE mic_hdl,
                                               OVERRIDE_EP_HANDLE spkr_hdl);

/**
 * \brief  Function to set the ratemaching adjustment for the overridden
 *         endpoint. Only supported when the ratematching ability of the
 *         overriden endpoint is HW.
 *
 * \param  ep_hdl Endpoint handle.
 * \param  value  The new value of the rate adjustment.
 *
 * \return True on success.
 */
extern bool opmgr_override_set_ratematch_adjustment(OVERRIDE_EP_HANDLE ep_hdl,
                                                    int value);

/**
 * \brief  Function to set whether hw warp to be applied directly or
 *         accumulatively.
 *
 * \param  ep_hdl Endpoint handle.
 * \param  enable When true, warp will be applied directly.
 *
 * \return True on success.
 */
extern bool opmgr_override_set_direct_warp(OVERRIDE_EP_HANDLE ep_hdl,
                                           bool enable);

/**
 * \brief  Function to get the shift amount from the cbops parameters.
 *         The sign of the value should be decided by the operator.
 *
 * \param  cbops_parameters Pointer to cbops parameters.
 *
 * \return Amount of shift.
 */
extern int get_shift_from_cbops_parameters(CBOPS_PARAMETERS* cbops_parameters);

/**
 * \brief  Function to get the sampling rate of the overridden endpoint
 *
 * \param  ep_hdl Endpoint handle.
 * \param  value  Pointer to hold the value of the sampling rate.
 *
 * \return True on success.
 */
extern bool opmgr_override_get_sample_rate(OVERRIDE_EP_HANDLE ep_hdl,
                                           unsigned* value);

/**
 * \brief  Function to get the current hw warp applied to endpoint (if any)
 *
 * \param  ep_hdl Endpoint handle.
 * \param  value  Pointer to hold the value hw warp.
 *
 * \return True on success.
 */
extern bool opmgr_override_get_hw_warp(OVERRIDE_EP_HANDLE ep_hdl,
                                       int* value);

/**
 * \brief  Function to set the ADC gain the overridden endpoint. Returns false
 *         if the operation was not successful. Only codec endoints can set the
 *         gain.
 *
 * \param  ep_hdl Endpoint handle.
 * \param  value  The new value of the ADC gain of a codec.
 *
 * \return True on success.
 */
extern bool opmgr_override_set_ep_gain(OVERRIDE_EP_HANDLE ep_hdl,
                                       uint32 value);

/**
 * \brief  Function to set the ratemach enacting for the overridden endpoint.
 *         Only supported when the ratematching ability of the overriden
 *         endpoint is HW.
 *
 * \param  ep_hdl Endpoint handle.
 * \param  value  The new value of the ratematch enacting.
 *
 * \return True on success.
 */
extern bool opmgr_override_set_ratematch_enacting(OVERRIDE_EP_HANDLE ep_hdl,
                                                  bool enable);

/**
 * \brief  Function to get the ratematching ability for the overridden
 *         endpoint.
 *
 * \param  ep_hdl Endpoint handle.
 * \param  value  Pointer to hold the value of the ratematching ability.
 *
 * \return True on success.
 */
extern bool opmgr_override_get_ratematch_ability(OVERRIDE_EP_HANDLE ep_hdl,
                                                 RATEMATCHING_SUPPORT* value);

#ifdef INSTALL_THREAD_OFFLOAD
/**
 * \brief Helper function for finding out whether an operator 
 *        should use the thread offload feature
 *
 * \param op_data The data structure of the operator to query
 */
extern bool opmgr_op_thread_offload(OPERATOR_DATA *op_data);
#endif

#endif /* OPMGR_FOR_OPS_H */
