/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stub_capability.c
 * \ingroup  capabilities
 *
 *  A Stub implementation of a Capability that can be built and communicated
 *  with. This is provided to accelerate the development of new capabilities.
 *
 */

#include "capabilities.h"
#include "stub_capability.h"

/****************************************************************************
Private Type Definitions
*/

/* stub_capability capability-specific per-instance data */
typedef struct
{
    /** define here all the capability specific data */

    /** cap specific data */
    unsigned spec_cap_data;

} STUB_CAPABILITY_OP_DATA;

/****************************************************************************
Private Function Definitions
*/

/****************************************************************************
Private Constant Declarations
*/
#define STUB_CAPABILITY_ID  0xFFFF /* CHANGE THIS VALUE TO THAT ALLOCATED IN wiki/Capability_IDs */

/** The stub capability function handler table */
const handler_lookup_struct stub_capability_handler_table =
{
    base_op_create,           /* OPCMD_CREATE */
    base_op_destroy,          /* OPCMD_DESTROY */
    stub_capability_start,    /* OPCMD_START */
    base_op_stop,             /* OPCMD_STOP */
    base_op_reset,            /* OPCMD_RESET */
    base_op_connect,          /* OPCMD_CONNECT */
    base_op_disconnect,       /* OPCMD_DISCONNECT */
    base_op_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,  /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry stub_capability_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {0, NULL}
};


/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA stub_capability_cap_data =
{
    STUB_CAPABILITY_ID,             /* Capability ID */
    0, 1,                           /* Version information - hi and lo parts */
    1, 1,                           /* Max number of sinks/inputs and sources/outputs */
    &stub_capability_handler_table, /* Pointer to message handler function table */
    stub_capability_opmsg_handler_table,    /* Pointer to operator message handler function table */
    stub_capability_process_data,           /* Pointer to data processing function */
    0,                              /* Reserved */
    sizeof(STUB_CAPABILITY_OP_DATA) /* Size of capability-specific per-instance data */
};

/* Start operator function - example customising a base class handler */
bool stub_capability_start(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *response_id, void **response_data)
{
    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }
    /*
     * Specific start operator code goes here
     */

    return TRUE;
}


/* Data processing function */
void stub_capability_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    /*
     * Capability processing code goes here
     */
}

