/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   device_database_serialiser Device Database Serialiser
\ingroup    bt_domain
\brief	    Interface for the Device Database Serialiser module.
*/

#ifndef DEVICE_DATABASE_SERIALISER_H_
#define DEVICE_DATABASE_SERIALISER_H_

#include <csrtypes.h>
#include <device.h>

/*\{*/

typedef uint8 (*get_persistent_device_data_len_bits)(device_t device);

typedef void (*serialise_persistent_device_data)(device_t device, void *buf, uint8 offset);

typedef void (*deserialise_persistent_device_data)(device_t device, void *buf, uint8 data_length, uint8 offset);

/*! \brief Initialise the Device Database Serialiser.
*/
void DeviceDbSerialiser_Init(void);

/*! \brief This function is used to register a Persistent Device Data User module
 *         with the Device Database Serialiser.
 *
 * \param  Identifier for the PDDU being registered
 *
 * \param  Pointer to a function that the Device Database Serialiser shall use
 *         to get the length (in bits) of this PDDU's Persistent Device Data.
 *
 * \param  Pointer to a function that the Device Database Serialiser shall call
 *         to cause the PDDU to serialise its Persistent Device Data into a
 *         provided buffer.
 *
 * \param  Pointer to a function that the Device Database Serialiser shall call
 *         to cause the PDDU to deserialise its Persistent Device Data from a
 *         provided buffer and offset bit index.
*/
void DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        unsigned pddu_id,
        get_persistent_device_data_len_bits get_len,
        serialise_persistent_device_data ser,
        deserialise_persistent_device_data deser);

/*! \brief Serialise the set of Persistent Device Data.
*/
void DeviceDbSerialiser_Serialise(void);

/*! \brief Deserialise the set of Persistent Device Data.
*/
void DeviceDbSerialiser_Deserialise(void);

/*\}*/

#endif /* DEVICE_DATABASE_SERIALISER_H_ */
