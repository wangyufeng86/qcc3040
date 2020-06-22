/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Configuration related definitions for the HFP component.
*/

#ifndef HFP_PROFILE_CONFIG_H_
#define HFP_PROFILE_CONFIG_H_

/*! Number of volume steps to use per HFP UI volume event.
 The full volume range is 0-15 */
 #define hfpConfigGetHfpVolumeStep() (1)

/*! Default speaker gain */
#define HFP_SPEAKER_GAIN    (10)

/*! Default microphone gain */
#define HFP_MICROPHONE_GAIN (15)

/*! Auto answer call on connect */
#define HFP_CONNECT_AUTO_ANSWER

/*! Disable - Auto transfer call on connect */
#undef HFP_CONNECT_AUTO_TRANSFER

/*! Enable HF battery indicator */
#define appConfigHfpBatteryIndicatorEnabled() (1)

/*! User PSKEY to store HFP configuration */
#define PS_HFP_CONFIG           (1)

/*! Enable Super-wideband SCO */
#define appConfigScoSwbEnabled() (TRUE)

#endif /* HFP_PROFILE_CONFIG_H_ */
