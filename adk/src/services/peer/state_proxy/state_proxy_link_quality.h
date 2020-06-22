/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Link quality measurement
*/

#ifndef STATE_PROXY_LINK_QUALITY_H
#define STATE_PROXY_LINK_QUALITY_H

#include "connection.h"

/*  \brief Kick link quality code to start/stop measurements based on changes
    to external state. Link quality measurements will be taken if links are
    connected and there are clients registered for link quality events.
*/
void stateProxy_LinkQualityKick(void);

/*! \brief Handle link quality measurement result
    \param The result.
*/
void stateProxy_HandleClDmLinkQualityBdaddrCfm(const CL_DM_LINK_QUALITY_BDADDR_CFM_T *cfm);

/*! \brief Handle interval timer for measuring link quality.
*/
void stateProxy_HandleIntervalTimerLinkQuality(void);

/*! \brief Handle remote link quality message */
void stateProxy_HandleRemoteLinkQuality(const STATE_PROXY_LINK_QUALITY_T *msg);

#endif /* STATE_PROXY_LINK_QUALITY_H */
