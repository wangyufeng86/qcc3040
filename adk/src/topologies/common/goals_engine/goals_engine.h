/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Module for managing running and contention between topology goals.

This module provides a generic goal engine that allows a topology to define its
own specific goals and the various parameters that control how and when they
will be run and which procedure they trigger.

Typically a topology shall define its goal ids in an enum and the properties
for each goal id will be defined in a goal table - one entry per goal id.
These are passed in to #GoalsEngine_CreateGoalSet to create an instance of a
goal_set_t.

Once a goal_set_t instance has been created the topology can activate a Goal
and the goals engine will run the Procedure associated with the Goal based on
whether other gGals are running, if the new Goal can run concurrently with
them, or if it needs to be marked as pending and held in an internal queue.


Goal Table

The entries in the goal table have various properties - some are required and
some are optional. This header defines a number of macros to simplify defining
the entries in the goal table based on how they are intended to be run. For an
example see the #GOAL or #GOAL_SUCCESS and related macros.


Goal Properties

An entry in the goal table can have the following properties:

* A unique goal_id
* A contention action that defines how the Goal is processed if it is activated
  while other Goal(s) are already active.
* An optional list of Goal(s) that can run concurrently with this Goal.
* An associated Procedure, or Scripted Procedure, that is started when the Goal
  is run.
* An optional Rule event that is generated when the Procedure started by the
  Goal completes.
** Different Rule events can be generated for success, fail or timeout.


\par Interaction Betwen Rules, Goals and Procedures.

A Goal is activated when a Rule makes a decision to run that Goal. The Rule
decision can optionally include a custom data payload which the goals engine
passes on to its associated Procedure when the Goal is run.

The topology defines its own Rules, Goals and Procedures but uses the common
rules, goals and scripting engines to handle the logic of how they are
processed.

In practice some of the knowledge of how the Rules, Goals and Procedures
interact is known only to the topology itself. For example, the contents of the
optional Rule decision payload is known only by the topology itself.

Because of this the topology still needs to implement some "glue" operations.
For example, to pass on Rule decisions to the Goal set the topology must
convert between the MessageId of the Rule decision and the goal_id it
represents.

Another example is when a Procedure completes and a Goal generates a new Rule
event. The topology must pass the new Rule event to the currently running Rule
set.

*/

#ifndef GOALS_ENGINE_H_
#define GOALS_ENGINE_H_

#include <rules_engine.h>

#include "procedures.h"
#include "script_engine.h"


/*! A goal id. Maximum value is limited by the number of bits in goal_mask. */
typedef uint8 goal_id;

/*! The value for 'no goal'. A client must not use this as the id of a real goal. */
#define GOAL_ID_NONE (0)

/*! The first usable goal id that a client can use for their own goals. */
#define GOAL_ID_MIN (1)

/*! The last usable goal id that a client can use for their own goals. */
#define GOAL_ID_MAX (63)

/*! A set of goals, stored as bits in a mask. */
typedef unsigned long long goal_mask;

/* Make sure the goal mask is at least 8 bytes in size. */
STATIC_ASSERT(sizeof(goal_mask) >= 8, GoalsEngine_maskSizeBadness);


/*! Mask to use when no goals are set. */
#define GOAL_MASK_NONE  (0x0ULL)


/*! Types of handling contention when adding a goal with already
    existing active goals. */
typedef enum
{
    /*! Queue the goal to be run when current active goals complete. */
    goal_contention_wait,

    /*! Queue the goal to be run once no goals are active and cancel
        any active goals. Will also cancel queued goals. */
    goal_contention_cancel,

    /*! Queue the current goal to be run once only goals with which it
        can conncurrently run are active. */
    goal_contention_concurrent,
} goal_contention_t;

/*! Definition of a goal and corresponding procedure to achieve the goal.

    \note Elements are ordered to slightly reduce the structure size
*/
typedef struct
{
    /*! Procedure to use to achieve the goal. */
    procedure_id proc;

    /*! How to handle contention with existing active goals
        when this goal is added to be run */
    goal_contention_t contention;

    /*! Goal which if active, can be summarily cleared, before starting
       this this goal's procedure. */
    goal_id exclusive_goal;

    /*! Event to set in the topology rules engine on successful
        completion of this goal. */
    rule_events_t success_event;

    /*! Event to set in the topology rules engine on failed
        completion of this goal due to timeout. */
    rule_events_t timeout_event;

    /*! Event to set in the topology rules engine on general failed
        completion of this goal. */
    rule_events_t failed_event;

    /*! Structure of function pointers implementing procedure interface. */
    const procedure_fns_t* proc_fns;

    /*! Scripted procedure handle. */
    const procedure_script_t* proc_script;

    /*! Bitmask of goals which this goal can run concurrently with. */
    const goal_id* concurrent_goals;
} goal_entry_t;

/*! \brief Opaque handle to a goal set instance.

    This object contains both the goals table and the current state
    of goal processing.
*/
typedef struct goal_set_tag *goal_set_t;


/*
    Macros to simplify adding goals to a goal table.
*/
/*! Macro to add goal to the goals table. */
#define GOAL(goal_name, proc_name, fns, exclusive_goal_name) \
   [goal_name] = { .proc = proc_name, .proc_fns = fns, \
                   .exclusive_goal = exclusive_goal_name, \
                   .contention = goal_contention_wait }

/*! Macro to add goal to the goals table, which can run concurrently with other goals. */
#define GOAL_WITH_CONCURRENCY(goal_name, proc_name, fns, exclusive_goal_name, concurrent_goals_mask) \
    [goal_name] =  { .proc = proc_name, .proc_fns = fns, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_concurrent, \
                     .concurrent_goals = concurrent_goals_mask }

/*! Macro to add goal to the goals table, which can run concurrently with other goals and generates event
    on successful completion. */
#define GOAL_WITH_CONCURRENCY_SUCCESS(goal_name, proc_name, fns, exclusive_goal_name, _success_event, concurrent_goals_mask) \
    [goal_name] =  { .proc = proc_name, .proc_fns = fns, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_concurrent, \
                     .concurrent_goals = concurrent_goals_mask, \
                     .success_event = _success_event }

/*! Macro to add goal to the goals table and define an event to generate
    on timeout or failure completion. */
#define GOAL_WITH_TIMEOUT_AND_FAIL(goal_name, proc_name, fns, exclusive_goal_name, _timeout_event, _failure_event) \
    [goal_name] =  { .proc = proc_name, .proc_fns = fns, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_wait, \
                     .timeout_event = _timeout_event, \
                     .failed_event = _failure_event }

/*! Macro to add goal to the goals table, which can run concurrently with other goals
    and which will generate an event on timeout failure completion. */
#define GOAL_WITH_CONCURRENCY_TIMEOUT(goal_name, proc_name, fns, exclusive_goal_name, _timeout_event, concurrent_goals_mask) \
    [goal_name] =  { .proc = proc_name, .proc_fns = fns, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_concurrent, \
                     .concurrent_goals = concurrent_goals_mask, \
                     .timeout_event = _timeout_event }

/*! Macro to add goal to the goals table and define an event to generate
    on successful completion. */
#define GOAL_SUCCESS(goal_name, proc_name, fns, exclusive_goal_name, _success_event) \
    [goal_name] =  { .proc = proc_name, .proc_fns = fns, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_wait, \
                     .success_event = _success_event }

/*! Macro to add goal to the goals table, which when set will cancel
    any actives goals. */
#define GOAL_CANCEL(goal_name, proc_name, fns, exclusive_goal_name) \
    [goal_name] =  { .proc = proc_name, .proc_fns = fns, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_cancel }

/*! Macro to add scripted goal to the goals table. */
#define SCRIPT_GOAL(goal_name, proc_name, script, exclusive_goal_name) \
    [goal_name] = { .proc = proc_name, .\
                     proc_script = script, \
                    .exclusive_goal = exclusive_goal_name, \
                    .contention = goal_contention_wait }

/*! Macro to add scripted goal to the goals table and define an event to
    generate on successful completion. */
#define SCRIPT_GOAL_SUCCESS(goal_name, proc_name, script, exclusive_goal_name, _success_event) \
    [goal_name] =  { .proc = proc_name, \
                     .proc_script = script, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_wait, \
                     .success_event = _success_event }

/*! Macro to add a scripted goal to the goals table.
    If there are any goals active when this goal is added, they will be cancelled and the
    new goal executed when the cancel has completed..
    The goal also defines an event to generate on successful completion. */
#define SCRIPT_GOAL_CANCEL_SUCCESS(goal_name, proc_name, script, exclusive_goal_name, _success_event) \
    [goal_name] =  { .proc = proc_name, \
                     .proc_script = script, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_cancel, \
                     .success_event = _success_event }

/*! Macro to add scripted goal to the goals table.
    If the timeout parameter is provided and the goal results in a timeout, the timeout
    event will be sent to the rules engine.
    If the failed parameter is provided and the goal results in a failure,a failed event
    will be sent to the rules engine. */
#define SCRIPT_GOAL_TIMEOUT_FAILED(goal_name, proc_name, script, exclusive_goal_name, _timeout_event, _failed_event) \
    [goal_name] =  { .proc = proc_name, \
                     .proc_script = script, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_wait, \
                     .timeout_event = _timeout_event, \
                     .failed_event = _failed_event }

/*! Macro to add scripted cancel goal to the goals table.
    If there are any goals active when this goal is added, they will be cancelled and the
    new goal executed when the cancel has completed..
    If the success parameter is provided and the goal results in a success, the success
    event will be sent to the rules engine.
    If the failed parameter is provided and the goal results in a failure, a failed event
    will be sent to the rules engine. */
#define SCRIPT_GOAL_CANCEL_SUCCESS_FAILED(goal_name, proc_name, script, exclusive_goal_name, _success_event, _failed_event) \
    [goal_name] =  { .proc = proc_name, \
                     .proc_script = script, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_cancel, \
                     .success_event = _success_event, \
                     .failed_event = _failed_event }

/*! Macro to add scripted goal to the goals table.
    If the success parameter is provided and the goal results in a success, the success
    event will be sent to the rules engine.
    If the timeout parameter is provided and the goal results in a timeout, the timeout
    event will be sent to the rules engine.
    If the failed parameter is provided and the goal results in a failure, a failed event
    will be sent to the rules engine. */
#define SCRIPT_GOAL_SUCCESS_TIMEOUT_FAILED(goal_name, proc_name, script, exclusive_goal_name, \
                                                  _success_event, _timeout_event, _failed_event) \
    [goal_name] =  { .proc = proc_name, \
                     .proc_script = script, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_wait, \
                     .success_event = _success_event, \
                     .timeout_event = _timeout_event, \
                     .failed_event = _failed_event }

/*! Macro to add scripted cancel goal to the goals table.
    If there are any goals active when this goal is added, they will be cancelled and the
    new goal executed when the cancel has completed..
    If the success parameter is provided and the goal results in a success, the success
    event will be sent to the rules engine.
    If the timeout parameter is provided and the goal results in a timeout, the timeout
    event will be sent to the rules engine.
    If the failed parameter is provided and the goal results in a failure, a failed event
    will be sent to the rules engine. */
#define SCRIPT_GOAL_CANCEL_SUCCESS_TIMEOUT_FAILED(goal_name, proc_name, script, exclusive_goal_name, \
                                                  _success_event, _timeout_event, _failed_event) \
    [goal_name] =  { .proc = proc_name, \
                     .proc_script = script, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_cancel, \
                     .success_event = _success_event, \
                     .timeout_event = _timeout_event, \
                     .failed_event = _failed_event }

/*! Macro to add scripted cancel goal to the goals table.
    If there are any goals active when this goal is added, they will be cancelled and the
    new goal executed when the cancel has completed..
    If the failed parameter is provided and the goal results in a failure, a failed event
    will be sent to the rules engine. */
#define SCRIPT_GOAL_CANCEL_FAILED(goal_name, proc_name, script, exclusive_goal_name, _failed_event) \
    [goal_name] =  { .proc = proc_name, \
                     .proc_script = script, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_cancel, \
                     .failed_event = _failed_event }

/*! Macro to add scripted goal to the goals table and define an event to
    generate on timeout failure completion. */
#define SCRIPT_GOAL_TIMEOUT(goal_name, proc_name, script, exclusive_goal_name, _timeout_event) \
    [goal_name] =  { .proc = proc_name, \
                     .proc_script = script, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_wait, \
                     .timeout_event = _timeout_event }

/*! Macro to add scripted goal to the goals table, which when set will cancel
    any active goals. */
#define SCRIPT_GOAL_CANCEL(goal_name, proc_name, script, exclusive_goal_name) \
    [goal_name] =  { .proc = proc_name, \
                     .proc_script = script, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_cancel }

/*! Macro to add scripted cancel goal to the goals table.
    If there are any goals active when this goal is added, they will be cancelled and the
    new goal executed when the cancel has completed..
    If the failed parameter is provided and the goal results in a failure, a failed event
    will be sent to the rules engine.
    If the timeout parameter is provided and the goal results in a timeout, the timeout
    event will be sent to the rules engine. */
#define SCRIPT_GOAL_CANCEL_TIMEOUT_FAILED(goal_name, proc_name, script, exclusive_goal_name, \
                                            _timeout_event, _failed_event) \
    [goal_name] =  { .proc = proc_name, \
                     .proc_script = script, \
                     .exclusive_goal = exclusive_goal_name, \
                     .contention = goal_contention_cancel, \
                     .timeout_event = _timeout_event, \
                     .failed_event = _failed_event }

/*! Macro for initialisation of a concurrent goals list.
    Converts a list of goals to a pointer for structure initialisation.

    \note Size cannot be determined at runtime, so adds a terminator
          avoiding an extra value for length. Length could be calculated
          at compile time, but wastes space where concurrency is not used
 */
#define CONCURRENT_GOALS_INIT(...) (const goal_id[]){__VA_ARGS__, GOAL_ID_NONE}


/*! Initialisation parameters for a new goal_set_t instance. */
typedef struct {
    /*! Pointer to a table of goals */
    const goal_entry_t      *goals;

    /*! Number of entries in the goal table */
    size_t                  goals_count;

    /*! Task used to queue goals waiting to be run. */
    Task                    pending_goal_queue_task;

    /*! Task to pass to a procedure when starting or cancelling it. */
    Task                    proc_result_task;

    /*! Procedure start/cancel/confirm functions to pass into procedures
        or procedure scipts. These are defined by the application to allow
        for customisation. */
    procedure_start_cfm_func_t    proc_start_cfm_fn;
    procedure_cancel_cfm_func_t   proc_cancel_cfm_fn;
    procedure_complete_func_t     proc_complete_cfm_fn;
} goal_set_init_params_t;


/*! \brief Create a goal set instance from a goals table.

    Create a goal set from a goals table and application specific procedure
    confirmation functions.

    This function will allocate the memory for the goal_set from the pmalloc
    pools.

    \param[in] init_params The static params to initialise the goal set with.

    \return Handle to a new goal set instance.
*/
goal_set_t GoalsEngine_CreateGoalSet(const goal_set_init_params_t *init_params);

/*! \brief Free any resources used by a goal set.

    Use this to free up the memory and any other resources used by a goal set.

    \param[in] goal_set The goal set instance to destroy.
*/
void GoalsEngine_DestroyGoalSet(goal_set_t goal_set);

/*! \brief Add a new active goal to a goal set.

    Deals with contention between new goal and any currently active goals.
    Existing active goals may require cancellation (depending on the requirements
    of the new goal), or may just be left to complete in the normal manner.

    Where cancellation or completion of existing active goals is required
    the new goal will be queued and re-delivered once conditions meet those
    required by the goal.

    \param[in] goal_set The goal set instance.
    \param[in] new_goal The goal to activate.
    \param[in] task The client Task activating the goal.
    \param[in] rule_id The id of the goal data message
    \param[in] goal_data Pointer to the goal data payload
    \param[in] goal_data_size Size of the goal data payload
*/
void GoalsEngine_ActivateGoal(goal_set_t goal_set, goal_id new_goal, Task task, MessageId rule_id,
                         Message goal_data, size_t goal_data_size);

/*! \brief Clear a goal from the list of current goals.

    \param[in] goal_set Goal set to clear the goal from.
    \param[in] goal Type of goal to clear.

    \note Clearing a goal may be due to successful completion
          or successful cancellation.
*/
void GoalsEngine_ClearGoal(goal_set_t goal_set, goal_id goal);

/*! \brief Determine if a goal is currently being executed.

    \param[in] goal_set Goal set to check against.
    \param[in] goal Goal to check.
*/
bool GoalsEngine_IsGoalActive(goal_set_t goal_set, goal_id goal);

/*! \brief Check if there are any pending goals in a goal set.

    \param[in] goal_set Goal set to check.

    \return TRUE if there are one or more goals queued; FALSE otherwise.
*/
bool GoalsEngine_IsAnyGoalPending(goal_set_t goal_set);

/*! \brief Find the goal id associated with the given procedure id.

    Note: If the procedure id does not match any goal in the goal set this
    function will panic.

    \param[in] goal_set Goal set to search through.
    \param[in] proc Procedure id to search for.

    \return the goal_id associated with the procedure.
*/
goal_id GoalsEngine_FindGoalForProcedure(goal_set_t goal_set, procedure_id proc);

/*! \brief Get the rule event generated when a goal procedure completes.

    \param[in] goal_set The goal set instance.
    \param[in] goal Goal to get the result event for.
    \param[in] result The result code returned by the completed procedure
*/
rule_events_t GoalsEngine_GetGoalCompleteEvent(goal_set_t goal_set, goal_id goal, procedure_result_t result);


/*
    Test functions
*/

void GoalsEngine_DebugLogGoalSet(goal_set_t goal_set);

#endif /* GOALS_ENGINE_H_ */
