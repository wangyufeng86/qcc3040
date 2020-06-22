/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   volume_limit Volume Limit
\ingroup    volume_group
\brief      Provides a mechanism for limiting the output volume.
*/

#ifndef VOLUME_LIMIT_H_
#define VOLUME_LIMIT_H_

#include "volume_types.h"

#define VOLUME_LIMIT_DISABLED FULL_SCALE_VOLUME

/*\{*/

/*! \brief Sets the volume limiter.

    \param volume_limit The volume limit
 */
void Volume_SetLimit(volume_t volume_limit);

/*! \brief Removes the system volume.

 */
void Volume_RemoveLimit(void);

/*! \brief Gets the current volume limit.

    \return The current volume limit
 */
volume_t Volume_GetLimit(void);

/*! \brief Checks if there is a volume limit set.

    \return TRUE if there is a volume limit set, else FALSE
 */
bool Volume_IsLimitSet(void);

/*\}*/

#endif /* VOLUME_LIMIT_H_ */
