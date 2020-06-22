/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup HAL Hardware Abstraction Layer
 * \file  hal_time.h
 *
 * Public header file for HAL get time function.
 *
 * \ingroup HAL
 *
 */
#ifndef _HAL_TIME_H_
#define _HAL_TIME_H_

#include "types.h"

/**
 * \brief  Get current system time
 */
extern TIME hal_get_time(void);

/**
 * \brief Tight loop to wait for a given number of microseconds for hal operations
 *
 * \param delay Number of microseconds to delay
 */
extern void hal_delay_us(unsigned delay);

#ifdef DESKTOP_TEST_BUILD
extern void (*hal_delay_us_test_hook)(unsigned delay);
#endif

#endif /* _HAL_TIME_H_ */
