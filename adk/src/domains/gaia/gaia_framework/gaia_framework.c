/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for the gaia framework API
*/

#include "gaia_framework.h"

#include <logging.h>
#include <panic.h>

#include "gaia_framework_command.h"
#include "gaia_framework_feature.h"
#include "gaia_core_plugin.h"


bool GaiaFramework_Init(Task init_task)
{
    DEBUG_LOG("GaiaFramework_Init");

    GaiaFrameworkCommand_ResetVendorSpecificHandler();

    GaiaFrameworkFeature_Init();

    GaiaCorePlugin_Init();

    return GaiaFrameworkInternal_Init(init_task);
}

void GaiaFramework_RegisterFeature(uint8 feature_id, uint8 version_number, gaia_framework_command_handler_fn_t command_handler, gaia_framework_send_all_notifications_fn_t send_notifications)
{
    DEBUG_LOG("GaiaFramework_RegisterFeature");

    PanicFalse(GaiaFrameworkFeature_AddToList(feature_id, version_number, command_handler, send_notifications));
}

void GaiaFramework_RegisterVendorSpecificHandler(gaia_framework_vendor_specific_handler_fn_t command_handler)
{
    DEBUG_LOG("GaiaFramework_RegisterVendorSpecificHandler");

    PanicFalse(GaiaFrameworkCommand_RegisterVendorSpecificHandler(command_handler));
}

void GaiaFramework_SendResponse(uint8 feature_id, uint8 pdu_id, uint8 length, uint8 * payload)
{
    GAIA_TRANSPORT *transport = GaiaGetTransport();
    uint16 command_id;

    DEBUG_LOG("GaiaFramework_SendResponse");

    command_id = gaiaFramework_BuildCommandId(feature_id, pdu_type_response, pdu_id);

    GaiaBuildAndSendSynch(transport, GAIA_V3_VENDOR_ID, command_id, GAIA_STATUS_NONE, length, payload);
}

void GaiaFramework_SendError(uint8 feature_id, uint8 pdu_id, uint8 status_code)
{
    GAIA_TRANSPORT *transport = GaiaGetTransport();
    uint16 command_id;
    DEBUG_LOG("GaiaFramework_SendError");

    command_id = gaiaFramework_BuildCommandId(feature_id, pdu_type_error, pdu_id);

    GaiaBuildAndSendSynch(transport, GAIA_V3_VENDOR_ID, command_id, GAIA_STATUS_NONE, 1, &status_code);
}

void GaiaFramework_SendNotification(uint8 feature_id, uint8 notification_id, uint8 length, uint8 * payload)
{
    GAIA_TRANSPORT *transport = GaiaGetTransport();
    uint16 command_id;

    DEBUG_LOG("GaiaFramework_SendNotification");

    if (GaiaFrameworkFeature_IsNotificationsActive(feature_id))
    {
        command_id = gaiaFramework_BuildCommandId(feature_id, pdu_type_notification, notification_id);

        GaiaBuildAndSendSynch(transport, GAIA_V3_VENDOR_ID, command_id, GAIA_STATUS_NONE, length, payload);
    }
}

