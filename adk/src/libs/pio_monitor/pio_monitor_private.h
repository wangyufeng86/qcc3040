/*
Copyright (c) 2019  Qualcomm Technologies International, Ltd.
*/

#ifndef PIO_MONITOR_PRIVATE_H_
#define PIO_MONITOR_PRIVATE_H_

/* control debug generation */
#ifdef PIO_MONITOR_DEBUG_LIB
#include <stdio.h>
#define PIOM_DEBUG(x)  printf x
#else
#define PIOM_DEBUG(x)
#endif

/* Ideal sizes for best memory use are 1,5,9,13 */
#define MAX_PIOS_PER_TASK   9
#define EMPTY_PIO           0xFF

typedef struct PioMonitorClient
{
    struct PioMonitorClient    *next;
    Task                        task;
    uint16                      states;     /* Max 16 */
    uint8                       numPios;
    uint8                       pios[MAX_PIOS_PER_TASK];
} PioMonitorClient_t;

typedef struct {
    TaskData                    task;

    PioMonitorClient_t         *cl_head;
    PioMonitorClient_t         *cl_tail;

    uint16                      debounce_reads;
    uint16                      debounce_period;
} PioMonitorState_t;

#endif  /* PIO_MONITOR_PRIVATE_H_ */
