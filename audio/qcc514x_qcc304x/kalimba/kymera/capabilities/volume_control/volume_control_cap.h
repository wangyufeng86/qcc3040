/**
* Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
* \file  volume_control_cap_c.h
* \ingroup  capabilities
*
*  Volume Control
*
*/

#ifndef VOL_CTRL_CAP_C_H
#define VOL_CTRL_CAP_C_H

#include "vol_ctrl_gen_c.h"
#include "buffer/cbuffer_c.h"
#include "volume/shared_volume_control.h"
#include "op_msg_utilities.h"
#include "opmgr/opmgr_for_ops.h"
#include "pl_timers/pl_timers.h"
#ifdef VOLUME_CONTROL_AUX_TTP_SUPPORT
#include "ttp/timed_playback.h"
#include "ttp_utilities.h"
#endif /* VOLUME_CONTROL_AUX_TTP_SUPPORT */

/* Capability Version */
#define VOL_CTRL_CAP_VERSION_MINOR            0

#ifdef VOLUME_CONTROL_AUX_TTP_SUPPORT
/* CONSTANTS for AUX_TTP_SUPPORT */
/* Aux timed playback will use main channel timestamp for
 * timed mixing, if we stop receiving timestamp from main
 * channel we continue to rebuild timestamp based on last
 * received timestamp from main channel, however this is for
 * a limited period and we will discontinue that after a certain
 * period.
 */
#define VOL_CTRL_AUX_USE_MAIN_TIMESTAMP_PERIOD_MS 200

/* Configuring for aux timed playback mode is only for one
 * aux playback, it will also expire if we don't observe the
 * actual aux data within a certain period.
 */
#define VOL_CTRL_AUX_TTP_EXPIRY_PERIOD_MS 100

/* When receiving config message we expect the requested
 * time to play to be at least this amount in future.
 */
#define VOL_CTRL_AUX_MIN_TTP_IN_FUTURE_MS 30

/* When receiving config message we expect the requested
 * TTP to be no later than this amount in future.
 */
#define VOL_CTRL_AUX_MAX_TTP_IN_FUTURE_MS 20000

/* apply a max to downstream latency setting,
 * this is applied when param is set using operator
 * message and is the same as max value in xml
 */
#define VOL_CTRL_MAX_DOWNSTREAM_LATENCY_MS 200

#endif /* VOLUME_CONTROL_AUX_TTP_SUPPORT */

/* Aux Channel States */
typedef enum
{
   /* Aux isn't connected, or we are not seeing any aux
    * input or it isn't enough for one run.
    */
   AUX_STATE_NO_AUX = 0,

   /* We are seeing AUX input, but we are in the process of
    * applying necessary fading to main channel, so we still
    * haven't started mix aux into main.
    */
   AUX_STATE_START_AUX,

   /* Aux is being mixed, we stay here for as long as aux
    * has enough data.
    */
   AUX_STATE_IN_AUX,

   /* We just stopped seeing Aux input, main channel is faded back
    * to its original level, once done we go back to NO_AUX */
   AUX_STATE_END_AUX
}vol_ctlr_aux_state;

/** Operator specific TTR event IDs.
 * Chose a range which does not overlap with Source Sync's or
 * AEC Reference's ranges, to help search for events without
 * needing to filter by op id / cap id.
 */
enum {
    VOL_CTRL_TTR_EVENT_AUX_STATE = TIMING_TRACE_OP_EVENT_OP_BASE + 4,
    VOL_CTRL_TTR_EVENT_AUX_STARTED_MIXING
};

/* Parameter Access structures */

typedef struct vol_ctrl_aux_params
{
   unsigned aux_scale;                        /**< Scaling applied to AUX channel */
   unsigned atk_tc;                           /**< hold time ramping in to AUX audio  (0.01/msec) fractional */
   unsigned dec_tc;                           /**< hold time ramping out of AUX audio (0.01/msec) fractional */
}vol_ctrl_aux_params_t;

/* priority AUX mixing only supports 4 AUX channels
   aux_routing is 5 bits per priority level.
   The entries are in decreasing priority

   bit4:     Priority Level is valid
   bit3:     Mute AUX for channel
   bit2..0:  AUX Channel Index
*/
typedef struct vol_ctrl_chan_params
{
    unsigned aux_routing;                                       /**< priority list for mixing AUX channel(s) */
    unsigned prim_scale[VOL_CTRL_CONSTANT_AUX_NUM_PRIORITIES];  /**< fractional attenuation when mixing aux */
}vol_ctrl_chan_params_t;


/* Volume Control Gains */
typedef struct vol_ctrl_gains
{
   unsigned master_gain;                                        /**< Overall gain for audio */
   unsigned mute;
   unsigned auxiliary_gain[VOL_CTRL_CONSTANT_NUM_CHANNELS];     /**< Volume for Aux stream */
   unsigned channel_trims[VOL_CTRL_CONSTANT_NUM_CHANNELS];      /**< Per channel Trim*/
}vol_ctrl_gains_t;


/* Volume Control Channels */

typedef struct vol_ctrl_aux_channel
{
   tCbuffer *buffer;
   vol_ctlr_aux_state state;
   unsigned           transition;
   unsigned           advance_buffer;

}vol_ctrl_aux_channel_t;

#ifdef VOLUME_CONTROL_AUX_TTP_SUPPORT
typedef struct vol_ctrl_aux_channel_ttp
{

   /* TRUE means timed playback for aux0 is enabled by user,
    * This flags is automatically cleared when the aux playback
    * finishes or when we don't receive aux input at all by
    * expiry_time.
    */
   bool enabled;

   /* user configured: time to play for aux0, this will be
    * the time that first sample of aux is expected to be
    * played at the speaker.
    */
   TIME time_to_play;

   /* When aux is configured for timed playback, aux data must
    * be available before the gate time, if it isn't seen at
    * all after some time the request will expire. Stored to avoid
    * recalculation at run time.
    */
   TIME expiry_time;

   /* time to open the gate for auxiliary input to for timed playback,
    * aux gate opens earlier than ttp mixing time, so it gives enough
    * time to the operator to reduce the main gain before starting
    * to mix the aux. Stored to avoid recalculation at run time.
    */
   TIME gate_time;

   /* If we actively receive timestamp from input we will use it for
    * timed playback, if not the operator will generate TTP itself.
    * This flag is enabled when time stamps are generated by the
    * operator itself.
    */
   bool generate_ttp;

   /* user configured, this is the fine rate that should apply
    * to auxiliary input so it won't drift when aux is very long.
    * TODO: This is currently not used.
    */
   int  drift_rate;

}vol_ctrl_aux_channel_ttp_t;
#endif /* VOLUME_CONTROL_AUX_TTP_SUPPORT */

typedef struct vol_ctrl_channel
{
   int       chan_idx;       /**< index for stream source buffer */

   unsigned aux_mix_gain;   /**< mix ratio for aux audio  Q.XX */
   unsigned prim_mix_gain;  /**< mix ratio for channel audio Q.XX */

   unsigned limit_gain_log2;     /**< Attenuation to prevent saturation */
   unsigned limiter_gain_linear; /**< Attenuation applied for saturation prevention Q.XX*/
   unsigned last_peak;           /**< Last peak measurement Q.XX */

   unsigned channel_gain;   /**< Q5.xx channel gain */
   unsigned last_volume;    /**< last digital gain applied to channel*/

   tCbuffer *aux_buffer;
}vol_ctrl_channel_t;

/* Time constants for adjustments */
typedef struct vol_time_constants
{
   unsigned num_words;
   unsigned vol_tc;
   unsigned sat_tc;
   unsigned sat_tcp5;
   unsigned period;
}vol_time_constants_t;

/**
 * Extended data structure for Volume Control capability
 */
/* Note that structure name has changed to "_vol_ctrl_data", the
 * structure has been extended compared to the one defined for ROM,
 * renaming is needed so downloadable can be linked against ROM,
 * otherwise structure's SIZE symbol will clash with that defined
 * in ROM elf. Use of the structure fields in ASM files must also be
 * updated accordingly.
 */
typedef struct _vol_ctrl_data
{
   tCbuffer *input_buffer[VOL_CTRL_CONSTANT_NUM_CHANNELS];          /**< Pointer to Sink Terminals  */
   tCbuffer *output_buffer[VOL_CTRL_CONSTANT_NUM_CHANNELS];         /**< Pointer to Source Terminals  */
   tCbuffer *wait_on_space_buffer;
   tCbuffer *wait_on_data_buffer;

#ifdef INSTALL_METADATA
    tCbuffer *metadata_ip_buffer;   /** The input buffer with metadata to transport from */
    tCbuffer *metadata_op_buffer;   /** The output buffer with metadata to transport to */
    vol_ctrl_aux_channel_t *metadata_aux_channel;   /** The aux channel with metadata to transport from */
    metadata_tag* last_eoftag;      /** An EOF tag from last run which had no metadata  */
    unsigned last_tag_data_remaining;      /** The amount of not-yet-consumed data from previous tag  */
#endif /* INSTALL_METADATA */
   unsigned num_channels;                                           /**< number of active channels */
   unsigned touched_sink;                                           /**< touched for active channels */
   unsigned touched_src;                                            /**< touched for active channels */
   vol_ctrl_channel_t *channels;

   vol_ctrl_aux_channel_t  aux_channel[VOL_CTRL_CONSTANT_NUM_CHANNELS];    /**< Auxiliary Channels  */

   unsigned sample_rate;

   unsigned Ovr_Control;                                    /**< obpm control overrides  */
   unsigned post_gain;                                      /**< gain applied outside of DSP control  */
   vol_ctrl_gains_t host_vol;
   vol_ctrl_gains_t obpm_vol;
   vol_ctrl_gains_t *lpvols;

   SHARED_VOLUME_CONTROL_OBJ *shared_volume_ptr;         /**< NDVC noise level */

   VOL_CTRL_PARAMETERS       parameters;
   /** additionally used fields */
   unsigned ReInitFlag;
   CPS_PARAM_DEF parms_def;

   vol_time_constants_t tc;

   unsigned aux_connected;
   unsigned aux_active;
   unsigned aux_state;				/**< AUX current usage */
   unsigned aux_in_use;
   unsigned aux_buff_size;
   bool     aux_pending;
   bool     used_all_input;
   tTimerId pending_timer;

   unsigned vol_initialised;

   unsigned mute_period;                               /**< Mute period in msec */
   unsigned cur_mute_gain;
   int      mute_increment;

   bool stream_based;                                   /**< Main channels are sync'd */

#ifdef VOLUME_CONTROL_AUX_TTP_SUPPORT

    /* aux ttp only supported for aux channel 0 */
    vol_ctrl_aux_channel_ttp_t aux0_ttp;

   /* Whether we are actively receiving timestamp from
    * main input channel
    */
   bool main_timestamp_valid;

   /* last valid timestamp we received from main channel */
   TIME last_main_real_timestamp;

   /* This is the timestamp that is used to decide
    * the start of mixing of an auxiliary input with
    * ttp enabled by the user. It holds a timestamp
    * corresponding to first sample of next input chunk
    * to process. aux0_ttp.time_to_play is compared to
    * this timestamp to start of mixing process.
    * Note: Valid only if current_timestamp_valid is TRUE
    */
   TIME current_timestamp;

   /* Whether current_timestamp is valid */
   bool current_timestamp_valid;

   /* A remainder to avoid error accumulation
    * when operator updates timestamp
    */
   unsigned current_timestamp_update_remainder;

   /* number of samples consumed in last run,
    * used for calculating timestamp from
	* previously received/calculated timestamp.
    */
   unsigned prev_consumed_samples;

   /* Worked out by the operator, this is just an
    * estimation of (maximum) downstream latency.
    * Requested ttp_playback_time must be enough
    * into the future so we can meet the deadline
    * with this amount of latency in downstream.
    * Only used when the operator itself generates
    * timestamp for aux mixing (aux0_ttp.generate_ttp=True).
    */
   TIME_INTERVAL downstream_latency_estimate;

   /* debug only, shows that mixing has started. */
   bool dbg_aux_mixing_started;

   /* downstream latency param set by the user */
   unsigned parameter_downstream_latency_ms;
#endif
   unsigned reserved_1;     /**< For extension */
   unsigned reserved_2;

} VOL_CTRL_DATA_T;

#ifdef VOLUME_CONTROL_AUX_TIMING_TRACE
/* Helper structures for trace */
typedef struct vol_ctrl_packed_aux_state
{
    union {
        struct {
            unsigned aux_state       : 8;
            bool     aux_pending     : 8;
#ifdef VOLUME_CONTROL_AUX_TTP_SUPPORT
            bool     ttp_enabled     : 8;
            bool     generate_ttp    : 8;
#endif /* VOLUME_CONTROL_AUX_TTP_SUPPORT */
        } f;
        unsigned w;
    } u;
} VOL_CTRL_PACKED_AUX_STATE;
#endif /* VOLUME_CONTROL_AUX_TIMING_TRACE */

/****************************************************************************
Private Function Definitions
 */
extern bool setup_processing(VOL_CTRL_DATA_T   *op_extra_data);
extern void destroy_processing(VOL_CTRL_DATA_T   *op_extra_data);

/* Receive capability handler functions declarations */
extern bool vol_ctlr_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool vol_ctlr_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool vol_ctlr_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool vol_ctlr_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool vol_ctlr_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool vol_ctlr_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool vol_ctlr_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/* Data processing function */
extern void vol_ctlr_timer_task(void *kick_object);
extern void vol_ctlr_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched);

/* Operator message handlers */
extern bool vol_ctlr_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool vol_ctlr_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool vol_ctlr_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool vol_ctlr_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool vol_ctlr_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

extern bool vol_ctlr_opmsg_set_ucid(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool vol_ctlr_opmsg_get_ps_id(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

extern bool vol_ctlr_opmsg_set_sample_rate(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool vol_ctlr_opmsg_data_stream_based(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
#ifdef VOLUME_CONTROL_AUX_TTP_SUPPORT
extern bool vol_ctlr_opmsg_set_aux_time_to_play(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
bool vol_ctlr_opmsg_set_downstream_latency_est(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/* macro to get downstream latency estimate field, this
 * macro might slightly be different for downloadable builds.
 */
#define VOL_CTRL_DOWNSTREAM_LATENCY_EST(op_extra_data)\
   op_extra_data->parameter_downstream_latency_ms
#endif /* VOLUME_CONTROL_AUX_TTP_SUPPORT */
#endif  /* VOL_CTRL_CAP_C_H */
