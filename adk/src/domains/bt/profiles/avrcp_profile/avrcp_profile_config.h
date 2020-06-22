/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       avrcp_profile_config.h
\brief      Configuration related definitions for AVRCP state machine.
*/

#ifndef AVRCP_PROFILE_CONFIG_H_
#define AVRCP_PROFILE_CONFIG_H_

/*! \brief Time between volume changes (in milliseconds) */
#define AVRCP_CONFIG_VOLUME_REPEAT_TIME               (300)

/*! Qualcomm IEEE company ID */
#define appConfigIeeeCompanyId()  (0x00025BUL)

/*! Time to wait for AVRCP playback status notification after sending an AVRCP
 *  passthrough command that would change the playback status */
#define appConfigAvrcpPlayStatusNotificationTimeout() D_SEC(1)

#endif /* AVRCP_PROFILE_CONFIG_H_ */
