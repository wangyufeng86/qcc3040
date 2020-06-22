/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       peer_signalling_config.h
\brief      Configuration related definitions for the interface to module providing signalling to headset peer device.
*/

#ifndef PEER_SIGNALLING_CONFIG_H_
#define PEER_SIGNALLING_CONFIG_H_


/*! Inactivity timeout after which peer signalling channel will be disconnected, 0 to leave connected (in sniff) */
#define appConfigPeerSignallingChannelTimeoutSecs()   (0)

/*! Maximum number of times to try the SDP search for the peer_signalling attributes.
    After this many attempts the connection request will be failed. */
#define appConfigPeerSigSdpSearchTryLimit()         (3)


#endif /* PEER_SIGNALLING_CONFIG_H_ */
