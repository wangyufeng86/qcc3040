/*!
\copyright  Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_init.c
\brief      Initialisation module
*/

#include "app_task.h"
#include "earbud_init.h"
#include "earbud_common_rules.h"
#include "earbud_config.h"
#include "earbud_led.h"
#include "earbud_ui_config.h"
#ifdef INCLUDE_FAST_PAIR
#include "earbud_config.h"
#endif
#include "authentication.h"
#include "adk_log.h"
#include "unexpected_message.h"
#include "earbud_test.h"
#include "proximity.h"
#include "temperature.h"
#include "touch.h"
#include "handset_service.h"
#include "pairing.h"
#include "power_manager.h"
#include "earbud_ui.h"
#include "earbud_sm.h"
#include "earbud_handover.h"
#include "voice_ui.h"
#ifdef ENABLE_AUDIO_TUNING_MODE
#include "voice_audio_tuning_mode.h"
#endif
#include "gaia_framework.h"
#include "earbud_gaia_plugin.h"
#include "earbud_gaia_tws.h"
#include "upgrade_gaia_plugin.h"
#include "gatt_handler.h"
#include "gatt_connect.h"
#include "gatt_server_battery.h"
#include "gatt_server_gatt.h"
#include "gatt_server_gap.h"
#include "device_upgrade.h"
#ifdef INCLUDE_DFU_PEER
#include "device_upgrade_peer.h"
#endif
#include "gaia.h"
#include "connection_manager_config.h"
#include "device_db_serialiser.h"
#include "bt_device_class.h"
#include "le_advertising_manager.h"
#include "le_scan_manager.h"
#include "link_policy.h"
#include "logical_input_switch.h"
#include "input_event_manager.h"
#include "pio_monitor.h"
#include "local_addr.h"
#include "local_name.h"
#include "connection_message_dispatcher.h"
#include "ui.h"
#include "volume_messages.h"
#include "volume_service.h"
#include "media_player.h"
#include "telephony_service.h"
#include "audio_sources.h"
#include "voice_sources.h"
#include "peer_link_keys.h"
#include "peer_ui.h"
#include "telephony_messages.h"
#include "anc_state_manager.h"
#include "aec_leakthrough.h"
#include "audio_curation.h"
#include "anc_gaia_plugin.h"

#ifdef INCLUDE_AMA
#include "ama.h"
#endif
#ifdef INCLUDE_AMA_LE
#include "gatt_server_ama.h"
#endif
#if defined(HAVE_9_BUTTONS)
#include "9_buttons.h"
#elif defined(HAVE_7_BUTTONS)
#include "7_buttons.h"
#elif defined(HAVE_6_BUTTONS)
#include "6_buttons.h"
#elif defined(HAVE_4_BUTTONS)
#include "4_buttons.h"
#elif defined(HAVE_2_BUTTONS)
#include "2_button.h"
#elif defined(HAVE_1_BUTTON)
#include "1_button.h"
#endif

#include <init.h>
#include <bredr_scan_manager.h>
#include <connection_manager.h>
#include <device_list.h>
#include <hfp_profile.h>
#include <scofwd_profile.h>
#include <handover_profile.h>
#include <mirror_profile.h>
#include <charger_monitor.h>
#include <message_broker.h>
#include <profile_manager.h>
#include <state_proxy.h>
#include <peer_signalling.h>
#include <tws_topology.h>
#include <peer_find_role.h>
#include <peer_pair_le.h>
#include <key_sync.h>
#include <ui_prompts.h>
#include <ui_tones.h>
#include <ui_leds.h>
#include <fast_pair.h>
#include <tx_power.h>
#include <multidevice.h>
#ifdef QCC5141_FF_HYBRID_ANC_AA
#ifdef HW_VERSION_YE134
#include <pio_common.h>
#include <microphones_config.h>
#endif
#endif

#ifdef INCLUDE_QCOM_CON_MANAGER
#include <qualcomm_connection_manager.h>
#endif

#include <panic.h>
#include <pio.h>
#include <stdio.h>
#include <feature.h>

#ifdef INCLUDE_GAA
#include <gaa.h>
#endif

#ifdef INCLUDE_GAA_LE
#include <gatt_server_gaa_comm.h>
#include <gatt_server_ams_proxy.h>
#include <gatt_server_ancs_proxy.h>
#include <gatt_server_gaa_media.h>
#endif

#ifdef INCLUDE_GATT_SERVICE_DISCOVERY
#include <gatt_client.h>
#include <gatt_service_discovery.h>
#endif
#include <device_test_service.h>

#ifdef INCLUDE_ACCESSORY
#include "accessory.h"
#include "accessory_tws.h"
#include "request_app_launch.h"
#include "rtt.h"
#endif
#include "earbud_setup_audio.h"
#include "earbud_setup_hfp.h"
#include "earbud_setup_unexpected_message.h"
#include "earbud_setup_fast_pair.h"

/*!< Structure used while initialising */
initData    app_init;

#ifdef INCLUDE_MIRRORING

#define IS_ACL_DISCONNECT_FOR_BLE(flags) (DM_ACL_FLAG_ULP & flags)

/*! \brief This function checks the address received in CL_DM_ACL_CLOSED_IND and
    if the address belongs to the local device it changes it to the address of
    the peer device.

    Rarely, if dynamic handover fails, the stack can send a CL_DM_ACL_CLOSED_IND
    to the application with the local device's BR/EDR BD_ADDR, instead of the
    peer earbud's BR/EDR BD_ADDR. This occurs due to the handling of address
    swapping during handover. To work-around this issue, this function changes
    any CL_DM_ACL_CLOSED_IND containing this device's BR/EDR BD_ADDR to the
    address of the peer earbud's BR/EDR BD_ADDR. This means the disconnection is
    handled correctly in the application.

    \param[in] pointer to CL_DM_ACL_CLOSED_IND_T message
*/
static bool appValidateAddressInDisconnetInd(CL_DM_ACL_CLOSED_IND_T *ind)
{
    bdaddr my_addr;
    bool ret_val=FALSE;

    BdaddrSetZero(&my_addr);

    /* Check if the received address is public */
    if(ind->taddr.type == TYPED_BDADDR_PUBLIC &&
        !IS_ACL_DISCONNECT_FOR_BLE(ind->flags) &&
        appDeviceGetMyBdAddr(&my_addr))
    {
        /* Check if the address received in CL_DM_ACL_CLOSED_IND_T message and
         * local device address is same
         */
        if(BdaddrIsSame(&my_addr, &ind->taddr.addr))
        {
            /* If the address received is same as local device address then update it
             * to peer device address */
            if(appDeviceGetPeerBdAddr(&ind->taddr.addr))
            {
                ret_val=TRUE;
                DEBUG_LOG_VERBOSE("appValidateAddressInDisconnetInd, Address in CL_DM_ACL_CLOSED_IND updated to addr %04x,%02x,%06lx",
                          ind->taddr.addr.nap,
                          ind->taddr.addr.uap,
                          ind->taddr.addr.lap);
            }
        }
    }

    return ret_val;
}
#endif


static bool appPioInit(Task init_task)
{
#ifndef USE_BDADDR_FOR_LEFT_RIGHT
    /* Make PIO2 an input with pull-up */
    PioSetMapPins32Bank(0, 1UL << appConfigHandednessPio(), 1UL << appConfigHandednessPio());
    PioSetDir32Bank(0, 1UL << appConfigHandednessPio(), 0);
    PioSet32Bank(0, 1UL << appConfigHandednessPio(), 1UL << appConfigHandednessPio());
    DEBUG_LOG_INFO("appPioInit, left %d, right %d", appConfigIsLeft(), appConfigIsRight());

    Multidevice_SetType(multidevice_type_pair);
    Multidevice_SetSide(appConfigIsLeft() ? multidevice_side_left : multidevice_side_right);
#endif
#ifdef QCC5141_FF_HYBRID_ANC_AA
#ifdef HW_VERSION_YE134
    /* Config BIAS Pins active low*/
    PioSetActiveLevel(appConfigMic2Pio(), FALSE);
    PioSetActiveLevel(appConfigMic3Pio(), FALSE);
#endif
    /* Make LDO1V8 output high to enable supply for the sensors */
    PioSetMapPins32Bank(0, 1UL << RDP_PIO_LDO1V8, 1UL << RDP_PIO_LDO1V8);
    PioSetDir32Bank(0, 1UL << RDP_PIO_LDO1V8, 1UL << RDP_PIO_LDO1V8);
    PioSet32Bank(0, 1UL << RDP_PIO_LDO1V8, 1UL << RDP_PIO_LDO1V8);
#endif

    UNUSED(init_task);

    return TRUE;
}

static bool appLicenseCheck(Task init_task)
{
    if (FeatureVerifyLicense(APTX_CLASSIC))
        DEBUG_LOG_VERBOSE("appLicenseCheck: aptX Classic is licensed, aptX A2DP CODEC is enabled");
    else
        DEBUG_LOG_WARN("appLicenseCheck: aptX Classic not licensed, aptX A2DP CODEC is disabled");

    if (FeatureVerifyLicense(APTX_CLASSIC_MONO))
        DEBUG_LOG_VERBOSE("appLicenseCheck: aptX Classic Mono is licensed, aptX TWS+ A2DP CODEC is enabled");
    else
        DEBUG_LOG_WARN("appLicenseCheck: aptX Classic Mono not licensed, aptX TWS+ A2DP CODEC is disabled");

    if (FeatureVerifyLicense(CVC_RECV))
        DEBUG_LOG_VERBOSE("appLicenseCheck: cVc Receive is licensed");
    else
        DEBUG_LOG_WARN("appLicenseCheck: cVc Receive not licensed");

    if (FeatureVerifyLicense(CVC_SEND_HS_1MIC))
        DEBUG_LOG_VERBOSE("appLicenseCheck: cVc Send 1-MIC is licensed");
    else
        DEBUG_LOG_WARN("appLicenseCheck: cVc Send 1-MIC not licensed");

    if (FeatureVerifyLicense(CVC_SEND_HS_2MIC_MO))
        DEBUG_LOG_VERBOSE("appLicenseCheck: cVc Send 2-MIC is licensed");
    else
        DEBUG_LOG_WARN("appLicenseCheck: cVc Send 2-MIC not licensed");

    UNUSED(init_task);
    return TRUE;
}

/*! \brief Forward CL_INIT_CFM message to the init task handler. */
static void appInitFwdClInitCfm(const CL_INIT_CFM_T * cfm)
{
    CL_INIT_CFM_T *copy = PanicUnlessNew(CL_INIT_CFM_T);
    *copy = *cfm;

    MessageSend(Init_GetInitTask(), CL_INIT_CFM, copy);
}

/*! \brief Handle Connection library confirmation message */
static void appInitHandleClInitCfm(const CL_INIT_CFM_T *cfm)
{
    if (cfm->status != success)
        Panic();

    /* Set the class of device to indicate this is a headset */
    ConnectionWriteClassOfDevice(AUDIO_MAJOR_SERV_CLASS | RENDER_MAJOR_SERV_CLASS |
                                 AV_MAJOR_DEVICE_CLASS | HEADSET_MINOR_DEVICE_CLASS
#ifndef INCLUDE_MIRRORING
                                | CAPTURING_MAJOR_SERV_CLASS /* for A2DP SRC */
#endif
                                 );

    /* Allow SDP without security, requires authorisation */
    ConnectionSmSetSecurityLevel(0, 1, ssp_secl4_l0, TRUE, TRUE, FALSE);

    /* Reset security mode config - always turn off debug keys on power on */
    ConnectionSmSecModeConfig(appGetAppTask(), cl_sm_wae_acl_owner_none, FALSE, TRUE);

    appInitFwdClInitCfm(cfm);
}

/*! \brief Connection library Message Handler

    This function is the main message handler for the main application task, every
    message is handled in it's own seperate handler function.  The switch
    statement is broken into seperate blocks to reduce code size, if execution
    reaches the end of the function then it is assumed that the message is
    unhandled.
*/
static void appHandleClMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    DEBUG_LOG_V_VERBOSE("appHandleClMessage called, message id = 0x%x", id);
    /* Handle Connection Library messages that are not sent directly to
       the requestor */
    if (CL_MESSAGE_BASE <= id && id < CL_MESSAGE_TOP)
    {
        bool handled = FALSE;

        if (id == CL_INIT_CFM)
        {
            appInitHandleClInitCfm((const CL_INIT_CFM_T *)message);
            return;
        }

#ifdef INCLUDE_MIRRORING
        if(id == CL_DM_ACL_CLOSED_IND)
        {
            appValidateAddressInDisconnetInd((CL_DM_ACL_CLOSED_IND_T *)message);
        }
#endif

        /* Pass connection library messages in turn to the modules that
           are interested in them.
         */
        handled |= LeScanManager_HandleConnectionLibraryMessages(id, message, handled);
        handled |= PeerPairLe_HandleConnectionLibraryMessages(id, message, handled);
        handled |= Pairing_HandleConnectionLibraryMessages(id, message, handled);
        handled |= ConManagerHandleConnectionLibraryMessages(id, message, handled);
        handled |= appLinkPolicyHandleConnectionLibraryMessages(id, message, handled);
        handled |= appAuthHandleConnectionLibraryMessages(id, message, handled);
        handled |= LeAdvertisingManager_HandleConnectionLibraryMessages(id, message, handled);
        handled |= appTestHandleConnectionLibraryMessages(id, message, handled);
        handled |= PeerFindRole_HandleConnectionLibraryMessages(id, message, handled);
        handled |= LocalAddr_HandleConnectionLibraryMessages(id, message, handled);
        handled |= MirrorProfile_HandleConnectionLibraryMessages(id, message, handled);
#ifdef INCLUDE_FAST_PAIR
        handled |= FastPair_HandleConnectionLibraryMessages(id, message, handled);
#endif
        handled |= BtDevice_HandleConnectionLibraryMessages(id, message, handled);
        handled |= appSmHandleConnectionLibraryMessages(id, message, handled);

        if (handled)
        {
            return;
        }
    }

    DEBUG_LOG_VERBOSE("appHandleClMessage called but unhandled, message id = %d", id);
    UnexpectedMessage_HandleMessage(id);
}

/*! \brief Connection library initialisation */
static bool appConnectionInit(Task init_task)
{
    static const msg_filter filter = {msg_group_acl | msg_group_mode_change};

    ConnectionMessageDispatcher_Init();

    /* Initialise the Connection Manager */
#if defined(APP_SECURE_CONNECTIONS)
    ConnectionInitEx3(ConnectionMessageDispatcher_GetHandler(), &filter, appConfigMaxTrustedDevices(), CONNLIB_OPTIONS_SC_ENABLE);
#else
    ConnectionInitEx3(ConnectionMessageDispatcher_GetHandler(), &filter, appConfigMaxTrustedDevices(), CONNLIB_OPTIONS_NONE);
#endif

    Init_SetInitTask(init_task);

    ConnectionMessageDispatcher_RegisterInitClient(&app_init.task);

    return TRUE;
}

static bool appInitTransportManagerInitFixup(Task init_task)
{
    UNUSED(init_task);

    TransportMgrInit();

    return TRUE;
}

static bool appInit_EarRole(bool left_ear)
{
    if (left_ear)
    {
        return appConfigIsLeft();
    }
    else
    {
        return appConfigIsRight();
    }
}

/*! Peer pairing needs to know which devices are Left/Right.

    This is only knowledge that the application has. The Init is
    the only direct call from the application to peer pairing.

    So use a Fixup function to supply the necessary function to peer
    pairing.
 */
static bool appInitPeerPairLeInitFixup(Task init_task)
{
    return PeerPairLe_Init(init_task, appInit_EarRole);
}


static bool appMessageDispatcherRegister(Task init_task)
{
    Task client = &app_init.task;

    UNUSED(init_task);

    ConnectionMessageDispatcher_RegisterInquiryClient(client);
    ConnectionMessageDispatcher_RegisterCryptoClient(client);
    ConnectionMessageDispatcher_RegisterCsbClient(client);
    ConnectionMessageDispatcher_RegisterLeClient(client);
    ConnectionMessageDispatcher_RegisterTdlClient(client);
    ConnectionMessageDispatcher_RegisterL2capClient(client);
    ConnectionMessageDispatcher_RegisterLocalDeviceClient(client);
    ConnectionMessageDispatcher_RegisterPairingClient(client);
    ConnectionMessageDispatcher_RegisterLinkPolicyClient(client);
    ConnectionMessageDispatcher_RegisterTestClient(client);
    ConnectionMessageDispatcher_RegisterRemoteConnectionClient(client);
    ConnectionMessageDispatcher_RegisterRfcommClient(client);
    ConnectionMessageDispatcher_RegisterScoClient(client);
    ConnectionMessageDispatcher_RegisterSdpClient(client);

    return TRUE;
}

static bool appInitDeviceDbSerialiser(Task init_task)
{
    UNUSED(init_task);

    DeviceDbSerialiser_Init();

    /* Register persistent device data users */
    BtDevice_RegisterPddu();

#ifdef INCLUDE_FAST_PAIR
    FastPair_RegisterPersistentDeviceDataUser();
#endif

    DeviceList_Init(appConfigMaxTrustedDevices());

    DeviceDbSerialiser_Deserialise();

    return TRUE;
}


#ifdef USE_BDADDR_FOR_LEFT_RIGHT
static bool appConfigInit(Task init_task)
{
    /* Get local device address */
    ConnectionReadLocalAddr(init_task);

    return TRUE;
}

static bool appInitHandleClDmLocalBdAddrCfm(Message message)
{
    CL_DM_LOCAL_BD_ADDR_CFM_T *cfm = (CL_DM_LOCAL_BD_ADDR_CFM_T *)message;
    if (cfm->status != success)
        Panic();

    InitGetTaskData()->appInitIsLeft = cfm->bd_addr.lap & 0x01;

    DEBUG_LOG_INFO("appInit, bdaddr %04x:%02x:%06x left %d, right %d",
                    cfm->bd_addr.nap, cfm->bd_addr.uap, cfm->bd_addr.lap, appConfigIsLeft(), appConfigIsRight());

    Multidevice_SetType(multidevice_type_pair);
    Multidevice_SetSide(appConfigIsLeft() ? multidevice_side_left : multidevice_side_right);

    return TRUE;
}
#endif

static const InputActionMessage_t* appInitGetInputActions(uint16* input_actions_dim)
{
    const InputActionMessage_t* input_actions = NULL;
#ifdef HAVE_1_BUTTON
#if defined(INCLUDE_GAA) || defined(INCLUDE_AMA)
    if (appConfigIsRight())
    {
        DEBUG_LOG_VERBOSE("appInitGetInputActions voice_assistant_message_group");
        *input_actions_dim = ARRAY_DIM(voice_assistant_message_group);
        input_actions = voice_assistant_message_group;
    }
    else
#endif /* INCLUDE_GAA || INCLUDE_AMA */
    {
        DEBUG_LOG_VERBOSE("appInitGetInputActions media_message_group");
        *input_actions_dim = ARRAY_DIM(media_message_group);
        input_actions = media_message_group;
    }
#else /* HAVE_1_BUTTON */
    *input_actions_dim = ARRAY_DIM(default_message_group);
    input_actions = default_message_group;
#endif /* HAVE_1_BUTTON */
    return input_actions;
}

static bool appInputEventMangerInit(Task init_task)
{
    const InputActionMessage_t* input_actions = NULL;
    uint16 input_actions_dim = 0;
    UNUSED(init_task);
    input_actions = appInitGetInputActions(&input_actions_dim);
    PanicNull((void*)input_actions);
    InputEventManagerInit(LogicalInputSwitch_GetTask(), input_actions,
                          input_actions_dim, &input_event_config);
    return TRUE;
}

#ifdef INIT_DEBUG
/*! Debug function blocks execution until appInitDebugWait is cleared:
    apps1.fw.env.vars['appInitDebugWait'].set_value(0) */
static bool appInitDebug(Task init_task)
{
    volatile static bool appInitDebugWait = TRUE;
    while(appInitDebugWait);

    UNUSED(init_task);
    return TRUE;
}
#endif

#ifdef INCLUDE_FAST_PAIR
static bool appTxPowerInit(Task init_task)
{
    bool result = TxPower_Init(init_task);
    TxPower_SetTxPowerPathLoss(BOARD_TX_POWER_PATH_LOSS);
    return result;
}
#endif

#ifdef INCLUDE_DFU
static bool earbud_DfuAppRegister(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG_VERBOSE("earbud_DfuAppRegister");

    appUpgradeClientRegister(SmGetTask());

    return TRUE;
}

#if 0
static bool earbud_EarbudGaiaPluginRegister(Task init_task)
{
    DEBUG_LOG_VERBOSE("earbud_EarbudGaiaPluginRegister");

    EarbudGaiaPlugin_Init();
    EarbudGaiaTws_Init(init_task);

    return TRUE;
}
#endif

static bool earbud_UpgradeGaiaPluginRegister(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("earbud_UpgradeGaiaPluginRegister");

    UpgradeGaiaPlugin_Init();

    return TRUE;
}
#endif

#ifdef INCLUDE_DFU_PEER
static bool earbud_PeerDfuAppRegister(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG_VERBOSE("earbud_PeerDfuAppRegister");

    appUpgradePeerClientRegister(SmGetTask());

    return TRUE;
}
#endif

#ifdef ENABLE_ANC
static bool earbud_AncGaiaPluginRegister(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG_VERBOSE("earbud_AncGaiaPluginRegister");

    AncGaiaPlugin_Init();

    return TRUE;
}
#endif

/*! \brief Table of initialisation functions */
static const init_table_entry_t appInitTable[] =
{
#ifdef INIT_DEBUG
    {appInitDebug,          0, NULL},
#endif
    {appPioInit,            0, NULL},
    {PioMonitorInit,        0, NULL},
    {Ui_Init,               0, NULL},
    {appLicenseCheck,       0, NULL},
#ifdef INCLUDE_TEMPERATURE
    {appTemperatureInit,    TEMPERATURE_INIT_CFM, NULL},
#endif
    {appBatteryInit,        MESSAGE_BATTERY_INIT_CFM, NULL},
#ifdef INCLUDE_CHARGER
    {appChargerInit,        0, NULL},
#endif
    {LedManager_Init,       0, NULL},
    {appPowerInit,          APP_POWER_INIT_CFM, NULL},
    {appConnectionInit,     CL_INIT_CFM, NULL},
    {appMessageDispatcherRegister, 0, NULL},
#ifdef USE_BDADDR_FOR_LEFT_RIGHT
    {appConfigInit,         CL_DM_LOCAL_BD_ADDR_CFM, appInitHandleClDmLocalBdAddrCfm},
#endif
    {appInputEventMangerInit, 0, NULL},
    {appPhyStateInit,       PHY_STATE_INIT_CFM, NULL},
    {appLinkPolicyInit,     0, NULL},
    {LocalAddr_Init,        0, NULL},
    {ConManagerInit,        0, NULL},
    {CommonRules_Init,      0, NULL},
    {PrimaryRules_Init,     0, NULL},
    {SecondaryRules_Init,   0, NULL},
    {appInitDeviceDbSerialiser, 0, NULL},
    {appDeviceInit,         CL_DM_LOCAL_BD_ADDR_CFM, appDeviceHandleClDmLocalBdAddrCfm},
    {BredrScanManager_Init, BREDR_SCAN_MANAGER_INIT_CFM, NULL},
    {LocalName_Init, LOCAL_NAME_INIT_CFM, NULL},
    {LeAdvertisingManager_Init,     0, NULL},
    {LeScanManager_Init,     0, NULL},
    {AudioSources_Init,      0, NULL},
    {VoiceSources_Init,      0, NULL},
    {Volume_InitMessages,   0, NULL},
    {VolumeService_Init,    0, NULL},
    {appAvInit,             AV_INIT_CFM, NULL},
    {appPeerSigInit,        PEER_SIG_INIT_CFM, NULL},
    {LogicalInputSwitch_Init,     0, NULL},
    {Pairing_Init,          PAIRING_INIT_CFM, NULL},
    {Telephony_InitMessages, 0, NULL},
    {appHfpInit,            APP_HFP_INIT_CFM, NULL},
    {appKymeraInit,         0, NULL},
#ifdef INCLUDE_SCOFWD
    {ScoFwdInit,            SFWD_INIT_CFM, NULL},
#endif
#ifdef INCLUDE_QCOM_CON_MANAGER
    {QcomConManagerInit,QCOM_CON_MANAGER_INIT_CFM,NULL},
#endif
#ifdef INCLUDE_MIRRORING
    {HandoverProfile_Init,  HANDOVER_PROFILE_INIT_CFM, NULL},
	{MirrorProfile_Init,    MIRROR_PROFILE_INIT_CFM, NULL},
#endif
#ifdef ENABLE_ANC
    {AncStateManager_Init, 0, NULL},
#endif

#ifdef ENABLE_AEC_LEAKTHROUGH
    {AecLeakthrough_Init, 0, NULL},
#endif
    {StateProxy_Init,       0, NULL},
    {MediaPlayer_Init,       0, NULL},
    {appInitTransportManagerInitFixup, 0, NULL},        //! \todo TransportManager does not meet expected init interface
#if defined(INCLUDE_DFU) || defined(ENABLE_ANC)
    {GaiaFramework_Init,           APP_GAIA_INIT_CFM, NULL},   // Gatt needs GAIA

#if 0
    {earbud_EarbudGaiaPluginRegister,           0   , NULL},
#endif
#endif
#ifdef INCLUDE_DFU
    {earbud_UpgradeGaiaPluginRegister,             0, NULL},
#endif
#ifdef ENABLE_ANC
    {earbud_AncGaiaPluginRegister,             0, NULL},
#endif
    {GattConnect_Init,      0, NULL},   // GATT functionality is initialised by calling GattConnect_Init then GattConnect_ServerInitComplete.
    // All GATT Servers MUST be initialised after GattConnect_Init and before GattConnect_ServerInitComplete.
    {GattHandlerInit,      0, NULL},
    {appInitPeerPairLeInitFixup, 0, NULL},
    {KeySync_Init,          0, NULL},
    {ProfileManager_Init,   0, NULL},
    {HandsetService_Init,   0, NULL},
    {PeerFindRole_Init,     0, NULL},
    {TwsTopology_Init,      TWS_TOPOLOGY_INIT_CFM, NULL},
    {PeerLinkKeys_Init,     0, NULL},
#ifdef INCLUDE_GATT_BATTERY_SERVER
    {GattServerBattery_Init,0, NULL},
#endif
#ifdef INCLUDE_GATT_GAIA_SERVER
    {gaiaFrameworkInternal_GattServerInit, 0, NULL},
#endif
    {GattServerGatt_Init,   0, NULL},
    {GattServerGap_Init,    0, NULL},
#ifdef INCLUDE_GAA_LE
    {GattServerGaaMedia_Init, 0, NULL},
    {GattServerGaaComm_Init, 0, NULL},
    {GattServerAmsProxy_Init, 0, NULL},
    {GattServerAncsProxy_Init, 0, NULL},
#endif
#ifdef INCLUDE_AMA_LE
    {GattServerAma_Init, 0, NULL},
#endif
    {appSmInit,             0, NULL},
#ifdef INCLUDE_DFU
    {appUpgradeEarlyInit, 0, NULL},
    {earbud_DfuAppRegister, 0, NULL},
    {appUpgradeInit,        UPGRADE_INIT_CFM, NULL},    // Upgrade wants to start a connection (can be gatt)
#endif
#ifdef INCLUDE_DFU_PEER
    {appDeviceUpgradePeerEarlyInit, 0, NULL},
    {earbud_PeerDfuAppRegister, 0, NULL},
    {appDeviceUpgradePeerInit,  DEVICE_UPGRADE_PEER_INIT_CFM, NULL},
#endif
    {VoiceUi_Init,   0, NULL},
#ifdef ENABLE_AUDIO_TUNING_MODE
    {VoiceAudioTuningMode_Init, 0, NULL},
#endif
    {AudioCuration_Init, 0, NULL},
    {UiPrompts_Init,     0, NULL},
    {UiTones_Init,       0, NULL},
    {UiLeds_Init,        0, NULL},
    {PeerUi_Init,        0, NULL},
    {EarbudUi_Init,      0, NULL},
    {TelephonyService_Init, 0, NULL},

#ifdef INCLUDE_MIRRORING
    {EarbudHandover_Init, 0, NULL},
#endif
#ifdef INCLUDE_FAST_PAIR
    {appTxPowerInit, 0 , NULL},
    {FastPair_Init,         0, NULL},
#endif

#ifdef INCLUDE_GATT_SERVICE_DISCOVERY
    {GattServiceDiscovery_Init, 0, NULL},
#endif
    // All GATT Servers MUST be initialised before GATT initialisation is complete.
    {GattConnect_ServerInitComplete, GATT_CONNECT_SERVER_INIT_COMPLETE_CFM, NULL},

#ifdef INCLUDE_GAA
    {Gaa_Init, 0, NULL},
#endif

#ifdef INCLUDE_DEVICE_TEST_SERVICE
    {DeviceTestService_Init, 0, NULL},
#endif

#ifdef INCLUDE_ACCESSORY
    {Accessory_Init, 0, NULL},
    {Accessory_tws_Init, 0, NULL},
    {AccessoryFeature_RequestAppLaunchInit, 0, NULL},
    {Rtt_Init, 0, NULL},
#endif

#ifdef INCLUDE_AMA
    {Ama_Init, 0, NULL},
#endif
};


void appInit(void)
{
    unsigned registrations_array_dim;

    app_init.task.handler = appHandleClMessage;

    registrations_array_dim = (unsigned)message_broker_group_registrations_end -
                              (unsigned)message_broker_group_registrations_begin;
    PanicFalse((registrations_array_dim % sizeof(message_broker_group_registration_t)) == 0);
    registrations_array_dim /= sizeof(message_broker_group_registration_t);

    MessageBroker_Init(message_broker_group_registrations_begin,
                       registrations_array_dim);

    LedManager_SetHwConfig(&earbud_led_config);

    /* Initialise input event manager with auto-generated tables for
     * the target platform. Connect to the logical Input Switch component */
    Earbud_SetBundlesConfig();

    AppsCommon_StartInit(appGetAppTask(), appInitTable, ARRAY_DIM(appInitTable));
}

void appInitSetInitialised(void)
{
    const ui_config_table_content_t* config_table;
    unsigned config_table_size;

    config_table = EarbudUi_GetConfigTable(&config_table_size);
    Ui_SetConfigurationTable(config_table, config_table_size);

    LogicalInputSwitch_SetLogicalInputIdRange(MIN_INPUT_ACTION_MESSAGE_ID,
                                              MAX_INPUT_ACTION_MESSAGE_ID);
#ifdef INCLUDE_CAPSENSE
    /* initialize capsense library */
    const touchEventConfig *touch_events;
    touch_events = EarbudUi_GetCapsenseEventTable(&config_table_size);
    touchSensorClientRegister(LogicalInputSwitch_GetTask(), config_table_size, touch_events);
#endif
    /* UI and App is completely initialized, system is ready for inputs */
    PioMonitorEnable();

    /* complete power manager initialisation to enable sleep */
    appPowerInitComplete();

    appLinkPolicyRegisterTestTask(&testTask);

    Earbud_SetupUnexpectedMessage();

    Earbud_SetupHfp();

    Earbud_SetupAudio();

#ifdef INCLUDE_FAST_PAIR
    Earbud_SetupFastPair();
#endif

#ifdef INCLUDE_DFU
    DEBUG_LOG_VERBOSE("Registration of SmGetTask() with GAIA");

    GaiaFrameworkInternal_ClientRegister(SmGetTask());
#endif

    Init_SetCompleted();
}
