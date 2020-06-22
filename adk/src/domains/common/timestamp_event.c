/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Timestamp events.
*/

#include "timestamp_event.h"

#include "vm.h"
#include "panic.h"

#ifndef DISABLE_TIMESTAMP_EVENT

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#define ASSERT_ID_IS_VALID(id) PanicFalse((id) < NUMBER_OF_TIMESTAMP_EVENTS)

/* Store the timestamp when the event occurred */
static uint16 timestamp_events[NUMBER_OF_TIMESTAMP_EVENTS];

void TimestampEvent(timestamp_event_id_t id)
{
    ASSERT_ID_IS_VALID(id);

    timestamp_events[id] = (uint16)VmGetClock();
}

static uint16 timestampEvent_Get(timestamp_event_id_t id)
{
    ASSERT_ID_IS_VALID(id);

    return timestamp_events[id];
}

uint32 TimestampEvent_Delta(timestamp_event_id_t id1, timestamp_event_id_t id2)
{
    uint16 delta = (uint16)(timestampEvent_Get(id2) - timestampEvent_Get(id1));

    return (uint32)delta;
}

uint16 TimestampEvent_GetTime(timestamp_event_id_t id)
{
    return timestampEvent_Get(id);
}

#endif /* DISABLE_TIMESTAMP_EVENT */
