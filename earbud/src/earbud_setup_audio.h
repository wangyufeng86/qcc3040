/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    earbud
\brief      Pre and post init audio setup.
 
Functions to configure audio according to the application needs.
 
*/

#ifndef EARBUD_SETUP_AUDIO_H_
#define EARBUD_SETUP_AUDIO_H_

#include <csrtypes.h>
#include <message.h>

/*@{*/

/*! \brief Configures downloadable capabilities.

    It must be called before appKymeraInit().
*/
void Earbud_SetBundlesConfig(void);

/*! \brief Post init audio configuration.

    It is setting up the audio chain configuration.

    It must be called after appKymeraInit(), but before the audio is used.
*/
void Earbud_SetupAudio(void);

/*@}*/

#endif /* EARBUD_SETUP_AUDIO_H_ */
