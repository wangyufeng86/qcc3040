/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for the gaia framework core plugin
*/

#include "gaia_core_plugin.h"

#include <logging.h>
#include <panic.h>

#include "gaia_framework_feature.h"
#include "device_info.h"
#include "power_manager.h"
#include "charger_monitor.h"


/*! \brief Function pointer definition for the command handler

    \param pdu_id      PDU specific ID for the message

    \param length      Length of the payload

    \param payload     Payload data
*/
static void gaiaCorePlugin_MainHandler(uint8 pdu_id, uint8 payload_length, const uint8 *payload);

static void gaiaCorePlugin_GetApiVersion(void);
static void gaiaCorePlugin_GetSupportedFeatures(void);
static void gaiaCorePlugin_GetSupportedFeaturesNext(void);
static void gaiaCorePlugin_GetSerialNumber(void);
static void gaiaCorePlugin_GetVariant(void);
static void gaiaCorePlugin_GetApplicationVersion(void);
static void gaiaCorePlugin_DeviceReset(void);
static void gaiaCorePlugin_RegisterNotification(uint8 payload_length, const uint8 *payload);
static void gaiaCorePlugin_UnresgisterNotification(uint8 payload_length, const uint8 *payload);

/*! \brief Function pointer definition for the command handler

    \param payload_length  Length of the payload

    \param payload         Payload

    \return  True if successfull
*/
static bool gaiaCorePlugin_GetSupportedFeaturesPayload(uint8 payload_length, uint8 *payload);

/*! \brief Function that sends all available notifications
*/
static void gaiaCorePlugin_SendAllNotifications(void);

/*! \brief Function that sends all available notifications
*/
static void gaiaCorePlugin_SendChargerStatusNotification(bool plugged);

/*! \brief Gaia core plugin function to be registered as an observer of charger messages

    \param Task     Task passed to the registration function

    \param id       Messages id sent over from the charger

    \param message  Message
*/
static void gaiaCorePlugin_ChargerTask(Task task, MessageId message_id, Message message);

static bool charger_client_is_registered = FALSE;

static TaskData gaia_core_plugin_task;

static bool current_charger_plugged_in_state = FALSE;


void GaiaCorePlugin_Init(void)
{
    DEBUG_LOG("GaiaCorePlugin_Init");

    GaiaFramework_RegisterFeature(GAIA_CORE_FEATURE_ID, GAIA_CORE_PLUGIN_VERSION, gaiaCorePlugin_MainHandler, gaiaCorePlugin_SendAllNotifications);

    /* Register the core gaia plugin as an observer for charger messages */
    gaia_core_plugin_task.handler = gaiaCorePlugin_ChargerTask;
    charger_client_is_registered = appChargerClientRegister((Task)&gaia_core_plugin_task);
}

static void gaiaCorePlugin_MainHandler(uint8 pdu_id, uint8 payload_length, const uint8 *payload)
{
    DEBUG_LOG("gaiaCorePlugin_MainHandler called for %d", pdu_id);

    switch (pdu_id)
    {
        case get_api_version:
            gaiaCorePlugin_GetApiVersion();
            break;

        case get_supported_features:
            gaiaCorePlugin_GetSupportedFeatures();
            break;

        case get_supported_features_next:
            gaiaCorePlugin_GetSupportedFeaturesNext();
            break;

        case get_serial_number:
            gaiaCorePlugin_GetSerialNumber();
            break;

        case get_variant:
            gaiaCorePlugin_GetVariant();
            break;

        case get_application_version:
            gaiaCorePlugin_GetApplicationVersion();
            break;

        case device_reset:
            gaiaCorePlugin_DeviceReset();
            break;

        case register_notification:
            gaiaCorePlugin_RegisterNotification(payload_length, payload);
            break;

        case unresgister_notification:
            gaiaCorePlugin_UnresgisterNotification(payload_length, payload);
            break;

        default:
            DEBUG_LOG("gaiaCorePlugin_MainHandler UNHANDLED call for %d", pdu_id);
            GaiaFramework_SendError(GAIA_CORE_FEATURE_ID, pdu_id, command_not_supported);
            break;
    }
}

static void gaiaCorePlugin_GetApiVersion(void)
{
    uint8 value[2] = { GAIA_V3_VERSION_MAJOR, GAIA_V3_VERSION_MINOR };

    DEBUG_LOG("gaiaCorePlugin_GetApiVersion");

    GaiaFramework_SendResponse(GAIA_CORE_FEATURE_ID, get_api_version, 2, value);
}

static void gaiaCorePlugin_GetSupportedFeatures(void)
{
    uint8 response_payload_length;
    uint8 *response_payload = NULL;
    uint8 number_of_registered_features;
    uint8 more_to_come = 0;
    bool failed = FALSE;

    DEBUG_LOG("gaiaCorePlugin_GetSupportedFeatures");

    number_of_registered_features = GaiaFrameworkFeature_GetNumberOfRegisteredFeatures();

    response_payload_length = (number_of_registered_features * 2) + 1;

    if (response_payload_length > 0)
    {
        response_payload = PanicNull(malloc(response_payload_length * sizeof(uint8)));

        response_payload[0] = more_to_come;

        if (gaiaCorePlugin_GetSupportedFeaturesPayload((response_payload_length - 1), &response_payload[1]))
        {
            GaiaFramework_SendResponse(GAIA_CORE_FEATURE_ID, get_supported_features, response_payload_length, response_payload);
        }
        else
        {
            failed = TRUE;
        }

        free(response_payload);
    }

    if (failed)
    {
        DEBUG_LOG("gaiaCorePlugin_GetSupportedFeatures, FAILED");
        GaiaFramework_SendError(GAIA_CORE_FEATURE_ID, get_supported_features, 0);
    }
}

static void gaiaCorePlugin_GetSupportedFeaturesNext(void)
{
    DEBUG_LOG("gaiaCorePlugin_GetSupportedFeaturesNext");
    GaiaFramework_SendError(GAIA_CORE_FEATURE_ID, get_supported_features_next, command_not_supported);
}

static void gaiaCorePlugin_GetSerialNumber(void)
{
    const char * response_payload = DeviceInfo_GetSerialNumber();
    uint8 response_payload_length = strlen(response_payload);

    DEBUG_LOG("gaiaCorePlugin_GetSerialNumber");

    GaiaFramework_SendResponse(GAIA_CORE_FEATURE_ID, get_serial_number, response_payload_length, (uint8 *)response_payload);
}

static void gaiaCorePlugin_GetVariant(void)
{
    const char * response_payload = DeviceInfo_GetName();
    uint8 response_payload_length = strlen(response_payload);

    DEBUG_LOG("gaiaCorePlugin_GetVariant");

    GaiaFramework_SendResponse(GAIA_CORE_FEATURE_ID, get_variant, response_payload_length, (uint8 *)response_payload);
}

static void gaiaCorePlugin_GetApplicationVersion(void)
{
    const char * response_payload = DeviceInfo_GetHardwareVersion();
    uint8 response_payload_length = strlen(response_payload);

    DEBUG_LOG("gaiaCorePlugin_GetApplicationVersion");

    GaiaFramework_SendResponse(GAIA_CORE_FEATURE_ID, get_application_version, response_payload_length, (uint8 *)response_payload);
}

static void gaiaCorePlugin_DeviceReset(void)
{
    DEBUG_LOG("gaiaCorePlugin_DeviceReset");

    GaiaFramework_SendResponse(GAIA_CORE_FEATURE_ID, device_reset, 0, NULL);

    appPowerReboot();
}

static void gaiaCorePlugin_RegisterNotification(uint8 payload_length, const uint8 *payload)
{
    bool error = TRUE;

    DEBUG_LOG("gaiaCorePlugin_RegisterNotification");

    if (payload_length > 0)
    {
        if (GaiaFrameworkFeature_RegisterForNotifications(payload[0]))
        {
            GaiaFramework_SendResponse(GAIA_CORE_FEATURE_ID, register_notification, 0, NULL);
            GaiaFrameworkFeature_SendAllNotifications(payload[0]);
            error = FALSE;
        }
    }

    if (error)
    {
        DEBUG_LOG("gaiaCorePlugin_RegisterNotification, feature %d is not registered with the framework", payload[0]);
        GaiaFramework_SendError(GAIA_CORE_FEATURE_ID, register_notification, 0);
    }
}

static void gaiaCorePlugin_UnresgisterNotification(uint8 payload_length, const uint8 *payload)
{
    bool error = TRUE;

    DEBUG_LOG("gaiaCorePlugin_UnresgisterNotification");

    if (payload_length > 0)
    {
        if (GaiaFrameworkFeature_UnregisterForNotifications(payload[0]))
        {
            GaiaFramework_SendResponse(GAIA_CORE_FEATURE_ID, unresgister_notification, 0, NULL);
            error = FALSE;
        }
    }

    if (error)
    {
        DEBUG_LOG("gaiaCorePlugin_UnresgisterNotification, feature %d is not registered with the framework", payload[0]);
        GaiaFramework_SendError(GAIA_CORE_FEATURE_ID, unresgister_notification, 0);
    }
}

static void gaiaCorePlugin_SendAllNotifications(void)
{
    DEBUG_LOG("gaiaCorePlugin_SendAllNotifications");

    if (charger_client_is_registered)
    {
        switch(appChargerIsConnected())
        {
            case CHARGER_CONNECTED:
            case CHARGER_CONNECTED_NO_ERROR:
                current_charger_plugged_in_state = TRUE;
                break;

            case CHARGER_DISCONNECTED:
            default:
                current_charger_plugged_in_state = FALSE;
        }

        gaiaCorePlugin_SendChargerStatusNotification(current_charger_plugged_in_state);
    }
}

static void gaiaCorePlugin_SendChargerStatusNotification(bool plugged)
{
    uint8 payload = (uint8)plugged;

    DEBUG_LOG("gaiaCorePlugin_SendChargerStatusNotification");
    GaiaFramework_SendNotification(GAIA_CORE_FEATURE_ID, charger_status_notification, 1, &payload);
}

static void gaiaCorePlugin_ChargerTask(Task task, MessageId message_id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    bool charger_plugged = current_charger_plugged_in_state;

    switch(message_id)
    {
        case CHARGER_MESSAGE_DETACHED:
            charger_plugged = FALSE;
            break;

        case CHARGER_MESSAGE_ATTACHED:
        case CHARGER_MESSAGE_COMPLETED:
        case CHARGER_MESSAGE_CHARGING_OK:
        case CHARGER_MESSAGE_CHARGING_LOW:
            charger_plugged = TRUE;
            break;

        default:
            DEBUG_LOG("gaiaCorePlugin_ChargerTask, Unknown charger message");
            break;
    }

    if (charger_plugged != current_charger_plugged_in_state)
    {
        current_charger_plugged_in_state = charger_plugged;
        gaiaCorePlugin_SendChargerStatusNotification(current_charger_plugged_in_state);
    }
}

static bool gaiaCorePlugin_GetSupportedFeaturesPayload(uint8 payload_length, uint8 *payload)
{
    uint8 payload_index;
    feature_list_handle_t * handle = NULL;
    bool success = TRUE;

    DEBUG_LOG("gaiaCorePlugin_GetSupportedFeaturesPayload");

    for (payload_index = 0; payload_index < payload_length; payload_index+=2 )
    {
        handle = GaiaFrameworkFeature_GetNextHandle(handle);

        if (handle != NULL)
        {
            success = GaiaFrameworkFeature_GetFeatureIdAndVersion(handle, &payload[payload_index], &payload[payload_index + 1]);
        }
        else
        {
            DEBUG_LOG("gaiaCorePlugin_GetSupportedFeaturesPayload, FAILED");
            success = FALSE;
        }

        if (!success)
        {
            break;
        }
    }

    return success;
}
