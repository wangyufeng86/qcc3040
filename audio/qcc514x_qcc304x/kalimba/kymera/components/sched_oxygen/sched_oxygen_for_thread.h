/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 ****************************************************************************
 * \file sched_oxygen_for_thread.h
 *
 * This header file is for use by the thread offload component only.
 *
 * When the secondary processor is running in thread offload mode,
 * the scheduler is either running the last scheduled RPC or it is idle.
 */

#ifndef SCHED_OXYGEN_FOR_THREAD_H
#define SCHED_OXYGEN_FOR_THREAD_H

/* \brief Prevent from scheduler from sleeping.
 *
 * \note This function must not be used from the primary processor.
 */
extern void sched_disallow_sleep(void);

/* \brief Allow the scheduler to sleep.
 *
 * \note This function must not be used from the primary processor.
 */
extern void sched_allow_sleep(void);

#endif /* SCHED_OXYGEN_FOR_THREAD_H */
