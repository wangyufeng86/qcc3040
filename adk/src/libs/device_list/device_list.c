/*!
\copyright  Copyright (c) 2018 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      A collection of device objects that can be retrieved by property type and value.
*/

#include <string.h>
#include <stdlib.h>

#include <device_list.h>
#include <panic.h>

static device_t *device_list = NULL;
static uint8 trusted_device_list = 0;

void DeviceList_Init(uint8 num_devices)
{
    PanicNotZero(device_list);

    trusted_device_list = num_devices;

    device_list = (device_t *)PanicUnlessMalloc(trusted_device_list * sizeof(device_t));
    memset(device_list, 0, (trusted_device_list * sizeof(device_t)));
}

unsigned DeviceList_GetNumOfDevices(void)
{
    int i;
    unsigned count = 0;

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i])
            count++;
    }

    return count;
}

void DeviceList_RemoveAllDevices(void)
{
    int i;

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i])
        {
            /* Should the device be destroyed by this function? */
            device_list[i] = 0;
        }
    }
    free(device_list);
    device_list = NULL;
}

bool DeviceList_AddDevice(device_t device)
{
    int i;
    bool added = FALSE;

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i] == 0)
        {
            device_list[i] = device;
            added = TRUE;
            break;
        }
        else if (device_list[i] == device)
        {
            added = FALSE;
            break;
        }
    }

    return added;
}

void DeviceList_RemoveDevice(device_t device)
{
    int i;

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i] == device)
        {
            device_list[i] = 0;
            /* Should the device be destroyed by this function? */
            break;
        }
    }
}

static void deviceList_FindMatchingDevices(device_property_t id, const void *value, size_t size, bool just_first, device_t *device_array, unsigned *len_device_array)
{
    int i;
    unsigned num_found_devices = 0;

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i])
        {
            void *property;
            size_t property_size;

            if (Device_GetProperty(device_list[i], id, &property, &property_size))
            {
                if ((size == property_size) && !memcmp(value, property, size))
                {
                    device_array[num_found_devices] = device_list[i];
                    num_found_devices += 1;

                    if (just_first)
                        break;
                }
            }
        }
    }
    *len_device_array = num_found_devices;
}

device_t DeviceList_GetFirstDeviceWithPropertyValue(device_property_t id, const void *value, size_t size)
{
    device_t found_devices[1] = {0};
    unsigned num_found_devices = 0;

    deviceList_FindMatchingDevices(id, value, size, TRUE, found_devices, &num_found_devices);
    PanicFalse(num_found_devices <= 1);

    return found_devices[0];
}

void DeviceList_GetAllDevicesWithPropertyValue(device_property_t id, void *value, size_t size, device_t **device_array, unsigned *len_device_array)
{
    unsigned num_found_devices = 0;

    device_t *found_devices = NULL;

    found_devices = (device_t*)PanicUnlessMalloc(trusted_device_list * sizeof(device_t));
    memset(found_devices, 0, trusted_device_list * sizeof(device_t) );

    deviceList_FindMatchingDevices(id, value, size, FALSE, found_devices, &num_found_devices);

    *device_array = found_devices;
    if (num_found_devices)
    {
        memcpy(*device_array, found_devices, num_found_devices*sizeof(device_t));
    }

    *len_device_array = num_found_devices;
}

void DeviceList_Iterate(device_list_iterate_callback_t action, void *action_data)
{
    int i;

    PanicFalse(action);

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i])
        {
            action(device_list[i], action_data);
        }
    }
}