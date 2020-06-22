/*****************************************************************************
*
* Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
*
*****************************************************************************/
/**
 * \file
 */

#ifndef SUB_HOST_WAKE_SCHED_H_
#define SUB_HOST_WAKE_SCHED_H_

#define SUB_HOST_WAKE_SCHED_TASK(m)

#define SUB_HOST_WAKE_BG_INT(m)\
    BG_INT(m, (sub_host_wake, sub_host_wake_bg_int_handler))

#endif /* SUB_HOST_WAKE_SCHED_H_ */
