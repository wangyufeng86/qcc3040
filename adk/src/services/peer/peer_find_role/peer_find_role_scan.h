/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Peer find role component responsible for LE scanning.

It manages high and low duty scan.

Expected behaviour is:
- When PeerFindRole_FindRole(high_speed_time_ms) and high_speed_time_ms > 0, then high duty scan is started.
- If peer is not found within high_speed_time_ms then on timeout scan is changed to low duty.

Scan mode (high/low duty cycle) can be changed while scan is active.
Stopping and starting scan doesn't change scan mode by itself.

Mode is changed when pfr->role_timeout_ms becomes or cease to be 0 and
peer_find_role_start_scanning_if_inactive() or peer_find_role_update_scan_mode_if_active()
is called.
 
*/

#ifndef PEER_FIND_ROLE_SCAN_H_
#define PEER_FIND_ROLE_SCAN_H_

#include "le_scan_manager.h"

/*@{*/

/*! \brief Start scanning if not in progress

  If scan is already running, but conditions have changed and
  scan duty duty cycle should be changed then scan will be restarted
  with new duty cycle.
*/
void peer_find_role_start_scanning_if_inactive(void);

/*! \brief Update the scan mode when scan is already active

  Expected use of this function is to change scan mode,
  without checking if scan should or shouldn't be turned on.
  This function doesn't change scan state just its mode.
 */
void peer_find_role_update_scan_mode_if_active(void);

/*! \brief Stop scanning if in progress */
void peer_find_role_stop_scan_if_active(void);

/* \brief LE_SCAN_MANAGER_START_CFM message handler

   \param cfm LE_SCAN_MANAGER_START_CFM payload
*/
void peer_find_role_handle_le_scan_manager_start_cfm(const LE_SCAN_MANAGER_START_CFM_T* cfm);

/* \brief LE_SCAN_MANAGER_STOP_CFM message handler

   \param cfm LE_SCAN_MANAGER_STOP_CFM payload
*/
void peer_find_role_handle_le_scan_manager_stop_cfm(const LE_SCAN_MANAGER_STOP_CFM_T *cfm);

/*@}*/

#endif /* PEER_FIND_ROLE_SCAN_H_ */
