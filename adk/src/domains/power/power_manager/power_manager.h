/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       power_manager.h
\brief	    Header file for the Power Management.
*/

#ifndef POWER_MANAGER_H_
#define POWER_MANAGER_H_

#include <message.h>
#include <task_list.h>
#include "domain_message.h"

/*
@startuml
    POWER_STATE_INIT : Initialising power, shutdown if battery is too low
    POWER_STATE_OK : No reason to sleep / shutdown
    POWER_STATE_TERMINATING_CLIENTS_NOTIFIED : POWER_SHUTDOWN_PREPARE_IND sent to clients on entry
    POWER_STATE_TERMINATING_CLIENTS_RESPONDED : Powering off
    POWER_STATE_SOPORIFIC_CLIENTS_NOTIFIED : POWER_SLEEP_PREPARE_IND sent to clients on entry
    POWER_STATE_SOPORIFIC_CLIENTS_RESPONDED : Going to sleep

    POWER_STATE_INIT #Pink --> POWER_STATE_OK : Battery ok or charging
    POWER_STATE_OK #LightGreen --> POWER_STATE_TERMINATING_CLIENTS_NOTIFIED : Power event (e.g. low battery)
    POWER_STATE_TERMINATING_CLIENTS_NOTIFIED --> POWER_STATE_TERMINATING_CLIENTS_RESPONDED : All clients responded appPowerShutdownPrepareResponse()
    POWER_STATE_TERMINATING_CLIENTS_RESPONDED --> POWER_STATE_OK : Power event whilst terminating (e.g. charging)

    POWER_STATE_OK --> POWER_STATE_SOPORIFIC_CLIENTS_NOTIFIED : All clients allowing sleep
    POWER_STATE_SOPORIFIC_CLIENTS_NOTIFIED --> POWER_STATE_SOPORIFIC_CLIENTS_RESPONDED : All clients responded appPowerSleepPrepareResponse()
    POWER_STATE_SOPORIFIC_CLIENTS_RESPONDED --> POWER_STATE_OK : Power event whilst soporific (e.g. charging)

@enduml
*/
typedef enum
{
    POWER_STATE_INIT,
    POWER_STATE_OK,
    POWER_STATE_TERMINATING_CLIENTS_NOTIFIED,
    POWER_STATE_TERMINATING_CLIENTS_RESPONDED,
    POWER_STATE_SOPORIFIC_CLIENTS_NOTIFIED,
    POWER_STATE_SOPORIFIC_CLIENTS_RESPONDED,
} powerState;

/*! @brief Power control module state. */
typedef struct
{
    TaskData task;                /*!< Task for Power control messages */
    task_list_with_data_t clients;/*!< List of client tasks */
    uint32 performance_req_count; /*!< Counts the number of requestors for VM_PERFORMANCE profile */
    powerState state;             /*!< The state */
    bool allow_dormant:1;           /*!< Flag that can be modified during testing to disable dormant */
    bool user_initiated_shutdown:1; /*!< Flag that a shutdown was user initiated */
    bool init_complete:1;           /*!< Flag that initialisation is complete and sleep is permitted */
} powerTaskData;

/*! Power client messages, delivered to appPowerClientRegister() clients and
    to clients registered via the message broker.
    \note Clients registering via the message broker will automatically be set
    to allow sleep - equivalent to the client calling the function
    appPowerClientAllowSleep.
    \note If registering via the message broker, the client _must_ still obey
    the requirements of the interface, that is handling and reponding to
    APP_POWER_SLEEP_PREPARE_IND, APP_POWER_SHUTDOWN_PREPARE_IND. */

enum powerClientMessages
{
    /*! Message indicating power module initialisation is complete. */
    APP_POWER_INIT_CFM = POWER_APP_MESSAGE_BASE,
    /*! Message indicating power module clients should prepare to sleep.
        In response, clients should call #appPowerSleepPrepareResponse() when
        prepared to sleep. */
    APP_POWER_SLEEP_PREPARE_IND,
    /*! Message indicating that an external event caused power to cancel sleep.
        Will be sent to clients after all clients have responded to
        #POWER_SLEEP_PREPARE_IND. */
    APP_POWER_SLEEP_CANCELLED_IND,
    /*! Message indicating power module clients shoud prepare to shutdown.
        In response, clients should all #appPowerShutdownPrepareResponse() when
        prepared to shutdown. No response required. */
    APP_POWER_SHUTDOWN_PREPARE_IND,
    /*! Message indicating that an external event caused power to cancel shutdown.
        Will be sent to clients after all clients have responded to
        #POWER_SHUTDOWN_PREPARE_IND. No response required. */
    APP_POWER_SHUTDOWN_CANCELLED_IND,
    /*! Message indicating the application has powered on */
    POWER_ON,
    /*! Message indicating the application is powering off */
    POWER_OFF
};

/*! \brief Power ui provider contexts */
typedef enum
{
    context_power_on,
    context_powering_off,
    context_entering_sleep

} power_provider_context_t;

/*!< Information for power_control */
extern powerTaskData   app_power;

/*! Get pointer to data for power control */
#define PowerGetTaskData()       (&app_power)

/*! \brief returns power task pointer to requesting component

    \return power task pointer.
*/
Task PowerGetTask(void);

/*! \brief The client's response to #POWER_SHUTDOWN_PREPARE_IND.
           This informs power that the client is prepared to shutdown.
    \param task The client task.
*/
void appPowerShutdownPrepareResponse(Task task);

/*! \brief The client's response to #POWER_SLEEP_PREPARE_IND.
           This informs power that the client is prepared to sleep.
    \param task The client task.
*/
void appPowerSleepPrepareResponse(Task task);

/*! \brief Request to power off device
    \return TRUE if the device will now attempt to power off or if the device is
            currently powering off. FALSE if the device cannot currently power
            off (for instance, because the charger is attached, or because
            the device is currently entering sleep).

    If the device can power off, it will inform its clients and wait for a
    response from each (e.g. to allow them to perform tasks related to shutting
    down). After all responses are received, the device will shut down.

    It is possible, whilst waiting for the clients to respond, that the device
    state will change and it will be unable to shutdown (e.g. if a charger is
    attached). In this case after receiving all reponses, the client will send
    a #POWER_SHUTDOWN_CANCELLED_IND to clients informing them that shutdown was
    cancelled.
*/
bool appPowerOffRequest(void);

/*! \brief Power on device.

    This function is called to request powering on the device.  Turning on the
    power supply is handled by the state machine.
*/
void appPowerOn(void);

/*! \brief Reboot device.

    This function is called when the power-off watchdog has expired, this
    means we have failed to shutdown after 10 seconds.

    We should now force a reboot.
*/
void appPowerReboot(void);

/*! \brief Initialise power control task

    Called at start up to initialise the data for power control.
*/
bool appPowerInit(Task init_task);

/*! @brief Register to receive primary power notifications.
    @param task The client's task.
*/
void appPowerClientRegister(Task task);

/*! @brief Unregister a task from receiving power notifications.
    @param task The client task to unregister.
    Silently ignores unregister requests for a task not previously registered
*/
void appPowerClientUnregister(Task task);

/*! @brief Inform power that sleep is allowed.
    @param task The client's task, previously registered with #appPowerClientRegister.
    If all power's clients allow sleep, power will sleep.
*/
void appPowerClientAllowSleep(Task task);

/*! @brief Inform power that sleep is prohibited.
    @param task The client's task, previously registered with #appPowerClientRegister.
    If any of power's client prohibit sleep, power will not sleep.
*/
void appPowerClientProhibitSleep(Task task);

/*! \brief Request the VM runs in VM_PERFORMANCE mode.
    \note The module counts the number of requests to run in VM_PERFORMANCE and
          only reverts to run in VM_BALANCED mode when no VM_PERFORMANCE
          requestors remain.
*/
void appPowerPerformanceProfileRequest(void);

/*! \brief Relinquish request for the VM to run in VM_PERFORMANCE mode.
    \note The module counts the number of requests to run in VM_PERFORMANCE and
          only reverts to run in VM_BALANCED mode when no VM_PERFORMANCE
          requestors remain.
*/
void appPowerPerformanceProfileRelinquish(void);

/*! \brief Signal to power manager that application initialisation is completed.
    \note This must be called to enable sleep (dormant) functionality, and is 
          expected to be called after all power manager clients have registered
          their task.
*/
void appPowerInitComplete(void);

#endif /* POWER_MANAGER_H_ */
