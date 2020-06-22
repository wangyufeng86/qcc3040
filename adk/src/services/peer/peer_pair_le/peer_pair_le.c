/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Miscellaneous functions for the PEER PAIRING OVER LE service
*/

#include <gatt.h>

#include <bdaddr.h>
#include <panic.h>
#include <logging.h>
#include <gatt_handler.h>
#include <gatt_root_key_server_uuids.h>
#include <uuid.h>

#include "peer_pair_le.h"
#include "peer_pair_le_init.h"
#include "peer_pair_le_private.h"
#include "pairing.h"

#include <ui.h>


/* Data local to the peer pairing module */
peerPairLeTaskData peer_pair_le = {0};


static bool peerPairLe_updateScannedDevice(const CL_DM_BLE_ADVERTISING_REPORT_IND_T *scan,
                                          peerPairLeFoundDevice *device)
{
    bool is_existing_device = BdaddrTypedIsSame(&device->taddr, &scan->current_taddr);

    if (is_existing_device)
    {
        if (scan->rssi > device->rssi)
        {
            device->rssi= scan->rssi;
        }
    }
    return is_existing_device;
}


static bool peerPairLe_orderTwoScans(peerPairLeFoundDevice *first, peerPairLeFoundDevice *second)
{
    /* If uninitialised then the address will be empty */
    if (BdaddrTypedIsEmpty(&first->taddr))
    {
        *first = *second;
        PeerPairLe_DeviceSetEmpty(second);
        return TRUE;
    }
    else if (second->rssi > first->rssi)
    {
        peerPairLeFoundDevice temp;
        
        temp = *first;
        *first = *second;
        *second = temp;
        return TRUE;
    }
    return FALSE;
}

static void peerPairLe_ShowScannedDevice(int index)
{
    peerPairLeRunTimeData *ppl = PeerPairLeGetData();
    peerPairLeFoundDevice *device = &ppl->scanned_devices[index];

    UNUSED(device);

    DEBUG_LOG("  %d: %02x %04x %02x %06x rssi:%d",
              index,
              device->taddr.type,
              device->taddr.addr.nap,
              device->taddr.addr.uap,
              device->taddr.addr.lap,
              device->rssi);
}

static void peerPairLe_orderScannedDevices(void)
{
    peerPairLeRunTimeData *ppl = PeerPairLeGetData();

    peerPairLe_orderTwoScans(&ppl->scanned_devices[0], &ppl->scanned_devices[1]);

    DEBUG_LOG("peerPairLe_orderScannedDevices scanned_devices:");
    peerPairLe_ShowScannedDevice(0);
    peerPairLe_ShowScannedDevice(1);
}

static bool peerPairLe_updateScannedDevices(const CL_DM_BLE_ADVERTISING_REPORT_IND_T *scan)
{
    peerPairLeRunTimeData *ppl = PeerPairLeGetData();

    if (!peerPairLe_updateScannedDevice(scan, &ppl->scanned_devices[0]))
    {
        if (!peerPairLe_updateScannedDevice(scan, &ppl->scanned_devices[1]))
        {
            return FALSE;
        }
        peerPairLe_orderScannedDevices();
    }
    return TRUE;
}

/*! \brief Check the advert contains the correct UUID.
    \todo Remove this when the firmware correctly supports filtering
*/
static bool peerPairLe_UuidMatches(const CL_DM_BLE_ADVERTISING_REPORT_IND_T *scan)
{
    /* Allow for legacy pairing, where left/right is not known or checked */
    const uint8 uuid_common[] = {UUID_128_FORMAT_uint8(UUID128_ROOT_KEY_SERVICE)}; 
    const uint8 uuid_left[] = {UUID_128_FORMAT_uint8(UUID128_ROOT_KEY_SERVICE_LEFT)}; 
    const uint8 uuid_right[] = {UUID_128_FORMAT_uint8(UUID128_ROOT_KEY_SERVICE_RIGHT)}; 
    const uint8 *uuid = uuid_common;

    if (   scan->size_advertising_data < 17
        || ble_ad_type_complete_uuid128 != scan->advertising_data[1])
    {
        return FALSE;
    }

    /* We are checking the information received, so if we are left then we
       expect right */
    if (PeerPairLeIsLeft())
    {
        uuid = uuid_right;
    }
    else if (PeerPairLeIsRight())
    {
       uuid = uuid_left;
    }

    return (0 == memcmp(uuid, scan->advertising_data + 2, sizeof(uuid_common)));
}

/*! \brief provides the Peer Pairing current context to the UI module

    \param[in]  void

    \return     current context of Peer Pair service.
*/
static unsigned peerPairLe_GetCurrentContext(void)
{
    peer_pairing_provider_context_t context = context_peer_pairing_idle;

    if (PeerPairLeIsRunning()) {
        context = context_peer_pairing_active;
    }
    return (unsigned)context;
}

void PeerPairLe_HandleFoundDeviceScan(const CL_DM_BLE_ADVERTISING_REPORT_IND_T *scan)
{
    peerPairLeRunTimeData *ppl = PeerPairLeGetData();
    peerPairLeFoundDevice temp;
    PEER_PAIR_LE_STATE state = peer_pair_le_get_state();

    if (   PEER_PAIR_LE_STATE_DISCOVERY != state
        && PEER_PAIR_LE_STATE_SELECTING != state)
    {
        DEBUG_LOG("peerPairLehandleFoundDeviceScan. Advert in unexpected state:%d. No:%d rssi:%d bdaddr %04X:%02X:%06lX",
                        state,
                        scan->num_reports, scan->rssi, 
                        scan->current_taddr.addr.nap,scan->current_taddr.addr.uap,scan->current_taddr.addr.lap);

        return;
    }

    DEBUG_LOG("peerPairLehandleFoundDeviceScan state %d lap 0x%04x rssi %d",
              state, scan->current_taddr.addr.lap, scan->rssi);
    
    /* Eliminate scan results that we are not interested in */
    if (0 == scan->num_reports)
    {
        DEBUG_LOG("peerPairLehandleFoundDeviceScan ignore - no reports");
        return;
    }

    if (!peerPairLe_UuidMatches(scan))
    {
        DEBUG_LOG("peerPairLehandleFoundDeviceScan ignore - UUID");
        return;
    }

    if (scan->rssi < appConfigPeerPairLeMinRssi())
    {
        DEBUG_LOG("peerPairLehandleFoundDeviceScan ignore - rssi %d", scan->rssi);
        return;
    }

    /* See if it a fresh scan for an existing device */
    if (peerPairLe_updateScannedDevices(scan))
    {
        DEBUG_LOG("peerPairLehandleFoundDeviceScan ignore - existing device");
        return;
    }

    temp.taddr = scan->current_taddr;
    temp.rssi = scan->rssi;

    if (peerPairLe_orderTwoScans(&ppl->scanned_devices[1],&temp))
    {
        peerPairLe_orderScannedDevices();
    }

    if (PEER_PAIR_LE_STATE_DISCOVERY == peer_pair_le_get_state())
    {
        peer_pair_le_set_state(PEER_PAIR_LE_STATE_SELECTING);
    }
}

bool PeerPairLe_Init(Task init_task, ear_function ear_role)
{
    UNUSED(init_task);

    peer_pair_le_init(ear_role);

    Ui_RegisterUiProvider(ui_provider_peer_pairing, peerPairLe_GetCurrentContext);
    
    return TRUE;
}

void PeerPairLe_FindPeer(Task task)
{
    peer_pair_le_start_service();
    PeerPairLeSetClient(task);

    MessageSend(PeerPairLeGetTask(), PEER_PAIR_LE_INTERNAL_FIND_PEER, NULL);
}


bool PeerPairLe_HandleConnectionLibraryMessages(MessageId id, Message message,
                                              bool already_handled)
{
    UNUSED(already_handled);
    UNUSED(message);

    bool handled = FALSE;

    if (   PEER_PAIR_LE_STATE_INITIALISED != peer_pair_le_get_state()
        && PeerPairLeGetData())
    {
        switch (id)
        {
            case CL_DM_BLE_SET_SCAN_PARAMETERS_CFM:
                DEBUG_LOG("PeerPairLeHandleConnectionLibraryMessages. CL_DM_BLE_SET_SCAN_PARAMETERS_CFM");
                break;

            default:
                break;
        }
    }
    return handled;
}

