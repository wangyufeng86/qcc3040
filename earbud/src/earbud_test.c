/*!
\copyright  Copyright (c) 2017 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of specifc application testing functions
*/

#include "earbud_test.h"

#include "init.h"
#include "adk_log.h"
#include "earbud_config.h"
#include "pairing.h"
#include "power_manager.h"
#include "earbud_sm.h"
#include "scofwd_profile_config.h"
#include "battery_monitor_config.h"
#include "charger_monitor_config.h"
#include "kymera_config.h"
#include "kymera_cvc.h"
#include "temperature_config.h"
#include "peripherals/thermistor.h"
#include "ui.h"
#include "peer_link_keys.h"
#include "microphones.h"
#include "volume_messages.h"
#include "device_db_serialiser.h"
#include "audio_sources.h"
#include "voice_sources.h"
#include "gaia_framework_feature.h"
#include "gaia_features.h"
#include "anc_gaia_plugin.h"

#include "anc_state_manager.h"

#include "voice_ui_container.h"
#include "voice_audio_manager.h"

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

#include <device_properties.h>
#include <device_list.h>
#include <device.h>
#include <hfp_profile.h>
#include <scofwd_profile.h>
#include <handover_profile.h>
#include <battery_monitor.h>
#include <av.h>
#include <av_config.h>
#include <logical_input_switch.h>
#include <profile_manager.h>
#include <peer_find_role_private.h>
#include <peer_pair_le_private.h>
#include <peer_signalling.h>
#include <handset_service.h>
#include <connection_manager.h>
#include <connection.h>
#include <mirror_profile.h>
#include <bredr_scan_manager.h>
#include <device_test_service_test.h>
#include <bredr_scan_manager.h>
#include <tws_topology_private.h>
#include <hdma_utils.h>


#include <cryptovm.h>
#include <ps.h>
#include <boot.h>
#include <feature.h>
#include <panic.h>
#include <stdio.h>
#include <bdaddr.h>

#ifdef INCLUDE_ACCESSORY
#include "accessory.h"
#include "request_app_launch.h"
#endif

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

typedef enum
{
    WAIT_FOR_IDLE_THEN_BRING_OUT_OF_CASE,
} delayed_test_event_t;

static void delayedInternalTestEventHandler(Task task, MessageId id, Message msg);
static const TaskData delayedInternalTestEventHandlerTask = { delayedInternalTestEventHandler };

static const va_audio_encode_config_t va_encode_config_table[] =
{
    {.encoder = va_audio_codec_sbc, .encoder_params.sbc =
        {
            .bitpool_size = 24,
            .block_length = 16,
            .number_of_subbands = 8,
            .allocation_method = sbc_encoder_allocation_method_loudness,
        }
    },
    {.encoder = va_audio_codec_msbc, .encoder_params.msbc = {.bitpool_size = 24}},
    {.encoder = va_audio_codec_opus, .encoder_params.opus = {.frame_size = 40}},
};

static const struct
{
    va_wuw_engine_t engine;
    unsigned        capture_ts_based_on_wuw_start_ts:1;
    uint16          max_pre_roll_in_ms;
    uint16          pre_roll_on_capture_in_ms;
    const char     *model;
} wuw_detection_start_table[] =
{
    {va_wuw_engine_qva, TRUE, 2000, 500, "tfd_0.bin"}
};

static void testTaskHandler(Task task, MessageId id, Message message);
TaskData testTask = {testTaskHandler};

static struct
{
    uint16 testcase;
    uint16 iteration;
    uint16 last_marker;
} test_support = {0};

static struct
{
    unsigned         start_capture_on_detection:1;
    va_audio_codec_t encoder_for_capture_on_detection;
    va_wuw_engine_t  wuw_engine;
} va_config = {0};

static bool earbudTest_PopulateVaEncodeConfig(va_audio_codec_t encoder, va_audio_encode_config_t *config)
{
    bool status = FALSE;
    unsigned i;

    for(i = 0; i < ARRAY_DIM(va_encode_config_table); i++)
    {
        if (va_encode_config_table[i].encoder == encoder)
        {
            *config = va_encode_config_table[i];
            status = TRUE;
            break;
        }
    }

    return status;
}

static bool earbudTest_PopulateWuwDetectionStartParams(va_wuw_engine_t engine, va_audio_wuw_detection_params_t *params)
{
    bool status = FALSE;
    unsigned i;

    params->mic_config.sample_rate = 16000;

    for(i = 0; i < ARRAY_DIM(wuw_detection_start_table); i++)
    {
        if (wuw_detection_start_table[i].engine == engine)
        {
            params->wuw_config.engine = wuw_detection_start_table[i].engine;
            params->wuw_config.model = FileFind(FILE_ROOT, wuw_detection_start_table[i].model, strlen(wuw_detection_start_table[i].model));
            params->max_pre_roll_in_ms = wuw_detection_start_table[i].max_pre_roll_in_ms;
            if (params->wuw_config.model != FILE_NONE)
            {
                status = TRUE;
            }
            break;
        }
    }

    return status;
}

static bool earbudTest_PopulateStartCaptureTimeStamp(const va_audio_wuw_detection_info_t *wuw_info, uint32 *timestamp)
{
    bool status = FALSE;
    unsigned i;

    for(i = 0; i < ARRAY_DIM(wuw_detection_start_table); i++)
    {
        if (wuw_detection_start_table[i].engine == va_config.wuw_engine)
        {
            uint32 pre_roll = wuw_detection_start_table[i].pre_roll_on_capture_in_ms * 1000;

            if (wuw_detection_start_table[i].capture_ts_based_on_wuw_start_ts)
            {
                *timestamp = wuw_info->start_timestamp - pre_roll;
            }
            else
            {
                *timestamp = wuw_info->end_timestamp - pre_roll;
            }

            status = TRUE;
            break;
        }
    }

    return status;
}

static unsigned earbudTest_DropDataInSource(Source source)
{
    SourceDrop(source, SourceSize(source));
    return 0;
}

static va_audio_wuw_detected_response_t earbudTest_WuwDetected(const va_audio_wuw_detection_info_t *wuw_info)
{
    va_audio_wuw_detected_response_t response = {0};

    if (va_config.start_capture_on_detection &&
        earbudTest_PopulateVaEncodeConfig(va_config.encoder_for_capture_on_detection, &response.capture_params.encode_config) &&
        earbudTest_PopulateStartCaptureTimeStamp(wuw_info, &response.capture_params.start_timestamp))
    {
        response.start_capture = TRUE;
        response.capture_callback = earbudTest_DropDataInSource;
    }

    return response;
}

static void delayedInternalTestEventHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);
    UNUSED(msg);
    switch(id)
    {
        case WAIT_FOR_IDLE_THEN_BRING_OUT_OF_CASE:
            if(!appTestIsTopologyIdle())
            {
                MessageSendLater((Task) &delayedInternalTestEventHandlerTask, WAIT_FOR_IDLE_THEN_BRING_OUT_OF_CASE,
                                                     NULL, 100);
            }
            else
            {
                appTestPhyStateOutOfCaseEvent();
            }
            break;
        default:
            break;
    }
}

/*! \brief Handle ACL Mode Changed Event

    This message is received when the link mode of an ACL has changed from sniff
    to active or vice-versa.  Currently the application ignores this message.
*/
static void appTestHandleClDmModeChangeEvent(CL_DM_MODE_CHANGE_EVENT_T *evt)
{
    DEBUG_LOG_VERBOSE("appHandleClDmModeChangeEvent, event %d, %x,%x,%lx", evt->mode, evt->bd_addr.nap, evt->bd_addr.uap, evt->bd_addr.lap);

    if (appConfigScoForwardingEnabled())
    {
        CL_DM_MODE_CHANGE_EVENT_T *fwd = PanicUnlessNew(CL_DM_MODE_CHANGE_EVENT_T);

        /* Support for the Sco Forwarding test command, appTestScoFwdLinkStatus() */
        *fwd = *evt;

        MessageSend(&testTask,CL_DM_MODE_CHANGE_EVENT,fwd);
    }
}

bool appTestHandleConnectionLibraryMessages(MessageId id, Message message,
                                            bool already_handled)
{
    switch(id)
    {
        case CL_DM_MODE_CHANGE_EVENT:
            appTestHandleClDmModeChangeEvent((CL_DM_MODE_CHANGE_EVENT_T *)message);
            return TRUE;
    }

    return already_handled;
}

/*! \brief Returns the current battery voltage
 */
uint16 appTestGetBatteryVoltage(void)
{
    DEBUG_LOG_ALWAYS("appTestGetBatteryVoltage, %u", appBatteryGetVoltage());
    return appBatteryGetVoltage();
}

void appTestSetBatteryVoltage(uint16 new_level)
{
    DEBUG_LOG_ALWAYS("appTestSetBatteryVoltage, %u", new_level);
    appBatteryTestSetFakeLevel(new_level);

}

bool appTestBatteryStateIsOk(void)
{
    return (battery_level_ok == appBatteryGetState());
}

bool appTestBatteryStateIsLow(void)
{
    return (battery_level_low == appBatteryGetState());
}

bool appTestBatteryStateIsCritical(void)
{
    return (battery_level_critical == appBatteryGetState());
}

bool appTestBatteryStateIsTooLow(void)
{
    return (battery_level_too_low == appBatteryGetState());
}

uint32 appTestBatteryFilterResponseTimeMs(void)
{
    return BATTERY_FILTER_LEN * appConfigBatteryReadPeriodMs();
}

#ifdef HAVE_THERMISTOR
uint16 appTestThermistorDegreesCelsiusToMillivolts(int8 temperature)
{
    return appThermistorDegreesCelsiusToMillivolts(temperature);
}
#endif

/*! \brief Put Earbud into Handset Pairing mode
*/
void appTestPairHandset(void)
{
    DEBUG_LOG_ALWAYS("appTestPairHandset");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_sm_pair_handset);
}

/*! \brief Delete all Handset pairing
*/
void appTestDeleteHandset(void)
{
    DEBUG_LOG_ALWAYS("appTestDeleteHandset");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_sm_delete_handsets);
}

/*! \brief Delete Earbud peer pairing
*/
bool appTestDeletePeer(void)
{
    bdaddr bd_addr;

    DEBUG_LOG_ALWAYS("appTestDeletePeer");
    /* Check if we have previously paired with an earbud */
    if (appDeviceGetPeerBdAddr(&bd_addr))
    {
        return appDeviceDelete(&bd_addr);
    }
    else
    {
        DEBUG_LOG_WARN("appTestDeletePeer: NO PEER TO DELETE");
        return FALSE;
    }
}

bool appTestGetPeerAddr(bdaddr *peer_address)
{
    DEBUG_LOG_ALWAYS("appTestGetPeerAddr");
    if (appDeviceGetPeerBdAddr(peer_address))
        return TRUE;
    else
        return FALSE;
}


static bool appTestReadLocalAddr_completed = FALSE;
void appTestReadLocalAddr(void)
{
    DEBUG_LOG_ALWAYS("appTestReadLocalAddr started");

    appTestReadLocalAddr_completed = FALSE;
    ConnectionReadLocalAddr(&testTask);
}


static bdaddr appTestGetLocalAddr_address = {0};
bool appTestGetLocalAddr(bdaddr *addr)
{
    DEBUG_LOG_ALWAYS("appTestGetLocalAddr. Read:%d %04x:%02x:%06x", 
                     appTestReadLocalAddr_completed, 
                     appTestGetLocalAddr_address.nap, appTestGetLocalAddr_address.uap, appTestGetLocalAddr_address.lap);

    *addr = appTestGetLocalAddr_address;
    return appTestReadLocalAddr_completed;
}


/*! \brief Return if Earbud is in a Pairing mode
*/
bool appTestIsPairingInProgress(void)
{
    bool pip = !appPairingIsIdle();

    DEBUG_LOG_ALWAYS("appTestIsPairingInProgress:%d", pip);

    return pip;
}

/*! \brief Initiate Earbud A2DP connection to the Handset
*/
bool appTestHandsetA2dpConnect(void)
{
    bdaddr bd_addr;

    DEBUG_LOG_ALWAYS("appTestHandsetA2dpConnect");
    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        return appAvA2dpConnectRequest(&bd_addr, A2DP_CONNECT_NOFLAGS);
    }
    else
    {
        return FALSE;
    }
}

/*! Used to block the rules for handset pairing */
bool appTestHandsetPairingBlocked = FALSE;

/*! \brief Stop the earbud pairing with a handset
 */
void appTestBlockAutoHandsetPairing(bool block)
{
    bool was_blocked = appTestHandsetPairingBlocked;
    bool paired_already = appTestIsHandsetPaired();

    appTestHandsetPairingBlocked = block;

    DEBUG_LOG_ALWAYS("appTestBlockAutoHandsetPairing(%d) was %d (paired:%d)",
                    block, was_blocked, paired_already);
}

/*! \brief Return if Earbud has a handset pairing
 */
bool appTestIsHandsetPaired(void)
{
    bool paired = BtDevice_IsPairedWithHandset();

    DEBUG_LOG_ALWAYS("appTestIsHandsetPaired:%d",paired);

    return paired;
}

/*! \brief Return if Earbud has an Handset A2DP connection
*/
bool appTestIsHandsetA2dpConnected(void)
{
    bool connected = FALSE;
    bdaddr bd_addr;

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
        connected = theInst && appA2dpIsConnected(theInst);
    }

    DEBUG_LOG_ALWAYS("appTestIsHandsetA2dpConnected:%d", connected);

    /* If we get here then there's no A2DP connected for handset */
    return connected;
}

bool appTestIsHandsetA2dpMediaConnected(void)
{
    bool connected = FALSE;
    bdaddr bd_addr;

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
        connected = theInst && appA2dpIsConnectedMedia(theInst);
    }

    DEBUG_LOG_ALWAYS("appTestIsHandsetA2dpMediaConnected:%d", connected);

    /* If we get here then there's no A2DP connected for handset */
    return connected;
}

/*! \brief Return if Earbud is in A2DP streaming mode with the handset
*/
bool appTestIsHandsetA2dpStreaming(void)
{
    bool streaming = FALSE;
    bdaddr bd_addr;

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
        streaming = theInst && appA2dpIsStreaming(theInst);
    }

    DEBUG_LOG_ALWAYS("appTestIsHandsetA2dpStreaming:%d", streaming);

    /* If we get here then there's no A2DP connected for handset */
    return streaming;
}

bool appTestIsA2dpPlaying(void)
{
    avrcp_play_status play_status = appAvPlayStatus();
    bool playing = play_status == avrcp_play_status_playing;

    DEBUG_LOG_ALWAYS("appTestIsA2dpPlaying:%d, status %u", playing, play_status);

    return playing;
}

/*! \brief Initiate Earbud AVRCP connection to the Handset
*/
bool appTestHandsetAvrcpConnect(void)
{
    bdaddr bd_addr;

    DEBUG_LOG_ALWAYS("appTestHandsetAvrcpConnect");
    if (appDeviceGetHandsetBdAddr(&bd_addr))
        return  appAvAvrcpConnectRequest(NULL, &bd_addr);
    return FALSE;
}

/*! \brief Return if Earbud has an Handset AVRCP connection
*/
bool appTestIsHandsetAvrcpConnected(void)
{
    bool connected = FALSE;
    bdaddr bd_addr;

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
        connected = theInst && appAvrcpIsConnected(theInst);
    }

    DEBUG_LOG_ALWAYS("appTestIsHandsetAvrcpConnected = %d", connected);

    /* If we get here then there's no AVRCP connected for handset */
    return connected;
}

/*! \brief Return if Earbud has an Handset HFP connection
*/
bool appTestIsHandsetHfpConnected(void)
{
    bool connected = appHfpIsConnected();

    DEBUG_LOG_ALWAYS("appTestIsHandsetHfpConnected= %d", connected);

    return connected;
}

/*! \brief Return if Earbud has an Handset HFP SCO connection
*/
bool appTestIsHandsetHfpScoActive(void)
{
    bool active = appHfpIsScoActive();

    DEBUG_LOG_ALWAYS("appTestIsHandsetHfpScoActive:%d", active);

    return active;
}

/*! \brief Initiate Earbud HFP Voice Dial request to the Handset
*/
void appTestHandsetHfpVoiceDial(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpVoiceDial");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_hfp_voice_dial);
}

void appTestHandsetHfpMuteToggle(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpMute");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_hfp_mute_toggle);
}

void appTestHandsetHfpVoiceTransferToAg(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpVoiceTransferToAg");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_hfp_transfer_to_ag);
}

void appTestHandsetHfpVoiceTransferToHeadset(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpVoiceTransferToHeadset");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_hfp_transfer_to_headset);
}

void appTestHandsetHfpCallAccept(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpCallAccept");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_voice_call_accept);
}

void appTestHandsetHfpCallReject(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpCallReject");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_voice_call_reject);
}

void appTestHandsetHfpCallHangup(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpCallHangup");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_voice_call_hang_up);
}

bool appTestHandsetHfpCallLastDialed(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpCallLastDialed");
    if (appHfpIsConnected())
    {
        appHfpCallLastDialed();
        return TRUE;
    }
    else
        return FALSE;
}

void appTestHandsetHfpVolumeDownStart(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpVolumeDownStart");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_hfp_volume_down_start);
}

void appTestHandsetHfpVolumeUpStart(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpVolumeUpStart");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_hfp_volume_up_start);
}

void appTestHandsetHfpVolumeStop(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpVolumeStop");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_hfp_volume_stop);
}

bool appTestHandsetHfpSetScoVolume(uint8 volume)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpScoVolume");
    if(appHfpIsCall() || ScoFwdIsReceiving())
    {
        Volume_SendVoiceSourceVolumeUpdateRequest(voice_source_hfp_1, event_origin_local, volume);
        return TRUE;
    }
    else
        return FALSE;
}

bool appTestIsHandsetHfpMuted(void)
{
    bool muted = appHfpIsMuted();

    DEBUG_LOG_ALWAYS("appTestIsHandsetHfpMuted;%d", muted);

    return muted;
}

bool appTestIsHandsetHfpCall(void)
{
    bool is_call = appHfpIsCall();

    DEBUG_LOG_ALWAYS("appTestIsHandsetHfpCall:%d", is_call);

    return is_call;
}

bool appTestIsHandsetHfpCallIncoming(void)
{
    bool incoming = appHfpIsCallIncoming();

    DEBUG_LOG_ALWAYS("appTestIsHandsetHfpCallIncoming:%d", incoming);

    return incoming;
}

bool appTestIsHandsetHfpCallOutgoing(void)
{
    bool outgoing = appHfpIsCallOutgoing();

    DEBUG_LOG_ALWAYS("appTestIsHandsetHfpCallOutgoing:%d", outgoing);

    return outgoing;
}

/*! \brief Return if Earbud has a connection to the Handset
*/
bool appTestIsHandsetConnected(void)
{
    bool connected = appTestIsHandsetA2dpConnected() ||
                     appTestIsHandsetAvrcpConnected() ||
                     appTestIsHandsetHfpConnected();

    DEBUG_LOG_ALWAYS("appTestIsHandsetConnected = %d", connected);

    return connected;
}

/*! \brief Initiate Earbud A2DP connection to the the Peer
*/
bool appTestPeerA2dpConnect(void)
{
    bdaddr bd_addr;

    DEBUG_LOG_ALWAYS("appTestPeerA2dpConnect");
    if (appDeviceGetPeerBdAddr(&bd_addr))
    {
        return appAvA2dpConnectRequest(&bd_addr, A2DP_CONNECT_NOFLAGS);
    }

    return FALSE;
}

/*! \brief Return if Earbud has a Peer A2DP connection
*/
bool appTestIsPeerA2dpConnected(void)
{
    bool connected = FALSE;
    bdaddr bd_addr;

    if (appDeviceGetPeerBdAddr(&bd_addr))
    {
        /* Find peer AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
        if (theInst)
        {
            connected = appA2dpIsConnected(theInst);
        }
        else
        {
            /* If A2DP is not connected, test if A2DP mirroring is connected instead */
            connected = MirrorProfile_IsConnected();
        }
    }

    DEBUG_LOG_ALWAYS("appTestIsPeerA2dpConnected:%d", connected);

    return connected;
}


/*! \brief Check if Earbud is in A2DP streaming mode with peer Earbud
 */
bool appTestIsPeerA2dpStreaming(void)
{
    bool streaming = FALSE;
    bdaddr bd_addr;

    if (appDeviceGetPeerBdAddr(&bd_addr))
    {
        /* Find peer AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
        if (theInst)
        {
            streaming = appA2dpIsStreaming(theInst);
        }
        else
        {
            /* If A2DP is not streaming, test if A2DP mirroring is streaming instead */
            streaming = MirrorProfile_IsA2dpActive();
        }
    }

    DEBUG_LOG_ALWAYS("appTestIsPeerA2dpStreaming:%d", streaming);

    return streaming;
}


/*! \brief Initiate Earbud AVRCP connection to the the Peer
*/
bool appTestPeerAvrcpConnect(void)
{
    bdaddr bd_addr;

    DEBUG_LOG_ALWAYS("appTestPeerAvrcpConnect");
    if (appDeviceGetPeerBdAddr(&bd_addr))
        return  appAvAvrcpConnectRequest(NULL, &bd_addr);

    return FALSE;
}

/*! \brief Return if Earbud has a Peer AVRCP connection
*/
bool appTestIsPeerAvrcpConnected(void)
{
    bool connected = FALSE;
    bdaddr bd_addr;

    if (appDeviceGetPeerBdAddr(&bd_addr))
    {
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
        if (theInst)
        {
            connected = appAvrcpIsConnected(theInst);
        }
        else
        {
            /* If AVRCP isn't connected, consider a peer signalling connection
               as the equivalent connection */
            connected = appPeerSigIsConnected();
        }
    }

    DEBUG_LOG_ALWAYS("appTestIsPeerAvrcpConnected:%d", connected);

    return connected;
}

/*! \brief Send the AV play/pause toggle command
*/
void appTestAvTogglePlayPause(void)
{
    DEBUG_LOG_ALWAYS("appTestAvTogglePlayPause");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_toggle_play_pause);
}

#ifdef INCLUDE_MIRRORING
#include "hdma.h"
/*! \brief Start dynamic handover procedure */
bool earbudTest_DynamicHandover(void)
{
    DEBUG_LOG_ALWAYS("earbudTest_DynamicHandover");
    return Hdma_ExternalHandoverRequest();
}
#endif

/*! \brief Send the Avrcp pause command to the Handset
*/
void appTestAvPause(void)
{
    DEBUG_LOG_ALWAYS("appTestAvPause");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_pause);
}

/*! \brief Send the Avrcp play command to the Handset
*/
void appTestAvPlay(void)
{
    DEBUG_LOG_ALWAYS("appTestAvPlay");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_play);
}

/*! \brief Send the Avrcp stop command to the Handset
*/
void appTestAvStop(void)
{
    DEBUG_LOG_ALWAYS("appTestAvStop");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_stop_av_connection);
}

/*! \brief Send the Avrcp forward command to the Handset
*/
void appTestAvForward(void)
{
    DEBUG_LOG_ALWAYS("appTestAvForward");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_av_forward);
}

/*! \brief Send the Avrcp backward command to the Handset
*/
void appTestAvBackward(void)
{
    DEBUG_LOG_ALWAYS("appTestAvBackward");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_av_backward);
}

/*! \brief Send the Avrcp fast forward state command to the Handset
*/
void appTestAvFastForwardStart(void)
{
    DEBUG_LOG_ALWAYS("appTestAvFastForwardStart");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_av_fast_forward_start);
}

/*! \brief Send the Avrcp fast forward stop command to the Handset
*/
void appTestAvFastForwardStop(void)
{
    DEBUG_LOG_ALWAYS("appTestAvFastForwardStop");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_fast_forward_stop);
}

/*! \brief Send the Avrcp rewind start command to the Handset
*/
void appTestAvRewindStart(void)
{
    DEBUG_LOG_ALWAYS("appTestAvRewindStart");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_av_rewind_start);
}

/*! \brief Send the Avrcp rewind stop command to the Handset
*/
void appTestAvRewindStop(void)
{
    DEBUG_LOG_ALWAYS("appTestAvRewindStop");
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_rewind_stop);
}

/*! \brief Send the Avrcp volume change command to the Handset
*/
bool appTestAvVolumeChange(int8 step)
{
    DEBUG_LOG_ALWAYS("appTestAvVolumeChange %d", step);
    return appAvVolumeChange(step);
}

/*! \brief Send the Avrcp pause command to the Handset
*/
void appTestAvVolumeSet(uint8 volume)
{
    DEBUG_LOG_ALWAYS("appTestAvVolumeSet %d", volume);
    appAvVolumeSet(volume, NULL);
}

void appTestPowerAllowDormant(bool enable)
{
    powerTaskData *thePower = PowerGetTaskData();

    DEBUG_LOG_ALWAYS("appTestPowerAllowDormant %d", enable);
    thePower->allow_dormant = enable;
}

/*! \brief Test the generation of link kets */
extern void TestLinkkeyGen(void)
{
    bdaddr bd_addr;
    uint16 lk[8];
    uint16 lk_out[8];

    bd_addr.nap = 0x0002;
    bd_addr.uap = 0x5B;
    bd_addr.lap = 0x00FF02;

    lk[0] = 0x9541;
    lk[1] = 0xe6b4;
    lk[2] = 0x6859;
    lk[3] = 0x0791;
    lk[4] = 0x9df9;
    lk[5] = 0x95cd;
    lk[6] = 0x9570;
    lk[7] = 0x814b;

    DEBUG_LOG_ALWAYS("appTestPowerAllowDormant");
    PeerLinkKeys_GenerateKey(&bd_addr, lk, 0x74777332UL, lk_out);

#if 0
    bd_addr.nap = 0x0000;
    bd_addr.uap = 0x74;
    bd_addr.lap = 0x6D7031;

    lk[0] = 0xec02;
    lk[1] = 0x34a3;
    lk[2] = 0x57c8;
    lk[3] = 0xad05;
    lk[4] = 0x3410;
    lk[5] = 0x10a6;
    lk[6] = 0x0a39;
    lk[7] = 0x7d9b;
#endif

    PeerLinkKeys_GenerateKey(&bd_addr, lk, 0x6c656272UL, lk_out);

}

/*! \brief Test the cryptographic key conversion function, producing an H6 key */
extern void TestH6(void)
{
    uint8 key_h7[16] = {0xec,0x02,0x34,0xa3,0x57,0xc8,0xad,0x05,0x34,0x10,0x10,0xa6,0x0a,0x39,0x7d,0x9b};
    //uint32 key_id = 0x6c656272;
    uint32 key_id = 0x7262656c;
    uint8 key_h6[16];

    DEBUG_LOG_ALWAYS("appTestPowerAllowDormant");
    CryptoVmH6(key_h7, key_id, key_h6);
    printf("H6: ");
    for (int h6_i = 0; h6_i < 16; h6_i++)
        printf("%02x ", key_h6[h6_i]);
    printf("\n");
}

/*! \brief report the saved handset information */
extern void appTestHandsetInfo(void)
{
    bdaddr bd_addr;
    DEBUG_LOG_ALWAYS("appTestHandsetInfo");
    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        DEBUG_LOG_VERBOSE("appTestHandsetInfo, bdaddr %04x,%02x,%06lx",
                   bd_addr.nap, bd_addr.uap, bd_addr.lap);
    }
}

#include "adk_log.h"

/*! \brief Simple test function to make sure that the DEBUG_LOG macros
        work */
extern void TestDebug(void)
{
    DEBUG_LOG_ALWAYS("TestDebug (Always): test %d %d", 1, 2);
    DEBUG_LOG_ERROR("TestDebug (Error): test");
}

/*! \brief Generate event that Earbud is now in the case. */
void appTestPhyStateInCaseEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestPhyStateInCaseEvent");
    appPhyStateInCaseEvent();
}

/*! \brief Generate event that Earbud is now out of the case. */
void appTestPhyStateOutOfCaseEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestPhyStateOutOfCaseEvent");
    appPhyStateOutOfCaseEvent();
}

/*! \brief Generate event that Earbud is now in ear. */
void appTestPhyStateInEarEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestPhyStateInEarEvent");
    appPhyStateInEarEvent();
}

/*! \brief Generate event that Earbud is now out of the ear. */
void appTestPhyStateOutOfEarEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestPhyStateOutOfEarEvent");
    appPhyStateOutOfEarEvent();
}

/*! \brief Generate event that Earbud is now moving */
void appTestPhyStateMotionEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestPhyStateMotionEvent");
    appPhyStateMotionEvent();
}

/*! \brief Generate event that Earbud is now not moving. */
void appTestPhyStateNotInMotionEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestPhyStateNotInMotionEvent");
    appPhyStateNotInMotionEvent();
}

bool appTestPhyStateIsInEar(void)
{
    return (PHY_STATE_IN_EAR == appPhyStateGetState());
}


bool appTestPhyStateOutOfEar(void)
{
    return     PHY_STATE_OUT_OF_EAR == appPhyStateGetState()
            || PHY_STATE_OUT_OF_EAR_AT_REST == appPhyStateGetState();
}


bool appTestPhyStateIsInCase(void)
{
    return (PHY_STATE_IN_CASE == appPhyStateGetState());
}

static bool reset_happened = TRUE;

/*! \brief Reset an Earbud to factory defaults.
    Will drop any connections, delete all pairing and reboot.
*/
void appTestFactoryReset(void)
{
    DEBUG_LOG_ALWAYS("appTestFactoryReset");
    reset_happened = FALSE;
    LogicalInputSwitch_SendPassthroughDeviceSpecificLogicalInput(ui_input_factory_reset_request);
}

/*! \brief Determine if a reset has happened
    Will return TRUE only once after a reset.
    All subsequent calls will return FALSE
*/
bool appTestResetHappened(void)
{
    bool result = reset_happened;

    DEBUG_LOG_ALWAYS("appTestResetHappened: %d", result);

    if(reset_happened) {
        reset_happened = FALSE;
    }

    return result;
}

/*! \brief Determine if the earbud has a paired peer earbud.
*/
bool appTestIsPeerPaired(void)
{
    bool paired = BtDevice_IsPairedWithPeer();

    DEBUG_LOG_ALWAYS("appTestIsPeerPaired:%d",paired);

    return paired;
}

void appTestConnectHandset(void)
{
    DEBUG_LOG_ALWAYS("appTestConnectHandset");
    appSmConnectHandset();
}

bool appTestConnectHandsetA2dpMedia(void)
{
    DEBUG_LOG_ALWAYS("appTestConnectHandsetA2dpMedia");
    return appAvConnectHandsetA2dpMedia();
}

bool appTestIsPeerSyncComplete(void)
{
    DEBUG_LOG_ALWAYS("appTestIsPeerSyncComplete:1 ** DEPRECATED **");
    /* test scripts may rely on this, for now just return TRUE so
     * they keep running. */
    return TRUE;
}

static void testTaskHandler(Task task, MessageId id, Message message)
{
    static lp_power_mode   powermode = 42;
    static uint16          interval = (uint16)-1;

    UNUSED(task);

    switch (id)
    {
        case CL_DM_ROLE_CFM:
            {
                const CL_DM_ROLE_CFM_T *cfm = (const CL_DM_ROLE_CFM_T *)message;
                bdaddr  peer;
                tp_bdaddr sink_addr;

                if (  !SinkGetBdAddr(cfm->sink, &sink_addr)
                   || !appDeviceGetPeerBdAddr(&peer))
                {
                    return;
                }

                if (   BdaddrIsSame(&peer, &sink_addr.taddr.addr)
                    && hci_success == cfm->status)
                {
                    if (hci_role_master == cfm->role)
                    {
                        DEBUG_LOG_VERBOSE("SCO FORWARDING LINK IS: MASTER");
                    }
                    else if (hci_role_slave == cfm->role)
                    {
                        DEBUG_LOG_VERBOSE("SCO FORWARDING LINK IS: SLAVE");
                    }
                    else
                    {
                        DEBUG_LOG_VERBOSE("SCO FORWARDING LINK STATE IS A MYSTERY: %d",cfm->role);
                    }
                    DEBUG_LOG_VERBOSE("SCO FORWARDING POWER MODE (cached) IS %d (sniff = %d)",powermode,lp_sniff);
                    DEBUG_LOG_VERBOSE("SCO FORWARDING INTERVAL (cached) IS %d",interval);
                }
            }
            break;

        case CL_DM_MODE_CHANGE_EVENT:
            {
                bdaddr peer;
                const CL_DM_MODE_CHANGE_EVENT_T *mode = (const CL_DM_MODE_CHANGE_EVENT_T *)message;

                if (!appDeviceGetPeerBdAddr(&peer))
                    return;

                if (BdaddrIsSame(&mode->bd_addr, &peer))
                {
                    powermode = mode->mode;
                    interval = mode->interval;
                }
            }
            break;

        case CL_DM_LOCAL_BD_ADDR_CFM:
            {
                const CL_DM_LOCAL_BD_ADDR_CFM_T *addr_cfm = (const CL_DM_LOCAL_BD_ADDR_CFM_T *)message;
                if (addr_cfm->status == hci_success)
                {
                    appTestReadLocalAddr_completed = TRUE;
                    appTestGetLocalAddr_address = addr_cfm->bd_addr;

                    DEBUG_LOG_ALWAYS("appTestReadLocalAddr completed. %04x:%02x:%06x", 
                                     appTestGetLocalAddr_address.nap, 
                                     appTestGetLocalAddr_address.uap, 
                                     appTestGetLocalAddr_address.lap);
                }
                else
                {
                    DEBUG_LOG_WARN("appTestReadLocalAddr failed. Status:%d", 
                                   addr_cfm->status);
                }
            }
            break;
    }
}

void appTestScoFwdLinkStatus(void)
{
    scoFwdTaskData *theScoFwd = GetScoFwd();

    DEBUG_LOG_ALWAYS("appTestScoFwdLinkStatus");

    ConnectionGetRole(&testTask,theScoFwd->link_sink);
}

bool appTestScoFwdConnect(void)
{
    bdaddr peer;
    DEBUG_LOG_ALWAYS("appTestScoFwdConnect");

    if (!appDeviceGetPeerBdAddr(&peer) || !appConfigScoForwardingEnabled())
        return FALSE;

    ScoFwdConnectPeer(&testTask);
    return TRUE;
}

bool appTestScoFwdDisconnect(void)
{
    DEBUG_LOG_ALWAYS("appTestScoFwdDisconnect");

    if (!appConfigScoForwardingEnabled())
        return FALSE;


    ScoFwdDisconnectPeer(&testTask);
    return TRUE;
}

bool appTestPowerOff(void)
{
    DEBUG_LOG_ALWAYS("appTestPowerOff");
    return appPowerOffRequest();
}

/* Remove these once feature_if.h has been updated */
#define APTX_ADAPTIVE (40)
#define APTX_ADAPTIVE_MONO (41)

bool appTestLicenseCheck(void)
{
    const uint8 license_table[7] =
    {
        APTX_CLASSIC, APTX_CLASSIC_MONO,
        CVC_RECV, CVC_SEND_HS_1MIC, CVC_SEND_HS_2MIC_MO,
        APTX_ADAPTIVE_MONO
    };
    const bool license_enabled[7] =
    {
        appConfigAptxEnabled(), appConfigAptxEnabled(),
        TRUE, TRUE, TRUE,
        appConfigAptxAdaptiveEnabled()
    };

    bool licenses_ok = TRUE;

    DEBUG_LOG_ALWAYS("appTestLicenseCheck");
    for (int i = 0; i < ARRAY_DIM(license_table); i++)
    {
        if (license_enabled[i])
        {
            if (!FeatureVerifyLicense(license_table[i]))
            {
                DEBUG_LOG_ALWAYS("appTestLicenseCheck: License for feature %d not valid", license_table[i]);
                licenses_ok = FALSE;
            }
            else
                DEBUG_LOG_ALWAYS("appTestLicenseCheck: License for feature %d valid", license_table[i]);
        }
    }

    return licenses_ok;
}

void appTestConnectAfterPairing(bool enable)
{
    UNUSED(enable);
    DEBUG_LOG_WARN("appTestConnectAfterPairing ** DEPRECATED **");
}

bool appTestScoFwdForceDroppedPackets(unsigned percentage_to_drop, int multiple_packets)
{
#ifdef INCLUDE_SCOFWD_TEST_MODE
    scoFwdTaskData *theScoFwd = GetScoFwd();

    if (!(0 <= percentage_to_drop && percentage_to_drop <= 100))
    {
        return FALSE;
    }
    if (multiple_packets > 100)
    {
        return FALSE;
    }

    theScoFwd->percentage_to_drop = percentage_to_drop;
    theScoFwd->drop_multiple_packets = multiple_packets;
    return TRUE;
#else
    UNUSED(percentage_to_drop);
    UNUSED(multiple_packets);
    return FALSE;
#endif
}

bool appTestMicFwdLocalMic(void)
{
    if (appConfigScoForwardingEnabled() && appConfigMicForwardingEnabled())
    {
        if (ScoFwdIsSending())
        {
            appKymeraScoUseLocalMic();
            return TRUE;
        }
    }
    return FALSE;
}

bool appTestMicFwdRemoteMic(void)
{
    if (appConfigScoForwardingEnabled() && appConfigMicForwardingEnabled())
    {
        if (ScoFwdIsSending())
        {
            appKymeraScoUseRemoteMic();
            return TRUE;
        }
    }
    return FALSE;
}

bool appTestIsPtsMode(void)
{
    return A2dpProfile_IsPtsMode();
}

void appTestSetPtsMode(bool Enabled)
{
    A2dpProfile_SetPtsMode(Enabled);
}

bool appTestPtsSetAsPeer(void)
{
    bdaddr handset_addr;
    bdaddr peer_addr;
    device_t device;
    uint16 flags = 0;
    deviceType type = DEVICE_TYPE_EARBUD;

    if (appDeviceGetHandsetBdAddr(&handset_addr))
    {
        device = BtDevice_GetDeviceForBdAddr(&handset_addr);
        if (device)
        {
            if (appDeviceGetPeerBdAddr(&peer_addr))
            {
                ConnectionAuthSetPriorityDevice(&peer_addr, FALSE);
                ConnectionSmDeleteAuthDevice(&peer_addr);
            }
            Device_SetProperty(device, device_property_type, &type, sizeof(deviceType));
            Device_GetPropertyU16(device, device_property_flags, &flags);
            flags |= DEVICE_FLAGS_SECONDARY_ADDR;
            flags |= DEVICE_FLAGS_IS_PTS;
            Device_SetPropertyU16(device, device_property_flags, flags);
            Device_SetPropertyU8(device, device_property_supported_profiles, DEVICE_PROFILE_A2DP);
            
            DeviceDbSerialiser_Serialise();
            
            appPowerReboot();
            return TRUE;
        }
    }

    DEBUG_LOG_ALWAYS("appTestPtsSetAsPeer, failed");
    return FALSE;
}

/*! \brief Determine if appConfigScoForwardingEnabled is TRUE
*/
bool appTestIsScoFwdIncluded(void)
{
    return appConfigScoForwardingEnabled();
}

/*! \brief Determine if appConfigMicForwardingEnabled is TRUE.
*/
bool appTestIsMicFwdIncluded(void)
{
    return appConfigMicForwardingEnabled();
}

/*! \brief Initiate earbud handset handover */
void appTestInitiateHandover(void)
{
    appSmInitiateHandover();
}

/*! \brief Macro to create function declaration/definition that returns the value
    of the specified configuration.
    \param returntype The type returned by the function.
    \param name Given a configuration named appConfigXYZ(), XYZ should be passed
    to the name argument and a function will be created called appTestConfigXYZ().*/
#define MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(returntype, name) \
returntype appTestConfig##name(void);                   \
returntype appTestConfig##name(void)                    \
{                                                       \
    return appConfig##name();                           \
}

/*! \name Config Accessor functions.
    \note These don't have a public declaration as they are not expected to be
    called within the earbud code - the appConfig functions/macros should be used
    instead.
*/
//!@{
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, BatteryFullyCharged)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, BatteryVoltageOk)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, BatteryVoltageLow)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, BatteryVoltageCritical)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(int8, BatteryChargingTemperatureMax)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(int8, BatteryChargingTemperatureMin)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(int8, BatteryDischargingTemperatureMax)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(int8, BatteryDischargingTemperatureMin)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint32, BatteryReadPeriodMs)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, ChargerTrickleCurrent)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, ChargerPreCurrent)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, ChargerPreFastThresholdVoltage)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, ChargerFastCurrent)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, ChargerTerminationCurrent)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, ChargerTerminationVoltage)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint32, ChargerPreChargeTimeoutMs)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint32, ChargerFastChargeTimeoutMs)
#ifdef INCLUDE_TEMPERATURE
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint32, TemperatureMeasurementIntervalMs)
#endif
//!@}


const uint16 FFA_coefficients[15] = {0xFD66, 0x00C4, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0101, 0xFFE6, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
const uint16 FFB_coefficients[15] = {0xFE66, 0xFF5F, 0xFFC8, 0x0028, 0x0071, 0x001C, 0xFFC9, 0x00E1, 0xFFEC, 0xFF18, 0xFFF2, 0x0028, 0x0015, 0x0027, 0xFFE6};
const uint16 FB_coefficients[15] =  {0x004C, 0xFCE0, 0xFF5E, 0x0118, 0x0094, 0x0016, 0xFFCB, 0x00C6, 0x0100, 0xFDD4, 0xFF4C, 0x0161, 0xFF64, 0xFFFD, 0x0057};
const uint16 ANC0_FLP_A_SHIFT_1 = 8; //LPF
const uint16 ANC0_FLP_A_SHIFT_2 = 9;
const uint16 ANC0_FLP_B_SHIFT_1 = 8;
const uint16 ANC0_FLP_B_SHIFT_2 = 8;
const uint16 ANC1_FLP_A_SHIFT_1 = 5;
const uint16 ANC1_FLP_A_SHIFT_2 = 5;
const uint16 ANC1_FLP_B_SHIFT_1 = 5;
const uint16 ANC1_FLP_B_SHIFT_2 = 5;
const uint16 DC_FILTER_SHIFT = 7;

#include <operator.h>
#include <vmal.h>
#include <micbias.h>
#include <audio_anc.h>
#include <cap_id_prim.h>
#include <opmsg_prim.h>
#include "kymera.h"

extern void appKymeraExternalAmpSetup(void);
extern void appKymeraExternalAmpControl(bool enable);

extern void appTestAncSetFilters(void);
void appTestAncSetFilters(void)
{
    //Source ADC_A = (Source)PanicFalse(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));
    Source ADC_A = (Source)PanicFalse(StreamAudioSource(AUDIO_HARDWARE_DIGITAL_MIC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));

    /* lLad IIRs for both ANC0 and ANC1 */
    PanicFalse(AudioAncFilterIirSet(AUDIO_ANC_INSTANCE_0, AUDIO_ANC_PATH_ID_FFA, 15, FFA_coefficients));
    PanicFalse(AudioAncFilterIirSet(AUDIO_ANC_INSTANCE_0, AUDIO_ANC_PATH_ID_FFB, 15, FFB_coefficients));
    PanicFalse(AudioAncFilterIirSet(AUDIO_ANC_INSTANCE_0, AUDIO_ANC_PATH_ID_FB, 15, FB_coefficients));
    //PanicFalse(AudioAncFilterIirSet(AUDIO_ANC_INSTANCE_1, AUDIO_ANC_PATH_ID_FFA, 15, FFA_coefficients));
    //PanicFalse(AudioAncFilterIirSet(AUDIO_ANC_INSTANCE_1, AUDIO_ANC_PATH_ID_FFB, 15, FFB_coefficients));
    //PanicFalse(AudioAncFilterIirSet(AUDIO_ANC_INSTANCE_1, AUDIO_ANC_PATH_ID_FB, 15, FB_coefficients));

    /* Set LPF filter shifts */
    PanicFalse(AudioAncFilterLpfSet(AUDIO_ANC_INSTANCE_0, AUDIO_ANC_PATH_ID_FFA, ANC0_FLP_A_SHIFT_1, ANC0_FLP_A_SHIFT_2));
    //PanicFalse(AudioAncFilterLpfSet(AUDIO_ANC_INSTANCE_1, AUDIO_ANC_PATH_ID_FFA, ANC1_FLP_A_SHIFT_1, ANC1_FLP_A_SHIFT_2));

    /* Set DC filters */
    PanicFalse(SourceConfigure(ADC_A, STREAM_ANC_FFA_DC_FILTER_ENABLE, 1));
    PanicFalse(SourceConfigure(ADC_A, STREAM_ANC_FFA_DC_FILTER_SHIFT, DC_FILTER_SHIFT));
    //PanicFalse(SourceConfigure(ADC_B, STREAM_ANC_FFA_DC_FILTER_ENABLE, 1));
    //PanicFalse(SourceConfigure(ADC_B, STREAM_ANC_FFA_DC_FILTER_SHIFT, DC_FILTER_SHIFT));

    /* Set LPF gains */
    PanicFalse(SourceConfigure(ADC_A, STREAM_ANC_FFA_GAIN, 128));
    PanicFalse(SourceConfigure(ADC_A, STREAM_ANC_FFA_GAIN_SHIFT, 0));
    //PanicFalse(SourceConfigure(ADC_B, STREAM_ANC_FFA_GAIN, 128));
    //PanicFalse(SourceConfigure(ADC_B, STREAM_ANC_FFA_GAIN_SHIFT, 0));
}

extern void appTestAncSetup(void);
void appTestAncSetup(void)
{
    const uint16 sample_rate = 48000;
    Source adc_a = NULL, adc_b = NULL;
    Sink dac_l = NULL, dac_r = NULL;

    OperatorFrameworkEnable(1);

    if(appConfigAncFeedForwardMic())
    {
        adc_a = Microphones_TurnOnMicrophone(appConfigAncFeedForwardMic(), sample_rate, high_priority_user);
    }
    if(appConfigAncFeedBackMic())
    {
        adc_b = Microphones_TurnOnMicrophone(appConfigAncFeedBackMic(), sample_rate, high_priority_user);
    }
    SourceSynchronise(adc_a, adc_b);

    /* Get the DAC output sinks */
    dac_l = (Sink)PanicFalse(StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));
    PanicFalse(SinkConfigure(dac_l, STREAM_CODEC_OUTPUT_RATE, sample_rate));
    PanicFalse(SinkConfigure(dac_l, STREAM_CODEC_OUTPUT_GAIN, 12));
    dac_r = (Sink)StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
    if(dac_r)
    {
        PanicFalse(SinkConfigure(dac_r, STREAM_CODEC_OUTPUT_RATE, sample_rate));
        PanicFalse(SinkConfigure(dac_r, STREAM_CODEC_OUTPUT_GAIN, 12));
    }

    /* Set DAC gains */
    if(dac_l)
    {
        PanicFalse(SinkConfigure(dac_r, STREAM_CODEC_RAW_OUTPUT_GAIN, 7));
    }
    if(dac_r)
    {
        PanicFalse(SinkConfigure(dac_r, STREAM_CODEC_RAW_OUTPUT_GAIN, 7));
    }

    /* feedforward or feedback with analog mics */
    if(dac_l && adc_a)
    {
        PanicFalse(SourceConfigure(adc_a, STREAM_ANC_INSTANCE, AUDIO_ANC_INSTANCE_0));
    }
    if(dac_r && adc_b)
    {
        PanicFalse(SourceConfigure(adc_b, STREAM_ANC_INSTANCE, AUDIO_ANC_INSTANCE_1));
    }
    if(dac_l && adc_a)
    {
        PanicFalse(SourceConfigure(adc_a, STREAM_ANC_INPUT, AUDIO_ANC_PATH_ID_FFA));
    }
    if(dac_r && adc_b)
    {
        PanicFalse(SourceConfigure(adc_b, STREAM_ANC_INPUT, AUDIO_ANC_PATH_ID_FFA));
    }

    /* Associate DACS */
    if(dac_l && adc_a)
    {
        PanicFalse(SinkConfigure(dac_l, STREAM_ANC_INSTANCE, AUDIO_ANC_INSTANCE_0));
    }
    if(dac_r && adc_b)
    {
        PanicFalse(SinkConfigure(dac_r, STREAM_ANC_INSTANCE, AUDIO_ANC_INSTANCE_1));
    }

    /* Setup ANC filters */
    appTestAncSetFilters();

    /* Turn on ANC */
    PanicFalse(AudioAncStreamEnable(0x9, dac_r && adc_b ? 0x09 : 0x0));

    appKymeraExternalAmpSetup();
    appKymeraExternalAmpControl(TRUE);
}

#define GAIN_DB_TO_Q6N_SF (11146541)
#define GAIN_DB(x)      ((int32)(GAIN_DB_TO_Q6N_SF * (x)))

extern void appTestAudioPassthrough(void);
void appTestAudioPassthrough(void)
{
    const uint16 sample_rate = 48000;
    const int32 initial_gain = GAIN_DB(0);

    OperatorFrameworkEnable(1);

    /* Set up MICs */
    Source ADC_A = Microphones_TurnOnMicrophone(appConfigAncFeedForwardMic(), sample_rate, high_priority_user);
    Source ADC_B = Microphones_TurnOnMicrophone(appConfigAncFeedBackMic(), sample_rate, high_priority_user);
    SourceSynchronise(ADC_A, ADC_B);

    /* Get the DAC output sinks */
    Sink DAC_L = (Sink)PanicFalse(StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));
    PanicFalse(SinkConfigure(DAC_L, STREAM_CODEC_OUTPUT_RATE, sample_rate));
    PanicFalse(SinkConfigure(DAC_L, STREAM_CODEC_OUTPUT_GAIN, 12));
    Sink DAC_R = (Sink)StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
    if (DAC_R)
    {
        PanicFalse(SinkConfigure(DAC_R, STREAM_CODEC_OUTPUT_RATE, sample_rate));
        PanicFalse(SinkConfigure(DAC_R, STREAM_CODEC_OUTPUT_GAIN, 12));
    }

    /* Set DAC gains */
    PanicFalse(SinkConfigure(DAC_L, STREAM_CODEC_RAW_OUTPUT_GAIN, 7));
    if (DAC_R)
        PanicFalse(SinkConfigure(DAC_R, STREAM_CODEC_RAW_OUTPUT_GAIN, 7));

    /* Now create the operator for routing the audio */
    Operator passthrough = (Operator)PanicFalse(VmalOperatorCreate(CAP_ID_BASIC_PASS));

    /* Configure the operators
     * Using the information from CS-312572-UG, Kymera Capability
     * Library User Guide
     */
    uint16 set_gain[] = { OPMSG_COMMON_ID_SET_PARAMS, 1, 1, 1, UINT32_MSW(initial_gain), UINT32_LSW(initial_gain) };
    PanicFalse(VmalOperatorMessage(passthrough, set_gain, 6, NULL, 0));

    /* And connect everything */
    /* ...line_in to the passthrough */
    PanicFalse(StreamConnect(ADC_A, StreamSinkFromOperatorTerminal(passthrough, 0)));
    if (ADC_B && DAC_R)
        PanicFalse(StreamConnect(ADC_B, StreamSinkFromOperatorTerminal(passthrough, 1)));

    /* ...and passthrough to line out */
    PanicFalse(StreamConnect(StreamSourceFromOperatorTerminal(passthrough, 0), DAC_L));
    if (DAC_R)
        PanicFalse(StreamConnect(StreamSourceFromOperatorTerminal(passthrough, 1), DAC_R));

    /* Finally start the operator */
    PanicFalse(OperatorStartMultiple(1, &passthrough, NULL));

    appKymeraExternalAmpSetup();
    appKymeraExternalAmpControl(TRUE);
}



#include <usb_hub.h>

extern void appTestUsbAudioPassthrough(void);
void appTestUsbAudioPassthrough(void)
{
    const uint16 sample_rate = 48000;

    OperatorFrameworkEnable(1);

    /* Attach USB */
    //UsbHubAttach();

    /* Get the DAC output sinks */
    Sink DAC_L = (Sink)PanicFalse(StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));
    PanicFalse(SinkConfigure(DAC_L, STREAM_CODEC_OUTPUT_RATE, sample_rate));
    PanicFalse(SinkConfigure(DAC_L, STREAM_CODEC_OUTPUT_GAIN, 12));
    Sink DAC_R = (Sink)StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
    if (DAC_R)
    {
        PanicFalse(SinkConfigure(DAC_R, STREAM_CODEC_OUTPUT_RATE, sample_rate));
        PanicFalse(SinkConfigure(DAC_R, STREAM_CODEC_OUTPUT_GAIN, 12));
    }

    /* Set DAC gains */
    PanicFalse(SinkConfigure(DAC_L, STREAM_CODEC_RAW_OUTPUT_GAIN, 7));
    if (DAC_R)
        PanicFalse(SinkConfigure(DAC_R, STREAM_CODEC_RAW_OUTPUT_GAIN, 7));

    /* Now create the operator for routing the audio */
    Operator usb_rx = (Operator)PanicFalse(VmalOperatorCreate(CAP_ID_USB_AUDIO_RX));

    /* Configure the operators
     * Using the information from CS-312572-UG, Kymera Capability
     * Library User Guide
     */
    uint16 set_config[] =
    {
        OPMSG_USB_AUDIO_ID_SET_CONNECTION_CONFIG,
        0,                  // data_format
        sample_rate / 25,   // sample_rate
        1,                  // number_of_channels
        2 * 8,              // subframe_size
        2 * 8               // subframe_resolution
    };

    PanicFalse(VmalOperatorMessage(usb_rx, set_config, 6, NULL, 0));

    /* And connect everything */
    /* ...USB sournce to USB Rx operator */
    PanicFalse(StreamConnect(StreamUsbEndPointSource(end_point_iso_in), StreamSinkFromOperatorTerminal(usb_rx, 0)));

    /* ...and USB Rx operator to line out */
    PanicFalse(StreamConnect(StreamSourceFromOperatorTerminal(usb_rx, 0), DAC_L));
    if (DAC_R)
        PanicFalse(StreamConnect(StreamSourceFromOperatorTerminal(usb_rx, 1), DAC_R));

    /* Finally start the operator */
    PanicFalse(OperatorStartMultiple(1, &usb_rx, NULL));

    appKymeraExternalAmpSetup();
    appKymeraExternalAmpControl(TRUE);
}

#define ANC_TUNING_SINK_USB_LEFT      0 /*can be any other backend device. PCM used in this tuning graph*/
#define ANC_TUNING_SINK_MIC1_LEFT     4 /* must be connected to internal ADC. Analog or digital */

extern void appTestAncTuningSetSource(void);
void appTestAncTuningSetSource(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    uint16 set_source_control[8] =
    {
        0x2002, 2,
        6, 0, ANC_TUNING_SINK_MIC1_LEFT,
        8, 0, ANC_TUNING_SINK_USB_LEFT,
    };
    PanicFalse(VmalOperatorMessage(theKymera->anc_tuning,
                                   &set_source_control, SIZEOF_OPERATOR_MESSAGE(set_source_control),
                                   NULL, 0));
}


bool appTestEnterDfuWhenEnteringCase(bool unused)
{
    bool enter_dfu_logical_input_sent = FALSE;

    DEBUG_LOG_ALWAYS("appTestEnterDfuWhenEnteringCase");

    UNUSED(unused);

    if (appPeerSigIsConnected() && appSmIsOutOfCase())
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_DFU, NULL);
        enter_dfu_logical_input_sent = TRUE;
    }

    DEBUG_LOG_ALWAYS("appTestEnterDfuWhenEnteringCase cmd_sent=%d", enter_dfu_logical_input_sent);

    return enter_dfu_logical_input_sent;
}

bool appTestIsInitialisationCompleted(void)
{
    bool completed = Init_IsCompleted();

    DEBUG_LOG_ALWAYS("appTestIsInitialisationCompleted:%d", completed);

    return completed;
}

bool appTestIsPrimary(void)
{
    bool prim = appSmIsPrimary();

    DEBUG_LOG_ALWAYS("appTestIsPrimary: %d",prim);

    return prim;
}

bool appTestIsRight(void)
{
    bool right = appConfigIsRight();

    DEBUG_LOG_ALWAYS("appTestIsRight: %d", right);

    return right;
}

bool appTestIsTopologyRole(app_test_topology_role_enum_t checked_role)
{
    bool role_matches = FALSE;
    tws_topology_role role = TwsTopology_GetRole();

    switch (checked_role)
    {
        case app_test_topology_role_none:
            role_matches = (role == tws_topology_role_none);
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. No role:%d", role_matches);
            break;

        case app_test_topology_role_dfu:
            role_matches = (role == tws_topology_role_dfu);
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. DFU:%d", role_matches);
            break;

        case app_test_topology_role_any_primary:
            role_matches = (role == tws_topology_role_primary);
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. Primary:%d (Acting:%d)",
                            role_matches, TwsTopology_IsActingPrimary());
            break;

        case app_test_topology_role_primary:
            role_matches = TwsTopology_IsFullPrimary();
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. Primary(Full):%d", role_matches);
            break;

        case app_test_topology_role_acting_primary:
            role_matches = TwsTopology_IsActingPrimary();
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. Acting Primary:%d", role_matches);
            break;

        case app_test_topology_role_secondary:
            role_matches = (role == tws_topology_role_secondary);
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. Secondary:%d", role_matches);
            break;

        default:
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. Unsupported role:%d",checked_role);
            Panic();
            break;
    }

    return role_matches;
}


bool appTestIsTopologyIdle(void)
{
    bool idle = (TwsTopology_GetRole() == app_test_topology_role_none);

    DEBUG_LOG_ALWAYS("appTestIsTopologyIdle:%d", idle);

    return idle;
}


bool appTestIsTopologyRunning(void)
{
    bool running = twsTopology_IsRunning();

    DEBUG_LOG_ALWAYS("appTestIsTopologyRunning:%d", running);

    return running;
}


/*! List of application states to be included in specific debug.

    Macros are used as it allows us to define a string for log 
    purposes, and create an entry in a switch statement while
    keeping the two synchronised.
 */
#define NAMED_STATES(APP_S) \
    APP_S(APP_STATE_STARTUP) \
    APP_S(APP_STATE_HANDSET_PAIRING) \
    APP_S(APP_STATE_IN_CASE_IDLE) \
    APP_S(APP_STATE_IN_CASE_DFU) \
    APP_S(APP_STATE_OUT_OF_CASE_IDLE) \
    APP_S(APP_STATE_IN_EAR_IDLE)

/*! Macro to create a hydra logging string for each state */
#define HYD_STRING(_state) HYDRA_LOG_STRING(HLS_STATE_NAME_ ## _state, #_state);
/*! Macro to create a hydra logging string for each state */
#define STATE_CASE(_state) case _state: state_name = HLS_STATE_NAME_ ## _state; break;

bool appTestIsApplicationState(appState checked_state)
{
    bool state_matches;
    appState state = appSmGetState();
    const char *state_name;

    NAMED_STATES(HYD_STRING)

    state_matches = state == checked_state;

    switch (checked_state)
    {
        NAMED_STATES(STATE_CASE)

        default:
            DEBUG_LOG_ALWAYS("appTestIsApplicationState. State:x%x:%d (state is x%x)",
                            checked_state, state_matches, state);
            return state_matches;
    }

    if (state_matches)
    {
        DEBUG_LOG_ALWAYS("appTestIsApplicationState. %s:%d", state_name, state_matches);
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestIsApplicationState. %s:%d (state is x%x)",
                            state_name, state_matches, state);
    }
    return state_matches;
}


bool appTestIsPeerFindRoleRunning(void)
{
    bool running = peer_find_role_is_running();

    DEBUG_LOG_ALWAYS("appTestIsPeerFindRoleRunning:%d", running);

    return running;
}


bool appTestIsPeerPairLeRunning(void)
{
    bool running = PeerPairLeIsRunning();

    DEBUG_LOG_ALWAYS("appTestIsPeerPairLeRunning:%d", running);

    return running;
}


static void earbudTest_ReportProperty(device_t device, device_property_t property)
{
    if (Device_IsPropertySet(device,property))
    {
        uint8 value = 0;
        uint16 u16_value = 0;
        switch (property)
        {
        case device_property_bdaddr:
            {
                bdaddr bd_addr = DeviceProperties_GetBdAddr(device);
                DEBUG_LOG_VERBOSE("\tbdaddr nap %x uap %x lap %x", bd_addr.nap, bd_addr.uap, bd_addr.lap);
            }
            break;
        case device_property_a2dp_volume:
            Device_GetPropertyU8(device, property, &value);
            DEBUG_LOG_VERBOSE("\ta2dp_volume %02x", value);
            break;
        case device_property_hfp_profile:
            Device_GetPropertyU8(device, property, &value);
            DEBUG_LOG_VERBOSE("\thfp_profile %02x", value);
            break;
        case device_property_type:
            Device_GetPropertyU8(device, property, &value);
            DEBUG_LOG_VERBOSE("\ttype %02x", value);
            break;
        case device_property_link_mode:
            Device_GetPropertyU8(device, property, &value);
            DEBUG_LOG_VERBOSE("\tlink_mode %02x", value);
            break;
        case device_property_supported_profiles:
            Device_GetPropertyU8(device, property, &value);
            DEBUG_LOG_VERBOSE("\tsupported_profiles %02x", value);
            break;
        case device_property_last_connected_profiles:
            Device_GetPropertyU8(device, property, &value);
            DEBUG_LOG_VERBOSE("\tlast_connected_profiles %02x", value);
            break;
        case device_property_connected_profiles:
            Device_GetPropertyU8(device, property, &value);
            DEBUG_LOG_VERBOSE("\tcurrently connected_profiles %02x", value);
            break;
        case device_property_flags:
            Device_GetPropertyU16(device, property, &u16_value);
            DEBUG_LOG_VERBOSE("\tflags %04x", u16_value);
            break;
        case device_property_sco_fwd_features:
            Device_GetPropertyU16(device, property, &u16_value);
            DEBUG_LOG_VERBOSE("\tsco_fwd_features %04x", u16_value);
            break;
        case device_property_mru:
            Device_GetPropertyU8(device, property, &value);
            DEBUG_LOG_VERBOSE("\tmru %1x", value);
            break;
        case device_property_profile_request_index:
            Device_GetPropertyU8(device, property, &value);
            DEBUG_LOG_VERBOSE("\tprofile_request_index %1x", value);
            break;
        case device_property_profiles_connect_order:
            {
                profile_t *ptr_profile_order = NULL;
                size_t size;
                PanicFalse(Device_GetProperty(device, property, (void *)&ptr_profile_order, &size));
                profile_t array[5] = {profile_manager_bad_profile};
                memcpy(array, ptr_profile_order, MIN(size,5));
                DEBUG_LOG_VERBOSE("\tprofiles_connect_order %d,%d,%d,%d,%d", array[0], array[1], array[2], array[3], array[4]);
            }
            break;
        default:
            break;
        }
    }
}

static void earbudTest_ReportDeviceData(device_t device, void * data)
{
    UNUSED(data);

    DEBUG_LOG_ALWAYS("Device %x", device);
    for (int property=0; property < device_property_max_num; property++)
    {
        if (Device_IsPropertySet(device, property))
        {
            earbudTest_ReportProperty(device, property);
        }
    }
}

void EarbudTest_DeviceDatabaseReport(void)
{
    DEBUG_LOG_ALWAYS("DeviceDatabase");
    DeviceList_Iterate(earbudTest_ReportDeviceData, NULL);
}

extern uint8 profile_list[4];

static device_t earbudTest_GetHandset(void)
{
    bool is_mru_handset = TRUE;
    device_t handset_device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_mru, &is_mru_handset, sizeof(uint8));
    if (!handset_device)
    {
        bdaddr handset_address = {0,0,0};
        if (appDeviceGetHandsetBdAddr(&handset_address))
        {
            handset_device = BtDevice_GetDeviceForBdAddr(&handset_address);
        }
    }
    return handset_device;
}

void EarbudTest_ConnectHandset(void)
{
    bdaddr handset_addr = {0};

    if (appDeviceGetHandsetBdAddr(&handset_addr))
    {
        device_t handset_device = BtDevice_GetDeviceForBdAddr(&handset_addr);
        uint8 profiles_to_connect = BtDevice_GetLastConnectedProfilesForDevice(handset_device);

        if (!profiles_to_connect)
        {
            Device_GetPropertyU8(handset_device,device_property_supported_profiles, &profiles_to_connect);
        }

        DEBUG_LOG_ALWAYS("EarbudTest_ConnectHandset lap=%6x profiles=%02x", handset_addr.lap, profiles_to_connect);

        HandsetService_ConnectAddressRequest(SmGetTask(), &handset_addr, profiles_to_connect);
    }
}

void EarbudTest_DisconnectHandset(void)
{
    bdaddr handset_addr = {0};

    if (appDeviceGetHandsetBdAddr(&handset_addr))
    {
        DEBUG_LOG_ALWAYS("EarbudTest_DisconnectHandset lap=%6x", handset_addr.lap);

        HandsetService_DisconnectRequest(SmGetTask(), &handset_addr);
    }
}

bool appTestIsHandsetFullyConnected(void)
{
    bool connected = FALSE;
    device_t handset_device = earbudTest_GetHandset();

    if (handset_device)
    {
        connected = HandsetService_Connected(handset_device);
        DEBUG_LOG_ALWAYS("appTestIsHandsetFullyConnected:%d", connected);
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestIsHandsetFullyConnected:0  No handset device");
    }
    return connected;
}

bool appTestIsHandsetAclConnected(void)
{
    bool connected = FALSE;
    device_t handset_device = earbudTest_GetHandset();

    if (handset_device)
    {
        bdaddr addr = DeviceProperties_GetBdAddr(handset_device);
        connected = ConManagerIsConnected(&addr);
        DEBUG_LOG_ALWAYS("appTestIsHandsetAclConnected:%d 0x%06x", connected, addr.lap);
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestIsHandsetAclConnected:0 No handset device/addr");
    }
    return connected;
}


bool appTestPrimaryAddressIsFromThisBoard(void)
{
    bdaddr self;
    uint16 flags;
    bool is_primary = FALSE;

    HYDRA_LOG_STRING(mine, "MINE");
    HYDRA_LOG_STRING(peers, "PEERS");


    appDeviceGetMyBdAddr(&self);

    if (appDeviceGetFlags(&self, &flags))
    {
        if (flags & DEVICE_FLAGS_MIRRORING_C_ROLE)
        {
            is_primary = TRUE;
        }
    }

    DEBUG_LOG_ALWAYS("appTestPrimaryAddressIsFromThisBoard %04x%02x%06x %s",
            self.nap, self.uap, self.lap,
            is_primary ? mine : peers);

    return is_primary;
}

static void earbudTest_RestartFindRole(void)
{
    appTestPhyStateInCaseEvent();
    MessageCancelAll((Task) &delayedInternalTestEventHandlerTask, WAIT_FOR_IDLE_THEN_BRING_OUT_OF_CASE);
    MessageSendLater((Task) &delayedInternalTestEventHandlerTask, WAIT_FOR_IDLE_THEN_BRING_OUT_OF_CASE,
                                         NULL, 100);
}

void EarbudTest_PeerFindRoleOverrideScore(uint16 score)
{
    DEBUG_LOG_ALWAYS("EarbudTest_PeerFindRoleOverrideScore 0x%x", score);
    PeerFindRole_OverrideScore((grss_figure_of_merit_t)score);
    if(!appTestIsTopologyIdle())
    {
        earbudTest_RestartFindRole();
    }
}

bool EarbudTest_IsPeerSignallingConnected(void)
{
    return appPeerSigIsConnected();
}

bool EarbudTest_IsPeerSignallingDisconnected(void)
{
    return appPeerSigIsDisconnected();
}

#ifdef INCLUDE_HDMA_RSSI_EVENT
const hdma_thresholds_t *appTestGetRSSIthreshold(void)
{
    DEBUG_LOG_VERBOSE("halfLife: critical [%d] high [%d], low [%d]", rssi.halfLife_ms.critical, rssi.halfLife_ms.high, rssi.halfLife_ms.low);
    DEBUG_LOG_VERBOSE("maxAge: critical [%d] high [%d], low [%d]", rssi.maxAge_ms.critical, rssi.maxAge_ms.high, rssi.maxAge_ms.low);
    DEBUG_LOG_VERBOSE("absThreshold: critical [%d] high [%d], low [%d]", rssi.absThreshold.critical, rssi.absThreshold.high, rssi.absThreshold.low);
    DEBUG_LOG_VERBOSE("relThreshold: critical [%d] high [%d], low [%d]", rssi.relThreshold.critical, rssi.relThreshold.high, rssi.relThreshold.low);
    return &rssi;
}
#endif

#ifdef INCLUDE_HDMA_MIC_QUALITY_EVENT
const hdma_thresholds_t *appTestGetMICthreshold(void)
{
	DEBUG_LOG_VERBOSE("halfLife: critical [%d] high [%d], low [%d]", mic.halfLife_ms.critical, mic.halfLife_ms.high, mic.halfLife_ms.low);
	DEBUG_LOG_VERBOSE("maxAge: critical [%d] high [%d], low [%d]", mic.maxAge_ms.critical, mic.maxAge_ms.high, mic.maxAge_ms.low);
	DEBUG_LOG_VERBOSE("absThreshold: critical [%d] high [%d], low [%d]", mic.absThreshold.critical, mic.absThreshold.high, mic.absThreshold.low);
	DEBUG_LOG_VERBOSE("relThreshold: critical [%d] high [%d], low [%d]", mic.relThreshold.critical, mic.relThreshold.high, mic.relThreshold.low);
	return &mic;
}
#endif


/* private API added to upgrade library */
extern bool UpgradePSClearStore(void);
bool appTestUpgradeResetState(void)
{
    DEBUG_LOG_ALWAYS("appTestUpgradeResetState");

    return UpgradePSClearStore();
}

void EarbudTest_EnterInCaseDfu(void)
{
    DEBUG_LOG_ALWAYS("EarbudTest_EnterInCaseDfu");

    appSmEnterDfuModeInCase(TRUE, TRUE);
    appTestPhyStateInCaseEvent();
}

uint16 appTestSetTestNumber(uint16 test_number)
{
    test_support.testcase = test_number;

    return test_number;
}

uint16 appTestSetTestIteration(uint16 test_iteration)
{
    test_support.iteration = test_iteration;

    return test_iteration;
}

uint16 appTestWriteMarker(uint16 marker)
{
    static unsigned testcase = 0;
    test_support.last_marker = marker;

    if (   test_support.testcase 
        && (  test_support.testcase != testcase
           || marker == 0))
    {
        testcase = test_support.testcase;

        if (test_support.iteration)
        {
            DEBUG_LOG_VERBOSE("@@@Testcase:%d  Iteration:%d -------------------------------");
        }
        else
        {
            DEBUG_LOG_VERBOSE("@@@Testcase:%d  ------------------------------------------");
        }
    }

    if (marker)
    {
        if (test_support.testcase && test_support.iteration)
        {
            DEBUG_LOG_VERBOSE("@@@Testcase marker: TC%d Iteration:%d Step:%d *************************",
                    testcase, test_support.iteration, marker);
        }
        else if (test_support.testcase)
        {
            DEBUG_LOG_VERBOSE("@@@Testcase marker: TC%d Step:%d *************************",
                    testcase, marker);
        }
        else
        {
            DEBUG_LOG_VERBOSE("@@@Testcase marker: Iteration:%d Step:%d *************************",
                    test_support.iteration, marker);
        }
    }

    return marker;
}

void appTestVaTap(void)
{
    DEBUG_LOG_ALWAYS("appTestVaTap");
    /* Simulates a "Button Down -> Button Up -> Single Press Detected" sequence
    for the default configuration of a dedicated VA button */
#ifndef INCLUDE_AMA
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_1);
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_6);
#endif
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_3);
}

void appTestVaDoubleTap(void)
{
    DEBUG_LOG_ALWAYS("appTestVaDoubleTap");
    /* Simulates a "Button Down -> Button Up -> Button Down -> Button Up -> Double Press Detected" 
    sequence for the default configuration of a dedicated VA button */
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_1);
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_6);
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_1);
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_6);
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_4);
}

void appTestVaPressAndHold(void)
{
    DEBUG_LOG_ALWAYS("appTestVaPressAndHold");
    /* Simulates a "Button Down -> Hold" sequence for the default configuration
    of a dedicated VA button */
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_1);
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_5);
}

void appTestVaRelease(void)
{
    DEBUG_LOG_ALWAYS("appTestVaRelease");
    /* Simulates a "Button Up" event for the default configuration
    of a dedicated VA button */
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_6);
}

void appTestCvcPassthrough(void)
{
    DEBUG_LOG_ALWAYS("appTestCvcPassthrough");
    kymera_EnableCvcPassthroughMode();
}

void EarbudTest_SetAncEnable(void)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_SetAncEnable");
        Ui_InjectUiInput(ui_input_anc_on);
    }
}

void EarbudTest_SetAncDisable(void)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_SetAncDisable");
        Ui_InjectUiInput(ui_input_anc_off);
    }
}

void EarbudTest_SetAncToggleOnOff(void)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_SetAncToggleOnOff");
        Ui_InjectUiInput(ui_input_anc_toggle_on_off);
    }
}

void EarbudTest_SetAncMode(anc_mode_t mode)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_SetAncMode");
        switch(mode)
        {
            case anc_mode_1:
                Ui_InjectUiInput(ui_input_anc_set_mode_1);
                break;
            case anc_mode_2:
                Ui_InjectUiInput(ui_input_anc_set_mode_2);
                break;
            case anc_mode_3:
                Ui_InjectUiInput(ui_input_anc_set_mode_3);
                break;
            case anc_mode_4:
                Ui_InjectUiInput(ui_input_anc_set_mode_4);
                break;
            case anc_mode_5:
                Ui_InjectUiInput(ui_input_anc_set_mode_5);
                break;
            case anc_mode_6:
                Ui_InjectUiInput(ui_input_anc_set_mode_6);
                break;
            case anc_mode_7:
                Ui_InjectUiInput(ui_input_anc_set_mode_7);
                break;
            case anc_mode_8:
                Ui_InjectUiInput(ui_input_anc_set_mode_8);
                break;
            case anc_mode_9:
                Ui_InjectUiInput(ui_input_anc_set_mode_9);
                break;
            case anc_mode_10:
                Ui_InjectUiInput(ui_input_anc_set_mode_10);
                break;
            default:
                Ui_InjectUiInput(ui_input_anc_set_mode_1);
                break;
        }
    }
}

void EarbudTest_SetAncNextMode(void)
{
    DEBUG_LOG_ALWAYS("EarbudTest_SetAncNextMode");
    Ui_InjectUiInput(ui_input_anc_set_next_mode);
}

bool EarbudTest_GetAncstate(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GetAncstate");
     return AncStateManager_IsEnabled();

}

anc_mode_t EarbudTest_GetAncMode(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GetAncMode");
     return AncStateManager_GetMode();
}

static void earbudTest_SendGaiaCommandToAnc(uint8 pdu_id, uint8 payload_size, uint8 payload)
{
     DEBUG_LOG_ALWAYS("earbudTest_SendGaiaCommandToAnc");
     uint8 pdu_type = 1; //Unused
     GaiaFrameworkFeature_SendToFeature(GAIA_AUDIO_CURATION_FEATURE_ID, pdu_type, pdu_id, payload_size, &payload);
}

void EarbudTest_GAIACommandGetAncState(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandGetAncState");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_get_anc_state, 1, 0);
}

void EarbudTest_GAIACommandSetAncEnable(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandSetAncEnable");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_set_anc_state, ANC_GAIA_SET_ANC_STATE_PAYLOAD_LENGTH, ANC_GAIA_SET_ANC_STATE_ENABLE);
}

void EarbudTest_GAIACommandSetAncDisable(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandSetAncDisable");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_set_anc_state, ANC_GAIA_SET_ANC_STATE_PAYLOAD_LENGTH, ANC_GAIA_SET_ANC_STATE_DISABLE);
}

void EarbudTest_GAIACommandGetAncNumOfModes(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandGetAncNumOfModes");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_get_num_anc_modes, 1, 0);
}

void EarbudTest_GAIACommandSetAncMode(uint8 mode)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandSetAncMode");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_set_anc_mode, ANC_GAIA_SET_ANC_MODE_PAYLOAD_LENGTH, mode);
}

void EarbudTest_GAIACommandGetAncLeakthroughGain(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandGetAncLeakthroughGain");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_get_configured_leakthrough_gain, 1, 0);
}

void EarbudTest_SetAncLeakthroughGain(uint8 gain)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_SetAncLeakthroughGain %d", gain);
        AncStateManager_StoreAncLeakthroughGain(gain);
        Ui_InjectUiInput(ui_input_anc_set_leakthrough_gain);
    }
}

uint8 EarbudTest_GetAncLeakthroughGain(void)
{
    DEBUG_LOG("EarbudTest_GetAncLeakthroughGain");
    return AncStateManager_GetAncLeakthroughGain();
}

void EarbudTest_GAIACommandSetAncLeakthroughGain(uint8 gain)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandSetAncLeakthroughGain");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_set_configured_leakthrough_gain, ANC_GAIA_SET_ANC_LEAKTHROUGH_GAIN_PAYLOAD_LENGTH, gain);
}

void EarbudTest_EnableLeakthrough(void)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_EnableLeakthrough");
        Ui_InjectUiInput(ui_input_leakthrough_on);
    }
}

void EarbudTest_DisableLeakthrough(void)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_DisableLeakthrough");
        Ui_InjectUiInput(ui_input_leakthrough_off);
    }
}

void EarbudTest_ToggleLeakthroughOnOff(void)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_ToggleLeakthroughOnOff");
        Ui_InjectUiInput(ui_input_leakthrough_toggle_on_off);
    }
}

void EarbudTest_SetLeakthroughMode(leakthrough_mode_t leakthrough_mode)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_SetLeakthroughMode");
        switch(leakthrough_mode)
        {
            case LEAKTHROUGH_MODE_1:
                Ui_InjectUiInput(ui_input_leakthrough_set_mode_1);
                break;
            case LEAKTHROUGH_MODE_2:
                Ui_InjectUiInput(ui_input_leakthrough_set_mode_2);
                break;
            case LEAKTHROUGH_MODE_3:
                Ui_InjectUiInput(ui_input_leakthrough_set_mode_3);
                break;
            default:
                DEBUG_LOG("Invalid value of leakthrough mode is passed");
                break;
        }
    }
}

void EarbudTest_SetNextLeakthroughMode(void)
{
    if(appPhyStateIsOutOfCase())
    {
        Ui_InjectUiInput(ui_input_leakthrough_set_next_mode);
    }
}

leakthrough_mode_t EarbudTest_GetLeakthroughMode(void)
{
    DEBUG_LOG("EarbudTest_GetLeakthroughMode");
    return AecLeakthrough_GetMode();
}

bool EarbudTest_IsLeakthroughEnabled(void)
{
    DEBUG_LOG("EarbudTest_IsLeakthroughEnabled");
    return AecLeakthrough_IsLeakthroughEnabled();
}

void appTestSetFixedRole(peer_find_role_fixed_role_t role)
{
    DEBUG_LOG_ALWAYS("appTestSetFixedRole role=%d", role);
    PeerFindRole_SetFixedRole(role);
}

appKymeraScoMode appTestGetHfpCodec(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    if (theKymera)
    {
        DEBUG_LOG("appTestGetHfpCodec codec=%d", theKymera->sco_info->mode);
        return theKymera->sco_info->mode;
    }
    else
    {
        return NO_SCO;
    }
}


#ifdef INCLUDE_ACCESSORY

void appTestRequestAppLaunch(const char * app_name)
{
    DEBUG_LOG_ALWAYS("appTestRequestRttAppLaunch");
    bdaddr handset_address = {0,0,0};
    if(appDeviceGetHandsetBdAddr(&handset_address))
    {
        AccessoryFeature_RequestAppLaunch(handset_address, app_name, launch_with_user_alert);
    }
}

bool appTestIsAccessoryConnected(void)
{
    DEBUG_LOG_ALWAYS("appTestIsAccessoryConnected");
    bool is_connected = FALSE;
    bdaddr handset_address = {0,0,0};
    if(appDeviceGetHandsetBdAddr(&handset_address))
    {
        is_connected = Accessory_IsConnected(handset_address);
    }
    return is_connected;
}

#endif

bool appTestAnyBredrConnection(void)
{
    bool bredr_connection = ConManagerAnyTpLinkConnected(cm_transport_bredr);

    DEBUG_LOG_ALWAYS("appTestAnyBredrConnection: %d", bredr_connection);

    return bredr_connection;
}


int appTestGetCurrentAudioVolume(void)
{
    return AudioSources_GetVolume(audio_source_a2dp_1).value;
}

int appTestGetCurrentVoiceVolume(void)
{
    return VoiceSources_GetVolume(voice_source_hfp_1).value;
}

void EarbudTest_SetActiveVa2GAA(void)
{
    VoiceUi_SelectVoiceAssistant(voice_ui_provider_gaa);
}

void EarbudTest_SetActiveVa2AMA(void)
{
    VoiceUi_SelectVoiceAssistant(voice_ui_provider_ama);
}


bool EarbudTest_StartVaCapture(va_audio_codec_t encoder)
{
    bool status = FALSE;
    va_audio_voice_capture_params_t params = {0};

    params.mic_config.sample_rate = 16000;

    if (earbudTest_PopulateVaEncodeConfig(encoder, &params.encode_config))
    {
        status = VoiceAudioManager_StartCapture(earbudTest_DropDataInSource, &params);
    }

    return status;
}

bool EarbudTest_StopVaCapture(void)
{
    return VoiceAudioManager_StopCapture();
}

bool EarbudTest_StartVaWakeUpWordDetection(va_wuw_engine_t wuw_engine, bool start_capture_on_detection, va_audio_codec_t encoder)
{
    bool status = FALSE;
    va_audio_wuw_detection_params_t params = {0};

    va_config.start_capture_on_detection = start_capture_on_detection;
    va_config.encoder_for_capture_on_detection = encoder;
    va_config.wuw_engine = wuw_engine;

    if (earbudTest_PopulateWuwDetectionStartParams(wuw_engine, &params))
    {
        status = VoiceAudioManager_StartDetection(earbudTest_WuwDetected, &params);
    }

    return status;
}

bool EarbudTest_StopVaWakeUpWordDetection(void)
{
    return VoiceAudioManager_StopDetection();
}


bool appTestIsDeviceTestServiceEnabled(void)
{
    bool enabled = DeviceTestService_TestMode();

    DEBUG_LOG_ALWAYS("appTestIsDeviceTestServiceEnabled:%d", enabled);
    return enabled;
}


bool appTestIsDeviceTestServiceActive(void)
{
    bool active = FALSE;

#ifdef INCLUDE_DEVICE_TEST_SERVICE
    active = DeviceTestServiceIsActive();
#endif

    DEBUG_LOG_ALWAYS("appTestIsDeviceTestServiceActive. Active:%d", active);

    return active;
}


bool appTestIsDeviceTestServiceAuthenticated(void)
{
    bool authenticated = FALSE;

#ifdef INCLUDE_DEVICE_TEST_SERVICE
    authenticated = DeviceTestServiceIsAuthenticated();
#endif

    DEBUG_LOG_ALWAYS("appTestIsDeviceTestServiceAuthenticated. Authenticated:%d", authenticated);

    return authenticated;
}


bool appTestDeviceTestServiceEnable(bool enable)
{
    bool saved = FALSE;

    DeviceTestService_SaveTestMode((uint16)enable);

    /* The SaveTestMode API does not check if the PSKEY write was
       successful. Check now by reading the value back.
     */
    saved = enable == DeviceTestService_TestMode();

    DEBUG_LOG_ALWAYS("appTestDeviceTestServiceEnable. Saved:%d", saved);

    return saved;
}

unsigned appTestDeviceTestServiceDevices(device_t **devices)
{
    unsigned num_found = deviceTestService_GetDtsDevices(devices);

    DEBUG_LOG_ALWAYS("appTestDeviceTestServiceDevices. Devices:%u", 
                      num_found);

    return num_found;
}


bool appTestIsBredrScanEnabled(void)
{
    bool scan_enabled = !BredrScanManager_IsScanDisabled();

    DEBUG_LOG_ALWAYS("appTestIsBredrScanEnabled Enabled:%d", scan_enabled);

    return scan_enabled;
}
bool appTestIsPreparingToSleep(void)
{
    appState state = appSmGetState();
    return (state & APP_SUBSTATE_SOPORIFIC_TERMINATING) != 0;
}


#ifdef GC_SECTIONS
/**
 * @brief appTestShowKeptSymbols
 *
 * A list of ADK and library functions and/or variables that are referenced
 * directly via pydbg test scripts, but which are otherwise unused given the
 * current software configuration.
 *
 * This is an alternative mechanism to scattering #pragma unitcodesection KEEP_PM
 * over the library code.  Doing that would unnecessarily include code in production
 * builds even if such code was not actually used.
 *
 * So here we fake our interest by making pointers to things of interest.
 * Nothing is called or dereferenced, but it's enough to stop the linker
 * garbage collecting otherwise unused resources.
 *
 * It's not necessary to call this function, but it's harmess if you do.
 */
static const void *tableOfSymbolsToKeep[] = {
    (void*)ConnectionReadTxPower,
};
void appTestShowKeptSymbols(void);
void appTestShowKeptSymbols(void)
{
    for( size_t i = 0 ; i < sizeof(tableOfSymbolsToKeep)/sizeof(tableOfSymbolsToKeep[0]) ; i++ )
    {
        if( tableOfSymbolsToKeep[i] != NULL )
        {
            DEBUG_LOG_ALWAYS("Have %p",tableOfSymbolsToKeep[i]);
        }
    }
}
#endif
