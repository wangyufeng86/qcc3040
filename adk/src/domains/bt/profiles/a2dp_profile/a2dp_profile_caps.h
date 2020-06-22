/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief
*/

#ifndef A2DP_PROFILE_CAPS_H_
#define A2DP_PROFILE_CAPS_H_


/*
 * Common macro definitions
 */
#define SPLIT_IN_2_OCTETS(x) ((x) >> 8)  & 0xFF, (x) & 0xFF
#define SPLIT_IN_4_OCTETS(x) (x >> 24) & 0xFF, (x >> 16) & 0xFF, (x >> 8) & 0xFF, x & 0xFF

#define DECODE_RESOURCE_ID (1)      /*!< Resource ID for endpoint definition, indicating decoding. That is, incoming audio */
#define ENCODE_RESOURCE_ID (2)      /*!< Resource ID for endpoint definition, indicating encoding. That is, outgoing audio */

/*! The maximum AAC bitrate */
#define AAC_BITRATE 264630

void appAvUpdateSbcMonoTwsCapabilities(uint8 *caps, uint32 sample_rate);
void appAvUpdateAptxMonoTwsCapabilities(uint8 *caps, uint32 sample_rate);
void appAvUpdateSbcCapabilities(uint8 *caps, uint32 sample_rate);

#endif /* A2DP_PROFILE_CAPS_H_ */
