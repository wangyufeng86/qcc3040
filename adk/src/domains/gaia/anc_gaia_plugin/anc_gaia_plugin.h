/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for the gaia anc framework plugin
*/

#ifndef ANC_GAIA_PLUGIN_H_
#define ANC_GAIA_PLUGIN_H_

#include <gaia_features.h>
#include <gaia_framework.h>
#include <anc.h>


/*! \brief Gaia ANC plugin version
*/
#define ANC_GAIA_PLUGIN_VERSION 0

#define ANC_GAIA_GET_ANC_STATE_RESPONSE_PAYLOAD_LENGTH 0x01
#define ANC_GAIA_SET_ANC_STATE_PAYLOAD_LENGTH 0x01
#define ANC_GAIA_GET_ANC_NUM_OF_MODES_RESPONSE_PAYLOAD_LENGTH 0x01
#define ANC_GAIA_GET_ANC_CURRENT_MODE_RESPONSE_PAYLOAD_LENGTH 0x01
#define ANC_GAIA_SET_ANC_MODE_PAYLOAD_LENGTH 0x01
#define ANC_GAIA_GET_ANC_LEAKTHROUGH_GAIN_RESPONSE_PAYLOAD_LENGTH 0x01
#define ANC_GAIA_SET_ANC_LEAKTHROUGH_GAIN_PAYLOAD_LENGTH 0x01
#define ANC_GAIA_ANC_STATE_NOTIFICATION_PAYLOAD_LENGTH 0x01
#define ANC_GAIA_ANC_MODE_CHANGE_NOTIFICATION_PAYLOAD_LENGTH 0x01
#define ANC_GAIA_ANC_GAIN_CHANGE_NOTIFICATION_PAYLOAD_LENGTH 0x01

#define ANC_GAIA_SET_ANC_STATE_DISABLE 0x00
#define ANC_GAIA_SET_ANC_STATE_ENABLE 0x01


#define ANC_GAIA_STATE_DISABLE 0x00
#define ANC_GAIA_STATE_ENABLE 0x01

/*! \brief These are the ANC commands provided by the GAIA framework
*/
typedef enum
{
    /*! To provide state of ANC H/W of Primary earbud(Assumed that ANC state is always synchronized between earbuds) */
    anc_gaia_get_anc_state = 1,
    /*! Enables/Disables the ANC Hardware and state will be synchronized between arbuds */
    anc_gaia_set_anc_state,
    /*! Returns number of filter mode configurations supported */
    anc_gaia_get_num_anc_modes,
    /*! Returns number of filter mode configurations supported */
    anc_gaia_get_current_anc_mode,
    /*! Configures ANC HW with particular configuration of filter coefficients and other parameters, mode will be synchronoized between earbuds */
    anc_gaia_set_anc_mode,
    /*! Obtains configured leakthrough gain for current mode*/
    anc_gaia_get_configured_leakthrough_gain,
    /*! Sets Leakthrough gain for current mode, gain will be synchronized between earbuds */
    anc_gaia_set_configured_leakthrough_gain,
    /*! Total number of commands Assigning to penultimste value as enum starts from 'one'.*/
    number_of_anc_commands = anc_gaia_set_configured_leakthrough_gain,
} anc_gaia_plugin_command_ids_t;


/*! \brief These are the ANC notifications provided by the GAIA framework
*/
typedef enum
{
    /*! The device sends the notification when ANC state gets updated by the device */
    anc_gaia_anc_state_notification = 0,
    /*! The device sends the notification when ANC mode gets updated by the device */
    anc_gaia_anc_mode_change_notification,
    /*! The device sends the notification when ANC gain gets updated by the device */
    anc_gaia_anc_gain_change_notification,
    /*! Total number of commands */
    number_of_anc_notifications,
} anc_gaia_plugin_notification_ids_t;


/*! \brief Gaia Anc plugin init function
*/
void AncGaiaPlugin_Init(void);


#endif /* ANC_GAIA_PLUGIN_H_ */
