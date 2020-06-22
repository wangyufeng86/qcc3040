/****************************************************************************
Copyright (c) 2005 - 2018 Qualcomm Technologies International, Ltd.

FILE NAME
    power_utils.c

DESCRIPTION
    This file contains helper functions for battery charging.

NOTES

**************************************************************************/


/****************************************************************************
    Header files
*/

#include <charger.h>
#include <panic.h>
#include <vm.h>
#include <ps.h>

#include "power_private.h"
#include "power_utils.h"
#include "power_charger.h"
#include "power_onchip.h"
/****************************************************************************
NAME
    PowerUtilInit

DESCRIPTION
    Initialise the power utilities.

RETURNS
    void
*/

void powerUtilInit(void)
{

}

/****************************************************************************
NAME
    powerUtilFatalError

DESCRIPTION
    Disable charger before panic.

RETURNS
    void
*/
void powerUtilFatalError(void)
{
    Panic();
}

/****************************************************************************
NAME
    powerUtilChargerConfigure

DESCRIPTION
    Configure the charger.

RETURNS
    void
*/
void powerUtilChargerConfigure(charger_config_key key, uint16 value)
{
    ChargerConfigure(key, value);
}

/****************************************************************************
NAME
    powerUtilChargerEnable

DESCRIPTION
    Enables/disables the charger.

RETURNS
    bool
*/
bool powerUtilChargerEnable(bool enable)
{
    return ChargerConfigure(CHARGER_ENABLE, enable);
}

/****************************************************************************
NAME
    powerUtilSetChargerCurrents

DESCRIPTION
    Set the charger current

RETURNS
    void
*/
void powerUtilSetChargerCurrents(uint16 current)
{
    powerChargerConfigHelper(CHARGER_CONFIG_CURRENT, current);
}

/****************************************************************************
NAME
    powerUtilMapChargerKeys

DESCRIPTION
    Map charger keys to correct values for ChargerConfigure() Trap.

RETURNS
    charger_config_key
*/
charger_config_key powerUtilMapChargerKeys(charger_chg_cfg_key key)
{
    switch (key)
    {
        case CHARGER_CONFIG_TRIM:
            return CHARGER_TRIM;

        case CHARGER_CONFIG_CURRENT:
            return CHARGER_CURRENT;

        case CHARGER_CONFIG_SUPPRESS_LED0:
            return CHARGER_SUPPRESS_LED0;

        case CHARGER_CONFIG_ENABLE_BOOST:
            return CHARGER_ENABLE_BOOST;

        case CHARGER_CONFIG_USE_EXT_TRAN:
            return CHARGER_USE_EXT_TRAN;

        case CHARGER_CONFIG_TERM_VOLTAGE:
            return CHARGER_TERM_VOLTAGE;

        case CHARGER_CONFIG_ATTACH_RESET_ENABLE:
            return CHARGER_ATTACH_RESET_ENABLE;

        case CHARGER_CONFIG_ENABLE_HIGH_CURRENT_EXTERNAL_MODE:
            return CHARGER_ENABLE_HIGH_CURRENT_EXTERNAL_MODE;

        case CHARGER_CONFIG_SET_EXTERNAL_TRICKLE_CURRENT:
            return CHARGER_SET_EXTERNAL_TRICKLE_CURRENT;

        case CHARGER_CONFIG_ENABLE:
            return CHARGER_ENABLE;

        /* These are not used on V1 charger. */
        case CHARGER_CONFIG_STANDBY_FAST_HYSTERESIS:
        case CHARGER_CONFIG_TRICKLE_CURRENT:
        case CHARGER_CONFIG_PRE_CURRENT:
        case CHARGER_CONFIG_FAST_CURRENT:
        case CHARGER_CONFIG_EXTERNAL_RESISTOR:
        case CHARGER_CONFIG_USE_EXTERNAL_RESISTOR_FOR_FAST_CHARGE:
        case CHARGER_CONFIG_PRE_FAST_THRESHOLD:
        case CHARGER_CONFIG_ITERM_CTRL:
        case CHARGER_CONFIG_STATE_CHANGE_DEBOUNCE:
        default:
            return CHARGER_KEY_INVALID;
    }
}

/****************************************************************************
NAME
    powerUtilGetChargerVoltageLevel

DESCRIPTION
    Returns TRUE if charging voltage is too high, FALSE otherwise.

RETURNS
    bool
*/

bool powerUtilGetChargerVoltageLevel(void)
{
    return (power->vchg >= power->config.vchg.limit);
}

/****************************************************************************
NAME
    powerUtilGetVoltageScale

DESCRIPTION
    Returns the voltage scalar for V1 chargers.

RETURNS
    uint16
*/

uint16 powerUtilGetVoltageScale(void)
{
    return 0x14;
}

/****************************************************************************
NAME
    powerUtilGetVoltage

DESCRIPTION
    Returns the voltage derived from ADC reading and Vref.

RETURNS
    uint16
*/

uint16 powerUtilGetVoltage(const MessageAdcResult *result, uint16 vref_reading)
{
    uint16 res;

    /* On BC7 platforms, there is an issue whereby the VREF voltage is unstable and inaccurate and
       is not read by the same ADC as vbat/vchg, therefore when calculating values for VBAT etc,
       substitue a fixed reference value instead to improve accuracy of these readings */

    /* for inputs of vbat,vchg, use a fixed reference voltage to improve accuracy, VREF is on
       a different ADC to that of the vbat/vchg inputs and hence not accurate to use in calculations */
    if (result->adc_source>adcsel_vref)
    {
        /* for inputs other than AIOs voltage(mV) = reading(ADC counts) * (1350mV/1024) */
        res = (uint16)(((uint32)result->reading * 1350) / 1024);
    }
    /* for aio inputs continue to use the VREF reading which comes from the same ADC as the aio inputs */
    else
    {
        /* for AIOs voltage(mV) = reading(ADC counts) * (mv_per_count = VREF(mV)/VREF(ADC counts)) */
        res = (uint16)(((uint32)VmReadVrefConstant() * result->reading) / vref_reading);
    }
    PRINT(("POWER: BC7 res %u\n", res));

    return res;
}

/****************************************************************************
NAME
    powerUtilGetChargerState

DESCRIPTION
    Get current power library state for the charger (derived from f/w state)

RETURNS
    power_charger_state
*/
power_charger_state powerUtilGetChargerState(void)
{
    switch (ChargerStatus())
    {
        case TRICKLE_CHARGE:
            return power_charger_trickle;

        case FAST_CHARGE:
            return power_charger_fast;

        case DISABLED_ERROR:
            return power_charger_disabled;

        case STANDBY:
            return power_charger_complete;

        case NO_POWER:
            return power_charger_disconnected;

        default:
            break;
    }

    return power->chg_state;
}

/****************************************************************************
NAME
    powerUtilGetThermalConfig

DESCRIPTION
    Get configuration for die temperature management

RETURNS
    void
*/
void powerUtilGetThermalConfig(void)
{
    power->thermal.config.shutdown_period = 0;
    power->thermal.config.shutdown_long_period = 0;
    power->thermal.config.shutdown_temperature = 0;
    power->thermal.config.rampdown_temperature = 0;
    power->thermal.config.reenable_temperature = 0;
}

/****************************************************************************
NAME
    powerUtilIsChargerEnabled

DESCRIPTION
    Check if charger is enabled

RETURNS
    TRUE if enabled, FALSE otherwise
*/
bool powerUtilIsChargerEnabled(void)
{
    /* Have to use trap, called before Power lib initialised */
    charger_status status = ChargerStatus();
    return ((status == TRICKLE_CHARGE) ||
            (status == FAST_CHARGE));
}

/****************************************************************************
NAME
    powerUtilGetThermalDieTemperature

DESCRIPTION
    Get die temperature

RETURNS
    int16
*/
int16 powerUtilGetThermalDieTemperature(void)
{
    return INVALID_TEMPERATURE;
}

/****************************************************************************
NAME
    powerUtilInitCompleteMask

DESCRIPTION
    Returns the initialisation mask.

RETURNS
    uint16
*/
uint16 powerUtilInitCompleteMask(void)
{
    return (power_init_vref | power_init_vbat | power_init_vthm | power_init_vchg);
}

/****************************************************************************
NAME
    powerUtilGetAdcReadPeriod

DESCRIPTION
    Returns the period between ADC read requests.

RETURNS
    Delay
*/
Delay powerUtilGetAdcReadPeriod(power_adc* adc)
{
   return (powerChargerDisconnected() ? adc->period_no_chg : adc->period_chg);
}

/****************************************************************************
NAME
    powerUtilHandleChargerProgressMonitorReq

DESCRIPTION
    Handles requests for Charger Progress Monitor.

RETURNS
    void
*/
void powerUtilHandleChargerProgressMonitorReq(Task task)
{
    UNUSED(task);
    /* Not supported. */
}

/****************************************************************************
NAME
    powerUtilIsChargerProgressMonitorAdc

DESCRIPTION
    Returns TRUE if ADC is Charger Progress Monitor, FALSE otherwise.

RETURNS
    bool
*/
bool powerUtilIsChargerProgressMonitorAdc(vm_adc_source_type adc_source)
{
    UNUSED(adc_source);
    return FALSE;
}

/****************************************************************************
NAME
    powerUtilAdcRequest

DESCRIPTION
    Returns TRUE if ADC is successful, FALSE otherwise.

RETURNS
    bool
*/
bool powerUtilAdcRequest(Task task, vm_adc_source_type adc_source, bool is_vthm, bool drive_ics, uint16 delay)
{
    UNUSED(is_vthm);
    UNUSED(drive_ics);
    UNUSED(delay);
    ADC_PRINT(("POWER: Trp AdcRequest(source = %u)\n", adc_source));
    return (AdcRequest(task, adc_source));
}


/****************************************************************************
NAME
    powerUtilConditionVchgReading
    
DESCRIPTION
    Condition Vchg reading. Provides an opportunity to correct the Vchg reading
    if required by the H/W.
*/
uint16 powerUtilConditionVchgReading(uint16 vchg_reading)
{
    return(vchg_reading);
}

/****************************************************************************
NAME
    powerUtilIsOverVoltage

DESCRIPTION
    This function is called by applications to check whether the battery
    is in Over Voltage.

RETURNS
    bool
*/
bool powerUtilIsOverVoltage(void)
{
    return FALSE;
}

/****************************************************************************
NAME
    powerUtilIsBatteryBelowTerminationVoltage

DESCRIPTION
    This function is called by applications to check whether the battery
    voltage is below the termination voltage.

RETURNS
    bool
*/
bool powerUtilIsBatteryBelowTerminationVoltage(uint16 vbat_millivolts)
{
    return (vbat_millivolts < power->vterm);
}

/****************************************************************************
NAME
    powerUtilChargerFullCurrent

DESCRIPTION
    Determines if the charger is (potentially) drawing full current from
    underlying hardware. Can be called before Power library is initialised.

    Full current is drawn when not in trickle charge state.

RETURNS
    TRUE if enabled, FALSE otherwise
*/
bool powerUtilChargerFullCurrent(void)
{
    charger_status status = ChargerStatus();
    return (status == FAST_CHARGE);
}
