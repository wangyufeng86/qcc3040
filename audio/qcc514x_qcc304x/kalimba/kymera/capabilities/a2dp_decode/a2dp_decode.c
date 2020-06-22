/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  a2dp_decode.c
 * \ingroup  capabilities
 *
 * Common framework for A2DP codecs to use.
 *
 */

 /****************************************************************************
Include Files
*/
#include "capabilities.h"
#include "codec_c.h"
#include "a2dp_common_decode.h"
#include "mem_utils/shared_memory_ids.h"
#include "fault/fault.h"

/****************************************************************************
Public Constant Declarations
*/

const opmsg_handler_lookup_table_entry a2dp_decode_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE, a2dp_dec_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE, a2dp_dec_opmsg_disable_fadeout},
    {0, NULL}
};

/****************************************************************************
Private Function Definitions
*/
static inline A2DP_DECODER_PARAMS *get_instance_data(OPERATOR_DATA *op_data)
{
    return (A2DP_DECODER_PARAMS *) base_op_get_instance_data(op_data);
}

/****************************************************************************
Public Function Declarations
*/

/**
 * \brief Connects a capability terminal to a buffer.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the connect request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
 bool a2dp_decode_connect(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *response_id, void **response_data)
{
    A2DP_DECODER_PARAMS *decoder_data = get_instance_data(op_data);
    unsigned terminal_id;

    /* Check that the capability is not running */
    if (opmgr_op_is_running(op_data))
    {
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }

    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);

    switch (terminal_id)
    {
        case INPUT_TERMINAL_ID:
            decoder_data->codec.in_buffer = OPMGR_GET_OP_CONNECT_BUFFER(message_data);
            break;
        case LEFT_OUT_TERMINAL_ID:
            decoder_data->op_out_left = OPMGR_GET_OP_CONNECT_BUFFER(message_data);
#ifdef INSTALL_METADATA
            if (decoder_data->metadata_op_buffer == NULL)
            {
                if (buff_has_metadata(decoder_data->op_out_left))
                {
                    decoder_data->metadata_op_buffer = decoder_data->op_out_left;
                }
            }
#endif /* INSTALL_METADATA */

            decoder_data->codec.out_left_buffer = decoder_data->op_out_left;
            break;
        case RIGHT_OUT_TERMINAL_ID:
            decoder_data->op_out_right = OPMGR_GET_OP_CONNECT_BUFFER(message_data);
#ifdef INSTALL_METADATA
            if (decoder_data->metadata_op_buffer == NULL)
            {
                if (buff_has_metadata(decoder_data->op_out_right))
                {
                    decoder_data->metadata_op_buffer = decoder_data->op_out_right;
                }
            }
#endif /* INSTALL_METADATA */

            decoder_data->codec.out_right_buffer = decoder_data->op_out_right;
            break;
        /* NB No default as can't happen */
    }
    return TRUE;
}

/**
 * \brief Disconnects a capability terminal from a buffer.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the disconnect request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool a2dp_decode_disconnect(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *response_id, void **response_data)
{
    A2DP_DECODER_PARAMS * decoder_data = get_instance_data(op_data);
    unsigned terminal_id = OPMGR_GET_OP_DISCONNECT_TERMINAL_ID(message_data);

    /* Check that the capability is not running, Only the sink can be
     * disconnected whilst running!*/
    if (opmgr_op_is_running(op_data))
    {
        if (terminal_id != INPUT_TERMINAL_ID )
        {
            return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
        }
    }

    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    switch (terminal_id)
    {
        case INPUT_TERMINAL_ID:
            decoder_data->codec.in_buffer = NULL;
            break;
        case LEFT_OUT_TERMINAL_ID:
#ifdef INSTALL_METADATA
            if (decoder_data->metadata_op_buffer == decoder_data->op_out_left)
            {
                decoder_data->metadata_op_buffer = NULL;
            }
#endif /* INSTALL_METADATA */

            decoder_data->op_out_left = NULL;
            decoder_data->codec.out_left_buffer = NULL;
            break;
        case RIGHT_OUT_TERMINAL_ID:
#ifdef INSTALL_METADATA
            if (decoder_data->metadata_op_buffer == decoder_data->op_out_right)
            {
                decoder_data->metadata_op_buffer = NULL;
            }
#endif /* INSTALL_METADATA */


            decoder_data->op_out_right = NULL;
            decoder_data->codec.out_right_buffer = NULL;
            break;
        /* NB No default as can't happen */
    }
    return TRUE;
}

/**
 * \brief Starts the a2pd_decode capability so decoding will be attempted on a
 * kick.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the start request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool a2dp_decode_start(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *response_id, void **response_data)
{
    A2DP_DECODER_PARAMS *decoder_data = get_instance_data(op_data);

    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    if (opmgr_op_is_running(op_data))
    {
        /* Operator already started nothing to do. */
        return TRUE;
    }

    /* At least the sink and the 1st source need to be connected. TODO in
     * stereo mode 2 sources should be connected. */
    if (!(decoder_data->codec.in_buffer != NULL && decoder_data->codec.out_left_buffer != NULL))
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

#ifdef INSTALL_METADATA
    /* Any input metadata should have been configured as 2 octets per sample by
     * the source. Refuse to start if metadata is present and this is not the case. */
    if (buff_has_metadata(decoder_data->codec.in_buffer))
    {
        if(ENCODED_DATA_OCTETS_IN_WORD != cbuffer_get_usable_octets(decoder_data->codec.in_buffer))
        {
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
            return TRUE;
        }
    }

#endif /* INSTALL_METADATA */

    /* Hydra flushes the local and remote buffer at this point and then sets up
     * warping algorithm. This feels like latency control and rate-matching
     * which is to be done in endpoints. */
    return TRUE;
}

/**
 * \brief Reports the buffer requirements of the requested capability terminal
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the buffer size request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
 bool a2dp_decode_buffer_details(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *response_id, void **response_data)
{
    return a2dp_decode_buffer_details_core(op_data, message_data,response_id, response_data,A2DP_DECODE_INPUT_BUFFER_SIZE,A2DP_DECODE_OUTPUT_BUFFER_SIZE, 0);
}


bool a2dp_decode_buffer_details_core(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *response_id, void **response_data,
                                    unsigned inp_bufsize, unsigned out_bufsize, unsigned extra_out_buf_size)
{
#ifdef INSTALL_METADATA
    A2DP_DECODER_PARAMS *op_extra_data = get_instance_data(op_data);
#endif
    OP_BUF_DETAILS_RSP *resp;
    unsigned terminal_id;

    if (!base_op_buffer_details(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    terminal_id = OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data);
    resp = (OP_BUF_DETAILS_RSP*) *response_data;

    if ((terminal_id & TERMINAL_SINK_MASK) == TERMINAL_SINK_MASK)
    {
        /* return the sink buffer size */
        resp->b.buffer_size = inp_bufsize;
    }
    else
    {
        /* return the source buffer size */
        resp->b.buffer_size = MAX(out_bufsize, resp->b.buffer_size + extra_out_buf_size);
    }

#ifdef INSTALL_METADATA
    if ((terminal_id & TERMINAL_SINK_MASK) == TERMINAL_SINK_MASK)
    {
        if(op_extra_data->codec.in_buffer != NULL &&
            buff_has_metadata(op_extra_data->codec.in_buffer))
        {
            resp->metadata_buffer = op_extra_data->codec.in_buffer;
        }
        else
        {
            resp->metadata_buffer = NULL;
        }
    }
    else
    {
        resp->metadata_buffer = op_extra_data->metadata_op_buffer;
    }
    resp->supports_metadata = TRUE;
#endif /* INSTALL_METADATA */

    return TRUE;
}

/**
 * \brief Reports the data format of the requested capability terminal
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the data format request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool a2dp_decode_get_data_format(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *response_id, void **response_data)
{
    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    if (OPMGR_GET_OP_DATA_FORMAT_TERMINAL_ID(message_data) & TERMINAL_SINK_MASK)
    {
        ((OP_STD_RSP*)*response_data)->resp_data.data = AUDIO_DATA_FORMAT_ENCODED_DATA;
    }
    else
    {
        /* The sources are audio samples */
        ((OP_STD_RSP*)*response_data)->resp_data.data = AUDIO_DATA_FORMAT_FIXP;
    }

    return TRUE;
}


/**
 * \brief Returns the block size
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the get_block_size request message payload
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool a2dp_decode_get_sched_info(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *response_id, void **response_data)
{
    OP_SCHED_INFO_RSP* resp;

    resp = base_op_get_sched_info_ex(op_data, message_data, response_id);
    if (resp == NULL)
    {
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }
    *response_data = resp;

    /* Input and output are both unknown so send the default response. */

    return TRUE;
}


/**
 * \brief Save (out_buffer) write ptrs before calling a2dp_decoder_decode
 *
 * \param decoder_data Pointer to the decoder params.
 * \param write_ptrs Pointer to the location to store ptrs
 */
void a2dp_decode_buffer_get_write_ptrs(A2DP_DECODER_PARAMS *decoder_data,
                                                stereo_ptrs *write_ptrs)
{
    PL_ASSERT(write_ptrs != NULL);

    if (decoder_data->codec.out_right_buffer != NULL)
    {
        write_ptrs->right = decoder_data->codec.out_right_buffer->write_ptr;
    }
    else
    {
        write_ptrs->right = NULL;
    }

    if (decoder_data->codec.out_left_buffer != NULL)
    {
        write_ptrs->left = decoder_data->codec.out_left_buffer->write_ptr;
    }
    else
    {
        write_ptrs->left = NULL;
    }
}

/**
 * \brief Check and Perform stereo fade-out operation using mono_cbuffer_fadeout
 *
 * \param decoder_data Pointer to the decoder params.
 * \param samples Number of samples.
 * \param Ptr to saved (out_buffer) write ptrs before calling a2dp_decoder_decode.
 *        Can be NULL if caller didn't need to save the buffers
 *
 * \return TRUE if fadeout operation was performed (and finished)
 */
bool a2dp_decode_check_and_perform_fadeout(A2DP_DECODER_PARAMS *decoder_data,
                                unsigned samples, stereo_ptrs *write_ptrs)
{
    tCbuffer fadeout_buffer;

    /* Is fadeout enabled? if yes, do it on the new output data,
     * otherwise return FALSE */
    if (decoder_data->left_fadeout.fadeout_state == NOT_RUNNING_STATE)
    {
        return FALSE;
    }

    /* If it is enabled it is enabled on both channels. Call it on left and
     * right. They should both fadeout at the same rate so if one says
     * finished then they both should be and we tell the host. */

    /* A small optimisation here, if there is only a mono output connected
     * then we only need to call fade out on the left channel, which is
     * why we tested the left state above. If left is fading out it is
     * definitely connected so we always fade it out. By fading out the
     * right channel first (iff it is connected) we don't need to think
     * about it's response or when to send the fade out done message.
     */

    if (decoder_data->codec.out_right_buffer != NULL)
    {
        if (write_ptrs != NULL && write_ptrs->right != NULL)
        {
            fadeout_buffer = *decoder_data->codec.out_right_buffer;
            fadeout_buffer.read_ptr = write_ptrs->right;
            mono_cbuffer_fadeout(&fadeout_buffer,
                                samples, &(decoder_data->right_fadeout));
        }
        else
        {
            mono_cbuffer_fadeout(decoder_data->codec.out_right_buffer,
                                 samples, &(decoder_data->right_fadeout));
        }
    }

    if (write_ptrs != NULL && write_ptrs->left != NULL)
    {
        fadeout_buffer = *decoder_data->codec.out_left_buffer;
        fadeout_buffer.read_ptr = write_ptrs->left;
        return mono_cbuffer_fadeout(&fadeout_buffer,
                                    samples, &(decoder_data->left_fadeout));

    }
    else
    {
        return mono_cbuffer_fadeout(decoder_data->codec.out_left_buffer,
                                    samples, &(decoder_data->left_fadeout));
    }
}

/* **************************** Operator message handlers ******************************** */

/**
 * \brief Enables a fadeout on the a2dp decoder output channels.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message payload
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool a2dp_dec_opmsg_enable_fadeout(OPERATOR_DATA *op_data,
                    void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    A2DP_DECODER_PARAMS *decoder_data = get_instance_data(op_data);

    /* Fade out only makes sense if we have an output buffer connected */
    if (decoder_data->codec.out_left_buffer == NULL)
    {
        return FALSE;
    }
    common_set_fadeout_state(&decoder_data->left_fadeout, RUNNING_STATE);

    /* If there is only a mono connection then this is done. */
    if (decoder_data->codec.out_right_buffer != NULL)
    {
        common_set_fadeout_state(&decoder_data->right_fadeout, RUNNING_STATE);
    }

    return TRUE;
}

/**
 * \brief Disables a fadeout on the a2dp decoder output channels.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message payload
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool a2dp_dec_opmsg_disable_fadeout(OPERATOR_DATA *op_data,
                    void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    A2DP_DECODER_PARAMS* op_extra_data = get_instance_data(op_data);
    /* No need to check which terminals are connected as setting to
     * NOT_RUNNING_STATE is safe. */
    common_set_fadeout_state(&(op_extra_data->left_fadeout), NOT_RUNNING_STATE);
    common_set_fadeout_state(&(op_extra_data->right_fadeout), NOT_RUNNING_STATE);

    return TRUE;
}

