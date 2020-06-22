/****************************************************************************
 * Copyright (c) 2008 - 2017 Qualcomm Technologies International, Ltd.
 ****************************************************************************
 * \file pl_timers_for_sched.h
 * \ingroup pl_timers
 *
 * header for the internals of the timer, which is shared between timer and
 * scheduler
 *
 * NOTES:
 * This file contains bits of the timer that only scheduler code should see
 *
 * The timer module has 2 internal lists for casual and strict timed events
 *
 * The casual timed events are stored in a list, which is accessed and
 * serviced from the scheduler loop. The list and access functions are
 * declared here.
 *
 */
#ifndef PL_TIMERS_FOR_SCHED_H
#define PL_TIMERS_FOR_SCHED_H

/****************************************************************************
Include Files
*/
#include "pl_timers/pl_timers.h"

/****************************************************************************
Public Type Declarations
*/

/**
 * Defines an abstract timer handle. This is only ever used as a pointer
 */
typedef struct tTimerStuctTag
{
    struct tTimerStuctTag *next; /**<Pointer to the next timer in a linked list */
    /** Timer ID is a unique combination of timer id count and various flags based on
     * event parameters. */
    tTimerId timer_id;
    void *data_pointer; /**< data pointer passed to timer expiry function */
    tTimerEventFunction TimedEventFunction; /**< Timer expiry handler function */
    union 
    {
        TIME event_time; /** Strict time deadline */
        struct
        {
            TIME earliest_time;
            TIME latest_time;
        }casual; /** Structure containing casual time data*/
    } variant;
} tTimerStruct;


typedef struct tEventsQueueTag
{
    tTimerStruct *first_event; /**< Head of event queue */

    /** The last time a timer on this message queue fired. This is provided so
     * that a user can request this if they want to schedule a periodic timer
     * during the timer handler. */
    TIME last_fired;

    /* Functions that access the queue */

    /* functions that act on queue elements */
    TIME (*get_expiry_time)(tTimerStruct *t); /**< Get the expiry time */
    TIME (*get_latest_time)(tTimerStruct *t); /**< Get the latest expiry time */
    /**
     * comparison function, which returns TRUE if event expires earlier than
     * given time
     */
    bool (*is_event_time_earlier_than)(tTimerStruct *t, TIME time);
} tEventsQueue;


/****************************************************************************
Global Variable Definitions
*/
/*
 * The timer event queues are shared between the scheduler and the timer
 * module.
 */
extern tEventsQueue casual_events_queue;
extern tEventsQueue strict_events_queue;

/****************************************************************************
Public Function Prototypes
*/
/**
 * \brief Timer service routine, which traverses through the casual event list
 * and services expired events.

 *
 * \note This function is internal to platform and is called by the scheduler.
 * This function expects to be called with interrupts blocked. This is for
 * efficiency as the scheduler always has interrupts blocked immediately prior
 * to calling. Interrupts are unblocked when an event handler is called. The
 * function exits with interrupts in the same state as when called.
 */
void timers_service_expired_casual_events(void);

/**
 * \brief Returns when the event list needs to be serviced next. For strict timers, this
 * returns the earliest absolute expiry and for casual timers, this returns the "earliest"
 * latest time.
 *
 * \param[in] event_queue pointer to the queue of timed events, which is to be checked
 * \param[out] next_time Holds the expiry time of the first event in the queue, if there
 * are any more timers in the queue
 *
 * \return Returns TRUE if there are any more timers in the list else FALSE
 *
 * \note This function is internal to platform and is called by scheduler and timer
 * interrupt service routine. An external version is declared below.
 */
bool timers_get_next_event_time_int(tEventsQueue* event_queue, TIME *next_time);

/**
 * \brief Finds the deadline of the next timer (strict or casual) that is set to expire.
 *
 * \param[out] next_time Holds the expiry time of the first event to expire, if there
 * are any.
 *
 * \return Returns TRUE if a valid timer was found, else FALSE
 */
extern bool timers_get_next_event_time(TIME *next_time);

/**
 * \brief Gets scheduled expiry time for the given timer id
 *
 * \param[in] timer_id event Id for which expiry time is required
 *
 * \return The scheduled expiry time for the event
 *
 * \note This function is used mainly from timer test applications. In the case of casual
 * events, the expiry time returned is the earliest time.
 */
extern TIME get_timer_expiry_time(tTimerId timer_id);
/**
 * \brief Gets pointer to the event the given timer id
 *
 * \param[in] timer_id event Id for which pointer is required
 *
 * \return Pointer to the timer structure
 *
 * \note This function is used mainly from timer test applications.
 */
extern tTimerStruct *get_timer_from_id(tTimerId timer_id);

/**
 * \brief The Timer2 hardware interrupt handler which causes the scheduler to check for
 * a casual timer to service.
 */
extern void casual_kick_event(void);

#ifdef UNIT_TEST_BUILD
/* Module test functions */
extern void test_set_time(TIME time);
extern void test_add_time(TIME_INTERVAL time_delta);
#endif /* UNIT_TEST_BUILD */

#endif /* PL_TIMERS_FOR_SCHED_H */
