/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  ttp.h
 * \ingroup ttp
 * 
 * Public header file for the time-to-play (TTP) module.
 */

#ifndef TTP_H
#define TTP_H

#include "types.h"

/**
 * TTP mode 
 */
typedef enum 
{
    TTP_MODE_FULL_TTP, 
    TTP_MODE_FREE_RUN,
    TTP_MODE_FULL_TTP_ONLY,
    TTP_MODE_FREE_RUN_ONLY
} ttp_mode;


typedef struct ttp_context ttp_context;

/** 
 * Status structure
 * This returns all the data a typical TTP client needs 
 */
typedef struct 
{
    TIME            ttp;                /**< Calculated time to play */
    int             sp_adjustment;      /**< Current sample period adjustment */
    unsigned        err_offset_id;      /**< ID for error offset */
    bool            stream_restart;     /**< TRUE if the stream has restarted */
}
ttp_status;

/** 
 * Parameters structure
 * Various configurable values affecting the TTP generation
 */
typedef struct 
{
    unsigned        nominal_sample_rate;
    unsigned        filter_gain;
    unsigned        err_scale;
    TIME_INTERVAL   startup_period;
}
ttp_params;

/** 
 * Parameters structure for the ttp state params
 * The values used to set the initial ttp state.
 */
typedef struct 
{
    TIME            ttp;               /* time to play value */
    int             sp_adjustment;     /* sample  period adjustment */
    TIME_INTERVAL   latency;           /* target latency */
    ttp_mode         mode;             /* TTP startup mode */
}
ttp_state_params;


typedef enum 
{
    TTP_TYPE_NONE,
    TTP_TYPE_PCM,
    TTP_TYPE_A2DP
} ttp_source_type;

#define INFO_ID_INVALID 0

/**
 * \brief Initialise time to play.
 *
 * Typically called at operator create. Initialises the time to play
 * functionality for a new instance, and returns a pointer to a
 * ttp_context structure. This should be stored by the caller for
 * subsequent use, and eventually freed by calling ttp_free.
 *
 * \return Pointer to a new (empty) ttp_context structure.
 */
extern ttp_context *ttp_init(void);

/**
 * \brief Reset time-to-play.
 *
 * Resets TTP state without changing parameters. Typically used for streams
 * that can stop and restart.
 * 
 * \param context Pointer to active TTP context structure.
 */
extern void ttp_reset(ttp_context *context);

/**
 * \brief Populate TTP parameters
 *
 * Populates a ttp_params structure with some appropriate default values 
 * for the given source type.
 *
 * \param params Pointer to structure to be populated.
 * \param source_type Enum describing the nature of the source data.
 */
extern void ttp_get_default_params(ttp_params *params,
                                   ttp_source_type source_type);

/**
 * \brief Extract target latency from operator message.
 *
 * Helper function to extract the target latency value from 
 * a received OPMSG_COMMON_SET_TTP_LATENCY message.
 *
 * \param message_data Pointer to message payload.
 *
 * \return Latency value extracted from payload.
 */
extern TIME_INTERVAL ttp_get_msg_latency(void *message_data);

/**
 * \brief Extract sample period adjustment from operator message
 *
 * Helper function to extract the sample period adjustment value from 
 * a received OPMSG_COMMON_SET_TTP_SPADJ message.
 *
 * \param message_data Pointer to message payload.
 *
 * \return Sample period adjustment value extracted from payload.
 */
extern int ttp_get_msg_sp_adjustment(void *message_data);

/**
 * \brief  Extract signed value to adjust ttp timestamp from the operator message.
 *
 * \param  message_data   pointer to message payload
 *
 * \return ttp_adjust     Signed value extracted from the payload. This is 
 *                        the time interval to adjust the next ttp generation.
 *
 * Helper function to extract the signed value to adjust ttp timestamp from 
 * a received OPMSG_COMMON_ADJUST_TTP_TIMESTAMP message
 */
extern TIME_INTERVAL ttp_get_msg_adjust_ttp_timestamp(void *message_data);

/**
 * \brief  Extract latency limits from operator message.
 *
 * Helper function to extract the latency limits from 
 * a received OPMSG_COMMON_SET_LATENCY_LIMITS message
 *
 * \param message_data Pointer to message payload.
 * \param min_latency  Pointer to store low limit value.
 * \param max_latency  Pointer to store high limit value.
 */
extern void ttp_get_msg_latency_limits(void *message_data,
                                       TIME_INTERVAL *min_latency,
                                       TIME_INTERVAL *max_latency);

/**
 * \brief Extract TTP parameters from operator message.
 *
 * Helper function to extract the parameter values from
 * a received OPMSG_COMMON_SET_TTP_PARAMS message.
 *
 * \param params       Pointer to parameter struct to populate.
 * \param message_data Pointer to message payload.
 */
extern void ttp_get_msg_params(ttp_params *params,
                               void *message_data);

/**
 * \brief Set the ttp state parameters from the operator message.
 *
 * Helper function to extract the state parameter values from
 * a received OPMSG_COMMON_SET_TTP_STATE message.
 * 
 * \param state_params Pointer to state struct to populate.
 * \param message_data Pointer to message payload.
 *
 */
extern void ttp_get_msg_state_params(ttp_state_params *state_params,
                                     void *message_data);

/**
 * \brief Get the current ttp state status of the provided context
 *
 *  Function to retrieve the current ttp state status information
 *
 *  \param context           Pointer to the TTP context structure
 *  \param ttp_state_params  pointer to the ttp state params to get the state
 *  \params ttp_resync_count number of times ttp resynchronised since startup
 */
extern void ttp_get_state_params_status(ttp_context *context,
                                        ttp_state_params *state_params,
                                        unsigned *ttp_resync_count); 

/**
 * \brief Configure TTP target latency
 *
 * Typically called at operator start, when all the required information
 * is available.
 *
 * \param context        Pointer to active TTP context structure.
 * \param target_latency Required target latency.
 */
extern void ttp_configure_latency(ttp_context *context,
                                  TIME_INTERVAL target_latency);

/**
 * \brief Configure TTP sample period adjustment.
 *
 * Typically called by the application while running ttp in the free run mode
 * to correct the sample peroid adjustment especially in TWS+ use cases.
 *
 * \param context       Pointer to active TTP context structure.
 * \param sp_adjustment Required sample period adjustment.
 *
 * \return bool  return TRUE if sp_adjument is set successfully
 */
extern bool ttp_configure_sp_adjustment(ttp_context *context,
                                        int sp_adjustment);

/**
 * \brief  Adjust the TTP timestamp for next generating timestamp.
 *
 * \param  context         pointer to active TTP context structure.
 *
 * \param  ttp_adjust      time interval to adjust the next timestamp.
 *                         This can be negative to adjust ttp to an early time. 
 *
 * signed value to adjust the next timestamp
 * Typically called by the application while running ttp in the free run mode to  syync up the 
 * ttp of the primary and secondary earbuds especially in TWS+ use cases.
 */
extern void ttp_adjust_ttp_timestamp(ttp_context *context, TIME_INTERVAL ttp_adjust);

/**
 * \brief  Configure TTP latency limits.
 *
 * A max_latency value of zero means there is no upper limit
 * (other than that implied by available buffering)
 * 
 * \param context     Pointer to active TTP context structure.
 * \param min_latency Minimum latency value to trigger a reset.
 * \param max_latency Maximum latency value to trigger a reset.
 */
extern void ttp_configure_latency_limits(ttp_context *context,
                                         TIME_INTERVAL min_latency,
                                         TIME_INTERVAL max_latency);

/**
 * \brief Configure TTP context parameters.
 *
 * Configures sample period estimation parameters. Typically called at operator
 * start, when all the required information is available. Note that the sample
 * rate might not be known at this point. If the nominal_sample_rate value in
 * the supplied structure is zero, the existing context value will be unchanged.
 *
 * \param context Pointer to active TTP context structure.
 * \param params  Pointer to parameter data.
 */
extern void ttp_configure_params(ttp_context *context,
                                 const ttp_params *params);

/**
 * \brief Configure TTP state parameters.
 *
 * \param context      Pointer to active TTP context structure.
 * \param state_params Pointer to the state parameter data.
 */
extern void ttp_configure_state_params(ttp_context *context,
                                       const ttp_state_params *state_params);

/**
 * \brief Configure TTP sample rate.
 *
 * Configures nominal sample rate for TTP calculations. Typically called at
 * operator start, when all the required information is available.
 *
 * \param context     Pointer to active TTP context structure.
 * \param sample_rate Nominal sample rate, in Hz.
 */
extern void ttp_configure_rate(ttp_context *context,
                               unsigned sample_rate);

/**
 * \brief Update TTP context and return new time-to-play.
 *
 * \param context Pointer to active TTP context structure.
 * \param time    Local microsecond time when some data was received (ToA).
 * \param samples Number of samples in the data.
 * \param status  Pointer to status structure for return data.
 *
 * \note This should be called each time the TTP-generating operator receives
 * some data. It updates internal state based on the given time and number of
 * samples, and provides the calculated time to play for the first sample.
 */
extern void ttp_update_ttp(ttp_context *context,
                           TIME time,
                           unsigned samples,
                           ttp_status *status);

/**
 * \brief  Update TTP context, using source-relative timestamp
 *
 * \param context     Pointer to active TTP context structure.
 * \param toa         Local microsecond time when some data was received.
 * \param source_time Source timestamp.
 * \param status      Pointer to status structure for return data.
 *
 * \note This should be called each time the TTP-generating operator receives
 * some data. It updates internal state based on the given ToA and number of
 * samples, and provides the calculated time to play for the first sample.
 */
extern void ttp_update_ttp_from_source_time(ttp_context *context,
                                            TIME toa,
                                            TIME source_time,
                                            ttp_status *status);

/**
 * \brief Get current sample-period adjustment factor.
 *
 * \param context Pointer to active TTP context structure.
 *
 * \return calculated SP adjustment value (fractional difference from nominal)
 *
 * \note This information is also available in the status structure populated
 *       by ttp_update_ttp. A separate access function is provided in case
 *       clients need the adjustment independently from calculating a new TTP
 *       value.
 */
extern int ttp_get_sp_adjustment(ttp_context *context);

/**
 * \brief Function calculates the timestamp for a new tag based on a
 *        previous tag.
 *
 *  new timestamp [us] = last timestamp +
 *                       ((samples to new * 1 000 000) / sample rate [samples/sec]) *
 *                        (1 + sample period adjustment)
 *
 * \param last_timestamp Baseline timestamp.
 * \param nr_of_samples  Samples available to consume.
 * \param sample_rate    The rate in which the samples are consumed in samples/sec.
 * \param sp_adjust      Fractional sample period adjustment value.
 *
 * \return New timestamp.
 */
extern unsigned ttp_get_next_timestamp(unsigned last_timestamp,
                                       unsigned nr_of_samples,
                                       unsigned sample_rate,
                                       int sp_adjust);

/**
 * \brief Function calculates the timestamp for a new tag based on a
 *        later tag.
 *
 *  new timestamp [us] = last timestamp -
 *                       ((samples to new * 1 000 000) / sample rate [samples/sec]) *
 *                        (1 + sample period adjustment)
 *
 * \param last_timestamp Baseline timestamp.
 * \param nr_of_samples  Samples available to consume.
 * \param sample_rate    The rate in which the samples are consumed in samples/sec.
 * \param sp_adjust      Fractional sample period adjustment value.
 *
 * \return New timestamp.
 */
extern unsigned ttp_get_prev_timestamp(unsigned last_timestamp,
                                       unsigned nr_of_samples,
                                       unsigned sample_rate,
                                       int sp_adjust);

/**
 * \brief  Free TTP context
 *
 * \param  context   pointer to context structure to be freed.
 *
 * Frees the previously-allocated context structure, typically called at operator destroy.
 * This is probably just a thin wrapper round pfree/pdelete,
 * but the caller shouldn't assume that to be the case.
 * Freeing a NULL context pointer is safely ignored
 */
extern void ttp_free(ttp_context *context);

/* Functions from ttp_info.c */

/**
 * \brief Create a new info entry.
 *
 * \param data Pointer to data item to pass in metadata.
 * 
 * \return Identifier for data item, or zero if the entry creation fails.
 */
extern unsigned ttp_info_create(void *data);

/**
 * \brief Get a pointer from an info entry.
 *
 * \param id Info entry identifier.
 * 
 * \return Pointer to data item, or NULL if invalid.
 */
extern void *ttp_info_get(unsigned id);

/**
 * \brief Destroy an info entry.
 *
 * \param id Info entry identifier.
 * 
 * \return TRUE if destroyed, or FALSE if invalid.
 */
extern bool ttp_info_destroy(unsigned id);

#endif /* TTP__H */
