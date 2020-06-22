/*****************************************************************************
*
* Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
*
*****************************************************************************/
/**
 * \file
 *
 * Scheduler header for subwd
 */

#ifndef SUBWD_SCHED_H
#define SUBWD_SCHED_H

#define SUBWD_SCHED_TASK(m)

#define SUBWD_BG_INT(m)                             \
    BG_INT(m, (subwd, subwd_bg_int_handler))

#endif /* SUBWD_SCHED_H */
