/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup Audio Audio endpoint
 * \ingroup endpoints
 * \file stream_endpoint_audio.h
 *
 */

#ifndef STREAM_ENDPOINT_AUDIO_H
#define STREAM_ENDPOINT_AUDIO_H

/****************************************************************************
Include Files
*/
#include "stream_common.h"
#include "cbops_mgr/cbops_mgr.h"

/****************************************************************************
Private Constant declarations
*/

#define AUDIO_BUFFER_SIZE 128

/** Max number of sync'ed endpoints, to be updated when more needed */
#define NR_MAX_SYNCED_EPS (CBOPS_MAX_NR_CHANNELS)

/** Amount of sink processing time that all the cbops can take without the headroom
 *  getting used up in chains between start and first kick.
 *  The time spent from start to first kick on any
 *  channel has to be less than the time represented by the headroom samples
 *  placed at start in the sink output buffer - otherwise by the time we get to
 *  first kick on some channel(s), that buffer wraps. Some empirical values are
 *  unavoidable, these take into account the measurements and also the
 *  algorithmic needs.
 */
#if defined(CHIP_BASE_HYDRA)
#define CBOP_PROCESSING_TIME_ALLOWANCE_IN_USECS 850
#elif defined(CHIP_BASE_BC7)
#define CBOP_PROCESSING_TIME_ALLOWANCE_IN_USECS 750
#endif

/****************************************************************************
Private Type Declarations
*/

/****************************************************************************
Public Macro Declarations for accessing audio endpoint "base class" info.
*/

/* Masks for the audio endpoint key fields.
 * The key for audio encodes the hardware, instance and channel into 24
 * bits. Bits 3:0 are the channel, bits 7:4 the instance, bits 15:8 the
 * hardware type and bits 16:23 are always zero.
 */
#define AUDIO_EP_DEVICE_MASK                      0x00FF
#define AUDIO_EP_DEVICE_SHIFT                     8
#define AUDIO_EP_INSTANCE_MASK                    0x000F
#define AUDIO_EP_INSTANCE_SHIFT                   4
#define AUDIO_EP_CHANNEL_MASK                     0x000F
#define AUDIO_EP_CHANNEL_SHIFT                    0

/* Get device ID from endpoint key. There can be other such macros to extract the other parts,
 * as & when needed.
 */
#define GET_DEVICE_FROM_AUDIO_EP_KEY(key)  (((key) >> AUDIO_EP_DEVICE_SHIFT) & AUDIO_EP_DEVICE_MASK)
#define GET_INSTANCE_FROM_AUDIO_EP_KEY(key)  (((key) >> AUDIO_EP_INSTANCE_SHIFT) & AUDIO_EP_INSTANCE_MASK)
#define GET_CHANNEL_FROM_AUDIO_EP_KEY(key)  (((key) >> AUDIO_EP_CHANNEL_SHIFT) & AUDIO_EP_CHANNEL_MASK)

/** The number of samples headroom to allow in the buffer to compensate for any
 * rate missmatch variation in buffer levels before the RM system compensates */
#define AUDIO_RM_HEADROOM_AMOUNT    2

/****************************************************************************
Public Function Declarations
*/

/****************************************************************************
Private Function Declarations
*/

/**
 * \brief audio_kick function
 *
 * \param endpoint pointer to the endpoint that received a kick
 * \param kick_dir direction of the kick
 */
extern void stream_audio_kick(ENDPOINT *endpoint, ENDPOINT_KICK_DIRECTION kick_dir);

/**
 * \brief Determines whether two audio endpoints share a clock source.
 *
 * \param ep1        First endpoint
 * \param ep2        Second endpoint
 * \param both_local Caller should query timing on both endpoints and pass on
 *                   whether both endpoints are locally clocked. This is not
 *                   done inside the call because it may be cached in the
 *                   caller.
 * \return True if both endpoints have the same clock source
 */
extern bool stream_audio_have_same_clock(ENDPOINT *ep1,
                                         ENDPOINT *ep2,
                                         bool both_local);

/**
 * \brief Obtains rate measurement data
 *
 * \param endpoint pointer to the endpoint that received a kick
 * \return delta samples from the last kick (OR) an indication
           that delta samples from the last kick wasn't computed
 */
extern unsigned stream_audio_get_rm_data(ENDPOINT *endpoint);

/**
 * \brief Accumulates rate measurement data
 *
 * \param endpoint pointer to the endpoint that received a kick
 * \param num_cbops_read Count of samples consumed by endpoint's cbops
 * \param num_cbops_written Count of samples produced by endpoint's cbops
 *
 * \return delta samples from the last kick (OR) an indication
           that delta samples from the last kick wasn't computed
 */
extern void stream_audio_process_rm_data(ENDPOINT *endpoint,
                                         unsigned num_cbops_read,
                                         unsigned num_cbops_written);

/**
 * \brief Set cbops values for an endpoint.
 *
 * \param ep    pointer to an endpoint that owns a cbops chain
 * \param vals  pointer to structure of cbop values used by cbops mgr in chain creation
 */
extern void stream_audio_set_cbops_param_vals(ENDPOINT *ep,
                                              CBOP_VALS *vals);

/**
 * \brief Obtain the audio buffer length based on the sample rate of the
 *        endpoint. The buffer must be big enough to hold samples arrived in
 *        two kick periods.
 *
 * \param sample_rate of the endpoint.
 * \param dir direction of the endpoint
 * \param get_hw_size TRUE to round up to a power of two for MMU buffers
 *
 * \return length of the required buffer
 */
extern unsigned int stream_audio_get_buffer_length(uint32 sample_rate,
                                                   ENDPOINT_DIRECTION dir,
                                                   bool get_hw_size);

#endif /* STREAM_ENDPOINT_AUDIO_H */
