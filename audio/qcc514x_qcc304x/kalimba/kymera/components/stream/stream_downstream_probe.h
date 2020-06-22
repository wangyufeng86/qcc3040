/****************************************************************************
 * Copyright (c) 2020 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file stream_downstream_probe.h
 * \ingroup stream
 *
 * This file contains public stream functions that are used by operators
 * and capabilities.
 */

#ifndef STREAM_DOWNSTREAM_PROBE_H
#define STREAM_DOWNSTREAM_PROBE_H

#include "types.h"
#include "stream/stream_common.h"
#include "buffer/cbuffer_c.h"

/*****************************************
 * Type definitions
 */

typedef struct stream_short_downstream_probe_result
{
    /** True if the buffer ends at a constant rate real endpoint */
    bool    constant_rate_buffer_consumer   :  8;

    /** True if the connection is via an in-place chain */
    bool    is_in_place                     :  8;

    /** Pointer to cbuffer connected to the source terminal.
     */
    tCbuffer* first_cbuffer;

    /** Pointer to cbuffer at tail end of in-place chain, if any.
     */
    tCbuffer* tail_cbuffer;

    /* Additional information to add could include sample rate
     * and block size of the consumer, if it turns out to be
     * useful, and easily obtained.
     */

} STREAM_SHORT_DOWNSTREAM_PROBE_RESULT;


/*****************************************
 * Public function declarations
 */

/**
 * \brief Query whether a source endpoint is connected to
 *        a constant rate sink (audio sink or AEC Ref real sink terminal)
 * \param source_ep_id External endpoint ID of the source endpoint,
 *                     usually an operator terminal
 * \param result Pointer to result structure. On return, the result
 *               is valid if the return value is TRUE.
 * \return TRUE if query could be processed, FALSE if there
 *         was an internal error.
 * \note A simple latency estimate is possible if this function
 *       returns TRUE and the constant_rate_buffer_consumer field
 *       of the result is TRUE.
 *       If an answer cannot be obtained synchronously, this
 *       function will return FALSE.
 */
extern bool stream_short_downstream_probe(
        ENDPOINT_ID source_ep_id,
        STREAM_SHORT_DOWNSTREAM_PROBE_RESULT* result);

/**
 * \brief Get the amount buffered downstream. For an in-place
 * chain this includes the buffers downstream from the passed
 * first buffer.
 * Number of samples buffered. The types of endpoints
 * for which constant_rate_buffer_consumer is TRUE
 * only support PCM data, so compressed data amounts
 * in octets don't need to be considered.
 */
extern unsigned stream_calc_downstream_amount_words(
        tCbuffer* first_cbuffer,
        tCbuffer* tail_cbuffer);

#endif /* STREAM_DOWNSTREAM_PROBE_H */
