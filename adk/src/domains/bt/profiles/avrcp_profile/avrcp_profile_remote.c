/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       av_headset_av_remote.c
\brief      AV remote control/play status handling
*/

/* Only compile if AV defined */
#ifdef INCLUDE_AV

#include <panic.h>
#include <string.h>

#include "av.h"
#include "avrcp_profile_config.h"
#include "adk_log.h"
#include "avrcp_profile.h"


/*! Macro for creating messages */
#define MAKE_AV_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/*! Code assertion that can be checked at run time. This will cause a panic. */
#define assert(x)   PanicFalse(x)


static void avrcpProfile_Pause(audio_source_t source);
static void avrcpProfile_Play(audio_source_t source);
static void avrcpProfile_PlayPause(audio_source_t source);
static void avrcpProfile_Stop(audio_source_t source);
static void avrcpProfile_Forward(audio_source_t source);
static void avrcpProfile_Back(audio_source_t source);
static void avrcpProfile_FastForward(audio_source_t source, bool is_pressed);
static void avrcpProfile_Rewind(audio_source_t source, bool is_pressed);
static unsigned avrcpProfile_Context(audio_source_t source);


static const media_control_interface_t avrcp_control_interface = {
    .Play = avrcpProfile_Play,
    .Pause = avrcpProfile_Pause,
    .PlayPause = avrcpProfile_PlayPause,
    .Stop = avrcpProfile_Stop,
    .Forward = avrcpProfile_Forward,
    .Back = avrcpProfile_Back,
    .FastForward = avrcpProfile_FastForward,
    .FastRewind = avrcpProfile_Rewind,
    .NextGroup = NULL,
    .PreviousGroup = NULL,
    .Shuffle = NULL,
    .Repeat = NULL,
    .Context = avrcpProfile_Context
};

static unsigned avrcpProfile_Context(audio_source_t source)
{
    UNUSED(source);
    /* Use the context function in Av for now */
    return AvGetCurrentContext();
}

const media_control_interface_t * AvrcpProfile_GetMediaControlInterface(void)
{
    return &avrcp_control_interface;
}


/*! \brief Send remote control command

    This function is called to send an AVRCP Passthrough (remote control) command, if there are
    multiple AV instances then it will pick the instance which is an A2DP sink.
*/
static void appAvRemoteControl(avc_operation_id op_id, uint8 rstate, bool beep, uint16 repeat)
{
    avInstanceTaskData *theInst = appAvInstanceFindAvrcpForPassthrough();
    DEBUG_LOG("appAvRemoteControl");

    /* Check there is an instance to send AVRCP passthrough */
    if (theInst)
    {
        /* Send AVRCP passthrough request*/
        appAvrcpRemoteControl(theInst, op_id, rstate, beep, repeat);
    }
}

/***************************************************************************
DESCRIPTION
    Send AVRCP pause command
*/
static void avrcpProfile_Pause(audio_source_t source)
{
    UNUSED(source);

    /* Check there is an instance to send AVRCP passthrough */
    avInstanceTaskData *theInst = appAvInstanceFindAvrcpForPassthrough();
    if (theInst)
    {
        DEBUG_LOG("avrcpProfilePause");
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_PLAY_REQ);
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_PAUSE_REQ);
        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_PAUSE_REQ, NULL,
                                 &theInst->avrcp.playback_lock);
        appAvSendUiMessageId(AV_REMOTE_CONTROL);
    }

}


/***************************************************************************
DESCRIPTION
    Send AVRCP play command
*/
static void avrcpProfile_Play(audio_source_t source)
{
    UNUSED(source);
    /* Check there is an instance to send AVRCP passthrough */
    avInstanceTaskData *theInst = appAvInstanceFindAvrcpForPassthrough();
    if (theInst)
    {
        DEBUG_LOG("avrcpProfilePlay");
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_PLAY_REQ);
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_PAUSE_REQ);
        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_PLAY_REQ, NULL,
                                 &theInst->avrcp.playback_lock);
        appAvSendUiMessageId(AV_REMOTE_CONTROL);
    }
}

/***************************************************************************
DESCRIPTION
    Send AVRCP play/pause command
*/
static void avrcpProfile_PlayPause(audio_source_t source)
{
    UNUSED(source);
    /* Check there is an instance to send AVRCP passthrough */
    avInstanceTaskData *theInst = appAvInstanceFindAvrcpForPassthrough();
    if (theInst)
    {
        DEBUG_LOG("avrcpProfilePlayPause");
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_PLAY_REQ);
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_PAUSE_REQ);
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_PLAY_TOGGLE_REQ);
        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_PLAY_TOGGLE_REQ, NULL,
                                 &theInst->avrcp.playback_lock);
        appAvSendUiMessageId(AV_REMOTE_CONTROL);
    }
}


/***************************************************************************
DESCRIPTION
    Send AVRCP stop command
*/
static void avrcpProfile_Stop(audio_source_t source)
{
    UNUSED(source);
    DEBUG_LOG("avrcpProfileStop");

    appAvRemoteControl(opid_stop, 0, TRUE, 0);
    appAvRemoteControl(opid_stop, 1, FALSE, 0);

    appAvHintPlayStatus(avrcp_play_status_stopped);
}

/***************************************************************************
DESCRIPTION
    Send AVRCP forward command
*/
static void avrcpProfile_Forward(audio_source_t source)
{
    UNUSED(source);
    DEBUG_LOG("avrcpProfileForward");
    appAvRemoteControl(opid_forward, 0, TRUE, 0);
    appAvRemoteControl(opid_forward, 1, FALSE, 0);
}

/***************************************************************************
DESCRIPTION
    Send AVRCP back command
*/
static void avrcpProfile_Back(audio_source_t source)
{
    UNUSED(source);
    DEBUG_LOG("avrcpProfileBack");
    appAvRemoteControl(opid_backward, 0, TRUE, 0);
    appAvRemoteControl(opid_backward, 1, FALSE, 0);

}

/*! \brief Send Volume Up start command
*/
void appAvVolumeUpRemoteStart(void)
{
    DEBUG_LOG("appAvVolumeUpRemoteStart");
    appAvRemoteControl(opid_volume_up, 0, FALSE, AVRCP_CONFIG_VOLUME_REPEAT_TIME);
}

/*! \brief Send Volume Up stop command
*/
void appAvVolumeUpRemoteStop(void)
{
    DEBUG_LOG("appAvVolumeUpRemoteStop");
    appAvRemoteControl(opid_volume_up, 1, FALSE, AVRCP_CONFIG_VOLUME_REPEAT_TIME);
}

/*! \brief Send Volume Down start command
*/
void appAvVolumeDownRemoteStart(void)
{
    DEBUG_LOG("appAvVolumeDownRemoteStart");
    appAvRemoteControl(opid_volume_down, 0, FALSE, AVRCP_CONFIG_VOLUME_REPEAT_TIME);
}

/*! \brief Send Volume Down stop command
*/
void appAvVolumeDownRemoteStop(void)
{
    DEBUG_LOG("appAvVolumeDownRemoteStop");
    appAvRemoteControl(opid_volume_down, 1, FALSE, AVRCP_CONFIG_VOLUME_REPEAT_TIME);
}

/***************************************************************************
DESCRIPTION
    Send AVRCP Fast Forward command.
*/
static void avrcpProfile_FastForward(audio_source_t source, bool is_pressed)
{
    UNUSED(source);

    DEBUG_LOG("avrcpProfileFastForward Pressed=%d", is_pressed);

    if(is_pressed)
    {
        appAvRemoteControl(opid_fast_forward, 0, TRUE, D_SEC(1));
    }
    else
    {
        appAvRemoteControl(opid_fast_forward, 1, TRUE, 0);
    }
}


/***************************************************************************
DESCRIPTION
    Send AVRCP rewind command
*/
static void avrcpProfile_Rewind(audio_source_t source, bool is_pressed)
{
    UNUSED(source);

    DEBUG_LOG("avrcpProfileRewind Pressed=%d", is_pressed);

    if(is_pressed)
    {
        appAvRemoteControl(opid_rewind, 0, TRUE, D_SEC(1));
    }
    else
    {
        appAvRemoteControl(opid_rewind, 1, TRUE, 0);
    }
}


#endif

