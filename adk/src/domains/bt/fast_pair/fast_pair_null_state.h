/*******************************************************************************
Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    fast_pair_null_state.h

DESCRIPTION
    Event handling functions for the Fast Pair Null State.
*/

#ifndef FAST_PAIR_NULL_STATE_H_
#define FAST_PAIR_NULL_STATE_H_

#include "fast_pair.h"
#include "fast_pair_events.h"


/******************************************************************************
NAME
    fastPair_StateNullHandleEvent

DESCRIPTION
    Event handler for the Fast Pair Null/Qurantine State.

RETURNS
    Bool indicating if the event was successfully processed.
*/
bool fastPair_StateNullHandleEvent(fast_pair_state_event_t event);

#endif
