/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       charger_monitor_config.h
\brief      Configuration related definitions for charger monitoring.
*/

#ifndef CHARGER_MONITOR_CONFIG_H_
#define CHARGER_MONITOR_CONFIG_H_


/*! The time to debounce charger state changes (ms).
    The charger hardware will have a more limited range. */
#define appConfigChargerStateChangeDebounce()          (128)

/*! Trickle-charge current (mA) */
#ifdef QCC3020_FF_ENTRY_LEVEL_AA
#define appConfigChargerTrickleCurrent()               (8)    /* Adjust for Aura LC RDP to meet battery spec */
#else
#define appConfigChargerTrickleCurrent()               (10)
#endif
/*! Pre-charge current (mA)*/
#define appConfigChargerPreCurrent()                   (20)

/*! Pre-charge to fast-charge threshold */
#define appConfigChargerPreFastThresholdVoltage()      (3000)

/*! Fast-charge current (mA) */
#define appConfigChargerFastCurrent()                  (FAST_CHARGE_CURRENT)

/*! Fast-charge (constant voltage) to standby transition point.
    Percentage of the fast charge current */
#define appConfigChargerTerminationCurrent()           (10)

/*! Fast-charge Vfloat voltage */
#define appConfigChargerTerminationVoltage()           (4200)

/*! Standby to fast charge hysteresis (mV) */
#define appConfigChargerStandbyFastVoltageHysteresis() (250)

/* Enable short timeouts for charger/battery platform testing */
#ifdef CF133_BATT
#define CHARGER_PRE_CHARGE_TIMEOUT_MS D_MIN(5)
#define CHARGER_FAST_CHARGE_TIMEOUT_MS D_MIN(15)
#else
#define CHARGER_PRE_CHARGE_TIMEOUT_MS D_MIN(0)
#define CHARGER_FAST_CHARGE_TIMEOUT_MS D_MIN(0)
#endif

/*! The charger will be disabled if the pre-charge time exceeds this limit.
    Following a timeout, the charger will be re-enabled when the charger is detached.
    Set to zero to disable the timeout. */
#define appConfigChargerPreChargeTimeoutMs() CHARGER_PRE_CHARGE_TIMEOUT_MS

/*! The charger will be disabled if the fast-charge time exceeds this limit.
    Following a timeout, the charger will be re-enabled when the charger is detached.
    Set to zero to disable the timeout. */
#define appConfigChargerFastChargeTimeoutMs() CHARGER_FAST_CHARGE_TIMEOUT_MS

#endif /* CHARGER_MONITOR_CONFIG_H_ */
