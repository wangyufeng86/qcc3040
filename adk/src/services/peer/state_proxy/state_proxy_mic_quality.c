/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Microphone quality measurement
*/

#include "state_proxy_private.h"
#include "state_proxy_marshal_defs.h"
#include "state_proxy_mic_quality.h"
#include "state_proxy_client_msgs.h"
#include "kymera.h"

/* Mic quality measurement interval */
#define STATE_PROXY_MIC_QUALITY_INTERVAL_MS 500

static void stateProxy_SendIntervalTimerMessage(void)
{
    MessageSendLater(stateProxy_GetTask(), STATE_PROXY_INTERNAL_TIMER_MIC_QUALITY,
                     NULL, STATE_PROXY_MIC_QUALITY_INTERVAL_MS);
}

static void stateProxy_NotifyMicQualityClients(state_proxy_source source,
                                               uint8 mic_quality)
{
    STATE_PROXY_MIC_QUALITY_T mc = { mic_quality };

    /* notify event specific clients */
    stateProxy_MsgStateProxyEventClients(source,
                                         state_proxy_event_type_mic_quality,
                                         &mc);

    stateProxy_MarshalToConnectedPeer(MARSHAL_TYPE(STATE_PROXY_MIC_QUALITY_T), &mc, sizeof(mc));
}

static void stateProxy_NextMeasurement(void)
{
    state_proxy_data_t *local = stateProxy_GetLocalData();
    local->mic_quality = appKymeraScoVoiceQuality();
    stateProxy_NotifyMicQualityClients(state_proxy_source_local, local->mic_quality);
}

void stateProxy_MicQualityKick(void)
{
    state_proxy_data_t *local = stateProxy_GetLocalData();
    bool enable = FALSE;

    if (stateProxy_AnyClientsRegisteredForEvent(state_proxy_event_type_mic_quality))
    {
        if (local->flags.sco_active)
        {
            enable = TRUE;
        }
    }
    if (enable && !stateProxy_IsMeasuringMicQuality())
    {
        stateProxy_SendIntervalTimerMessage();
        stateProxy_SetMeasuringMicQuality(TRUE);
        stateProxy_NextMeasurement();
    }
    else if (!enable && stateProxy_IsMeasuringMicQuality())
    {
        MessageCancelAll(stateProxy_GetTask(), STATE_PROXY_INTERNAL_TIMER_MIC_QUALITY);
        stateProxy_SetMeasuringMicQuality(FALSE);
        local->mic_quality = MIC_QUALITY_UNAVAILABLE;
        stateProxy_NotifyMicQualityClients(state_proxy_source_local, local->mic_quality);
    }
}

void stateProxy_HandleIntervalTimerMicQuality(void)
{
    stateProxy_SetMeasuringMicQuality(FALSE);
    stateProxy_MicQualityKick();
}

void stateProxy_HandleRemoteMicQuality(const STATE_PROXY_MIC_QUALITY_T *msg)
{
    state_proxy_data_t *remote = stateProxy_GetRemoteData();
    remote->mic_quality = msg->mic_quality;
    stateProxy_NotifyMicQualityClients(state_proxy_source_remote, msg->mic_quality);
}
