/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  source_sync_test.c
 * \ingroup  capabilities
 *
 *  src_sync operator
 *
 * This file packages and exports some internal functions to a test harness.
 * In particular the inline functions for time unit conversions would not
 * be easily accessible in an unit test build otherwise.
 * There are no references to these functions in regular code, so the linker
 * should discard them.
 */
/****************************************************************************
Include Files
*/

#include "source_sync_defs.h"
#include "source_sync_test.h"

unsigned src_sync_test_usec_to_sec_frac(unsigned usec)
{
    return src_sync_usec_to_sec_frac(usec);
}

unsigned src_sync_test_samples_to_usec(unsigned num_samples, unsigned inv_sample_rate)
{
    return src_sync_samples_to_usec(num_samples, inv_sample_rate);
}

unsigned src_sync_test_samples_to_time(unsigned n, unsigned ts)
{
    return src_sync_samples_to_time(n, ts);
}

/**
 * This is an implementation for unit tests.
 */
#ifndef __KCC__
unsigned src_sync_sample_rate_to_inv_sample_rate(unsigned sample_rate)
{
    double t = (1<<SRC_SYNC_INV_RATE_RESOLUTION)*1.0 / sample_rate;
    /* convert it to Q1.31 */
    return (unsigned)(t * (1u<<(DAWTH-1)) + 0.5);
}
#endif
