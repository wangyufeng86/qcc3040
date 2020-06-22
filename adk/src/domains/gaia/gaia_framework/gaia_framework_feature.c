/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for the gaia framework API
*/

#include "gaia_framework_feature.h"

#include <logging.h>
#include <panic.h>



/*! \brief Feature list node */
struct feature_list_item
{
    /*! Feature ID of the plugin to be registered */
    uint8 feature_id;

    /*! Version number of the plugin to be registered */
    uint8 version_number;

    /*! Flag set by mobile app if it has registered for it's notifications */
    bool notifications_active;

    /*! Command handler of the plugin to be registered */
    gaia_framework_command_handler_fn_t command_handler;

    gaia_framework_send_all_notifications_fn_t send_notifications;

    /*! Next node */
    struct feature_list_item * next_item;
};


static struct feature_list_item * feature_list;
static uint8 number_of_registered_features;


/*! \brief Creates a new item for the feature list

    \param  feature_id          Feature ID of the plugin to be registered

    \param  version_number      Version number of the plugin to be registered

    \param  command_handler     Command handler of the plugin to be registered

    \param  send_notifications  Sends all notifications of the registered feature

    \return New item for the feature list containing the features interface
*/
static struct feature_list_item * gaiaFrameworkFeature_CreateFeatureListItem(uint8 feature_id, uint8 version_number, gaia_framework_command_handler_fn_t command_handler, gaia_framework_send_all_notifications_fn_t send_notifications);

/*! \brief Checks if a feature is registered

    \param  feature_id  Feautue Id for the plugin

    \return Returns the feature list entry otherwise the next available empty slot
*/
static struct feature_list_item * gaiaFrameworkFeature_FindFeature(uint8 feature_id);

/*! \brief Sets the notification flag of a speceific feature

    \param  feature_id  Feautue Id for the plugin

    \param  set         Sets or resets the notification flag

    \return True if feature is found and the flag is set to the specific value
*/
static bool gaiaFrameworkFeature_SetNotificationFlag(uint8 feature_id, bool set);


void GaiaFrameworkFeature_Init(void)
{
    DEBUG_LOG("GaiaFrameworkFeature_Init");

    feature_list = NULL;
    number_of_registered_features = 0;
}

bool GaiaFrameworkFeature_AddToList(uint8 feature_id, uint8 version_number, gaia_framework_command_handler_fn_t command_handler, gaia_framework_send_all_notifications_fn_t send_notifications)
{
    bool registered = TRUE;

    DEBUG_LOG("GaiaFramework_RegisterFeature");

    if (!gaiaFrameworkFeature_FindFeature(feature_id))
    {
        struct feature_list_item * new_feature = gaiaFrameworkFeature_CreateFeatureListItem(feature_id, version_number, command_handler, send_notifications);
        new_feature->next_item = feature_list;
        feature_list = new_feature;
        number_of_registered_features++;
    }
    else
    {
        DEBUG_LOG("GaiaFramework_RegisterFeature, Feature ID %d has already been registered", feature_id);
        registered = FALSE;
    }

    return registered;
}

bool GaiaFrameworkFeature_SendToFeature(uint8 feature_id, uint8 pdu_type, uint8 pdu_specific_id, uint8 payload_size, const uint8 *payload)
{
    bool registered = FALSE;
    struct feature_list_item * entry = NULL;

    UNUSED(pdu_type);

    DEBUG_LOG("GaiaFramework_SendToFeature of Feature %d", feature_id);

    entry = gaiaFrameworkFeature_FindFeature(feature_id);

    if (entry != NULL)
    {
        (*entry).command_handler(pdu_specific_id, payload_size, payload);
        registered = TRUE;
    }
    else
    {
        DEBUG_LOG("GaiaFramework_SendToFeature of Feature %d NOT FOUND", feature_id);
    }

    return registered;
}

bool GaiaFrameworkFeature_RegisterForNotifications(uint8 feature_id)
{
    DEBUG_LOG("GaiaFrameworkFeature_RegisterForNotifications");

    return gaiaFrameworkFeature_SetNotificationFlag(feature_id, TRUE);
}

bool GaiaFrameworkFeature_UnregisterForNotifications(uint8 feature_id)
{
    DEBUG_LOG("GaiaFrameworkFeature_UnregisterForNotifications");

    return gaiaFrameworkFeature_SetNotificationFlag(feature_id, FALSE);
}

bool GaiaFrameworkFeature_IsNotificationsActive(uint8 feature_id)
{
    bool feature_is_registered = FALSE;
    struct feature_list_item * entry = gaiaFrameworkFeature_FindFeature(feature_id);

    DEBUG_LOG("GaiaFrameworkFeature_IsNotificationsActive");

    if(entry != NULL)
    {
        feature_is_registered = entry->notifications_active;
    }

    return feature_is_registered;
}

void GaiaFrameworkFeature_SendAllNotifications(uint8 feature_id)
{
    struct feature_list_item * entry = gaiaFrameworkFeature_FindFeature(feature_id);

    DEBUG_LOG("GaiaFrameworkFeature_SendAllNotifications");

    PanicNull(entry);

    if (entry->send_notifications != NULL)
    {
        entry->send_notifications();
    }
    else
    {
        DEBUG_LOG("GaiaFrameworkFeature_SendAllNotifications failed, feature %d has no notification handler registered", feature_id);
    }

}

uint8 GaiaFrameworkFeature_GetNumberOfRegisteredFeatures(void)
{
    DEBUG_LOG("GaiaFrameworkFeature_GetNumberOfRegisteredFeatures");

    return number_of_registered_features;
}

feature_list_handle_t *GaiaFrameworkFeature_GetNextHandle(feature_list_handle_t * handle)
{
    struct feature_list_item * entry = NULL;

    DEBUG_LOG("GaiaFrameworkFeature_GetNextHandle");

    if (handle == NULL)
    {
        entry = feature_list;
    }
    else
    {
        entry = ((struct feature_list_item *)(handle))->next_item;
    }

    return (feature_list_handle_t *)entry;
}

bool GaiaFrameworkFeature_GetFeatureIdAndVersion(feature_list_handle_t *handle, uint8 *feature_id, uint8 *version_number)
{
    bool success = FALSE;
    struct feature_list_item * entry = NULL;

    DEBUG_LOG("GaiaFrameworkFeature_GetFeatureIdAndVersion");

    if(handle != NULL)
    {
        entry = (struct feature_list_item *)(handle);
        *feature_id = entry->feature_id;
        *version_number = entry->version_number;
        success = TRUE;
    }

    return success;
}

static struct feature_list_item * gaiaFrameworkFeature_CreateFeatureListItem(uint8 feature_id, uint8 version_number, gaia_framework_command_handler_fn_t command_handler, gaia_framework_send_all_notifications_fn_t send_notifications)
{
    struct feature_list_item * new_entry = NULL;

    DEBUG_LOG("gaiaFrameworkFeature_CreateFeatureListItem");

    new_entry = PanicUnlessNew(struct feature_list_item);

    new_entry->feature_id = feature_id;
    new_entry->version_number = version_number;
    new_entry->notifications_active = FALSE;
    new_entry->command_handler = command_handler;
    new_entry->send_notifications = send_notifications;
    new_entry->next_item = NULL;

    return new_entry;
}

static struct feature_list_item * gaiaFrameworkFeature_FindFeature(uint8 feature_id)
{
    struct feature_list_item * entry = NULL;

    DEBUG_LOG("gaiaFrameworkFeature_FindFeature %d", feature_id);

    entry = feature_list;

    while (entry != NULL)
    {
        if (entry->feature_id == feature_id)
        {
            break;
        }
        else
        {
            entry = entry->next_item;
        }
    }

    return entry;
}

static bool gaiaFrameworkFeature_SetNotificationFlag(uint8 feature_id, bool set)
{
    bool flag_set = FALSE;
    struct feature_list_item * entry = gaiaFrameworkFeature_FindFeature(feature_id);

    DEBUG_LOG("gaiaFrameworkFeature_SetNotificationFlag");

    if(entry != NULL)
    {
        entry->notifications_active = set;
        flag_set = TRUE;
    }

    return flag_set;
}
