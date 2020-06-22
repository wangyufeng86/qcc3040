/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       task_list.c
\brief      Implementation of a simple list of VM tasks.
*/

#include "task_list.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hydra_macros.h>

#include <panic.h>

/*! Accessor for total number of tasks in the list */
#define taskList_Size(list) ((list)->base.size_list)

/*! Set the list size */
#define taskList_SizeSet(list, size) ((list)->base.size_list = (size))

/*! Accessor for number of tasks in the flexible array */
#define taskList_FlexibleSize(list) ((list)->base.size_flexible_tasks)

/*! Set the list flexible size */
#define taskList_FlexibleSizeSet(list, size) ((list)->base.size_flexible_tasks = (size))

/*! Accessor for the list type */
#define taskList_Type(list) ((list)->base.list_type)

/*! Set the list type */
#define taskList_TypeSet(list, type) ((list)->base.list_type = (type))

/*! Accessor for the 'no destroy' flag */
#define taskList_NoDestroy(list) ((list)->base.no_destroy)

/*! Set the list no destroy value */
#define taskList_NoDestroySet(list, value) ((list)->base.no_destroy = (value))

/*! Sizeof a flexible task list */
#define taskList_FlexibleSizeof(flexible_tasks) (sizeof(task_list_flexible_t) + ((flexible_tasks) * sizeof(Task)))


/******************************************************************************
 * Internal functions
 ******************************************************************************/

/*! \brief Find the task at a index (considering flexible array)

    \param[in]  list        Pointer to a Tasklist.
    \param[in] index        Index to find task.

    \return Task The task at the index.
 */
static Task taskList_GetTaskAtIndex(task_list_t *list, uint16 index)
{
    task_list_flexible_t *flex = STRUCT_FROM_MEMBER(task_list_flexible_t, base, list);
    uint16 flex_size = taskList_FlexibleSize(list);
    uint16 list_size = taskList_Size(list);
    Task task;

    /* Tasks are placed in this order:
        1. In the flexible array (if it exists)
        2. In the union's task
        3. In the union's dynamically allocated array (2) is moved to first element in this array.
    */

    if (index < flex_size)
    {
        task = flex->flexible_tasks[index];
    }
    else if ((index == flex_size) && (list_size == (index + 1)))
    {
        task = list->u.task;
    }
    else
    {
        task = list->u.tasks[index - flex_size];
    }
    return task;
}

/*! \brief Find the index in the task list array for a given task.

    \param[in]  list        Pointer to a Tasklist.
    \param[in]  search_task Task to search for on list.
    \param[out] index       Index at which search_task is found.

    \return bool TRUE search_task found and index returned.
                 FALSE search_task not found, index not valid.
 */
static bool taskList_FindTaskIndex(task_list_t *list, Task search_task, uint16* index)
{
    bool task_index_found = FALSE;
    uint16 iter = 0;

    for (iter = 0; iter < taskList_Size(list); iter++)
    {
        if (taskList_GetTaskAtIndex(list, iter) == search_task)
        {
            *index = iter;
            task_index_found = TRUE;
        }
    }

    return task_index_found;
}

/*! \brief Set the task at a given index.

    \param[in]  list        Pointer to a Tasklist.
    \param[in]  task        Task to set.
    \param[in] index        Index of task to set.
 */
static void taskList_SetTaskAtIndex(task_list_t *list, Task task, uint16 index)
{
    task_list_flexible_t *flex = STRUCT_FROM_MEMBER(task_list_flexible_t, base, list);
    uint16 flex_size = taskList_FlexibleSize(list);
    uint16 list_size = taskList_Size(list);

    if (index < flex_size)
    {
        flex->flexible_tasks[index] = task;
    }
    else if ((index == flex_size) && (index == (list_size - 1)))
    {
        list->u.task = task;
    }
    else
    {
        list->u.tasks[index - flex_size] = task;
    }
}

/*! \brief Convert task_list_t to task_list_with_data_t.
    \param list Pointer to the list to convert.
    \return The task_list_with_data_t.
*/
static inline task_list_with_data_t *taskList_ToListWithData(task_list_t *list)
{
    return STRUCT_FROM_MEMBER(task_list_with_data_t, base, list);
}

/*! \brief Get pointer to task_list_data_t array from a task list.
    \param list Pointer to the list (must be task_list_with_data_t).
    \return Pointer to the array of task_list_data_t.
*/
static inline task_list_data_t *taskList_GetListData(task_list_t *list)
{
    return taskList_ToListWithData(list)->data;
}

/*! \brief Resize a list (optionally with data)
    \param list Pointer to the list to be resized.
    \param The new size of the list.

    This function does not check current size of the list - always resizing to
    the new_size requested.

    This function handles the list having a flexible array of tasks in addition
    to a dynamically allocated list of tasks.
*/
static void taskList_Resize(task_list_t *list, uint16 new_size)
{
    /* Number of statically allocated tasks in the list (flexible array and task in union) */
    uint16 flex_size = taskList_FlexibleSize(list);
    uint16 static_size = flex_size + 1;
    uint16 old_size = taskList_Size(list);

    if (new_size == static_size)
    {
        if (old_size > static_size)
        {
            Task move_task = list->u.tasks[0];
            free(list->u.tasks);
            list->u.tasks = NULL;
            list->u.task = move_task;
        }
        else if (old_size < static_size)
        {
            list->u.task = NULL;
        }
    }
    else if (new_size > static_size)
    {
        size_t alloc_size = sizeof(Task) * (new_size - flex_size);

        if (old_size == static_size)
        {
            Task move_task = list->u.task;
            list->u.tasks = PanicNull(malloc(alloc_size));
            list->u.tasks[0] = move_task;
        }
        else
        {
           list->u.tasks = PanicNull(realloc(list->u.tasks, alloc_size));
        }
    }
    else
    {
        if (old_size > static_size)
        {
            free(list->u.tasks);
            list->u.tasks = NULL;
        }
        else if (old_size == static_size)
        {
            list->u.task = NULL;
        }
    }

    if (taskList_Type(list) == TASKLIST_TYPE_WITH_DATA)
    {
        task_list_with_data_t *list_with_data = taskList_ToListWithData(list);
        if (new_size)
        {
            task_list_data_t *list_data = list_with_data->data;
            list_data = realloc(list_data, sizeof(*list_data) * new_size);
            list_with_data->data = PanicNull(list_data);
        }
        else
        {
            if (list_with_data->data)
            {
                free(list_with_data->data);
                list_with_data->data = NULL;
            }
        }
    }

    taskList_SizeSet(list, new_size);
}

/*! \brief Helper function that iterates through a list but also returns the index.

    \param[in]      list        Pointer to a Tasklist.
    \param[in,out]  next_task   IN Task from which to continue iterating though list.
                                OUT Next task in the list after next_task passed in.
    \param[out]     index       Index in the list at which OUT next_task was returned.

    \return bool TRUE iteration successful, another next_task and index provided.
                 FALSE empty list or end of list reached. next_task and index not
                       valid.
 */
static bool iterateIndex(task_list_t* list, Task* next_task, uint16* index)
{
    bool iteration_successful = FALSE;
    uint16 tmp_index = 0;

    PanicNull(list);
    PanicNull(next_task);
    PanicNull(index);

    /* list not empty */
    if (taskList_Size(list))
    {
        /* next_task == NULL to start at tmp_index 0 */
        if (*next_task == 0)
        {
            *next_task = taskList_GetTaskAtIndex(list, tmp_index);
            *index = tmp_index;
            iteration_successful =  TRUE;
        }
        else
        {
            /* move to next task */
            if (taskList_FindTaskIndex(list, *next_task, &tmp_index))
            {
                tmp_index += 1;
                if (tmp_index < taskList_Size(list))
                {
                    *next_task = taskList_GetTaskAtIndex(list, tmp_index);
                    *index = tmp_index;
                    iteration_successful =  TRUE;
                }
            }
            else
            {
                /* end of the list */
                *next_task = 0;
            }
        }
    }

    return iteration_successful;
}

/*! \brief Helper function that creates a null terminated array of Tasks.

    \param[in]      list        Pointer to a Tasklist.

    \return Task * the created array of tasks

    If the Task List is empty this function will not allocate any memory, the caller should check
    the value returned is not null before attempting to use it.

    The Task array is in the form required for the Multicast MessageSend traps.
 */
static Task * taskList_CreateNullTerminatedTaskArray(task_list_t *list)
{
    Task *task_list = NULL;
    uint16 number_of_tasks = taskList_Size(list);

    if (number_of_tasks != 0)
    {
        Task next_task = NULL;
        uint16 index;

        /* Account for null terminator */
        number_of_tasks++;
        task_list = (Task *) PanicUnlessMalloc(number_of_tasks*sizeof(Task));

        while (iterateIndex(list, &next_task, &index))
        {
            task_list[index] = next_task;
        }
        task_list[++index] = NULL;
    }

    return task_list;
}

/******************************************************************************
 * External API functions
 ******************************************************************************/
/*! \brief Create a task_list_t.
 */
task_list_t* TaskList_Create(void)
{
    return TaskList_CreateWithCapacity(1);
}

task_list_t* TaskList_CreateWithCapacity(unsigned capacity)
{
    task_list_flexible_t* new_list;

    if (capacity == 0)
    {
        capacity = 1;
    }

    /* All tasks list can store one task, so flexible array needs to be -1 */
    new_list = malloc(taskList_FlexibleSizeof(capacity-1));

    TaskList_InitialiseWithCapacity(new_list, capacity);
    taskList_NoDestroySet(&new_list->base, FALSE);

    return &new_list->base;
}

void TaskList_Initialise(task_list_t* list)
{
    task_list_flexible_t *flex_list = STRUCT_FROM_MEMBER(task_list_flexible_t, base, list);
    TaskList_InitialiseWithCapacity(flex_list, 1);
}

void TaskList_InitialiseWithCapacity(task_list_flexible_t* flex_list, unsigned capacity)
{
    task_list_t *base;

    PanicNull(flex_list);
    PanicFalse(capacity <= TASK_LIST_MAX_TASKS);

    memset(flex_list, 0, taskList_FlexibleSizeof(capacity-1));

    base = &flex_list->base;
    taskList_TypeSet(base, TASKLIST_TYPE_STANDARD);
    taskList_NoDestroySet(base, TRUE);
    taskList_FlexibleSizeSet(base, capacity-1);
}

/*! \brief Create a task_list_t that can also store associated data.
 */
task_list_t* TaskList_WithDataCreate(void)
{
    task_list_with_data_t *new_list = malloc(sizeof(*new_list));

    TaskList_WithDataInitialise(new_list);
    taskList_NoDestroySet(&new_list->base, FALSE);

    return &new_list->base;
}

void TaskList_WithDataInitialise(task_list_with_data_t* list)
{
    PanicNull(list);
    memset(list, 0, sizeof(*list));
    taskList_TypeSet(&list->base, TASKLIST_TYPE_WITH_DATA);
    taskList_NoDestroySet(&list->base, TRUE);
}

/*! \brief Destroy a task_list_t.
 */
void TaskList_Destroy(task_list_t* list)
{
    PanicNull(list);
    if (taskList_NoDestroy(list))
    {
        Panic();
    }

    taskList_Resize(list, 0);

    if (taskList_Type(list) == TASKLIST_TYPE_WITH_DATA)
    {
        task_list_with_data_t *list_with_data = taskList_ToListWithData(list);
        free(list_with_data);
        list_with_data = NULL;
    }
    else
    {
        free(list);
        list = NULL;
    }
}

/*! \brief Add a task to a list.
 */
bool TaskList_AddTask(task_list_t* list, Task add_task)
{
    bool task_added = FALSE;

    PanicNull(list);
    PanicNull(add_task);

    /* if not in the list */
    if (!TaskList_IsTaskOnList(list, add_task))
    {
        taskList_Resize(list, taskList_Size(list) + 1);
        taskList_SetTaskAtIndex(list, add_task, taskList_Size(list) - 1);

        task_added = TRUE;
    }

    return task_added;
}

/*! \brief Add a task and data to a list.
 */
bool TaskList_AddTaskWithData(task_list_t* list, Task add_task, const task_list_data_t* data)
{
    bool task_with_data_added = FALSE;

    if (TaskList_IsTaskListWithData(list) && TaskList_AddTask(list, add_task))
    {
        task_list_data_t *list_data = taskList_GetListData(list);

        list_data[taskList_Size(list)-1] = *data;
        task_with_data_added = TRUE;
    }

    return task_with_data_added;
}

/*! \brief Determine if a task is on a list.
 */
bool TaskList_IsTaskOnList(task_list_t* list, Task search_task)
{
    uint16 tmp;

    PanicNull(list);

    return taskList_FindTaskIndex(list, search_task, &tmp);
}

/*! \brief Remove a task from a list.
 */
bool TaskList_RemoveTask(task_list_t* list, Task del_task)
{
    bool task_removed = FALSE;
    uint16 index = 0;

    PanicNull(list);
    PanicNull(del_task);

    if (taskList_FindTaskIndex(list, del_task, &index))
    {
        uint16 iter;
        /* Move tasks into space created by removed task */
        for (iter = index ; iter < taskList_Size(list) - 1; iter++)
        {
            Task next = taskList_GetTaskAtIndex(list, iter + 1);
            taskList_SetTaskAtIndex(list, next, iter);
        }

        if (taskList_Type(list) == TASKLIST_TYPE_WITH_DATA)
        {
            task_list_data_t *list_data = taskList_GetListData(list) + index;
            size_t tomove = sizeof(*list_data) * (taskList_Size(list) - index - 1);
            memmove(list_data, list_data + 1, tomove);
        }

        taskList_Resize(list, taskList_Size(list) - 1);

        task_removed = TRUE;
    }

    return task_removed;
}

void TaskList_RemoveAllTasks(task_list_t* list)
{
    PanicNull(list);
    
    /* tasklist cannot remove all tasks for lists with data as the data would need
     * to be removed as well, and is owned by the tasklist owner. */
    if (taskList_Type(list) == TASKLIST_TYPE_WITH_DATA)
    {
        Panic();
    }

    taskList_Resize(list, 0);
}

/*! \brief Return number of tasks in list.
 */
uint16 TaskList_Size(task_list_t* list)
{
    return list ? taskList_Size(list) : 0;
}

/*! \brief Iterate through all tasks in a list.
 */
bool TaskList_Iterate(task_list_t* list, Task* next_task)
{
    uint16 tmp_index = 0;
    return iterateIndex(list, next_task, &tmp_index);
}

/*! \brief Iterate through all tasks in a list returning a copy of data as well.
 */
bool TaskList_IterateWithData(task_list_t* list, Task* next_task, task_list_data_t* data)
{
    bool iteration_successful = FALSE;
    task_list_data_t *raw_data = NULL;

    if (TaskList_IterateWithDataRaw(list, next_task, &raw_data))
    {
        *data = *raw_data;
        iteration_successful = TRUE;
    }

    return iteration_successful;
}

/*! \brief Iterate through all tasks in a list returning raw data stored in the
    task_list_t.
 */
bool TaskList_IterateWithDataRaw(task_list_t* list, Task* next_task, task_list_data_t** raw_data)
{
    bool iteration_successful = FALSE;
    uint16 tmp_index = 0;

    if (TaskList_IsTaskListWithData(list) && iterateIndex(list, next_task, &tmp_index))
    {
        *raw_data = taskList_GetListData(list) + tmp_index;
        iteration_successful = TRUE;
    }

    return iteration_successful;
}

/*! \brief Iterate through all tasks in a list calling handler each iteration. */
bool TaskList_IterateWithDataRawFunction(task_list_t *list, TaskListIterateWithDataRawHandler handler, void *arg)
{
    Task next_task = 0;
    uint16 index = 0;
    bool proceed = TRUE;

    PanicFalse(TaskList_IsTaskListWithData(list));

    while (proceed && iterateIndex(list, &next_task, &index))
    {
        task_list_data_t *data = taskList_GetListData(list) + index;
        proceed = handler(next_task, data, arg);
    }

    return proceed;
}

/*! \brief Create a duplicate task list.
 */
task_list_t *TaskList_Duplicate(task_list_t* list)
{
    task_list_t *new_list = NULL;

    PanicNull(list);

    if (taskList_Type(list) == TASKLIST_TYPE_STANDARD)
    {
        new_list = TaskList_CreateWithCapacity(taskList_FlexibleSize(list));
    }
    else
    {
        new_list = TaskList_WithDataCreate();
    }

    if (new_list)
    {
        unsigned index;

        new_list->base = list->base;
        taskList_NoDestroySet(new_list, FALSE);
        taskList_Resize(new_list, taskList_Size(list));

        for (index = 0; index < taskList_Size(list); index++)
        {
            Task copytask = taskList_GetTaskAtIndex(list, index);
            taskList_SetTaskAtIndex(new_list, copytask, index);
        }

        if (taskList_Type(new_list) == TASKLIST_TYPE_WITH_DATA)
        {
            task_list_data_t *old_data = taskList_ToListWithData(list)->data;
            task_list_data_t *new_data = taskList_ToListWithData(new_list)->data;
            memcpy(new_data, old_data, sizeof(task_list_data_t) * taskList_Size(list));
        }
    }

    return new_list;
}

/*! \brief Send a message (with message body) to all tasks in the task list.
*/
void TaskList_MessageSendWithSize(task_list_t *list, MessageId id, void *data, size_t size_data)
{
    TaskList_MessageSendLaterWithSize(list, id, data, size_data, D_IMMEDIATE);
}

/*! \brief Send a message (with message body and delay) to all tasks in the task list.
*/
void TaskList_MessageSendLaterWithSize(task_list_t *list, MessageId id, void *data, size_t size_data, uint32 delay)
{
    PanicNull(list);

    Task *task_list = taskList_CreateNullTerminatedTaskArray(list);
    if (task_list)
    {
        if (size_data == 0)
        {
            PanicNotNull(data);
        }

        MessageSendMulticastLater(task_list, id, data, delay);

        free(task_list);
    }
    else
    {
        free(data);
    }
}

/*! \brief Get a copy of the data stored in the list for a given task.
*/
bool TaskList_GetDataForTask(task_list_t* list, Task search_task, task_list_data_t* data)
{
    bool data_copied = FALSE;
    task_list_data_t *raw_data = NULL;

    if (TaskList_GetDataForTaskRaw(list, search_task, &raw_data))
    {
        *data = *raw_data;
        data_copied = TRUE;
    }

    return data_copied;
}

/*! \brief Get the address of the data stored in the list for a given task.
*/
bool TaskList_GetDataForTaskRaw(task_list_t* list, Task search_task, task_list_data_t** raw_data)
{
    bool data_copied = FALSE;
    uint16 tmp;

    if (TaskList_IsTaskListWithData(list) && taskList_FindTaskIndex(list, search_task, &tmp))
    {
        *raw_data = taskList_GetListData(list) + tmp;
        data_copied = TRUE;
    }

    return data_copied;
}

/*! \brief Set the data stored in the list for a given task.
*/
bool TaskList_SetDataForTask(task_list_t* list, Task search_task, const task_list_data_t* data)
{
    bool data_set = FALSE;
    uint16 tmp;

    if (TaskList_IsTaskListWithData(list) && taskList_FindTaskIndex(list, search_task, &tmp))
    {
        *(taskList_GetListData(list) + tmp) = *data;
        data_set = TRUE;
    }

    return data_set;
}

/*! \brief Determine if the list is one that supports data.
*/
bool TaskList_IsTaskListWithData(task_list_t* list)
{
    PanicNull(list);
    return taskList_Type(list) == TASKLIST_TYPE_WITH_DATA;
}

void TaskList_Init(void)
{

}
