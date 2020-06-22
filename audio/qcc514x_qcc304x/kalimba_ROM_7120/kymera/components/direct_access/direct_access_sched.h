/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file sqif_direct_access_sched.h
 * \ingroup direct_access
 *
 * Definition of oxygen tasks for External Flash direct access.
 */


#ifndef DIRECT_ACCESS_SCHED_H

    #define DIRECT_ACCESS_SCHED_H

    #define DIRECT_ACCESS_SCHED_TASK(m)                                      \
        SCHED_TASK_START(m, (direct_access, direct_access_task_init,         \
                             direct_access_handler, RUNLEVEL_BASIC))         \
        SCHED_TASK_QUEUE(m, (direct_access, DIRECT_ACCESS_TASK_QUEUE_ID))    \
        SCHED_TASK_END(m, (direct_access))
    #define DIRECT_ACCESS_BG_INT(m)

#endif /* DIRECT_ACCESS_SCHED_H */

