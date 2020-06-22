/*!
\copyright  Copyright (c) 2015 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       bt_device.h
\brief      Header file for the device management.
*/

#ifndef BT_DEVICE_H_
#define BT_DEVICE_H_

#include <connection.h>
#include <device.h>
#include <hfp.h>
#include <task_list.h>
#include "domain_message.h"
#include "bt_device_typedef.h"

/*! \brief Device Manager UI Provider contexts */
typedef enum
{
    context_handset_connected,
    context_handset_not_connected,

} dm_provider_context_t;

/*! \brief TWS+ version number. */
#define DEVICE_TWS_VERSION  (0x0500)
/*! \brief Device supports HFP */
#define DEVICE_PROFILE_HFP     (1 << 0)
/*! \brief Device supports A2DP */
#define DEVICE_PROFILE_A2DP    (1 << 1)
/*! \brief Device supports AVRCP */
#define DEVICE_PROFILE_AVRCP   (1 << 2)
/*! \brief Device supports SCO Forwarding. */
#define DEVICE_PROFILE_SCOFWD   (1 << 3)
/*! \brief Device supports peer signalling. */
#define DEVICE_PROFILE_PEERSIG  (1 << 4)
/*! \brief Device supports Handover profile. */
#define DEVICE_PROFILE_HANDOVER (1 << 5)

/*! \brief Device supports Mirror profile. */
#define DEVICE_PROFILE_MIRROR   (1 << 6)

/*! Bit in handset flags defining if we need to send the link key to the peer earbud. */
#define DEVICE_FLAGS_HANDSET_LINK_KEY_TX_REQD       (1 << 0)
/*! Bit in handset flags defining if we need to handset address to the peer earbud. */
#define DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD   (1 << 1)
/*! Bit in handset flags indicating handset has just paired and we shouldn't
 * initiate connection to it.  Flag will be cleared as soon as handset connects to us. */
#define DEVICE_FLAGS_JUST_PAIRED                    (1 << 2)
/*! Bit in handset flags indicating this handset was pre-paired on request from peer. */
#define DEVICE_FLAGS_PRE_PAIRED_HANDSET             (1 << 3)
/*! Bit in flags indicating that whilst device type is peer, it is actually PTS tester. */
#define DEVICE_FLAGS_IS_PTS                         (1 << 4)
/*! Bit in flags indicating that this device is ME
    \todo Probably duplicated by DEVICE_TYPE_SELF, but how we manage change of address is still TBD */
#define DEVICE_FLAGS_MIRRORING_ME                   (1 << 6)
/*! Bit in flags indicating that, when required for mirroring, this device has the Central role */
#define DEVICE_FLAGS_MIRRORING_C_ROLE               (1 << 7)
/*! Bit in flags indicating that this device's address is the primary address. */
#define DEVICE_FLAGS_PRIMARY_ADDR                   (1 << 8)
/*! Bit in flags indicating that this device's address is the primary address. */
#define DEVICE_FLAGS_SECONDARY_ADDR                 (1 << 9)
/*! Bit in flags indicating that in process of adding-deleting device */
#define DEVICE_FLAGS_KEY_SYNC_PDL_UPDATE_IN_PROGRESS  (1 << 10)
/*! Define that can be used when not setting any flags */
#define DEVICE_FLAGS_NO_FLAGS                       0
/*! Define the initial capacity of the device version client data task list */
#define DEVICE_VERSION_CLIENT_TASKS_LIST_INIT_CAPACITY      1


typedef enum
{
    DEVICE_TYPE_UNKNOWN = 0, /*!< Remote device type is unknown (default) */
    DEVICE_TYPE_EARBUD,      /*!< Remote device is an earbud */
    DEVICE_TYPE_HANDSET,     /*!< Remote device is a handset */
    DEVICE_TYPE_SELF,        /*!< This device is an earbud - and it's me */
} deviceType;

/*! \brief Types of device link modes. */
typedef enum
{
    DEVICE_LINK_MODE_UNKNOWN = 0,           /*!< The Bluetooth link mode is unknown */
    DEVICE_LINK_MODE_NO_SECURE_CONNECTION,  /*!< The Bluetooth link is not using secure connections */
    DEVICE_LINK_MODE_SECURE_CONNECTION,     /*!< The Bluetooth link is using secure connections */
} deviceLinkMode;

/*! \brief Device manager task data. */
typedef struct
{
    TaskData task;                /*!< Device Manager task */

    TASK_LIST_WITH_INITIAL_CAPACITY(DEVICE_VERSION_CLIENT_TASKS_LIST_INIT_CAPACITY) device_version_client_tasks; /*!< List of tasks interested in device version changes */

    uint8 pdd_len;                /*!< The length of the Persistent Device Data frame used by the Device Manager. */

} deviceTaskData;

/*!< App device management task */
extern deviceTaskData  app_device;

/*! Get pointer to Device Management data structure */
#define DeviceGetTaskData()  (&app_device)
#define DeviceGetVersionClientTasks() (task_list_flexible_t *)(&app_device.device_version_client_tasks)

/*! \brief Register BT Device Persistent Device Data User with Device DB Serialiser. */
void BtDevice_RegisterPddu(void);

/*! \brief Initialse the device manager application module. */
bool appDeviceInit(Task init_task);

/*! \brief Completes initialisation.

    appDeviceInit() is calling ConnectionReadLocalAddr() which sends CL_DM_LOCAL_BD_ADDR_CFM.
    Application initialisation must wait for that message after calling appDeviceInit(),
    and then call appDeviceHandleClDmLocalBdAddrCfm().
    Only then initialisation of bt_device is complete.

    \param message Message payload of CL_DM_LOCAL_BD_ADDR_CFM.

    \return TRUE when successful
 */
bool appDeviceHandleClDmLocalBdAddrCfm(Message message);

/*! \brief Checks if device list is full.

    When it is full, no more devices can be added.

    \return TRUE when it is full
*/
bool BtDevice_IsFull(void);

/*! \brief Handler for unsolicited connection library messages.

    This function needs to be called by the task that is registered for
    connection library messages, as the majority of these are sent to
    a single registered task.

    \param      id      The identifier of the connection library message
    \param      message Pointer to the message content. Can be NULL.
    \param      already_handled Flag indicating if another task has already
                        dealt with this message

    \return TRUE if the handler has dealt with the message, FALSE otherwise
*/
bool BtDevice_HandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled);

/*! \brief Get a handle to the device which has the given BT address.

    \param bd_addr Pointer to read-only device BT address.
    \return The device handle if a device is found, NULL otherwise.
*/
device_t BtDevice_GetDeviceForBdAddr(const bdaddr *bd_addr);

/*! \brief Check whether a device handle is valid

    It is possible to delete devices.
    In rare circumstances this can lead to a device handle no longer
    being valid.

    \param device   The device to check for validity

    \returns TRUE if the device is a valid device entry
 */
bool BtDevice_DeviceIsValid(device_t device);

/*! \brief Find if we have device data for a given BT address.

    \param bd_addr Pointer to read-only device BT address.
    \return TRUE if device found, FALSE otherwise
*/
bool BtDevice_isKnownBdAddr(const bdaddr *bd_addr);

/*! \brief Get peer BT address.

    \param bd_addr BT address of the peer earbud if found. A NULL pointer may be
    passed if the caller just wants to determine if a peer is paired, but doesn't
    want to get the BT address.
    \return TRUE if peer earbud device found, FALSE otherwise
*/
bool appDeviceGetPeerBdAddr(bdaddr *bd_addr);

/*! \brief Get handset BT address.

    Return the first handset BT address.

    \param bd_addr BT address of first handset device.
    \return bool TRUE success, FALSE failure and Bluetooth address not set.
*/
bool appDeviceGetHandsetBdAddr(bdaddr *bd_addr);

/*! \brief Check whether device is paired with handset.

    \return TRUE if handset device is paired, FALSE otherwise
*/
bool BtDevice_IsPairedWithHandset(void);

/*! \brief Check whether device is paired with peer.

    \return TRUE if peer earbud device is paired, FALSE otherwise
*/
bool BtDevice_IsPairedWithPeer(void);


/*! \brief Get the flags associated with a device.

    Return the attributes from the PDL

    \param[in]  bd_addr BT address to look up
    \param[out] flags   Value to hold retrieved flags

    \return bool TRUE success, FALSE failure.
*/
bool appDeviceGetFlags(bdaddr *bd_addr, uint16 *flags);


/*! \brief Get the BT address of this device.

    \param[out] bd_addr BT address of this device.
    \return bool TRUE success, FALSE failure and Bluetooth address not set.
*/
bool appDeviceGetMyBdAddr(bdaddr *bd_addr);

/*! \brief Determine if a BR/EDR address matches a known handset device.

    \note This function should only be called if the address is known
    to be for a standard Bluetooth device (not Bluetooth Low Energy).
    Where a typed Bluetooth Address is used (\ref typed_bdaddr)
    the type must be \ref TYPED_BDADDR_PUBLIC, but this is not
    a sufficient check.

    \note \ref appDeviceTypeIsHandset() is an alternative function that
    can be used if the device may not have been fully paired yet.

    \param bd_addr Pointer to read-only device BT address.

    \return bool TRUE if is a handset and attributes have been exchange,
        FALSE otherwise.
*/
bool appDeviceTypeIsHandset(const bdaddr *bd_addr);

/*! \brief Determine if a BR/EDR address matches a handset device.

    \note this function should only be used if the device may not
    be fully paired. \ref appDeviceIsHandset() also includes a check
    that attribute exchange has completed.

    \note This function should only be called if the address is known
    to be for a standard Bluetooth device (not Bluetooth Low Energy).
    Where a typed Bluetooth Address is used (\ref typed_bdaddr)
    the type must be \ref TYPED_BDADDR_PUBLIC, but this is not
    a sufficient check.

    \param bd_addr Pointer to read-only device BT address.

    \return bool TRUE if is a handset, FALSE if not a handset.
*/
bool appDeviceIsHandset(const bdaddr *bd_addr);

/*! \brief Determine if a device is an Earbud.

    \note This function should only be called if the address is known
    to be for a standard Bluetooth device (not Bluetooth Low Energy).
    Where a typed Bluetooth Address is used (\ref typed_bdaddr)
    the type must be \ref TYPED_BDADDR_PUBLIC, but this is not
    a sufficient check.

    \param bd_addr Pointer to read-only device BT address.

    \return bool TRUE if is an earbud, FALSE if not an earbud.

    \note Being earbud in this context means it is not a handset.
    As such TRUE will be returned if bd_addr belongs to this device,
    or to the remote peer earbud.
 */
bool appDeviceIsPeer(const bdaddr *bd_addr);

/*! \brief Determine if a BLE device is our bonded peer.

    \param tpaddr Pointer to read-only device BT address.

    \return TRUE if device is peer, FALSE otherwise.
*/
bool BtDevice_LeDeviceIsPeer(const tp_bdaddr *tpaddr);

/*! \brief Determine if a device supports a particular profile

    \param bd_addr Pointer to read-only device BT address.
    \return bool TRUE if profile supported, FALSE if not supported.
*/
bool BtDevice_IsProfileSupported(const bdaddr *bd_addr, uint8 profile_to_check);

/*! \brief Set flag to indicate a particular profile is supported

    \param bd_addr Pointer to read-only device BT address.
    \return the handle to the Device on which the supported profile was set.
*/
device_t BtDevice_SetSupportedProfile(const bdaddr *bd_addr, uint8 profile_to_set);

/*! \brief Set device supported link mode

    \param bd_addr Pointer to read-only device BT address.
    \param link_mode The device supported deviceLinkMode
*/
void appDeviceSetLinkMode(const bdaddr *bd_addr, deviceLinkMode link_mode);

/*! \brief Determine if a handset is connected.

    \return bool TRUE handset is connected, FALSE handset is not connected.
*/
bool appDeviceIsHandsetConnected(void);

/*! \brief Check number of handsets connected.

    \return number of handsets connected at the given time.
*/
unsigned appDeviceNumOfHandsetsConnected(void);

/*! \brief Determine if a handset has A2DP disconnected.

    \return bool TRUE handset is disconnected, FALSE handset is connected.
*/
bool appDeviceIsHandsetA2dpDisconnected(void);

/*! \brief Determine if a handset has A2DP connected.

    \return bool TRUE handset is connected, FALSE handset is not connected.
*/
bool appDeviceIsHandsetA2dpConnected(void);

/*! \brief Determine if a handset is streaming A2DP.

    \return bool TRUE handset is streaming, FALSE handset is not streaming.
*/
bool appDeviceIsHandsetA2dpStreaming(void);

/*! \brief Determine if a handset AVRCP disconnected.

    \return bool TRUE handset is disconnected, FALSE handset is connected.
*/
bool appDeviceIsHandsetAvrcpDisconnected(void);

/*! \brief Determine if a handset has AVRCP connected.

    \return bool TRUE handset is connected, FALSE handset is not connected.
*/
bool appDeviceIsHandsetAvrcpConnected(void);

/*! \brief Determine if a handset has HFP connected.

    \return bool TRUE handset is connected, FALSE handset is not connected.
*/
bool appDeviceIsHandsetHfpConnected(void);

/*! \brief Determine if a handset has HFP disconnected.

    \return bool TRUE handset is disconnected, FALSE handset is not disconnected.
*/
bool appDeviceIsHandsetHfpDisconnected(void);

/*! \brief Determine if a handset has an active SCO connection

    \return TRUE if handset has a SCO connection, FALSE otherwose
*/
bool appDeviceIsHandsetScoActive(void);

/*! \brief Determine if a connected to peer earbud.

    \return bool TRUE peer is connected, FALSE peer is not connected.
*/
bool appDeviceIsPeerConnected(void);

/*! \brief Determine if a connected with A2DP to peer earbud.

    \return bool TRUE peer is connected, FALSE peer is not connected.
*/
bool appDeviceIsPeerA2dpConnected(void);

/*! \brief Determine if a connected with AVRCP to peer earbud.

    \return bool TRUE peer is connected, FALSE peer is not connected.
*/
bool appDeviceIsPeerAvrcpConnected(void);

/*! \brief Determine if a connected with AVRCP to peer earbud for AV usage.

    \return bool TRUE peer is connected, FALSE peer is not connected.
*/
bool appDeviceIsPeerAvrcpConnectedForAv(void);

/*! \brief Determine if SCOFWD is connected to peer earbud.

    \return bool TRUE SCOFWD to peer is connected, FALSE otherwise.
*/
bool appDeviceIsPeerScoFwdConnected(void);

/*! \brief Determine if mirror_profile is connected to peer earbud.

    \return bool TRUE mirror_profile to peer is connected, FALSE otherwise.
*/
bool appDeviceIsPeerMirrorConnected(void);

/*! \brief Set flag for handset device indicating if address needs to be sent to peer earbud.

    \param handset_bd_addr BT address of handset device.
    \param reqd  TRUE Flag is set, link key is required to be sent to peer earbud.
                 FALSE Flag is clear, link key does not need to be sent to peer earbud.
    \return bool TRUE Success, FALSE failure device not known.
*/
bool appDeviceSetHandsetAddressForwardReq(const bdaddr *handset_bd_addr, bool reqd);

/*! \brief Determine if a BT address is for a TWS+ handset.

    \param handset_bd_addr Pointer to read-only handset BT address.
    \return bool TRUE address is for TWS+ handset, FALSE either not a handset or not TWS+ handset.
*/
bool appDeviceIsTwsPlusHandset(const bdaddr *handset_bd_addr);

/*! \brief Delete device from pair device list and cache.

    \param bd_addr Pointer to read-only device BT address to delete.
    \return bool TRUE if device deleted, FALSE if device not delete due to being connected.
*/
bool appDeviceDelete(const bdaddr *bd_addr);

/*! \brief Delete all handset devices from the device database and paired device list.
*/
void BtDevice_DeleteAllHandsetDevices(void);

/*! \brief Determine if there is any profile (A2DP,AVRCP,HFP) connected to a handset.

    \return bool TRUE if any profile is connected, FALSE no profiles are connected.
 */
bool appDeviceIsHandsetAnyProfileConnected(void);

/*! \brief Register a client task to receive device version updates.

    \param[in] client_task Task which will receive CON_MANAGER_DEVICE_TYPE_IND message
 */
void appDeviceRegisterDeviceVersionClient(Task client_task);

/*! \brief Update the most recently used device in the PDL and update cache.
    \param bd_addr Pointer to the device BT address most recently used.
*/
void appDeviceUpdateMruDevice(const bdaddr *bd_addr);

/*! Get a handle to a device in the Device Database by its BT address, if it
    doesn't already exist then create it and add it to the Device List.

    \param bd_addr      Pointer to the BT address of the new device
    \param type         The deviceType to create

    \returns the created device
*/
device_t BtDevice_GetDeviceCreateIfNew(const bdaddr *bd_addr, deviceType type);

/*! \brief Get the Primary BT address.
    
    \param[out] bd_addr Pointer to BT address into which Primary address will be written on success.
 
    \return bool TRUE success and bd_addr is valid.
                 FALSE failure and bd_addr is not valid.
*/
bool appDeviceGetPrimaryBdAddr(bdaddr* bd_addr);

/*! \brief Get the Secondary BT address.
 
    \param[out] bd_addr Pointer to BT address into which Secondary address will be written on success.
 
    \return bool TRUE success and bd_addr is valid.
                 FALSE failure and bd_addr is not valid.
*/
bool appDeviceGetSecondaryBdAddr(bdaddr* bd_addr);

/*! \brief Test is an address is the primary address.

    \param bd_addr Pointer to the BT address of the device
*/
bool appDeviceIsPrimary(const bdaddr* bd_addr);

/*! \brief Test is an address is the secondary address.

    \param bd_addr Pointer to the BT address of the device
*/
bool appDeviceIsSecondary(const bdaddr* bd_addr);

/*! \brief Check to determine if this devices address matches the primary address.

    \return bool TRUE if this device has the primary address, else FALSE.
*/
bool BtDevice_IsMyAddressPrimary(void);

/*! \brief Get the deviceType from the provided Device handle.

    \param device Device handle.

    \return deviceType the type of the device.
*/
deviceType BtDevice_GetDeviceType(device_t device);

/*! \brief Set flag to indicate whether the provided profile was connected or not

    \param device the device to modify
    \param profile_mask A bit mask of the profiles to set as connected/diconnected
    \param connected TRUE if profile was connected, FALSE if it wasn't connected.
*/
void BtDevice_SetLastConnectedProfilesForDevice(device_t device, uint8 profile_mask, bool connected);

/*! \brief Set flag to indicate whether the provided profile was connected or not

    \param bd_addr Pointer to read-only device BT address.
    \param profile_mask A bit mask of the profiles to set as connected/diconnected
    \param connected TRUE if profile was connected, FALSE if it wasn't connected.
*/
void BtDevice_SetLastConnectedProfiles(const bdaddr *bd_addr, uint8 profile_mask, bool connected);

/*! \brief Determine which profiles were connected to a device.
*/
uint8 BtDevice_GetLastConnectedProfilesForDevice(device_t device);

/*! \brief Determine which profiles were connected to a device.
*/
uint8 BtDevice_GetLastConnectedProfiles(const bdaddr *bd_addr);

/*! \brief Determine if a device had profiles connected. */
bool BtDevice_GetWasConnected(const bdaddr *bd_addr);

void BtDevice_SetConnectedProfiles(device_t device, uint8 connected_profiles_mask );

uint8 BtDevice_GetConnectedProfiles(device_t device);

/*! \brief Set flag for handset device indicating if link key needs to be sent to
           peer earbud.

    \param handset_bd_addr BT address of handset device.
    \param reqd  TRUE link key TX is required, FALSE link key TX not required.
    \return bool TRUE Success, FALSE failure.
 */
bool BtDevice_SetHandsetLinkKeyTxReqd(bdaddr *handset_bd_addr, bool reqd);

/*! \brief Determine if a device has just paired.

    \param bd_addr Pointer to read-only BT device address.
    \return bool TRUE address is just paired device, FALSE not just paired.
*/
bool BtDevice_IsJustPaired(const bdaddr *bd_addr);

/*! \brief Writes the device attribute profile connected/supported flags based on
    peer's profile connection state. Is much faster than calling individual
    WasConnected/IsSupported functions for each profile. As it only read attributes
    once and only writes attributes when modified.
    \param bd_addr The peer's handset address.
    \param peer_a2dp_connected TRUE if the peer's A2DP profile is connected to the handset.
    \param peer_avrcp_connected TRUE if the peer's AVRCP profile is connected to the handset.
    \param peer_hfp_connected TRUE if the peer's HFP profile is connected to the handset.
    \param peer_hfp_profile_connected The hfp version of the peer's handset.
    \return uint8 Bitmask of profiles that were known to be supported before this function was called.
*/
uint8 BtDevice_SetProfileConnectedAndSupportedFlagsFromPeer(const bdaddr *bd_addr,
    bool peer_a2dp_connected, bool peer_avrcp_connected, bool peer_hfp_connected,
    hfp_profile peer_hfp_profile_connected);
/*! \brief Get Battery Server client config for the "left" battery

    \param bd_addr Pointer to read-only device BT address.
    \param[out] config The client configuration for the left battery
    \note The config should only be requested for handsets.
*/
bool appDeviceGetBatterServerConfigLeft(const bdaddr *bd_addr, uint16* config);

/*! \brief Set Battery Server client config for the "left" battery

    \param bd_addr Pointer to read-only device BT address.
    \param config The client configuration
    \note The config should only be set for handsets.
*/
bool appDeviceSetBatterServerConfigLeft(const bdaddr *bd_addr, uint16 config);

/*! \brief Get Battery Server client config for the "right" battery

    \param bd_addr Pointer to read-only device BT address.
    \param[out] config The client configuration for the right battery
    \note The config should only be requested for handsets.
*/
bool appDeviceGetBatterServerConfigRight(const bdaddr *bd_addr, uint16* config);

/*! \brief Set Battery Server client config for the "right" battery

    \param bd_addr Pointer to read-only device BT address.
    \param config The client configuration
    \note The config should only be set for handsets.
*/
bool appDeviceSetBatterServerConfigRight(const bdaddr *bd_addr, uint16 config);

/*! \brief Set GATT Server client config

    \param bd_addr Pointer to read-only device BT address.
    \param config The client configuration
    \note The config should only be set for handsets.
*/
bool appDeviceSetGattServerConfig(const bdaddr *bd_addr, uint16 config);

/*! \brief Get GATT Server client config

    \param bd_addr Pointer to read-only device BT address.
    \param[out] config The client configuration for the right battery
    \note The config should only be set for handsets.
*/
bool appDeviceGetGattServerConfig(const bdaddr *bd_addr, uint16* config);

/*! \brief Set GATT Server services changed flag

    \param bd_addr Pointer to read-only device BT address.
    \param flag The services changed flag
    \note The flag should only be set for handsets.
*/
bool appDeviceSetGattServerServicesChanged(const bdaddr *bd_addr, uint8 flag);

/*! \brief Get GATT Server services changed flag

    \param bd_addr Pointer to read-only device BT address.
    \param[out] flag The services changed flag
    \note The flag should only be set for handsets.
*/
bool appDeviceGetGattServerServicesChanged(const bdaddr *bd_addr, uint8* flag);


/*! \brief Swap BT addresses of two peer devices in the device data base.

     It swap addresses and associated flags in internal representation of devices.
     It doesn't actually swap addresses of physical devices.
     \note Function will panic if parameters passed are NULL.

     \param bd_addr_1 Address of one of the devices. Parameters are validated.
                      Address must correspond to a device in the device data base.
                      Address must belong to a peer device (it can't be headset for example).
                      bd_addr_1 and bd_addr_2 must be different.
     \param bd_addr_2 Address of the other device.

     \return TRUE if parameters were valid and swap have happened, FALSE otherwise.
 */
bool BtDevice_SwapAddresses(const bdaddr *bd_addr_1, const bdaddr *bd_addr_2);

/*! \brief Swap BT addresses of two peer devices in the device data base.

    It takes BT address of which the local device just have been changed by calling VmOverrideBdaddr().
    The same address should be passed to both of this functions.
    Addresses and associated flags are swapped in devices in the device database
    to match actual state of physical devices.

    \param new_bd_addr Address which should be assigned to the local device.

    \return TRUE if parameter was valid, FALSE otherwise.
            TRUE will be returned also when local address is already new_bd_addr.
            In this case no address swap will happen.

 */
bool BtDevice_SetMyAddress(const bdaddr *new_bd_addr);

void BtDevice_PrintAllDevices(void);

/*! \brief Get the device data for the provided device handle.

    \param device Device handle.
    \param device_data[out] Data associated with the device handle.
*/
void BtDevice_GetDeviceData(device_t device, bt_device_pdd_t *device_data);

/*! \brief Set the device data for the provided device handle.

    \param device Device handle.
    \param device_data Data associated with the device handle.
*/
void BtDevice_SetDeviceData(device_t device, const bt_device_pdd_t *device_data);

/*! \brief Stores device data in ps after some delay

*/
void BtDevice_StorePsDeviceDataWithDelay(void);

/*! \brief Retrieves the public address for a given random or public address.

    \param source_taddr Pointer to a public or random #typed_bdaddr.
    \param public_taddr If the public address is found it will be returned to the location pointed to by this parameter.
    
    \return TRUE if the source_addr was a public or successfully resolved random address, FALSE otherwise.
*/
bool BtDevice_GetPublicAddress(const typed_bdaddr *source_tpaddr, typed_bdaddr *public_tpaddr);

/*! \brief Check if the public bdaddr matches the typed_bdaddr once resolved to a public address.

    \param public_addr Pointer to a public bdaddr for comparison.
    \param taddr Pointer to a public or random typed_bdaddr that can be resolved to a public address for comparison.
    
    \return TRUE if the public_addr is the same as the resolved taddr, FALSE otherwise.
*/
bool BtDevice_ResolvedBdaddrIsSame(const bdaddr *public_addr, const typed_bdaddr *taddr);

/*! \brief Check if two typed addresses match. A match happens if:
           - Both addresses are public and match
           - Both addresses are private and match
           - Both addresses do not match, but can be resolved to the same public address

    \param taddr1 Pointer to a public or random typed_bdaddr
    \param taddr2 Pointer to a public or random typed_bdaddr
    
    \return TRUE if the addresses match, FALSE otherwise.
*/
bool BtDevice_BdaddrTypedIsSame(const typed_bdaddr *taddr1, const typed_bdaddr *taddr2);

/*! \brief Assign the default properties for a device based on its type

    \param device The device to set defaults for
    
    \return TRUE if all defaults set successfully, otherwise FALSE
*/
bool BtDevice_SetDefaultProperties(device_t device);

/*! \brief Update device_property_flags

    \param device The device to set flags for
    \param flags_to_modify The flags to modify
    \param flags The value to set for each flag
    
    \return TRUE if flags were updated successfully, otherwise FALSE
*/
bool BtDevice_SetFlags(device_t device, uint16 flags_to_modify, uint16 flags);

#endif /* BT_DEVICE_H_ */
