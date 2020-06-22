/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  base_op.c
 * \ingroup  operators
 *
 *  Base operator
 *
 */

#include "platform/pl_assert.h"
#include "pmalloc/pl_malloc.h"
#include "base_op.h"
#include "opmgr/opmgr_operator_data.h"
#include "stream/stream_for_ops.h"

/****************************************************************************
Private Function Declarations
*/

/****************************************************************************
Public Function Declarations
*/
/* ********************************** API functions ************************************* */

/* will allocate and create a success message, with zeroed error_code field */
bool base_op_build_std_response_ex(OPERATOR_DATA *op_data, STATUS_KYMERA status, void **response_data)
{
    OP_STD_RSP* resp = xzpnew(OP_STD_RSP);

    if (resp == NULL)
    {
        return FALSE;
    }

    resp->op_id = op_data->id;
    resp->resp_data.err_code = 0;
    resp->status = status;

    *response_data = resp;
    return TRUE;
}

bool base_op_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    return base_op_build_std_response_ex(op_data, STATUS_OK, response_data);
}


bool base_op_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    return base_op_build_std_response_ex(op_data, STATUS_OK, response_data);
    /* Most operators will do more checks e.g.
     *  are all needed inputs and outputs connected? */
}


bool base_op_stop(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    /* Mark the operator as stopped. */
    base_op_stop_operator(op_data);
    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    return base_op_build_std_response_ex(op_data, STATUS_OK, response_data);
}


/* Derived op will create its specific things after having called this */
bool base_op_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    return TRUE;
}


/* Derived operator must have freed up its more specific op data before calling the base_op destroy */
bool base_op_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    /* Set up the a default success response information. */
    return base_op_build_std_response_ex(op_data, STATUS_OK, response_data);
}


bool base_op_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    /* Set up the a default success response information. */
    return base_op_build_std_response_ex(op_data, STATUS_OK, response_data);
}


bool base_op_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    /* Set up the default success response information. */
    return base_op_build_std_response_ex(op_data, STATUS_OK, response_data);
}

bool base_op_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    OP_BUF_DETAILS_RSP* resp;
    uint32 sample_rate;
    unsigned buffer_size;
    TIME_INTERVAL kick_period;

    resp = xzpnew(OP_BUF_DETAILS_RSP);
    if (resp == NULL)
    {
        return FALSE;
    }

    sample_rate = stream_if_get_system_sampling_rate();
    kick_period = stream_if_get_system_kick_period();

    /* Max possible supported sample rate is 192kHz,
     * which allows up to about 22ms before this would overflow
     */
    PL_ASSERT(kick_period <= 20*MILLISECOND);
    buffer_size = (unsigned)(1 + (sample_rate * kick_period)/1000000);

    /* Set up the a default success response information */
    resp->op_id = op_data->id;
    resp->status = STATUS_OK;
    resp->can_override = FALSE;
    resp->needs_override = FALSE;
    resp->supplies_buffer = FALSE;
    resp->runs_in_place = FALSE;
    resp->b.buffer_size = buffer_size;

    *response_data = resp;
    return TRUE;
}


bool base_op_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    OP_SCHED_INFO_RSP* resp;

    resp = xzpnew(OP_SCHED_INFO_RSP);
    if (resp == NULL)
    {
        return FALSE;
    }

    *response_data = resp;
    return TRUE;
}

OP_SCHED_INFO_RSP* base_op_get_sched_info_ex(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id)
{
    OP_SCHED_INFO_RSP* resp;

    resp = xzpnew(OP_SCHED_INFO_RSP);
    if (resp == NULL)
    {
        return NULL;
    }

    resp->op_id = op_data->id;
    resp->status = STATUS_OK;

    return resp;
}

bool base_op_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    /* Set up the a default success response information */
    if (base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        ((OP_STD_RSP*)*response_data)->resp_data.data = AUDIO_DATA_FORMAT_FIXP;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void base_op_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    /* Do nothing */
    return;
}

void* base_op_get_instance_data(OPERATOR_DATA *op_data)
{
    return op_data->extra_op_data;
}

CAP_ID base_op_get_cap_id(OPERATOR_DATA *op_data)
{
    return op_data->cap_data->id;
}

INT_OP_ID base_op_get_int_op_id(OPERATOR_DATA *op_data)
{
    return op_data->id;
}

EXT_OP_ID base_op_get_ext_op_id(OPERATOR_DATA *op_data)
{
    return INT_TO_EXT_OPID(op_data->id);
}

OPERATOR_DATA *base_op_clone_operator_data(OPERATOR_DATA *op_data)
{
    OPERATOR_DATA *clone;
    clone = xzpnew(OPERATOR_DATA);
    if (clone != NULL)
    {
        *clone = *op_data;
    }
    return clone;
}

#ifdef PROFILER_ON
void base_op_profiler_start(OPERATOR_DATA *op_data)
{
    if (op_data->profiler != NULL)
    {
        PROFILER_START(op_data->profiler);
    }
}

void base_op_profiler_stop(OPERATOR_DATA *op_data)
{
    if (op_data->profiler != NULL)
    {
        PROFILER_STOP(op_data->profiler);
    }
}

void base_op_profiler_add_kick(OPERATOR_DATA *op_data)
{
    if (op_data->profiler != NULL)
    {
        op_data->profiler->kick_inc++;
    }
}
#endif

void base_op_stop_operator(OPERATOR_DATA *op_data)
{
    op_data->state = OP_NOT_RUNNING;
}

void base_op_stop_kicks(OPERATOR_DATA *op_data, STOP_KICK dir)
{
    op_data->stop_chain_kicks = dir;
}

const CAPABILITY_DATA* base_op_get_cap_data(OPERATOR_DATA *op_data)
{
    return op_data->cap_data;
}

void* base_op_get_class_ext(OPERATOR_DATA *op_data)
{
    return op_data->cap_class_ext;
}

void base_op_set_class_ext(OPERATOR_DATA *op_data, void *class_data)
{
    op_data->cap_class_ext = class_data;
}

/* **************************** Common operator message handlers ******************************** */

/* Handler for getting cap version (default information: major and minor revision numbers. */
bool base_op_opmsg_get_capability_version(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    *resp_length = OPMSG_RSP_PAYLOAD_SIZE_RAW_DATA(BASE_OP_VERSION_LENGTH);
    *resp_data = (OP_OPMSG_RSP_PAYLOAD *)xzpmalloc(OPMSG_RSP_PAYLOAD_SIZE_RAW_DATA(BASE_OP_VERSION_LENGTH)*sizeof(unsigned));
    if (*resp_data == NULL)
    {
        return FALSE;
    }
    /* echo the opmsgID/keyID */
    (*resp_data)->msg_id = OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);

    /* some version numbers Hi and Lo, from static cap data */
    (*resp_data)->u.raw_data[0] = op_data->cap_data->version_msw;
    (*resp_data)->u.raw_data[1] = op_data->cap_data->version_lsw;

    return TRUE;
}


/* ******************************* Helper functions ************************************ */

/* check that terminal ID is valid for the given operator */
bool base_op_is_terminal_valid(OPERATOR_DATA* op_data, unsigned terminal_id)
{
    /* in a bit more readable form: if terminal is a source, check against max nr of sources, ditto for sinks */
    if((terminal_id & TERMINAL_SINK_MASK) == 0)
    {
        if((terminal_id & TERMINAL_NUM_MASK) >= op_data->cap_data->max_sources)
        {
            return FALSE;
        }
    }
    else
    {
        if((terminal_id & TERMINAL_NUM_MASK) >= op_data->cap_data->max_sinks)
        {
            return FALSE;
        }
    }

    return TRUE;
}
