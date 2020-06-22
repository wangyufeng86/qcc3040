/****************************************************************************
 * Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup stream Stream Subsystem
 *
 * \file  stream_for_ompgr.h
 *
 * stream header file.
 * This file contains stream functions that are only publicly available to opmgr
 */

#ifndef STREAM_FOR_OPMGR_H
#define STREAM_FOR_OPMGR_H

#include "stream_common.h"

#ifdef INSTALL_UNINTERRUPTABLE_ANC

#include "hal_audio_anc.h"
#include "stream_prim.h"
#include "stream_type_alias.h"

#endif /* INSTALL_UNINTERRUPTABLE_ANC */

#if defined(INSTALL_AUDIO_LOOPBACK_ENHANCEMENTS)
/* Stream loopback enable enumeration type */
typedef enum {
    STREAM_TEST_ENABLES_KCODEC_LOOPBACK_OFF = 0,
    STREAM_TEST_ENABLES_KCODEC_SHORT_DIG_LOOPBACK = 1,
    STREAM_TEST_ENABLES_KCODEC_LONG_DIG_LOOPBACK = 2,
    STREAM_TEST_ENABLES_KCODEC_SHORT_ANA_LOOPBACK = 3,
    STREAM_TEST_ENABLES_KCODEC_LONG_ANA_LOOPBACK = 4
} stream_loopback_mode_type;

/**
 * \brief Control the Streplus audio loopback.
 *
 * \param stream_loopback_mode Desired loopback mode.
 * \param chan_swap_en         FALSE: No-swap, TRUE: Swap; channel swap.
 * \param pwm_to_ana_en        FALSE: Disable, TRUE: Enable; PWM to Analogue
 *                             control.
 * \param pwm_to_ana_lpf_en    FALSE: Disable, TRUE: Enable; PWM to Analogue
 *                             LPF control.
 *
 */
void stream_loopback_config_test_enables(
    stream_loopback_mode_type stream_loopback_mode,
    bool chan_swap_en,
    bool pwm_to_ana_en,
    bool pwm_to_ana_lpf_en);

/**
 * \brief Control the Streplus audio loopback routing.
 *
 * \param adc_channel Destination ADC channel.
 * \param dac_channel Source DAC channel.
 *
 */
void stream_loopback_config_routing(
    uint16 adc_channel,
    uint16 dac_channel);
#endif /* defined(INSTALL_AUDIO_LOOPBACK_ENHANCEMENTS) */

/**
 * \brief Kicks an endpoint from outside of streams.
 *
 * \param ep       The endpoint to call the kick function of.
 * \param kick_dir The direction that the kick has come from.
 *
 */
extern void stream_if_kick_ep(ENDPOINT *ep, ENDPOINT_KICK_DIRECTION kick_dir);

/**
 * \brief Given an endpoint id returns the endpoint that is connected to it.
 *
 * \param ep_id The id of the endpoint to work from.
 *
 * \return The endpoint which is connected to that represented by ep_id.
 *         If there is no connection then NULL is returned.
 */
extern ENDPOINT *stream_get_connected_ep_from_id(unsigned ep_id);

/**
 * \brief Accessor for getting the ID of an endpoint.
 *
 * \param ep The endpoint to get the ID of.
 *
 * \return The ID of the ep.
 */
extern unsigned stream_ep_id_from_ep(ENDPOINT *ep);

/**
 * \brief Generic function used to ask an endpoint not to bother kicking
 *        the endpoint it is connected to.
 *
 * \note Not every endpoint will take notice of this request.
 *
 * \param ep The endpoint that to ask to stop propagating kicks.
 */
extern void stream_disable_kicks_from_endpoint(ENDPOINT *ep);

/**
 * \brief Enable all operator endpoints specified by the operator external id
 *
 * \param opid The external operator id
 *
 */
extern void stream_enable_operator_endpoints(unsigned opid);

/**
 * \brief Disable all operator endpoints specified by the operator external id
 *
 * \param opid The external operator id
 *
 */
extern void stream_disable_operator_endpoints(unsigned opid);

/**
 * \brief Destroys all the endpoints of an operator. Sinks are destroyed
 *        first.
 *
 * \param opid        The external operator id
 * \param num_sinks   The maximum number of sinks the operator could have
 * \param num_sources The maximum number of sources the operator could have
 *
 * \return Whether all endpoints were successfully destroyed
 */
extern bool stream_destroy_all_operators_endpoints(unsigned opid,
                                                   unsigned num_sinks,
                                                   unsigned num_sources);

/**
 * \brief Cancel the kick at the end of the in place chain.
 *
 * \param ep Pointer to the endpoint to start the traverse with.
 */
extern void in_place_cancel_tail_kick(ENDPOINT *ep);

/**
 * \brief Enables a shadow endpoint.
 *        It will start any real endpoint that ep is connected to.
 *        The shadow endpoint will be marked as enabled.
 *
 * \param epid The external endpoint id
 */
extern void stream_enable_shadow_endpoint(unsigned epid);

/**
 * \brief Disables a shadow endpoint.
 *        It will stop any real endpoint that ep is connected to.
 *        The shadow endpoint will be marked as disabled.
 *
 * \param epid The external endpoint id
 */
extern void stream_disable_shadow_endpoint(unsigned epid);

#endif /* STREAM_FOR_OPMGR_H */

