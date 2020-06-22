/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  switched_passthrough_consumer.c
 * \ingroup  capabilities
 *
 *  Switch between consuming input data and passing though input data to
 *  the output.
 *
 */

#include "capabilities.h"
#include "switched_passthrough_consumer.h"
#include "fault/fault.h"

typedef enum _SPC_MODE
{
    /* No data is passed through all the others consumed. */
    SPC_MODE_CONSUME = 0,
    /* Sink 0 is passed through; all the others consumed. */
    SPC_MODE_PASSTHROUGH_0,
    SPC_MODE_PASSTHROUGH_1,
    SPC_MODE_PASSTHROUGH_2,
    SPC_MODE_PASSTHROUGH_3,
    SPC_MODE_PASSTHROUGH_4,
    SPC_MODE_PASSTHROUGH_5,
    SPC_MODE_PASSTHROUGH_6,
    SPC_MODE_PASSTHROUGH_7,
    /* Maximum number of passthrough ports */
    SPC_MODE_SIZE,

    /* Sink 0 & 1 are passed through; all the others consumed. */
    SPC_MODE_SPC1_TAGSYNC_DUAL = 16,
    /* Sink 0 is passed through; sink 1 is tag synced consumed. */
    SPC_MODE_SPC1_TAGSYNC_0,
    /* Sink 1 is passed through; sink 0 is tag synced consumed. */
    SPC_MODE_SPC1_TAGSYNC_1,

    /* Sink 0 & 1 are passed through; all the others consumed. */
    SPC_MODE_SPC2_TAGSYNC_DUAL = 32,
    /* Sink 0 is passed through; sink 1 is tag synced consumed. */
    SPC_MODE_SPC2_TAGSYNC_0,
    /* Sink 1 is passed through; sink 0 is tag synced consumed. */
    SPC_MODE_SPC2_TAGSYNC_1

}SPC_MODE;

typedef enum _TRANSITION
{
    transition_none          = 0,
    transition_dual_2_sync_0 = 1,
    transition_dual_2_sync_1 = 2,
    transition_sync_0_2_dual = 3,
    transition_sync_1_2_dual = 4,

} TRANSITION;

#define SPC_NUMBER_INPUTS           (SPC_MODE_SIZE - 1)
#define SPC_NUMBER_OUTPUTS          (2)

typedef struct SPC_OP_DATA
{
    /** Consume data from input buffer. */
    tCbuffer*   ip_buffers[SPC_NUMBER_INPUTS];

    /** Passthrough data to output buffer. */
    tCbuffer*   op_buffers[SPC_NUMBER_OUTPUTS];

    /** Current mode */
    SPC_MODE      current_mode;

    /** Next mode */
    SPC_MODE      next_mode;

    /** The type of data consumed/passed by the capability.*/
    AUDIO_DATA_FORMAT data_format;

    /** The requested size of the output buffer, or 0 for the default size. */
    unsigned output_buffer_size;

    TRANSITION transition;
    unsigned wait_kicks;
    unsigned opbuf_len[SPC_NUMBER_OUTPUTS];
    unsigned frame_size;

} SPC_OP_DATA;

/* A structure containing a set of cbuffer processing function pointers */
struct _cbuffer_functions
{
    unsigned (*space)(tCbuffer *cbuffer);
    unsigned (*data)(tCbuffer *cbuffer);
    unsigned (*copy)(tCbuffer *dst, tCbuffer *src, unsigned to_copy);
    void (*advance_rd_ptr)(tCbuffer *cbuffer, unsigned to_advance);
    void (*advance_wr_ptr)(tCbuffer *cbuffer, unsigned to_advance);
};

/****************************************************************************
Private Function Definitions
*/
static void spc_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);
static bool spc_op_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
static bool spc_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
static bool spc_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
static bool spc_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
static bool spc_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
static bool spc_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
static bool spc_opmsg_transition_request(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
static bool spc_opmsg_set_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
static bool spc_opmsg_set_buffer_size(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
static bool spc_opmsg_select_passthrough_request(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
static bool spc_opmsg_set_block_size(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

static bool spc_connect_disconnect_common(OPERATOR_DATA *op_data, unsigned terminal_id, tCbuffer *buffer);

/****************************************************************************
Private Constant Declarations
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define SWITCHED_PASSTHROUGH_CONSUMER_ID CAP_ID_DOWNLOAD_SWITCHED_PASSTHROUGH_CONSUMER
#else
#define SWITCHED_PASSTHROUGH_CONSUMER_ID CAP_ID_SWITCHED_PASSTHROUGH_CONSUMER
#endif

#define SPC_DEFAULT_BLOCK_SIZE 1

/** The stub capability function handler table */
const handler_lookup_struct spc_handler_table =
{
    spc_op_create,       /* OPCMD_CREATE */
    base_op_destroy,      /* OPCMD_DESTROY */
    base_op_start,        /* OPCMD_START */
    base_op_stop,         /* OPCMD_STOP */
    base_op_reset,        /* OPCMD_RESET */
    spc_connect,          /* OPCMD_CONNECT */
    spc_disconnect,       /* OPCMD_DISCONNECT */
    spc_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    spc_get_data_format,  /* OPCMD_DATA_FORMAT */
    spc_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry spc_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_SPC_ID_TRANSITION, spc_opmsg_transition_request},
    {OPMSG_SPC_ID_SET_DATA_FORMAT, spc_opmsg_set_data_format},
    {OPMSG_SPC_ID_SELECT_PASSTHROUGH, spc_opmsg_select_passthrough_request},
    {OPMSG_COMMON_ID_SET_BUFFER_SIZE, spc_opmsg_set_buffer_size},
    {OPMSG_SPC_ID_SET_BLOCK_SIZE, spc_opmsg_set_block_size},
    {0, NULL}
};


/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA switched_passthrough_consumer_cap_data =
{
    SWITCHED_PASSTHROUGH_CONSUMER_ID,             /* Capability ID */
    2, 1,                               /* Version information - hi and lo parts */
    SPC_NUMBER_INPUTS,                  /* Max number of sinks/inputs */
    SPC_NUMBER_OUTPUTS,                 /* Max number sources/outputs */
    &spc_handler_table,                 /* Pointer to message handler function table */
    spc_opmsg_handler_table,            /* Pointer to operator message handler function table */
    spc_process_data,                   /* Pointer to data processing function */
    0,                                  /* Reserved */
    sizeof(SPC_OP_DATA)                 /* Size of capability-specific per-instance data */
};

/* Standard cbuffer function pointers */
const struct _cbuffer_functions cbuffer_functions = {
    cbuffer_calc_amount_space_in_words,
    cbuffer_calc_amount_data_in_words,
    cbuffer_copy,
    cbuffer_advance_read_ptr,
    cbuffer_advance_write_ptr,
};

/* Octet access (_ex) cbuffer function pointers */
const struct _cbuffer_functions cbuffer_ex_functions = {
    cbuffer_calc_amount_space_ex,
    cbuffer_calc_amount_data_ex,
    cbuffer_copy_ex,
    cbuffer_advance_read_ptr_ex,
    cbuffer_advance_write_ptr_ex,
};

#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_SWITCHED_PASSTHROUGH_CONSUMER, SPC_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_SWITCHED_PASSTHROUGH_CONSUMER, SPC_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

static inline SPC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SPC_OP_DATA *) base_op_get_instance_data(op_data);
}

static bool MAP_MODE_TO_PORT(SPC_MODE mode)
{
    if (mode < SPC_MODE_SIZE)
    {
        return (mode - SPC_MODE_PASSTHROUGH_0);
    }
    switch (mode)
    {
        case SPC_MODE_SPC1_TAGSYNC_0 :
        case SPC_MODE_SPC2_TAGSYNC_0 :
        {
            return 0;
        }
        case SPC_MODE_SPC1_TAGSYNC_1 :
        case SPC_MODE_SPC2_TAGSYNC_1 :
        {
            return 1;
        }
        default:
        {
            return SPC_MODE_SIZE;
        }
    }
}

static bool is_valid_input_number(SPC_MODE mode, unsigned terminal_id)
{
    PL_ASSERT(mode > SPC_MODE_CONSUME);

    if ((mode == SPC_MODE_SPC1_TAGSYNC_DUAL) ||
        (mode == SPC_MODE_SPC2_TAGSYNC_DUAL))
    {
        if ((terminal_id == 0) ||
            (terminal_id == 1)    )
        {
            return TRUE;
        }
    }
    else
    {
        unsigned port = MAP_MODE_TO_PORT(mode);    
        if (terminal_id == port)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static inline SPC_MODE adjust_mode_to_buffers(SPC_MODE mode, tCbuffer* buffers[], bool inout)
{
    SPC_MODE map_mode = mode;

    switch (mode)
    {
        case SPC_MODE_SPC1_TAGSYNC_DUAL:
        {
            if ((buffers[0] == NULL) &&
                (buffers[1] == NULL)    )
            {
                map_mode = SPC_MODE_CONSUME;
            }
            else
            if (buffers[0] == NULL)
            {
                map_mode = SPC_MODE_SPC1_TAGSYNC_1;
            }
            else
            if (buffers[1] == NULL)
            {
                map_mode = SPC_MODE_SPC1_TAGSYNC_0;
            }
            break;
        }
        case SPC_MODE_SPC2_TAGSYNC_DUAL:
        {
            if ((buffers[0] == NULL) &&
                (buffers[1] == NULL)    )
            {
                map_mode = SPC_MODE_CONSUME;
            }
            else
            if (buffers[0] == NULL)
            {
                map_mode = SPC_MODE_SPC2_TAGSYNC_1;
            }
            else
            if (buffers[1] == NULL)
            {
                map_mode = SPC_MODE_SPC2_TAGSYNC_0;
            }
            break;
        }
        case SPC_MODE_SPC1_TAGSYNC_0:
        case SPC_MODE_SPC2_TAGSYNC_0:
        {
            if (buffers[0] == NULL)
            {
                map_mode = SPC_MODE_CONSUME;
            }
            break;
        }
        case SPC_MODE_SPC1_TAGSYNC_1:
        case SPC_MODE_SPC2_TAGSYNC_1:
        {
            if (buffers[1] == NULL)
            {
                map_mode = SPC_MODE_CONSUME;
            }
            break;
        }
        default:
        {
            unsigned port = 0;
            if (inout)
            {
                port = MAP_MODE_TO_PORT(mode);
            }
            if ((port >= SPC_MODE_SIZE) ||
                (buffers[port] == NULL)    )
            {
                map_mode = SPC_MODE_CONSUME;
            }
            break;
        }
    }
    return map_mode;
}

static inline bool is_sync_tag_mode(SPC_MODE mode, unsigned *out0, unsigned *out1)
{
    switch (mode)
    {
        case SPC_MODE_SPC1_TAGSYNC_0:
            *out0 = 0;
            *out1 = 0;
            return TRUE;
        case SPC_MODE_SPC1_TAGSYNC_1:
            *out0 = 1;
            *out1 = 1;
            return TRUE;
        case SPC_MODE_SPC1_TAGSYNC_DUAL:
            *out0 = 0;
            *out1 = 1;
            return TRUE;
        default:
            return FALSE;
    }
}

static inline bool is_sync_spc2_tag_mode(SPC_MODE mode, unsigned *out0, unsigned *out1)
{
    switch (mode)
    {
        case SPC_MODE_SPC2_TAGSYNC_0:
            *out0 = 0;
            *out1 = 0;
            return TRUE;
        case SPC_MODE_SPC2_TAGSYNC_1:
            *out0 = 1;
            *out1 = 1;
            return TRUE;
        case SPC_MODE_SPC2_TAGSYNC_DUAL:
            *out0 = 0;
            *out1 = 1;
            return TRUE;
        default:
            return FALSE;
    }
}

static unsigned mtag_len(SPC_OP_DATA *opx_data, unsigned in, unsigned out, bool *passthrough)
{
    const struct _cbuffer_functions *cbuffer;
    unsigned data_size;
    tCbuffer *op_buffer;
    tCbuffer *ip_buffer;
    unsigned buffer_size;
    unsigned data_to_handle, complete_data = 0;
    unsigned input_data;

    /* Select which set of cbuffer functions to use. The normal cbuffer functions
       are used to process PCM data. The _ex cbuffer functions are used to process
       other data types (which may contain frames with an odd number of octets). */
    switch (opx_data->data_format)
    {
        case AUDIO_DATA_FORMAT_FIXP:
        case AUDIO_DATA_FORMAT_FIXP_WITH_METADATA: 
            data_size = OCTETS_PER_SAMPLE;
            cbuffer = &cbuffer_functions;
            break; 
        default:
            data_size = 1;
            cbuffer = &cbuffer_ex_functions;
            break;
    }

    ip_buffer = opx_data->ip_buffers[in];
    op_buffer = opx_data->op_buffers[out];

    buffer_size = cbuffer_get_size_in_octets(ip_buffer);
    input_data = cbuffer->data(ip_buffer);
    complete_data = 0;
    data_to_handle = input_data;
    if (opx_data->current_mode > SPC_MODE_CONSUME &&
            (is_valid_input_number(opx_data->current_mode, in) == TRUE) &&
            (op_buffer != NULL))
    {
        unsigned output_space = cbuffer->space(op_buffer);
        unsigned out_buf_size = cbuffer_get_size_in_octets(op_buffer);
        if (buffer_size > out_buf_size)
        {
            buffer_size = out_buf_size;
        }

        *passthrough = TRUE;
        if (output_space < input_data)
        {
            L3_DBG_MSG3("SPC: ### [input%d] input data %d didn't fit in space %d",
                                        in, input_data, output_space);
            data_to_handle = output_space;
        }
    }

    if (buff_has_metadata(ip_buffer))
    {
         metadata_tag * mtag = buff_metadata_peek(ip_buffer);

        /* Scan all the available tags and compute the amount of data
         * in the input buffer that corresponds to the complete tags.
         * Only this data will be processed, together with the
         * associated metadata. This way we will always output
         * data aligned to metadata and can switch mode at any time.
         */
        while (mtag != NULL)
        {
            if (mtag->length > buffer_size)
            {
                /* This tag is bigger than either the input or output buffer.
                 * This is not going to work: warn the user to increase
                 * buffer sizes.
                 */
                fault_diatribe(FAULT_AUDIO_SPC_TAG_BIGGER_THAN_BUFFER, mtag->length);

                L2_DBG_MSG4("SPC: [input%d] Tag is too big: %d. "
                            "buffer sizes: in %d, out %d",
                            in, mtag->length, ip_buffer->size, op_buffer->size);

                /* For the time being, just go on, we might be lucky and
                 * keep working in the current mode.
                 */
                complete_data = buffer_size;
                break;
            }
            else
            {
                unsigned tmp = complete_data + mtag->length/data_size;
                if (tmp > data_to_handle)
                {
                   /* This is the last tag, for which we haven't received all
                    * the data yet. We will not consider any of its data.
                    */
                    break;
                }

                mtag = mtag->next;
                complete_data = tmp;
            }
        }
    }
    else
    {
        /* No metadata. Process all available data. */
        complete_data = input_data;
    }

    return complete_data;
}

static unsigned outputdata_len(SPC_OP_DATA *opx_data, unsigned out)
{
    const struct _cbuffer_functions *cbuffer;
    tCbuffer *op_buffer;
    unsigned data_in_buffer = 0;

    /* Select which set of cbuffer functions to use. The normal cbuffer functions
       are used to process PCM data. The _ex cbuffer functions are used to process
       other data types (which may contain frames with an odd number of octets). */
    switch (opx_data->data_format)
    {
        case AUDIO_DATA_FORMAT_FIXP:
        case AUDIO_DATA_FORMAT_FIXP_WITH_METADATA: 
            cbuffer = &cbuffer_functions;
            break; 
        default:
            cbuffer = &cbuffer_ex_functions;
            break;
    }

    op_buffer = opx_data->op_buffers[out];
    if (opx_data->current_mode > SPC_MODE_CONSUME && (op_buffer != NULL))
    {
        data_in_buffer = cbuffer->data(op_buffer);
    }
    return data_in_buffer;
}

static unsigned bufferdata_len(SPC_OP_DATA *opx_data, unsigned in, unsigned out, bool *passthrough)
{
    const struct _cbuffer_functions *cbuffer;
    tCbuffer *op_buffer;
    tCbuffer *ip_buffer;
    unsigned data_to_handle, complete_data = 0;
    unsigned input_data;

    /* Select which set of cbuffer functions to use. The normal cbuffer functions
       are used to process PCM data. The _ex cbuffer functions are used to process
       other data types (which may contain frames with an odd number of octets). */
    switch (opx_data->data_format)
    {
        case AUDIO_DATA_FORMAT_FIXP:
        case AUDIO_DATA_FORMAT_FIXP_WITH_METADATA: 
            cbuffer = &cbuffer_functions;
            break; 
        default:
            cbuffer = &cbuffer_ex_functions;
            break;
    }

    ip_buffer = opx_data->ip_buffers[in];
    op_buffer = opx_data->op_buffers[out];

    input_data = cbuffer->data(ip_buffer);
    complete_data = 0;
    data_to_handle = input_data;
    if (opx_data->current_mode > SPC_MODE_CONSUME &&
            (is_valid_input_number(opx_data->current_mode, in) == TRUE) &&
            (op_buffer != NULL))
    {
        unsigned output_space = cbuffer->space(op_buffer);

        *passthrough = TRUE;
        if (output_space < input_data)
        {
            L3_DBG_MSG3("SPC: ### [input%d] input data %d didn't fit in space %d",
                                        in, input_data, output_space);
            data_to_handle = output_space;
        }
    }
    else if (opx_data->current_mode > SPC_MODE_CONSUME &&
             (op_buffer != NULL))
    {
        unsigned output_space = cbuffer->space(op_buffer);

        data_to_handle = output_space;
    }

    /* No metadata. Process all available data. */
    complete_data = data_to_handle;

    return complete_data;
}

static unsigned process_data(SPC_OP_DATA *opx_data, unsigned complete_data, unsigned in, unsigned out, bool passthrough, TOUCHED_TERMINALS *touched)
{
    const struct _cbuffer_functions *cbuffer;
    unsigned data_size;
    tCbuffer *op_buffer;
    tCbuffer *ip_buffer;
    unsigned touched_snk_mask = 1 << in;
    unsigned touched_src_mask = 1 << out;
    unsigned input_data;

    /* Select which set of cbuffer functions to use. The normal cbuffer functions
       are used to process PCM data. The _ex cbuffer functions are used to process
       other data types (which may contain frames with an odd number of octets). */
    switch (opx_data->data_format)
    {
        case AUDIO_DATA_FORMAT_FIXP:
        case AUDIO_DATA_FORMAT_FIXP_WITH_METADATA: 
            data_size = OCTETS_PER_SAMPLE;
            cbuffer = &cbuffer_functions;
            break; 
        default:
            data_size = 1;
            cbuffer = &cbuffer_ex_functions;
            break;
    }

    ip_buffer  = opx_data->ip_buffers[in];
    op_buffer  = opx_data->op_buffers[out];
    input_data = cbuffer->data(ip_buffer);

    if (passthrough)
    {
        unsigned copied = cbuffer->copy(op_buffer, ip_buffer, complete_data);

        PL_ASSERT(op_buffer != NULL);

        L3_DBG_MSG3("SPC#1 passed through [input%d] %d of %d",
                    in, copied, input_data);
        touched->sources |= touched_src_mask;
        metadata_strict_transport(ip_buffer, op_buffer, copied * data_size);
        if (copied != complete_data)
        {
            L2_DBG_MSG3("SPC#1 error on [input%d] %d of %d",
                        in, copied, input_data);
        }
        touched->sinks |= touched_snk_mask;
        return copied;
    }
    else
    {
        metadata_tag *mtag;
        unsigned b4idx, afteridx;

        L3_DBG_MSG3("SPC#1 discard [input%d] %d of %d",
                    in, complete_data, input_data);

        cbuffer->advance_rd_ptr(ip_buffer, complete_data);
        mtag = buff_metadata_remove(ip_buffer, complete_data * data_size, &b4idx, &afteridx);
        buff_metadata_tag_list_delete(mtag);
        touched->sinks |= touched_snk_mask;
        return complete_data;
    }
}

static unsigned process_data_spc2(SPC_OP_DATA *opx_data, unsigned complete_data, unsigned in, unsigned out, bool passthrough, bool transition, TOUCHED_TERMINALS *touched)
{
    const struct _cbuffer_functions *cbuffer;
    unsigned data_size;
    tCbuffer *op_buffer;
    tCbuffer *ip_buffer;
    unsigned touched_snk_mask = 1 << in;
    unsigned touched_src_mask = 1 << out;
    unsigned input_data;

    /* Select which set of cbuffer functions to use. The normal cbuffer functions
       are used to process PCM data. The _ex cbuffer functions are used to process
       other data types (which may contain frames with an odd number of octets). */
    switch (opx_data->data_format)
    {
        case AUDIO_DATA_FORMAT_FIXP:
        case AUDIO_DATA_FORMAT_FIXP_WITH_METADATA: 
            data_size = OCTETS_PER_SAMPLE;
            cbuffer = &cbuffer_functions;
            break; 
        default:
            data_size = 1;
            cbuffer = &cbuffer_ex_functions;
            break;
    }

    ip_buffer  = opx_data->ip_buffers[in];
    op_buffer  = opx_data->op_buffers[out];
    input_data = cbuffer->data(ip_buffer);

    if (passthrough)
    {
        unsigned copied = cbuffer->copy(op_buffer, ip_buffer, complete_data);

        PL_ASSERT(op_buffer != NULL);

        L3_DBG_MSG3("SPC#2 passed through [input%d] %d of %d",
                    in, copied, input_data);

        touched->sources |= touched_src_mask;
        metadata_strict_transport(ip_buffer, op_buffer, copied * data_size);
        if (copied != complete_data)
        {
            L2_DBG_MSG3("SPC#2 error on [input%d] %d of %d",
                        in, copied, input_data);
        }
        touched->sinks |= touched_snk_mask;
        return copied;
    }
    else
    {
        L3_DBG_MSG3("SPC#2 discard [input%d] %d of %d",
                    in, complete_data, input_data);

        cbuffer->advance_wr_ptr(op_buffer, complete_data);
        touched->sources |= touched_src_mask;

        /* Drain unused Aptx buffer */
        if (transition && (ip_buffer != NULL) && (cbuffer->data(ip_buffer) > 0))
        {
            if (cbuffer->data(ip_buffer) < complete_data)
            {
                complete_data = cbuffer->data(ip_buffer);
            }

            metadata_tag *mtag;
            unsigned b4idx, afteridx;

            cbuffer->advance_rd_ptr(ip_buffer, complete_data);
            mtag = buff_metadata_remove(ip_buffer, complete_data * data_size, &b4idx, &afteridx);

            buff_metadata_tag_list_delete(mtag);
            touched->sinks |= touched_snk_mask;
        }

        return complete_data;
    }
}

static bool spc1_transition(SPC_OP_DATA *opx_data)
{
    if ((opx_data->current_mode == SPC_MODE_SPC1_TAGSYNC_DUAL) &&
        (opx_data->next_mode    == SPC_MODE_SPC1_TAGSYNC_0) )
    {
        opx_data->transition = transition_dual_2_sync_0;
    }
    else
    if ((opx_data->current_mode == SPC_MODE_SPC1_TAGSYNC_DUAL) &&
        (opx_data->next_mode    == SPC_MODE_SPC1_TAGSYNC_1) )
    {
        opx_data->transition = transition_dual_2_sync_1;
    }
    else
    if ((opx_data->current_mode == SPC_MODE_SPC1_TAGSYNC_0) &&
        (opx_data->next_mode    == SPC_MODE_SPC1_TAGSYNC_DUAL) )
    {
        opx_data->transition = transition_sync_0_2_dual;
    }
    else
    if ((opx_data->current_mode == SPC_MODE_SPC1_TAGSYNC_1) &&
        (opx_data->next_mode    == SPC_MODE_SPC1_TAGSYNC_DUAL) )
    {
        opx_data->transition = transition_sync_1_2_dual;
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

static bool spc2_transition(SPC_OP_DATA *opx_data)
{
    if ((opx_data->current_mode == SPC_MODE_SPC2_TAGSYNC_0) &&
        (opx_data->next_mode    == SPC_MODE_SPC2_TAGSYNC_DUAL) )
    {
        opx_data->transition = transition_sync_0_2_dual;
    }
    else
    if ((opx_data->current_mode == SPC_MODE_SPC2_TAGSYNC_1) &&
        (opx_data->next_mode    == SPC_MODE_SPC2_TAGSYNC_DUAL) )
    {
        opx_data->transition = transition_sync_1_2_dual;
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}


static void spc1_reset_transition_state(SPC_OP_DATA *opx_data)
{
    opx_data->opbuf_len[0] = 0;
    opx_data->opbuf_len[1] = 0;
    opx_data->wait_kicks   = 0;
    opx_data->transition   = transition_none;
}

static bool spc1_is_empty_opbuffer(SPC_OP_DATA *opx_data, TOUCHED_TERMINALS *touched)
{
    unsigned len[SPC_NUMBER_OUTPUTS];
    bool stalled = FALSE;

    len[0] = outputdata_len(opx_data, 0);
    len[1] = outputdata_len(opx_data, 1);

    if ((len[0] == opx_data->opbuf_len[0]) &&
        (len[1] == opx_data->opbuf_len[1])   )
    {
        stalled = TRUE;
    }

    opx_data->opbuf_len[0] = len[0];
    opx_data->opbuf_len[1] = len[1];

    if (stalled)
    {
        stalled = FALSE;
        switch (opx_data->transition)
        {
            case transition_sync_0_2_dual :
            {
                if (len[0] <= opx_data->frame_size) stalled = TRUE;
                break;
            }
            case transition_sync_1_2_dual :
            {
                if (len[1] <= opx_data->frame_size) stalled = TRUE;
                break;
            }
            default:
            {
                if ((len[0] == opx_data->opbuf_len[0]) &&
                    (len[1] == opx_data->opbuf_len[1])   )
                {
                    stalled = TRUE;
                }
                break;
            }
        }
    }

    touched->sinks = 3;
    return stalled;
}

static bool spc2_is_empty_ipbuffer(SPC_OP_DATA *opx_data, TOUCHED_TERMINALS *touched)
{
    const struct _cbuffer_functions *cbuffer;
    unsigned data0 = 0, data1 = 0;

    /* Select which set of cbuffer functions to use. The normal cbuffer functions
       are used to process PCM data. The _ex cbuffer functions are used to process
       other data types (which may contain frames with an odd number of octets). */
    switch (opx_data->data_format)
    {
        case AUDIO_DATA_FORMAT_FIXP:
        case AUDIO_DATA_FORMAT_FIXP_WITH_METADATA: 
            cbuffer = &cbuffer_functions;
            break; 
        default:
            cbuffer = &cbuffer_ex_functions;
            break;
    }

    if (opx_data->ip_buffers[0] != NULL) 
    {
        data0 = cbuffer->data(opx_data->ip_buffers[0]);
    }
    if (opx_data->ip_buffers[1] != NULL) 
    {
        data1 = cbuffer->data(opx_data->ip_buffers[1]);
    }

    return ((data0 != 0) && (data1 != 0));
}


/* Data processing function */
static void spc_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    SPC_OP_DATA *opx_data = get_instance_data(op_data);
    unsigned input_num;
    unsigned out0, out1;
    unsigned aptx0=0, aptx1=0;
    bool mode_switch_ok = TRUE;

    if (opx_data->current_mode != opx_data->next_mode)
    {
        bool transition_at_next_kick = FALSE;

        if (spc1_transition(opx_data))
        {
            opx_data->wait_kicks++;
            if (spc1_is_empty_opbuffer(opx_data, touched))
            {
                transition_at_next_kick = TRUE;
            }
            else
            {
                return;
            }
        }

        if (spc2_transition(opx_data))
        {
            mode_switch_ok = spc2_is_empty_ipbuffer(opx_data, touched);
        }

        if (mode_switch_ok)
        {
            /* We need to transition. Either between inputs, or between an 
               input and consume all */
            opx_data->current_mode = opx_data->next_mode;

            L2_DBG_MSG2("*** SPC transition success. Current now %d [wait %d].",
                             opx_data->current_mode, opx_data->wait_kicks);

            if (transition_at_next_kick) return;

            spc1_reset_transition_state(opx_data);
        }
    }

    if (opx_data->current_mode > SPC_MODE_CONSUME)
    {
        SPC_MODE mode = opx_data->current_mode;
        mode = adjust_mode_to_buffers(mode, opx_data->ip_buffers, 1);
        mode = adjust_mode_to_buffers(mode, opx_data->op_buffers, 0);
        opx_data->current_mode = opx_data->next_mode = mode;
    }

    if (is_sync_spc2_tag_mode(opx_data->current_mode, &out0, &out1))
    {
        unsigned complete_data, complete_data0, complete_data1;
        bool passthrough0 = FALSE, passthrough1 = FALSE, transition = FALSE;

        complete_data0 = bufferdata_len(opx_data, 0, out0, &passthrough0);
        complete_data1 = bufferdata_len(opx_data, 1, out1, &passthrough1);
        if (complete_data0 > complete_data1) complete_data = complete_data1;
        else complete_data = complete_data0;
        if (complete_data != 0)
        {
            if (!passthrough0 || !passthrough1) transition = 1;
            aptx0 = process_data_spc2(opx_data, complete_data, 0, 0, passthrough0, transition, touched);
            aptx1 = process_data_spc2(opx_data, complete_data, 1, 1, passthrough1, transition, touched);
            if (aptx0 != aptx1)
            {
                L2_DBG_MSG3("SPC#2: possible sync loss (processed %d on 0, %d on 1, mode %d", aptx0, aptx1, opx_data->current_mode);
            }
        }
    }
    else
    if (is_sync_tag_mode(opx_data->current_mode, &out0, &out1))
    {
        unsigned complete_data, complete_data0, complete_data1, blocks;
        bool passthrough0 = FALSE, passthrough1 = FALSE;

        complete_data0 = mtag_len(opx_data, 0, out0, &passthrough0);
        complete_data1 = mtag_len(opx_data, 1, out1, &passthrough1);
        if (complete_data0 > complete_data1) complete_data = complete_data1;
        else complete_data = complete_data0;
        blocks = complete_data / opx_data->frame_size;
        complete_data = blocks * opx_data->frame_size;
        if (complete_data != 0)
        {
            aptx0 = process_data(opx_data, complete_data, 0, 0, passthrough0, touched);
            aptx1 = process_data(opx_data, complete_data, 1, 1, passthrough1, touched);
            if (aptx0 != aptx1)
            {
                L2_DBG_MSG3("SPC#1: possible sync loss (processed %d on 0, %d on 1, mode %d", aptx0, aptx1, opx_data->current_mode);
            }
        }
    }
    else
    {
        /* Now process all inputs, consuming or passing through as relevant */
        for (input_num = 0; input_num < SPC_NUMBER_INPUTS; input_num++)
        {
            unsigned complete_data;
            bool passthrough = FALSE;

            if (!opx_data->ip_buffers[input_num])
            {
                /* This input is disconnected. */
                /* Nothing more to do with this */
                continue;
            }

            complete_data = mtag_len(opx_data, input_num, 0, &passthrough);
            if (complete_data != 0)
            {
                process_data(opx_data, complete_data, input_num, 0, passthrough, touched);
            }
        }
    }
}

static bool spc_connect_disconnect_common(OPERATOR_DATA *op_data, unsigned terminal_id, tCbuffer *buffer)
{
    SPC_OP_DATA *opx_data = get_instance_data(op_data);
    bool is_sink = terminal_id & TERMINAL_SINK_MASK;

    if (is_sink)
    {
        terminal_id = terminal_id & ~TERMINAL_SINK_MASK;
        if (terminal_id >= SPC_NUMBER_INPUTS)
        {
            /* Invalid input terminal id */
            return FALSE;
        }
        if (buffer == NULL)
        {
            /* An input terminal is being disconnected.
             */

            if (opx_data->next_mode > SPC_MODE_CONSUME &&
                (is_valid_input_number(opx_data->next_mode, terminal_id) == TRUE))
            {
                /* Input is passthrough (or will soon be)
                 * This will be handled in process_data
                 */
                L2_DBG_MSG("SPC Disconnection of Passthrough input.");
            }
        }
        else
        {
            /* An input terminal is being connected. */
        }

        /* Apply the connection/disconnection */
        opx_data->ip_buffers[terminal_id] = buffer;
    }
    else
    {
        terminal_id = terminal_id & ~TERMINAL_SINK_MASK;
        if (terminal_id >= SPC_NUMBER_OUTPUTS)
        {
            /* Invalid input terminal id */
            return FALSE;
        }
        if (buffer == NULL)
        {
            /* The output terminal is being disconnected.
             */
            if (opx_data->next_mode != SPC_MODE_CONSUME)
            {
                /* Operator is in passthrough mode (or will soon be).
                 * Cannot passthrough without an output.
                 * This will be handled in process_data
                 */
                L2_DBG_MSG("SPC Disconnection of output during Passthrough.");
                /* If the operator doesn't get kicked (input buffers can be
                 * full) we need to wait for the application to set a mode switch.
                 * That will also kick the operator and consume the inputs.
                 * Add a kick here to remove dependency from apps.
                 */
                opmgr_kick_operator(op_data);
            }
        }
        else
        {
            /* The output terminal is being connected. */
            if (buff_has_metadata(buffer))
            {
                unsigned usable_octets = get_octets_per_word(opx_data->data_format);
                cbuffer_set_usable_octets(buffer, usable_octets);
            }
        }

        /* Apply the connection/disconnection */
        opx_data->op_buffers[terminal_id] = buffer;
    }

    return TRUE;
}

static bool spc_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    unsigned terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    tCbuffer *buffer = OPMGR_GET_OP_CONNECT_BUFFER(message_data);

    if (!base_op_connect(op_data, message_data, response_id, response_data))
    {
        /* Shouldn't change anything if there is not enough memory for the response.*/
        return FALSE;
    }

    if (!spc_connect_disconnect_common(op_data, terminal_id, buffer))
    {
         base_op_change_response_status(response_data, STATUS_CMD_FAILED);
         return TRUE;
    }

    return TRUE;
}

static bool spc_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    unsigned terminal_id = OPMGR_GET_OP_DISCONNECT_TERMINAL_ID(message_data);

    if (!base_op_disconnect(op_data, message_data, response_id, response_data))
    {
        /* Shouldn't change anything if there is not enough memory for the response.*/
        return FALSE;
    }

    if (!spc_connect_disconnect_common(op_data, terminal_id, NULL))
    {
         base_op_change_response_status(response_data, STATUS_CMD_FAILED);
         return TRUE;
    }

    return TRUE;
}

static bool spc_op_create(OPERATOR_DATA *op_data, void *message_data,
                          unsigned *response_id, void **response_data)
{
    SPC_OP_DATA *opx_data = get_instance_data(op_data);

    /* call base_op create, which also allocates and fills response message */
    if (!base_op_create(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    opx_data->frame_size = 64;

    return TRUE;
}

static bool spc_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SPC_OP_DATA *opx_data = get_instance_data(op_data);
    bool result = base_op_buffer_details(op_data, message_data, response_id, response_data);

    if (result)
    {
        unsigned terminal_id = OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data);
        OP_BUF_DETAILS_RSP *resp = *response_data;

        if (0 == (terminal_id & TERMINAL_SINK_MASK))
        {
            /* Output, use the output buffer size */
            resp->b.buffer_size = opx_data->output_buffer_size;
        }
        resp->metadata_buffer = NULL;
        resp->supports_metadata = TRUE;
    }
    return result;
}

static bool spc_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    OP_SCHED_INFO_RSP* resp;

    resp = base_op_get_sched_info_ex(op_data, message_data, response_id);
    if (resp == NULL)
    {
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }
    *response_data = resp;

    resp->block_size = SPC_DEFAULT_BLOCK_SIZE;

    return TRUE;
}

static bool spc_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SPC_OP_DATA *opx_data = get_instance_data(op_data);
    bool result = base_op_get_data_format(op_data, message_data, response_id, response_data);

    if (result)
    {
        ((OP_STD_RSP*)*response_data)->resp_data.data = opx_data->data_format;
    }
    return result;
}

static bool spc_opmsg_transition_request(OPERATOR_DATA *op_data, void *message_data, 
                                            unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SPC_OP_DATA *opx_data = get_instance_data(op_data);
    OPMSG_SPC_MODE new_mode = (OPMSG_SPC_MODE) OPMSG_FIELD_GET(message_data, OPMSG_SPC_CHANGE_MODE, NEW_MODE);

    if (new_mode == OPMSG_SPC_MODE_SPC2_DATASYNC_DUAL)
    {
        opx_data->next_mode = SPC_MODE_SPC2_TAGSYNC_DUAL;
        return TRUE;
    } 
    else if (new_mode == OPMSG_SPC_MODE_SPC2_DATASYNC_1)
    {
        opx_data->next_mode = SPC_MODE_SPC2_TAGSYNC_1;
        return TRUE;
    } 
    else if (new_mode == OPMSG_SPC_MODE_SPC2_DATASYNC_0)
    {
        opx_data->next_mode = SPC_MODE_SPC2_TAGSYNC_0;
        return TRUE;
    } 
    else if (new_mode == OPMSG_SPC_MODE_TAGSYNC_DUAL)
    {
        opx_data->next_mode = SPC_MODE_SPC1_TAGSYNC_DUAL;
        return TRUE;
    } 
    else if (new_mode == OPMSG_SPC_MODE_TAGSYNC_1)
    {
        opx_data->next_mode = SPC_MODE_SPC1_TAGSYNC_1;
        return TRUE;
    } 
    else if (new_mode == OPMSG_SPC_MODE_TAGSYNC_0)
    {
        opx_data->next_mode = SPC_MODE_SPC1_TAGSYNC_0;
        return TRUE;
    } 
    else if (new_mode == OPMSG_SPC_MODE_PASSTHROUGH1)
    {
        opx_data->next_mode = SPC_MODE_PASSTHROUGH_1;
        return TRUE;
    } 
    else if (new_mode == OPMSG_SPC_MODE_PASSTHROUGH)
    {
        opx_data->next_mode = SPC_MODE_PASSTHROUGH_0;
        return TRUE;
    } 
    else if (new_mode == OPMSG_SPC_MODE_CONSUMER)
    {
        opx_data->next_mode = SPC_MODE_CONSUME;

        if (opx_data->current_mode != opx_data->next_mode &&
                opmgr_op_is_running(op_data))
        {
            /* A switch may be needed if the input is full.
             * Try to kick the operator to perform the switch.
             */
            opmgr_kick_operator(op_data);
        }
        return TRUE;
    }

    return FALSE;

}

static bool spc_opmsg_select_passthrough_request(OPERATOR_DATA *op_data, void *message_data, 
                                                 unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SPC_OP_DATA *opx_data = get_instance_data(op_data);
    uint16 new_input = OPMSG_FIELD_GET(message_data, OPMSG_SPC_SELECT_PASSTHROUGH, NEW_INPUT);
    SPC_MODE new_mode;
    unsigned out0, out1;

    if ((new_input >= SPC_MODE_SIZE) &&
        (!is_sync_tag_mode((SPC_MODE)new_input, &out0, &out1)))
    {
        /* Invalid input */
        return FALSE;
    }

    new_mode = (SPC_MODE)new_input;

    if (opx_data->current_mode != opx_data->next_mode)
    {
        /* We need to wait until the previous mode switch is complete. */
        return FALSE;
    }

    if (new_mode > SPC_MODE_CONSUME)
    {
        SPC_MODE adjust_mode = opx_data->current_mode;

        /* Verify that the mode matches input buffers used in that mode.
         * Return mode or amended mode based on available input buffers. */
        adjust_mode = adjust_mode_to_buffers(adjust_mode, opx_data->ip_buffers, 1);
        /* Verify that the mode matches output buffers used in that mode.
         * Return mode or amended mode based on available output buffers. */
        adjust_mode = adjust_mode_to_buffers(adjust_mode, opx_data->op_buffers, 0);
        /* Switching to passthrough */
        if (adjust_mode != new_mode)
        {
            /* Cannot passthrough if selected input or the output
             * is not connected
             */
            return FALSE;
        }
    }

    opx_data->next_mode = new_mode;

    if (opx_data->current_mode != opx_data->next_mode &&
            opmgr_op_is_running(op_data))
    {
        /* A switch is needed. Try to kick the operator to perform the switch. */
        opmgr_kick_operator(op_data);
    }
    return TRUE;
}

static bool spc_opmsg_set_block_size(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SPC_OP_DATA *opx_data = get_instance_data(op_data);
    opx_data->frame_size = OPMSG_FIELD_GET(message_data, OPMSG_SPC_SET_BLOCK_SIZE, BLOCK_SIZE);
    return TRUE;
}

static bool spc_opmsg_set_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SPC_OP_DATA *opx_data = get_instance_data(op_data);
    opx_data->data_format = OPMSG_FIELD_GET(message_data, OPMSG_SPC_SET_DATA_FORMAT, DATA_TYPE);
    return TRUE;
}

static bool spc_opmsg_set_buffer_size(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SPC_OP_DATA *opx_data = get_instance_data(op_data);
    opx_data->output_buffer_size = OPMSG_FIELD_GET(message_data, OPMSG_COMMON_SET_BUFFER_SIZE, BUFFER_SIZE);
    return TRUE;
}
