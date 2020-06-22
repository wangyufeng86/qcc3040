/* Copyright (c) 2011 - 2019 Qualcomm Technologies International, Ltd. */
/*  */
/*!
\defgroup bdaddr bdaddr
\ingroup vm_libs

\brief Helper routines for Bluetooth addresses
*/

/** @{ */

#ifndef BDADDR_H_
#define BDADDR_H_

#include <bdaddr_.h>
#include <app/bluestack/types.h>
#include <app/bluestack/bluetooth.h>

#include <string.h>

/*!
    \brief Convert the Bluestack BD_ADDR_T Bluetooth address to the VM bdaddr 
    type.

    \param[out] out Pointer to the bdaddr struct to hold the address after 
    conversion.
    \param[in] in Pointer to the BD_ADDR_T struct holding the address to be
    converted.
*/
void BdaddrConvertBluestackToVm(bdaddr *out, const BD_ADDR_T *in);

/*!
    \brief Convert the VM bdaddr Bluetooth address to the Bluestack BD_ADDR_T 
    type. 

    \param[out] out Pointer to the BD_ADDR_T struct to hold the address after 
    conversion.
    \param[in] in Pointer to the BD_ADDR_T struct holding the address to be
    converted.
*/

void BdaddrConvertVmToBluestack(BD_ADDR_T *out, const bdaddr *in);

/*!
    \brief Convert the Bluestack TYPED_BD_ADDR_T Bluetooth address to the VM 
    typed_bdaddr type.

    \param[out] out Pointer to the typed_bdaddr struct to hold the address 
    after conversion.
    \param[in] in Pointer to the TYPED_BD_ADDR_T struct holding the address 
    to be converted.
*/
void BdaddrConvertTypedBluestackToVm(
        typed_bdaddr            *out, 
        const TYPED_BD_ADDR_T   *in
        );

/*!
    \brief Convert the VM typed_bdaddr Bluetooth address to the Bluestack 
    TYPED_BD_ADDR_T type.

    \param[out] out Pointer to the TYPED_BD_ADDR_T struct to hold the address 
    after conversion.
    \param[in] in Pointer to the typed_bdaddr struct holding the address to be
    converted.
*/
void BdaddrConvertTypedVmToBluestack(
        TYPED_BD_ADDR_T         *out,
        const typed_bdaddr      *in
        );

/*!
    \brief Convert the Bluestack TP_BD_ADDR_T Bluetooth address to the VM 
    typed_bdaddr type.

    \param[out] out Pointer to the tp_bdaddr struct to hold the address 
    after conversion.
    \param[in] in Pointer to the TP_BD_ADDR_T struct holding the address 
    to be converted.
*/
void BdaddrConvertTpBluestackToVm(
        tp_bdaddr            *out, 
        const TP_BD_ADDR_T   *in
        );

/*!
    \brief Convert the VM tp_bdaddr Bluetooth address to the Bluestack 
    TP_BD_ADDR_T type.

    \param[out] out Pointer to the TP_BD_ADDR_T struct to hold the address 
    after conversion.
    \param[in] in Pointer to the tp_bdaddr struct holding the address to be
    converted.
*/
void BdaddrConvertTpVmToBluestack(
        TP_BD_ADDR_T         *out,
        const tp_bdaddr      *in
        );

/*!
	\brief Macro which calls memset to set the Bluetooth address passed in to 
    zero.

    \param[in] in Pointer to the Bluetooth address.
    \warning The 'in' parameter is not type checked.

    \returns pointer to 'in' passes as parameter
*/
#define BdaddrSetZero(in) memset((in), 0, sizeof(bdaddr))

/*!
	\brief Compares two Bluetooth addresses.

    \param[in] first Pointer to the first Bluetooth address.
    \param[in] second pointer to the second Bluetooth address.

    \return Returns TRUE if they are the same, else returns FALSE.

*/
bool BdaddrIsSame(const bdaddr *first, const bdaddr *second);

/*!
	\brief Checks is the address passed is zero.

    \param[in] in Pointer to the Bluetooth address.

    \return Returns TRUE if the address passed is zero, else returns FALSE.
*/
bool BdaddrIsZero(const bdaddr *in);

/*!
	\brief Sets the Typed Bluetooth address passed to empty. That is, the 
    address is set to 0 and the type is set to INVALID.

    \param[in] in Pointer to the Typed Bluetooth address.
*/
void BdaddrTypedSetEmpty(typed_bdaddr *in);

/*!
	\brief Compares two Typed Bluetooth addresses.

    \param[in] first Pointer to the first Typed Bluetooth address.
    \param[in] second pointer to the second Typed Bluetooth address.

    \return TRUE if the addresses are the same, else returns FALSE.
*/
bool BdaddrTypedIsSame(const typed_bdaddr *first, const typed_bdaddr *second);

/*!
	\brief Checks if the Typed Bluetooth Address is empty.

    \param[in] in Pointer to the Typed Bluetooth address.

    \return Returns TURE if the Typed Bluetooth address passed has the address
    part set to zero and the type set to INVALID
*/
bool BdaddrTypedIsEmpty(const typed_bdaddr *in);

/*!
	\brief Sets the Transport Bluetooth address passed to empty.  
    -# The address '->addrt->addr' set to all 0
    -# The address type '->addrt->type' set to TYPED_BDADDR_INVALID. 
    -# The transport '->transport' set to TRANSPORT_NONE.

    \param[in] in Pointer to the Transport Bluetooth address.
*/
void BdaddrTpSetEmpty(tp_bdaddr *in);

/*!
	\brief Compares two Transport Bluetooth addresses.

    \param[in] first Pointer to the first Transport Bluetooth address.
    \param[in] second pointer to the second Transport Bluetooth address.

    \return TRUE if the addresses are the same, else returns FALSE.
*/
bool BdaddrTpIsSame(const tp_bdaddr *first, const tp_bdaddr *second);

/*!
	\brief Checks if the Transport Bluetooth Address is empty.

    \param[in] in Pointer to the Typed Bluetooth address.

    \return Returns TRUE if the Typed Bluetooth address passed has:
    -# The address '->addrt->addr'set to all 0
    -# The address type '->addrt->type' set to TYPED_BDADDR_INVALID. 
    -# The transport '->transport' set to TRANSPORT_NONE.
*/
bool BdaddrTpIsEmpty(const tp_bdaddr *in);

/*!
    \brief Convert a BR/EDR VM bdaddr to a Transport Bluetooth Address
    -# The address '->addrt->addr' set to *in
    -# The address type '->addrt->type' set to TYPED_BDADDR_PUBLIC. 
    -# The transport '->transport' set to TRANSPORT_BREDR_ACL.

    \param[out] out Pointer to the tp_bdaddr struct to hold the address 
    after conversion.
    \param[in] in Pointer to the bdaddr struct holding the address to be
    converted.
*/
void BdaddrTpFromBredrBdaddr(tp_bdaddr* out, const bdaddr* in);

/*!
    \brief Convert a typed_bdaddr and DM_ACL_FLAGS to a Transport Bluetooth Address
    -# The typed address '->addrt' set to *in
    -# The transport '->transport' set to TRANSPORT_BLE_ACL if DM_ACL_FLAG_ULP is set
    -# The transport '->transport' set to TRANSPORT_BREDR_ACL if DM_ACL_FLAG_ULP is not set

    \param[out] out Pointer to the tp_bdaddr struct to hold the address 
    after conversion.
    \param[in] in Pointer to the typed_bdaddr struct holding the address to be
    converted.
    \param[in] flags Flags indicating connection transport. See DM_ACL_FLAG_*
    bitfield flags in app/bluestack/dm_prim.h.
*/
void BdaddrTpFromTypedAndFlags(tp_bdaddr* out, const typed_bdaddr* in, const uint16 flags);

#endif /* BDADDR_H_ */

/** @} */
