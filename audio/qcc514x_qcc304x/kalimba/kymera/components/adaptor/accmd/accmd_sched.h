/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file accmd_sched.h
 * \ingroup accmd
 * Task definitions for accmd
 */
#ifndef ACCMD_SCHED_H
#define ACCMD_SCHED_H

#define ACCMD_SCHED_TASK(m)                                     \
    SCHED_TASK_START(m, (accmd, accmd_task_init,                \
                         accmd_task, RUNLEVEL_BASIC))          \
    SCHED_TASK_QUEUE(m, (accmd, ACCMD_QUEUE))                   \
    SCHED_TASK_END(m, (accmd))

#define ACCMD_BG_INT(m)

#endif /* ACCMD_SCHED_H */
