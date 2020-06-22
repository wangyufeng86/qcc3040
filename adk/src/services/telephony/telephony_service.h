/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   telephony_service Telephony Service
\ingroup    services
\brief      The Telephony service provides a high level API for performing Telephony actions on a voice source,
            through the use of a sources telephony control interface.

The Telephony service uses \ref audio_domain Audio domain.
*/

#ifndef TELEPHONY_SERVICE_H_
#define TELEPHONY_SERVICE_H_

#include <message.h>

/*\{*/

/*! \brief Initialises the telephony service.

    \param init_task Not used

    \return TRUE
 */
bool TelephonyService_Init(Task init_task);

/*\}*/

#endif /* TELEPHONY_SERVICE_H_ */
