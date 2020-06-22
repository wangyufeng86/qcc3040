/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_battery.h
\brief      State proxy battery event handling.
*/

#ifndef STATE_PROXY_BATTERY_H
#define STATE_PROXY_BATTERY_H

#include <battery_monitor.h>

/*! \brief Get battery state for initial state message. */
void stateProxy_GetInitialBatteryState(void);

/*! \brief Handle local events for battery state change.
    \param[in] state Battery state change event message.
*/
void stateProxy_HandleBatteryLevelUpdateState(const MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T* state);
/*! \brief Handle local events for battery voltage change.
    \param[in] state Battery voltage change event message.
*/
void stateProxy_HandleBatteryLevelUpdateVoltage(const MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T* voltage);

/*! \brief Handle remote events for battery state change.
    \param[in] state Battery state change event message.
 */
void stateProxy_HandleRemoteBatteryLevelState(const MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T* state);
/*! \brief Handle remote events for battery voltage change.
    \param[in] state Battery voltage change event message.
 */
void stateProxy_HandleRemoteBatteryLevelVoltage(const MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T* voltage);

/*! \brief Helper function to register for battery events. */
void stateProxy_RegisterBatteryClient(void);

#endif /* STATE_PROXY_BATTERY_H */
