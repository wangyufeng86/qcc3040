/****************************************************************************
 * Copyright (c) 2014 - 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup stream Stream Subsystem
 * \file  stream_for_adaptors.h
 * \ingroup stream
 *
 * Stream header file used by adaptor(s).
 *
 */

#ifndef STREAM_FOR_OVERRIDE_H
#define STREAM_FOR_OVERRIDE_H

#include "stream_common.h"
#include "stream_for_opmgr.h"

#include "stream/stream_common.h"


typedef struct ENDPOINT_GET_CONFIG_RESULT ENDPOINT_GET_CONFIG_RESULT;

typedef enum OVERRIDE_CONFIG_KEYS
{
    /* These values should match with ACCMD_INFO_KEY values */
    OVERRIDE_AUDIO_SAMPLE_RATE = 0x0000,

    /* These values should match with ENDPOINT_INT_CONFIGURE_KEYS values */
    OVERRIDE_RATEMATCH_ABILITY = 0x010007,
    OVERRIDE_RATEMATCH_ENACTING = 0x010008,
    OVERRIDE_RATEMATCH_RATE = 0x010009,
    OVERRIDE_RATEMATCH_ADJUSTMENT =  0x01000A,
    OVERRIDE_SET_INPUT_GAIN = 0x01000E,
    OVERRIDE_SET_OUTPUT_GAIN = 0x01000F,
    OVERRIDE_RATE_ADJUST_OP = 0x01001A,
    OVERRIDE_HW_WARP_APPLY_MODE = 0x01001B,
    OVERRIDE_CURRENT_HW_WARP = 0x01001C
} OVERRIDE_CONFIG_KEYS;

/**
 * \brief Get configuration from the connected endpoint.
 *
 * \param endpoint     Pointer to an endpoint
 * \param key          ID of the configuration
 * \param config_value Passed on further as ENDPOINT_GET_CONFIG_RESULT value
 * \param result_value Pointer to the result value
 *
 * \return TRUE if successful, FALSE for error(s)
 */
extern bool stream_get_connected_to_endpoint_config(ENDPOINT *endpoint,
                                                    OVERRIDE_CONFIG_KEYS key,
                                                    uint32 config_value,
                                                    uint32 *result_value);

/**
 * \brief Set configuration for the connected endpoint.
 *
 * \param endpoint Pointer to an endpoint
 * \param key      ID of the configuration
 * \param value    Pointer to the value.
 *
 * \return TRUE if successful, FALSE for error(s)
 */
extern bool stream_configure_connected_to_endpoint(ENDPOINT *endpoint,
                                                   OVERRIDE_CONFIG_KEYS key,
                                                   uint32 value);

/**
 * \brief Function to check if the connected endpoint is clocked locally or
 *        remotely.
 *
 * \param  endpoint  pointer to an endpoint
 *
 * \return TRUE if the connected endpoint is locally clocked, FALSE otherwise.
 */
extern bool stream_connected_to_endpoint_is_locally_clocked(ENDPOINT *endpoint);

/**
 * \brief Function to tests whether two connected to endpoints use the same
 *        clock source
 *
 * \param ep1 Pointer to an endpoint
 * \param ep2 Pointer to another endpoint
 *
 * \return TRUE if the two connected endpoints have same clock source, FALSE
 *         otherwise.
 */
extern bool stream_connected_to_endpoints_have_same_clock_source(ENDPOINT *ep1,
                                                                 ENDPOINT *ep2);
/**
 * \brief  Function to tests whether there is a sidetone path enabled between two
 * overridden endpoints.
 *
 * \param  mic_hdl The handle for the operator endpoint which overrides a real
 *                 mic endpoint.
 * \param  spkr_hdl The second handle for the operator endpoint which overrides
 *                 a real speaker endpoint.
 *
 * \return TRUE if there is sidetone path between two overridden endpoints else FALSE.
 */
extern bool stream_connected_to_endpoints_have_sidetone_route(ENDPOINT *mic_ep,
                                                              ENDPOINT *spkr_ep);

/**
 * \brief Finds the endpoint whose clock source is seen at the kymera side
 *        boundary of an endpoint.
 *
 * \param ep The endpoint whose boundary the clock source is requested of
 *
 * \return The endpoint whose clock source is present at the boundary. This may
 *         be the endpoint ep.
 */
extern ENDPOINT *stream_rm_get_clk_src_from_pair(ENDPOINT *ep);

/**
 * \brief computes the rate adjustment between endpoints
 *
 * \param ep_src Handle of the source endpoint
 * \param src_rate Rate of source if not enacting endpoint
 * \param ep_sink Handle of sink the endpoint
 * \param sink_rate Rate of sink if not enacting endpoint
 *
 * \return The rate adjustment, If endpoints invalid then zero.
 */
extern unsigned stream_rm_get_rate_adjustment(ENDPOINT *ep_src,
                                              unsigned src_rate,
                                              ENDPOINT *ep_sink,
                                              unsigned sink_rate);

/**
 * \brief Obtains whether the endpoint is a source or a sink.
 *
 * \param endpoint Pointer to the endpoint that we want the direction from
 *
 * \return Whether the endpoint is a source or a sink.
 */
ENDPOINT_DIRECTION stream_direction_from_endpoint(ENDPOINT *endpoint);

#endif /* STREAM_FOR_OVERRIDE_H */
