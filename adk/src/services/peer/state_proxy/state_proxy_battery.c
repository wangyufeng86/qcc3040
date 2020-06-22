/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_.c
\brief      State proxy battery event handling.
*/

/* local includes */
#include "state_proxy.h"
#include "state_proxy_private.h"
#include "state_proxy_marshal_defs.h"
#include "state_proxy_battery.h"

/* framework includes */
#include <battery_monitor.h>
#include <peer_signalling.h>

/* system includes */
#include <panic.h>
#include <logging.h>
#include <stdlib.h>

/*! \brief Get battery state for initial state message. */
void stateProxy_GetInitialBatteryState(void)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    proxy->local_state->battery = appBatteryGetState();
    proxy->local_state->battery_voltage = appBatteryGetVoltage();
}

/*! \brief Handle local events for battery state change. */
static void stateProxy_HandleBatteryLevelUpdateStateImpl(const MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T* state,
                                                    state_proxy_source source)
{
    DEBUG_LOG("stateProxy_HandleBatteryLevelUpdateState %d", source);

    /* update state */
    stateProxy_GetData(source)->battery = state->state;

    /* notify event specific clients */
    stateProxy_MsgStateProxyEventClients(source,
                                         state_proxy_event_type_battery_state,
                                         state);

    if (source == state_proxy_source_local)
    {
        stateProxy_MarshalToConnectedPeer(MARSHAL_TYPE(MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T), state, sizeof(*state));
    }
}

/*! \brief Handle local events for battery voltage change. */
static void stateProxy_HandleBatteryLevelUpdateVoltageImpl(const MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T* voltage,
                                                           state_proxy_source source)
{
    DEBUG_LOG("stateProxy_HandleBatteryLevelUpdateVoltage %d", source);

    /* update state */
    stateProxy_GetData(source)->battery_voltage = voltage->voltage_mv;

    /* notify event specific clients */
    stateProxy_MsgStateProxyEventClients(source,
                                         state_proxy_event_type_battery_voltage,
                                         voltage);

    if (source == state_proxy_source_local)
    {
        stateProxy_MarshalToConnectedPeer(MARSHAL_TYPE(MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T), voltage, sizeof(*voltage));
    }
}

void stateProxy_HandleBatteryLevelUpdateState(const MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T* state)
{
    stateProxy_HandleBatteryLevelUpdateStateImpl(state, state_proxy_source_local);
}

void stateProxy_HandleBatteryLevelUpdateVoltage(const MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T* voltage)
{
    stateProxy_HandleBatteryLevelUpdateVoltageImpl(voltage, state_proxy_source_local);
}

void stateProxy_HandleRemoteBatteryLevelState(const MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T* state)
{
    stateProxy_HandleBatteryLevelUpdateStateImpl(state, state_proxy_source_remote);
}

void stateProxy_HandleRemoteBatteryLevelVoltage(const MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T* voltage)
{
    stateProxy_HandleBatteryLevelUpdateVoltageImpl(voltage, state_proxy_source_remote);
}

/*! \brief Helper function to register for battery events.
    \param
    \return
*/
void stateProxy_RegisterBatteryClient(void)
{
    batteryRegistrationForm form;
    
    /* register for battery state */
    form.task = stateProxy_GetTask();
    form.representation = battery_level_repres_state;
    form.hysteresis = 0;
    appBatteryRegister(&form);

    /* register for battery voltage */
    form.task = stateProxy_GetTask();
    form.representation = battery_level_repres_voltage;
    form.hysteresis = 10;
    appBatteryRegister(&form);
}

