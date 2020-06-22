/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       temperature.c
\brief      Top level temperature sensing implementation. Uses a temperature sensor
            e.g. thermistor to actually perform the measurements.
*/

#ifdef INCLUDE_TEMPERATURE

#include <panic.h>
#include <pio.h>
#include <vm.h>
#include <hydra_macros.h>
#include <limits.h>
#include <task_list.h>

#include "adk_log.h"
#include "temperature.h"
#include "temperature_config.h"
#include "temperature_sensor.h"

/*! Messages sent within the temperature module. */
enum headset_temperature_internal_messages
{
    /*! Message sent to trigger a temperature measurement */
    MESSAGE_TEMPERATURE_INTERNAL_MEASUREMENT_TRIGGER = 1,
};


/*! The indexes in a #task_list_data_t arr_s8 to store the client data. */
enum client_indexes
{
    CLIENT_LOWER_LIMIT_INDEX,
    CLIENT_UPPER_LIMIT_INDEX,
    CLIENT_CURRENT_STATE_INDEX,
};

/*!< Task information for temperature */
temperatureTaskData app_temperature;

/*! \brief Inform a single client of temperature events */
static bool appTemperatureServiceClient(Task task, task_list_data_t *data, void *arg)
{
    temperatureTaskData *temperature = arg;
    int8 lower_limit = data->arr_s8[CLIENT_LOWER_LIMIT_INDEX];
    int8 upper_limit = data->arr_s8[CLIENT_UPPER_LIMIT_INDEX];
    int8 t = temperature->temperature;
    temperatureState next_state;

    next_state = (t >= upper_limit) ? TEMPERATURE_STATE_ABOVE_UPPER_LIMIT :
                    (t <= lower_limit) ? TEMPERATURE_STATE_BELOW_LOWER_LIMIT :
                        TEMPERATURE_STATE_WITHIN_LIMITS;

    if (next_state != data->arr_s8[CLIENT_CURRENT_STATE_INDEX])
    {
        MESSAGE_MAKE(ind, TEMPERATURE_STATE_CHANGED_IND_T);
        ind->state = next_state;
        data->arr_s8[CLIENT_CURRENT_STATE_INDEX] = next_state;
        MessageSend(task, TEMPERATURE_STATE_CHANGED_IND, ind);
    }

    /* Iterate through every client */
    return TRUE;
}

/*! \brief Inform all clients of temperature events */
static void appTemperatureServiceClients(temperatureTaskData *temperature)
{
    TaskList_IterateWithDataRawFunction(temperature->clients, appTemperatureServiceClient, temperature);
}

/*! \brief Handle temperature messages */
static void appTemperatureHandleMessage(Task task, MessageId id, Message message)
{
    temperatureTaskData *temperature = (temperatureTaskData *)task;
    switch (id)
    {
        case MESSAGE_TEMPERATURE_INTERNAL_MEASUREMENT_TRIGGER:
            appTemperatureSensorRequestMeasurement(task);
        break;

        default:
        {
            int8 t_new = temperature->temperature;
            if (appTemperatureSensorHandleMessage(task, id, message, &t_new))
            {
                temperature->lock = 0;
                temperature->temperature = t_new;
                appTemperatureServiceClients(temperature);
                MessageSendLater(&temperature->task,
                                 MESSAGE_TEMPERATURE_INTERNAL_MEASUREMENT_TRIGGER, NULL,
                                 appConfigTemperatureMeasurementIntervalMs());
            }
        }
        break;
    }
}


bool appTemperatureInit(Task init_task)
{
    temperatureTaskData *temperature = TemperatureGetTaskData();

    DEBUG_LOG("appTemperatureInit");

    temperature->clients = TaskList_WithDataCreate();
    temperature->task.handler = appTemperatureHandleMessage;
    temperature->temperature = SCHAR_MIN;

    appTemperatureSensorInit();
    appTemperatureSensorRequestMeasurement(&temperature->task);

    temperature->lock = 1;
    MessageSendConditionally(init_task, TEMPERATURE_INIT_CFM, NULL, &temperature->lock);

    return TRUE;
}

void appTemperatureDestroy(void)
{
    temperatureTaskData *temperature = TemperatureGetTaskData();
    DEBUG_LOG("appTemperatureDestroy");

    TaskList_Destroy(temperature->clients);
    memset(temperature, 0, sizeof(*temperature));
}

bool appTemperatureClientRegister(Task task, int8 lower_limit, int8 upper_limit)
{
    temperatureTaskData *temperature = TemperatureGetTaskData();
    task_list_data_t data = {0};

    DEBUG_LOG("appTemperatureClientRegister Task=%p (%d, %d)", task, lower_limit, upper_limit);

    data.arr_s8[CLIENT_LOWER_LIMIT_INDEX] = lower_limit;
    data.arr_s8[CLIENT_UPPER_LIMIT_INDEX] = upper_limit;
    data.arr_s8[CLIENT_CURRENT_STATE_INDEX] = TEMPERATURE_STATE_UNKNOWN;
    appTemperatureServiceClient(task, &data, temperature);
    PanicFalse(TaskList_AddTaskWithData(temperature->clients, task, &data));
    return TRUE;
}

void appTemperatureClientUnregister(Task task)
{
    temperatureTaskData *temperature = TemperatureGetTaskData();

    DEBUG_LOG("appTemperatureClientUnregister Task=%p", task);

    PanicFalse(TaskList_RemoveTask(temperature->clients, task));
}

temperatureState appTemperatureClientGetState(Task task)
{
    temperatureTaskData *temperature = TemperatureGetTaskData();
    task_list_data_t *data;
    PanicFalse(TaskList_GetDataForTaskRaw(temperature->clients, task, &data));
    return (temperatureState)data->arr_s8[CLIENT_CURRENT_STATE_INDEX];
}

int8 appTemperatureGet(void)
{
    return TemperatureGetTaskData()->temperature;
}

#endif
