/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Contains helper functions for accessing the device properties data.
*/

#include "device_properties.h"

#include <bdaddr.h>
#include <csrtypes.h>
#include <device.h>
#include <panic.h>

bdaddr DeviceProperties_GetBdAddr(device_t device)
{
    void * object = NULL;
    size_t size = 0;
    PanicFalse(Device_GetProperty(device, device_property_bdaddr, &object, &size));
    PanicFalse(size==sizeof(bdaddr));
    return *((bdaddr *)object);
}

void DeviceProperties_SanitiseBdAddr(bdaddr *bd_addr)
{
    // Sanitise bluetooth address VMCSA-1007
    bdaddr sanitised_bdaddr = {0};
    sanitised_bdaddr.uap = bd_addr->uap;
    sanitised_bdaddr.lap = bd_addr->lap;
    sanitised_bdaddr.nap = bd_addr->nap;
    memcpy(bd_addr, &sanitised_bdaddr, sizeof(bdaddr));
}

void DeviceProperties_SetBdAddr(device_t device, bdaddr *bd_addr)
{
    DeviceProperties_SanitiseBdAddr((bdaddr *)bd_addr);
    Device_SetProperty(device, device_property_bdaddr, (void*)bd_addr, sizeof(bdaddr));
}
