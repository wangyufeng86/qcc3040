/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup swbs
 * \file  swbs_struct.h
 * \ingroup capabilities
 *
 * Super Wide Band Speech type header. <br>
 *
 */

#ifndef SWBS_TYPES_H
#define SWBS_TYPES_H
/*****************************************************************************
Include Files
*/
#include "sco_struct.h"
#include "axBuf_t.h"

#define APTX_ADAPTIVE_IN_BUF_SIZE           0xF78  // = 3300 = 825 words
#define APTX_ADAPTIVE_OUT_BUF_SIZE          0x2D00 // = 11520 SPACE FOR 2 OUTPUT PACKETS
                                             // 4608 (1152 samples) + 64

#define SWBS_BIT_ERROR_BUF_SIZE 0x7C // 122 (rounded up to 124) bytes
											 
/** capability-specific extra operator data for SWBS ENC */
typedef struct SWBS_ENC_OP_DATA
{
    /** Fade-out parameters */
    FADEOUT_PARAMS fadeout_parameters;

    /** Terminal buffers */
    SCO_TERMINAL_BUFFERS buffers;

    /* This is where additional SWBS-specific information can start */

    /* The codec deta structure - in the old days, it is what value at $caps.sco.CODEC_DATA_STRUC_PTR_FIELD used to point to */
    void* codec_data[64];            // aptX codec data struct
	
    unsigned codecMode;

    /** aptX adaptive uses axBuf types for data input/output */
    axBuf_t  axInBuf;
    axBuf_t  axOutBuf;

    /** Amount of data to process each time */
    unsigned frame_size;

    /** Kick forward each time SWBS_ENC is kicked (to kick SCO_SINK_EP on TESCO rate) */
    bool forward_all_kicks_enabled;
} SWBS_ENC_OP_DATA;


/** capability-specific extra operator data for SWBS DEC */
typedef struct SWBS_DEC_OP_DATA
{
    /** Common part of SCO Rcv operators' extra data - it packages the buffer size,
     *  fadeout parameters and SCO parameters so that common code can safely reference same fields.
     */
    SCO_COMMON_RCV_OP_DATA sco_rcv_op_data;

    /** The codec data structure - in the old days, it is what value at $caps.sco.CODEC_DATA_STRUC_PTR_FIELD used to point to */
    void* codec_data[64];             // aptX codec data struct

    /** aptX adaptive uses axBuf types for data input/output */
    axBuf_t  axInBuf;
    axBuf_t  axOutBuf;

    unsigned codecMode;

    unsigned storedWP;
    bool     overlap_finished;
	unsigned num_out_channels;
    unsigned packet_size;
	
    /** First valid packet flag */
    unsigned received_first_valid_pkt;

    /** Address of next packet value */
    unsigned* next_packet_value;

    /**  num of times no data at kick and we needed to fake, ie, not priming */
    unsigned md_bad_kick_attmpt_fake;
    /**  num of times no data at kick and we had to fake, ie, validate returned NZ */
    unsigned md_bad_kick_faked;
    /** number of good packets found during one kick period */
    unsigned good_pkts_per_kick;

#ifdef SCO_DEBUG
    /** decoder statistics */
    unsigned swbs_dec_dbg_stats_enable; /* flag for debug stats enabling */

    unsigned swbs_dec_stats_count;    /* Seq number of stats message */
    unsigned swbs_dec_stats_last_ts;  /* last time stamp */
    unsigned swbs_dec_num_kicks;   /* Num of kicks in period */
    unsigned swbs_dec_num_pkts_searched; /* num of times metadata read was called in period */
    unsigned swbs_dec_metadata_found;    /* num of times metadata read returned success in period */
    unsigned swbs_dec_error_pkts;  /* num of packets with error status in metadata hdr in period */
    unsigned swbs_dec_validate_ret_nz;   /* num of times validate returned non zero values: zero means no space in op buffer */
    unsigned swbs_dec_validate_ret_good; /* num of times validate returned values >= 120. */
#endif /* SCO_DEBUG */

    unsigned swbs_dec_no_output;   /*  num of times decoder process returned no_output status code */
    unsigned swbs_dec_fake_pkt;   /*  num of times decoder process returned no_output code and PLC output a fake packet */
    unsigned swbs_dec_good_output; /*  num of times decoder process returned good output status code */
    unsigned swbs_dec_bad_output;  /*  num of times decoder process returned bad output status code */
    unsigned swbs_dec_invalid_validate_result;  /*  SWBS validate returned an invalid result */

    /** flags for touched terminals, bit0 is input, bit12 is output terminal touched flag */
    unsigned terminals_touched;
    /** number of samples resulting from SWBS validate - this will match, if PLC is installed, the PLC packet length eventually */
    unsigned swbs_dec_output_samples;
#ifdef ESCO_SUPPORT_ERRORMASK
    void * pBitErrorBuff;
#endif

} SWBS_DEC_OP_DATA;

#endif /* SWBS_TYPES_H */
