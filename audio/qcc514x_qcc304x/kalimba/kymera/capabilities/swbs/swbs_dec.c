/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  swbs_dec.c
 * \ingroup  operators
 *
 *  SWBS_DEC operator
 *
 */

/****************************************************************************
Include Files
*/
#include "swbs_private.h"
#include "swbs_dec_gen_c.h"
#include "op_msg_utilities.h"
#include "op_msg_helpers.h"

#include "patch/patch.h"

#ifdef ADAPTIVE_R2_BUILD
#include "aptXAdaptiveTypes_r2.h"
#include "aptXAdaptiveDecode_r2.h"
#include "aptXAdaptiveDecoderMemory_r2.h"
#else
#include "aptXAdaptiveTypes.h"
#include "aptXAdaptiveDecode.h"
#include "aptXAdaptiveDecoderMemory.h"
#endif

/****************************************************************************
Private Constant Definitions
*/

/****************************************************************************
Private Type Definitions
*/


/****************************************************************************
Private Constant Declarations
*/

/** The SWBS decoder capability function handler table */
const handler_lookup_struct swbs_dec_handler_table =
{
    swbs_dec_create,           /* OPCMD_CREATE */
    swbs_dec_destroy,          /* OPCMD_DESTROY */
    swbs_dec_start,            /* OPCMD_START */
    base_op_stop,              /* OPCMD_STOP */
    swbs_dec_reset,            /* OPCMD_RESET */
    swbs_dec_connect,          /* OPCMD_CONNECT */
    swbs_dec_disconnect,       /* OPCMD_DISCONNECT */
    swbs_buffer_details,       /* OPCMD_BUFFER_DETAILS */
    swbs_dec_get_data_format,  /* OPCMD_DATA_FORMAT */
    swbs_dec_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table */
const opmsg_handler_lookup_table_entry swbs_dec_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,      base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE,               swbs_dec_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE,              swbs_dec_opmsg_disable_fadeout},
    {OPMSG_SWBS_DEC_ID_SET_FROM_AIR_INFO,          swbs_dec_opmsg_set_from_air_info},
    {OPMSG_SWBS_DEC_ID_FORCE_PLC_OFF,              swbs_dec_opmsg_force_plc_off},
    {OPMSG_SWBS_DEC_ID_FRAME_COUNTS,               swbs_dec_opmsg_frame_counts},
    {OPMSG_SWBS_DEC_ID_CODEC_MODE,                 swbs_dec_opmsg_codec_mode},
    {OPMSG_COMMON_ID_SET_CONTROL,                  swbs_dec_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,                   swbs_dec_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                 swbs_dec_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                   swbs_dec_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                   swbs_dec_opmsg_obpm_get_status},
#ifdef SCO_RX_OP_GENERATE_METADATA
    {OPMSG_COMMON_ID_SET_BUFFER_SIZE,             sco_common_rcv_opmsg_set_buffer_size},
    {OPMSG_COMMON_SET_TTP_LATENCY,                sco_common_rcv_opmsg_set_ttp_latency},
#endif
    {0, NULL}};


const CAPABILITY_DATA swbs_dec_cap_data =
    {
        SWBS_DEC_CAP_ID,             /* Capability ID */
        1,1,//SWBS_DEC_WB_VERSION_MAJOR, 1,   /* Version information - hi and lo parts */
        1, 1,                           /* Max number of sinks/inputs and sources/outputs */
        &swbs_dec_handler_table,        /* Pointer to message handler function table */
        swbs_dec_opmsg_handler_table,   /* Pointer to operator message handler function table */
        swbs_dec_process_data,          /* Pointer to data processing function */
        0,                              /* Reserved */
        sizeof(SWBS_DEC_OP_DATA)        /* Size of capability-specific per-instance data */
    };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_SWBS_DEC, SWBS_DEC_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_SWBS_DEC, SWBS_DEC_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */


/****************************************************************************
Private Function Definitions
*/
axErr_t swbs_decode_init_decoder_structs(SWBS_DEC_OP_DATA *op_data);
void OnPacketDecoded_swbs ( aptXDecodeEventType_t eType,  aptXDecodeEventUser_t* pUser, void* pvDecoderData );

static inline SWBS_DEC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SWBS_DEC_OP_DATA *) base_op_get_instance_data(op_data);\
}

/* ******************************* Helper functions ************************************ */

/* initialise various working data params of the specific operator */
static void swbs_dec_reset_working_data(OPERATOR_DATA *op_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* rcvData = &x_data->sco_rcv_op_data;

    patch_fn_shared(swbs_decode);

    if(rcvData != NULL)
    {
        /* Initialise fadeout-related parameters */
        rcvData->fadeout_parameters.fadeout_state = NOT_RUNNING_STATE;
        rcvData->fadeout_parameters.fadeout_counter = 0;
        rcvData->fadeout_parameters.fadeout_flush_count = 0;
    }


    /* clear first valid packet info */
    x_data->received_first_valid_pkt = 0;
#ifdef SCO_DEBUG
    x_data->swbs_dec_dbg_stats_enable = 0;
#endif

    /* Now reset the decoder - re-using old but slightly massaged function in ASM */
    swbs_dec_reset_aptx_data(op_data);
}


/* free the memory allocated for aptX Adaptive dec (shared and non-shared) */
static void swbs_dec_free_state_data(OPERATOR_DATA* op_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);

    patch_fn_shared(swbs_decode);

    if (x_data->codec_data != NULL)
    {
        /* free aptX adaptive internal tables */
        aptXDecode_KalimbaFreeMemory((void*)x_data->codec_data[0] ,(void*)&x_data->codec_data[0], 1);

        /* now free the codec data buffers */
        pdelete(x_data->codec_data[1]);
        pdelete(x_data->codec_data[2]);
#ifdef ESCO_SUPPORT_ERRORMASK
        pdelete(x_data->pBitErrorBuff);
#endif
    }
}



/* ********************************** API functions ************************************* */

/* TODO: a large part of this can be re-used from SCO RCV - so may move those out into a common helper function */
bool swbs_dec_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SWBS_DEC_OP_DATA* swbs_dec = get_instance_data(op_data);

    patch_fn_shared(swbs_decode);

    /* call base_op create, which also allocates and fills response message */
    if(!base_op_create(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    swbs_dec->md_bad_kick_attmpt_fake = 0;
    swbs_dec->md_bad_kick_faked = 0;

    /* Initialise some of the operator data that is common between NB and WB receive. It can only be called
     * after the PLC structure is allocated (if PLC present in build) */
    sco_common_rcv_initialise(&swbs_dec->sco_rcv_op_data);


    /* create aptX Adaptive data object */
    if (AX_OK != swbs_decode_init_decoder_structs(swbs_dec))
    {
        swbs_dec_free_state_data(op_data);
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* initialise some more SWBS decoder-specific data  */
    swbs_dec_reset_working_data(op_data);
    swbsdec_init_dec_param(op_data);

    return TRUE;
}


bool swbs_dec_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SWBS_DEC_OP_DATA* swbs_dec = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_rcv = &swbs_dec->sco_rcv_op_data;
    L2_DBG_MSG2("t = %08x: Destroying wbs_dec operator %04x", time_get_time(), base_op_get_ext_op_id(op_data));
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
    if(base_op_destroy(op_data, message_data, response_id, response_data))
    {
        /* now destroy all the capability specific data */
        swbs_dec_free_state_data(op_data);
        return TRUE;
    }
    return FALSE;
}


bool swbs_dec_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    if(!base_op_reset(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* now initialise specific working data */
    swbs_dec_reset_working_data(op_data);
    swbsdec_init_dec_param(op_data);
    return TRUE;
}


bool swbs_dec_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SWBS_DEC_OP_DATA *xdata = get_instance_data(op_data);

    patch_fn_shared(swbs_decode);

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
    if (   xdata->sco_rcv_op_data.buffers.ip_buffer == NULL
        || xdata->sco_rcv_op_data.buffers.op_buffer == NULL)
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* clear first valid packet info */
    xdata->received_first_valid_pkt = 0;

#ifdef SCO_RX_OP_GENERATE_METADATA
    /* connected endpoint tp sco rx endpoint is stored here, it's needed for its API call
     * to reset metadata when flushing buffer.
     */
    xdata->sco_rcv_op_data.sco_source_ep = stream_get_connected_endpoint_from_terminal_id(base_op_get_int_op_id(op_data), 0);
#endif

    L2_DBG_MSG2("t=%08x: swbs_dec operator %04x started", time_get_time(), base_op_get_ext_op_id(op_data));
    return TRUE;
}


bool swbs_dec_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SWBS_DEC_OP_DATA *x_data = get_instance_data(op_data);

    return sco_common_connect(op_data, message_data, response_id, response_data,
            &x_data->sco_rcv_op_data.buffers,
            NULL);
}


bool swbs_dec_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SWBS_DEC_OP_DATA *x_data = get_instance_data(op_data);

    return sco_common_disconnect(op_data, message_data, response_id, response_data,
                &(x_data->sco_rcv_op_data.buffers),
                NULL);
}

bool swbs_dec_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data)
{
    return sco_common_get_data_format(op_data, message_data, response_id, response_data,
            AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP_WITH_METADATA, AUDIO_DATA_FORMAT_FIXP);
}

bool swbs_dec_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data)
{
    return sco_common_get_sched_info(op_data, message_data, response_id, response_data,
            SWBS_DEC_DEFAULT_INPUT_BLOCK_SIZE, SWBS_DEC_DEFAULT_OUTPUT_BLOCK_SIZE);
}

/* ************************************* Data processing-related functions and wrappers **********************************/

void swbs_dec_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data;
    unsigned status;

    patch_fn(swbs_decode_process_data);

    sco_data = &x_data->sco_rcv_op_data;

    /* We can't have started in this state, so if we lose a buffer carry on
     * and hope that it comes back or we are stopped. If not then the error
     * (lack of data) should propagate to something that cares. */
    if (sco_data->buffers.ip_buffer == NULL || sco_data->buffers.op_buffer == NULL)
    {
        return;
    }

    /* call ASM function */
    status = swbs_dec_processing(op_data);

    /* Is fadeout enabled? if yes, do it on the current output data, if processing has actually produced output.
     * Now the slight migraine is that deep inside decoder, it may have decided to decode two frames - so the last guy that
     * really had to know the final outcome of how many samples were produced is PLC. Its packet size after return will be
     * the most reliable indicator of how many samples we need to process. If PLC is not installed, then WBS validate() function
     * is the only thing that tells the real story - of course, without PLC this would work in a very funny way.                    ///// CHECK THIS
     */
    if( (sco_data->fadeout_parameters.fadeout_state != NOT_RUNNING_STATE) && status)
    {
        /* the wrapper below takes output Cbuffer and fadeout params, and the current packet size in words is from PLC */
        if(mono_cbuffer_fadeout(sco_data->buffers.op_buffer,
                                x_data->swbs_dec_output_samples,
                                &(sco_data->fadeout_parameters)))
        {
            common_send_simple_unsolicited_message(op_data, OPMSG_REPLY_ID_FADEOUT_DONE);
        }
    }

    touched->sources = status;
}


/* **************************** Operator message handlers ******************************** */


bool swbs_dec_opmsg_enable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    common_set_fadeout_state(&x_data->sco_rcv_op_data.fadeout_parameters, RUNNING_STATE);

    return TRUE;
}

bool swbs_dec_opmsg_disable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    common_set_fadeout_state(&x_data->sco_rcv_op_data.fadeout_parameters, NOT_RUNNING_STATE);

    return TRUE;
}


/* TODO: This function can be lifted into a sco_fw "sco_common.c" or similar, provided that wbs_dec_build_simple_response
 * is also lifted out there. The trick is that the extra op data parts it refers to are actually identical between NB and WB
 * operator data structs intentionally for such situations!
 * Therefore WB and/or NB SCO can make use of this as common sco helper function.
 */
bool swbs_dec_opmsg_set_from_air_info(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    return sco_common_rcv_set_from_air_info_helper(op_data, &x_data->sco_rcv_op_data,
                                                   message_data);
}


bool swbs_dec_opmsg_force_plc_off(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    return sco_common_rcv_force_plc_off_helper(&x_data->sco_rcv_op_data, message_data);
}


bool swbs_dec_opmsg_frame_counts(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    return sco_common_rcv_frame_counts_helper(&x_data->sco_rcv_op_data,
                                              message_data, resp_length, resp_data);

}

bool swbs_dec_opmsg_codec_mode(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
  //  patch_fn(swbs_dec_opmsg_codec_mode_helper);

    SWBS_DEC_OP_DATA* swbs_dec_params = get_instance_data(op_data);
    APTX_DECODE_STATE pDecodeState = (APTX_DECODE_STATE)swbs_dec_params->codec_data[0];

    if(OPMSG_FIELD_GET(message_data, OPMSG_SWBS_DEC_CODEC_MODE, CODEC_MODE) == SWBS_CODEC_MODE4)
    {
        /* set codec mode to Mode 4 (24kHz) */
        swbs_dec_params->codecMode = SWBS_CODEC_MODE4;
    }
    else
    {
        /* set codec mode to Mode 0 (32kHz) */
        swbs_dec_params->codecMode = SWBS_CODEC_MODE0;
    }
    aptXDecode_updateSpeechMode(pDecodeState,swbs_dec_params->codecMode);

    return TRUE;
}

bool swbs_dec_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* In the case of this capability, nothing is done for control message. Just follow protocol and ignore any content. */
    return cps_control_setup(message_data, resp_length, resp_data,NULL);
}


bool swbs_dec_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    return FALSE;
}

bool swbs_dec_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
   SWBS_DEC_OP_DATA* swbs_dec = get_instance_data(op_data);

   return cpsGetDefaultsMsgHandler(&swbs_dec->sco_rcv_op_data.parms_def,message_data, resp_length,resp_data);
}

bool swbs_dec_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Set the parameter(s). For future proofing, it is using the whole mechanism, although currently there is only one field
     * in opdata structure that is a setable parameter. If later there will be more (ever), must follow contiquously the first field,
     * as commented and instructed in the op data definition. Otherwise consider moving them into a dedicated structure.
     */
//   SWBS_DEC_OP_DATA* swbs_dec = get_instance_data(op_data);
//   return cpsSetParameterMsgHandler(&swbs_dec->sco_rcv_op_data.parms_def,message_data, resp_length,resp_data);

    return FALSE;
}

bool swbs_dec_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_DEC_OP_DATA* swbs_dec_params = get_instance_data(op_data);
    SCO_RCV_PARAMS* sco_rcv_params = &swbs_dec_params->sco_rcv_op_data.sco_rcv_parameters;
    unsigned* resp;

    if(!common_obpm_status_helper(message_data,resp_length,resp_data,sizeof(SWBS_DEC_STATISTICS),&resp))
    {
         return FALSE;
    }

    /* Fill the statistics as needed */
    if(resp)
    {
        resp = cpsPack2Words(sco_rcv_params->sco_pkt_size, sco_rcv_params->t_esco, resp);
        resp = cpsPack2Words(sco_rcv_params->frame_count, sco_rcv_params->frame_error_count, resp);
        resp = cpsPack2Words(sco_rcv_params->md_late_pkts, sco_rcv_params->md_early_pkts, resp);
        resp = cpsPack2Words(sco_rcv_params->out_of_time_pkt_cnt, sco_rcv_params->md_out_of_time_reset, resp);
        resp = cpsPack2Words(swbs_dec_params->swbs_dec_no_output, swbs_dec_params->swbs_dec_fake_pkt, resp);
        resp = cpsPack2Words(swbs_dec_params->swbs_dec_good_output, swbs_dec_params->swbs_dec_bad_output, resp);
    }

    return TRUE;
}

axErr_t swbs_decode_init_decoder_structs(SWBS_DEC_OP_DATA *aptx_data)
{
    aptXConfig_t       decodeConfig ;
    aptXDecodeSetup_t  decodeSetup;

    patch_fn_shared(swbs_decode);

    void * pAllocatedMemory = NULL;
    void * pAsmStruct= NULL;
    APTX_DECODE_STATE pDecodeState = NULL;

    axBuf_t *pInBuf  = &aptx_data->axInBuf;
    axBuf_t *pOutBuf = &aptx_data->axOutBuf;

    axErr_t err;

#ifdef ESCO_SUPPORT_ERRORMASK
    aptx_data->pBitErrorBuff = xzppmalloc(SWBS_BIT_ERROR_BUF_SIZE, MALLOC_PREFERENCE_NONE);   // Dummy bit error buffer
#endif
    decodeSetup.decodeMode       = APTX_DECODE_MODE_NORMAL;
    decodeSetup.channelReference = APTX_SELECT_STEREO;
    decodeSetup.requestedProfile = APTX_PROFILE_SPEECH          ;
    decodeSetup.bDisableSyncLock = TRUE;

    aptx_data->codec_data[1] = xzppmalloc(SWBS_DEC_INPUT_BUFFER_SIZE, MALLOC_PREFERENCE_DM1);    // Input Buffer (read only)
    if (NULL == aptx_data->codec_data[1])
      return AX_ERROR;
    aptx_data->codec_data[2] = xzppmalloc(SWBS_DEC_OUTPUT_BUFFER_SIZE, MALLOC_PREFERENCE_DM2);   // Output Buffer
    if (NULL == aptx_data->codec_data[2])
      return AX_ERROR;


    aptx_data->num_out_channels = 1;

    pAsmStruct=&aptx_data->codec_data[0];
    pAllocatedMemory = aptXDecode_KalimbaAllocMemory(pAsmStruct, 1);
    if (NULL == pAllocatedMemory)
      return AX_ERROR;

    aptx_data->packet_size = 60;

    err = aptXDecode_Create( &pDecodeState, &decodeSetup, aptx_data->codecMode, pAllocatedMemory ) ;

    aptXDecode_SetEvent ( pDecodeState, APTX_DECODE_EVENT_PACKET_DECODED, OnPacketDecoded_swbs, (void *)aptx_data, NULL );

    int8_t * pBuffer1 = (int8_t*)aptx_data->codec_data[1];
    int8_t * pBuffer2 = (int8_t*)aptx_data->codec_data[2];
    aptx_data->codec_data[0] = pDecodeState;

   // Create the decoderInput axBuf buffer
    AxBufInit( pInBuf) ;
    pInBuf->dat.pn32 = (int32_t*)pBuffer1;
    pInBuf->nFill    = SWBS_DEC_INPUT_BUFFER_SIZE; // MAX_INPUT_BUFFER_SIZE ;
    pInBuf->nMax     = SWBS_DEC_INPUT_BUFFER_SIZE; // MAX_INPUT_BUFFER_SIZE ;
    pInBuf->fmt.flow = AXFMT_LINEAR;
    pInBuf->fmt.type = AXFMT_NOTDEFINED;
    pInBuf->fmt.nSpan = 1;


    //Create the decoderOutput axBuf buffer
    AxBufInit( pOutBuf ) ;
    pOutBuf->dat.pn32 = (int32_t*)pBuffer2;
    pOutBuf->nFill    = SWBS_DEC_OUTPUT_BUFFER_SIZE; // MAX_OUTPUT_BUFFER_SIZE ;
    pOutBuf->nMax     = SWBS_DEC_OUTPUT_BUFFER_SIZE; // MAX_OUTPUT_BUFFER_SIZE ;
    pOutBuf->fmt.flow = AXFMT_LINEAR;
    pOutBuf->fmt.type = AXFMT_STEREO_32BIT;
    pOutBuf->fmt.nSpan = 1;


    if (aptXDecode_GetConfig( pDecodeState, &decodeConfig ) != AX_OK)
    {
        return AX_ERROR;
    }

    return err;
}

void OnPacketDecoded_swbs ( aptXDecodeEventType_t eType,  aptXDecodeEventUser_t* pUser, void* pvDecoderData )
{
    SWBS_DEC_OP_DATA *pCodecData = (SWBS_DEC_OP_DATA *)pUser->pvContext;

    pCodecData->storedWP = pCodecData->axOutBuf.nWPtr;
    pCodecData->overlap_finished = TRUE;
}

/**
 * \brief The SWBS codec works with either 24k or 32k audio streams. This is
 *        selected after creation, setting OPMSG_SWBS_DEC_ID_CODEC_MODE.
 *        There will be two corresponding CONNECTION_TYPEs (UWBS and SWBS)
 *        needed by the sco_fw to process metadata.
 *
 * \param opdata SWBS operator data.
 *
 * \return       UWBS if the codec is configured for 24k audio
 *               SWBS if the codec is configured for 32k audio
 */
CONNECTION_TYPE swbs_get_sco_connection_type(OPERATOR_DATA *op_data)
{
    SWBS_DEC_OP_DATA* swbs_dec_params = get_instance_data(op_data);

    if (swbs_dec_params->codecMode == SWBS_CODEC_MODE4)
    {
        return UWBS;
    }
    else
    {
        return SWBS;
    }
}

