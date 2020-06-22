/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \defgroup endpoints Endpoints
 * \ingroup stream
 *
 * \file stream_for_ops.h
 *
 * This file contains public stream functions that are used by operators and capabilities. <br>
 *
 */

#ifndef STREAM_FOR_OPS_H
#define STREAM_FOR_OPS_H

#include "types.h"
#include "status_prim.h"
#include "stream_prim.h"
#include "stream/stream_common.h"
#include "stream/stream_audio_data_format.h"
#include "adaptor/connection_id.h"
#include "buffer.h"

/**
 * \brief Get the system streaming rate
 *
 * \return system sampling_rate
 */
extern uint32 stream_if_get_system_sampling_rate(void);

/**
 * \brief Get the system kick period
 *
 * This is the maximum value across all endpoints which have configurable kick periods
 *
 * \return kick period in microseconds
 */
extern TIME_INTERVAL stream_if_get_system_kick_period(void);

/**
 * \brief get the endpoint that is connected to an operator's terminal
 *
 * \param opid operators id
 * \param terminal_id terminal ID
 *
 * \return connected operator to the terminal, NULL if not connected
 */
extern ENDPOINT *stream_get_connected_endpoint_from_terminal_id(unsigned opid,  unsigned terminal_id);

/**
 * \brief get current sample rate of an endpoint
 *
 * \param ep pointer to endpoint structure
 * \param value pointer to be populated with endpoint's sample rate
 *
 * \return TRUE if *value is populated with sample rate else FALSE.
 *
 * NOTE: Only endpoint types that that support EP_SAMPLE_RATE config key
 *       may return success.
 */
extern bool stream_get_sample_rate(ENDPOINT *ep, uint32 *value);

/**
 * \brief
 *
 * \param ep    Pointer to endpoint to configure
 * \param path
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_input(ENDPOINT *ep, STREAM_ANC_PATH path);

/**
 * \brief
 *
 * \param ep       Pointer to endpoint to configure
 * \param instance
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_instance(ENDPOINT *ep,
                                          STREAM_ANC_INSTANCE instance);

/**
 * \brief
 *
 * \param ep       Pointer to endpoint to configure
 * \param bitfield
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_control(ENDPOINT *ep, uint32 bitfield);

/**
 * \brief
 *
 * \param ep    Pointer to endpoint to configure
 * \param path
 * \param value
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_gain(ENDPOINT *ep, STREAM_ANC_PATH path,
                                      unsigned value);

/**
 * \brief
 *
 * \param ep    Pointer to endpoint to configure
 * \param path
 * \param value
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_gain_shift(ENDPOINT *ep,
                                            STREAM_ANC_PATH path,
                                            unsigned value);

/**
 * \brief
 *
 * \param ep    Pointer to endpoint to configure
 * \param path
 * \param value
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_dc_filter_enable(ENDPOINT *ep,
                                                  STREAM_ANC_PATH path,
                                                  unsigned value);

/**
 * \brief
 *
 * \param ep    Pointer to endpoint to configure
 * \param path
 * \param value
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_dc_filter_shift(ENDPOINT *ep,
                                                 STREAM_ANC_PATH path,
                                                 unsigned value);

/**
 * \brief
 *
 * \param ep    Pointer to endpoint to configure
 * \param path
 * \param value
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_dmic_x2_enable(ENDPOINT *ep,
                                                STREAM_ANC_PATH path,
                                                unsigned value);
/**
 * \brief Returns the number of coefficient of the filter
 *        in a specified path of an ANC block.
 *
 * \param path  path to query
 *
 * \return number coefficients
 */
extern unsigned stream_anc_get_filters_coeff_number(STREAM_ANC_PATH path);

/**
 * \brief Configure an ANC IIR filter (sets the coefficients)
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id      ANC input path ID (e.g. FFA, FFB, FB).
 * \param num_coeffs   Number of coefficients.
 * \param coeffs       Pointer to an array of IIR coefficients.
 *
 * \return TRUE if successful
 */
extern bool stream_anc_set_anc_iir_coeffs(STREAM_ANC_INSTANCE instance,
                                          STREAM_ANC_PATH path_id,
                                          unsigned num_coeffs,
                                          const uint16 *coeffs);

/**
 * \brief update coefficients from foreground to background
 * \param instance ANC instance ID (e.g. ANC0, ANC1).
 */

extern void stream_anc_update_background_iir_coeffs(STREAM_ANC_INSTANCE anc_instance);

/**
 * \brief Configure ANC activee iir coefficients
 *
 * \param instance ANC instance ID (e.g. ANC0, ANC1).
 * \param Bank type (e.g. foreground, background).
 */

extern void stream_anc_select_active_iir_coeffs(STREAM_ANC_INSTANCE anc_instance,
                                                STREAM_ANC_BANK coeff_set);

/**
 * \brief Configure ANC tuning options
 *
 * \param instance ANC instance ID (e.g. ANC0, ANC1).
 * \param chain.
 */
extern void stream_anc_set_anc_tune(STREAM_ANC_INSTANCE instance,
                                    unsigned chain);

/**
 * \brief Wrapper to enable/disable ANC with license check.
 *
 * \param con_id        Feature ID (passed as first parameter to the callback)
 * \param anc_enable_0  Bitfield controlling instance ANC0 signal paths.
 * \param anc_enable_1  Bitfield controlling instance ANC1 signal paths.
 * \param resp_callback callback function pointer for sending the response.
 *
 * \note: Calls the secure ANC interface and supplies a user callback of
 * the form:
 *
 * bool stream_anc_dummy_callback(unsigned dummy_con_id, unsigned dummy_status)
 *
 * This can be used to determine the completion status of the command.
 */
extern void stream_anc_enable_wrapper(CONNECTION_LINK con_id,
                                      uint16 anc_enable_0,
                                      uint16 anc_enable_1,
                                      bool (*resp_callback)(CONNECTION_LINK con_id,
                                                            STATUS_KYMERA status));

/**
 * \brief Configure an ANC LPF filter (sets the LPF coefficients)
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id      ANC input path ID (e.g. FFA, FFB, FB).
 * \param shift1       Coefficient 1 expressed as a shift.
 * \param shift2       Coefficient 2 expressed as a shift.
 *
 * \return TRUE if successful
 */
extern bool stream_anc_set_anc_lpf_coeffs(STREAM_ANC_INSTANCE instance,
                                          STREAM_ANC_PATH path_id,
                                          uint16 shift1,
                                          uint16 shift2);

/**
 * \brief Copy the foreground gain set to the background gain set
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 *
 * \note Gains for the FFA, FFB, and FB LPF are shadowed
 *       (but gain shifts are not)
 */
extern void stream_anc_update_background_gains(
    STREAM_ANC_INSTANCE anc_instance);

/**
 * \brief Retrieve the ANC enable/disable state
 *
 * \param anc_enable_0_ptr Pointer to result (bitfield controlling
 *                         instance ANC0 signal paths).
 * \param anc_enable_1_ptr Pointer to result (bitfield controlling
 *                         instance ANC1 signal paths).
 *
 */
extern void stream_get_anc_enable(uint16 *anc_enable_0_ptr,
                                  uint16 *anc_enable_1_ptr);

#if defined(INSTALL_ANC_CLIP_THRESHOLD)

/**
 * \brief Configure the clipping/threshold detection ANC output level
 *
 * \param anc_instance ANC instance
 * \param level        Threshold level to configure
 *
 * \return none
 */
extern void stream_anc_set_clip_level(STREAM_ANC_INSTANCE anc_instance,
                                      uint32 level);

/**
 * \brief Enable/disable the ANC output threshold detector
 *
 * \param anc_instance ANC instance
 * \param callback NULL:     disable ANC threshold detection
 *                 non-NUll: pointer to function to be called
 *                           on exceeding ANC detection threshold
 *
 * \return none
 */
extern void stream_anc_detect_enable(STREAM_ANC_INSTANCE anc_instance,
                                     void (*callback)(void));

#endif /* defined(INSTALL_ANC_CLIP_THRESHOLD) */

/**
 * \brief Place-holder function for patching purposes.
 *
 * \param ptr Pointer to function parameters (to be used as required)
 *
 */
extern void stream_anc_user1(void *ptr);

/**
 * \brief Place-holder function for patching purposes.
 *
 * \param ptr Pointer to function parameters (to be used as required)
 *
 */
extern void stream_anc_user2(void *ptr);

/**
 * \brief Enables the Sigma-Delta Modulator on the feedback tuning output.
 *
 * \param endpoint     Pointer to the endpoint.
 *
 * \return TRUE if successful, FALSE if error.
 */
extern bool stream_anc_enable_sdm(ENDPOINT *endpoint);

/** Indicates which filter input to connect to the feedback monitor. */
typedef enum
{
    ANC_FBMON_FFA = 0, /*!< Feed Forward Filter (typical outside microphone) */
    ANC_FBMON_FB = 1,  /*!< Feedback Filter */
} ANC_FBMON_SRC;

/**
 * \brief Connect Feedback monitor to an IIR filter input.
 *
 * \param endpoint Pointer to the endpoint.
 * \param source   Which IIR filter input to connect.
 *
 * \return TRUE if successful, FALSE if error.
 */
extern bool stream_anc_connect_feedback_monitor(ENDPOINT *endpoint,
                                                ANC_FBMON_SRC source);

/**
 * \brief get the cbuffer associated with a transform
 *
 * \param transform_id transform extrenal ID
 *
 * \return cbuffer associated with the transfor or NULL if not found
 */
extern tCbuffer* stream_get_buffer_from_external_transform_id(TRANSFORM_ID transform_id);

#endif /* STREAM_FOR_OPS_H */
