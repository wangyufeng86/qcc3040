/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Microphone quality measurement
*/

#ifndef STATE_PROXY_MIC_QUALITY_H
#define STATE_PROXY_MIC_QUALITY_H

#include "state_proxy_private.h"

/*  \brief Kick mic quality code to start/stop measurements based on changes
    to external state. Mic quality measurements will be taken if eSCO is active
    and there are clients registered for mic quality events.
*/
void stateProxy_MicQualityKick(void);

/*! \brief Handle interval timer for measuring microphone quality.
*/
void stateProxy_HandleIntervalTimerMicQuality(void);

/*! \brief Handle remote link quality message */
void stateProxy_HandleRemoteMicQuality(const STATE_PROXY_MIC_QUALITY_T *msg);

#endif /* STATE_PROXY_MIC_QUALITY_H */
