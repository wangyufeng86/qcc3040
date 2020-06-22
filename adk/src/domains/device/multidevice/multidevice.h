/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   multidevice Multidevice
\ingroup    device_domain
\brief      Indicates if this device belongs to a pair of devices.
 
This component is expected to be configured by an application.
This component can be used to determine if this device belongs to a pair or it is a standalone device.
When this device belongs to a pair, then it can be determined if it is left or right side of the pair.
 
*/

#ifndef MULTIDEVICE_H_
#define MULTIDEVICE_H_

#include <csrtypes.h>

/*@{*/

/*! \brief Type of this device. */
typedef enum
{
    /*! This is standalone device. */
    multidevice_type_single,

    /*! This device belongs to a pair. */
    multidevice_type_pair
} multidevice_type_t;

typedef enum
{
    /*! This device doesn't have 'side'. Used for standalone devices. */
    multidevice_side_both,

    /*! This device is left in the pair. */
    multidevice_side_left,

    /*! This device is right in the pair. */
    multidevice_side_right
} multidevice_side_t;

/*! \brief Set the type of this device.

    It should be called by an application.

    \param type Type of a device.
*/
void Multidevice_SetType(multidevice_type_t type);

/*! \brief Get the type of this device.

    \return Type of a device.
*/
multidevice_type_t Multidevice_GetType(void);

/*! \brief Set the side for this device.

    It should be called by an application.

    \param side Side of a device.
*/
void Multidevice_SetSide(multidevice_side_t side);

/*! \brief Get the side of this device.

    \return Side of a device.
*/
multidevice_side_t Multidevice_GetSide(void);

/*! \brief Check if device side is left.

    This function will panic if when device type is not multidevice_type_pair.

    \return TURE if side of this device is left.
*/
bool Multidevice_IsLeft(void);


/*@}*/

#endif /* MULTIDEVICE_H_ */
