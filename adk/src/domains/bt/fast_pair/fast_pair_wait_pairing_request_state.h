/*******************************************************************************
Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    fast_pair_wait_pairing_request_state.h

DESCRIPTION
    Event handling functions for the Fast Pair Wait for Pairing Request State.
*/



#ifndef FAST_PAIR_WAIT_PAIRING_REQUEST_STATE_H_
#define FAST_PAIR_WAIT_PAIRING_REQUEST_STATE_H_

#include "fast_pair.h"
#include "fast_pair_events.h"

/******************************************************************************
NAME
    fastPair_StateWaitPairingRequestHandleEvent

DESCRIPTION
    Event handler for the Fast Pair Wait for Pairing Request State.

RETURNS
    Bool indicating if the event was successfully processed.
*/
bool fastPair_StateWaitPairingRequestHandleEvent(fast_pair_state_event_t event);

#endif
