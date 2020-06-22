/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       battery_monitor_config.h
\brief      Configuration related definitions for battery monitoring.
*/

#ifndef BATTERY_MONITOR_CONFIG_H_
#define BATTERY_MONITOR_CONFIG_H_


//!@{ @name Battery voltage levels in milli-volts
#define appConfigBatteryFullyCharged()      (4200)
#define appConfigBatteryVoltageOk()         (3600)
#define appConfigBatteryVoltageLow()        (3300)
#define appConfigBatteryVoltageCritical()   (3000)
//!@}

//!@{ @name Battery temperature limits in degrees Celsius.
#define appConfigBatteryChargingTemperatureMax() 45
#define appConfigBatteryChargingTemperatureMin() 0
#define appConfigBatteryDischargingTemperatureMax() 60
#define appConfigBatteryDischargingTemperatureMin() -20
//!@}

/*! The interval at which the battery voltage is read. */
#define appConfigBatteryReadPeriodMs() D_SEC(2)

/*! Margin to apply on battery readings before accepting that
    the level has changed. Units of milli-volts */
#define appConfigSmBatteryHysteresisMargin() (50)

#endif /* BATTERY_MONITOR_CONFIG_H_ */
