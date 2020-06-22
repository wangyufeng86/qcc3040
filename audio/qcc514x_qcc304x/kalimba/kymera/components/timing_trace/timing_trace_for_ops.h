/****************************************************************************
 * Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file timing_trace_for_ops.h
 * \ingroup timing_trace
 *
 * Public event recorder API for capabilities
 */
#ifndef TIMING_TRACE_FOR_OPS_H
#define TIMING_TRACE_FOR_OPS_H

#include "timing_trace_types_for_ops.h"


/******************************
 * Public Function Declarations / Inline Definitions
 */

/**
 * \brief Check before each call to timing trace recorder functions
 *        like timing_trace_record_op_event.
 *
 * \return TRUE if the timing trace recorder functions can be
 *         called and the trace data transport is active.
 */
extern bool timing_trace_is_enabled(void);




#endif /* TIMING_TRACE_FOR_OPS_H */
