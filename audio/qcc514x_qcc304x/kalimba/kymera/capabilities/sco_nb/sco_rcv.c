/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  sco_rcv.c
 * \ingroup  operators
 *
 *  SCO send operator
 *
 */

/****************************************************************************
Include Files
*/

#include "sco_nb_private.h"
#include "sco_common_funcs.h"
#include "sco_rcv_gen_c.h"
#include "capabilities.h"

#include "sco_fw_c.h"
#include "sco_struct.h"

#include "mem_utils/scratch_memory.h"

#ifdef SCO_RCV_DEBUG_FRAME
#include <stdio.h>
#endif


/****************************************************************************
Private Constant Definitions
*/

/* sizeof(SCO_RCV_PARAMETERS) in words */
#define SCO_RCV_NUM_PARAMETERS      (sizeof(SCO_RCV_PARAMETERS) >> LOG2_ADDR_PER_WORD)

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define SCO_RCV_CAP_ID CAP_ID_DOWNLOAD_SCO_RCV
#else
#define SCO_RCV_CAP_ID CAP_ID_SCO_RCV
#endif

/****************************************************************************
Private Type Definitions
*/

/****************************************************************************
Private Constant Declarations
*/
/** The SCO receive capability function handler table */
const handler_lookup_struct sco_rcv_handler_table =
{
    sco_rcv_create,           /* OPCMD_CREATE */
    sco_rcv_destroy,          /* OPCMD_DESTROY */
    sco_rcv_start,            /* OPCMD_START */
    base_op_stop,             /* OPCMD_STOP */
    sco_rcv_reset,            /* OPCMD_RESET */
    sco_rcv_connect,          /* OPCMD_CONNECT */
    sco_rcv_disconnect,       /* OPCMD_DISCONNECT */
    sco_rcv_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    sco_rcv_get_data_format,  /* OPCMD_DATA_FORMAT */
    sco_rcv_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table */
const opmsg_handler_lookup_table_entry sco_rcv_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,      base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE,               sco_rcv_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE,              sco_rcv_opmsg_disable_fadeout},
    {OPMSG_SCO_RCV_ID_SET_FROM_AIR_INFO,           sco_rcv_opmsg_set_from_air_info},
#ifdef INSTALL_PLC100
    {OPMSG_SCO_RCV_ID_FORCE_PLC_OFF,               sco_rcv_opmsg_force_plc_off},
#endif /* INSTALL_PLC100 */
    {OPMSG_SCO_RCV_ID_FRAME_COUNTS,                sco_rcv_opmsg_frame_counts},
    {OPMSG_COMMON_ID_SET_CONTROL,                  sco_rcv_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,                   sco_rcv_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                 sco_rcv_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                   sco_rcv_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                   sco_rcv_opmsg_obpm_get_status},
#ifdef SCO_RX_OP_GENERATE_METADATA
    {OPMSG_COMMON_ID_SET_BUFFER_SIZE,             sco_common_rcv_opmsg_set_buffer_size},
    {OPMSG_COMMON_SET_TTP_LATENCY,                sco_common_rcv_opmsg_set_ttp_latency},
#endif
    {0, NULL}};

const CAPABILITY_DATA sco_rcv_cap_data =
{
    SCO_RCV_CAP_ID ,            /* Capability ID */
    SCO_RCV_NB_VERSION_MAJOR, SCO_RCV_NB_VERSION_MINOR,  /* Version information - hi and lo parts */
    1, 1,                           /* Max number of sinks/inputs and sources/outputs */
    &sco_rcv_handler_table,         /* Pointer to message handler function table */
    sco_rcv_opmsg_handler_table,    /* Pointer to operator message handler function table */
    sco_rcv_process_data,           /* Pointer to data processing function */
    0,                              /* Reserved */
    sizeof(SCO_COMMON_RCV_OP_DATA)         /* Size of capability-specific per-instance data */
};
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_SCO_RCV, SCO_COMMON_RCV_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_SCO_RCV, SCO_COMMON_RCV_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

/****************************************************************************
Public Function Declarations
*/

/****************************************************************************
Private Function Definitions
*/
static inline SCO_COMMON_RCV_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SCO_COMMON_RCV_OP_DATA *) base_op_get_instance_data(op_data);
}

/* ********************************** API functions ************************************* */

bool sco_rcv_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SCO_COMMON_RCV_OP_DATA* sco_rcv = get_instance_data(op_data);

    /* call base_op create, which also allocates and fills response message */
    if(!base_op_create(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

#ifdef INSTALL_PLC100
    /* operator buffer size - using the same default */
    if(!cpsInitParameters(&sco_rcv->parms_def,(unsigned*)SCO_RCV_GetDefaults(base_op_get_cap_id(op_data)),(unsigned*)&sco_rcv->sco_rcv_parameters.force_plc_off,sizeof(SCO_RCV_PARAMETERS)))
    {
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* setup the PLC structure, zero initialise it so that we can detect if an
     * allocation has happened when we unwind */
    sco_rcv->sco_rcv_parameters.plc100_struc = xzpnew(PLC100_STRUC);
    if( sco_rcv->sco_rcv_parameters.plc100_struc == NULL)
    {
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }
    // allocate speech buffer
    sco_rcv->sco_rcv_parameters.plc100_struc->speech_buf = xpnewn(SP_BUF_LEN_NB, int);
    if( sco_rcv->sco_rcv_parameters.plc100_struc->speech_buf == NULL)
    {
        /* Free PLC structure and associated allocs */
        sco_common_rcv_destroy_plc_data(sco_rcv);
        /* Change the already allocated response to command failed. No extra error info.*/
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }
    sco_rcv->sco_rcv_parameters.plc100_struc->speech_buf_start = sco_rcv->sco_rcv_parameters.plc100_struc->speech_buf;

    // allocate the ola buffer
    sco_rcv->sco_rcv_parameters.plc100_struc->ola_buf = xpnewn(OLA_LEN_NB, int);
    if( sco_rcv->sco_rcv_parameters.plc100_struc->ola_buf == NULL)
    {
        /* Free PLC structure and associated allocs */
        sco_common_rcv_destroy_plc_data(sco_rcv);
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    // initialise the rest of the structure
    sco_rcv->sco_rcv_parameters.plc100_struc->consts = get_plc100_constants(PLC100_NB_CONSTANTS);
    if( sco_rcv->sco_rcv_parameters.plc100_struc->consts == NULL)
    {
        /* Free PLC structure and associated allocs */
        sco_common_rcv_destroy_plc_data(sco_rcv);
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }
#endif /* INSTALL_PLC100 */

    /* Initialise some of the operator data that is common between NB and WB receive. It can only be called
     * after the PLC structure is allocated (if PLC present in build) */
    sco_common_rcv_initialise(sco_rcv);

    /* initialise specific data (was allocated and pointer to it filled by OpMgr  */
    sco_common_rcv_reset_working_data(sco_rcv);

    /* initialize scratch memory for CVSD decoder*/
#ifdef CVSD_CODEC_SOFTWARE
    if (!scratch_register())
    {
        return TRUE;
    }
    if (!scratch_reserve(SCRATCH_SIZE_WORDS, MALLOC_PREFERENCE_NONE))
    {
        return(FALSE);
    }
#endif /* CVSD_CODEC_SOFTWARE */

    return TRUE;
}


bool sco_rcv_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SCO_COMMON_RCV_OP_DATA* sco_rcv = get_instance_data(op_data);
    L2_DBG_MSG2("t = %08x: Destroying sco_rcv operator %04x", time_get_time(), base_op_get_ext_op_id(op_data));
    L2_DBG_MSG5("md_num_kicks = %u, num_bad_kicks %u, num_kicks_no_data %u, md_late_pkts %u, md_early_pkts %u",
        sco_rcv->sco_rcv_parameters.md_num_kicks, sco_rcv->sco_rcv_parameters.num_bad_kicks,
        sco_rcv->sco_rcv_parameters.num_kicks_no_data,
        sco_rcv->sco_rcv_parameters.md_late_pkts, sco_rcv->sco_rcv_parameters.md_early_pkts);
#ifdef INSTALL_PLC100
    L2_DBG_MSG4("md_pkt_faked =%u, frame_count %u, md_pkt_size_changed %u, num_good_pkts_per_kick %u",
        sco_rcv->sco_rcv_parameters.md_pkt_faked, sco_rcv->sco_rcv_parameters.frame_count,
        sco_rcv->sco_rcv_parameters.md_pkt_size_changed, sco_rcv->sco_rcv_parameters.num_good_pkts_per_kick);
#endif
#ifdef SCO_DEBUG_STATUS_COUNTERS
    L2_DBG_MSG4("md_status_ok=%u, md_status_crc %u, md_status_nothing %u, md_status_not_sched %u",
        sco_rcv->sco_rcv_parameters.md_status_ok, sco_rcv->sco_rcv_parameters.md_status_crc,
        sco_rcv->sco_rcv_parameters.md_status_nothing, sco_rcv->sco_rcv_parameters.md_status_not_sched);
#endif
    /* call base_op destroy that creates and fills response message, too */
    if(!base_op_destroy(op_data, message_data, response_id, response_data))
    {
        return(FALSE);
    }
#ifdef INSTALL_PLC100
    /* delete all PLC data and NULLify the PLC struct ptr */
    sco_common_rcv_destroy_plc_data(get_instance_data(op_data));
#endif

#ifdef CVSD_CODEC_SOFTWARE
    scratch_deregister();
#endif
    return TRUE;
}


bool sco_rcv_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SCO_COMMON_RCV_OP_DATA *sco_rcv = get_instance_data(op_data);
    if(!base_op_reset(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* now initialise specific working data */
    sco_common_rcv_reset_working_data(sco_rcv);

    return TRUE;
}


bool sco_rcv_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SCO_COMMON_RCV_OP_DATA *sco_rcv = get_instance_data(op_data);

    /* TODO - flush input buffer and zero output buffer contents? or endpoint will do it */

    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    if (opmgr_op_is_running(op_data))
    {
        return TRUE;
    }

    /* Sanity check for buffers being connected.
     * We can't do much useful without */
    if (sco_rcv->buffers.ip_buffer == NULL || sco_rcv->buffers.op_buffer == NULL)
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

#ifdef SCO_RX_OP_GENERATE_METADATA
    /* connected endpoint to sco rx endpoint is stored here, it's needed for its API call
     * to reset metadata when flushing buffer.
     */
    sco_rcv->sco_source_ep = stream_get_connected_endpoint_from_terminal_id(base_op_get_int_op_id(op_data), 0);
#endif
    L2_DBG_MSG2("t=%08x: sco_rcv operator %04x started", time_get_time(), base_op_get_ext_op_id(op_data));

    return TRUE;
}


bool sco_rcv_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SCO_COMMON_RCV_OP_DATA *x_data = get_instance_data(op_data);
#ifdef INSTALL_PLC100

    unsigned terminal;


    bool status = sco_common_connect(op_data, message_data, response_id, response_data,
            &x_data->buffers, &terminal);

    /* if connecting source terminal and previous steps were OK, hook up output buffer beauty with the PLC beast */
    if(status && (terminal == OUTPUT_TERMINAL_ID))
    {
        x_data->sco_rcv_parameters.plc100_struc->output = x_data->buffers.op_buffer;
    }

    return status;
#else
    return sco_common_connect(op_data, message_data, response_id, response_data,
            &x_data->buffers, NULL);
#endif /* INSTALL_PLC100 */
}


bool sco_rcv_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SCO_COMMON_RCV_OP_DATA *x_data = get_instance_data(op_data);
#ifdef INSTALL_PLC100

    unsigned terminal;


    bool status = sco_common_disconnect(op_data, message_data, response_id, response_data,
            &x_data->buffers, &terminal);

    /* if disconnecting source terminal and all previous steps were OK, disconnect output buffer from PLC */
    if(status && (terminal == OUTPUT_TERMINAL_ID))
    {
        x_data->sco_rcv_parameters.plc100_struc->output = NULL;
    }

    return status;
#else
    return sco_common_disconnect(op_data, message_data, response_id, response_data,
            &x_data->buffers, NULL);
#endif /* INSTALL_PLC100 */
}


bool sco_rcv_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SCO_COMMON_RCV_OP_DATA *sco_data = get_instance_data(op_data);
    if(!base_op_buffer_details(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    if (OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data) & TERMINAL_SINK_MASK)
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size= SCO_RCV_INPUT_BUFFER_SIZE;
    }
    else
    {
        /* user can configure to have larger buffer size for output side */
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = MAX(SCO_RCV_OUTPUT_BUFFER_SIZE,
                                                                   sco_data->sco_rcv_parameters.output_buffer_size);

    }
#ifdef SCO_RX_OP_GENERATE_METADATA
    L2_DBG_MSG("SCO_RX_OP_GENERATE_METADATA, metadata is supported");
    /* supports metadata in both side  */
    ((OP_BUF_DETAILS_RSP*)*response_data)->metadata_buffer = 0;
    ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = TRUE;
#endif /* SCO_RX_OP_GENERATE_METADATA */

    L4_DBG_MSG1("sco_rcv_buffer_details  %d \n", ((OP_STD_RSP*)*response_data)->resp_data.data);

    return TRUE;
}

bool sco_rcv_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data)
{
#ifdef CVSD_CODEC_SOFTWARE
    return sco_common_get_data_format(op_data, message_data, response_id, response_data,
        AUDIO_DATA_FORMAT_16_BIT_WITH_METADATA, AUDIO_DATA_FORMAT_FIXP);
#else
    return sco_common_get_data_format(op_data, message_data, response_id, response_data,
        AUDIO_DATA_FORMAT_FIXP_WITH_METADATA, AUDIO_DATA_FORMAT_FIXP);
#endif //CVSD_CODEC_SOFTWARE
}

bool sco_rcv_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data)
{
    return sco_common_get_sched_info(op_data, message_data, response_id, response_data, 0, 0);
}

/* ************************************* Data processing-related functions and wrappers **********************************/


void sco_rcv_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    unsigned status, output_words;
    SCO_COMMON_RCV_OP_DATA* x_data = get_instance_data(op_data);
    tCbuffer temp_cbuf;

    patch_fn(sco_rcv_process_data);

    SCO_DBG_MSG("sco_rcv_process_data\n");
    /* We can't have started in this state, so if we lose a buffer carry on
     * and hope that it comes back or we are stopped. If not then the error
     * (lack of data) should propagate to something that cares. */
    if (x_data->buffers.ip_buffer == NULL || x_data->buffers.op_buffer == NULL)
    {
        return;
    }

    /* always send forward kick;
     * the operator normally send a kick forward anyway if the sco packet
     * has been received at expected time. However since occasionally this might
     * not be the case we kick forward so the connected operator receives sufficient
     * kicks.
     */
    touched->sources = TOUCHED_SOURCE_0;

    /* don't bother doing anything if we see less data than our block size */
    if(cbuffer_calc_amount_data_in_words(x_data->buffers.ip_buffer) < SCO_RCV_MIN_BLOCK_SIZE)
    {
        return;
    }

    temp_cbuf = *(x_data->buffers.op_buffer);


#ifdef CVSD_CODEC_SOFTWARE
    x_data->sco_rcv_parameters.ptScratch = (int*)scratch_commit(SCRATCH_SIZE_WORDS, MALLOC_PREFERENCE_NONE);
#endif

    status = sco_rcv_processing(x_data);

#ifdef CVSD_CODEC_SOFTWARE
    scratch_free();
    output_words = x_data->sco_rcv_parameters.sco_pkt_size;
#else
    output_words = x_data->sco_rcv_parameters.sco_pkt_size / 2;
#endif


    /* Is fadeout enabled? if yes, do it on the current output data, if processing has actually produced output */
    if( (x_data->fadeout_parameters.fadeout_state != NOT_RUNNING_STATE) && status)
    {
        /* the wrapper below takes output Cbuffer and fadeout params, and we'll use the current packet size in words */
        if (mono_cbuffer_fadeout(&temp_cbuf, output_words,
                                 &(x_data->fadeout_parameters)))
        {
            common_send_simple_unsolicited_message(op_data, OPMSG_REPLY_ID_FADEOUT_DONE);
        }
    }
}


/* **************************** Operator message handlers ******************************** */

bool sco_rcv_opmsg_enable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_COMMON_RCV_OP_DATA* x_data = get_instance_data(op_data);
    common_set_fadeout_state(&x_data->fadeout_parameters, RUNNING_STATE);

    return TRUE;
}


bool sco_rcv_opmsg_disable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_COMMON_RCV_OP_DATA* x_data = get_instance_data(op_data);
    common_set_fadeout_state(&x_data->fadeout_parameters, NOT_RUNNING_STATE);

    return TRUE;
}


bool sco_rcv_opmsg_set_from_air_info(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_COMMON_RCV_OP_DATA* x_data = get_instance_data(op_data);
    return sco_common_rcv_set_from_air_info_helper(op_data, x_data,  message_data);
}

#ifdef INSTALL_PLC100
bool sco_rcv_opmsg_force_plc_off(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_COMMON_RCV_OP_DATA* x_data = get_instance_data(op_data);
    return sco_common_rcv_force_plc_off_helper(x_data, message_data);
}
#endif /* INSTALL_PLC100 */


bool sco_rcv_opmsg_frame_counts(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_COMMON_RCV_OP_DATA* x_data = get_instance_data(op_data);
    return sco_common_rcv_frame_counts_helper(x_data, message_data, resp_length, resp_data);
}


bool sco_rcv_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* In the case of this capability, nothing is done for control message. Just follow protocol and ignore any content. */
    return cps_control_setup(message_data, resp_length, resp_data,NULL);
}


bool sco_rcv_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
#ifdef INSTALL_PLC100
   SCO_COMMON_RCV_OP_DATA *sco_rcv = get_instance_data(op_data);

   return cpsGetParameterMsgHandler(&sco_rcv->parms_def ,message_data, resp_length,resp_data);
#else
    return FALSE;
#endif /* INSTALL_PLC100 */
}

bool sco_rcv_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
   SCO_COMMON_RCV_OP_DATA *sco_rcv = get_instance_data(op_data);

   return cpsGetDefaultsMsgHandler(&sco_rcv->parms_def ,message_data, resp_length,resp_data);
}

bool sco_rcv_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Set the parameter(s). For future proofing, it is using the whole mechanism, although currently there is only one field
     * in opdata structure that is a setable parameter. If later there will be more (ever), must follow contiguously the first field,
     * as commented and instructed in the op data definition. Otherwise consider moving them into a dedicated structure.
     */
#ifdef INSTALL_PLC100
   SCO_COMMON_RCV_OP_DATA *sco_rcv = get_instance_data(op_data);

   return cpsSetParameterMsgHandler(&sco_rcv->parms_def ,message_data, resp_length,resp_data);
#else
    return FALSE;
#endif /* INSTALL_PLC100 */
}

bool sco_rcv_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_COMMON_RCV_OP_DATA *sco_rcv = get_instance_data(op_data);
    SCO_RCV_PARAMS* sco_rcv_params = &sco_rcv->sco_rcv_parameters;
    unsigned* resp;

    if(!common_obpm_status_helper(message_data,resp_length,resp_data,sizeof(SCO_RCV_STATISTICS),&resp))
    {
         return FALSE;
    }

    /* Fill the statistics as needed.
     */
    if (resp)
    {
        resp = cpsPack2Words(sco_rcv_params->sco_pkt_size, sco_rcv_params->t_esco, resp);
        resp = cpsPack2Words(sco_rcv_params->frame_count, sco_rcv_params->frame_error_count, resp);
        resp = cpsPack2Words(sco_rcv_params->md_late_pkts, sco_rcv_params->md_early_pkts, resp);
        cpsPack2Words(sco_rcv_params->out_of_time_pkt_cnt, sco_rcv_params->md_out_of_time_reset, resp);
    }

    return TRUE;
}
