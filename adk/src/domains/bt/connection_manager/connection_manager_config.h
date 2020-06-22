/*!
\copyright  Copyright (c) 2018 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       connection_manager_config.h
\brief      Configuration related definitions for the connection manager state machine.
*/

#ifndef CONNECTION_MANAGER_CONFIG_H_
#define CONNECTION_MANAGER_CONFIG_H_


/*! Number of paired devices that are remembered */
#define appConfigMaxTrustedDevices()     (6)

/*! Page timeout to use as one earbud attempting connection to the other Earbud. */
#define appConfigEarbudPageTimeout()    (0x4000)

/*! Page timeout to use for connecting to any non-peer Earbud devices. */
#define appConfigDefaultPageTimeout()   (0x4000)

/*! The page timeout multiplier for Handsets after link-loss.
    Multiplier should be chosen carefully to make sure total page timeout doesn't exceed 0xFFFF */
#define appConfigHandsetLinkLossPageTimeoutMultiplier()    (4)

/*! Maximum number of Handsets can be connected to earbud at the same time */
#define appConfigMaxNumOfHandsetsCanConnect()  (1)
#endif /* CONNECTION_MANAGER_CONFIG_H_ */
