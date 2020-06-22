/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  swbs_enc.c
 * \ingroup  operators
 *
 *  SWBS_ENC operator
 *
 */

/****************************************************************************
Include Files
*/
#include "swbs_private.h"

#include "patch/patch.h"


#include "aptXAdaptiveTypes.h"
#include "aptXAdaptiveEncode.h"
#include "aptXAdaptiveEncoderMemory.h"
/****************************************************************************
Private Type Definitions
*/

/****************************************************************************
Private Constant Declarations
*/

/** The SWBS encoder capability function handler table */
const handler_lookup_struct swbs_enc_handler_table =
{
    swbs_enc_create,           /* OPCMD_CREATE */
    swbs_enc_destroy,          /* OPCMD_DESTROY */
    base_op_start,             /* OPCMD_START */
    base_op_stop,              /* OPCMD_STOP */
    swbs_enc_reset,            /* OPCMD_RESET */
    swbs_enc_connect,          /* OPCMD_CONNECT */
    swbs_enc_disconnect,       /* OPCMD_DISCONNECT */
    swbs_buffer_details,       /* OPCMD_BUFFER_DETAILS */
    swbs_enc_get_data_format,  /* OPCMD_DATA_FORMAT */
    swbs_enc_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table */
const opmsg_handler_lookup_table_entry swbs_enc_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,      base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE,               swbs_enc_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE,              swbs_enc_opmsg_disable_fadeout},
    {OPMSG_SCO_SEND_ID_SET_TO_AIR_INFO,            swbs_enc_opmsg_set_to_air_info},
    {OPMSG_SWBS_ENC_ID_CODEC_MODE,                 swbs_enc_opmsg_codec_mode},
    {0, NULL}};


const CAPABILITY_DATA swbs_enc_cap_data =
    {
        SWBS_ENC_CAP_ID,                /* Capability ID */
        0, 1,                           /* Version information - hi and lo parts */
        1, 1,                           /* Max number of sinks/inputs and sources/outputs */
        &swbs_enc_handler_table,        /* Pointer to message handler function table */
        swbs_enc_opmsg_handler_table,   /* Pointer to operator message handler function table */
        swbs_enc_process_data,          /* Pointer to data processing function */
        0,                              /* Reserved */
        sizeof(SWBS_ENC_OP_DATA)        /* Size of capability-specific per-instance data */
    };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_SWBS_ENC, SWBS_ENC_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_SWBS_ENC, SWBS_ENC_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */


/****************************************************************************
Private Function Definitions
*/
axErr_t swbs_encode_init_encoder_structs(SWBS_ENC_OP_DATA *op_data);
static inline SWBS_ENC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SWBS_ENC_OP_DATA *) base_op_get_instance_data(op_data);
}

/* ******************************* Helper functions ************************************ */


/* initialise various working data params of the specific operator */
static void swbs_enc_reset_working_data(OPERATOR_DATA *op_data)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);

    patch_fn_shared(swbs_encode);

    if(x_data != NULL)
    {
        /* Initialise fadeout-related parameters */
        x_data->fadeout_parameters.fadeout_state = NOT_RUNNING_STATE;
        x_data->fadeout_parameters.fadeout_counter = 0;
        x_data->fadeout_parameters.fadeout_flush_count = 0;
    }

    /* Now reset the encoder - re-using old but slightly massaged function in ASM */
    swbs_enc_reset_aptx_data(op_data);
}


/* free the memory allocated for SBC encoder (shared and non-shared) */
static void swbs_enc_free_state_data(OPERATOR_DATA* op_data)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);
    patch_fn_shared(swbs_encode);
    if (x_data->codec_data != NULL)
    {
        /* free the codec data object */
        aptXEncode_KalimbaFreeMemory((void*)x_data->codec_data[0] ,(void*)&x_data->codec_data[0], 1);
                
        /* now free the codec data buffers */
        pdelete(x_data->codec_data[1]);
        pdelete(x_data->codec_data[2]);
    }
}



/* ********************************** API functions ************************************* */

bool swbs_enc_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SWBS_ENC_OP_DATA* swbs_enc = get_instance_data(op_data);

    patch_fn_shared(swbs_encode);

    /* call base_op create, which also allocates and fills response message */
    if(!base_op_create(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

      /* create aptX Adaptive data object */
	if (AX_OK != swbs_encode_init_encoder_structs(swbs_enc))
    {
        swbs_enc_free_state_data(op_data);
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }
	
	
    /* initialise some more SWBS encoder-specific data  */
    swbs_enc_reset_working_data(op_data);
    swbsenc_init_encoder(op_data);
    return TRUE;
}


bool swbs_enc_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    if(base_op_destroy(op_data, message_data, response_id, response_data))
    {
        /* now destroy all the aptX encoder and shared codec paraphernalia */
        swbs_enc_free_state_data(op_data);
        return TRUE;
    }

    return FALSE;
}


bool swbs_enc_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    if(!base_op_reset(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* now initialise specific working data */
    swbs_enc_reset_working_data(op_data);

    return TRUE;
}

bool swbs_enc_connect(OPERATOR_DATA *op_data, void *message_data,
        unsigned *response_id, void **response_data)
{
    SWBS_ENC_OP_DATA* swbs_enc = get_instance_data(op_data);
    return sco_common_connect(op_data, message_data, response_id, response_data,
            &swbs_enc->buffers, NULL);
}

bool swbs_enc_disconnect(OPERATOR_DATA *op_data, void *message_data,
        unsigned *response_id, void **response_data)
{
    SWBS_ENC_OP_DATA* swbs_enc = get_instance_data(op_data);
    return sco_common_disconnect(op_data, message_data, response_id, response_data,
            &swbs_enc->buffers, NULL);
}

bool swbs_enc_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data)
{
    return sco_common_get_data_format(op_data, message_data, response_id, response_data,
            AUDIO_DATA_FORMAT_FIXP, AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP);
}

bool swbs_enc_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data)
{
    return sco_common_get_sched_info(op_data, message_data, response_id, response_data,
            SWBS_ENC_DEFAULT_INPUT_BLOCK_SIZE, SWBS_ENC_DEFAULT_OUTPUT_BLOCK_SIZE);
}

/* ************************************* Data processing-related functions and wrappers **********************************/

void swbs_enc_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);
    unsigned available_space, samples_to_process;
    unsigned count = 0, max_count;

    patch_fn(swbs_encode_process_data);

    /* work out amount of input data to process, based on output space and input data amount */
    available_space = cbuffer_calc_amount_space_in_words(x_data->buffers.op_buffer);
    samples_to_process = cbuffer_calc_amount_data_in_words(x_data->buffers.ip_buffer);

    /* Loop until we've done all we need to.
     * The frame size should be a multiple of the output block size. Round up anyway just in case
     */
    max_count = (x_data->frame_size + SWBS_ENC_DEFAULT_OUTPUT_BLOCK_SIZE - 1)/SWBS_ENC_DEFAULT_OUTPUT_BLOCK_SIZE;

    while ((available_space >= SWBS_ENC_DEFAULT_OUTPUT_BLOCK_SIZE) &&
           (samples_to_process >= SWBS_ENC_DEFAULT_INPUT_BLOCK_SIZE) &&
           (count < max_count))
    {
        /* Is fadeout enabled? if yes, do it on the current input data */
        if(x_data->fadeout_parameters.fadeout_state != NOT_RUNNING_STATE)
        {
            /* the wrapper below takes output Cbuffer and fadeout params, use input block size */
            if(mono_cbuffer_fadeout(x_data->buffers.ip_buffer, SWBS_ENC_DEFAULT_INPUT_BLOCK_SIZE,
                                     &(x_data->fadeout_parameters)))
            {
                common_send_simple_unsolicited_message(op_data, OPMSG_REPLY_ID_FADEOUT_DONE);
            }
        }

        swbsenc_process_frame(op_data);

        available_space -= SWBS_ENC_DEFAULT_OUTPUT_BLOCK_SIZE;
        samples_to_process -= SWBS_ENC_DEFAULT_INPUT_BLOCK_SIZE;

        touched->sources =  TOUCHED_SOURCE_0;
        count++;
    }

    /* Check if we want to kick the endpoint on Tesco rate (means forward all kicks) */
    if(x_data->forward_all_kicks_enabled)
    {
        touched->sources =  TOUCHED_SOURCE_0;
    }

    if (samples_to_process < SWBS_ENC_DEFAULT_INPUT_BLOCK_SIZE)
    {
        /* If there isn't enough data to process another frame kick backwards */
        touched->sinks = TOUCHED_SINK_0;
    }

}


/* **************************** Operator message handlers ******************************** */


bool swbs_enc_opmsg_enable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);
    common_set_fadeout_state(&x_data->fadeout_parameters, RUNNING_STATE);

    return TRUE;
}

bool swbs_enc_opmsg_disable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);
    common_set_fadeout_state(&x_data->fadeout_parameters, NOT_RUNNING_STATE);

    return TRUE;
}

bool swbs_enc_opmsg_set_to_air_info(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);

    x_data->frame_size = OPMSG_FIELD_GET(message_data, OPMSG_WBS_ENC_SET_TO_AIR_INFO, FRAME_SIZE);

    return TRUE;
}

bool swbs_enc_opmsg_codec_mode(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
  //  patch_fn(swbs_dec_opmsg_codec_mode_helper);

    SWBS_ENC_OP_DATA* swbs_enc_params = get_instance_data(op_data);
    void* pEncodeState = (void*)swbs_enc_params->codec_data[0];

    if(OPMSG_FIELD_GET(message_data, OPMSG_SWBS_ENC_CODEC_MODE, CODEC_MODE) == SWBS_CODEC_MODE4)
    {
        /* set codec mode to Mode 4 (24kHz) */
        swbs_enc_params->codecMode = SWBS_CODEC_MODE4;
    }
    else
    {
        /* set codec mode to Mode 0 (32kHz) */
        swbs_enc_params->codecMode = SWBS_CODEC_MODE0;
    }
	aptXEncode_updateSpeechMode(pEncodeState,swbs_enc_params->codecMode);

    return TRUE;
}

/*
 * Notes on motivation to introduce message OPMSG_WBS_ENC_ID_FORWARD_ALL_KICKS:
 *
 * In case of WBS encoded transmission via SCO, mainly 2 configurations are considered:
 *    case 1: Tesco=12, SOC_PACKET_SIZE=60 Bytes (default config, 2-EV3) --> Tesco period = 12 x 625us = 7500us
 *    case 2: Tesco=6, SCO_PACKET_SIZE=30 Bytes (EV3) --> Tesco period = 6 x 625us = 3750us
 *
 * The SWBS_ENCODER operator is kicked at its input on the Tesco rate by a kicking object.
 * Despite of that and before introducing OPMSG_WBS_ENC_ID_FORWARD_ALL_KICKS to WBS_ENC capability, the WBS_ENCODER operator kicked itself at its output
 * the subsequent SCO_SINK_EP on a fix rate of 7500us (= 12 x 625 us) and delivers (multiple of) 60 Bytes independent whether Tesco is 12 or 6.
 * 
 * In case 1.) this is actually fine. 
 * Even in case 2.) this might be OK as long as SCO_SINK_EP considers in-place processing (one buffer for in and output), 
 * even though in the 2nd case, we get twice the amount of Bytes (2 SCO packets comprising 60 Bytes instead of 1 with 30 Bytes) 
 * on half the Tesco related rate (every 7500us instead of every 3750us).
 * 
 * However, there is an issue in case that SCO_SINK_EP delivers the received data 'SCO packet wise' (with SCO_PACKET_SIZE=30 Bytes) to a 
 * double TX output buffer (each of size=SCO_PACKET_SIZE) and doesn't have a ring buffer (where we could write as much as available):
 * In order to write to one of these TX output buffers by AUDIO_SS (resp. SCO_SINK_EP) whereas the BT_SS reads from the other one, 
 * the SCO_SINK_EP can only write (max.) 1 SCO packet per kick to the TX buffer and needs to be kicked on the Tesco related rate.
 * In particular, to preserve the double buffering principle, it cannot write two SCO packets in one kick on half the rate...
 *
 * Accordingly, the OPMSG_WBS_ENC_ID_FORWARD_ALL_KICKS message was introduced to configure the SWBS_ENCODER operator in a way that it forwards all 
 * kicks from its input to its output in order to get the subsequent SCO_SINK_EP kicked on the Tesco related rate.
 *
 * By default, OPMSG_WBS_ENC_ID_FORWARD_ALL_KICKS = FALSE (means, behavior as before introducing this message)
 *
 */
bool swbs_enc_opmsg_forward_all_kicks(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);
    L4_DBG_MSG("swbs_enc_opmsg_forward_all_kicks  \n");

    x_data->forward_all_kicks_enabled = ((unsigned*)message_data)[3];

    return TRUE;
}


axErr_t swbs_encode_init_encoder_structs(SWBS_ENC_OP_DATA *aptx_data)
{
    void * pAllocatedMemory = NULL;
    void*  pEncoderState = NULL  ;
    void * pAsmStruct = NULL;

    aptXParameters_t aptXParams;
	axErr_t err          = AX_OK ;

    patch_fn_shared(swbs_encode);

    aptXChannelConfig_t channelConfig = APTX_CHANNEL_MONO;

	  //Set up the aptXParameters struct for Super Wideband Speech
    aptXParams.primaryProfile   = APTX_PROFILE_SPEECH       ;
    aptXParams.secondaryProfile = APTX_PROFILE_NONE        ;
    aptXParams.channelConfig    = channelConfig           ;
    aptXParams.uSampleRate      = 32000      ;                       // This should be a modifiable parameter.
    aptXParams.uBitRate         = 64000      ;                       // This should be a modifiable parameter.
    aptXParams.processMode      = APTX_PROCESS_MODE_FIXED ;
	
    pAsmStruct=(void*)&aptx_data->codec_data[0];
    pAllocatedMemory = aptXEncode_KalimbaAllocMemory(pAsmStruct, &aptXParams, 1);
    if( NULL == pAllocatedMemory )
      return AX_ERROR ;

    err = aptXEncode_Create( &pEncoderState, &aptXParams, aptx_data->codecMode, pAllocatedMemory ) ;
    if( AX_OK != err )
      return err ;
	
    axBuf_t *pInBuf  = &aptx_data->axInBuf;
    axBuf_t *pOutBuf = &aptx_data->axOutBuf;
    

    aptx_data->codec_data[0] = pEncoderState;
    aptx_data->codec_data[1] = xzppmalloc(SWBS_ENC_INPUT_BUFFER_SIZE, MALLOC_PREFERENCE_DM1);    // Input Buffer (read only)
    if( NULL == aptx_data->codec_data[1] )
      return AX_ERROR ;
    aptx_data->codec_data[2] = xzppmalloc(SWBS_ENC_OUTPUT_BUFFER_SIZE, MALLOC_PREFERENCE_DM2);   // Output Buffer
    if( NULL == aptx_data->codec_data[2] )
      return AX_ERROR ;

    int8_t * pBuffer1 = (int8_t*)aptx_data->codec_data[1];
    int8_t * pBuffer2 = (int8_t*)aptx_data->codec_data[2];
	
	//Create the encoderInput buffer
    AxBufInit( pInBuf) ;
    pInBuf->dat.pn32 = (int32_t*)pBuffer1;
    pInBuf->nRPtr    = (int32_t)pInBuf->dat.pn32;
    pInBuf->nWPtr    = (int32_t)pInBuf->dat.pn32;
    pInBuf->nFill    = (int32_t)pInBuf->dat.pn32 + SWBS_ENC_INPUT_BUFFER_SIZE ;
    pInBuf->nMax     = (int32_t)pInBuf->dat.pn32 + SWBS_ENC_INPUT_BUFFER_SIZE ;
    pInBuf->fmt.flow = AXFMT_LINEAR;
    pInBuf->fmt.type = AXFMT_NOTDEFINED;
    pInBuf->fmt.nSpan = 1;

	//Create the encoderOutput buffer
    AxBufInit( pOutBuf ) ;
    pOutBuf->dat.pn32 = (int32_t*)pBuffer2;
    pOutBuf->nFill    = (int32_t)pOutBuf->dat.pn32 + SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES ;
    pOutBuf->nMax     = (int32_t)pOutBuf->dat.pn32 + (SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES + 32) ;
    pOutBuf->fmt.flow = AXFMT_LINEAR;
    pOutBuf->fmt.type = AXFMT_STEREO_32BIT;
    pOutBuf->fmt.nSpan = 1;
	
    return err;
}
