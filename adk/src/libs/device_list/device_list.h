/*!
\copyright  Copyright (c) 2018 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      A collection of device objects that can be retrieved by property type and value.
*/
#ifndef DEVICE_LIST_H_
#define DEVICE_LIST_H_

#include <device.h>

/*! \brief Defines a type for the action function pointer to use with the
           DeviceList_Iterate API.*/
typedef void (*device_list_iterate_callback_t)(device_t device, void *data);

/*! \brief Initialise the device list module. 

    \param num_devices maximum number of devices can be stored in tusted device list.
*/
void DeviceList_Init(uint8 num_devices);

/*! \brief Get the number of devices in the list.

    \return The number of devices currently in the list.
*/
unsigned DeviceList_GetNumOfDevices(void);

/*! \brief Add a device to the list.

    If the device list is full or memory could not be allocated to add the
    #device this function will fail and return FALSE.

    \param device The device to add to the list.
i
    \return TRUE if device was added, FALSE if the add failed for any reason.
*/
bool DeviceList_AddDevice(device_t device);

/*! \brief Remove a device from the list.

    The device is only removed from the list - it is not destroyed. The owner
    of the device must destroy it when it is no longer needed.

    If the device is not in the list or is not valid this function will not
    alter the list and will not panic.

    \param device Device to remove from the list.
*/
void DeviceList_RemoveDevice(device_t device);

/*! \brief Remove all devices from the list.

    The devices are only removed from the list - they are not destroyed.
    The owner(s) of the device(s) must destroy them when no longer needed.

    \note After calling this function, the device list cannot be used
    until DeviceList_Init() has been called again.
*/
void DeviceList_RemoveAllDevices(void);

/*! \brief Find the first device in the list which has a matching property value.

    The device will only be found if it contains the given property and the
    value of the property matches the #size bytes pointed to by #value.

    The caller is expected to know the correct size and format of the property
    being searched for.

    Examples of types of property search:

    Searching for a uint32 property
    \code
    uint32 value = 0x42;

    device = DeviceList_GetFirstDeviceWithPropertyValue(property_id, &value, sizeof(value));
    \endcode

    Searching for a bdaddr property
    \code
    bdaddr value = { 0xFF0F, 0xFF, 0xEF };

    device = DeviceList_GetFirstDeviceWithPropertyValue(property_id, &value, sizeof(value));
    \endcode

    Searching for a pointer property
    \code
    a_struct *value = &my_struct;

    device = DeviceList_GetFirstDeviceWithPropertyValue(property_id, &value, sizeof(value));
    \endcode

    \param id Property to match on a device.
    \param value Pointer to the value of the property to match.
    \param size Size of the property value.

    \return Handle to the first device that matches, or 0 if no match.
*/
device_t DeviceList_GetFirstDeviceWithPropertyValue(device_property_t id, const void *value, size_t size);


/*! \brief Find all the devices in the list which match a specified property value.

    This function is similar to DeviceList_GetFirstDeviceWithPropertyValue, but it returns an array 
    of device handles for all the devices which have the matching property value.

    This function will allocate a buffer of memory to return the device_array. It is the callers
    responsibility to free this buffer when it has used the device handles. The buffer is a copy of the
    device handles in the DeviceList, the list is unaffected when the returned device array is free'd.

    \param id Property to match on a device.
    \param value Pointer to the value of the property to match.
    \param size Size of the property value.
    \param [out] device_array A pointer to the device array of the devices which match the specified property value.
    \param [out] len_device_array A pointer to the size of the device array.

*/
void DeviceList_GetAllDevicesWithPropertyValue(device_property_t id, void *value, size_t size, device_t **device_array, unsigned *len_device_array);

/*! \brief Iterate through the list, calling the supplied action function for each device

    The action function shall be passed the device as a parameter. See the function pointer
    type definition device_list_iterate_callback_t.

    \warning The action function should not add, delete or change the order of the devices
    contained in the device list

    \param action This is the action function that will be called for each device

    \param action_data This parameter can be used to pass data to the action callback
*/
void DeviceList_Iterate(device_list_iterate_callback_t action, void *action_data);

#endif // DEVICE_LIST_H_
