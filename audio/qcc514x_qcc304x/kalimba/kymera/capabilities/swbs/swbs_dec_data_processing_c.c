/****************************************************************************
 * Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  swbs_dec_data_processing_c.c
 * \ingroup  operators
 *
 *  SWBS decode operator
 *
 */
/****************************************************************************
Include Files
*/
#ifdef ADAPTIVE_R2_BUILD
#include "axModContainerDefs_r2.h"
#else
#include "axModContainerDefs.h"
#endif
#include "swbs_private.h"

#include "patch/patch.h"
#include "pl_assert.h"

/****************************************************************************
Private Constant Definitions
*/
#define INPUT_BLOCK_SIZE                           (30)
#define OUTPUT_BLOCK_SIZE                          (240)
#define OUTPUT_BLOCK_SIZE_24KHZ                    (180)

/****************************************************************************
Private Type Definitions
*/

/****************************************************************************
Private Function Declarations
*/
static unsigned decode_packet(OPERATOR_DATA *op_data, stream_sco_metadata* sco_metadata);
static void swbs_dec_update_expected_pkts(SCO_COMMON_RCV_OP_DATA* sco_data);
/*****************************************************************************
Private Constant Declarations
*/

/****************************************************************************
Private Function Definitions
*/
static inline SWBS_DEC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SWBS_DEC_OP_DATA *) base_op_get_instance_data(op_data);
}

/****************************************************************************
Public Function Definitions
*/

/**
 * Processing function for the swbs_dec operators.
 * \param op_data
 * \param touched
 */
unsigned swbs_dec_processing(OPERATOR_DATA *op_data)
{
    SWBS_DEC_OP_DATA* swbs_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* sco_data = &(swbs_data->sco_rcv_op_data);

    stream_sco_metadata sco_metadata;
    unsigned ret_val = TOUCHED_NOTHING;
    stream_sco_metadata_status status = SCO_METADATA_NOT_READ;
    CONNECTION_TYPE conn_type = swbs_get_sco_connection_type(op_data);
#ifdef SCO_DEBUG
    /* increment the kick count for swbs. This might be a duplicated in sco_rcv_parameters.md_num_kicks */
    swbs_data->swbs_dec_num_kicks++;
#endif
    /* Init run specific variables for SWBS */
    swbs_data->good_pkts_per_kick = 0;
    swbs_data->swbs_dec_output_samples = 0;

    /* increment the kick count */
    sco_data->sco_rcv_parameters.md_num_kicks++;

    /*
     * calculate the expected packets - if packet size was not yet initialised, this will
     * always return 1
     */
    swbs_dec_update_expected_pkts(sco_data);

    /* Log the SCO state */
    print_SCO_state(sco_data);


    /*
     * If expected time stamp is not initialised, decoder is yet to produce data
     * since it was started. Similarly, packet size may not yet be set. So try to make
     * sense of whatever data we see in input - for each kick, we should have some data
     * to read and hopefully find at least a metadata header to update the needed packet
     * size and timestamp information. If somehow not even a header worth of data arrived,
     * then we have no choice other than flag this as a bad kick and return later with
     * renewed hope...
     */
    if (!enough_data_to_run(sco_data, METADATA_HEADER_SIZE))
    {
        /* This should never happen. In case it does, just hope that for the next kick
         * we will have enough data on the output.
         */
        L2_DBG_MSG3("t=%08x: Kick: %d, bad kicks: %d:: Warning! Not enough data to run for the first time",
                    time_get_time(),
                    sco_data->sco_rcv_parameters.md_num_kicks,
                    sco_data->sco_rcv_parameters.num_bad_kicks);

        /* There is no valid packet in the input buffer. */
        sco_data->sco_rcv_parameters.num_bad_kicks ++;
        sco_data->sco_rcv_parameters.num_kicks_no_data ++;

        /* Before exiting increment the bad kick counter. */
        sco_fw_check_bad_kick_threshold(sco_data);
        /* Fake a packet */
        if (enough_space_to_run(sco_data, OUTPUT_BLOCK_SIZE))
        {
            sco_data->sco_rcv_parameters.exp_pkts--;
            if (sco_data->sco_rcv_parameters.exp_pkts==0)
            {
                /* No packet on the input buffer and we want to
                 * generate a fake packet, so we need to make sure
                 * the late packet will not be used later.
                 */
                sco_fw_update_expected_timestamp(sco_data);

                ret_val |= fake_packet_swbs(op_data, conn_type);
            }
        }


        return ret_val;
    }


    if (!enough_space_to_run(sco_data, OUTPUT_BLOCK_SIZE))
    {
        /* This should never happen. In case it does, just hope that for the next kick
         * we will have enough data on the output.
         */
        SCO_DBG_MSG("Warning! Not enough space to run for the first time");

        /* There is no valid packet in the input buffer. */
        sco_data->sco_rcv_parameters.num_bad_kicks ++;

        /* Before exiting increment the bad kick counter. */
        sco_fw_check_bad_kick_threshold(sco_data);

        return ret_val;
    }

    /* Read the metadata using SWBS specific transformations for the header. */
    status = read_packet_metadata(sco_data, &sco_metadata, conn_type);

    while(sco_data->sco_rcv_parameters.exp_pkts > 0)
    {
        patch_fn_shared(swbs_decode_processing);

#ifdef SCO_DEBUG
        /* Increment number of packet searched for SWBS. */
        swbs_data->swbs_dec_num_pkts_searched ++;
#endif
        /* Check if there is enough data in the input buffer to read the packet payload.*/
        if ((status == SCO_METADATA_INVALID) ||
            !enough_data_to_run(sco_data, CONVERT_OCTETS_TO_SAMPLES(sco_metadata.packet_length)) )
        {
            /*
             * sync lost flush the input buffer
             */
            sco_rcv_flush_input_buffer(sco_data);

            /* There is no valid packet in the input buffer. */
            sco_data->sco_rcv_parameters.num_bad_kicks ++;
            L2_DBG_MSG3("t=%08x: Kick: %d, bad kicks: %d:: Warning! Invalid meta data, faking packet",
                        time_get_time(),
                        sco_data->sco_rcv_parameters.md_num_kicks,
                        sco_data->sco_rcv_parameters.num_bad_kicks);

            /* Fake a packet */
            sco_data->sco_rcv_parameters.exp_pkts--;
            if (sco_data->sco_rcv_parameters.exp_pkts==0)
            {
                ret_val |= fake_packet_swbs(op_data, conn_type);
            }

            /* No  valid packet on the input buffer which mean there is no reason to
             * try and check it again. Increment the expected timestamp.*/
            sco_fw_update_expected_timestamp(sco_data);

            /* Before exiting update the bad kick counter. */
            sco_fw_check_bad_kick_threshold(sco_data);

            return ret_val;
        }

        /* Check if the packet size changed. */
        if (sco_metadata.packet_length != sco_data->sco_rcv_parameters.sco_pkt_size)
        {
            /* update expected packet */
            sco_data->sco_rcv_parameters.sco_pkt_size = sco_metadata.packet_length;
            sco_data->sco_rcv_parameters.md_pkt_size_changed++;
            /* sco_data->sco_rcv_parameters.exp_pkts only gets update if is  set to 0. */
            sco_data->sco_rcv_parameters.exp_pkts = 0;
            swbs_dec_update_expected_pkts(sco_data);

        }
#ifdef SCO_DEBUG
        swbs_data->swbs_dec_metadata_found ++;
#endif
        print_sco_metadata(&sco_metadata);

        status = analyse_sco_metadata(sco_data, &sco_metadata);

        patch_fn_shared(swbs_decode_processing);

        switch(status)
        {
            case SCO_METADATA_LATE:
            {
                /* Progress the input buffer read address past this invalid packet */
                sco_data->sco_rcv_parameters.md_late_pkts++;

                /* discard packet*/
                discard_packet(sco_data, &sco_metadata);

                if (!enough_data_to_run(sco_data, METADATA_HEADER_SIZE) ||
                    !enough_space_to_run(sco_data, OUTPUT_BLOCK_SIZE))
                {
                    /* This should never happen. In case it does, just hope that for the next kick
                     * we will have enough data on the output.
                     */
                    L2_DBG_MSG3("t=%08x: Kick: %u, Late: %u:: Warning! Packet late and not enough input data or not enough output data to run again",
                        time_get_time(), sco_data->sco_rcv_parameters.md_num_kicks, sco_data->sco_rcv_parameters.md_late_pkts);

                    /* Before exiting increment the bad kick counter. */
                    sco_fw_check_bad_kick_threshold(sco_data);
                    return ret_val;
                }

                status = read_packet_metadata(sco_data, &sco_metadata, conn_type);
                break;
            }
            case SCO_METADATA_ON_TIME:
            {
                /* Decrement the expected packets. */
                sco_data->sco_rcv_parameters.exp_pkts--;

                /* Reset the counter if we get a good packet. This means that everything
                 * is in sync and we shouldn't take any actions if we only get
                 * late/early packets once in a while.
                 */
                sco_data->sco_rcv_parameters.out_of_time_pkt_cnt = 0;

#ifdef SCO_DEBUG_STATUS_COUNTERS
                /* Update status counters for records we pass to the decoder */
                sco_rcv_update_status_counters(sco_data, &sco_metadata);
#endif
                /* Decode the valid packet to the output buffer. */
                if ((sco_metadata.status == OK ) || (sco_metadata.status == CRC_ERROR_WITH_BIT_ERRORS))
                {
                    ret_val = decode_packet(op_data, &sco_metadata);
                }
                else
                {
                    sco_data->sco_rcv_parameters.frame_error_count++;

                    swbs_data->axInBuf.nRPtr = 0;
                    swbs_data->axInBuf.nWPtr = 0;

                    /* Call the PLC to generate new packet*/
                    ret_val =fake_packet_swbs(op_data, SWBS);

                    /* Increment the bad kick threshold and check if we need a SCO reset. */
                    sco_fw_check_bad_kick_threshold(sco_data);
                }

                /* Update the expected timestamp */
                sco_fw_update_expected_timestamp(sco_data);


                if(sco_data->sco_rcv_parameters.exp_pkts > 0)
                {
                    if (!enough_data_to_run(sco_data, METADATA_HEADER_SIZE) ||
                        !enough_space_to_run(sco_data, OUTPUT_BLOCK_SIZE))
                    {
                        /* There is not enough data to run again. Exit. */
                        SCO_DBG_MSG("Warning! Not enough data to run again");
                        return ret_val;
                    }

                    status = read_packet_metadata(sco_data, &sco_metadata, conn_type);
                }
                else
                {
                    return ret_val;
                }
                break;
            }
            case SCO_METADATA_EARLY:
            {
                /* Increment early packet count */
                sco_data->sco_rcv_parameters.md_early_pkts++;

                /* Fake a packet. */
                ret_val |= fake_packet_swbs(op_data, conn_type);

                /* Increment the timestamp with one. */
                sco_fw_update_expected_timestamp(sco_data);

                /* Increment the bad kick threshold and check if wee need a SCO reset. */
                sco_fw_check_bad_kick_threshold(sco_data);

                break;
            }
            default:
            {
                /* This shouldn't happen. This means that analyse_packet doesn't cover
                 * all the possible cases. */
                fault_diatribe(FAULT_AUDIO_INVALID_SCO_METADATA_STATE, status);
                break;
            }

        }/*switch */

    }/* while exp_packets */

    return ret_val;

}


/****************************************************************************
Private Function Definitions
*/
/**
 * \brief Decode a valid packet to the output buffer. Runs plc on the output.
 *
 * \param op_data - Pointer to the operator data
 * \param current_packet - current packet containing the metadata information.
 */
static unsigned decode_packet(OPERATOR_DATA *op_data, stream_sco_metadata* sco_metadata)
{
    SWBS_DEC_OP_DATA* swbs_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* sco_data = &(swbs_data->sco_rcv_op_data);
    CONNECTION_TYPE conn_type = swbs_get_sco_connection_type(op_data);
    unsigned swbs_packet_length = 0;
//    unsigned amount_advanced = 0;

    /* validate the current packet. */
    int validate = sco_decoder_swbs_validate(op_data, sco_metadata->packet_length, &swbs_packet_length);

   // AR- Validate returns in r1 240 bytes (1 frames worth of data) if everything
   // AR- is alright. If it returns 0, 1 or 2, then there is no data
   // AR- 0 means there is not enough space in output buffer
   // AR- 1 means there is not enough data in the input buffer
   // AR- 2 means there is some data in input buffer, but unable to identify the SWBS frame sync.

    /* before calling this function we only know that we have one full packet
     * It will be a serious bug if the validate function tries to go beyond one full packet
     */
 //   PL_ASSERT(amount_advanced <= CONVERT_OCTETS_TO_SAMPLES(sco_metadata->packet_length));

//    SCO_DBG_MSG1("validate retval = %4d", validate);

     if(validate == 240 || validate == 180)
    {
#ifdef SCO_DEBUG
        swbs_data->swbs_dec_output_samples += validate;
        swbs_data->swbs_dec_validate_ret_good ++;
#endif
    }
    else
    {
        if (!((validate == 0) || (validate == 1) || (validate == 2)))
        {
            fault_diatribe(FAULT_AUDIO_INVALID_WBS_VALIDATE_RESULT, validate);  // reuse WBS fault rather than make a SWBS fault
        }
    }

    patch_fn_shared(swbs_decode_processing);

#ifdef ESCO_SUPPORT_ERRORMASK
    int output_packet_size = sco_decoder_swbs_process_bit_error(op_data, sco_metadata, swbs_packet_length);
#else
    int output_packet_size = sco_decoder_swbs_process(op_data, sco_metadata, swbs_packet_length);
#endif
    SCO_DBG_MSG1("sco_decoder_swbs_process result = %4d", output_packet_size);
    /* sco_decoder_swbs_process does not increment the read pointer of the input buffer.
     * The sco packet length was checked in read_packet_metadata. */
    {
        /* advance the read pointer by one packet length, the input buffer has already
           been checked that it has one full packet, however part of that might have
           been consumed by the validate function
         */
        unsigned amount_to_advance = CONVERT_OCTETS_TO_SAMPLES(sco_metadata->packet_length);
#ifdef SCO_RX_OP_GENERATE_METADATA
        if(buff_has_metadata(sco_data->buffers.ip_buffer))
        {
            /* remove associated metadata before updating main buffer */
            sco_rcv_transport_metadata(sco_data, amount_to_advance, 0, conn_type);
        }
#endif
        /* part of packet might already be advanced by validate function */
//        amount_to_advance -= amount_advanced;
        cbuffer_advance_read_ptr(sco_data->buffers.ip_buffer, amount_to_advance);
    }

    if (output_packet_size == 0)
    {
        return TOUCHED_NOTHING;
    }
    else
    {

#ifdef SCO_RX_OP_GENERATE_METADATA
        /* update metadata first */
        sco_rcv_transport_metadata(sco_data, 0, output_packet_size, conn_type);
#endif
        cbuffer_advance_write_ptr(sco_data->buffers.op_buffer, output_packet_size);

        /* We produced an output so we don't expect more packets. */
        sco_data->sco_rcv_parameters.exp_pkts = 0;
        return TOUCHED_SOURCE_0;
    }
}

/**
 *  Calculates the expected packets needed to decode one SWBS frame.
 *
 * \param sco_data - Pointer to the SCO rcv operator data
 * \return Expected number of packets needed in worst case to decode one SWBS frame.
 */
static void swbs_dec_update_expected_pkts(SCO_COMMON_RCV_OP_DATA* sco_data)
{
    patch_fn_shared(swbs_decode_processing);

    /* Only update exp_pkts if it is 0. */
    if (sco_data->sco_rcv_parameters.exp_pkts == 0)
    {
        unsigned sco_packet_size_words = sco_rcv_get_packet_size(sco_data);

        sco_data->sco_rcv_parameters.exp_pkts =
                (INPUT_BLOCK_SIZE + (sco_packet_size_words - 1)) / sco_packet_size_words;
    }
}


/**
 *  Reset the internals of the decoder
 * \param op_data - operator data pointer
 */
void swbs_dec_reset_aptx_data(OPERATOR_DATA* op_data)
{
  //  SWBS_DEC_OP_DATA* swbs_data = get_instance_data(op_data);


}


/**
 * \brief Fakes a packet with aptX Adaptive PLC.
 *
 * TODO Consider to inline this function.
 * \param sco_data - Pointer to the common SCO rcv operator data
 */
unsigned fake_packet_swbs(OPERATOR_DATA *op_data, CONNECTION_TYPE type)
{
    patch_fn(swbs_fake_packet);
#ifdef USE_PLC_MSBC
    SWBS_DEC_OP_DATA* swbs_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* sco_data = &(swbs_data->sco_rcv_op_data);
    unsigned packet_size = 0;

    if (swbs_data->codecMode == SWBS_CODEC_MODE4)
        packet_size = 180;                 /* Mode 4 packet size is 180 samples */
    else
        packet_size = 240;                 /* Mode 0 packet size is 240 samples */

    if(!enough_space_to_run(sco_data, packet_size))
    {
        return TOUCHED_NOTHING;
    }

#ifdef SCO_RX_OP_GENERATE_METADATA
    /* keep metadata aligned with the buffer */
    sco_rcv_transport_metadata(sco_data,
                               0,           /* input_processed */
                               packet_size, /* output_generated */
                               type         /* type */);
#else
    NOT_USED(type);
#endif
    /*
    * Do packet loss concealment (PLC).
    */
    if (sco_data->sco_rcv_parameters.force_plc_off == 0)
    {
       swbs_run_plc(op_data, packet_size);
       return TOUCHED_SOURCE_0;
    }
    else
       return TOUCHED_NOTHING;

#else
    return TOUCHED_NOTHING;
#endif
}
