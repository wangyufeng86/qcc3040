/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Engine for managing the issue and contention of high-level application goals.
*/

#include <stdlib.h>

#include <logging.h>
#include <panic.h>

#include "goals_engine.h"



/*! Macro to create a goal mask from a #goal_id. */
#define GOAL_MASK(id) (((goal_mask)1ULL)<<(id))

/*! Remove a goal from goal_mask */
#define REMOVE_GOAL_FROM_MASK(goal_mask, goal)      ((goal_mask) &= (~GOAL_MASK(goal)))

/*! The bit to set when a queued goal is locked on its lock_mask. */
#define GOAL_LOCK_MASK_BIT   (1 << 0)

/*! Set a queued goals' lock from its lock mask. */
#define SET_LOCK_FROM_MASK(lock, lock_mask)         ((lock) = (((lock_mask) != GOAL_MASK_NONE) ? GOAL_LOCK_MASK_BIT : 0))

/*! Macro to iterate over all goals in the goal table.
    Helper allows for the fact that the first entry is unused

    \param goal_set The goal set to iterate over
    \param loop_index name of variable to define and use as the loop index
*/
#define FOR_ALL_GOALS(goal_set, loop_index) \
    for (int loop_index = 1; (loop_index) < (goal_set)->goals_count; (loop_index)++)

/*! Macro to iterate over all queued goals.

    \param goal_set The goal set to iterate over
    \param loop_index name of variable to define and use as the loop index
 */
#define FOR_ALL_QUEUED_GOALS(goal_set, loop_index) \
    for (int loop_index = 0; (loop_index) < (goal_set)->pending_goals.pending_goals_alloc_count; (loop_index)++)

/*! Initial count for the number of elements in the goal queue array. */
#define INITIAL_GOAL_QUEUE_COUNT (8)

/*! Data for a queued goal. */
typedef struct {
    /*! goal_id of the queued goal. */
    goal_id     id;

    /*! Id of the decision message sent by the rule that activated this goal. */
    MessageId   rule_id;

    /*! Lock used to hold the queued goal decision in the queue Task */
    uint16      lock;

    /*! Bitmask of the goals that are preventing this goal from running. */
    goal_mask   lock_mask;
} pending_goal_entry_t;

/* Safety check to make sure the queue entries do not become too big */
STATIC_ASSERT(sizeof(pending_goal_entry_t) <= 16, GoalsEngine_queueEntrySizeBadness);


typedef struct
{
    /*! Task used to queue the rule decisions of goals already decided but waiting to be run. */
    Task                    queue_task;

    /*! Number of rule decisions queued in #queue_task. */
    uint8                   queue_msg_count;

    /*! High watermark of the number of rule decisions in the #queue_task. */
    uint8                   queue_msg_count_max;

    /*! Number of elements allocated for the pending goal array. */
    uint8                   pending_goals_alloc_count;

    /*! Array of data for each pending goal_id.

        Entries with a goal_id of GOAL_ID_NONE are not in use and can
        be used to store new pending goals.

        \note If a goal is queued twice or more it will use only one entry in
        this array - the array entry is re-used. This is because the rule
        decision messages are stored on #queue_task and this can be used
        instead to tell how many instances of a given goal_id were queued. */
    pending_goal_entry_t    *pending_goals;
} pending_goals_t;

/*! Structure containing the goals engine context */
struct goal_set_tag {
    /*! Pointer to a table of goals */
    const goal_entry_t      *goals;

    /*! Number of entries in the goal table */
    size_t                  goals_count;

    /*! Task to pass to a procedure when starting or cancelling it. */
    Task                    proc_result_task;

    /*! Procedure start/cancel/confirm functions to pass into procedures
        or procedure scipts. These are defined by the application to allow
        for customisation. */
    procedure_start_cfm_func_t    proc_start_cfm_fn;
    procedure_cancel_cfm_func_t   proc_cancel_cfm_fn;
    procedure_complete_func_t     proc_complete_cfm_fn;

    /*! Bitmask of currently active goals. */
    goal_mask               active_goals_mask;

    /*! Pending goals that are blocked from running immediately. */
    pending_goals_t         pending_goals;
};


static bool goalsEngine_GoalCancelsActive(goal_set_t goal_set, goal_id goal);
static void goalsEngine_ResetPendingQueueIfEmpty(goal_set_t goal_set);


/******************************************************************************
 * Debug utility functions
 *****************************************************************************/

/*! Output the contents of the goal table via debug logging. */
static void goalsEngine_DebugLogGoalTable(const goal_set_t gs)
{
    DEBUG_LOG("goalsEngine_DebugLogGoals goals %p count %d", gs->goals, gs->goals_count);

    FOR_ALL_GOALS(gs, i)
    {
        const goal_entry_t *goal = &gs->goals[i];
        DEBUG_LOG("  contention %d exclusive %d", goal->contention, goal->exclusive_goal);
    }
}

/*! Output the contents of the goal queue via debug logging. */
static void goalsEngine_DebugLogGoalQueue(const goal_set_t gs)
{
    DEBUG_LOG("goalsEngine_DebugLogGoalQueue");

    FOR_ALL_QUEUED_GOALS(gs, i)
    {
        pending_goal_entry_t *entry = &gs->pending_goals.pending_goals[i];

        DEBUG_LOG("  goal_id %u rule_id %u lock 0x%x lock_mask 0x%08lx%08lx",
                  entry->id, entry->rule_id, entry->lock, PRINT_ULL(entry->lock_mask));
    }
}

/*! Output the state of a goal_set_t via debug logging */
void GoalsEngine_DebugLogGoalSet(goal_set_t goal_set)
{
    DEBUG_LOG("GoalsEngine_DebugLogGoalSet active_mask 0x%08lx%08lx", PRINT_ULL(goal_set->active_goals_mask));

    goalsEngine_DebugLogGoalTable(goal_set);
    goalsEngine_DebugLogGoalQueue(goal_set);
}

/******************************************************************************
 * Utility functions
 *****************************************************************************/

/*! \brief Validate the initialisation params for a goal_set_t.

    \param init_params The init params to validate.

    \return TRUE if the params are ok, FALSE otherwise.
*/
static bool goalsEngine_ValidateInitParams(const goal_set_init_params_t *init_params)
{
    if (!init_params->goals)
    {
        DEBUG_LOG("goalsEngine_ValidateInitParams goals table is NULL");
        return FALSE;
    }

    if ((init_params->goals_count < GOAL_ID_MIN)
        || (init_params->goals_count > GOAL_ID_MAX))
    {
        DEBUG_LOG("goalsEngine_ValidateInitParams goals count is out of range");
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * Goal and goal mask utility functions
 *****************************************************************************/

/*! \brief Reverse lookup of procedure to goal from the goals table. */
goal_id GoalsEngine_FindGoalForProcedure(goal_set_t goal_set, procedure_id proc)
{
    FOR_ALL_GOALS(goal_set, goal_index)
    {
        if (goal_set->goals[goal_index].proc == proc)
        {
            return (goal_id)goal_index;
        }
    }

    DEBUG_LOG("GoalsEngine_FindGoalForProcedure failed to find goal for proc 0x%x", proc);
    Panic();

    return GOAL_ID_NONE;
}

/*! \brief From a bitmask of goals return the least significant set goal_id.

    For example from a mask of goals with bit pattern 0b1010 it will return 2.

    \param goal_set Goal set instance
    \param mask Bitmask of goals to search.

    \return The goal_id for the least significant bit set in the bitmask.
*/
static goal_id goalsEngine_FirstGoalFromMask(goal_set_t goal_set, goal_mask mask)
{
    /* Development panic, should not be used with zero goal_mask.
     * Effectively PanicZero(), but that macro doesn't work with unsigned long long types */
    if (!mask)
    {
        Panic();
    }

    goal_id goal = GOAL_ID_NONE;
    do {
        goal++;
        mask >>= 1;
    } while (!(mask & 0x1));

    PanicFalse(goal < goal_set->goals_count);

    return goal;
}

/******************************************************************************
 * Goal Queue Handling
 *****************************************************************************/

/*! \brief Determine if a goal is scripted. */
static bool goalsEngine_IsScriptedGoal(goal_set_t goal_set, goal_id goal)
{
    return (goal_set->goals[goal].proc_script != NULL);
}

/*! \brief Determine if a goal is a type that will cancel in-progress goals. */
static bool goalsEngine_GoalCancelsActive(goal_set_t goal_set, goal_id goal)
{
    return (goal_set->goals[goal].contention == goal_contention_cancel);
}

/*! \brief Determine if a goal is a type that will cancel in-progress goals. */
static bool goalsEngine_GoalRunsConcurrently(goal_set_t goal_set, goal_id goal)
{
    return (goal_set->goals[goal].contention == goal_contention_concurrent);
}

/*! \brief Put goal onto queue to be re-delivered once wait_mask goals are cleared.

    Active goals may become cleared either by normal completion or due to
    cancellation.
*/
static void goalsEngine_QueueGoal(goal_set_t goal_set, MessageId rule_id, Message goal_data, size_t goal_data_size,
                                  goal_mask wait_mask, goal_id new_goal)
{
    int queue_index = -1;
    pending_goal_entry_t *entry = NULL;

    /* First try to find the same goal already in the queue */
    FOR_ALL_QUEUED_GOALS(goal_set, i)
    {
        /* Another instance of this goal_id may already be queued for this
           goal_set. If so, re-use the queue slot to store the updated wait_mask.

           Note: This does not change the previously queued goal decision Id
           (and optional payload) that was sent to queue.queue_task.

           After this instance of the goal_id has been queued there will be 2
           (or more) messages queued for queue_task, but only one entry per goal_id
           in the goal_queue.queue. This is intentional. */
        if (goal_set->pending_goals.pending_goals[i].id == new_goal)
        {
            DEBUG_LOG("goalsEngine_QueueGoal re-using slot %d for goal %u", i, new_goal);
            queue_index = i;
            break;
        }
    }

    /* If no existing entry in the queue, find the first unused slot. */
    if (queue_index == -1)
    {
        FOR_ALL_QUEUED_GOALS(goal_set, i)
        {
            if (goal_set->pending_goals.pending_goals[i].id == GOAL_ID_NONE)
            {
                queue_index = i;
                break;
            }
        }
    }

    /* todo: re-allocating the queue array to increase the size. */
    if (queue_index == -1)
    {
        DEBUG_LOG("goalsEngine_QueueGoal Max goal_ids reached");
        /* If this happens consider increasing INITIAL_GOAL_QUEUE_COUNT, or
           implementing reallocing queue.queue to increading the maximum count. */
        Panic();
    }

    DEBUG_LOG("goalsEngine_QueueGoal goal %u queue_idx %d rule_id 0x%x wait_mask 0x%08lx%08lx",
              new_goal, queue_index, rule_id, PRINT_ULL(wait_mask));

    entry = &goal_set->pending_goals.pending_goals[queue_index];

    entry->lock_mask = wait_mask;
    SET_LOCK_FROM_MASK(entry->lock, entry->lock_mask);
    entry->rule_id = rule_id;
    entry->id = new_goal;

    if (goal_data)
    {
        void* message = PanicUnlessMalloc(goal_data_size);
        memcpy(message, goal_data, goal_data_size);
        MessageSendConditionally(goal_set->pending_goals.queue_task, rule_id, message,
                                 &entry->lock);
    }
    else
    {
        MessageSendConditionally(goal_set->pending_goals.queue_task, rule_id, NULL,
                               &entry->lock);
    }

    /* Keep track of the number of messages queued on pending_goal_queue_task */
    goal_set->pending_goals.queue_msg_count++;

    /* Remember the maximum number of msgs queued */
    if (goal_set->pending_goals.queue_msg_count > goal_set->pending_goals.queue_msg_count_max)
    {
        goal_set->pending_goals.queue_msg_count_max = goal_set->pending_goals.queue_msg_count;
    }
}

/*! \brief Remove goal message from queue and update queued goal count. */
static void goalsEngine_DequeGoal(goal_set_t goal_set, goal_id goal)
{
    int cancelled_count = 0;

    DEBUG_LOG("goalsEngine_DequeGoal goal %d", goal);

    FOR_ALL_QUEUED_GOALS(goal_set, i)
    {
        pending_goal_entry_t *entry = &goal_set->pending_goals.pending_goals[i];

        if (entry->id == goal)
        {
            /* Cancel all queued rule decisions for this goal */
            cancelled_count = MessageCancelAll(goal_set->pending_goals.queue_task, entry->rule_id);
            goal_set->pending_goals.queue_msg_count -= cancelled_count;

            DEBUG_LOG("goalsEngine_DequeGoal queue_index %d rule_id 0x%x cancelled_count %u msg queue size %u",
                      i, entry->rule_id, cancelled_count,
                      goal_set->pending_goals.queue_msg_count);

            /* Remove the goal from the queue by resetting the values of this element. */
            entry->id = 0;
            entry->rule_id = 0;
            entry->lock_mask = 0;
            entry->lock = 0;
            break;
        }
    }
}

/*! \brief Find the number of goals in the #pending_goal_queue_task. */
static int goalsEngine_GoalQueueSize(goal_set_t goal_set)
{
    return goal_set->pending_goals.queue_msg_count;
}

/******************************************************************************
 * Goal processing functions; add/cancel/clear/start
 *****************************************************************************/

/*! \brief Decide if a goal can be run now

    Goals can NOT be started if
    * a goal is alredy running
    * there are queued goals, and this is a new goal

    \return TRUE if a goal can be started immediately
  */
static bool goalsEngine_GoalCanRunNow(goal_set_t goal_set, bool is_new_goal)
{
    bool can_run = (   (goal_set->active_goals_mask == GOAL_MASK_NONE)
                    && (   (goalsEngine_GoalQueueSize(goal_set) == 0)
                    || !is_new_goal));

    DEBUG_LOG("goalsEngine_GoalCanRunNow can_run %d active_mask 0x%08lx%08lx queue_size %u new_goal %d",
              can_run, PRINT_ULL(goal_set->active_goals_mask), goalsEngine_GoalQueueSize(goal_set), is_new_goal);
    return can_run;
}

/*! \brief Determine if a goal is currently being executed. */
bool GoalsEngine_IsGoalActive(goal_set_t goal_set, goal_id goal)
{
    goal_mask mask = GOAL_MASK(goal);
    return ((goal_set->active_goals_mask & mask) == mask);
}

/*! \brief Check if there are any pending goals in a goal set. */
bool GoalsEngine_IsAnyGoalPending(goal_set_t goal_set)
{
    return (goal_set->pending_goals.queue_msg_count != 0);
}

/*! \brief Cancel a single goal. */
static void goalsEngine_CancelGoal(goal_set_t goal_set, goal_id cancel_goal)
{
    if (goalsEngine_IsScriptedGoal(goal_set, cancel_goal))
    {
        DEBUG_LOG("goalsEngine_CancelGoal cancelling scripted goal %d", cancel_goal);
        ScriptEngine_CancelScript(goal_set->proc_result_task,
                                           goal_set->proc_cancel_cfm_fn);
    }
    else
    {
        DEBUG_LOG("goalsEngine_CancelGoal cancelling singular goal %d", cancel_goal);
        goal_set->goals[cancel_goal].proc_fns->proc_cancel_fn(goal_set->proc_cancel_cfm_fn);
    }
}

/*! \brief Cancel any active or queued exclusive goals.

    \param goal_set The goal set instance.
    \param new_goal The new goal to be achieved and considered here for exclusive property.
    \param wait_mask[out] Mask of goals that must complete before running new goal.

    \return TRUE if the goal has exclusive goals, FALSE otherwise.
*/
static bool goalsEngine_HandleExclusiveGoals(goal_set_t goal_set,
                                             goal_id new_goal,
                                             goal_mask* wait_mask)
{
    goal_id exclusive_goal = goal_set->goals[new_goal].exclusive_goal;
    bool has_active_exclusive_goal = FALSE;

    /* does the goal have an exclusive goal specified and that exclusive goal
     * is currently active? */
    if (    (exclusive_goal != GOAL_ID_NONE)
         && (exclusive_goal < goal_set->goals_count)
         && (GoalsEngine_IsGoalActive(goal_set, exclusive_goal)))
    {
        DEBUG_LOG("goalsEngine_HandleExclusiveGoals exclusive goal %d", new_goal);

        /* clear any pending queued exclusive goals */
        goalsEngine_DequeGoal(goal_set, exclusive_goal);

        /* cancel active exclusive goals */
        goalsEngine_CancelGoal(goal_set, exclusive_goal);

        /* if exclusive goal synchronously cancelled remove it
         * from the wait mask */
        if (!GoalsEngine_IsGoalActive(goal_set, exclusive_goal))
        {
            REMOVE_GOAL_FROM_MASK(*wait_mask, exclusive_goal);
        }
        has_active_exclusive_goal = TRUE;
    }

    return has_active_exclusive_goal;
}

/*! \brief Utility handler to clear up the pending goals queue. */
static void goalsEngine_ClearPendingGoalsQueue(goal_set_t goal_set)
{
    int count = 0;

    count = MessageFlushTask(goal_set->pending_goals.queue_task);
    if (count)
    {
        DEBUG_LOG("goalsEngine_ClearPendingGoalsQueue cancelled %d goal from queue", count);
    }
    goal_set->pending_goals.queue_msg_count = 0;
    goalsEngine_ResetPendingQueueIfEmpty(goal_set);
}

/*! \brief If the new goal is a cancellation goal, cancel active and queue goals.

    First check if #new_goal is a cancellation goal. If it is then cancel all
    active and queued goals and update the wait mask. If not then do not
    modify the wait mask.

    Note: Cancelling a goal can be synchronous or asynchronous - it depends on
    how the goal procedure is implemented.

    \param[in] goal_set The goal set instance.
    \param[in] new_goal The new goal to acheive; if it is a cancellation goal process the cancellations.
    \param[in,out] wait_mask Mask of goals that must complete cancellation before running new goal.

    \return TRUE if the goal has caused cancellation, FALSE otherwise.
*/
static bool goalsEngine_CheckIfNewGoalCancelsActiveGoals(goal_set_t goal_set,
    goal_id new_goal, goal_mask *wait_mask)
{
    goal_mask working_cancel_mask = goal_set->active_goals_mask;
    bool rc = FALSE;

    if (goalsEngine_GoalCancelsActive(goal_set, new_goal))
    {
        /* cancel anything waiting on the pending queue */
        goalsEngine_ClearPendingGoalsQueue(goal_set);
        DEBUG_LOG("goalsEngine_CheckIfNewGoalCancelsActiveGoals perform cancellations for goal %d", new_goal);

        /* cancel anything active */
        while (working_cancel_mask)
        {
            goal_id goal = goalsEngine_FirstGoalFromMask(goal_set, working_cancel_mask);
            goalsEngine_CancelGoal(goal_set, goal);
            REMOVE_GOAL_FROM_MASK(working_cancel_mask, goal);
        }

        /* whatever hasn't synchronously completed cancellation must
         * be waited to complete before executing new cancel goal */
        *wait_mask = goal_set->active_goals_mask;

        rc = TRUE;
    }

    return rc;
}

/*! \brief Check if a goal would cancel an already running cancellation goal.

    \param[in] goal_set The goal set instance.
    \param[in] new_goal ID of the new goal being considered to start.

    \return bool TRUE start the new goal, FALSE do not start this goal

    \note This function will clear the pending goal queue if the new goal
    is a cancelling goal.

    We don't want to run it if we're already running the same goal,
    but the conditions that generated it are still such that would expect
    to be running the goal and clearing any pending goals in the queue.
*/
static bool goalsEngine_HandleDuplicateCancellationGoal(goal_set_t goal_set, goal_id new_goal)
{
    bool do_not_add_duplicate_goal = FALSE;

    if (goalsEngine_GoalCancelsActive(goal_set, new_goal) && GoalsEngine_IsGoalActive(goal_set, new_goal))
    {
        DEBUG_LOG("goalsEngine_HandleDuplicateCancellationGoal goal %d", new_goal);
        goalsEngine_ClearPendingGoalsQueue(goal_set);
        do_not_add_duplicate_goal = TRUE;
    }

    return do_not_add_duplicate_goal;
}

/*! \brief Get the concurrent goals for a goal as a bitmask. */
static goal_mask goalsEngine_GetConcurrentGoalsMask(const goal_entry_t* goal)
{
    const goal_id *concurrent_goals = goal->concurrent_goals;
    goal_mask goals_mask = 0;

    if (concurrent_goals)
    {
        while (*concurrent_goals != GOAL_ID_NONE)
        {
            goals_mask |= GOAL_MASK(*concurrent_goals);
            concurrent_goals++;
        }
    }
    return goals_mask;
}


/*! \brief Remove from the wait mask any goals with which the new
           goal *can* run concurrently.

    \param         goal_set The goal set instance.
    \param         new_goal  The goal being considered
    \param[in,out] wait_mask Pointer to the mask of events. May be changed if
                   the return value is TRUE

    \return TRUE if the goal can run concurrently
*/
static bool goalsEngine_HandleConcurrentGoals(goal_set_t goal_set,
                                              goal_id new_goal,
                                              goal_mask *wait_mask)
{
    const goal_entry_t *goal = &goal_set->goals[new_goal];
    bool supports_concurrency = goalsEngine_GoalRunsConcurrently(goal_set, new_goal);

    if (supports_concurrency)
    {
        goal_mask mask = goalsEngine_GetConcurrentGoalsMask(goal);

        *wait_mask &= ~mask;

        DEBUG_LOG("goalsEngine_HandleConcurrentGoals Ok goal:%d (0x%08lx%08lx) mask 0x%08lx%08lx",
                    new_goal, PRINT_ULL(GOAL_MASK(new_goal)), PRINT_ULL(mask));
    }
    else
    {
        DEBUG_LOG("goalsEngine_HandleConcurrentGoals Issue failed goal:%d (0x%08lx%08lx) contention:%d",
                    new_goal, PRINT_ULL(GOAL_MASK(new_goal)), goal->contention);
    }

    return supports_concurrency;
}

/*! Update wait masks for queued goals

    Adjust the conditions for all goals that may currently be queued.

    Queued goals are sent conditional on a mask in the pending_goal_lock_mask
    array.

    The mask is set based on the goals that are active at the time the goal is
    queued, adjusted for the settings of the goal being queued.

    \param goal_set The goal set instance.
*/
static void goalsEngine_UpdateQueueMasks(goal_set_t goal_set)
{
    if (goal_set->pending_goals.queue_msg_count)
    {
        int changes = 0;

        FOR_ALL_QUEUED_GOALS(goal_set, i)
        {
            pending_goal_entry_t *queue_entry = &goal_set->pending_goals.pending_goals[i];

            /* We don't track which items are in the queue (messages to a task
               list). We can tell if a particular goal has been queued since
               the queue was last empty by checking the associated message ID */
            if (queue_entry->rule_id)
            {
                goal_mask wait_mask = goal_set->active_goals_mask;
                goal_id goal = queue_entry->id;
                const goal_entry_t *goal_entry = &goal_set->goals[goal];
                goal_mask old_mask = queue_entry->lock_mask;

                DEBUG_LOG("goalsEngine_UpdateQueueMasks i %d, goal %u", i, goal);

                /* When goals were queued originally, the wait mask was based on
                   the active goals with
                   * any goals that should be cancelled, removed (and cancels issued)
                   * any allowed concurrent goals also removed

                   When we update the queue here, there is no need to remove cancel
                   goals as cancellation is either already in progress, or they have
                   been added subsequently+.

                   So just remove allowed concurrent goals.

                   + The goal having been added subsequently is possibly an issue
                   but cannot be solved here.
                   */
                if (goal_entry->contention == goal_contention_concurrent)
                {
                    wait_mask &= ~goalsEngine_GetConcurrentGoalsMask(goal_entry);
                }

                if (wait_mask != old_mask)
                {
                    if (!wait_mask)
                    {
                        /* We are particularly interested in the release of concurrent goals (March 2020) */
                        DEBUG_LOG_INFO("goalsEngine_UpdateQueueMasks. Goal:%d (0x%08lx%08lx) released concurrent. Mask changed from 0x%08lx%08lx",
                                  goal, PRINT_ULL(GOAL_MASK(goal)), PRINT_ULL(old_mask));
                    }
                    else
                    {
                        DEBUG_LOG("goalsEngine_UpdateQueueMasks. Goal:%d (0x%08lx%08lx) mask changed from 0x%08lx%08lx to 0x%08lx%08lx",
                                  goal, PRINT_ULL(GOAL_MASK(goal)), PRINT_ULL(old_mask), PRINT_ULL(wait_mask));
                    }

                    queue_entry->lock_mask = wait_mask;
                    SET_LOCK_FROM_MASK(queue_entry->lock, queue_entry->lock_mask);
                    changes++;
                }
            }
        }

        if (!changes)
        {
            DEBUG_LOG("goalsEngine_UpdateQueueMasks. No changes. %d items queued",
                        goal_set->pending_goals.queue_msg_count);
        }
    }
}

/*! Clear the message IDs associated with queued goals

    Clear all goal message IDs if the pending goals queue is empty.
    This reduces the work goalsEngine_UpdateQueueMasks() has to do.

    Although the ID for a specific goal is fixed, it needs to be
    calculated which is why the array is used. The ID is only set when
    the goal is queued.

    \param goal_set The goal set instance.
 */
static void goalsEngine_ResetPendingQueueIfEmpty(goal_set_t goal_set)
{
    DEBUG_LOG("goalsEngine_ResetPendingQueueIfEmpty queue size %u", goalsEngine_GoalQueueSize(goal_set));

    if (0 == goalsEngine_GoalQueueSize(goal_set))
    {
        memset(goal_set->pending_goals.pending_goals, 0, (sizeof(pending_goal_entry_t) * goal_set->pending_goals.pending_goals_alloc_count));
    }
}

/*! \brief Start the procedure for achieving a goal.

    Handles starting either of the two types of procedure definitions
    we have, either singlular or scripted.

    \param goal_set The goal set instance.
    \param goal Goal to start
    \param goal_data Pointer to the data to pass to the procedure when starting the goal.
*/
static void goalsEngine_StartGoal(goal_set_t goal_set, goal_id goal, Message goal_data)
{
    const goal_entry_t* goal_entry = &goal_set->goals[goal];

    DEBUG_LOG_INFO("goalsEngine_StartGoal goal %d", goal);

    goal_set->active_goals_mask |= GOAL_MASK(goal);

    if (goalsEngine_IsScriptedGoal(goal_set, goal))
    {
        /* start scripted goal */
        ScriptEngine_StartScript(goal_set->proc_result_task,
                goal_entry->proc_script,
                goal_entry->proc,
                goal_set->proc_start_cfm_fn,
                goal_set->proc_complete_cfm_fn);
    }
    else
    {
        /* start single goal */
        goal_entry->proc_fns->proc_start_fn(goal_set->proc_result_task,
                goal_set->proc_start_cfm_fn,
                goal_set->proc_complete_cfm_fn,
                goal_data);
    }

    goalsEngine_UpdateQueueMasks(goal_set);
}

/******************************************************************************
 * Public functions
 *****************************************************************************/

/*! \brief Create a goal set instance from a goals table. */
goal_set_t GoalsEngine_CreateGoalSet(const goal_set_init_params_t *init_params)
{
    goal_set_t gs = NULL;

    PanicFalse(goalsEngine_ValidateInitParams(init_params));

    gs = PanicUnlessMalloc(sizeof(*gs));
    memset(gs, 0, sizeof(*gs));

    gs->goals = init_params->goals;
    gs->goals_count = init_params->goals_count;

    /* Initialise the memory for the goal queue */
    gs->pending_goals.queue_task = init_params->pending_goal_queue_task;
    gs->pending_goals.pending_goals_alloc_count = INITIAL_GOAL_QUEUE_COUNT;
    gs->pending_goals.pending_goals = PanicUnlessMalloc(sizeof(pending_goal_entry_t) * gs->pending_goals.pending_goals_alloc_count);
    memset(gs->pending_goals.pending_goals, 0, (sizeof(pending_goal_entry_t) * gs->pending_goals.pending_goals_alloc_count));

    gs->proc_result_task = init_params->proc_result_task;
    gs->proc_cancel_cfm_fn = init_params->proc_cancel_cfm_fn;
    gs->proc_complete_cfm_fn = init_params->proc_complete_cfm_fn;
    gs->proc_start_cfm_fn = init_params->proc_start_cfm_fn;

    DEBUG_LOG("GoalsEngine_CreateGoalSet goal_set_t size %u goal queue size %u", sizeof(*gs), sizeof(pending_goal_entry_t) * gs->pending_goals.pending_goals_alloc_count);

    return gs;
}

/*! \brief Free any resources used by a goal set. */
void GoalsEngine_DestroyGoalSet(goal_set_t goal_set)
{
    free(goal_set->pending_goals.pending_goals);
    free(goal_set);
}

/*! \brief Add a new active goal to a goal set. */
void GoalsEngine_ActivateGoal(goal_set_t goal_set, goal_id new_goal, Task task, MessageId rule_id,
                         Message goal_data, size_t goal_data_size)
{
    bool is_new_goal = (task != goal_set->pending_goals.queue_task);

    if (GOAL_ID_NONE == new_goal)
    {
        DEBUG_LOG("GoalsEngine_AddGoal. WARNING. Attempted to add a goal of GOAL_ID_NONE.");
        return;
    }

    /* decrement goal queue if this was delivered from it rather
     * than externally from the rules engine */
    if (!is_new_goal)
    {
        goal_set->pending_goals.queue_msg_count--;
    }

    DEBUG_LOG("GoalsEngine_AddGoal goal %d is_new_goal %d existing goals 0x%08lx%08lx", new_goal, is_new_goal, PRINT_ULL(goal_set->active_goals_mask));
    DEBUG_LOG("GoalsEngine_AddGoal pending_queue_size %u", goal_set->pending_goals.queue_msg_count);

    /* check if we're already running a cancelling goal and have been asked to
     * run it again, ignore if so */
    if (goalsEngine_HandleDuplicateCancellationGoal(goal_set, new_goal))
    {
        DEBUG_LOG("GoalsEngine_AddGoal cancelling goal %d already running, ignore", new_goal);
        return;
    }

    if (goalsEngine_GoalCanRunNow(goal_set, is_new_goal))
    {
        DEBUG_LOG("GoalsEngine_AddGoal start goal %d immediately", new_goal);
        /* no goals active, just start this new one */
        goalsEngine_StartGoal(goal_set, new_goal, goal_data);
    }
    else
    {
        /* default behaviour if goals cannot run yet is to be queued
         * pending completion of active goals. Setup a wait mask with
         * active goals, it may be modified by consideration of cancellation,
         * exclusive or concurrent goals later. */
        goal_mask wait_mask = goal_set->active_goals_mask;
        DEBUG_LOG("GoalsEngine_AddGoal must queue, wait_mask 0x%08lx%08lx", PRINT_ULL(wait_mask));

        /* first see if this goal will cancel everything */
        if (goalsEngine_CheckIfNewGoalCancelsActiveGoals(goal_set, new_goal, &wait_mask))
        {
            DEBUG_LOG("GoalsEngine_AddGoal %d is a cancel goal", new_goal);
            /* start immediately if possible, else queue and stop processing */
            if (!wait_mask && goalsEngine_GoalCanRunNow(goal_set, is_new_goal))
            {
                goalsEngine_StartGoal(goal_set, new_goal, goal_data);
            }
            else
            {
                goalsEngine_QueueGoal(goal_set, rule_id, goal_data, goal_data_size, wait_mask, new_goal);
            }
        }
        else
        {
            /* remove any goals from the wait_mask with which the new goal
             * can run concurrently, i.e. this goal will not need to wait
             * for them to complete in order to start. */
            bool concurrent = goalsEngine_HandleConcurrentGoals(goal_set, new_goal, &wait_mask);

            DEBUG_LOG("GoalsEngine_AddGoal after conncurrent handling, wait_mask 0x%08lx%08lx", PRINT_ULL(wait_mask));

            /* handle cancellation of any exclusive goals specified by
             * the new goal */
            goalsEngine_HandleExclusiveGoals(goal_set, new_goal, &wait_mask);

            DEBUG_LOG("GoalsEngine_AddGoal after exclusive handling, wait_mask 0x%08lx%08lx", PRINT_ULL(wait_mask));

            /* if goal supports concurrency and the wait_mask is clear then the
             * goal can be started now
             * OR
             * anything that needed cancelling has completed synchronously
             * then we may be able to start now
             *
             * otherwise queue the goal on
             * completion of the remaining goals in the wait_mask
             */
            if (   (concurrent && !wait_mask)
                || goalsEngine_GoalCanRunNow(goal_set, is_new_goal))
            {
                goalsEngine_StartGoal(goal_set, new_goal, goal_data);
            }
            else
            {
                goalsEngine_QueueGoal(goal_set, rule_id, goal_data, goal_data_size, wait_mask, new_goal);
            }
        }
    }

    goalsEngine_ResetPendingQueueIfEmpty(goal_set);
}

/*! \brief Clear a goal from the list of current goals.

    \param[in] goal Type of goal to clear from the list.

    \note Clearing a goal may be due to successful completion
    or successful cancellation.
*/
void GoalsEngine_ClearGoal(goal_set_t goal_set, goal_id goal)
{
    REMOVE_GOAL_FROM_MASK(goal_set->active_goals_mask, goal);

    DEBUG_LOG_INFO("GoalsEngine_ClearGoal goal %d. Mask now 0x%08lx%08lx", goal, PRINT_ULL(goal_set->active_goals_mask));

    /*! clear this goal from all pending goal locks,
        may result in a queued goal being delivered to start */
    FOR_ALL_QUEUED_GOALS(goal_set, i)
    {
        pending_goal_entry_t *entry = &goal_set->pending_goals.pending_goals[i];

        if (entry->id != GOAL_ID_NONE)
        {
            /* If this queue slot is in-use, update its lock_mask. */
            REMOVE_GOAL_FROM_MASK(entry->lock_mask, goal);
            SET_LOCK_FROM_MASK(entry->lock, entry->lock_mask);
        }
        else if (entry->lock_mask != GOAL_MASK_NONE)
        {
            /* If this queue slot is not in use it should have an empty lock_mask. */
            DEBUG_LOG("GoalsEngine_ClearGoal WARNING queue index %d lock_mask 0x%08lx%08lx", i, PRINT_ULL(entry->lock_mask));
        }
    }
}

/*! \brief Get the rule event generated when a goal procedure completes. */
rule_events_t GoalsEngine_GetGoalCompleteEvent(goal_set_t goal_set, goal_id goal, procedure_result_t result)
{
    const goal_entry_t* goal_entry = &goal_set->goals[goal];
    rule_events_t complete_event = 0;

    switch (result)
    {
    case procedure_result_success:
        complete_event = goal_entry->success_event;
        break;

    case procedure_result_timeout:
        complete_event = goal_entry->timeout_event;
        break;

    case procedure_result_failed:
        complete_event = goal_entry->failed_event;
        break;

    default:
        DEBUG_LOG("GoalsEngine_GetGoalCompleteEvent unhandled procedure result %d", result);
        break;
    }

    return complete_event;
}
