/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   hfp_profile
\brief      The voice source telephony control interface implementation for HFP sources
*/

#include "hfp_profile_telephony_control.h"

#include <hfp.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>

#include "hfp_profile.h"
#include "hfp_profile_voice_source_device_mapping.h"
#include "voice_sources_telephony_control_interface.h"
#include "telephony_messages.h"
#include "scofwd_profile.h"

static void hfpProfile_IncomingCallAccept(voice_source_t source);
static void hfpProfile_IncomingCallReject(voice_source_t source);
static void hfpProfile_OngoingCallTerminate(voice_source_t source);
static void hfpProfile_OngoingCallTransferAudioToAg(voice_source_t source);
static void hfpProfile_OngoingCallTransferAudioToSelf(voice_source_t source);
static void hfpProfile_InitiateCallUsingNumber(voice_source_t source, phone_number_t number);
static void hfpProfile_InitiateVoiceDial(voice_source_t source);

static const voice_source_telephony_control_interface_t hfp_telephony_interface =
{
    .IncomingCallAccept = hfpProfile_IncomingCallAccept,
    .IncomingCallReject = hfpProfile_IncomingCallReject,
    .OngoingCallTerminate = hfpProfile_OngoingCallTerminate,
    .OngoingCallTransferAudioToAg = hfpProfile_OngoingCallTransferAudioToAg,
    .OngoingCallTransferAudioToSelf = hfpProfile_OngoingCallTransferAudioToSelf,
    .InitiateCallUsingNumber = hfpProfile_InitiateCallUsingNumber,
    .InitiateVoiceDial = hfpProfile_InitiateVoiceDial
};

static void hfpProfile_IncomingCallAccept(voice_source_t source)
{
    UNUSED(source);
    switch(appGetHfp()->state)
    {
        case HFP_STATE_DISCONNECTED:
        {
            /* Connect SLC */
            if (!appHfpConnectHandset())
            {
                Telephony_NotifyError(source);
                break;
            }
        }
        /* Fall through */

        case HFP_STATE_CONNECTED_INCOMING:
        {
            Telephony_NotifyCallAnswered(source);

            /* Send message into HFP state machine */
            MessageSendConditionally(appGetHfpTask(), HFP_INTERNAL_HFP_CALL_ACCEPT_REQ,
                                     NULL, &appHfpGetLock());
        }
        break;

        default:
            break;
    }
}

static void hfpProfile_IncomingCallReject(voice_source_t source)
{
    switch (appGetHfp()->state)
    {
        case HFP_STATE_DISCONNECTED:
        {
            /* Connect SLC */
            if (!appHfpConnectHandset())
            {
                /* Play error tone to indicate we don't have a valid address */
                Telephony_NotifyError(source);
                break;
            }
        }
        /* Fall through */

        case HFP_STATE_CONNECTED_INCOMING:
        {
            Telephony_NotifyCallRejected(source);

            /* Send message into HFP state machine */
            MessageSendConditionally(appGetHfpTask(), HFP_INTERNAL_HFP_CALL_REJECT_REQ,
                                         NULL, &appHfpGetLock());
        }
        break;

        default:
            break;
    }
}

static void hfpProfile_OngoingCallTerminate(voice_source_t source)
{
    switch (appGetHfp()->state)
    {
        case HFP_STATE_DISCONNECTED:
        {
            /* Connect SLC */
            if (!appHfpConnectHandset())
            {
                Telephony_NotifyError(source);
                break;
            }
        }
        /* Fall through */

        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_ACTIVE:
        {
            Telephony_NotifyCallTerminated(source);

            /* Send message into HFP state machine */
            MessageSendConditionally(appGetHfpTask(), HFP_INTERNAL_HFP_CALL_HANGUP_REQ,
                                     NULL, &appHfpGetLock());
        }
        break;

        default:
            break;
    }
}

static void hfpProfile_TransferCallAudio(bool transfer_to_ag)
{
    switch (appGetHfp()->state)
    {
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        {
            HFP_INTERNAL_HFP_TRANSFER_REQ_T * message = (HFP_INTERNAL_HFP_TRANSFER_REQ_T*)PanicUnlessNew(HFP_INTERNAL_HFP_TRANSFER_REQ_T);

            message->transfer_to_ag = transfer_to_ag;
            MessageSendConditionally(appGetHfpTask(), HFP_INTERNAL_HFP_TRANSFER_REQ,
                                     message, &appHfpGetLock());

            Telephony_NotifyCallAudioTransferred(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));
        }
        break;

        default:
            break;
    }
}

static void hfpProfile_OngoingCallTransferAudioToAg(voice_source_t source)
{
    UNUSED(source);
    hfpProfile_TransferCallAudio(TRUE);
}

static void hfpProfile_OngoingCallTransferAudioToSelf(voice_source_t source)
{
    UNUSED(source);
    hfpProfile_TransferCallAudio(FALSE);
}

static void hfpProfile_InitiateCallUsingNumber(voice_source_t source, phone_number_t number)
{
    switch(appGetHfp()->state)
    {
        case HFP_STATE_DISCONNECTED:
        {
            /* Connect SLC */
            if (!appHfpConnectHandset())
            {
                /* Play error tone to indicate we don't have a valid address */
                Telephony_NotifyError(source);
                break;
            }
        }
        /* Fall through */
        case HFP_STATE_CONNECTED_IDLE:
        {
            HFP_INTERNAL_NUMBER_DIAL_REQ_T * message= (HFP_INTERNAL_NUMBER_DIAL_REQ_T *)PanicNull(calloc(1,sizeof(HFP_INTERNAL_NUMBER_DIAL_REQ_T)+number.number_of_digits-1));
            message->length = number.number_of_digits;
            memmove(message->number, number.digits, number.number_of_digits);
            MessageSendConditionally(appGetHfpTask(), HFP_INTERNAL_NUMBER_DIAL_REQ,
                                             message, &appHfpGetLock());

            Telephony_NotifyCallInitiatedUsingNumber(source);
        }
        break;
        default:
            break;
    }

}

static void hfpProfile_InitiateVoiceDial(voice_source_t source)
{
    UNUSED(source);
    switch (appGetHfp()->state)
    {
        case HFP_STATE_DISCONNECTED:
        {
            /* if we don't have a HFP connection but SCO FWD is connected, it means that we are the slave */
            if(ScoFwdIsConnected())
            {
                /* we send the command across the SCO FWD OTA channel asking the master to
                    trigger the CALL VOICE */
                ScoFwdCallVoice();
                break;
            }
            /* if SCO FWD is not connected we can try to enstablish SLC and proceed */
            else if(!appHfpConnectHandset())
            {
                Telephony_NotifyError(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));
                break;
            }
        }

        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        case HFP_STATE_CONNECTED_IDLE:
        {
            /* Send message into HFP state machine */
            MessageSendConditionally(appGetHfpTask(), HFP_INTERNAL_HFP_VOICE_DIAL_REQ,
                                     NULL, &appHfpGetLock());
        }
        break;

        default:
            break;
    }

}

const voice_source_telephony_control_interface_t * HfpProfile_GetTelephonyControlInterface(void)
{
    return &hfp_telephony_interface;
}
