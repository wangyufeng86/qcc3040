/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle Voice Assistant related internal APIs

*/

#ifndef KYMERA_VA_H_
#define KYMERA_VA_H_

#include "kymera.h"
#include "va_audio_types.h"
#include <Source.h>

/*! \brief Start capturing/encoding live mic data.
    \param params Parameters based on which the voice capture will be configured.
    \return The output of the capture chain as a mapped Source.
*/
Source Kymera_StartVaLiveCapture(const va_audio_voice_capture_params_t *params);

/*! \brief Stop capturing/encoding mic data.
*/
void Kymera_StopVaCapture(void);

/*! \brief Start Wake-Up-Word detection.
    \param wuw_detection_handler Task to receive the Wake-Up-Word detection message from audio.
    \param params Parameters based on which the Wake-Up-Word detection will be configured.
*/
void Kymera_StartVaWuwDetection(Task wuw_detection_handler, const va_audio_wuw_detection_params_t *params);

/*! \brief Stop Wake-Up-Word detection.
*/
void Kymera_StopVaWuwDetection(void);

/*! \brief Must immediately be called after a Wake-Up-Word detection message from audio.
 */
void Kymera_VaWuwDetected(void);

/*! \brief Start capturing/encoding wuw and live mic data.
           Must be called as a result of a Wake-Up-Word detection message from audio.
    \param params Parameters based on which the voice capture will be configured.
    \return The output of the capture chain as a mapped Source.
*/
Source Kymera_StartVaWuwCapture(const va_audio_wuw_capture_params_t *params);

/*! \brief Must be called when WuW is detected but ignored (capture is not started).
 */
void Kymera_IgnoreDetectedVaWuw(void);

/*! \brief Check if capture is active.
*/
bool Kymera_IsVaCaptureActive(void);

/*! \brief Check if Wake-Up-Word detection is active.
*/
bool Kymera_IsVaWuwDetectionActive(void);

#endif /* KYMERA_VA_H_ */
