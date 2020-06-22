/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file obpm_sched.h
 * \ingroup obpm
 * Task definitions for obpm
 */
#ifndef OBPM_SCHED_H
#define OBPM_SCHED_H

#define OBPM_SCHED_TASK(m)                                            \
    SCHED_TASK_START(m, (obpm, obpm_adaptor_init,                     \
                         obpm_adaptor_msg_handler, RUNLEVEL_BASIC))   \
    SCHED_TASK_QUEUE(m, (obpm, OBPM_MSG_ADAPTOR_TASK_QUEUE_ID))       \
    SCHED_TASK_END(m, (obpm))

#define OBPM_BG_INT(m)

#endif /* OBPM_SCHED_H */
