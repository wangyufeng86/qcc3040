/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    a2dp_handover_policy.h

DESCRIPTION
    This file defines stream handover policy configuration
    
NOTES
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER
*/


#ifndef _A2DP_HANDOVER_POLICY_H_
#define _A2DP_HANDOVER_POLICY_H_

/*!
    @brief Used to confgiure the handover policy on the specified source

    @param src Source to configure
    @param value Value to write to STREAM_SOURCE_HANDOVER_POLICY

    @return FALSE if the request could not be performed, TRUE otherwise
*/
bool a2dpSourceConfigureHandoverPolicy(Source src, uint32 value);

#endif /* _A2DP_HANDOVER_POLICY_H_ */

