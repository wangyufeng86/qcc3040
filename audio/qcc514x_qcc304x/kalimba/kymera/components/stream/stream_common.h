/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_common.h
 * \ingroup stream
 *
 * Stream public header file. <br>
 * This file contains public stream types and functions that are used also by operators and capabilities. <br>
 *
 */

#ifndef STREAM_COMMON_H
#define STREAM_COMMON_H

#include "stream_prim.h"

/**
 * The amount to shift a value to represent 1.0 in the fixed point representation
 * expected by the ratematching manager. Note: 22 even on KAL_ARCH4
 */
#define STREAM_RATEMATCHING_FIX_POINT_SHIFT 22

typedef struct ENDPOINT ENDPOINT;
typedef struct BUFFER_DETAILS BUFFER_DETAILS;

/**
 * The numeric representation of a Kymera object as exposed outside of the
 * audio subsystem. Only the 16 lower bits are used.
 */
typedef unsigned KYMERA_ID;

/**
 * The numeric representation of a endpoint as exposed outside of the audio
 * subsystem. Only the 16 lower bits are used. It can be cast to KYMERA_ID.
 */
typedef unsigned ENDPOINT_ID;

/**
 * The numeric representation of a transform as exposed outside of the audio
 * subsystem. Only the 16 lower bits are used. It can be cast to KYMERA_ID.
 */
typedef unsigned TRANSFORM_ID;

/**
 * Enumeration of endpoint directions
 */
typedef enum
{
    SOURCE = 0, /*!< Endpoint is a source. */
    SINK = 1    /*!< Endpoint is a sink. */
} ENDPOINT_DIRECTION;

/**
* Kick propagation direction
*/
typedef enum
{
   STREAM_KICK_INTERNAL,  /*!< Internally-originated kick. */
   STREAM_KICK_FORWARDS,  /*!< Forwards kick i.e. source-to-sink (AKA "more data"). */
   STREAM_KICK_BACKWARDS, /*!< Backwards kick i.e. sink-to-source (AKA "more space"). */
   STREAM_KICK_INVALID    /*!< Indicates an invalid kick direction value. */
} ENDPOINT_KICK_DIRECTION;

/**
 * Enumeration of the type of ratematching an endpoint can implement
 */
typedef enum
{
    RATEMATCHING_SUPPORT_NONE,   /*!< Can't do ratematching. */
    RATEMATCHING_SUPPORT_SW,     /*!< Can implement software ratematching. */
    RATEMATCHING_SUPPORT_HW,     /*!< Can implement hardware ratematching. */
    RATEMATCHING_SUPPORT_AUTO,   /*!< Naturally runs at the rate data is pulled/pushed, no SW/HW effort required. */
    RATEMATCHING_SUPPORT_MONITOR /*!< Endpoint needs its master rate. */
} RATEMATCHING_SUPPORT;

/**
 * \brief Get the endpoint that is connected to this endpoint.
 *
 * \param ep Pointer to an endpoint
 *
 * \return Pointer to the endpoint that is connected to ep.
 *         Null if not connected
 */
extern ENDPOINT *stream_get_endpoint_connected_to(ENDPOINT *ep);

/**
 * \brief Retrieves the device type of an audio endpoint.
 *
 * \param ep The endpoint to get the hardware type of
 *
 * \return The device type of the endpoint
 */
extern STREAM_DEVICE stream_get_device_type(ENDPOINT *ep);

#endif /* STREAM_COMMON_H */
