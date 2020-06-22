/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  conv_from_audio.c
 * \ingroup  capabilities
 *
 *  Convert audio samples to 16-bit
 *  TODO stereo (multichan), interleaving?
 *
 */

#include "conv_from_audio.h"
#include "conv_from_audio_struct.h"

/****************************************************************************
Private Type Definitions
*/

/* convert_from_audio based on ENCODER infrastructure,
    here is the capability-specific per-instance data */
typedef struct
{
    /** PARAMS must be the first parameters always */
    ENCODER_PARAMS    params;

    /** codec specific data */
    conv_from_audio_codec codec_data;

} CONVERT_FROM_AUDIO_OP_DATA;


/****************************************************************************
Private Function Declarations
*/

#define CONV_KEEP_MS_HWORD(w)   (w & 0xFFFF0000UL)

static inline unsigned *adjust_cbuffer_ptr(unsigned *p, tCbuffer *cb, unsigned cb_len)
{
    if (p >= (unsigned *) (cb->base_addr) + cb_len)
    {
        p -= cb_len;    /* circular buffer adjust pointers */
    }
    return p;
}

static bool conv_from_audio_frame_sizes(OPERATOR_DATA *op_data,
                         unsigned *in_size_samples, unsigned *out_size_octets);


/****************************************************************************
Private Constant Declarations
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define CONVERT_FROM_AUDIO_CAP_ID CAP_ID_DOWNLOAD_CONVERT_FROM_AUDIO
#else
#define CONVERT_FROM_AUDIO_CAP_ID CAP_ID_CONVERT_FROM_AUDIO
#endif

/** The stub capability function handler table */
const handler_lookup_struct conv_from_audio_handler_table =
{
    conv_from_audio_create,   /* OPCMD_CREATE */
    encoder_destroy,          /* OPCMD_DESTROY */
    encoder_start,            /* OPCMD_START */
    base_op_stop,             /* OPCMD_STOP */
    encoder_reset,            /* OPCMD_RESET */
    encoder_connect,          /* OPCMD_CONNECT */
    encoder_disconnect,       /* OPCMD_DISCONNECT */
    encoder_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    encoder_get_data_format,  /* OPCMD_DATA_FORMAT */
    encoder_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry conv_from_audio_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {0, NULL}
};


/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA convert_from_audio_cap_data =
{
	CONVERT_FROM_AUDIO_CAP_ID,			/* Capability ID */
    0, 1,                           	/* Version information - hi and lo parts */
    1, 1,                           	/* Max number of sinks/inputs and sources/outputs TODO */
    &conv_from_audio_handler_table,     /* Pointer to message handler function table */
    conv_from_audio_opmsg_handler_table,/* Pointer to operator message handler function table */
    encode_process_data,            	/* Pointer to data processing function */
    0,                              	/* Reserved */
    sizeof(CONVERT_FROM_AUDIO_OP_DATA)  /* Size of capability-specific per-instance data */
};

#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CONVERT_FROM_AUDIO, CONVERT_FROM_AUDIO_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CONVERT_FROM_AUDIO, CONVERT_FROM_AUDIO_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

const ENCODER_CAP_VIRTUAL_TABLE conv_from_audio_vt =
{
    (ENCODE_FN)conv_from_audio_frame_process_asm,
    NULL,                       	/* free data fn */
    NULL,                       	/* reset_fn */
    conv_from_audio_frame_sizes,    /* fn reports frame sizes */
    NULL                        	/* &conv_from_audio_scratch_table, */
};

/***************************************************************************
Private Function Declarations
*/

static inline CONVERT_FROM_AUDIO_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (CONVERT_FROM_AUDIO_OP_DATA *) base_op_get_instance_data(op_data);
}

/**
 * \brief Frees up any data that has been allocated by the operator
 * and sets the response field to failed.
 *
 * \param op_data Pointer to the operator instance data.
 * \param response Pointer to the response message to give a failed status
 */
static void free_data_and_fail(OPERATOR_DATA *op_data, void **response)
{
    patch_fn_shared(convert_from_audio_cap);

    /* Free the data and then override the response message status to fail
        (although there is no data to free for this cap) */
    base_op_change_response_status(response, STATUS_CMD_FAILED);
}


/**
 * \brief Allocates specific capability memory and initialises the operator.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool conv_from_audio_create(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *response_id, void **response_data)
{
	CONVERT_FROM_AUDIO_OP_DATA *conv_from_audio_data = get_instance_data(op_data);

	patch_fn_shared(convert_from_audio_cap);

    /* call base_op create, which also allocates and fills response message */
    if (!base_op_create(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* Create the link to the base class object */
    if (!encoder_base_class_init(op_data, &conv_from_audio_data->params, &(conv_from_audio_data->codec_data), &conv_from_audio_vt))
    {
        free_data_and_fail(op_data, response_data);
        return TRUE;
    }

    /* Init default encoding values */
    conv_from_audio_data->codec_data.samples_to_convert = 0;

    return TRUE;
}


/**
 * \brief Frame processing function.
 * Converts 32-bit words to 16-bit format.
 * TODO Currently only mono 16-bit implemented.
 *
 * \param op_data Pointer to the operator instance data.
 */
void conv_from_audio_frame_process(ENCODER_PARAMS *params)
{
    unsigned amount_in, space;
    unsigned *rdp, *wrp;
    unsigned wo;            /* octet offset */
    unsigned in_left_len, out_len;
    unsigned conv_w;
#ifdef DATAFORMAT_32
    unsigned conv_2w, conv_2w_sav;
#endif

    ENCODER e;
    conv_from_audio_codec *c;

    patch_fn_shared(convert_from_audio_cap);

    e = params->codec;
    c = e.encoder_data_object;

    amount_in = cbuffer_calc_amount_data_in_words(e.in_left_buffer);
    space = cbuffer_calc_amount_space_in_words(e.out_buffer);
    conv_w = MIN(amount_in, space);
    /* record samples_to_convert in cap_specific op_data */
    c->samples_to_convert = conv_w;

    if (0 == conv_w)
    {
        return;
    }

    rdp = (unsigned *) (e.in_left_buffer)->read_ptr;
    /* the output buffer is an octet buffer so use the _ex API */
    wrp = cbuffer_get_write_address_ex(e.out_buffer, &wo);
    PL_ASSERT( 0 == (wo & 1) );    /* (an odd offset is not allowed, must be half-word aligned) */
    in_left_len = cbuffer_get_size_in_words(e.in_left_buffer);
    out_len = cbuffer_get_size_in_words(e.out_buffer);

#ifdef DATAFORMAT_32

    /* might be a half-word offset to start with */
    if (wo == 2)
    {
        unsigned wtmp;
        
        wtmp = CONV_KEEP_MS_HWORD(*wrp);    /* first read what's already there (and then mix it) */
        *wrp++ = wtmp | ((*rdp++)>>16);
        rdp = adjust_cbuffer_ptr(rdp, e.in_left_buffer, in_left_len);
        wrp = adjust_cbuffer_ptr(wrp, e.out_buffer, out_len);
        conv_w--;                   /* one hword done */
    }

    /* deal in pairs of words, might be odd number, check at the end */
    conv_2w = conv_w/2;
    conv_2w_sav = conv_2w;
    while (conv_2w-- > 0)
    {
        unsigned wtmp;
        
        /* pack two samples, first goes MS 16-bit */
        wtmp = (*rdp++)>>16;
        rdp = adjust_cbuffer_ptr(rdp, e.in_left_buffer, in_left_len);

        *wrp++ = (wtmp <<16) + ((*rdp++)>>16);
        rdp = adjust_cbuffer_ptr(rdp, e.in_left_buffer, in_left_len);
        wrp = adjust_cbuffer_ptr(wrp, e.out_buffer, out_len);
    }
    cbuffer_set_write_address(e.out_buffer, wrp);
    if (2*conv_2w_sav != conv_w)
    {
        /* it was odd number of samples, convert last one
           it doesn't matter we overwrite LS 16-bit, amount_space will always report one word less */
        *wrp = CONV_KEEP_MS_HWORD(*rdp++);
        rdp = adjust_cbuffer_ptr(rdp, e.in_left_buffer, in_left_len);
        cbuffer_advance_write_ptr_ex(e.out_buffer, 2);
    }

#else

    while (conv_w-- > 0)
    {
        *wrp++ = (*rdp++)>>16;
        rdp = adjust_cbuffer_ptr(rdp, e.in_left_buffer, in_left_len);
        wrp = adjust_cbuffer_ptr(wrp, e.out_buffer, out_len);
    }
    cbuffer_set_write_address(e.out_buffer, wrp);

#endif  /* DATAFORMAT_32 */

    cbuffer_set_read_address(e.in_left_buffer, rdp);
}

/**
 * \brief reports input and output frame sizes
 *
 * \param in_size_samples pointer to number of samples consumed per frame (return value)
 * \param out_size_octets pointer to number of octets produced per frame (return value)
 *
 * \return FALSE because this is a sample-based encoder
 */
static bool conv_from_audio_frame_sizes(OPERATOR_DATA *op_data, unsigned *in_size_samples, unsigned *out_size_octets)
{
    patch_fn_shared(convert_from_audio_cap);

    /* Each input sample is converted to 2 octets */
    *in_size_samples = 1;
    *out_size_octets = 2;

    return FALSE;
}
