/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the audio_interface composite.
*/

#include "audio_sources_interface_registry.h"

#include <logging.h>
#include <panic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "audio_sources_audio_interface.h"
#include "audio_sources_media_control_interface.h"
#include "audio_sources_volume_interface.h"
#include "audio_sources_volume_control_interface.h"
#include "audio_sources_observer_interface.h"


/* ------------------------------ Definitions ------------------------------ */


/*! \brief Supported interfaces structure */
typedef struct
{
    /*! Audio source type */
    audio_source_t source;
    /*! Audio interface list. */
    const audio_source_audio_interface_t *audio_interface[MAX_AUDIO_INTERFACES];
    /*! Media control interface list. */
    const media_control_interface_t *media_control_interface[MAX_MEDIA_CONTROL_INTERFACES];
    /*! Volume interface list. */
    const audio_source_volume_interface_t *volume_interface[MAX_VOLUME_INTERFACES];
    /*! Volume control interface list. */
    const audio_source_volume_control_interface_t *volume_control_interface[MAX_VOLUME_CONTROL_INTERFACES];
    /*! Observer interface list. */
    const audio_source_observer_interface_t *observer_interface[MAX_OBSERVER_INTERFACES];
}  audio_interfaces_t;

typedef struct
{
    audio_interfaces_t interfaces[max_audio_sources-1];
    unsigned number_of_registered_sources;
} audio_source_interface_registry_t;


/* ------------------------------ Global Variables ------------------------------ */

static audio_source_interface_registry_t source_registry;


/* ------------------------------ Static function prototypes ------------------------------ */

static bool audioInterface_SourceIsValid(audio_source_t source);
static bool audioInterface_InterfaceTypeIsValid(audio_interface_types_t interface_type);
static interface_list_t audioInterface_GetInterfaceRegistryStorage(audio_interfaces_t * source, audio_interface_types_t interface_type);
static bool audioInterface_AdjustInterfaceList(void **interface, uint8 deleted_index, uint8 number_of_interfaces);
static unsigned audioInterface_GetRegistryIndexBySource(audio_source_t source);
static bool audioInterface_CreateInterfaceRegistryEntry(audio_interfaces_t * source, audio_interface_types_t interface_type, const void * interface);
static bool audioInterface_RemoveInterfaceRegistryEntry(audio_interfaces_t * source, audio_interface_types_t interface_type, const void *interface);
static interface_list_t audioInterface_GetRegisteredInterfaceList(audio_interfaces_t * source, audio_interface_types_t interface_type);


/* ------------------------------ API functions start here ------------------------------ */

void AudioInterface_Init(void)
{
    DEBUG_LOG("AudioInterface_Init called.");
    memset(source_registry.interfaces, 0, (max_audio_sources-1) * sizeof(audio_interfaces_t));
    source_registry.number_of_registered_sources = 0;
}

void AudioInterface_Register(audio_source_t source, audio_interface_types_t interface_type, const void * interface)
{
    unsigned registry_index;

    DEBUG_LOG("AudioInterface_Register, source: %d interface_type: %d interface: %p.", source, interface_type, interface);
    
    PanicFalse(audioInterface_SourceIsValid(source));
    PanicFalse(audioInterface_InterfaceTypeIsValid(interface_type));
    PanicNull((void *)interface);

    /* Gets/Creates a new source entry in registry */
    registry_index = audioInterface_GetRegistryIndexBySource(source);

    /* Creates a new interface entry in registry */
    if(!audioInterface_CreateInterfaceRegistryEntry(&source_registry.interfaces[registry_index], interface_type, interface))
    {
        Panic();
    }
}

void AudioInterface_UnRegister(audio_source_t source, audio_interface_types_t interface_type,const void * interface)
{
    unsigned registry_index;
    
    DEBUG_LOG("AudioInterface_UnRegister, source: %d interface_type: %d interface: %p.", source, interface_type, interface);
    
    PanicFalse(audioInterface_SourceIsValid(source));
    PanicFalse(audioInterface_InterfaceTypeIsValid(interface_type));
    PanicNull((void *)interface);

    registry_index = audioInterface_GetRegistryIndexBySource(source);
    if(audioInterface_RemoveInterfaceRegistryEntry(&source_registry.interfaces[registry_index], interface_type, interface))
    {
        DEBUG_LOG("audioInterface_RemoveInterfaceRegistryEntry, interface unregistered successfully");
    }
}

interface_list_t AudioInterface_Get(audio_source_t source, audio_interface_types_t interface_type)
{
    unsigned registry_index;

    DEBUG_LOG("AudioInterface_Get, source: %d interface_type: %d.", source, interface_type);

    PanicFalse(audioInterface_SourceIsValid(source));
    PanicFalse(audioInterface_InterfaceTypeIsValid(interface_type));

    registry_index = audioInterface_GetRegistryIndexBySource(source);

    return audioInterface_GetRegisteredInterfaceList(&source_registry.interfaces[registry_index], interface_type);
}

/* ------------------------------ Static functions start here ------------------------------ */

/*! \brief Ensures that the source passed is valid

    \param source Audio source type to be obtained

    \return True if the source is valid
*/
static bool audioInterface_SourceIsValid(audio_source_t source)
{
    bool is_valid = FALSE;

    if ((source > audio_source_none) && (source < max_audio_sources))
    {
        is_valid = TRUE;
    }
    else
    {
        DEBUG_LOG("audioInterface_SourceIsValid, invalid source.");
    }

    return is_valid;
}

/*! \brief Ensures that the interface type passed is valid

    \param interface_type Audio interface type

    \return True if the interface type is valid
*/
static bool audioInterface_InterfaceTypeIsValid(audio_interface_types_t interface_type)
{
    bool is_valid = FALSE;

    if ((interface_type >= audio_interface_type_audio_source_registry) && (interface_type <= audio_interface_type_observer_registry))
    {
        is_valid = TRUE;
    }
    else
    {
        DEBUG_LOG("audioInterface_IterfaceTypeIsValid, invalid interface type.");
    }

    return is_valid;
}

/*! \brief Gets the interface array pointer and maximum supported interfaces 

    \param source Audio source type to be obtained

    \param interface_type Audio interface type to be obtained

    \return Interface list
*/
static interface_list_t audioInterface_GetInterfaceRegistryStorage(audio_interfaces_t * source, audio_interface_types_t interface_type)
{
    interface_list_t registered_list = {NULL, 0};
    
    switch (interface_type)
    {
        case audio_interface_type_audio_source_registry:
            registered_list.interfaces = (void **) source->audio_interface;
            registered_list.number_of_interfaces = MAX_AUDIO_INTERFACES;
            break;

        case audio_interface_type_media_control:
            registered_list.interfaces = (void **) source->media_control_interface;
            registered_list.number_of_interfaces = MAX_MEDIA_CONTROL_INTERFACES;
            break;

        case audio_interface_type_volume_registry:
            registered_list.interfaces = (void **) source->volume_interface;
            registered_list.number_of_interfaces = MAX_VOLUME_INTERFACES;
        break;

        case audio_interface_type_volume_control:
            registered_list.interfaces = (void **) source->volume_control_interface;
            registered_list.number_of_interfaces = MAX_VOLUME_CONTROL_INTERFACES;
        break;

        case audio_interface_type_observer_registry:
            registered_list.interfaces = (void **) source->observer_interface;
            registered_list.number_of_interfaces = MAX_OBSERVER_INTERFACES;
        break;

        default:
            DEBUG_LOG("audioInterface_GetRegisteredInterfaceList, unsupported interface type.");
            Panic();
    }

    return registered_list;
}

/*! \brief Adjusts the registered list after removing the specified entry

    \param interface Registered interface list

    \param deleted_index Index entry to be removed

    \param number_of_interfaces Number of interfaces

    \return TRUE
*/
static bool audioInterface_AdjustInterfaceList(void **interface, uint8 deleted_index, uint8 number_of_interfaces)
{
    uint8 i = deleted_index;
    
    /* Delete index and reorder */
    while((i < number_of_interfaces - 1) && interface[i + 1])
    {
        interface[i] = interface[i + 1];
        i++;
    }

    /* Put the last entry as NULL */
    interface[i] = NULL;

    return TRUE;
}

/*! \brief Looks for a connection index based on source

    \param source Audio source type

    \return Index to the slot for the new connection
*/
static unsigned audioInterface_GetRegistryIndexBySource(audio_source_t source)
{
    unsigned index = 0;

    /* Checks if the source connection entry already exists in the registry */
    while (index < source_registry.number_of_registered_sources)
    {
        if (source_registry.interfaces[index].source == source)
        {
            return index;
        }
        index++;
    }

    /* Creates a new source connection entry in registry if the entry does not exist */
    if(source_registry.number_of_registered_sources < (max_audio_sources - 1))
    {
        source_registry.interfaces[source_registry.number_of_registered_sources].source = source;
        source_registry.number_of_registered_sources++;
    }
    else
    {
        Panic();
    }

    return (source_registry.number_of_registered_sources - 1);

}

/*! \brief Creates new interface entry in registry

    \param source Pointer to audio source data

    \param interface_type Audio interface type

    \parsm interface Pointer to the interface to be added

    \return TRUE if new interface entry is created otherwise FALSE
*/
static bool audioInterface_CreateInterfaceRegistryEntry(audio_interfaces_t * source, audio_interface_types_t interface_type, const void * interface)
{
    DEBUG_LOG("audioInterface_CreateInterfaceRegistryEntry, interface_type: %d.", interface_type);

    uint8 index = 0;

    interface_list_t registered_list = audioInterface_GetInterfaceRegistryStorage(source, interface_type);

    while(index < registered_list.number_of_interfaces)
    {
        if(!registered_list.interfaces[index])
        {
            registered_list.interfaces[index] = (void *) interface;
            return TRUE;
        }
        index++;
    }
    return FALSE;
}

/*! \brief Removes the specified interface from the registered interface list

    \param source Audio source type

    \param interface_type Audio interface type

    \param interface Pointer to the interface to be removed

    \return TRUE if interface is unregistered successfully
*/
static bool audioInterface_RemoveInterfaceRegistryEntry(audio_interfaces_t * source, audio_interface_types_t interface_type, const void *interface)
{
    uint8 index = 0;

    interface_list_t registered_list = audioInterface_GetInterfaceRegistryStorage(source, interface_type);

    while(index < registered_list.number_of_interfaces && registered_list.interfaces[index])
    {
        if(registered_list.interfaces[index] == interface)
        {
            return audioInterface_AdjustInterfaceList(registered_list.interfaces, index, registered_list.number_of_interfaces);
        }
        index++;
    }
    return FALSE;
}

/*! \brief Interface obtaining function

    \param source Audio source type to be obtained

    \param interface_type Audio interface type to be obtained

    \return Pointer to the registered interface list
*/
static interface_list_t audioInterface_GetRegisteredInterfaceList(audio_interfaces_t * source, audio_interface_types_t interface_type)
{
    DEBUG_LOG("audioInterface_GetRegisteredInterfaceList, interface_type: %d.", interface_type);
    uint8 index = 0;

    interface_list_t registered_list = audioInterface_GetInterfaceRegistryStorage(source, interface_type);

    while(index < registered_list.number_of_interfaces && registered_list.interfaces[index])
        index++;

    registered_list.number_of_interfaces = index;

    return registered_list;
}
