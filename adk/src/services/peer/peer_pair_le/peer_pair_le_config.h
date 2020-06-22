/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Configuration related definitions for the LE pairing service .

\todo       Defines came from BREDR pairing and may not all be relevant, used, described
            correctly.
*/

#ifndef PEER_PAIR_LE_CONFIG_H_
#define PEER_PAIR_LE_CONFIG_H_

#include <message.h>

#ifdef CF133_BATT
/*! The minimum RSSI to pair with a device. This limits pairing to devices 
    that are in close proximity. */
#define appConfigPeerPairLeMinRssi() (-60)
#else
/*! The minimum RSSI to pair with a device. This limits pairing to devices 
    that are in close proximity. */
#define appConfigPeerPairLeMinRssi() (-50)
#endif

/*! Minimum difference in RSSI if multiple devices discovered
    during peer pairing discovery. This is used if there are 
    at least two devices above the RSSI threshold. 

    If the RSSI of the top two devices is within this threshold, 
    then peer pairing discovery will restart. */
#define appConfigPeerPairLeMinRssiDelta() (10)

/*! Timeout in seconds for user initiated peer pairing */
#define appConfigPeerPairLeTimeout()       (120)

/*! The time to delay before connecting to a discovered device.
    This applies from the first advert seen for any device and
    allows additional devices to be discovered. The first device
    might not be the one wanted. */
#define appConfigPeerPairLeTimeoutPeerSelect()  D_SEC(2)


/*! The milliseconds to delay from server completing pairing to initiating
    disconnection. Normally, the client will disconnect from the server
    before this timeout expires. */
#define appConfigPeerPairLeTimeoutServerCompleteDisconnect() 500


#endif /* PEER_PAIR_LE_CONFIG_H_ */
