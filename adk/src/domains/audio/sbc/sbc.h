/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   sbc
\ingroup    audio_domain
\brief      Provides some helper APIs relating to SBC codec
*/

#ifndef SBC_H_
#define SBC_H_

#include <audio_sbc_encoder_params.h>

/*\{*/

/*! \brief Get SBC stream frame length.
    \param params SBC encoding parameters used.
    \return Frame length.
 */
unsigned Sbc_GetFrameLength(const sbc_encoder_params_t *params);

/*! \brief Get SBC stream bitrate.
    \param params SBC encoding parameters used.
    \return Bitrate.
 */
unsigned Sbc_GetBitrate(const sbc_encoder_params_t *params);

/*\}*/

#endif /* KYMERA_ADAPTATION_H_ */
