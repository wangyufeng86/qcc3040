/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for the gaia framework API
*/

#ifndef GAIA_FRAMEWORK_H_
#define GAIA_FRAMEWORK_H_

#include <gaia.h>

#include "gaia_framework_internal.h"

/*! GAIA major version number */
#define GAIA_V3_VERSION_MAJOR 3

/*! GAIA minor version number */
#define GAIA_V3_VERSION_MINOR 0

/*! \brief These are the default error codes provided by the GAIA framework
*/
typedef enum
{
    /*! An invalid Feature ID was specified */
    feature_not_supported = 0,
    /*! An invalid PDU Specific ID was specified */
    command_not_supported,
    /*! The host is not authenticated to use a Command ID or control */
    failed_not_authenticated,
    /*! The command was valid, but the device could not successfully carry out the command */
    failed_insufficient_resources,
    /*! The device is in the process of authenticating the host */
    authenticating,
    /*! An invalid parameter was used in the command */
    invalid_parameter,
    /*! The device is not in the correct state to process the command */
    incorrect_state,
    /*! The command is in progress */
    in_progress
} gaia_default_error_codes_t;

/*! \brief Function pointer definition for the command handler

    \param pdu_id      PDU specific ID for the message

    \param length      Length of the payload

    \param payload     Payload data
*/
typedef void (*gaia_framework_command_handler_fn_t)(uint8 pdu_id, uint8 payload_length, const uint8 *payload);

/*! \brief Function pointer definition for a vendor specific command handler

    \param command  The unhandled command in order to get information required out of it
*/
typedef void (*gaia_framework_vendor_specific_handler_fn_t)(GAIA_UNHANDLED_COMMAND_IND_T *command);


/*! \brief Function pointer that sends all available notifications
*/
typedef void (*gaia_framework_send_all_notifications_fn_t)(void);


/*! \brief Initialisation function for the framework

    \param  init_task Init task

    \return True if successful
*/
bool GaiaFramework_Init(Task init_task);

/*! \brief Registers a new feature to the framework

    \param  feature_id          Feature ID of the plugin to be registered

    \param  version_number      Version number of the plugin to be registered

    \param  command_handler     Command handler of the plugin to be registered

    \param  send_notifications  Sends all notifications of the registered feature. If no notifications are supported set to NULL
*/
void GaiaFramework_RegisterFeature(uint8 feature_id, uint8 version_number, gaia_framework_command_handler_fn_t command_handler, gaia_framework_send_all_notifications_fn_t send_notifications);

/*! \brief Registers a vendor specific command handler

    \param  command_handler     Vendor ID specific command handler
*/
void GaiaFramework_RegisterVendorSpecificHandler(gaia_framework_vendor_specific_handler_fn_t command_handler);

/*! \brief Creates a response for a command to be sent to the mobile application

    \param feature_id  Feature id for the plugin

    \param pdu_id      PDU specific ID for the message

    \param length      Length of the payload

    \param payload     Payload data
*/
void GaiaFramework_SendResponse(uint8 feature_id, uint8 pdu_id, uint8 length, uint8 * payload);

/*! \brief Creates a error reply for a command to be sent to the mobile application

    \param feature_id  Feature ID for the plugin

    \param pdu_id      PDU specific ID for the message

    \param length      Length of the payload

    \param payload     Payload data
*/
void GaiaFramework_SendError(uint8 feature_id, uint8 pdu_id, uint8 status_code);

/*! \brief Creates a notification to be sent to the mobile application

    \param feature_id       Feature ID for the plugin

    \param notification_id  Notification ID for the message

    \param length           Length of the payload

    \param payload          Payload data
*/
void GaiaFramework_SendNotification(uint8 feature_id, uint8 notification_id, uint8 length, uint8 * payload);


#endif /* GAIA_FRAMEWORK_H_ */
