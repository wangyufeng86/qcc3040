/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Queue utility required for HDMA Core Algorithm.
*/

#ifdef INCLUDE_HDMA

#include <stdlib.h>
#include "hdma_queue.h"
#ifndef DEBUG_HDMA_UT
#include <panic.h>
#endif
#include "hdma_utils.h"

static void hdma_ShiftQueueBaseTimestamp(queue_t* queue);

void Hdma_QueueDestroy(queue_t *q)
{
    if (q)
    {
        free(q);
    }
}

void Hdma_QueueCreate(queue_t *new_queue)
{
    if (new_queue)
    {
        new_queue->size = 0;
        new_queue->capacity = BUFFER_LEN;
        new_queue->rear = INDEX_NOT_DEFINED;
        new_queue->front = 0;
        new_queue->base_time = 0;
    }
}

uint8 Hdma_IsQueueFull(queue_t* queue)
{
    return (queue->size == queue->capacity);
}

uint8 Hdma_IsQueueEmpty(queue_t *queue)
{
    return (queue->size == 0);
}

void hdma_ShiftQueueBaseTimestamp(queue_t* queue)
{
    int index, i = 0;
	uint16 timestamp;

	timestamp = queue->quality[queue->front].timestamp;
    queue->base_time += queue->quality[queue->front].timestamp;
    for(index = queue->rear; i < queue->size; i++, index =((index - 1)%(queue->capacity)))
    {
        queue->quality[index].timestamp -= timestamp;
        if(index == 0)
        {
            index = queue->capacity;
        }
    }
}
void Hdma_QueueInsert(queue_t* queue,uint8 val, uint32 timestamp)
{
    if(Hdma_IsQueueEmpty(queue))
    {
        queue->rear = 0;
        queue->base_time = timestamp;
    }
    else
    {
        queue->rear = (queue->rear + 1)%queue->capacity;
    }
    queue->quality[queue->rear].data = val;
    queue->quality[queue->rear].timestamp = timestamp - queue->base_time;

    if (!Hdma_IsQueueFull(queue))
    {
        queue->size = queue->size + 1;
    }
    else
    {
        queue->front = (queue->front + 1)%queue->capacity; // If queue is full, override the front element
    }
    if(timestamp > (queue->base_time + FORTYEIGHTKB))
    {
        hdma_ShiftQueueBaseTimestamp(queue);
    }
}

uint8 Hdma_QueueDelete(queue_t *queue, uint32 *timestamp)
{
    uint8 val =0;
    if (Hdma_IsQueueEmpty(queue))
    {
        HDMA_DEBUG_LOG("Queue Underflow \n");
        *timestamp = 0;
        return val;
    }
    else
    {
        val = queue->quality[queue->front].data;
        *timestamp = queue->quality[queue->front].timestamp + queue->base_time;
        queue->front = (queue->front + 1)%(queue->capacity);
        (queue->size)--;
    }
    return val;
}

uint8 Hdma_GetQueueFront(queue_t* queue, uint32 *timestamp)
{
    if (Hdma_IsQueueEmpty(queue))
    {
        return INDEX_NOT_DEFINED;
    }
    *timestamp = queue->quality[queue->front].timestamp + queue->base_time;
    return queue->quality[queue->front].data;
}

uint8 Hdma_GetQueueRear(queue_t* queue, uint32 *timestamp)
{
    if (Hdma_IsQueueEmpty(queue))
    {
        return INDEX_NOT_DEFINED;
    }
    *timestamp = queue->quality[queue->rear].timestamp + queue->base_time;
    return queue->quality[queue->rear].data;
}
#endif /* INCLUDE_HDMA */
