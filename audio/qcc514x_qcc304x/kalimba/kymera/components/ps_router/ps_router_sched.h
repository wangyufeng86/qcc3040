/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file ps_router_sched.h
 * \ingroup ps
 *
 * Definition of oxygen tasks for ps_router.
 */
#ifndef PS_ROUTER_SCHED_H
#define PS_ROUTER_SCHED_H

#define PS_ROUTER_SCHED_TASK(m)                                 \
    SCHED_TASK_START(m, (ps_router, ps_router_task_init,        \
                         ps_router_handler, RUNLEVEL_BASIC))    \
    SCHED_TASK_QUEUE(m, (ps_router, PS_ROUTER_TASK_QUEUE_ID))   \
    SCHED_TASK_END(m, (ps_router))
#define PS_ROUTER_BG_INT(m)
#endif /* PS_ROUTER_SCHED_H */