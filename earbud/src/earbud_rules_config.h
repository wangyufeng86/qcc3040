/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       earbud_rules_config.h
\brief      Configuration related definitions for connection rules.
*/

#ifndef EARBUD_RULES_CONFIG_H_
#define EARBUD_RULES_CONFIG_H_


/*! Only allow upgrades when the request has been made by the user (through the UI)
    and the device is in the case.
  */
#define appConfigDfuOnlyFromUiInCase()              FALSE

/*! Can BLE be used to perform upgrades when not in the case */
#define appConfigDfuAllowBleUpgradeOutOfCase()      TRUE

/*! Can BREDR be used to perform upgrades when not in the case */
#define appConfigDfuAllowBredrUpgradeOutOfCase()    TRUE

/*! Allow 2nd Earbud to connect to TWS+ Handset after pairing */
#define ALLOW_CONNECT_AFTER_PAIRING (TRUE)

/*! Allow LED indications when Earbud is in ear */
#define appConfigInEarLedsEnabled() (TRUE)

#if !defined(CF133) && !defined(CG437) && !defined(CF020)

/*! Only enable LED indications when Earbud is out of ear */
#undef appConfigInEarLedsEnabled
#define appConfigInEarLedsEnabled() (FALSE)

#endif /* !defined(CF133) */

    /********************************************
     *   SETTINGS for Bluetooth Low Energy (BLE)
     ********************************************/

/*! Define whether BLE is allowed when out of the case.

    Restricting to use in the case only will reduce power
    consumption and extend battery life. It will not be possible
    to start an upgrade or read battery information.

    \note Any existing BLE connections will not be affected
    when leaving the case.
 */
#define appConfigBleAllowedOutOfCase()          (FALSE)

#endif /* EARBUD_RULES_CONFIG_H_ */
