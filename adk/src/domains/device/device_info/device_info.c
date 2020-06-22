/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief
*/

#include "device_info.h"

#include <panic.h>
#include <ps.h>
#include "local_name.h"

static const char device_hardware_version[] = "1.0.0";
static const char device_firmware_version[] = "1.0.0";
static const char device_current_language[] = "en";

#define PSKEY_USB_MANUF_STRING			705
#define PSKEY_USB_PRODUCT_STRING        706
#define PSKEY_USB_SERIAL_NUMBER_STRING  707

static const char * deviceInfo_ReadPsData(uint16 pskey)
{
    uint16 length = 0;
    char* buffer = NULL;
    length = PsFullRetrieve(pskey, NULL, 0)*sizeof(uint16);
    buffer = PanicUnlessMalloc(length + 1);
    PsFullRetrieve(pskey, buffer, length);
    buffer[length] = 0;
    return buffer;
}

const char * DeviceInfo_GetName(void)
{
    uint16 name_length;
    return (const char *)LocalName_GetName(&name_length);
}

const char * DeviceInfo_GetManufacturer(void)
{
    return deviceInfo_ReadPsData(PSKEY_USB_MANUF_STRING);
}

const char * DeviceInfo_GetModelId(void)
{
    return deviceInfo_ReadPsData(PSKEY_USB_PRODUCT_STRING);
}

const char * DeviceInfo_GetHardwareVersion(void)
{
    return device_hardware_version;
}

const char * DeviceInfo_GetFirmwareVersion(void)
{
    return device_firmware_version;
}

const char * DeviceInfo_GetSerialNumber(void)
{
    return deviceInfo_ReadPsData(PSKEY_USB_SERIAL_NUMBER_STRING);
}

const char * DeviceInfo_GetCurrentLanguage(void)
{
    return device_current_language;
}

