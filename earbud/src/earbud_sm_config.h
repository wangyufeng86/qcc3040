/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       earbud_sm_config.h
\brief      Configuration related definitions for the state machine.
*/

#ifndef EARBUD_SM_CONFIG_H_
#define EARBUD_SM_CONFIG_H_


/*! Timeout for entering the case after selecting DFU from the user
    interface */
#define appConfigDfuTimeoutToPlaceInCaseMs()        (D_SEC(60))

/*! Timeout for starting DFU mode after a restart caused by an
    upgrade completing. The timeout will only apply if the device
    is out of the case.

    The timeout can be set to 0, in which case there is no limit. */
#define appConfigDfuTimeoutToStartAfterRestartMs()  (D_SEC(30))

/*! Timeout for DFU mode, entered after a reboot in DFU mode.

    This is the timeout for an abnormal restart. Restarts that occur
    as part of the normal upgrade process use
    appConfigDfuTimeoutToStartAfterRestartMs()

    The timeout can be set to 0, in which case there is no limit. */
#define appConfigDfuTimeoutToStartAfterRebootMs()   (D_SEC(15))

/*! Timeout for DFU mode, entered from UI

    This is the timeout for starting an upgrade when the user has
    requested DFU and then placed the device into the case.

    The timeout can be set to 0, in which case there is no limit. */
#define appConfigDfuTimeoutAfterEnteringCaseMs()    (D_SEC(50))

/*! Timeout for DFU mode, requested from GAIA

    This is the timeout for starting an upgrade after the GAIA
    upgrade protocol has been connected. Only applicable in the
    in case DFU mode.

    The timeout can be set to 0, in which case there is no limit.
*/
#define appConfigDfuTimeoutAfterGaiaConnectionMs()  (D_SEC(45))

/*! Timeout to detect ending a GAIA upgrade connection shortly after starting

    This can be used by a host application to check whether the upgrade
    feature is supported. This should not count as an upgrade connection.
 */
#define appConfigDfuTimeoutCheckForSupportMs()      (D_SEC(3))

/*! Time to wait for successful disconnection of links to peer and handset
 *  before forcing factory reset. */
#define appConfigFactoryResetTimeoutMs()        (5000)

/*! Time to wait for successful disconnection of links to peer and handset
 *  in terminating substate before shutdown/sleep. */
#define appConfigLinkDisconnectionTimeoutTerminatingMs() D_SEC(5)

/*! Timeout for A2DP audio when earbud removed from ear. */
#define appConfigOutOfEarA2dpTimeoutSecs()      (2)

/*! Timeout within which A2DP audio will be automatically restarted
 *  if placed back in the ear. */
#define appConfigInEarA2dpStartTimeoutSecs()    (10)

/*! Timeout for SCO audio when earbud removed from ear. */
#define appConfigOutOfEarScoTimeoutSecs()      (2)

/*! This timer is active in APP_STATE_OUT_OF_CASE_IDLE if set to a non-zero value.
    On timeout, the SM will allow sleep. */
#define appConfigIdleTimeoutMs()   D_SEC(300)

/*! Should new connections be allowed when music is being played
    or when we are in a call.

    Selecting this option
    \li reduces power consumption slightly as the advertisements neccesary
    for a connection are relatively low power,
    \li stops any distortion from connections

    \note Existing connections are not affected by this option
 */
#define appConfigBleNewConnectionsWhenBusy()   (FALSE)

#endif /* EARBUD_SM_CONFIG_H_ */
