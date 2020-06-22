/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   audio_sources Audio Sources
\ingroup    audio_domain
\brief      The audio interface component provides generic API to register and get audio sources, and controls.
*/

#ifndef AUDIO_SOURCES_INTERFACE_H_
#define AUDIO_SOURCES_INTERFACE_H_

#include "audio_sources_list.h"

/*\{*/

/*! \brief Audio Interface Type */
typedef struct
{
    void ** interfaces;
    uint8 number_of_interfaces;
} interface_list_t;

/*! \brief Supported audio interface types */
typedef enum
{
    /*!< Audio source registration  interface enumeration */
    audio_interface_type_audio_source_registry = 0,
    /*!< Media control interface enumeration */
    audio_interface_type_media_control,
    /*!< Volume registration interface enumeration */
    audio_interface_type_volume_registry,
    /*!< Volume registration interface enumeration */
    audio_interface_type_volume_control,
    /*!< Volume observer registration interface enumeration */
    audio_interface_type_observer_registry,
    /*!< Maximum number of interface type */
    audio_interface_type_max
} audio_interface_types_t;


/*! \brief Interface initialisation function
*/
void AudioInterface_Init(void);

/*! \brief Interface registration function

    \param source Audio source type to be registered

    \param interface_type Audio interface type to be registered

    \param interface The interface to be registered
*/
void AudioInterface_Register(audio_source_t source, audio_interface_types_t interface_type, const void * interface);

/*! \brief Interface unregistration function

    \param source Audio source type to be unregistered

    \param interface_type Audio interface type to be unregistered

    \param interface The interface to be unregistered
*/
void AudioInterface_UnRegister(audio_source_t source, audio_interface_types_t interface_type, const void * interface);

/*! \brief Interface obtaining function

    \param source Audio source type

    \param interface_type Audio interface type

    \param[out] num_interfaces Number of registerd interfaces for the given interface_type

    \return Pointer to the registered interface list
*/
interface_list_t AudioInterface_Get(audio_source_t source, audio_interface_types_t interface_type);


/*\}*/

#endif /* AUDIO_SOURCES_INTERFACE_H_ */
