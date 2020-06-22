/****************************************************************************
 * Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup timing_trace Timing trace
 * \file timing_trace_types_for_ops.h
 * \ingroup timing_trace
 *
 * Public event recorder API for capabilities
 */
#ifndef TIMING_TRACE_TYPES_FOR_OPS_H
#define TIMING_TRACE_TYPES_FOR_OPS_H

#include "types.h"

/******************************
 * Constant Declarations
 */
/** TIMING_TRACE_EVENT_OP_EVENT */
typedef enum {
    /** Values for Timed Playback op/endpoint events */
    TIMING_TRACE_TTP_PLAYBACK_CONTROL = 0x01,
    TIMING_TRACE_TTP_DISCARD_SAMPLES = 0x02,
    TIMING_TRACE_TTP_INSERT_SILENCE_SAMPLES = 0x03,
    TIMING_TRACE_TTP_LATENCY_BELOW_MINIMUM = 0x04,
    TIMING_TRACE_TTP_LATENCY_ABOVE_MAXIMUM = 0x05,

    /** System events with op_id == 0 */
    TIMING_TRACE_SCHED_SLEEP_ENTER = 0x30,
    TIMING_TRACE_SCHED_SLEEP_EXIT = 0x31,

    /** Beginning of range for Qualcomm capabilities */
    TIMING_TRACE_OP_EVENT_OP_BASE = 0x40,

    /** Beginning of range for non-Qualcomm capabilities */
    TIMING_TRACE_OP_EVENT_EXT_BASE = 0x80,

    /** Indicate that this enum should fit in 8 bits */
    TIMING_TRACE_OP_EVENT_LAST = 0xff

} TIMING_TRACE_OP_EVENT_TYPE;

#endif /* TIMING_TRACE_TYPES_FOR_OPS_H */
