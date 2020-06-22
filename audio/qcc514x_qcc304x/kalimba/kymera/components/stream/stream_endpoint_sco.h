/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup sco Sco endpoint
 * \ingroup endpoints
 * \file stream_endpoint_sco.h
 *
 */

#ifndef _STREAM_ENDPOINT_SCO_H_
#define _STREAM_ENDPOINT_SCO_H_

/****************************************************************************
Include Files
*/
#include "stream/stream.h"


/****************************************************************************
 * Private Macro Declarations
 */
#ifdef INSTALL_SCO_RX_TOA_METADATA_GENERATE
#if defined(INSTALL_TTP) && defined(INSTALL_METADATA)
#define SCO_RX_GENERATE_METADATA
#endif /* defined(INSTALL_TTP) && defined(INSTALL_METADATA) */
#endif /* INSTALL_SCO_RX_TOA_METADATA_GENERATE */

/****************************************************************************
 * Private function declarations: Sco common functions.
 */

/**
 * \brief Gets the unique identifier of a sco link's clock source, it's
 * wallclock for comparing with other sco endpoints.
 *
 * \param ep The sco endpoint to get the wallclock id of
 *
 * \return A unique identifier representing the wallclock for this link
 */
unsigned stream_sco_get_wallclock_id(ENDPOINT *ep);

/**
 * \brief get the audio data format of the underlying hardware associated with
 * the endpoint
 *
 * \param endpoint pointer to the endpoint to get the data format of.
 *
 * \return the data format of the underlying hardware
 */
AUDIO_DATA_FORMAT sco_get_data_format (ENDPOINT *endpoint);

/**
 * \brief set the audio data format that the endpoint will place in/consume from
 * the buffer
 *
 * \param endpoint pointer to the endpoint to set the data format of.
 * \param format AUDIO_DATA_FORMAT requested to be produced/consumed by the endpoint
 *
 * \return whether the set operation was successful
 */
bool sco_set_data_format (ENDPOINT *endpoint, AUDIO_DATA_FORMAT format);

/**
 * \brief Get the hci_handle from a sco endpoint
 *
 * \param endpoint pointer to the sco endpoint
 *
 * \return hci_handle
 *
 * \* note This is a function in the SCO endpoint file so that
 *         if the key is ever changed then this function should
 *         be changed along side.
 */
unsigned int stream_sco_get_hci_handle(ENDPOINT *endpoint);

/**
 * \brief Common function to obtain the timing information from the endpoint
 *
 * \param endpoint pointer to the sco endpoint
 *        time_info pointer to timing data structure
 *
 * \return None
 */
void sco_common_get_timing (ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info);

/****************************************************************************
 * Protected function declaration: Endpoint functions.
 */

/**
 * \brief Close the endpoint
 */
bool sco_close(ENDPOINT *endpoint);

/**
 * \brief Connect to the endpoint
 */
bool sco_connect(ENDPOINT *endpoint, tCbuffer *Cbuffer_ptr, ENDPOINT *ep_to_kick, bool* start_on_connect);

/**
 * \brief Disconnect from the endpoint
 */
bool sco_disconnect(ENDPOINT *endpoint);

/**
 * \brief Retrieve the buffer details from the endpoint
 */
bool sco_buffer_details(ENDPOINT *endpoint, BUFFER_DETAILS *details);

/**
 * \brief Make the endpoint run any data processing and propagate kick
 */
void sco_kick(ENDPOINT *endpoint, ENDPOINT_KICK_DIRECTION kick_dir);

/**
 * \brief The sco endpoint is responsible for scheduling chain kicks:
 *        this function is called to perform any real-time scheduling
 *        that needs to occur per kick
 */
void sco_sched_kick(ENDPOINT *endpoint, KICK_OBJECT *ko);

/**
 * \brief The endpoint is responsible for scheduling chain kicks:
 *        this function initiates a kick interrupt source to start producing kicks.
 */
bool sco_start(ENDPOINT *endpoint, KICK_OBJECT *ko);

/**
 * \brief The endpoint is responsible for scheduling chain kicks:
 *        this function cancels the associated kick interrupt source.
 */
bool sco_stop(ENDPOINT *endpoint);

/**
 * \brief Configure the sco endpoint
 */
bool sco_configure(ENDPOINT *endpoint, unsigned int key, uint32 value);

/**
 * \brief Get endpoint configuration
 */
bool sco_get_config(ENDPOINT *endpoint, unsigned int key, ENDPOINT_GET_CONFIG_RESULT* result);

/**
 * \brief Obtain the timing information from the endpoint
 */
void sco_get_timing (ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info);

/**
 * \brief Report whether two endpoints have the same clock source
 */
bool sco_have_same_clock(ENDPOINT *ep1, ENDPOINT *ep2, bool both_local);


#endif /* !_STREAM_ENDPOINT_SCO_H_ */
