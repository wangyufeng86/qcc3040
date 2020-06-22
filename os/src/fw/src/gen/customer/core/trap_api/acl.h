#ifndef __ACL_H__
#define __ACL_H__
#include <app/acl/acl_if.h>

/*! file  @brief Traps specifically for handover operation */
      
#if TRAPSET_ACL_CONTROL

/**
 *  \brief  Enable/Disable reception of data on the ACL associated with the tp_bdaddr. The
 *  receiver will stop listening for data at L2CAP packet boundary.
 *         
 *  A timeout is provided with this trap. This timeout is only relevant when 
 *  disabling data reception. If the Bluetooth Controller takes more time during 
 *  disabling data reception , the timer will timeout and data reception will be 
 *  re-enabled. The timeout value is expected in micro-seconds. The max expected 
 *  timeout value is 2147 * 10^6 micor-seconds ~ 2147 secs. The trap will return 
 *  failure if the timeout value is more than the expected max value.
 *         
 *  \param addr Device address for which data reception to be enabled/disabled.
 *  \param enable TRUE to enable data reception otherwise FALSE.
 *  \param timeout Timeout in micro-seconds.
 *  \return TRUE for successful data reception enable/disable otherwise FALSE.
 * 
 * \ingroup trapset_acl_control
 */
bool AclReceiveEnable(const tp_bdaddr * addr, bool enable, uint32 timeout);

/**
 *  \brief  Check whether the received data has been processed/consumed by Bluetooth
 *  controller and Bluetooth upper stack.
 *         
 *  This trap needs to be called after the ACL data reception is disabled by 
 *  "AclReceiveEnable()" trap. A desired timeout value to be set by the caller. 
 *  In case of a timeout the caller may continue to call this trap. However, after 
 *  repeated calls if the received ACL data is still not processed, the caller 
 *  needs to re-enable ACL data reception. The timeout value is expected in 
 *  micro-seconds. The max expected timeout value is 2147 * 10^6 micro-seconds
 *  ~ 2147 secs.The trap will return failure if the timeout value is more than
 *  the expected max value.
 *         
 *  \param addr The address of the device for which data reception state to be checked. 
 *  \param timeout Timeout in micro-seconds. 
 *  \return Status of the operation. 
 * 
 * \ingroup trapset_acl_control
 */
acl_rx_processed_status AclReceivedDataProcessed(const tp_bdaddr * addr, uint32 timeout);

/**
 *  \brief Check if transmit data is pending in either Stream, Bluetooth upper stack or
 *  Bluetooth controller.
 *         
 *  \param addr The address of the device for which transmit data pending state to be checked. 
 *  \return TRUE if transmit data is pending, FALSE otherwise.
 *           
 * 
 * \ingroup trapset_acl_control
 */
bool AclTransmitDataPending(const tp_bdaddr * addr);
#endif /* TRAPSET_ACL_CONTROL */
#if TRAPSET_MIRRORING

/**
 *  \brief  Prepare the device for ACL handover. 
 *  \param acl_addr The address of the device for which handover to be prepared. 
 *  \param recipient The address of the recipient device. 
 *  \return Valid handle if handover preparation started successfully, otherwise returns
 *  0xFFFF. This handle should be used in subsequent Aclhandover traps as a handle
 *  for this handover session.
 * 
 * \ingroup trapset_mirroring
 */
uint16 AclHandoverPrepare(const tp_bdaddr * acl_addr, const tp_bdaddr * recipient);

/**
 *  \brief  Check if the device is prepared to handover the ACL.
 *         
 *  This trap should be called after AclHandoverPrepare() trap.
 *         
 *  \param handle The handle value returned by "AclHandoverPrepare()". 
 *  \return Status of the operation. 
 * 
 * \ingroup trapset_mirroring
 */
acl_handover_prepare_status AclHandoverPrepared(uint16 handle);

/**
 *  \brief  This trap should be called by both primary and secondary devices to commit to
 *  resuming operation with the mirrored device in the new secondary or primary
 *  role.
 *         
 * On the primary device, this trap should be called after successful device role
 *  switch.
 * On the secondary device, this trap should be called after slave address change
 *  indication.
 * This trap should not be used on primary or secondary if AclHandoverPrepared()
 *  trap on primary returned failure status.
 *         
 *  \param handle On the primary device, this will be the handle value returned by
 *  "AclHandoverPrepare()". On the secondary device, this will be
 *  connection_handle parameter in mirror acl link create indication 
 *  \return TRUE if operation is success, FALSE otherwise. 
 * 
 * \ingroup trapset_mirroring
 */
bool AclHandoverCommit(uint16 handle);

/**
 *  \brief  This trap may be called by the primary  and the secondary devices to cancel
 *  the handover operation. 
 *         
 * On the primary side this trap may be called after calling AclHandoverPrepared().
 * On the secondary side this trap may be called before getting remote slave
 *  bdaddr change indication.
 *         
 *  \param handle On the primary device, this will be the handle value returned by
 *  "AclHandoverPrepare()". On the secondary device, this will be
 *  connection_handle parameter in mirror acl link create indication
 *  \return TRUE if operation is success, FALSE otherwise. 
 * 
 * \ingroup trapset_mirroring
 */
bool AclHandoverCancel(uint16 handle);

/**
 *  \brief  Read debug statistics for reliable ACL mirroring. 
 *  This trap should be called on primary device only. 
 *  \param tpaddr Bluetooth address of device using ACL link that is being mirrored
 *  \param statistics Pointer to the memory to write debug statistics if successful.
 *  \return True if debug statistics have been read successfully, FALSE otherwise.
 * 
 * \ingroup trapset_mirroring
 */
bool AclReliableMirrorDebugStatistics(const tp_bdaddr * tpaddr, acl_reliable_mirror_debug_statistics_t * statistics);
#endif /* TRAPSET_MIRRORING */
#endif
