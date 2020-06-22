/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for the gaia framework earbud plugin
*/

#ifndef EARBUD_GAIA_PLUGIN_H_
#define EARBUD_GAIA_PLUGIN_H_

#include <gaia_features.h>

#include "gaia_framework.h"


/*! \brief Gaia earbud plugin version
*/
#define EARBUD_GAIA_PLUGIN_VERSION 1


/*! \brief These are the core notifications provided by the GAIA framework
*/
typedef enum
{
    /*! The device can generate a Notification when a handover happens */
    primary_earbud_about_to_change = 0,
} earbud_plugin_notifications_t;


/*! \brief These are the handover types
*/
typedef enum
{
    /*! Static handover */
    static_handover = 0,
    /*! Dynamic handover */
    dynamic_handover
} earbud_plugin_handover_types_t;


/*! \brief Gaia earbud plugin init function
*/
void EarbudGaiaPlugin_Init(void);

/*! \brief Gaia earbud primary about to change notification function
    \param handover_type    Type of handover
    \param delay            Delay in seconds
*/
void EarbudGaiaPlugin_PrimaryAboutToChange(earbud_plugin_handover_types_t handover_type, uint8 delay);


#endif /* EARBUD_GAIA_PLUGIN_H_ */
