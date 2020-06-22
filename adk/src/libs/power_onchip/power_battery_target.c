/****************************************************************************
Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.

FILE NAME
    power_battery_target.c

DESCRIPTION
    This file contains the battery monitoring functionality specific to
    a given architecture target

*/
#include <charger.h>
#include <power_onchip.h>

charger_battery_status PowerOnChipBatteryGetStatusAtBoot(void)
{
    return ChargerGetBatteryStatusAtBoot();
}
