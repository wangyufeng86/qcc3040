/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_iso_internal.c
 * \ingroup stream
 *
 * stream iso type file. <br>
 * This file contains stream functions for iso endpoints. <br>
 *
 */

/****************************************************************************
Include Files
*/

#include "stream_private.h"
#include "stream_endpoint_iso.h"
#include "stream/stream_for_sco_operators.h"

/****************************************************************************
Private Type Declarations
*/

/****************************************************************************
Private Constant Declarations
*/
/****************************************************************************
Private Macro Declarations
*/
#ifdef INSTALL_ISO_CHANNELS

/****************************************************************************
Private Function Declarations
*/
static void set_from_air_iso_specific_info(unsigned int sink_id,
                                           unsigned int transport_delay);
static bool set_from_air_iso_specific_info_cback(CONNECTION_LINK con_id,
                                                 STATUS_KYMERA status,
                                                 unsigned op_id,
                                                 unsigned num_resp_params,
                                                 unsigned *resp_params);

/****************************************************************************
Private Variable Definitions
*/

DEFINE_ENDPOINT_FUNCTIONS(iso_functions, iso_close, iso_connect,
                          iso_disconnect, iso_buffer_details,
                          iso_kick, iso_sched_kick, iso_start,
                          iso_stop, iso_configure, iso_get_config,
                          iso_get_timing, stream_sync_sids_dummy,
                          iso_have_same_clock);

/****************************************************************************
Protected Functions
*/

/**
 * Close the endpoint
 */
bool iso_close(ENDPOINT *endpoint)
{
    patch_fn_shared(stream_iso);
    return sco_close(endpoint);
}

/**
 * Connect to the endpoint
 */
bool iso_connect(ENDPOINT *endpoint, tCbuffer *Cbuffer_ptr, ENDPOINT *ep_to_kick, bool* start_on_connect)
{
    patch_fn_shared(stream_iso);
    return sco_connect(endpoint, Cbuffer_ptr, ep_to_kick, start_on_connect);
}

/**
 * Disconnect from the endpoint
 */
bool iso_disconnect(ENDPOINT *endpoint)
{
    patch_fn_shared(stream_iso);
    return sco_disconnect(endpoint);
}

/**
 * Retrieve the buffer details from the endpoint
 */
bool iso_buffer_details(ENDPOINT *endpoint, BUFFER_DETAILS *details)
{
    patch_fn_shared(stream_iso);
    return sco_buffer_details(endpoint, details);
}

/**
 * Make the endpoint run any data processing and propagate kick
 */
void iso_kick(ENDPOINT *endpoint, ENDPOINT_KICK_DIRECTION kick_dir)
{
    patch_fn_shared(stream_iso);
    sco_kick(endpoint, kick_dir);
}

/**
 * Perform any real-time scheduling that needs to occur per kick
 */
void iso_sched_kick(ENDPOINT *endpoint, KICK_OBJECT *ko)
{
    patch_fn_shared(stream_iso);
    sco_sched_kick(endpoint, ko);
}

/**
 * Initiate a kick interrupt source to start producing kicks
 */
bool iso_start(ENDPOINT *endpoint, KICK_OBJECT *ko)
{
    unsigned handle = 0;
    patch_fn_shared(stream_iso);

    if (!sco_start(endpoint, ko))
    {
        /* something went wrong */
        return FALSE;
    }

    if (endpoint->direction == SOURCE)
    {
        handle = stream_sco_get_hci_handle(endpoint);
        if (sco_params_received_get(handle))
        {
            /* Tell the connected operator what the transport delay is */
            set_from_air_iso_specific_info(endpoint->connected_to->id,
                                sco_iso_transport_delay_get(handle));

        }
    }
    return TRUE;
}

/**
 * Cancel the associated kick interrupt source
 */
bool iso_stop(ENDPOINT *endpoint)
{
    patch_fn_shared(stream_iso);
    return sco_stop(endpoint);
}

/**
 * Configure the endpoint
 */
bool iso_configure(ENDPOINT *endpoint, unsigned int key, uint32 value)
{
    patch_fn_shared(stream_iso);
    return sco_configure(endpoint, key, value);
}

/**
 * Get endpoint configuration
 */
bool iso_get_config(ENDPOINT *endpoint, unsigned int key, ENDPOINT_GET_CONFIG_RESULT* result)
{
    patch_fn_shared(stream_iso);
    return sco_get_config(endpoint, key, result);
}

/**
 * Obtain the timing information from the endpoint
 */
void iso_get_timing (ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info)
{
    patch_fn_shared(stream_iso);
    sco_get_timing(endpoint, time_info);
}

/**
 * Report whether two endpoints have the same clock source
 */
bool iso_have_same_clock(ENDPOINT *ep1, ENDPOINT *ep2, bool both_local)
{
    patch_fn_shared(stream_iso);
    return sco_have_same_clock(ep1, ep2, both_local);
}

/**
 * Create a new ISO endpoint
 */
ENDPOINT * iso_create_endpoint(unsigned key, ENDPOINT_DIRECTION dir)
{
    patch_fn_shared(stream_iso);
    return STREAM_NEW_ENDPOINT(iso, key, dir, INVALID_CON_ID);
}

/**
 * Get pointer to an existing ISO endpoint
 */
ENDPOINT * iso_get_endpoint(unsigned key, ENDPOINT_DIRECTION dir)
{
    patch_fn_shared(stream_iso);
    return stream_get_endpoint_from_key_and_functions(key, dir,
            &endpoint_iso_functions);
}

/****************************************************************************
Private Functions
*/

/**
 * Empty callback for operator message
 */
static bool set_from_air_iso_specific_info_cback(CONNECTION_LINK con_id,
                                                 STATUS_KYMERA status,
                                                 unsigned op_id,
                                                 unsigned num_resp_params,
                                                 unsigned *resp_params)
{
    patch_fn_shared(stream_iso);
    return TRUE;
}

/**
 * Send ISO-specific parameter (transport delay) to operator
 */
static void set_from_air_iso_specific_info(unsigned int sink_id,
                                  unsigned int transport_delay)
{
    /* Create a message and send it to the operator
     * We assume it's ISO_RCV, and the message IDs
     * and content is the same if more operators
     * can connect to this
     */
    unsigned params[2];
    patch_fn_shared(stream_iso);

    params[0] = OPMSG_ISO_RCV_ID_SET_TRANSPORT_DELAY;
    params[1] = transport_delay;

    opmgr_operator_message(OPMGR_ADAPTOR_OBPM, sink_id, sizeof(params)/sizeof(unsigned),
        params, set_from_air_iso_specific_info_cback);
}


#endif /* INSTALL_ISO_CHANNELS */

