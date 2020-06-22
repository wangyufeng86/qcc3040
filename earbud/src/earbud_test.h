/*!
\copyright  Copyright (c) 2017 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_test.h
\brief      Interface for specifc application testing functions
*/

#ifndef EARBUD_TEST_H
#define EARBUD_TEST_H

#include <bdaddr.h>
#include <earbud_sm.h>
#include <peer_find_role.h>
#include "aec_leakthrough.h"

#include <anc.h>
#include <device.h>
#include <va_audio_types.h>
#include <hdma_utils.h>

extern bool appTestHandsetPairingBlocked;
extern TaskData testTask;

/*! \brief Returns the current battery voltage
 */
uint16 appTestGetBatteryVoltage(void);

/*! \brief Sets the battery voltage to be returned by
    appBatteryGetVoltage().

    This test function also causes the battery module to read
    this new voltage level - which may be subjected to hysteresis.
 */
void appTestSetBatteryVoltage(uint16 new_level);

/*! \brief Return TRUE if the current battery state is battery_level_ok. */
bool appTestBatteryStateIsOk(void);

/*! \brief Return TRUE if the current battery state is battery_level_low. */
bool appTestBatteryStateIsLow(void);

/*! \brief Return TRUE if the current battery state is battery_level_critical. */
bool appTestBatteryStateIsCritical(void);

/*! \brief Return TRUE if the current battery state is battery_level_too_low. */
bool appTestBatteryStateIsTooLow(void);

/*! \brief Return the number of milliseconds taken for the battery measurement
    filter to fully respond to a change in input battery voltage */
uint32 appTestBatteryFilterResponseTimeMs(void);

#ifdef HAVE_THERMISTOR
/*! \brief Returns the expected thermistor voltage at a specified temperature.
    \param temperature The specified temperature in degrees Celsius.
    \return The equivalent milli-voltage.
*/
uint16 appTestThermistorDegreesCelsiusToMillivolts(int8 temperature);
#endif

/*! \brief Put Earbud into Handset Pairing mode
*/
void appTestPairHandset(void);

/*! \brief Delete all Handset pairing
*/
void appTestDeleteHandset(void);

/*! \brief Delete Earbud peer pairing

    Attempts to delete pairing to earbud. Check the debug output if
    this test command fails, as it will report failure reason.

    \return TRUE if pairing was successfully removed.
            FALSE if there is no peer pairing, or still connected to the device.
*/
bool appTestDeletePeer(void);

/*! \brief Get the devices peer bluetooth address

    \param  peer_address    pointer to the peer paired address if found
    \return TRUE if the device is peer paired
            FALSE if device is not peer paired
*/
bool appTestGetPeerAddr(bdaddr *peer_address);


/*! \brief Read the current address from the connection library

    The address is not returned directly, call appTestGetLocalAddr
    which will populate an address and also indicate if the address
    was updated after the last call to this function.

    \note for test purposes, simply calling this function will
    cause the current bdaddr to be displayed when it is retrieved.
 */
void appTestReadLocalAddr(void);


/*! \brief retrieve the local bdaddr last read

    An update of the address is requested using appTestReadLocalAddr.
    Normal usage would be to call appTestReadLocalAddr() then call
    this function until it returns True.

    From pydbg you can allocate memory for the address and use it as below

        bd = apps1.fw.call.pnew("bdaddr")
        apps1.fw.call.appTestGetLocalAddr(bd.address)
        print(bd)

    \param[out] addr Pointer to location to store the address

    \return TRUE if the address was updated following call to 
        appTestReadLocalAddr
 */
bool appTestGetLocalAddr(bdaddr *addr);


/*! \brief Return if Earbud is in a Pairing mode

    \return bool TRUE Earbud is in pairing mode
                 FALSE Earbud is not in pairing mode
*/
bool appTestIsPairingInProgress(void);

/*! \brief Initiate Earbud A2DP connection to the Handset

    \return bool TRUE if A2DP connection is initiated
                 FALSE if no handset is paired
*/
bool appTestHandsetA2dpConnect(void);

/*! \brief Stop the earbud automatically pairing with a handset

    Rules that permit pairing will be stopped while a block is in
    place.

    \param  block   Enable/Disable the block
*/
void appTestBlockAutoHandsetPairing(bool block);

/*! \brief Return if Earbud has a handset pairing

    \return TRUE Earbud is paired with at least one handset, 
            FALSE otherwise
*/
bool appTestIsHandsetPaired(void);

/*! \brief Return if Earbud has an Handset A2DP connection

    \return bool TRUE Earbud has A2DP Handset connection
                 FALSE Earbud does not have A2DP Handset connection
*/
bool appTestIsHandsetA2dpConnected(void);

/*! \brief Return if Earbud has an Handset A2DP media connection

    \return bool TRUE Earbud has A2DP Handset media connection
                 FALSE Earbud does not have A2DP Handset connection
*/
bool appTestIsHandsetA2dpMediaConnected(void);

/*! \brief Return if Earbud is in A2DP streaming mode with the handset

    \return bool TRUE Earbud is in A2DP streaming mode
                     FALSE Earbud is not in A2DP streaming mode
*/
bool appTestIsHandsetA2dpStreaming(void);

/*! \brief Initiate Earbud AVRCP connection to the Handset

    \return bool TRUE if AVRCP connection is initiated
                 FALSE if no handset is paired
*/


bool appTestIsA2dpPlaying(void);

bool appTestHandsetAvrcpConnect(void);

/*! \brief Return if Earbud has an Handset AVRCP connection

    \return bool TRUE Earbud has AVRCP Handset connection
                 FALSE Earbud does not have AVRCP Handset connection
*/
bool appTestIsHandsetAvrcpConnected(void);

/*! \brief Initiate Earbud HFP connection to the Handset

    \return bool TRUE if HFP connection is initiated
                 FALSE if no handset is paired
*/
bool appTestHandsetHfpConnect(void);

/*! \brief Return if Earbud has an Handset HFP connection

    \return bool TRUE Earbud has HFP Handset connection
                 FALSE Earbud does not have HFP Handset connection
*/
bool appTestIsHandsetHfpConnected(void);

/*! \brief Return if Earbud has an Handset HFP SCO connection

    \return bool TRUE Earbud has HFP SCO Handset connection
                 FALSE Earbud does not have HFP SCO Handset connection
*/
bool appTestIsHandsetHfpScoActive(void);

/*! \brief Initiate Earbud HFP Voice Dial request to the Handset
*/
void appTestHandsetHfpVoiceDial(void);

/*! \brief Toggle Microphone mute state on HFP SCO conenction to handset
*/
void appTestHandsetHfpMuteToggle(void);

/*! \brief Transfer HFP voice to the Handset
*/
void appTestHandsetHfpVoiceTransferToAg(void);

/*! \brief Transfer HFP voice to the Earbud
*/
void appTestHandsetHfpVoiceTransferToHeadset(void);

/*! \brief Accept incoming call, either local or forwarded (SCO forwarding)
*/
void appTestHandsetHfpCallAccept(void);

/*! \brief Reject incoming call, either local or forwarded (SCO forwarding)
*/
void appTestHandsetHfpCallReject(void);

/*! \brief End the current call, either local or forwarded (SCO forwarding)
*/
void appTestHandsetHfpCallHangup(void);

/*! \brief Initiated last number redial

    \return bool TRUE if last number redial was initiated
                 FALSE if HFP is not connected
*/
bool appTestHandsetHfpCallLastDialed(void);

/*! \brief Start decreasing the HFP volume
*/
void appTestHandsetHfpVolumeDownStart(void);

/*! \brief Start increasing the HFP volume
*/
void appTestHandsetHfpVolumeUpStart(void);

/*! \brief Stop increasing or decreasing HFP volume
*/
void appTestHandsetHfpVolumeStop(void);

/*! \brief Set the Hfp Sco volume

    \return bool TRUE if the volume set request was initiated
                 FALSE if HFP is not in a call
*/
bool appTestHandsetHfpSetScoVolume(uint8 volume);

/*! \brief Get microphone mute status

    \return bool TRUE if microphone muted,
                 FALSE if not muted
*/
bool appTestIsHandsetHfpMuted(void);

/*! \brief Check if call is in progress

    \return bool TRUE if call in progress,
                 FALSE if no call, or not connected
*/
bool appTestIsHandsetHfpCall(void);

/*! \brief Check if incoming call

    \return bool TRUE if call incoming,
                 FALSE if no call, or not connected
*/
bool appTestIsHandsetHfpCallIncoming(void);

/*! \brief Check if outgoing call

    \return bool TRUE if call outgoing,
                 FALSE if no call, or not connected
*/
bool appTestIsHandsetHfpCallOutgoing(void);

/*! \brief Return if Earbud has an ACL connection to the Handset

    It does not indicate if the handset is usable, with profiles
    connected. Use appTestIsHandsetConnected or 
    appTestIsHandsetFullyConnected.

    \return bool TRUE Earbud has an ACL connection
                 FALSE Earbud does not have an ACL connection to the Handset
*/
bool appTestIsHandsetAclConnected(void);

/*! \brief Return if Earbud has a profile connection to the Handset

    This can be HFP, A2DP or AVRCP. It does not indicate if there
    is an ACL connection.

    \return bool TRUE Earbud has a connection to the Handset
                 FALSE Earbud does not have a connection to the Handset
*/
bool appTestIsHandsetConnected(void);

/*! \brief Is the handset completely connected (all profiles)

    This function checks whether the handset device is connected
    completely. Unlike appTestIsHandsetConnected() this function
    checks that all the handset profiles required are connected.

    \return TRUE if there is a handset, all required profiles
        are connected, and we have not started disconnecting. 
        FALSE in all other cases.
 */
bool appTestIsHandsetFullyConnected(void);

/*! \brief Initiate Earbud A2DP connection to the the Peer

    \return bool TRUE if A2DP connection is initiated
                 FALSE if no Peer is paired
*/
bool appTestPeerA2dpConnect(void);

/*! \brief Return if Earbud has a Peer A2DP connection

    \return bool TRUE Earbud has A2DP Peer connection
                 FALSE Earbud does not have A2DP Peer connection
*/
bool appTestIsPeerA2dpConnected(void);

/*! \brief Check if Earbud is in A2DP streaming mode with peer Earbud

    \return TRUE if A2DP streaming to peer device
*/
bool appTestIsPeerA2dpStreaming(void);

/*! \brief Initiate Earbud AVRCP connection to the the Peer

    \return bool TRUE if AVRCP connection is initiated
                 FALSE if no Peer is paired
*/
bool appTestPeerAvrcpConnect(void);

/*! \brief Return if Earbud has a Peer AVRCP connection

    \return bool TRUE Earbud has AVRCP Peer connection
                 FALSE Earbud does not have AVRCP Peer connection
*/
bool appTestIsPeerAvrcpConnected(void);

/*! \brief Send the AV toggle play/pause command
*/
void appTestAvTogglePlayPause(void);

/*! \brief Start dynamic handover procedure.
    \return TRUE if handover was initiated, otherwise FALSE.
*/
bool earbudTest_DynamicHandover(void);

/*! \brief Send the Avrcp pause command to the Handset
*/
void appTestAvPause(void);

/*! \brief Send the Avrcp play command to the Handset
*/
void appTestAvPlay(void);

/*! \brief Send the Avrcp stop command to the Handset
*/
void appTestAvStop(void);

/*! \brief Send the Avrcp forward command to the Handset
*/
void appTestAvForward(void);

/*! \brief Send the Avrcp backward command to the Handset
*/
void appTestAvBackward(void);

/*! \brief Send the Avrcp fast forward state command to the Handset
*/
void appTestAvFastForwardStart(void);

/*! \brief Send the Avrcp fast forward stop command to the Handset
*/
void appTestAvFastForwardStop(void);

/*! \brief Send the Avrcp rewind start command to the Handset
*/
void appTestAvRewindStart(void);

/*! \brief Send the Avrcp rewind stop command to the Handset
*/
void appTestAvRewindStop(void);

/*! \brief Send the Avrcp volume change command to the Handset

    \param step Step change to apply to volume

    \return bool TRUE volume step change sent
                 FALSE volume step change was not sent
*/
bool appTestAvVolumeChange(int8 step);

/*! \brief Send the Avrcp pause command to the Handset

    \param volume   New volume level to set (0-127).
*/
void appTestAvVolumeSet(uint8 volume);

/*! \brief Allow tests to control whether the earbud will enter dormant.

    If an earbud enters dormant, cannot be woken by a test.

    \note Even if dormant mode is enabled, the application may not
        enter dormant. Dormant won't be used if the application is
        busy, or connected to a charger - both of which are quite
        likely for an application under test.

    \param  enable  Use FALSE to disable dormant, or TRUE to enable.
*/
void appTestPowerAllowDormant(bool enable);

/*! \brief Generate event that Earbud is now in the case. */
void appTestPhyStateInCaseEvent(void);

/*! \brief Generate event that Earbud is now out of the case. */
void appTestPhyStateOutOfCaseEvent(void);

/*! \brief Generate event that Earbud is now in ear. */
void appTestPhyStateInEarEvent(void);

/*! \brief Generate event that Earbud is now out of the ear. */
void appTestPhyStateOutOfEarEvent(void);

/*! \brief Generate event that Earbud is now moving */
void appTestPhyStateMotionEvent(void);

/*! \brief Generate event that Earbud is now not moving. */
void appTestPhyStateNotInMotionEvent(void);

/*! \brief Generate event that Earbud is now (going) off. */
void appTestPhyStateOffEvent(void);

/*! \brief Return TRUE if the earbud is in the ear. */
bool appTestPhyStateIsInEar(void);

/*! \brief Return TRUE if the earbud is out of the ear, but not in the case. */
bool appTestPhyStateOutOfEar(void);

/*! \brief Return TRUE if the earbud is in the case. */
bool appTestPhyStateIsInCase(void);

/*! \brief Reset an Earbud to factory defaults.
    Will drop any connections, delete all pairing and reboot.
*/
void appTestFactoryReset(void);

/*! \brief Determine if a reset has happened

    \return TRUE if a reset has happened since the last call to the function
*/
bool appTestResetHappened(void);

/*! \brief Connect to default handset. */
void appTestConnectHandset(void);

/*! \brief Connect the A2DP media channel to the handset
    \return True is request sent, else false
 */
bool appTestConnectHandsetA2dpMedia(void);

/*! \brief  Check if peer synchronisation was successful

    \returns TRUE if we are synchronised with the peer.
*/
bool appTestIsPeerSyncComplete(void);

/*! \brief Power off.
    \return TRUE if the device can power off - the device will drop connections then power off.
            FALSE if the device cannot currently power off.
*/
bool appTestPowerOff(void);

/*! \brief Determine if the earbud has a paired peer earbud.
    \return bool TRUE the earbud has a paired peer, FALSE the earbud has no paired peer.
*/
bool appTestIsPeerPaired(void);

/*! \brief Determine if the all licenses are correct.
    \return bool TRUE the licenses are correct, FALSE if not.
*/
bool appTestLicenseCheck(void);

/*! \brief Control 2nd earbud connecting to handset after TWS+ pairing.
    \param bool TRUE enable 2nd earbud connect to handset.
                FALSE disable 2nd earbud connect to handset, handset must connect.
    \note This API is deprecated, the feature is no longer supported.
 */
void appTestConnectAfterPairing(bool enable);

/*! \brief Asks the connection library about the sco forwarding link.

    The result is reported as debug.
 */
void appTestScoFwdLinkStatus(void);

/* \brief Enable or disable randomised dropping of SCO forwarding packets

    This function uses random numbers to stop transmissio of SCO forwarding
    packets, so causing error mechanisms to be used on the receiving side.
    Packet Loss Concealment (PLC) and possibly audio muting or disconnection.

    There are two modes.
    - set a percentage chance of a packet being dropped,
        if the previous packet was dropped
    - set the number of consecutive packets to drop every time.
        set this using a negative value for multiple_packets

    \param percentage        The random percentage of packets to not transmit
    \param multiple_packets  A negative number indicates the number of consecutive
                             packets to drop. \br 0, or a positive number indicates the
                             percentage chance a packet should drop after the last
                             packet was dropped. */
bool appTestScoFwdForceDroppedPackets(unsigned percentage_to_drop, int multiple_packets);

/*! \brief Requests that the L2CAP link used for SCO forwarding is connected.

    \returns FALSE if the device is not paired to another earbud, TRUE otherwise
 */
bool appTestScoFwdConnect(void);

/*! \brief Requests that the L2CAP link used for SCO forwarding is disconnected.

    \returns TRUE
 */
bool appTestScoFwdDisconnect(void);

/*! \brief Selects the local microphone for MIC forwarding

    Preconditions
    - in an HFP call
    - MIC forwarding enabled in the build
    - function called on the device connected to the handset

    \returns TRUE if the preconditions are met, FALSE otherwise

    \note will return TRUE even if the local MIC is currently selected
 */
bool appTestMicFwdLocalMic(void);

/*! \brief Selects the remote (forwarded) microphone for MIC forwarding

    Preconditions
    - in an HFP call
    - MIC forwarding enabled in the build
    - function called on the device connected to the handset

    \returns TRUE if the preconditions are met, FALSE otherwise

    \note will return TRUE even if the remote MIC is currently selected
 */
bool appTestMicFwdRemoteMic(void);

/*! \brief configure the Pts device as a peer device */
bool appTestPtsSetAsPeer(void);

/*! \brief Are we running in PTS test mode */
bool appTestIsPtsMode(void);

/*! \brief Set or clear PTS test mode */
void appTestSetPtsMode(bool);

/*! \brief Determine if appConfigScoForwardingEnabled is TRUE
    \return bool Return value of appConfigScoForwardingEnabled.
*/
bool appTestIsScoFwdIncluded(void);

/*! \brief Determine if appConfigMicForwardingEnabled is TRUE.
    \return bool Return value of appConfigMicForwardingEnabled.
*/
bool appTestIsMicFwdIncluded(void);

/*! \brief Initiate earbud handset handover */
void appTestInitiateHandover(void);


/*! Handler for connection library messages
    This function is called to handle connection library messages used for testing.
    If a message is processed then the function returns TRUE.

    \param  id              Identifier of the connection library message
    \param  message         The message content (if any)
    \param  already_handled Indication whether this message has been processed by
                            another module. The handler may choose to ignore certain
                            messages if they have already been handled.

    \returns TRUE if the message has been processed, otherwise returns the
        value in already_handled
 */
extern bool appTestHandleConnectionLibraryMessages(MessageId id, Message message,
                                                   bool already_handled);

/*! Set flag so that DFU mode is entered when the device next goes "in case"

    This test function injects the Logical Input (i.e. mimics the UI button press event)
    corresponding to "enter DFU mode when the Earbuds are next placed in the case".

    The DFU mode will be requested only if the peer earbud is connected and
    the earbud is out of the case.

    \param unused - kept for backwards compatibility

    \return TRUE if DFU mode was requested, FALSE otherwise.
*/
bool appTestEnterDfuWhenEnteringCase(bool unused);

/*! Has the application infrastructure initialised

    When starting, the application initialises the modules it uses.
    This function checks if the sequence of module initialisation has completed

    \note When this function returns TRUE it does not indicate that the application
    is fully initialised. That would depend on the application state, and the
    status of the device.

    \returns TRUE if initialisation completed
 */
bool appTestIsInitialisationCompleted(void);

/*! Determine if the Earbud is currently primary.

    \return TRUE Earbud is Primary or Acting Primary. FALSE in all
             other cases

    \note A return value of FALSE for one Earbud DOES NOT imply it is
        secondary. See \ref appTestIsSecondary
*/
bool appTestIsPrimary(void);

/*! Determine if the Earbud is the Right Earbud.

    \return TRUE Earbud is the Right Earbud. FALSE Earbud is the Left Earbud
*/
bool appTestIsRight(void);

typedef enum
{
    app_test_topology_role_none,
    app_test_topology_role_dfu,
    app_test_topology_role_any_primary,
    app_test_topology_role_primary,
    app_test_topology_role_acting_primary,
    app_test_topology_role_secondary,
} app_test_topology_role_enum_t;

/*! Check if the earbud topology is in the specified role

    The roles are specified in the test API. If toplogy 
    is modified, the test code may need to change but uses in 
    tests should not.

    \return TRUE The topology is in the specified role
*/
bool appTestIsTopologyRole(app_test_topology_role_enum_t role);


/*! Test function to report if the Topology is in a stable state.

    The intention is to use this API between tests. Other than
    the topology being in the No Role state, the implementation 
    is not defined here. */
bool appTestIsTopologyIdle(void);


/*! Test function to report if the Topology is running.

    \return TRUE if topology is active, FALSE otherwise.
 */
bool appTestIsTopologyRunning(void);


/*! Check if the application state machine is in the specified role

    The states are defined in earbud_sm.h, and can be accessed from 
    python by name - example
    \code
      apps1.fw.env.enums["appState"]["APP_STATE_IN_CASE_DFU"])
    \endcode

    \return TRUE The application state machine is in the specified state
*/
bool appTestIsApplicationState(appState checked_state);

/*! Check if peer find role is running

    \return TRUE if peer find role is looking for a peer, FALSE
            otherwise.
 */
bool appTestIsPeerFindRoleRunning(void);

/*! Check if peer pair is active

    \return TRUE if peer pair is trying to pair, FALSE
            otherwise.
 */
bool appTestIsPeerPairLeRunning(void);

/*! Report the contents of the Device Database. */
void EarbudTest_DeviceDatabaseReport(void);

void EarbudTest_ConnectHandset(void);
void EarbudTest_DisconnectHandset(void);

/*! Report if the primary bluetooth address originated with this board

    When devices are paired one of the two addresses is selected as the
    primary address. This process is effectively random. This function 
    indicates if the address chosen is that programmed into this device.

    The log will contain the actual primary bluetooth address.

    To find out if the device is using the primary address, use the 
    test command appTestIsPrimary().

    The Bluetooth Address can be retrieved by non test commands.
    Depending on the test automation it may not be possible to use that 
    technique (shown here)

    \code
        >>> prim_addr = apps1.fw.call.new("bdaddr")
        >>> apps1.fw.call.appDeviceGetPrimaryBdAddr(prim_addr.address)
        True
        >>> prim_addr
         |-bdaddr  : struct
         |   |-uint32 lap : 0x0000ff0d
         |   |-uint8 uap : 0x5b
         |   |-uint16 nap : 0x0002
        >>>
    \endcode

    \returns TRUE if the primary address is that originally assigned to 
        this board, FALSE otherwise
        FALSE
*/
bool appTestPrimaryAddressIsFromThisBoard(void);

/*! \brief Override the score used in role selection.                      
                                                                       
    Setting a non-zero value will use that value rather than the    
    calculated one for future role selection decisions.             
   
    Setting a value of 0, will cancel the override and revert to    
    using the calculated score.

    If roles have already been selected, this will cause a re-selection.
    
    \param score Score to use for role selection decisions.         
*/                                                                  
void EarbudTest_PeerFindRoleOverrideScore(uint16 score);

/*! \brief Check to determine whether peer signalling is connected.

    Used to determine whether a peer device is connected

    \returns TRUE if peer signalling is connected, else FALSE.
*/
bool EarbudTest_IsPeerSignallingConnected(void);

/*! \brief Check to determine whether peer signalling is disconnected.

    Used to determine whether a peer device is disconnected, note this is not necessarily the same as
    !EarbudTest_IsPeerSignallingConnected() due to intermediate peer signalling states

    \returns TRUE if peer signalling is disconnected, else FALSE.
*/
bool EarbudTest_IsPeerSignallingDisconnected(void);

/*! \brief Get RSSI threshold values.

    \returns pointer of the structure hdma_thresholds_t which holds data.
*/
const hdma_thresholds_t *appTestGetRSSIthreshold(void);

/*! \brief Get MIC threshold values.

    \returns pointer of the structure hdma_thresholds_t which holds data.
*/
const hdma_thresholds_t *appTestGetMICthreshold(void);

/*! Resets the PSKEY used to track the state of an upgrade 

    \return TRUE if the store was cleared
*/
bool appTestUpgradeResetState(void);

/*! Puts the Earbud into the In Case DFU state ready to test a DFU session

    This API also puts the peer Earbud in DFU mode, though will not put it
    In Case.
*/
void EarbudTest_EnterInCaseDfu(void);


/*! Set a test number to be displayed through appTestWriteMarker

    This is intended for use in test output. Set test_number to 
    zero to not display.

    \param  test_number The testcase number to use in output

    \return The test number. This can give output on the pydbg console 
        as well as the output.
*/
uint16 appTestSetTestNumber(uint16 test_number);


/*! Set a test iteration to be displayed through appTestWriteMarker

    This is intended for use in test output. Set test_iteration to 
    zero to not display.

    \param  test_iteration The test iteration number to use in output

    \return The test iteration. This can give output on the pydbg console 
        as well as the output.
*/
uint16 appTestSetTestIteration(uint16 test_iteration);


/*! Write a marker to output

    This is intended for use in test output

    \param  marker The value to include in the marker. A marker of 0 will
        write details of the testcase and iteration set through
        appTestSetTestNumber() and appTestSetTestIteration().

    \return The marker value. This can give output on the pydbg console 
        as well as the output.
*/
uint16 appTestWriteMarker(uint16 marker);


/*! \brief Send the VA Tap command
*/
void appTestVaTap(void);

/*! \brief Send the VA Double Tap command
*/
void appTestVaDoubleTap(void);

/*! \brief Send the VA Press and Hold command
*/
void appTestVaPressAndHold(void);

/*! \brief Send the VA Release command
*/
void appTestVaRelease(void);

void appTestCvcPassthrough(void);

/*! \brief  Set the ANC Enable state in the Earbud. */
void EarbudTest_SetAncEnable(void);

/*! \brief  Set the ANC Disable state in the Earbud. */
void EarbudTest_SetAncDisable(void);

/*! \brief  Set the ANC Enable/Disable state in both Earbuds. */
void EarbudTest_SetAncToggleOnOff(void);

/*! \brief  Set the ANC mode in both Earbuds.
    \param  mode Mode need to be given by user to set the dedicated mode.
*/
void EarbudTest_SetAncMode(anc_mode_t mode);

/*! \brief  To get the current ANC State in both Earbuds.
    \return bool TRUE if ANC enabled
                 else FALSE
*/

/*! \brief  Set the ANC next mode in both Earbuds. */
void EarbudTest_SetAncNextMode(void);

/*! \brief  To get the current ANC State in both Earbuds.
    \return bool TRUE if ANC enabled
                 else FALSE
*/
bool EarbudTest_GetAncstate(void);

/*! \brief  To get the current ANC mode in both Earbuds.
    \return Return the current ANC mode.
*/
anc_mode_t EarbudTest_GetAncMode(void);

/*! \brief  To inject ANC GAIA command for obtaining ANC state
*/
void EarbudTest_GAIACommandGetAncState(void);

/*! \brief  To inject ANC GAIA command for enabling ANC
*/
void EarbudTest_GAIACommandSetAncEnable(void);

/*! \brief  To inject ANC GAIA command for disabling ANC
*/
void EarbudTest_GAIACommandSetAncDisable(void);

/*! \brief  To inject ANC GAIA command to obtain number of ANC modes supported
*/
void EarbudTest_GAIACommandGetAncNumOfModes(void);

/*! \brief  To inject ANC GAIA command to change ANC Mode
*/
void EarbudTest_GAIACommandSetAncMode(uint8 mode);

/*! \brief  To inject ANC GAIA command to obtain ANC Leakthrough gain for current mode
*/
void EarbudTest_GAIACommandGetAncLeakthroughGain(void);

/*! \brief  To inject ANC GAIA command to set ANC lekathrough gain for current mode
*/
void EarbudTest_GAIACommandSetAncLeakthroughGain(uint8 gain);

/*! \brief Sets Leakthrough gain of ANC H/W for current mode
    \param gain Leakthrough gain to be set
*/
void EarbudTest_SetAncLeakthroughGain(uint8 gain);

/*! \brief Obtains Leakthrough gain of ANC H/W for current gain
    \return uint8 gain Leakthrough gain
*/
uint8 EarbudTest_GetAncLeakthroughGain(void);

/*! \brief  Enable leak-through in the Earbud. */
void EarbudTest_EnableLeakthrough(void);

/*! \brief  Disable leak-through in the Earbud. */
void EarbudTest_DisableLeakthrough(void);

/*! \brief  Toggle leak-through state (Enable or On/Disable or Off) in the Earbud. */
void EarbudTest_ToggleLeakthroughOnOff(void);

/*! \brief  Set the leak-through mode in the Earbud.
    \param  leakthrough_mode Mode need to be given by user to set the dedicated mode. valid values are LEAKTHROUGH_MODE_1,
    LEAKTHROUGH_MODE_2 and LEAKTHROUGH_MODE_3
*/
void EarbudTest_SetLeakthroughMode(leakthrough_mode_t leakthrough_mode);

/*! \brief  Set the next leak-through mode in the Earbud. */
void EarbudTest_SetNextLeakthroughMode(void);

/*! \brief Get the leak-through mode in the Earbud.
    \return Return the current leak-through mode.
*/
leakthrough_mode_t EarbudTest_GetLeakthroughMode(void);

/*! \brief Get the leak-through status (enabled or disabled) in the Earbud.
    \return bool TRUE if leak-through is enabled
                    else FALSE
*/
bool EarbudTest_IsLeakthroughEnabled(void);

/*! \brief Set the device to have a fixed role
*/
void appTestSetFixedRole(peer_find_role_fixed_role_t role);

/*! \brief To get current HFP codec
    The return value follows this enum
    typedef enum
    {
        NO_SCO,
        SCO_NB,
        SCO_WB,
        SCO_SWB,
        SCO_UWB
    } appKymeraScoMode;
*/
appKymeraScoMode appTestGetHfpCodec(void);


#ifdef INCLUDE_ACCESSORY

/*! \brief Send an app launch request
    \param app_name The name of the application to launch, in reverse DNS notation.
*/
void appTestRequestAppLaunch(const char * app_name);

/*! \brief Check the accessory connected state
    \return bool TRUE if accessory is connected, else FALSE
*/
bool appTestIsAccessoryConnected(void);

#endif /* INCLUDE_ACCESSORY */

/*! \brief Get the current audio source volume

    \return the current audio source volume represented in audio source volume steps
*/
int appTestGetCurrentAudioVolume(void);

/*! \brief Get the current voice source volume

    \return the current audio source volume represented in voice source volume steps
*/
int appTestGetCurrentVoiceVolume(void);

/*! \brief Set the active voice assistant to GAA
*/
void EarbudTest_SetActiveVa2GAA(void);

/*! \brief Set the active voice assistant to AMA
*/
void EarbudTest_SetActiveVa2AMA(void);

/*! \brief VA audio function to be used outside a voice assistant protocol
           Simulates a real case of PTT or TTT in terms of audio
           Starts the capture.
    \return bool TRUE on success, FALSE otherwise
*/
bool EarbudTest_StartVaCapture(va_audio_codec_t encoder);

/*! \brief VA audio function to be used outside a voice assistant protocol
           Simulates a real case of PTT or TTT in terms of audio
           Stops the capture.
    \return bool TRUE on success, FALSE otherwise
*/
bool EarbudTest_StopVaCapture(void);

/*! \brief VA audio function to be used outside a voice assistant protocol
           Simulates a real case of WuW detection.
           Starts the detection process.
    \return bool TRUE on success, FALSE otherwise
*/
bool EarbudTest_StartVaWakeUpWordDetection(va_wuw_engine_t wuw_engine, bool start_capture_on_detection, va_audio_codec_t encoder);

/*! \brief VA audio function to be used outside a voice assistant protocol
           Simulates a real case of WuW detection.
           Stops the detection process.
    \return bool TRUE on success, FALSE otherwise
*/
bool EarbudTest_StopVaWakeUpWordDetection(void);

/*! Checks whether there is an active BREDR connection.

    \return TRUE if there is a \b connected BREDR link. Any other cases,
        including links in the process of disconnecting and connecting
        are reported as FALSE
 */
bool appTestAnyBredrConnection(void);


/*! Checks if the BREDR Scan Manager is enabled for page or inquiry scan

    \return TRUE if either is enabled
 */
bool appTestIsBredrScanEnabled(void);

/*! Checks if the device test service is supported and enabled

    \see appTestIsDeviceTestServiceEnable to enable or disable
        the service (IF supported in this build)

    \return TRUE if the device test service is supported, and is 
        enabled, FALSE otherwise
 */
bool appTestIsDeviceTestServiceEnabled(void);


/*! Checks if the device test service is usable or in use

    This covers whether the service has been activated and is
    either connected, or connectable.

    \return TRUE if the device test service is currently active,
    FALSE otherwise.
 */
bool appTestIsDeviceTestServiceActive(void);


/*! Checks if the device test service has been authenticated

    \return TRUE if the service is activated, or does not
            need activation, FALSE in all other cases, including
            when authentication is in progress
 */
bool appTestIsDeviceTestServiceAuthenticated(void);


/*! Update the configuration key to enable or disable the 
    device test service

    The configuration will take effect the next time the device
    test service is activated, normally when the device
    restarts.

    \note This function is currently defined as taking a boolean.
        This may be changed in future to allow values other than 
        zero or one.  Zero will always mean disable

    \param enable   TRUE to enable the device test service, or
        FALSE to disable. \see the note.

    \return TRUE if the configuration value now matches the request,
            FALSE otherwise.
 */
bool appTestDeviceTestServiceEnable(bool enable);


/*! Find out how many devices have been used by the device test
   service.

   The device test service tracks devices that it is responsible
   for creating or using. This function allows an application to
   discover how many, and if requested, which devices.

    \param[out] devices Address of a pointer than can be populated
                with an array of devices. NULL is permitted.
                If the pointer is populated after the call then 
                the application is responsible for freeing the 
                memory

    \return The number of devices found. 0 is possible.
 */
unsigned appTestDeviceTestServiceDevices(device_t **devices);

/*!
 * \brief appTestIsPreparingToSleep
 * This returns true if the appplication state indicates the device
 * is about to sleep
 * \return true or false as per description.
 */
bool appTestIsPreparingToSleep(void);

#endif /* EARBUD_TEST_H */
