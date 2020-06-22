/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       scofwd_profile_config.h
\brief      Configuration related definitions for the SCO forwarding interface.
*/

#ifndef SCOFWD_PROFILE_CONFIG_H_
#define SCOFWD_PROFILE_CONFIG_H_

/*! The time to play delay added in the SCO receive path.

    A value of 40 will cause some missing / delayed packets in
    good test conditions.
    It is estimated that 60 is the lowest value that may be
    used in the real world, with random 2.4GHz interference.
    It is recommended that the final value used should be selected
    based on expected useage, and tolerance for delays vs. errors
    introduced by Packet Loss Concealment. */
#define appConfigScoFwdVoiceTtpMs()         (70)

/*! Enable SCO forwarding */
#ifdef INCLUDE_SCOFWD
#define appConfigScoForwardingEnabled()         (TRUE)
#else
#define appConfigScoForwardingEnabled()         (FALSE)
#endif

/*! Enable microphone forwarding (dependant on SCO forwarding) */
#ifdef INCLUDE_MICFWD
#define appConfigMicForwardingEnabled()         (TRUE)
#else
#define appConfigMicForwardingEnabled()         (FALSE)
#endif

/*! Number of times to retry SDP search for L2CAP PSM */
#define ScoFwdConfigSdpSearchRetryCount()   (2)

#endif /* SCOFWD_PROFILE_CONFIG_H_ */
