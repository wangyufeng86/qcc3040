/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Peer find role component responsible for LE scanning.

*/

#include "peer_find_role_scan.h"

#include <gatt_role_selection_server_uuids.h>
#include <logging.h>

#include "peer_find_role_private.h"


/*! \brief Set activity flag(s) for the find role task

    \param flags_to_set Bitmask of scanning activities that will block actions
*/
static void peer_find_role_scan_activity_set(peerFindRoleScanActiveStatus_t flags_to_set)
{
    uint16 old_busy = PeerFindRoleGetScanBusy();
    uint16 new_busy = old_busy | flags_to_set;

    if (new_busy != old_busy)
    {
        PeerFindRoleSetScanBusy(new_busy);
        DEBUG_LOG_VERBOSE("peer_find_role_scan_activity_set. Set 0x%x. Now 0x%x", flags_to_set, new_busy);
    }
}

/*! \brief Clear activity flag(s) for the find role task

    Clearing flags may cause a message to be sent

    \param flags_to_clear Bitmask of scanning activities that no longer block actions
*/
static void peer_find_role_scan_activity_clear(peerFindRoleScanActiveStatus_t flags_to_clear)
{
    uint16 old_busy = PeerFindRoleGetScanBusy();
    uint16 new_busy = old_busy & ~flags_to_clear;

    if (new_busy != old_busy)
    {
        PeerFindRoleSetScanBusy(new_busy);
        DEBUG_LOG_VERBOSE("peer_find_role_scan_activity_clear. Cleared 0x%x. Now 0x%x", flags_to_clear, new_busy);
    }
}

/*  \brief Build scan filter and start scanning.

    \param mode Scan mode high or low duty scan.
*/
static void peer_find_role_start_scanning(scan_state_mode_t mode)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();
    tp_bdaddr tp_addr;
    uint16 uuid = UUID16_ROLE_SELECTION_SERVICE;
    le_advertising_report_filter_t filter;

    tp_addr.transport = TRANSPORT_BLE_ACL;
    tp_addr.taddr.type = TYPED_BDADDR_PUBLIC;
    tp_addr.taddr.addr = pfr->primary_addr;

    filter.ad_type = ble_ad_type_complete_uuid16;
    filter.interval = filter.size_pattern = sizeof(uuid);
    filter.pattern = (uint8*)&uuid;
    filter.find_tpaddr = &tp_addr;

    if(mode == scan_state_low_duty)
    {
        DEBUG_LOG_INFO("peer_find_role_start_scanning: starting low duty scan");
        pfr->scan_mode = scan_state_low_duty;
        LeScanManager_Start(PeerFindRoleGetTask(),le_scan_interval_slow, &filter);

    }
    else
    {
        DEBUG_LOG_INFO("peer_find_role_start_scanning: starting high duty scan");
        pfr->scan_mode = scan_state_high_duty;
        LeScanManager_Start(PeerFindRoleGetTask(), le_scan_interval_fast, &filter);
    }


}

/*  \brief Check if switch from high to low duty scan should happen. */
static bool peer_find_role_is_it_high_to_low_duty_transtion(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    return pfr->scan_mode == scan_state_high_duty && pfr->role_timeout_ms == 0;
}

/*  \brief Check if switch from low to high duty scan should happen. */
static bool peer_find_role_is_it_low_to_high_duty_transition(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    return pfr->scan_mode == scan_state_low_duty && pfr->role_timeout_ms > 0;
}

void peer_find_role_start_scanning_if_inactive(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG_VERBOSE("peer_find_role_start_scanning_if_inactive. Scan for RRA too");

    if(!peer_find_role_media_active())
    {
        if (!LeScanManager_IsTaskScanning(PeerFindRoleGetTask()))
        {
            scan_state_mode_t mode = pfr->role_timeout_ms == 0 ? scan_state_low_duty : scan_state_high_duty;

            peer_find_role_scan_activity_set(PEER_FIND_ROLE_ACTIVE_SCANNING);
            BdaddrTypedSetEmpty(&pfr->peer_connection_typed_bdaddr);

            peer_find_role_start_scanning(mode);

            /* Record that scanning should be enabled, in case this fails and a re-try
               is required */
            pfr->scan_enable = TRUE;
        }
        else
        {
            /* When mode change is required then scan needs to be restarted. */
            peer_find_role_update_scan_mode_if_active();
        }

    }
}

void peer_find_role_update_scan_mode_if_active(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    DEBUG_LOG_VERBOSE("peer_find_role_update_scan_mode_if_active.");

    if(!peer_find_role_media_active())
    {
        if (LeScanManager_IsTaskScanning(PeerFindRoleGetTask()))
        {
            if(peer_find_role_is_it_high_to_low_duty_transtion())
            {
                DEBUG_LOG_VERBOSE("peer_find_role_update_scan_mode_if_active: high to low duty transition");
                pfr->scan_mode = scan_state_low_duty;
                LeScanManager_Stop(PeerFindRoleGetTask());
            }
            else if(peer_find_role_is_it_low_to_high_duty_transition())
            {
                DEBUG_LOG_VERBOSE("peer_find_role_update_scan_mode_if_active: low to high duty transition");
                pfr->scan_mode = scan_state_high_duty;
                LeScanManager_Stop(PeerFindRoleGetTask());
            }
        }

    }
}

void peer_find_role_stop_scan_if_active(void)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    /* Stop always resets scan to high duty scan as handling of stop cfm depends on scan mode */
    pfr->scan_mode = scan_state_high_duty;

    if (LeScanManager_IsTaskScanning(PeerFindRoleGetTask()))
    {
        DEBUG_LOG_INFO("peer_find_role_stop_scan_if_active stopping");
        LeScanManager_Stop(PeerFindRoleGetTask());
    }
    else
    {
        DEBUG_LOG_VERBOSE("peer_find_role_stop_scan_if_active not stopping");
    }
    /* Record that scanning should be disabled, in case this fails and a re-try
       is required */
    pfr->scan_enable = FALSE;
}


void peer_find_role_handle_le_scan_manager_start_cfm(const LE_SCAN_MANAGER_START_CFM_T* cfm)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    switch (cfm->status)
    {
        case LE_SCAN_MANAGER_RESULT_SUCCESS:
            DEBUG_LOG_VERBOSE("peer_find_role_handle_le_scan_manager_start_cfm success");
            peer_find_role_scan_activity_set(PEER_FIND_ROLE_ACTIVE_SCANNING);
        break;

        case LE_SCAN_MANAGER_RESULT_BUSY:
            if (pfr->scan_enable)
            {
                DEBUG_LOG_VERBOSE("peer_find_role_handle_le_scan_manager_start_cfm busy retrying");
                peer_find_role_start_scanning_if_inactive();
            }
        break;

        default:
        case LE_SCAN_MANAGER_RESULT_FAILURE:
            DEBUG_LOG("peer_find_role_handle_le_scan_manager_start_cfm  ignoring failure");
        break;
    }
}

void peer_find_role_handle_le_scan_manager_stop_cfm(const LE_SCAN_MANAGER_STOP_CFM_T *cfm)
{
    peerFindRoleTaskData *pfr = PeerFindRoleGetTaskData();

    /* Stop may be result of calling peer_find_role_stop_scan_if_active() or switching scan modes,
       that way in some conditions stop cfm handler is actually starting new scan. */

    switch (cfm->status)
    {
        case LE_SCAN_MANAGER_RESULT_SUCCESS:
            DEBUG_LOG("peer_find_role_handle_le_scan_manager_stop_cfm success");
            peer_find_role_scan_activity_clear(PEER_FIND_ROLE_ACTIVE_SCANNING);

            if(pfr->scan_mode == scan_state_low_duty)
            {
                DEBUG_LOG_VERBOSE("peer_find_role_handle_le_scan_manager_stop_cfm: switching to low duty scan");
                peer_find_role_start_scanning(TRUE);
            }
            else if(pfr->scan_enable)
            {
                DEBUG_LOG_VERBOSE("peer_find_role_handle_le_scan_manager_stop_cfm: start have been requested during stop");
                peer_find_role_start_scanning_if_inactive();
            }
        break;

        case LE_SCAN_MANAGER_RESULT_BUSY:
            if (!pfr->scan_enable)
            {
                DEBUG_LOG("peer_find_role_handle_le_scan_manager_stop_cfm busy retrying");
                peer_find_role_stop_scan_if_active();
            }
            else
            {
                LeScanManager_Stop(PeerFindRoleGetTask());
            }
        break;

        default:
        case LE_SCAN_MANAGER_RESULT_FAILURE:
            DEBUG_LOG("peer_find_role_handle_le_scan_manager_stop_cfm  ignoring failure");
        break;
    }
}


