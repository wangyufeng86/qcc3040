         /****************************************************************************
* Copyright (c) 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/

#ifndef _RATE_ADJUST_PRIVATE_H_
#define _RATE_ADJUST_PRIVATE_H_

/**
 * Include Files
 */
#include "base_multi_chan_op/base_multi_chan_op.h"
#include "audio_proc/sra_c.h"
#include "cbops/cbops_c.h"

/****************************************************************************
Private Constant Declarations
*/

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define RATE_ADJUST_CAP_ID CAP_ID_DOWNLOAD_RATE_ADJUST
#else
#define RATE_ADJUST_CAP_ID CAP_ID_RATE_ADJUST
#endif

#define RATE_ADJUST_MAX_CHANNELS                      (8)

/** Flag indicating if the operator is running in stream_based mode or not. */
#define RATE_ADJUST_CONFIG_STREAM_BASED               (0x010000)

/* sample (not frame) based processing */
#define RATE_ADJUST_DEFAULT_BLOCK_SIZE                1

/* minimum number of samples to repeat processing */
#define RATE_ADJUST_MIN_SAMPLES                       3

#define RATE_ADJUST_MIN_SAMPLE_RATE 8000
#define RATE_ADJUST_MAX_SAMPLE_RATE 192000

/* channel descriptor specific to rate adjust capability */
typedef struct rate_adjust_channels
{
   MULTI_CHANNEL_CHANNEL_STRUC   common;

}rate_adjust_channels;

/**
 * Capability-specific extra operator data
 */
typedef struct RATE_ADJUST_OP_DATA
{
    /* number of channels */
    unsigned num_channels;

    /* nominal sample rate */
    unsigned sample_rate;

    /* target rate value */
    unsigned target_rate;

    /* cbops graph (includes only rate adjust op */
    cbops_graph *sra_graph;

    /* rate adjust operator */
    cbops_op *rate_adjust_op;

    /* TRUE if the operator is in passthrough mode */
    bool rate_adjust_passthrough;

#ifdef INSTALL_METADATA
    multi_chan_ttp_last_tag last_tag;
    /* sample period in us, for optimisation only */
    unsigned sample_period;
#endif

} RATE_ADJUST_OP_DATA;

/****************************************************************************
Private Function Definitions
*/
/* opcmd handlers */
extern bool rate_adjust_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool rate_adjust_stop_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool rate_adjust_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool rate_adjust_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool rate_adjust_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/* opmsg handlers */
extern bool rate_adjust_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool rate_adjust_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool rate_adjust_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool rate_adjust_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool rate_adjust_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool rate_adjust_channel_create(OPERATOR_DATA *op_data,MULTI_CHANNEL_CHANNEL_STRUC *chan_ptr,unsigned chan_idx);
extern void rate_adjust_channel_destroy(OPERATOR_DATA *op_data,MULTI_CHANNEL_CHANNEL_STRUC *chan_ptr);
extern bool rate_adjust_opmsg_set_sample_rates(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool rate_adjust_opmsg_set_current_rate(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool rate_adjust_opmsg_set_target_rate(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool rate_adjust_opmsg_passthrough_mode(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/* process data function */
extern void rate_adjust_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);

/***************************************************************************
Internal Function Declarations
*/
extern bool init_rate_adjust_internal(OPERATOR_DATA *op_data);
extern void free_rate_adjust_internal(RATE_ADJUST_OP_DATA* op_extra_data);

#endif  // _RATE_ADJUST_PRIVATE_H_
