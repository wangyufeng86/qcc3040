/****************************************************************************
 * Copyright (c) 2017 Qualcomm Technologies International, Ltd.
 ****************************************************************************/
/**
 * \file  rate_adjust.c
 * \ingroup  capabilities
 *
 * capability wrapper for sw rate adjustment
 *
 */

/****************************************************************************
Include Files
*/

#include "capabilities.h"
#include "rate_adjust.h"
#include "common_conversions.h"
#include "rate_adjust_private.h"
#include "mem_utils/shared_memory_ids.h"
#include "rate_adjust_gen_c.h"

/** The rate adjust capability function handler table */
const handler_lookup_struct rate_adjust_handler_table =
{
    rate_adjust_create,           /* OPCMD_CREATE */
    rate_adjust_destroy,          /* OPCMD_DESTROY */
    rate_adjust_start,            /* OPCMD_START */
    rate_adjust_stop_reset,       /* OPCMD_STOP */
    rate_adjust_stop_reset,       /* OPCMD_RESET */
    multi_channel_connect,        /* OPCMD_CONNECT */
    multi_channel_disconnect,     /* OPCMD_DISCONNECT */
    multi_channel_buffer_details, /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,      /* OPCMD_DATA_FORMAT */
    multi_channel_sched_info      /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry rate_adjust_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_SET_DATA_STREAM_BASED, multi_channel_stream_based},
    {OPMSG_COMMON_SET_SAMPLE_RATE, rate_adjust_opmsg_set_sample_rates},
    {OPMSG_COMMON_SET_RATE_ADJUST_CURRENT_RATE, rate_adjust_opmsg_set_current_rate},
    {OPMSG_COMMON_SET_RATE_ADJUST_TARGET_RATE, rate_adjust_opmsg_set_target_rate},
    {OPMSG_COMMON_SET_RATE_ADJUST_PASSTHROUGH_MODE, rate_adjust_opmsg_passthrough_mode},
    {OPMSG_COMMON_ID_SET_BUFFER_SIZE, multi_channel_opmsg_set_buffer_size},

    /* obpm message */
    {OPMSG_COMMON_ID_SET_CONTROL,  rate_adjust_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,   rate_adjust_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS, rate_adjust_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,   rate_adjust_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,   rate_adjust_opmsg_obpm_get_status},

    {0, NULL}
};


/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA rate_adjust_cap_data =
{
    RATE_ADJUST_CAP_ID,                   /* Capability ID */
    RATE_ADJUST_RATE_ADJUST_VERSION_MAJOR, 1, /* Version information - hi and lo parts */
    RATE_ADJUST_MAX_CHANNELS,             /* Max number of sinks/inputs */
    RATE_ADJUST_MAX_CHANNELS,             /* Max number of sources/outputs */
    &rate_adjust_handler_table,           /* Pointer to message handler function table */
    rate_adjust_opmsg_handler_table,      /* Pointer to operator message handler function table */
    rate_adjust_process_data,             /* Pointer to data processing function */
    0,                                    /* Reserved */
    sizeof(RATE_ADJUST_OP_DATA)           /* Size of capability-specific per-instance data */
};
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_RATE_ADJUST, RATE_ADJUST_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_RATE_ADJUST, RATE_ADJUST_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

/****************************************************************************
Private Function Definitions
*/
static inline RATE_ADJUST_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (RATE_ADJUST_OP_DATA *) base_op_get_instance_data(op_data);
}

bool rate_adjust_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{

    /* if the operator is already running, ignore the start_req */
    if (opmgr_op_is_running(op_data))
    {
        return base_op_build_std_response_ex(op_data, STATUS_OK, response_data);
    }

    /* attempt to setup internal processing structures */
    if(!init_rate_adjust_internal(op_data))
    {
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }

    /* continue start up */
    return multi_channel_start(op_data, message_data, response_id, response_data);
}

bool rate_adjust_stop_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    RATE_ADJUST_OP_DATA *op_extra_data = get_instance_data(op_data);

    /* do nothing if rate_adjust is not running */
    if (opmgr_op_is_running(op_data))
    {
        free_rate_adjust_internal(op_extra_data);
    }
    return multi_channel_stop_reset(op_data,message_data,response_id,response_data);
}

bool rate_adjust_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    RATE_ADJUST_OP_DATA *op_extra_data = get_instance_data(op_data);

    L3_DBG_MSG("rate_adjust operator destroy \n");

    /* call base_op destroy that creates and fills response message, too */
    if(!base_op_destroy(op_data, message_data, response_id, response_data))
    {
        return(FALSE);
    }

    /* make sure that all internal structures are free */
    free_rate_adjust_internal(op_extra_data);

    /* free all allocated channels */
    multi_channel_detroy(op_data);

    return(TRUE);
}

bool rate_adjust_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    RATE_ADJUST_OP_DATA *op_extra_data = get_instance_data(op_data);

    L3_DBG_MSG("rate_adjust operator create \n");

    /* Form the response (assumes success). Set operator to not running state */
    if(!base_op_create(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* Allocate multi channels with metadata support */
    if( !multi_channel_create(op_data, MULTI_METADATA_FLAG, sizeof(rate_adjust_channels)))
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    multi_channel_set_callbacks(op_data, NULL, NULL);

    /* by default the operator is in passthrough mode */
    op_extra_data->rate_adjust_passthrough = TRUE;

    return TRUE;
}

/* Data processing function */
void rate_adjust_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    RATE_ADJUST_OP_DATA *rate_adjust = get_instance_data(op_data);

    if(rate_adjust->sra_graph != NULL)
    {

        /* This will return the minimum data/space available */
        unsigned max_samples_to_process = multi_channel_check_buffers(op_data, touched);

        do
        {

#ifdef INSTALL_METADATA
            /* if we have metadata, then also limit the amount
             * to process to the amount of available metadata
             */
            max_samples_to_process =
                multi_channel_metadata_limit_consumption(op_data, touched, max_samples_to_process);
#endif
            /* chance to fix up */
            patch_fn_shared(rate_adjust_op);
            if(max_samples_to_process > 0)
            {
                unsigned amount_produced;
                unsigned amount_consumed;

                /* do actual rate adjustment */
                cbops_process_data(rate_adjust->sra_graph, max_samples_to_process);

                /* get the amount consumed and  produced by cbops process */
                amount_produced = cbops_get_amount(rate_adjust->sra_graph, rate_adjust->num_channels);
                amount_consumed = cbops_get_amount(rate_adjust->sra_graph, 0);

#ifdef INSTALL_METADATA
                /* Transfer metadata from input to output */
                multi_channel_metadata_transfer(op_data,
                                                amount_consumed,
                                                amount_produced,
                                                &rate_adjust->last_tag);

                /* update phase difference for next transfer */
                rate_adjust->last_tag.timestamp_phase_correction =
                    frac_mult(rate_adjust->sample_period,
                              cbops_sra_get_phase(rate_adjust->rate_adjust_op));
#endif
            }

            /* get the amount that can be processed now,
             * loop will be repeated if more than a limit left
             * in the buffers
             */
            max_samples_to_process = multi_channel_check_buffers(op_data, touched);

        } while(max_samples_to_process >= RATE_ADJUST_MIN_SAMPLES);
    }
}

/**
 * Attempt to (re)initialise all internal structures for rate_adjust op
 * processing.
 *     returns - FALSE if initialisation failed, TRUE on success
 */
bool init_rate_adjust_internal(OPERATOR_DATA *op_data)
{
    RATE_ADJUST_OP_DATA *rate_adjust = get_instance_data(op_data);
    MULTI_CHANNEL_CHANNEL_STRUC *chan_ptr = multi_channel_first_active_channel(op_data);
    unsigned num_channels = multi_channel_get_channel_count(op_data);
    unsigned *idxs = NULL;
    unsigned ch_idx;
    cbops_op *rate_adjust_op;

    patch_fn_shared(rate_adjust_op);
    /* at this point sample rate must have been set,
     * and at least one channel must be connected */
    if((rate_adjust->sample_rate == 0) ||
       (num_channels == 0))
    {
        return FALSE;
    }

    /* no cbops graph should have been allocated */
    PL_ASSERT(NULL == rate_adjust->sra_graph);

    /* create the skeleton of the cbops graph,
     * number of input+output = (2 * num_channels) */
    rate_adjust->sra_graph = cbops_alloc_graph(num_channels * 2);
    if(rate_adjust->sra_graph == NULL)
    {
        goto init_failed;
    }

    /* Traverse through all the channels */
    ch_idx = 0;
    while (NULL != chan_ptr)
    {
        /* set the input buffer associated with this channel,
         * all the buffers are synced to first input channel */
        cbops_set_input_io_buffer(
            rate_adjust->sra_graph,    /* the graph */
            ch_idx,                    /* input channel index */
            0,                         /* first input channel index */
            chan_ptr->sink_buffer_ptr);/* buffer for this input index */

        cbops_set_output_io_buffer(
            rate_adjust->sra_graph,       /* the graph */
            ch_idx + num_channels,        /* output channel index */
            num_channels,                 /* first output channel index */
            chan_ptr->source_buffer_ptr); /* buffer for this output index */

        /* go to next channel */
        ch_idx++;
        chan_ptr = chan_ptr->next_active;
    }

    /* for precaution, number of channels
     * should match the channels in cbops graph
     */
    PL_ASSERT(ch_idx == num_channels);

    /* create a table of indexes */
    idxs = create_default_indexes(num_channels*2);
    if(idxs == NULL)
    {
        goto init_failed;
    }

    /* create a sw rate adjustment cbops operator */
    rate_adjust_op =
        create_sw_rate_adj_op(num_channels,              /* number of channels */
                              &idxs[0],                  /* address table for input channels indexes */
                              &idxs[num_channels],       /* address table for output channels indexes */
                              CBOPS_RATEADJUST_COEFFS,   /* sra filter coefficients */
                              &rate_adjust->target_rate, /* target rate address */
                              0);                        /* no shift */
    if(NULL == rate_adjust_op)
    {
        goto init_failed;
    }

    /* save the operator for easy access later */
    rate_adjust->rate_adjust_op = rate_adjust_op;

    /* by default the op will be in pass through mode,
     * but it could have changed by now by the user
     */
    cbops_rateadjust_passthrough_mode(rate_adjust_op, rate_adjust->rate_adjust_passthrough);

    /* add the operator to the graph */
    cbops_append_operator_to_graph(rate_adjust->sra_graph, rate_adjust_op);

    /* save number of channels */
    rate_adjust->num_channels =  num_channels;

#ifdef INSTALL_METADATA
    /* set initial phase (it isn't 0) */
    rate_adjust->last_tag.timestamp_phase_correction =
        frac_mult(rate_adjust->sample_period,
                  cbops_sra_get_phase(rate_adjust->rate_adjust_op));
#endif

    pfree(idxs);
    return TRUE;

  init_failed:
    if(idxs != NULL)
    {
        pfree(idxs);
    }
    return FALSE;
}

/**
 * Frees internal allocations for rate adjust op
 */
void free_rate_adjust_internal(RATE_ADJUST_OP_DATA *rate_adjust)
{
    /* delete the cbops graph */
    if(NULL != rate_adjust->sra_graph)
    {
        destroy_graph(rate_adjust->sra_graph);
        rate_adjust->sra_graph = NULL;
    }
    rate_adjust->rate_adjust_op = NULL;
    rate_adjust->target_rate = 0;
}

/* rate adjust operator obpm support */
bool rate_adjust_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* In the case of this capability, nothing is done for control message. Just follow protocol and ignore any content. */
    return cps_control_setup(message_data, resp_length, resp_data,NULL);
}
bool rate_adjust_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    return FALSE;
}
bool rate_adjust_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    return FALSE;
}
bool rate_adjust_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    return FALSE;
}
bool rate_adjust_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    return FALSE;
}

/**
 * handler for rate adjust set rate opmsg. The message contains one words:
 *    payload: [rate/25]
 * where rate is the nominal sample rate for incoming audio
 */
bool rate_adjust_opmsg_set_sample_rates(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    RATE_ADJUST_OP_DATA *rate_adjust = get_instance_data(op_data);
    unsigned rate;

    /* get the sample rate */
    rate = CONVERSION_SAMPLE_RATE_TO_HZ * OPMSG_FIELD_GET(message_data, OPMSG_COMMON_MSG_SET_SAMPLE_RATE, SAMPLE_RATE);

    /* sample rate must have been set before any
     * channel connection
     */
    if(multi_channel_get_channel_count(op_data) != 0)
    {
        return FALSE;
    }

    /* check the range */
    if(rate > RATE_ADJUST_MAX_SAMPLE_RATE ||
       rate < RATE_ADJUST_MIN_SAMPLE_RATE)
    {
        return FALSE;
    }

    /* set the sample rate */
    rate_adjust->sample_rate = rate;

#ifdef INSTALL_METADATA
    rate_adjust->last_tag.in_rate = rate;
    /* Calculate sample period, this is to avoid division when
     * calculating phase drift. We could have higher
     * resolution but 1us for this purpose is enough.
     */
    rate_adjust->sample_period = SECOND/rate;
#endif

    return TRUE;
}

/**
 * handler for OPMSG_COMMON_SET_RATE_ADJUST_TARGET_RATE operator message. The message contains two words:
 *    payload word 0: MSW16 of the target rate
 *    payload word 1: LSW16 of the target rate
 */
bool rate_adjust_opmsg_set_target_rate(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    RATE_ADJUST_OP_DATA *rate_adjust = get_instance_data(op_data);
    const unsigned *msg = (const unsigned *)message_data;

    /* set the target rate value */
    rate_adjust->target_rate = (unsigned)msg[4] + (unsigned)(msg[3] << 16);

    return TRUE;
}

/**
 * handler for OPMSG_COMMON_SET_RATE_ADJUST_PASSTHROUG_MODE operator message. The message contains one words:
 *    payload word 0: enable/disable passthrough mode
 */
bool rate_adjust_opmsg_passthrough_mode(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    RATE_ADJUST_OP_DATA *rate_adjust = get_instance_data(op_data);
    const unsigned *msg = (const unsigned *)message_data;
    bool enable = (bool) msg[3];

    /* Set the pass-through mode */
    rate_adjust->rate_adjust_passthrough = enable;

    if(NULL != rate_adjust->rate_adjust_op)
    {
        /* apply pass-through mode the graph is running */
        cbops_rateadjust_passthrough_mode(rate_adjust->rate_adjust_op, enable);
    }

    return TRUE;
}

/**
 * handler for OPMSG_COMMON_SET_RATE_ADJUST_CURRENT_RATE operator message. The message contains two words:
 *    payload word 0: MSW of rate
 *    payload word 1: LSW of rate
 */
bool rate_adjust_opmsg_set_current_rate(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    RATE_ADJUST_OP_DATA *rate_adjust = get_instance_data(op_data);
    const unsigned *msg = (const unsigned *)message_data;

    /* get current rate */
    unsigned  rate  =  (unsigned)msg[4] + (unsigned)(msg[3]<<16);


    /* This will turn off pass-through mode */
    rate_adjust->rate_adjust_passthrough = FALSE;

    if(NULL != rate_adjust->rate_adjust_op)
    {
        /* set the SW rate adjust warp value */
        cbops_sra_set_rate_adjust(rate_adjust->rate_adjust_op, rate);
    }

    return TRUE;
}

