/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/

#ifndef _IIR_RESAMPLER_PRIVATE_H_
#define _IIR_RESAMPLER_PRIVATE_H_

/**
 * Include Files
 */
#include "iir_resamplerv2_common.h"
#include "base_multi_chan_op/base_multi_chan_op.h"

/****************************************************************************
Private Constant Declarations
*/

#define IIR_RESAMPLER_MAX_CHANNELS                      (8)
/* config message bit field values bits 0-15, bits 16-23 are used for internal operator state */
#define IIR_RESAMPLER_CONFIG_DBL_PRECISION              (0x0001)
#define IIR_RESAMPLER_CONFIG_LOW_MIPS                   (0x0002)
/** Flag indicating if the operator is running in stream_based mode or not. */
#define IIR_RESAMPLER_CONFIG_STREAM_BASED               (0x010000)
#define IIR_RESAMPLER_EX_CONFIG_MASK                    (IIR_RESAMPLER_CONFIG_DBL_PRECISION | IIR_RESAMPLER_CONFIG_LOW_MIPS)
#define IIR_RESAMPLER_INT_CONFIG_MASK                   (~IIR_RESAMPLER_EX_CONFIG_MASK)

#define IIR_RESAMPLER_NO_SAMPLE_RATE                    0

/* Default value for temp buffer size */
#define IIR_RESAMPLER_TEMP_BUFFER_SIZE                  256

/* sample (not frame) based processing */
#define IIR_RESAMPLER_DEFAULT_BLOCK_SIZE                1

/**
 * IIR Resampler internal data structure
 */
typedef struct iir_resampler_internal
{
    /** iir_resamplerv2 common parameter data */
    iir_resamplerv2_common    common;

    /**
     * pointer to working data blocks
     * dynamically allocated at end of structure (following channel array)
     */
    unsigned* working;

    /**
     * channel-specific parameter data array
     * variable length depending on number of active channels
     */
    iir_resamplerv2_channel   channel[];

} iir_resampler_internal;

/* channel descriptor specific to PEQ capability */
typedef struct iir_resampler_channels
{
   MULTI_CHANNEL_CHANNEL_STRUC   common;

   unsigned *lpconv_fact;

}iir_resampler_channels;

/**
 * Capability-specific extra operator data
 */
typedef struct IIR_RESAMPLER_OP_DATA
{
    /** input sample rate in Hz */
    unsigned in_rate;

    /** output sample rate in Hz */
    unsigned out_rate;

    /** Conversion fraction used to relate number of input samples to output samples
     * This is encoded as a fraction with components representing integer and
     * fractional parts */
    unsigned conv_fact;

    /* Size of output block for kick processing */
    unsigned source_block_size;

    /** iir_resamplerv2 internal data pointer */
    iir_resampler_internal* iir_resamplerv2;

    /** iir_resamplerv2 shared filter memory pointer */
    void* lpconfig;

    /** amount of scratch memory that has been reserved in addrs */
    unsigned scratch_reserved;

    /** size of intermediate data buffer in words */
    unsigned temp_buffer_size;

    /** iir_resampler configuration flags */
    unsigned config;

#ifdef INSTALL_METADATA
    multi_chan_ttp_last_tag   last_tag;
#endif

} IIR_RESAMPLER_OP_DATA;

/****************************************************************************
Private Function Definitions
*/
/* opcmd handlers */
extern bool iir_resampler_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool iir_resampler_stop_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool iir_resampler_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool iir_resampler_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool iir_resampler_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/* opmsg handlers */
extern bool iir_resampler_opmsg_set_sample_rates(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool iir_resampler_opmsg_set_conversion_rate(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool iir_resampler_opmsg_set_config(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

// iir resampler obpm support
extern bool iir_resampler_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool iir_resampler_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool iir_resampler_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool iir_resampler_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool iir_resampler_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);


/* process data function */
extern void iir_resampler_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);

/***************************************************************************
Internal Function Declarations
*/
extern bool set_rates_iir_resampler_internal(OPERATOR_DATA *op_data, unsigned in_rate, unsigned out_rate);
extern bool init_iir_resampler_internal(OPERATOR_DATA *op_data);
extern void free_iir_resampler_internal(IIR_RESAMPLER_OP_DATA* op_extra_data);

extern void iir_resampler_reset_internal(IIR_RESAMPLER_OP_DATA* op_extra_data,
                                         iir_resampler_channels *chan_ptr);
extern int iir_resampler_amount_to_use(IIR_RESAMPLER_OP_DATA* op_extra_data,
                                       iir_resampler_channels *chan_ptr,
                                       unsigned *available_data);
extern unsigned iir_resampler_processing(IIR_RESAMPLER_OP_DATA* op_extra_data,
                                         unsigned amount_to_use,
                                         iir_resampler_channels *chan_ptr);


#endif  // _IIR_RESAMPLER_PRIVATE_H_
