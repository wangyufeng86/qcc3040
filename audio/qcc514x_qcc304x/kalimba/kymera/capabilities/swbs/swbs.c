/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  swbs.c
 * \ingroup  operators
 *
 *  SWBS_ENC/SWBS_DEC operator common code
 *
 */

/****************************************************************************
Include Files
*/
#include "swbs_private.h"

/***************************** SWBS shared tables ******************************/
/** memory shared between enc and dec */
/// Currently aptX doesn't use shared memory or scratch

/****************************************************************************
Public Function Definitions - shared by encoder and decoder
*/
bool swbs_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    unsigned swbs_in_size = SWBS_DEC_INPUT_BUFFER_SIZE;
    unsigned swbs_out_size = SWBS_DEC_OUTPUT_BUFFER_SIZE;

    if (base_op_get_cap_id(op_data) == CAP_ID_SWBS_ENC)
    {
        swbs_in_size = SWBS_ENC_INPUT_BUFFER_SIZE;
        swbs_out_size = SWBS_ENC_OUTPUT_BUFFER_SIZE;
    }
    else
    {
        /* for decoder, the output size might have been configured by the user */
        SWBS_DEC_OP_DATA* swbs_data = (SWBS_DEC_OP_DATA*) base_op_get_instance_data(op_data);
        SCO_COMMON_RCV_OP_DATA* sco_data = &(swbs_data->sco_rcv_op_data);
        swbs_out_size = MAX(swbs_out_size, sco_data->sco_rcv_parameters.output_buffer_size);
    }

    if(!base_op_buffer_details(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* Currently these have the same value but this isn't guaranteed */
    if (((unsigned *)message_data)[0] & TERMINAL_SINK_MASK)
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = swbs_in_size;
    }
    else
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = swbs_out_size;
    }

#ifdef SCO_RX_OP_GENERATE_METADATA
    if (base_op_get_cap_id(op_data) == SWBS_DEC_CAP_ID)
    {
        L2_DBG_MSG("SWBS_RX_OP_GENERATE_METADATA, metadata is supported");
        /* supports metadata in both side  */
        ((OP_BUF_DETAILS_RSP*)*response_data)->metadata_buffer = 0;
        ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = TRUE;
    }
#endif /* SCO_RX_OP_GENERATE_METADATA */

    L4_DBG_MSG2( "swbs_buffer_details (capID=%d)  %d \n", base_op_get_cap_id(op_data), ((OP_STD_RSP*)*response_data)->resp_data.data);

    return TRUE;
}
