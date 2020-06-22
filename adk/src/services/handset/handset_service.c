/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Handset service
*/

#include <bdaddr.h>
#include <vm.h>
#include <logging.h>
#include <task_list.h>

#include <bt_device.h>
#include <pairing.h>
#include <bredr_scan_manager.h>
#include <connection_manager.h>
#include <device_properties.h>
#include <local_addr.h>
#include <profile_manager.h>
#include <pairing.h>
#include <timestamp_event.h>
#include <key_sync.h>

#include "handset_service.h"
#include "handset_service_sm.h"
#include "handset_service_protected.h"

/*! Handset Service module data. */
handset_service_data_t handset_service;

/*
    Helper functions
*/

/*! Get if the handset service is in pairing mode. */
#define HandsetService_IsPairing() (HandsetService_Get()->pairing)

/*! Get if the handset service has a BLE connection. */
#define HandsetService_IsBleConnected() (HandsetService_Get()->ble_connected)

/*! Get if the handset service is connectable */
#define HandsetService_IsBleConnectable() (HandsetService_Get()->ble_connectable)

/*! Get handset service LE advertising data set select/release state */
#define HandsetService_GetLeAdvDataSetState() (HandsetService_Get()->ble_adv_state)

/*! Get handset service selected LE advertising data set */
#define HandsetService_GetLeAdvSelectedDataSet() (HandsetService_Get()->le_advert_data_set)

static void handsetService_SendLeConnectableIndication(bool connectable);

/*! Stores if the Handset can be paired.

    \param pairing The headset pairing state.
*/
static void HandsetService_SetPairing(bool pairing)
{ 
    HandsetService_Get()->pairing = pairing;
}

/*! Stores if the Handset has a BLE connection.

    \param ble_connected The BLE connected state.
*/
static void handsetService_SetBleConnected(bool ble_connected)
{ 
    HandsetService_Get()->ble_connected = ble_connected;
}

/*! \brief Disable advertising by releasing the LE advertising data set */
static void handsetService_DisableAdvertising(void)
{
    handset_service_data_t *hs = HandsetService_Get();

    HS_LOG("handsetService_DisableAdvertising, release set with handle=%p", hs->le_advert_handle);

    PanicFalse(LeAdvertisingManager_ReleaseAdvertisingDataSet(hs->le_advert_handle));
    
    hs->le_advert_handle = NULL;
    hs->ble_adv_state = handset_service_le_adv_data_set_state_releasing;
    
    HS_LOG("handsetService_DisableAdvertising,  handle=%p", hs->le_advert_handle);
}

/*! \brief Get advertising data set which needs to be selected */
static le_adv_data_set_t handsetService_GetLeAdvDataSetToBeSelected(void)
{    
    bool is_pairing = HandsetService_IsPairing();
    bool is_local_addr_public = LocalAddr_IsPublic();
        
    HS_LOG("handsetService_GetLeAdvDataSetToBeSelected, Is in pairing:%d, Is local address public:%d", is_pairing, is_local_addr_public);

    le_adv_data_set_t set;
    
    if(is_pairing || is_local_addr_public)
    {
        set = le_adv_data_set_handset_identifiable;
    }
    else
    {
        set = le_adv_data_set_handset_unidentifiable;
    }
    
    return set;
}

/*! \brief Enable advertising by selecting the LE advertising data set */
static void handsetService_EnableAdvertising(void)
{
    handset_service_data_t *hs = HandsetService_Get();

    le_adv_select_params_t adv_select_params;
    le_adv_data_set_handle adv_handle = NULL;

    HS_LOG("handsetService_EnableAdvertising, Le Adv State is %x, Le Adv Selected Data Set is %x", hs->ble_adv_state, hs->le_advert_data_set);

    adv_select_params.set = handsetService_GetLeAdvDataSetToBeSelected();

    adv_handle = LeAdvertisingManager_SelectAdvertisingDataSet(HandsetService_GetTask(), &adv_select_params);
    
    hs->ble_adv_state = handset_service_le_adv_data_set_state_selecting;
    
    hs->le_advert_data_set = adv_select_params.set;

    if (adv_handle != NULL)
    {
        hs->le_advert_handle = adv_handle;

        HS_LOG("handsetService_EnableAdvertising. Selected set with handle=%p", hs->le_advert_handle);
    }
}

/*! \brief Updates the BLE advertising data 
    \return TRUE if Advertising Data shall be updated, FALSE otherwise.  
*/
bool handsetService_UpdateAdvertisingData(void)
{
    handset_service_le_adv_data_set_state_t le_adv_state = HandsetService_GetLeAdvDataSetState();
    
    HS_LOG("handsetService_UpdateAdvertisingData. Le advertising data set select/release state is %x", le_adv_state);
    
    if( (handset_service_le_adv_data_set_state_releasing == le_adv_state) || (handset_service_le_adv_data_set_state_selecting == le_adv_state) )
    {
        return TRUE;
    }
    
    handset_service_data_t *hs = HandsetService_Get();
    bool is_le_connected = HandsetService_IsBleConnected();
    bool is_le_connectable = HandsetService_IsBleConnectable();
    bool is_le_adv_data_set_update_needed = HandsetService_GetLeAdvSelectedDataSet() != handsetService_GetLeAdvDataSetToBeSelected();

    HS_LOG("handsetService_UpdateAdvertisingData. Le Connection Status is %x Le Connectable Status is %x Is Data Set Update Needed %x", is_le_connected, is_le_connectable,is_le_adv_data_set_update_needed);
    
    if(hs->le_advert_handle)
    {
        HS_LOG("handsetService_UpdateAdvertisingData. There is an active data set with handle=%p", hs->le_advert_handle);

        le_adv_data_set_t data_set;
        
        data_set = handsetService_GetLeAdvDataSetToBeSelected();
    
        HS_LOG("handsetService_UpdateAdvertisingData. Active data set is %x, requested data set is %x", hs->le_advert_data_set, data_set);
        
        if (is_le_connected || !is_le_connectable || is_le_adv_data_set_update_needed)
        {
            handsetService_DisableAdvertising();
            return TRUE;
        }
    }
    else
    {
        HS_LOG("handsetService_UpdateAdvertisingData. There is no active data set");

        if (!is_le_connected && is_le_connectable)
        {
            handsetService_EnableAdvertising();
            return TRUE;
        }
    }

    return FALSE;
}

/*! Try to find an active handset state machine for a device_t.

    \param device Device to search for.
    \return Pointer to the matching state machine, or NULL if no match.
*/
static handset_service_state_machine_t *handsetService_GetSmForDevice(device_t device)
{
    handset_service_state_machine_t *sm_match = NULL;
    handset_service_state_machine_t *sm = HandsetService_GetSm();
    uint16 index;

    PanicNull(sm);

    for (index = 0; index < HANDSET_SERVICE_MAX_SM; index++)
    {
        if (sm && (sm->state != HANDSET_SERVICE_STATE_NULL)
            && (sm->handset_device == device))
        {
            sm_match = sm;
            break;
        }
        sm++;
    }

    return sm_match;
}

/*! Try to find an active BR/EDR handset state machine for an address.

    \param[in] addr BR/EDR address to search for.
    
    \return Pointer to the matching state machine, or NULL if no match.
*/
static handset_service_state_machine_t *handsetService_GetSmForBredrAddr(const bdaddr *addr)
{
    handset_service_state_machine_t *sm_match = NULL;
    handset_service_state_machine_t *sm = HandsetService_GetSm();
    uint16 index;
    const bdaddr *bredr_bdaddr;

    PanicNull(sm);

    if (!BdaddrIsZero(addr))
    {
        for (index = 0; index < HANDSET_SERVICE_MAX_SM; index++)
        {
            bredr_bdaddr = &sm->handset_addr;

            DEBUG_LOG_VERBOSE("handsetService_GetSmForBredrAddr Check index [%d] state [%d] addr [%04x,%02x,%06lx]",
                              index, sm->state, bredr_bdaddr->nap, bredr_bdaddr->uap, bredr_bdaddr->lap);

            if ((sm->state != HANDSET_SERVICE_STATE_NULL)
                && BdaddrIsSame(bredr_bdaddr, addr))
            {
                sm_match = sm;
                break;
            }

            sm++;
        }
    }

    return sm_match;
}

/*! Try to find an active LE handset state machine for a typed address.

    This function will check both the type (PUBLIC or RANDOM) and the bdaddr
    match the LE address for a handset state machine.

    \param taddr Typed bdaddr to search for.
    \return Pointer to the matching state machine, or NULL if no match.
*/
static handset_service_state_machine_t *handsetService_GetLeSmForTypedBdAddr(const typed_bdaddr *taddr)
{
    handset_service_state_machine_t *sm_match = NULL;
    handset_service_state_machine_t *sm = HandsetService_GetSm();
    uint16 index;

    PanicNull(sm);

    if (!BdaddrTypedIsEmpty(taddr))
    {
        for (index = 0; index < HANDSET_SERVICE_MAX_SM; index++)
        {
            typed_bdaddr le_taddr = sm->le_addr.taddr;

            DEBUG_LOG_VERBOSE("handsetService_GetLeSmForTypedBdAddr Check index [%d] state [%d] type [%d] addr [%04x,%02x,%06lx]",
                          index, sm->state, le_taddr.type, le_taddr.addr.nap, le_taddr.addr.uap, le_taddr.addr.lap);

            if ((sm->state != HANDSET_SERVICE_STATE_NULL)
                && BtDevice_BdaddrTypedIsSame(taddr, &le_taddr))
            {
                sm_match = sm;
                break;
            }

            sm++;
        }
    }

    return sm_match;
}

/*! Try to find an active handset state machine for a tp_bdaddr.

    Note: handset_service currently supports only one handset sm.

    \param tp_addr Address to search for.
    \return Pointer to the matching state machine, or NULL if no match.
*/
static handset_service_state_machine_t *handsetService_GetSmForTpBdAddr(const tp_bdaddr *tp_addr)
{
    handset_service_state_machine_t *sm = NULL;

    DEBUG_LOG("handsetService_GetSmForTpBdAddr transport [%d] type [%d] addr [%04x,%02x,%06lx]",
              tp_addr->transport, tp_addr->taddr.type,
              tp_addr->taddr.addr.nap, tp_addr->taddr.addr.uap, tp_addr->taddr.addr.lap);

    switch (tp_addr->transport)
    {
    case TRANSPORT_BREDR_ACL:
        {
            /* First try to match the BR/EDR address */
            sm = handsetService_GetSmForBredrAddr(&tp_addr->taddr.addr);
            if (!sm)
            {
                /* Second try to match the device_t handle */
                device_t dev = BtDevice_GetDeviceForBdAddr(&tp_addr->taddr.addr);
                if (dev)
                {
                    sm = handsetService_GetSmForDevice(dev);
                }
            }

            /* Third try to match to the LE address. */
            if (!sm)
            {
                sm = handsetService_GetLeSmForTypedBdAddr(&tp_addr->taddr);
            }
        }
        break;

    case TRANSPORT_BLE_ACL:
        {
            /* First try to match the LE address to a sm LE addr */
            sm = handsetService_GetLeSmForTypedBdAddr(&tp_addr->taddr);

            /* Second try to match the bdaddr component to a sm BR/EDR address */
            if (!sm)
            {
                sm = handsetService_GetSmForBredrAddr(&tp_addr->taddr.addr);
            }
        }
        break;

    default:
        HS_LOG("handsetService_GetSmForTpBdAddr Unsupported transport type %d", tp_addr->transport);
    }
    
    return sm;
}

/*! Helper function to get an active handset state machine based on a BR/EDR address.

    Note: handset_service currently supports only one handset sm.

    \param addr Address to search for. Treated as a BR/EDR address.
    \return Pointer to the matching state machine, or NULL if no match.
*/
static handset_service_state_machine_t *handsetService_GetSmForBdAddr(const bdaddr *addr)
{
    tp_bdaddr tp_addr = {0};

    BdaddrTpFromBredrBdaddr(&tp_addr, addr);
    return handsetService_GetSmForTpBdAddr(&tp_addr);
}

/*! \brief Create a new instance of a handset state machine.

    This will return NULL if a new state machine cannot be created,
    for example if the maximum number of handsets already exists.

    \param device Device to create state machine for.

    \return Pointer to new state machine, or NULL if it couldn't be created.
*/
static handset_service_state_machine_t *handsetService_CreateSm(device_t device)
{
    handset_service_state_machine_t *new_sm = NULL;
    handset_service_state_machine_t *sm = HandsetService_GetSm();
    uint16 index;

    for (index = 0; index < HANDSET_SERVICE_MAX_SM; index++)
    {
        if (sm->state == HANDSET_SERVICE_STATE_NULL)
        {
            new_sm = sm;
            HandsetServiceSm_Init(new_sm);
            HandsetServiceSm_SetDevice(new_sm, device);
            BdaddrTpSetEmpty(&new_sm->le_addr);
            HandsetServiceSm_SetState(new_sm, HANDSET_SERVICE_STATE_DISCONNECTED);
            break;
        }
        sm++;
    }

    return new_sm;
}

/*! \brief Create a new instance of a handset state machine for a LE connection.

    This will return NULL if a new state machine cannot be created,
    for example if the maximum number of handsets already exists.

    \param addr Address to create state machine for.

    \return Pointer to new state machine, or NULL if it couldn't be created.
*/
static handset_service_state_machine_t *handsetService_CreateLeSm(const tp_bdaddr *addr)
{
    handset_service_state_machine_t *new_sm = NULL;
    handset_service_state_machine_t *sm = HandsetService_GetSm();
    uint16 index;

    for (index = 0; index < HANDSET_SERVICE_MAX_SM; index++)
    {
        if (sm->state == HANDSET_SERVICE_STATE_NULL)
        {
            new_sm = sm;
            HandsetServiceSm_Init(new_sm);
            new_sm->le_addr = *addr;
            HandsetServiceSm_SetState(new_sm, HANDSET_SERVICE_STATE_DISCONNECTED);
            break;
        }
        sm++;
    }

    return new_sm;
}

/* \brief Check if a new handset is allowed to connect

   This function will check if a new handset should be allowed to connect.

   Currently we do not support more than one handset connected at a time, so
   we must be able to reject or disconnect any other handset that tries to
   connect.

   A handset is considered connected if the BR/EDR ACL is connected.
   For example, if the ACL is connected but the BR/EDR profiles are connecting
   then it is considered to be connected.
*/
bool handsetService_CheckHandsetCanConnect(const bdaddr *addr)
{
    handset_service_state_machine_t *sm = HandsetService_GetSm();
    uint16 index;
    bool can_connect = TRUE;

    for (index = 0; index < HANDSET_SERVICE_MAX_SM; index++)
    {
        if (HandsetServiceSm_IsBredrAclConnected(sm))
        {
            if (!BdaddrIsSame(&sm->handset_addr, addr))
            {
                can_connect = FALSE;
                break;
            }
        }

        sm++;
    }

    return can_connect;
}

/*! \brief Send a HANDSET_SERVICE_INTERNAL_CONNECT_REQ to a state machine. */
static void handsetService_InternalConnectReq(handset_service_state_machine_t *sm, uint8 profiles)
{
    MESSAGE_MAKE(req, HANDSET_SERVICE_INTERNAL_CONNECT_REQ_T);
    req->device = sm->handset_device;
    req->profiles = profiles;
    MessageSend(&sm->task_data, HANDSET_SERVICE_INTERNAL_CONNECT_REQ, req);
}

/*! \brief Send a HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ to a state machine. */
static void handsetService_InternalDisconnectReq(handset_service_state_machine_t *sm, const bdaddr *addr)
{
    MESSAGE_MAKE(req, HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ_T);
    req->addr = *addr;
    MessageSend(&sm->task_data, HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ, req);
}

/*! \brief Send a HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ to a state machine. */
static void handsetService_InternalConnectStopReq(handset_service_state_machine_t *sm)
{
    MESSAGE_MAKE(req, HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ_T);
    req->device = sm->handset_device;
    MessageSend(&sm->task_data, HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ, req);
}

/*! \brief Helper function for starting a connect req. */
static void handsetService_ConnectReq(Task task, const bdaddr *addr, uint8 profiles)
{
    handset_service_state_machine_t *sm = handsetService_GetSmForBdAddr(addr);
    device_t dev = BtDevice_GetDeviceForBdAddr(addr);

    TimestampEvent(TIMESTAMP_EVENT_HANDSET_CONNECTION_START);

    /* If the state machine doesn't exist yet, and we are allowed to connect
       to a new handset, create a new state machine. */
    if (!sm && handsetService_CheckHandsetCanConnect(addr))
    {
        HS_LOG("handsetService_ConnectReq creating new handset sm");
        sm = handsetService_CreateSm(dev);
    }

    if (sm)
    {
        if (!sm->handset_device)
        {
            HandsetServiceSm_SetDevice(sm, dev);
        }
        HandsetServiceSm_CompleteDisconnectRequests(sm, handset_service_status_cancelled);
        handsetService_InternalConnectReq(sm, profiles);
        TaskList_AddTask(&sm->connect_list, task);
    }
    else
    {
        HS_LOG("handsetService_ConnectReq Couldn't create a new handset sm");

        MESSAGE_MAKE(cfm, HANDSET_SERVICE_CONNECT_CFM_T);
        cfm->addr = *addr;
        cfm->status = handset_service_status_failed;
        MessageSend(task, HANDSET_SERVICE_CONNECT_CFM, cfm);
    }
}

/*
    Message handler functions
*/

static void handsetService_HandlePairingActivity(const PAIRING_ACTIVITY_T* pair_activity)
{
    DEBUG_LOG("handsetService_HandlePairingActivity");
    switch (pair_activity->status)
    {
        case pairingInProgress:
            if (!HandsetService_IsPairing())
            {
                HS_LOG("handsetService_HandlePairingActivity. Pairing Active");
                HandsetService_SetPairing(TRUE);
                handsetService_UpdateAdvertisingData();
            }
            break;
        case pairingNotInProgress:
            if (HandsetService_IsPairing())
            {
                HS_LOG("handsetService_HandlePairingActivity. Pairing Inactive");
                HandsetService_SetPairing(FALSE);
                handsetService_UpdateAdvertisingData();
            }
            break;
            
        case pairingSuccess:
            DEBUG_LOG("handsetService_HandlePairingActivity pairingSuccess");
            if(handsetService_GetSmForBdAddr(&pair_activity->device_addr))
            {
                device_t dev = BtDevice_GetDeviceForBdAddr(&pair_activity->device_addr);
            
                DEBUG_LOG("handsetService_HandlePairingActivity SM Found device %p", dev);
                
                if(!dev)
                {
                    DEBUG_LOG("handsetService_HandlePairingActivity Create New Handset Device");
                    dev = PanicNull(BtDevice_GetDeviceCreateIfNew(&pair_activity->device_addr, DEVICE_TYPE_HANDSET));
                    PanicFalse(BtDevice_SetDefaultProperties(dev));
                }
                
                if(BtDevice_GetDeviceType(dev) == DEVICE_TYPE_HANDSET)
                {
                    DEBUG_LOG("handsetService_HandlePairingActivity Synchronise Link Keys");
                    PanicFalse(BtDevice_SetFlags(dev, DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD, DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD));
                    KeySync_Sync();
                }
            }
            break;
        
        default:
            break;
    }
}

/*! \brief Update the state of LE advertising data set select/release operation */
static void handsetService_UpdateLeAdvertisingDataSetState(handset_service_le_adv_data_set_state_t state)
{
    handset_service_data_t *hs = HandsetService_Get();
    
    hs->ble_adv_state = state;
    handsetService_UpdateAdvertisingData();
}

/*! \brief Make message for LE connectable indication and send it to task list */
static void handsetService_SendLeConnectableIndication(bool connectable)
{    
    MESSAGE_MAKE(le_connectable_ind, HANDSET_SERVICE_LE_CONNECTABLE_IND_T);
    le_connectable_ind->status = handset_service_status_success;
    le_connectable_ind->le_connectable = connectable;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), HANDSET_SERVICE_LE_CONNECTABLE_IND, le_connectable_ind);
}

static void handsetService_HandleLeAdvMgrSelectDatasetCfm(const LE_ADV_MGR_SELECT_DATASET_CFM_T *cfm)
{
    HS_LOG("handsetService_HandleLeAdvMgrSelectDatasetCfm, cfm status is %x", cfm->status );
    
    if (cfm->status == le_adv_mgr_status_success)
    {
        handsetService_UpdateLeAdvertisingDataSetState(handset_service_le_adv_data_set_state_selected);
        handsetService_SendLeConnectableIndication(TRUE);
    }
    else
    {
        Panic();
    }

}

static void handsetService_HandleLeAdvMgrReleaseDatasetCfm(const LE_ADV_MGR_RELEASE_DATASET_CFM_T *cfm)
{
    HS_LOG("handsetService_HandleLeAdvMgrReleaseDatasetCfm, cfm status is %x", cfm->status );
    
    if (cfm->status == le_adv_mgr_status_success)
    {
        handsetService_UpdateLeAdvertisingDataSetState(handset_service_le_adv_data_set_state_not_selected);
        handsetService_SendLeConnectableIndication(FALSE);
    }
    else
    {
        Panic();
    }
}

/* \brief Handle a CON_MANAGER_TP_CONNECT_IND for BR/EDR and BLE connections */
static void handsetService_HandleConManagerTpConnectInd(const CON_MANAGER_TP_CONNECT_IND_T *ind)
{
    handset_service_state_machine_t *sm = handsetService_GetSmForTpBdAddr(&ind->tpaddr);

    HS_LOG("handsetService_HandleConManagerTpConnectInd sm [%p] type[%d] addr [%04x,%02x,%06lx]",
           sm, 
           ind->tpaddr.taddr.type, 
           ind->tpaddr.taddr.addr.nap, 
           ind->tpaddr.taddr.addr.uap, 
           ind->tpaddr.taddr.addr.lap);

    if (ind->incoming)
    {
        if (ind->tpaddr.transport == TRANSPORT_BLE_ACL)
        {
            /* Do not initiate pairing unless peer pairing has completed or we can't tell if device is our peer or a handset */
            if (BtDevice_IsPairedWithPeer() && !BtDevice_LeDeviceIsPeer(&ind->tpaddr))
            {
                HS_LOG("handsetService_HandleConManagerTpConnectInd received for LE handset");

#ifdef INCLUDE_BLE_PAIR_HANDSET_ON_CONNECT
                Pairing_PairLeAddress(HandsetService_GetTask(), &ind->tpaddr.taddr);
#endif
                handsetService_SetBleConnected(TRUE);

                handsetService_UpdateAdvertisingData();

                if (sm)
                {
                    HandsetServiceSm_HandleConManagerBleTpConnectInd(sm, ind);
                }
                else
                {
                    sm = handsetService_CreateLeSm(&ind->tpaddr);
                    assert(sm);
                    HandsetServiceSm_HandleConManagerBleTpConnectInd(sm, ind);
                }
            }
        }
        else if (ind->tpaddr.transport == TRANSPORT_BREDR_ACL)
        {
            if (appDeviceIsHandset(&ind->tpaddr.taddr.addr))
            {
                device_t device = BtDevice_GetDeviceForBdAddr(&ind->tpaddr.taddr.addr);

                if (!sm)
                {
                    sm = handsetService_CreateSm(device);
                }

                assert(sm);

                if (!sm->handset_device)
                {
                    HandsetServiceSm_SetDevice(sm, device);
                }

                /* Forward the connection to the state machine. */
                HandsetServiceSm_HandleConManagerBredrTpConnectInd(sm, ind);
            }
        }
    }
}

/* \brief Handle a CON_MANAGER_TP_DISCONNECT_IND for BR/EDR and BLE disconnections */
static void handsetService_HandleConManagerTpDisconnectInd(const CON_MANAGER_TP_DISCONNECT_IND_T *ind)
{
    handset_service_state_machine_t *sm = handsetService_GetSmForTpBdAddr(&ind->tpaddr);

    HS_LOG("handsetService_HandleConManagerTpDisconnectInd sm [%p] type[%d] addr [%04x,%02x,%06lx]",
           sm, 
           ind->tpaddr.taddr.type, 
           ind->tpaddr.taddr.addr.nap,
           ind->tpaddr.taddr.addr.uap,
           ind->tpaddr.taddr.addr.lap);

    if (ind->tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        if (BtDevice_IsPairedWithPeer() && !BtDevice_LeDeviceIsPeer(&ind->tpaddr))
        {
            handsetService_SetBleConnected(FALSE);
        
            handsetService_UpdateAdvertisingData();
    
            if (sm)
            {
                HandsetServiceSm_HandleConManagerBleTpDisconnectInd(sm, ind);
            }
        }
    }
    else if (ind->tpaddr.transport == TRANSPORT_BREDR_ACL)
    {
        if (sm)
        {
            HandsetServiceSm_HandleConManagerBredrTpDisconnectInd(sm, ind);
        }
    }
}

static void handsetService_HandleProfileManagerConnectedInd(const CONNECTED_PROFILE_IND_T *ind)
{
    bdaddr addr = DeviceProperties_GetBdAddr(ind->device);
    bool is_handset = appDeviceIsHandset(&addr);

    HS_LOG("handsetService_HandleProfileManagerConnectedInd device 0x%x profile 0x%x handset %d",
           ind->device, ind->profile, is_handset);

    if (is_handset)
    {
        handset_service_state_machine_t *sm = handsetService_GetSmForBdAddr(&addr);

        /* If state machine doesn't exist yet, need to create a new one */
        if (!sm)
        {
            HS_LOG("handsetService_HandleProfileManagerConnectedInd creating new handset sm");
            sm = handsetService_CreateSm(ind->device);
        }

        /* TBD: handset_service currently only supports one active handset, 
                so what should happen if a new device can't be created? */
        assert(sm);
        
        if (!sm->handset_device)
        {
            HandsetServiceSm_SetDevice(sm, ind->device);
        }

        /* Forward the connect ind to the state machine */
        HandsetServiceSm_HandleProfileManagerConnectedInd(sm, ind);
    }
}

static void handsetService_HandleProfileManagerDisconnectedInd(const DISCONNECTED_PROFILE_IND_T *ind)
{
    bdaddr addr = DeviceProperties_GetBdAddr(ind->device);
    bool is_handset = appDeviceIsHandset(&addr);

    HS_LOG("handsetService_HandleProfileManagerDisconnectedInd device 0x%x profile 0x%x handset %d",
           ind->device, ind->profile, is_handset);

    if (is_handset)
    {
        handset_service_state_machine_t *sm = handsetService_GetSmForDevice(ind->device);

        if (sm)
        {
            /* Forward the disconnect ind to the state machine */
            HandsetServiceSm_HandleProfileManagerDisconnectedInd(sm, ind);
        }
    }
}

static void handsetService_HandlePairingPairCfm(void)
{
    DEBUG_LOG("handsetService_HandlePairingPairCfm");
}

static void handsetService_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
    /* Connection Manager messages */
    case CON_MANAGER_TP_CONNECT_IND:
        handsetService_HandleConManagerTpConnectInd((const CON_MANAGER_TP_CONNECT_IND_T *)message);
        break;

    case CON_MANAGER_TP_DISCONNECT_IND:
        handsetService_HandleConManagerTpDisconnectInd((const CON_MANAGER_TP_DISCONNECT_IND_T *)message);
        break;

    /* Profile Manager messages */
    case CONNECTED_PROFILE_IND:
        handsetService_HandleProfileManagerConnectedInd((const CONNECTED_PROFILE_IND_T *)message);
        break;

    case DISCONNECTED_PROFILE_IND:
        handsetService_HandleProfileManagerDisconnectedInd((const DISCONNECTED_PROFILE_IND_T *)message);
        break;

    /* BREDR Scan Manager messages */
    case BREDR_SCAN_MANAGER_PAGE_SCAN_PAUSED_IND:
    case BREDR_SCAN_MANAGER_PAGE_SCAN_RESUMED_IND:
        /* These are informational so no need to act on them. */
        break;

    /* Pairing messages */
    case PAIRING_ACTIVITY:
        HS_LOG("handsetService_MessageHandler PAIRING_ACTIVITY id 0x%x", id);
        handsetService_HandlePairingActivity((PAIRING_ACTIVITY_T*)message);
        break;

    case PAIRING_PAIR_CFM:
        HS_LOG("handsetService_MessageHandler PAIRING_PAIR_CFM id 0x%x", id);
        handsetService_HandlePairingPairCfm();
        break;

    /* LE Advertising Manager messages */
    case LE_ADV_MGR_SELECT_DATASET_CFM:
        HS_LOG("handsetService_MessageHandler LE_ADV_MGR_SELECT_DATASET_CFM id 0x%x", id);
        handsetService_HandleLeAdvMgrSelectDatasetCfm((LE_ADV_MGR_SELECT_DATASET_CFM_T*)message);
        break;

    case LE_ADV_MGR_RELEASE_DATASET_CFM:
        HS_LOG("handsetService_MessageHandler LE_ADV_MGR_RELEASE_DATASET_CFM id 0x%x", id);
        handsetService_HandleLeAdvMgrReleaseDatasetCfm((LE_ADV_MGR_RELEASE_DATASET_CFM_T*)message);
        break;

    default:
        HS_LOG("handsetService_MessageHandler unhandled id 0x%x", id);
        break;
    }
}

/*
    Public functions
*/

bool HandsetService_Init(Task task)
{
    handset_service_data_t *hs = HandsetService_Get();
    memset(hs, 0, sizeof(*hs));
    hs->task_data.handler = handsetService_MessageHandler;

    ConManagerRegisterTpConnectionsObserver(cm_transport_all, &hs->task_data);

    ProfileManager_ClientRegister(&hs->task_data);

    Pairing_ActivityClientRegister(&hs->task_data);

    HandsetServiceSm_Init(hs->state_machine);

    TaskList_InitialiseWithCapacity(HandsetService_GetClientList(), HANDSET_SERVICE_CLIENT_LIST_INIT_CAPACITY);

    UNUSED(task);
    return TRUE;
}

void HandsetService_ClientRegister(Task client_task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), client_task);
}

void HandsetService_ClientUnregister(Task client_task)
{
    TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), client_task);
}

void HandsetService_ConnectAddressRequest(Task task, const bdaddr *addr, uint8 profiles)
{
    HS_LOG("HandsetService_ConnectAddressRequest addr [%04x,%02x,%06lx] profiles 0x%x",
            addr->nap, addr->uap, addr->lap, profiles);

    handsetService_ConnectReq(task, addr, profiles);
}

void HandsetService_ConnectMruRequest(Task task, uint8 profiles)
{
    bdaddr hs_addr;

    HS_LOG("HandsetService_ConnectMruRequest profiles 0x%x", profiles);

    if (appDeviceGetHandsetBdAddr(&hs_addr))
    {
        HS_LOG("HandsetService_ConnectMruRequest addr [%04x,%02x,%06lx]",
                hs_addr.nap, hs_addr.uap, hs_addr.lap);

        handsetService_ConnectReq(task, &hs_addr, profiles);
    }
    else
    {
        /* No MRU handset - send CFM immediately */
        MESSAGE_MAKE(cfm, HANDSET_SERVICE_CONNECT_CFM_T);
        BdaddrSetZero(&cfm->addr);
        cfm->status = handset_service_status_no_mru;
        MessageSend(task, HANDSET_SERVICE_CONNECT_CFM, cfm);
    }
}

void HandsetService_DisconnectTpAddrRequest(Task task, const tp_bdaddr *tp_addr)
{
    HS_LOG("HandsetService_DisconnectTpAddrRequest transport [%d] type [%d] addr [%04x,%02x,%06lx]",
              tp_addr->transport, tp_addr->taddr.type,
              tp_addr->taddr.addr.nap, tp_addr->taddr.addr.uap, tp_addr->taddr.addr.lap);

    handset_service_state_machine_t *sm = handsetService_GetSmForTpBdAddr(tp_addr);

    if (sm)
    {
        HandsetServiceSm_CompleteConnectRequests(sm, handset_service_status_cancelled);
        handsetService_InternalDisconnectReq(sm, &tp_addr->taddr.addr);
        TaskList_AddTask(&sm->disconnect_list, task);
    }
    else
    {
        HS_LOG("HandsetService_DisconnectTpAddrRequest sm not found");

        MESSAGE_MAKE(cfm, HANDSET_SERVICE_DISCONNECT_CFM_T);
        cfm->addr = tp_addr->taddr.addr;
        cfm->status = handset_service_status_success;
        MessageSend(task, HANDSET_SERVICE_DISCONNECT_CFM, cfm);
    }
}

void HandsetService_DisconnectRequest(Task task, const bdaddr *addr)
{
    HS_LOG("HandsetService_DisconnectRequest addr [%04x,%02x,%06lx]",
            addr->nap, addr->uap, addr->lap);

    tp_bdaddr tp_addr = {0};

    BdaddrTpFromBredrBdaddr(&tp_addr, addr);
    HandsetService_DisconnectTpAddrRequest(task, &tp_addr);
}

void HandsetService_StopConnect(Task task, const bdaddr *addr)
{
    handset_service_state_machine_t *sm = handsetService_GetSmForBdAddr(addr);

    if (sm)
    {
        if(sm->connect_stop_task)
        {
            if(sm->connect_stop_task != task)
            {
                DEBUG_LOG_ERROR("HandsetService_StopConnect Called by two Tasks: task %p connect_stop_task %p", task, sm->connect_stop_task);
                Panic();
            }
            else
            {
                DEBUG_LOG_WARN("HandsetService_StopConnect called twice; task %p", task);
            }
        }
        handsetService_InternalConnectStopReq(sm);
        sm->connect_stop_task = task;

        /* Flush any queued internal connect requests */
        HandsetServiceSm_CancelInternalConnectRequests(sm);
    }
    else
    {
        HS_LOG("HandsetService_StopConnect no handset connection to stop");

        MESSAGE_MAKE(cfm, HANDSET_SERVICE_CONNECT_STOP_CFM_T);
        cfm->addr = *addr;
        cfm->status = handset_service_status_disconnected;
        MessageSend(task, HANDSET_SERVICE_CONNECT_STOP_CFM, cfm);
    }
}

void HandsetService_ConnectableRequest(Task task)
{
    HS_LOG("HandsetService_ConnectableRequest");
    UNUSED(task);

    HandsetService_SetBleConnectable(TRUE);

    BredrScanManager_PageScanRequest(HandsetService_GetTask(), SCAN_MAN_PARAMS_TYPE_SLOW);
}

void HandsetService_CancelConnectableRequest(Task task)
{
    HS_LOG("HandsetService_CancelConnectableRequest");
    UNUSED(task);

    /* HandsetService_SetBleConnectable(FALSE) is not called here because
       the handset service continues BLE advertising even if BREDR is set to be
       not connectable. */

    BredrScanManager_PageScanRelease(HandsetService_GetTask());
}

void HandsetService_SendConnectedIndNotification(device_t device,
    uint8 profiles_connected)
{
    MESSAGE_MAKE(ind, HANDSET_SERVICE_CONNECTED_IND_T);
    ind->addr = DeviceProperties_GetBdAddr(device);
    ind->profiles_connected = profiles_connected;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), HANDSET_SERVICE_CONNECTED_IND, ind);
}

void HandsetService_SendDisconnectedIndNotification(const bdaddr *addr,
    handset_service_status_t status)
{
    MESSAGE_MAKE(ind, HANDSET_SERVICE_DISCONNECTED_IND_T);
    ind->addr = *addr;
    ind->status = status;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(HandsetService_GetClientList()), HANDSET_SERVICE_DISCONNECTED_IND, ind);
}

bool HandsetService_Connected(device_t device)
{
    handset_service_state_machine_t *sm = handsetService_GetSmForDevice(device);

    if (sm)
    {
        return (HANDSET_SERVICE_STATE_CONNECTED_BREDR == sm->state);
    }

    return FALSE;
}

bool HandsetService_IsBredrConnected(const bdaddr *addr)
{
    handset_service_state_machine_t *sm = handsetService_GetSmForBredrAddr(addr);
    bool bredr_connected = FALSE;

    if (sm)
    {
        bredr_connected = HandsetServiceSm_IsBredrAclConnected(sm);
    }

    HS_LOG("HandsetService_IsBredrConnected bredr_connected %u", bredr_connected);

    return bredr_connected;
}

bool HandsetService_GetConnectedLeHandsetTpAddress(tp_bdaddr *tp_addr)
{
    handset_service_state_machine_t *sm = HandsetService_GetSm();
    uint16 index;
    bool le_handset = FALSE;

    PanicNull(tp_addr);
    for (index = 0; index < HANDSET_SERVICE_MAX_SM; index++)
    {
        if (sm->state != HANDSET_SERVICE_STATE_NULL)
        {
            if (HandsetServiceSm_IsLeConnected(sm))
            {
                tp_addr->taddr = HandsetServiceSm_GetLeTpBdaddr(sm).taddr;
                tp_addr->transport = HandsetServiceSm_GetLeTpBdaddr(sm).transport;
                le_handset = TRUE;
                break;
            }
        }
        sm++;
    }

    return le_handset;
}

void HandsetService_SetBleConnectable(bool connectable)
{
    handset_service_data_t *hs = HandsetService_Get();

    DEBUG_LOG("HandsetService_SetBleConnectable connectable %d le_connectable %d adv_hdl %p",
              connectable, hs->ble_connectable, hs->le_advert_handle);

    if (connectable == hs->ble_connectable)
    {
        /* We still need to send a HANDSET_SERVICE_LE_CONNECTABLE_IND when the
           requested value is the same as the current value */
        handsetService_SendLeConnectableIndication(connectable);
    }
    else
    {
        /* Connectable flag has changed so may need to update the advertising
           data to match the new value. */
        hs->ble_connectable = connectable;
        if(!handsetService_UpdateAdvertisingData())
        {
            /* Advertisement data was not updated. Possibly device is already connected
               over LE and there is no active advertisement set.
               Send the indication to inform the client. */
            handsetService_SendLeConnectableIndication(connectable);
        }
    }
}
