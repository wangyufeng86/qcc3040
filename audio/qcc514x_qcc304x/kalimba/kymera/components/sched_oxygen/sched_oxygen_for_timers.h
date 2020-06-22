/****************************************************************************
 * Copyright (c) 2017 Qualcomm Technologies International, Ltd.
 ****************************************************************************
 * \file sched_oxygen_for_timers.h
 *
 * This header file is for use by pl_timers.c only.
 */
#ifndef SCHED_OXYGEN_FOR_TIMERS
#define SCHED_OXYGEN_FOR_TIMERS

#include "hydra/hydra_types.h"

/**
 * Return the priority of the current task
 */
extern uint16f current_task_priority(void);

/**
 * Tell the scheduler the casual-wakeup timer expired 
 * and there is (probably) a timer to service. WARNING! This function expects
 * to be called from interrupt (i.e. with interrupts blocked).
 */
extern void sched_background_kick_event(void);


#endif /* SCHED_OXYGEN_FOR_TIMERS */