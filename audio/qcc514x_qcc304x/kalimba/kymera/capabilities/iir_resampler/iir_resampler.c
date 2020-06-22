/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  iir_resampler.c
 * \ingroup  capabilities
 *
 * capability wrapper for iir_resamplerv2 algorithm.
 *
 */

/****************************************************************************
Include Files
*/

#include "capabilities.h"
#include "iir_resampler.h"
#include "common_conversions.h"
#include "mem_utils/scratch_memory.h"
#include "mem_utils/exported_constant_files.h"
#include "mem_utils/exported_constants.h"
#include "iir_resampler_private.h"

#include "iir_resampler_gen_c.h"
#include "../lib/audio_proc/iir_resamplev2_util.h"



/****************************************************************************
Private Constant Declarations
*/
/** The number of bits of the conversion factor that are available for
 * representing the integer portion */
#define INT_PART_BITS 5

/* legacy set_conversion_rate handler look-up table */
#define IIR_RESAMPLER_RATE_INDEX_LUT_LEN                9
const unsigned iir_resampler_rate_index_lut[IIR_RESAMPLER_RATE_INDEX_LUT_LEN] =
          {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000};

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define IIR_RESAMPLER_CAP_ID CAP_ID_DOWNLOAD_IIR_RESAMPLER
#else
#define IIR_RESAMPLER_CAP_ID CAP_ID_IIR_RESAMPLER
#endif

#define IIR_RESAMPLER_REPROCESS_MIN (2)

/****************************************************************************
Private Structure Definitions
*/

bool iir_resampler_channel_create(OPERATOR_DATA *op_data,MULTI_CHANNEL_CHANNEL_STRUC *chan_ptr,unsigned chan_idx);
void iir_resampler_channel_destroy(OPERATOR_DATA *op_data,MULTI_CHANNEL_CHANNEL_STRUC *chan_ptr);

void iir_config_pending_cb(OPERATOR_DATA *op_data,
                           uint16 cmd_id, void *msg_body,
                           pendingContext *context, unsigned cb_value);
void iir_release_constants(OPERATOR_DATA *op_data);

/** The IIR resampler capability function handler table */
const handler_lookup_struct iir_resampler_handler_table =
{
    iir_resampler_create,         /* OPCMD_CREATE */
    iir_resampler_destroy,        /* OPCMD_DESTROY */
    iir_resampler_start,          /* OPCMD_START */
    iir_resampler_stop_reset,     /* OPCMD_STOP */
    iir_resampler_stop_reset,     /* OPCMD_RESET */
    multi_channel_connect,        /* OPCMD_CONNECT */
    multi_channel_disconnect,     /* OPCMD_DISCONNECT */
    iir_resampler_buffer_details, /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,      /* OPCMD_DATA_FORMAT */
    multi_channel_sched_info  /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry iir_resampler_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_IIR_RESAMPLER_ID_SET_SAMPLE_RATES, iir_resampler_opmsg_set_sample_rates},
    {OPMSG_IIR_RESAMPLER_ID_SET_CONVERSION_RATE, iir_resampler_opmsg_set_conversion_rate},
    {OPMSG_IIR_RESAMPLER_ID_SET_CONFIG, iir_resampler_opmsg_set_config},
    {OPMSG_COMMON_SET_DATA_STREAM_BASED, multi_channel_stream_based},
    /* obpm message */
    {OPMSG_COMMON_ID_SET_CONTROL,  iir_resampler_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,   iir_resampler_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS, iir_resampler_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,   iir_resampler_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,   iir_resampler_opmsg_obpm_get_status},
    {0, NULL}
};


/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA iir_resampler_cap_data =
{
    IIR_RESAMPLER_CAP_ID,           /* Capability ID */
    IIR_RESAMPLER_IIRV2_VERSION_MAJOR, 1, /* Version information - hi and lo parts */
    IIR_RESAMPLER_MAX_CHANNELS,     /* Max number of sinks/inputs */
    IIR_RESAMPLER_MAX_CHANNELS,     /* Max number of sources/outputs */
    &iir_resampler_handler_table,   /* Pointer to message handler function table */
    iir_resampler_opmsg_handler_table,      /* Pointer to operator message handler function table */
    iir_resampler_process_data,             /* Pointer to data processing function */
    0,                              /* Reserved */
    sizeof(IIR_RESAMPLER_OP_DATA)   /* Size of capability-specific per-instance data */
};
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_IIR_RESAMPLER, IIR_RESAMPLER_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_IIR_RESAMPLER, IIR_RESAMPLER_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

/****************************************************************************
Private Function Definitions
*/
static inline IIR_RESAMPLER_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (IIR_RESAMPLER_OP_DATA *) base_op_get_instance_data(op_data);
}

/* Helper functions to load external constants */
#if defined(INSTALL_OPERATOR_CREATE_PENDING) && defined(INSTALL_CAPABILITY_CONSTANT_EXPORT)
void iir_config_pending_cb(OPERATOR_DATA *op_data,
                           uint16 cmd_id, void *msg_body,
                           pendingContext *context, unsigned cb_value)
{
    external_constant_callback_when_available(op_data, (void*)cb_value, cmd_id,
                                              msg_body, context);
}

#endif
void iir_release_constants(OPERATOR_DATA *op_data)
{
    NOT_USED(op_data);

#if defined(INSTALL_OPERATOR_CREATE_PENDING) && defined(INSTALL_CAPABILITY_CONSTANT_EXPORT)
    INT_OP_ID int_id = base_op_get_int_op_id(op_data);
    external_constant_release(iir_resamplev2_DynamicMemLowMipsDynTable_Main, int_id);
#endif
}


bool iir_resampler_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    IIR_RESAMPLER_OP_DATA *op_extra_data = get_instance_data(op_data);

    /* if the operator is already running, ignore the start_req */
    if (opmgr_op_is_running(op_data))
    {
        return base_op_build_std_response_ex(op_data, STATUS_OK, response_data);
    }

    /* start will fail if sample rates have not been set */
    if( (op_extra_data->in_rate  == IIR_RESAMPLER_NO_SAMPLE_RATE) ||
        (op_extra_data->out_rate == IIR_RESAMPLER_NO_SAMPLE_RATE) )
    {
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }

    /* attempt to setup internal processing structures (iir_resamplerv2) */
    if(!init_iir_resampler_internal(op_data))
    {
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }

    /* continue start up */
    return multi_channel_start(op_data, message_data, response_id, response_data);
}

bool iir_resampler_stop_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    IIR_RESAMPLER_OP_DATA *op_extra_data = get_instance_data(op_data);

    /* do nothing if iir_resampler is not running */
    if (opmgr_op_is_running(op_data))
    {
        free_iir_resampler_internal(op_extra_data);
    }
    return multi_channel_stop_reset(op_data,message_data,response_id,response_data);
}


bool iir_resampler_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    IIR_RESAMPLER_OP_DATA *op_extra_data = get_instance_data(op_data);

    /* call base_op destroy that creates and fills response message, too */
    if(!base_op_destroy(op_data, message_data, response_id, response_data))
    {
        return(FALSE);
    }

    /* make sure that all internal structures are free */
    free_iir_resampler_internal(op_extra_data);

    /* deregister intent to use scratch memory */
    scratch_deregister();

    /* free all allocated channels */
    multi_channel_detroy(op_data);

    /* delete the configuration list */
    iir_resamplerv2_delete_config_list();

    /* Release any constants allocated.*/
    iir_release_constants(op_data);

    return(TRUE);
}

bool iir_resampler_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    IIR_RESAMPLER_OP_DATA *op_extra_data = get_instance_data(op_data);
    L3_DBG_MSG("iir_resampler create \n");

    /* Form the response (assumes success). Set operator to not running state */
    if(!base_op_create(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* Allocate channels, in-place, no hot connect */
    if( !multi_channel_create(op_data,(MULTI_METADATA_FLAG),sizeof(iir_resampler_channels)) )
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }
    multi_channel_set_callbacks(op_data,iir_resampler_channel_create,iir_resampler_channel_destroy);

    /* register intent to use scratch memory utility */
    if(!scratch_register())
    {
        multi_channel_detroy(op_data);
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* iir_resamplerv2 internal data will be allocated at opcmd start */
    op_extra_data->iir_resamplerv2 = NULL;

    /* iir_resamplerv2 shared filter memory will be requested at opcmd start */
    op_extra_data->lpconfig = NULL;

    /* scratch memory will not be reserved until opcmd start */
    op_extra_data->scratch_reserved = 0;

    /* in/out rates have not been set */
    op_extra_data->in_rate  = IIR_RESAMPLER_NO_SAMPLE_RATE;
    op_extra_data->out_rate = IIR_RESAMPLER_NO_SAMPLE_RATE;

    /* iir_resamplev2 intermediate buffer should be as large as input buffers
     * to avoid throttling data flow. Default to large buffer size, but may
     * be updated at iir_resampler_buffer_details */
    op_extra_data->temp_buffer_size = IIR_RESAMPLER_TEMP_BUFFER_SIZE;

    /* dbl_precision, low_mips and stream_based processing are disabled by default */
    op_extra_data->config = 0;
    op_extra_data->source_block_size=IIR_RESAMPLER_DEFAULT_BLOCK_SIZE;

    /* We don't have a new constant table to add - only register our interest in the IIR RESAMPLER constant tables. */
    iir_resamplerv2_add_config_to_list(NULL);

    return TRUE;
}

bool iir_resampler_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
   /* Set buffer size to zero to get buffer size from base_op */
   multi_channel_set_buffer_size(op_data,0);
   if(multi_channel_buffer_details(op_data,message_data,response_id,response_data))
   {
      OP_BUF_DETAILS_RSP    *resp = (OP_BUF_DETAILS_RSP*) *response_data;
      IIR_RESAMPLER_OP_DATA *op_extra_data = get_instance_data(op_data);


      /* Match buffer size from previous resampler version */
      resp->b.buffer_size = IIR_RESAMPLER_TEMP_BUFFER_SIZE;

      /* update iir_resamplev2 intermediate buffer size */
      op_extra_data->temp_buffer_size = resp->b.buffer_size;

      return TRUE;
   }
   return FALSE;
}

/* Data processing function */
void iir_resampler_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    IIR_RESAMPLER_OP_DATA *op_extra_data = get_instance_data(op_data);
    iir_resampler_channels *chan_ptr     = (iir_resampler_channels*)multi_channel_first_active_channel(op_data);
    unsigned produced,active_channels;
    int consumed;

    patch_fn(iir_resampler_process_data);

    active_channels = multi_channel_active_channels(op_data);

    consumed = iir_resampler_amount_to_use(op_extra_data,chan_ptr,&produced);
    /* consumed is <0 if no channels, no data, or insufficient space */
    if(consumed<0)
    {
       return;
    }

    if((consumed==0) && (op_extra_data->conv_fact <= (1 << (DAWTH -INT_PART_BITS -1))) )
    {
         /* If consumed==0, there is at least one sample of data and one
          * sample of space. However even when downsampling, there are cases
          * where consuming 1 sample produces >1 sample, and
          * iir_resampler_amount_to_use may have returned 0 because it could
          * predict this. So try to get more space at the output. */
         touched->sinks |= active_channels;
         return;
    }

    do
    {
        /* If we consumed all the input signal a back kick */
        if(consumed==produced)
        {
            touched->sinks |= active_channels;
        }

        MULTI_CHAN_DBG_MSG2("IIR 0x%08x: consume = %d", (uintptr_t)(op_extra_data), consumed);

#ifdef INSTALL_METADATA
        consumed = multi_channel_metadata_limit_consumption(op_data,touched,consumed);
#endif

        if(consumed>0)
        {
           if(op_extra_data->in_rate==op_extra_data->out_rate)
           {
#ifdef INSTALL_METADATA
                 multi_channel_metadata_transfer(op_data,consumed,consumed,&op_extra_data->last_tag);
#endif
                 multi_channel_copy_mute(multi_channel_first_active_channel(op_data),consumed,0);

                 /* Signal forward kick */
                 touched->sources |= active_channels;
           }
           else
           {
                unsigned *temp_buffer_ptr;

                /* get a pointer to the scratch memory (Commit) for iir_resamplerv2 intermediate buffer */
                /* update intermediate data pointer in iir_resamplerv2 parameter structure */
                temp_buffer_ptr = (unsigned*) scratch_commit(op_extra_data->scratch_reserved, MALLOC_PREFERENCE_NONE);
                op_extra_data->iir_resamplerv2->common.intermediate_ptr = temp_buffer_ptr;

                /* call processing function.  Get actual amount produced */
                produced = iir_resampler_processing(op_extra_data,consumed,chan_ptr);
                MULTI_CHAN_DBG_MSG2("IIR 0x%08x: produced = 0x%08x", (uintptr_t)(op_extra_data), produced);

#ifdef INSTALL_METADATA
                multi_channel_metadata_transfer(op_data,consumed,produced,&op_extra_data->last_tag);
#endif

                /* Advance channel buffers */
                multi_channel_advance_buffers((MULTI_CHANNEL_CHANNEL_STRUC*)chan_ptr,consumed,produced);

                /* free the scratch memory used for temp intermediate buffer */
                scratch_free();

                /* Signal forward kick if produced data */
                if(produced>0)
                {
                   touched->sources |= active_channels;
                }
           }

           /* If some data was processed, re-check if more can be done.
            * iir_resampler_amount_to_use sometimes seems to limit what
            * it plans to do (B-255831). This also covers continuing
            * after transitional tags (multi_channel_metadata_limit_consumption).
            */
           consumed = iir_resampler_amount_to_use(op_extra_data,chan_ptr,&produced);

        }
        /* Don't continue processing tiny amounts */
    } while (consumed >= IIR_RESAMPLER_REPROCESS_MIN);
}

/**
 * handler for iir_resampler set rates opmsg. The message contains two words:
 *    payload: [in_rate/25], [out_rate/25]
 * where in_rate and out_rate are the input frequency and output frequency in Hz
 */
bool iir_resampler_opmsg_set_sample_rates(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    unsigned in_rate, out_rate;

    in_rate  = CONVERSION_SAMPLE_RATE_TO_HZ * OPMSG_FIELD_GET(message_data, OPMSG_MSG_IIR_RESAMPLER_SET_SAMPLE_RATES, INPUT_RATE);
    out_rate = CONVERSION_SAMPLE_RATE_TO_HZ * OPMSG_FIELD_GET(message_data, OPMSG_MSG_IIR_RESAMPLER_SET_SAMPLE_RATES, OUTPUT_RATE);

    return set_rates_iir_resampler_internal(op_data, in_rate, out_rate);
}

/**
 * handler for legacy set conversion rates opmsg. The message contains one word:
 *    payload: [rate_index]
 */
bool iir_resampler_opmsg_set_conversion_rate(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    unsigned rate_index = OPMSG_FIELD_GET(message_data, OPMSG_IIR_RESAMPLER_SET_CONVERSION_RATE, CONVERSION_RATE);
    unsigned in_rate    = (rate_index & 0xF0) >> 4;
    unsigned out_rate   = (rate_index & 0x0F);

    /* check that requested rate indeces are valid */
    if( (in_rate  >= IIR_RESAMPLER_RATE_INDEX_LUT_LEN) ||
        (out_rate >= IIR_RESAMPLER_RATE_INDEX_LUT_LEN) )
    {
        return FALSE;
    }

    /* read conversion rate frequencies (Hz) from rate index look-up table */
    in_rate  = iir_resampler_rate_index_lut[in_rate];
    out_rate = iir_resampler_rate_index_lut[out_rate];

    return set_rates_iir_resampler_internal(op_data, in_rate, out_rate);
}

/**
 * handler for set_config opmsg. set resampler configuration bit flags.
 *     one 16-bit word payload:
 *          +---------------+---+---+
 *          | 15 <------> 2 | 1 | 0 |
 *          |    RESERVED   | L | D |    L=low_mips, D=dbl_precision
 *          +---------------+---+---+
 */
bool iir_resampler_opmsg_set_config(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    IIR_RESAMPLER_OP_DATA *op_extra_data = get_instance_data(op_data);
    unsigned config = OPMSG_FIELD_GET(message_data, OPMSG_IIR_RESAMPLER_SET_CONFIG, CONFIG_FLAGS);

    /* ignore undefined config bits */
    config &= IIR_RESAMPLER_EX_CONFIG_MASK;

    if((op_extra_data->config & IIR_RESAMPLER_EX_CONFIG_MASK) == config)
    {
        /* configuration has not changed, so nothing to do */
        return TRUE;
    }


    /* check if the low mips configuration changed. */
    if ((op_extra_data->config ^ config) & IIR_RESAMPLER_CONFIG_LOW_MIPS)
    {
        if (opmgr_op_is_running(op_data))
        {
            /* You cannot change the low mips configuration on the fly because
             * the operator will die. */
            return FALSE;
        }
#if defined(INSTALL_OPERATOR_CREATE_PENDING) && defined(INSTALL_CAPABILITY_CONSTANT_EXPORT)
        if (config & IIR_RESAMPLER_CONFIG_LOW_MIPS)
        {
            INT_OP_ID int_id = base_op_get_int_op_id(op_data);
            /* Low mips mode will be enabled, load the external constants.*/
            /* Reserve (and request) any dynamic memory tables that may be in external
             * file system.
             * A negative return value indicates a fatal error */
            if (!external_constant_reserve(iir_resamplev2_DynamicMemLowMipsDynTable_Main, int_id))
            {
                L2_DBG_MSG("iir_resampler_opmsg_set_config - failed reserving constants");
                iir_release_constants(op_data);
                return TRUE;
            }

            /* Now see if these tables are available yet */
            if (!is_external_constant_available(iir_resamplev2_DynamicMemLowMipsDynTable_Main, int_id))
            {
                /* Free the response created above, before it gets overwritten with the pending data */
                pdelete(*resp_data);

                *resp_data = (void*)(pending_operator_cb)iir_config_pending_cb;

                L4_DBG_MSG("iir_resampler_opmsg_set_config - requesting callback when constants available");
                return (bool)HANDLER_INCOMPLETE;
            }
        }
        else
#endif
        {
            /* Low mips mode will be disabled, release the external constants.*/
            iir_release_constants(op_data);
        }
    }

    op_extra_data->config = (op_extra_data->config & IIR_RESAMPLER_INT_CONFIG_MASK) | config;

    /* if the op is running, need to clear up and reinitialize internal data */
    if (opmgr_op_is_running(op_data))
    {
        bool success;

        /* attempt to setup internal processing for the new configuration */
        opmgr_op_suspend_processing(op_data);
        success = init_iir_resampler_internal(op_data);
        opmgr_op_resume_processing(op_data);

        if (!success)
        {
            /* return failure, op will remain in not running state */
            base_op_stop_operator(op_data);
            return FALSE;
        }
    }

    return TRUE;
}



/**
 * \brief Calculates the conversion ratio as a fraction which represents integer
 * and fractional parts shifted to be stored as single precision fraction.
 *
 * \param in_rate The input terminal's sample rate
 * \param out_rate The output terminal's sample rate
 * \return The conversion factor encoded as a fraction
 */
static unsigned calc_conv_factor(unsigned in_rate, unsigned out_rate)
{
    /* As the fractional representation begins at the DAWTH-1 bit the integer
     * part of the conversion factor is bits DAWTH-1 to DAWTH-1 -INT_PART_BITS. */
    if (in_rate > out_rate)
    {
        unsigned conv_fact = frac_div(out_rate, in_rate);
        return conv_fact >> INT_PART_BITS;
    }
    else if (in_rate == out_rate)
    {
        return 1 << (DAWTH -INT_PART_BITS -1);
    }
    else
    {
        unsigned int_part = out_rate/in_rate;
        unsigned remainder = out_rate - int_part * in_rate;
        unsigned conv_fact = frac_div(remainder, in_rate);

        conv_fact >>= INT_PART_BITS;

        conv_fact |= int_part << (DAWTH -INT_PART_BITS -1);
        return conv_fact;
    }
}



/**
 * set_rates_iir_resampler_internal
 *   helper function to handle changes in sample rate.
 *
 * For multichannel iir_resampler operators, the same conversion will be applied
 * to every active channel, so all input channels of an iir_resampler must
 * receive data at the same rate, and all output channels will produce data at
 * the same rate. If different rates are desired per channel, multiple instances
 * of iir_resampler should be used.
 *
 * The sample rates may be changed while the operator is running, but this will
 * require the operator to free and reallocate/reinitialize all internal data
 * within the opmsg handler. If this fails for a reason such as insufficient RAM
 * the operator will return to a not-running state and must be restarted when
 * enough resources are available before processing will resume.
 */
bool set_rates_iir_resampler_internal(OPERATOR_DATA *op_data, unsigned in_rate, unsigned out_rate)
{
    bool low_mips;
    IIR_RESAMPLER_OP_DATA *op_extra_data = get_instance_data(op_data);
    unsigned block_size;

    patch_fn_shared(iir_resampler);

    if( (in_rate  == op_extra_data->in_rate) &&
        (out_rate == op_extra_data->out_rate) )
    {
        /* sample rates have not changed, so nothing to do */
        return TRUE;
    }

    /* based on current config, check if a low_mips config is desired */
    low_mips = (op_extra_data->config & IIR_RESAMPLER_CONFIG_LOW_MIPS) ? TRUE : FALSE;

    /* check if the requested sample rate corresponds to an existing resampler
     * configuration */
    if( (in_rate != out_rate) &&
        (iir_resamplerv2_get_id_from_rate(in_rate, out_rate, low_mips) == 0) )
    {
        return FALSE;
    }

    op_extra_data->in_rate = in_rate;
    op_extra_data->out_rate = out_rate;
#ifdef INSTALL_METADATA
    op_extra_data->last_tag.in_rate = in_rate;
#endif

    op_extra_data->conv_fact = calc_conv_factor(in_rate, out_rate);

    /* setup block size at output */
    block_size = frac_mult((1*IIR_RESAMPLER_DEFAULT_BLOCK_SIZE) << INT_PART_BITS, op_extra_data->conv_fact);
    if(block_size<IIR_RESAMPLER_DEFAULT_BLOCK_SIZE)
    {
       block_size = IIR_RESAMPLER_DEFAULT_BLOCK_SIZE;
    }
    op_extra_data->source_block_size = block_size;

    /* if the op is running, need to clear up and reinitialize internal data */
    if (opmgr_op_is_running(op_data))
    {
        bool success;

        /* attempt to setup internal processing for the new configuration */
        opmgr_op_suspend_processing(op_data);
        success = init_iir_resampler_internal(op_data);
        opmgr_op_resume_processing(op_data);

        if (!success)
        {
            /* return failure, op will remain in not running state */
            base_op_stop_operator(op_data);
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * Attempt to (re)initailize all internal structures for iir_resamplerv2
 * processing.
 *     returns - FALSE if initialization failed, TRUE on success
 */
bool init_iir_resampler_internal(OPERATOR_DATA *op_data)
{
    iir_resampler_channels *chan_ptr = (iir_resampler_channels*)multi_channel_first_active_channel(op_data);
    IIR_RESAMPLER_OP_DATA *op_extra_data = get_instance_data(op_data);
    void *lpconfig;


    iir_resampler_internal *iir_resamplerv2;
    unsigned num_channels, in_rate, out_rate, hist_size=0;
    bool dbl_precision, low_mips;

    patch_fn_shared(iir_resampler);

    /* check to ensure that no internal structures are currently allocated */
    free_iir_resampler_internal(op_extra_data);

    /* Check for connected channels */
    if(chan_ptr==NULL)
    {
       return FALSE;
    }

    /* fail if sample rates have not been set */
    in_rate  = op_extra_data->in_rate;
    out_rate = op_extra_data->out_rate;
    if( (in_rate  == IIR_RESAMPLER_NO_SAMPLE_RATE) || (out_rate== IIR_RESAMPLER_NO_SAMPLE_RATE) )
    {
        return(FALSE);
    }

    /* reserve the scratch memory required for iir_resamplerv2 intermediate buffer */
    if(!scratch_reserve(op_extra_data->temp_buffer_size*sizeof(unsigned), MALLOC_PREFERENCE_NONE))
    {
        free_iir_resampler_internal(op_extra_data);
        return(FALSE);
    }

    /* scratch memory has been reserved, remember how much to release */
    op_extra_data->scratch_reserved = op_extra_data->temp_buffer_size*sizeof(unsigned);

    dbl_precision = (op_extra_data->config & IIR_RESAMPLER_CONFIG_DBL_PRECISION) ? TRUE : FALSE;
    low_mips = (op_extra_data->config & IIR_RESAMPLER_CONFIG_LOW_MIPS) ? TRUE : FALSE;

    /* allocate filter */
    lpconfig = iir_resamplerv2_allocate_config_by_rate(in_rate, out_rate, low_mips);
    if(lpconfig)
    {
        hist_size = iir_resamplerv2_get_buffer_sizes(lpconfig,dbl_precision);
    }
    /* filter should have been allocated unless passthrough mode */
    else if (in_rate != out_rate)
    {
        free_iir_resampler_internal(op_extra_data);
        return(FALSE);
    }
    /* save pointer to iir_resamplerv2 shared filter memory */
    op_extra_data->lpconfig = lpconfig;

    /* allocate internal iir_resamplerv2 data structure for N channels */
    num_channels    = multi_channel_get_channel_count(op_data);
    iir_resamplerv2 = (iir_resampler_internal*) xzppmalloc(sizeof(iir_resampler_internal) +
                                            num_channels*(sizeof(iir_resamplerv2_channel) + hist_size*sizeof(unsigned)), MALLOC_PREFERENCE_DM1);
    if(!iir_resamplerv2)
    {
        free_iir_resampler_internal(op_extra_data);
        return(FALSE);
    }
    /* save pointer to iir_resamplerv2 internal data in op_extra_data */
    op_extra_data->iir_resamplerv2 = iir_resamplerv2;

    /* initialize common parameters */
    iir_resamplerv2->common.intermediate_size = op_extra_data->temp_buffer_size;
    iir_resamplerv2->common.dbl_precission    = dbl_precision;

    /* the history buffer starts after the end of the channel struct array */
    iir_resamplerv2->working = (unsigned*) (iir_resamplerv2->channel + num_channels);

    /* shift input to Q9.xx for processing head-room */
    iir_resamplerv2->common.input_scale  = -IIR_RESAMPLEV2_IO_SCALE_FACTOR;

    /* shift output back to Q1.xx */
    iir_resamplerv2->common.output_scale =  IIR_RESAMPLEV2_IO_SCALE_FACTOR;

    /* set internal pointer to shared memory iir_resamplerv2 data structure */
    iir_resamplerv2_set_config(&iir_resamplerv2->common,lpconfig);

    /* initialize iir_resamplerv2 channel data */
    iir_resampler_reset_internal(op_extra_data,chan_ptr);

    return TRUE;
}

void free_iir_resampler_internal(IIR_RESAMPLER_OP_DATA* op_extra_data)
{
    patch_fn_shared(iir_resampler);

    /* release iir_resamplerv2 shared filter memory */
    if(op_extra_data->lpconfig)
    {
        iir_resamplerv2_release_config(op_extra_data->lpconfig);
        op_extra_data->lpconfig = NULL;
    }

    /* free iir_resamplerv2 internal data */
    if(op_extra_data->iir_resamplerv2)
    {
        pfree(op_extra_data->iir_resamplerv2);
        op_extra_data->iir_resamplerv2 = NULL;
    }

    /* release any scratch memory that the task reserved */
    if(op_extra_data->scratch_reserved)
    {
        scratch_release(op_extra_data->scratch_reserved, MALLOC_PREFERENCE_NONE);
        op_extra_data->scratch_reserved = 0;
    }
}

bool iir_resampler_channel_create(OPERATOR_DATA *op_data,MULTI_CHANNEL_CHANNEL_STRUC *chan_ptr,unsigned chan_idx)
{
   iir_resampler_channels *lpchan       = (iir_resampler_channels*)chan_ptr;
   IIR_RESAMPLER_OP_DATA* op_extra_data = get_instance_data(op_data);

   lpchan->lpconv_fact = &op_extra_data->conv_fact;
   return TRUE;
}

void iir_resampler_channel_destroy(OPERATOR_DATA *op_data,MULTI_CHANNEL_CHANNEL_STRUC *chan_ptr)
{
}



/* iir resampler obpm support */
bool iir_resampler_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* In the case of this capability, nothing is done for control message. Just follow protocol and ignore any content. */
    return cps_control_setup(message_data, resp_length, resp_data,NULL);
}
bool iir_resampler_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    return FALSE;
}
bool iir_resampler_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Set the parameter(s). For future proofing, it is using the whole mechanism, although currently there is only one field
     * in opdata structure that is a setable parameter. If later there will be more (ever), must follow contiquously the first field,
     * as commented and instructed in the op data definition. Otherwise consider moving them into a dedicated structure.
     */

    return FALSE;
}
bool iir_resampler_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
     return FALSE;
}
bool iir_resampler_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    IIR_RESAMPLER_OP_DATA *op_extra_data = get_instance_data(op_data);

    unsigned* resp = NULL;

    if(!common_obpm_status_helper(message_data,resp_length,resp_data,sizeof(IIR_RESAMPLER_STATISTICS) ,&resp))
    {
         return FALSE;
    }

    if(resp)
    {
        resp = cpsPack2Words(op_extra_data->in_rate, op_extra_data->out_rate, resp);
    }

    return TRUE;
}
