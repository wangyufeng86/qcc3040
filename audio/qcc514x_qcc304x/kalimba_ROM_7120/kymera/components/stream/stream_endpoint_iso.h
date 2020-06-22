/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup sco ISO endpoint
 * \ingroup endpoints
 * \file stream_endpoint_iso.h
 *
 */

#ifndef _STREAM_ENDPOINT_ISO_H_
#define _STREAM_ENDPOINT_ISO_H_

/****************************************************************************
Include Files
*/
#include "stream/stream.h"
#include "stream_endpoint_sco.h"

/****************************************************************************
 * Private Macro Declarations
 */
#ifdef INSTALL_ISO_CHANNELS

/****************************************************************************
 * Protected function declaration: Endpoint functions.
 */

/**
 * \brief Close the endpoint
 *
 * \param *endpoint pointer to the endpoint that is being closed
 *
 * \return success or failure
 */
bool iso_close(ENDPOINT *endpoint);

/**
 * \brief Connect to the endpoint
 *
 * \param *endpoint pointer to the endpoint that is being connected
 * \param *Cbuffer_ptr pointer to the Cbuffer struct for the buffer that is being connected.
 * \param *ep_to_kick pointer to the endpoint which will be kicked after a successful
 *              run. Note: this can be different from the connected to endpoint when
 *              in-place running is enabled.
 * \param *start_on_connect return flag which indicates if the endpoint wants be started
 *              on connect. Note: The endpoint will only be started if the connected
 *              to endpoint wants to be started too.
 *
 * \return success or failure
 */
bool iso_connect(ENDPOINT *endpoint, tCbuffer *Cbuffer_ptr, ENDPOINT *ep_to_kick, bool* start_on_connect);

/**
 * \brief Disconnect from the endpoint
 *
 * \param *endpoint pointer to the endpoint that is being disconnected
 *
 * \return success or failure
 */
bool iso_disconnect(ENDPOINT *endpoint);

/**
 * \brief Retrieve the buffer details from the endpoint
 *
 * \param *endpoint pointer to the endpoint that is being queried
 * \param *details pointer to the buffer_detailes structure
 *
 * \return success or failure
 */
bool iso_buffer_details(ENDPOINT *endpoint, BUFFER_DETAILS *details);

/**
 * \brief Make the endpoint run any data processing and propagate kick
 *
 * \param *endpoint pointer to the endpoint that is being kicked
 * \param kick_dir direction to kick
 *
 * \return None
 */
void iso_kick(ENDPOINT *endpoint, ENDPOINT_KICK_DIRECTION kick_dir);

/**
 * \brief The iso endpoint is responsible for scheduling chain kicks:
 *        this function is called to perform any real-time scheduling
 *        that needs to occur per kick
 *
 * \param *endpoint pointer to the endpoint that is being scheduled
 * \param *ko pointer to the kick object to be kicked
 *
 * \return None
 */
void iso_sched_kick(ENDPOINT *endpoint, KICK_OBJECT *ko);

/**
 * \brief The endpoint is responsible for scheduling chain kicks:
 *        this function initiates a kick interrupt source to start producing kicks.
 *
 * \param *endpoint pointer to the endpoint that is being started
 * \param *ko pointer to the kick object to be kicked first
 *
 * \return success or failure
 */
bool iso_start(ENDPOINT *endpoint, KICK_OBJECT *ko);

/**
 * \brief The endpoint is responsible for scheduling chain kicks:
 *        this function cancels the associated kick interrupt source.
 *
 * \param *endpoint pointer to the endpoint that is being stopped
 *
 * \return success or failure
 */
bool iso_stop(ENDPOINT *endpoint);

/**
 * \brief Configure the endpoint
 *
 * \param *endpoint pointer to the endpoint that is being configured
 *
 * \return success or failure
 */
bool iso_configure(ENDPOINT *endpoint, unsigned int key, uint32 value);

/**
 * \brief Get endpoint configuration
 *
 * \param *endpoint pointer to the endpoint that is being queried
 *
 * \return success or failure
 */
bool iso_get_config(ENDPOINT *endpoint, unsigned int key, ENDPOINT_GET_CONFIG_RESULT* result);

/**
 * \brief Obtain the timing information from the endpoint
 *
 * \param *endpoint pointer to the endpoint that is being queried
 * \param *time_info pointer to timing information structure
 *
 * \return None
 */
void iso_get_timing (ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info);

/**
 * \brief Report whether two endpoints have the same clock source
 *
 * \param *ep1 Endpoint to compare with ep2.
 * \param *ep2 Endpoint to compare with ep1.
 * \param both_local boolean indicating if both endpoints are locally clocked.
 *
 * \return TRUE if ep1 and ep2 share a clock source, otherwise FALSE.
 */
bool iso_have_same_clock(ENDPOINT *ep1, ENDPOINT *ep2, bool both_local);

/****************************************************************************
 * Private function declaration: ISO private functions.
 */

/**
 * \brief Create a new ISO endpoint
 *
 * \param key unique key for ISO endpoints
 * \param dir endpooint direction (source/sink)
 *
 * \return pointer to created endpoint
 */
ENDPOINT * iso_create_endpoint(unsigned key, ENDPOINT_DIRECTION dir);

/**
 * \brief Get pointer to an existing ISO endpoint
 *
 * \param key unique key for ISO endpoints
 * \param dir endpooint direction (source/sink)
 *
 * \return pointer to endpoint
 */
ENDPOINT *iso_get_endpoint(unsigned int key, ENDPOINT_DIRECTION dir);


#endif /* INSTALL_ISO_CHANNELS */

#endif /* !_STREAM_ENDPOINT_SCO_H_ */
