/*
Copyright (c) 2019  Qualcomm Technologies International, Ltd.
*/

#ifndef PIO_MONITOR_H_
#define PIO_MONITOR_H_

#include <message.h>
#include "library.h"

#define PIOM_NUM_PIOS  (96)
#define PIOM_NUM_BANKS (PIOM_NUM_PIOS / 32)


typedef enum
{
    PIO_MONITOR_ENABLE_CFM = PIO_MONITOR_MESSAGE_BASE,
} PioMonitorMessage_t;

/*! @brief Initialise the PIO monitor manager */
bool PioMonitorInit(Task task);

/*! @brief Get the task for the PIO monitor */
Task PioMonitorGetTask(void);

/*! @brief Enable the PIO monitor.
 *  The PIO monitor will register with the OS to receive MESSAGE_PIO_CHANGED messages,
 *  and will send a PIO_MONITOR_ENABLE_CFM message to all registered tasks.
 */
void PioMonitorEnable(void);

/*! @brief Set the debounce parameters */
void PioMonitorSetDebounceParameters(uint16 debounce_reads, uint16 debounce_period);

/*! @brief Register a task to receive event notifications on the specified pio.
           When the specified pio changes state, the input event manager will
           send the client task a #MESSAGE_PIO_CHANGED.

    It is the caller's responsibility to configure the pio (e.g. as input,
    setting pull-up/pull-down resistors). PIO monitor will maintain the complete
    list of pios requiring debouncing by the firmware, and will call
    PioDebounce32Bank().
*/
void PioMonitorRegisterTask(Task client, uint8 pio);

/*! @brief Unregister a task. */
void PioMonitorUnregisterTask(Task client, uint8 pio);

#endif  /* PIO_MONITOR_H_ */
