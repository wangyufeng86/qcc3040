/*!
\copyright  Copyright (c) 2017-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera.c
\brief      Kymera Manager
*/

#include "kymera_private.h"
#include "kymera_config.h"
#include "av.h"
#include "a2dp_profile.h"
#include "scofwd_profile_config.h"
#include "kymera.h"
#include "usb_common.h"
#include "power_manager.h"
#include <vmal.h>

/*!< State data for the DSP configuration */
kymeraTaskData  app_kymera;

/*! Macro for creating messages */
#define MAKE_KYMERA_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);

static const appKymeraScoChainInfo *appKymeraScoChainTable = NULL;
static const appKymeraScoChainInfo *appKymeraScoSlaveChainTable = NULL;

static const capability_bundle_config_t *bundle_config = NULL;

static const kymera_chain_configs_t *chain_configs = NULL;

void appKymeraPromptPlay(FILE_INDEX prompt, promptFormat format, uint32 rate, rtime_t ttp,
                         bool interruptible, uint16 *client_lock, uint16 client_lock_mask)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraPromptPlay, queue prompt %d, int %u", prompt, interruptible);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_TONE_PROMPT_PLAY);
    message->tone = NULL;
    message->prompt = prompt;
    message->prompt_format = format;
    message->rate = rate;
    message->time_to_play = ttp;
    message->interruptible = interruptible;
    message->client_lock = client_lock;
    message->client_lock_mask = client_lock_mask;

    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_TONE_PROMPT_PLAY, message, &theKymera->lock);
    theKymera->tone_count++;
}

void appKymeraTonePlay(const ringtone_note *tone, rtime_t ttp, bool interruptible,
                       uint16 *client_lock, uint16 client_lock_mask)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraTonePlay, queue tone %p, int %u", tone, interruptible);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_TONE_PROMPT_PLAY);
    message->tone = tone;
    message->prompt = FILE_NONE;
    message->rate = KYMERA_TONE_GEN_RATE;
    message->time_to_play = ttp;
    message->interruptible = interruptible;
    message->client_lock = client_lock;
    message->client_lock_mask = client_lock_mask;

    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_TONE_PROMPT_PLAY, message, &theKymera->lock);
    theKymera->tone_count++;
}

void appKymeraCancelA2dpStart(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    MessageCancelAll(&theKymera->task, KYMERA_INTERNAL_A2DP_START);
}

void appKymeraA2dpStart(uint16 *client_lock, uint16 client_lock_mask,
                        const a2dp_codec_settings *codec_settings,
                        uint32 max_bitrate,
                        int16 volume_in_db, uint8 master_pre_start_delay,
                        uint8 q2q_mode, aptx_adaptive_ttp_latencies_t nq2q_ttp)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraA2dpStart, seid %u, lock %u, busy_lock %u, q2q %u", codec_settings->seid, theKymera->lock, theKymera->busy_lock, q2q_mode);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_A2DP_START);
    message->lock = client_lock;
    message->lock_mask = client_lock_mask;
    message->codec_settings = *codec_settings;
    message->volume_in_db = volume_in_db;
    message->master_pre_start_delay = master_pre_start_delay;
    message->q2q_mode = q2q_mode;
    message->nq2q_ttp = nq2q_ttp;
    message->max_bitrate = max_bitrate;
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_A2DP_START,
                             message,
                             &theKymera->lock);
}

void appKymeraA2dpStop(uint8 seid, Source source)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    MessageId mid = appA2dpIsSeidSource(seid) ? KYMERA_INTERNAL_A2DP_STOP_FORWARDING :
                                                KYMERA_INTERNAL_A2DP_STOP;
    DEBUG_LOG("appKymeraA2dpStop, seid %u", seid);

    /*Cancel any pending KYMERA_INTERNAL_A2DP_AUDIO_SYNCHRONISED message.
      A2DP might have been stopped while Audio Synchronization is still incomplete, 
      in which case this timed message needs to be cancelled.
     */
    MessageCancelAll(&theKymera->task, KYMERA_INTERNAL_A2DP_AUDIO_SYNCHRONISED);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_A2DP_STOP);
    message->seid = seid;
    message->source = source;
    MessageSendConditionally(&theKymera->task, mid, message, &theKymera->lock);
}

void appKymeraA2dpSetVolume(int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraA2dpSetVolume, volume %u", volume_in_db);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_A2DP_SET_VOL);
    message->volume_in_db = volume_in_db;
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_A2DP_SET_VOL, message, &theKymera->lock);
}

void appKymeraScoStartForwarding(Sink forwarding_sink, bool enable_mic_fwd)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraScoStartForwarding, queue sink %p, state %u", forwarding_sink, appKymeraGetState());
    PanicNull(forwarding_sink);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_SCO_START_FORWARDING_TX);
    message->forwarding_sink = forwarding_sink;
    message->enable_mic_fwd = enable_mic_fwd;
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_SCO_START_FORWARDING_TX, message, &theKymera->lock);
}

void appKymeraScoStopForwarding(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraScoStopForwarding, state %u", appKymeraGetState());

    if (!appKymeraHandleInternalScoForwardingStopTx())
        MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_SCO_STOP_FORWARDING_TX, NULL, &theKymera->lock);
}


kymera_chain_handle_t appKymeraScoCreateChain(const appKymeraScoChainInfo *info)
{    
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraCreateScoChain, mode %u, sco_fwd %u, mic_fwd %u, cvc_2_mic %u, rate %u", 
               info->mode, info->sco_fwd, info->mic_fwd, info->cvc_2_mic, info->rate);

    theKymera->sco_info = info;

    /* Create chain and return handle */
    theKymera->chainu.sco_handle = ChainCreate(info->chain);

    /* Configure DSP power mode appropriately for SCO chain */
    appKymeraConfigureDspPowerMode();

    return theKymera->chainu.sco_handle;
}

static void appKymeraScoStartHelper(Sink audio_sink, const appKymeraScoChainInfo *info, uint8 wesco,
                                    int16 volume_in_db, uint8 pre_start_delay, bool conditionally)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_SCO_START);
    PanicNull(audio_sink);

    message->audio_sink = audio_sink;
    message->wesco      = wesco;
    message->volume_in_db     = volume_in_db;
    message->pre_start_delay = pre_start_delay;
    message->sco_info   = info;
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_SCO_START, message, conditionally ? &theKymera->lock : NULL);
}

bool appKymeraScoStart(Sink audio_sink, appKymeraScoMode mode, bool *allow_scofwd, bool *allow_micfwd,
                       uint8 wesco, int16 volume_in_db, uint8 pre_start_delay)
{     
    const bool cvc_2_mic = appConfigScoMic2() != microphone_none;
    const appKymeraScoChainInfo *info = appKymeraScoFindChain(appKymeraScoChainTable,
                                                              mode, *allow_scofwd, *allow_micfwd,
                                                              cvc_2_mic);
    if (!info)
        info = appKymeraScoFindChain(appKymeraScoChainTable,
                                     mode, FALSE, FALSE, cvc_2_mic);
    
    if (info)
    {
        DEBUG_LOG("appKymeraScoStart, queue sink 0x%x", audio_sink);
        *allow_scofwd = info->sco_fwd;
        *allow_micfwd = info->mic_fwd;

        if (audio_sink)
        {
            DEBUG_LOG("appKymeraScoStart, queue sink 0x%x", audio_sink);
            appKymeraScoStartHelper(audio_sink, info, wesco, volume_in_db, pre_start_delay, TRUE);
            return TRUE;
        }
        else
        {
            DEBUG_LOG("appKymeraScoStart, invalid sink");
            return FALSE;
        }
    }
    else
    {
        DEBUG_LOG("appKymeraScoStart, failed to find suitable SCO chain");
        return FALSE;
    }
}

void appKymeraScoStop(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraScoStop");

    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_SCO_STOP, NULL, &theKymera->lock);
}

void appKymeraScoSlaveStartHelper(Source link_source, int16 volume_in_db, const appKymeraScoChainInfo *info, uint16 delay)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraScoSlaveStartHelper, delay %u", delay);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_SCO_SLAVE_START);
    message->link_source = link_source;
    message->volume_in_db = volume_in_db;
    message->sco_info = info;
    message->pre_start_delay = delay;
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_SCO_SLAVE_START, message, &theKymera->lock);
}

void appKymeraScoSlaveStart(Source link_source, int16 volume_in_db, bool allow_micfwd, uint16 pre_start_delay)
{
    DEBUG_LOG("appKymeraScoSlaveStart, source 0x%x", link_source);
    const bool cvc_2_mic = appConfigScoMic2() != microphone_none;
    
    const appKymeraScoChainInfo *info = appKymeraScoFindChain(appKymeraScoSlaveChainTable,
                                                              SCO_WB, FALSE, allow_micfwd,
                                                              cvc_2_mic);

    
    PanicNull(link_source);
    appKymeraScoSlaveStartHelper(link_source, volume_in_db, info, pre_start_delay);
}

void appKymeraScoSlaveStop(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraScoSlaveStop");

    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_SCOFWD_RX_STOP, NULL, &theKymera->lock);
}

void appKymeraScoSetVolume(int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraScoSetVolume msg, vol %u", volume_in_db);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_SCO_SET_VOL);
    message->volume_in_db = volume_in_db;

    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_SCO_SET_VOL, message, &theKymera->lock);
}

void appKymeraScoMicMute(bool mute)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraScoMicMute msg, mute %u", mute);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_SCO_MIC_MUTE);
    message->mute = mute;
    MessageSend(&theKymera->task, KYMERA_INTERNAL_SCO_MIC_MUTE, message);
}

void appKymeraScoUseLocalMic(void)
{
    /* Only do something if both EBs support MIC forwarding */
    if (appConfigMicForwardingEnabled())
    {
        kymeraTaskData *theKymera = KymeraGetTaskData();

        DEBUG_LOG("appKymeraScoUseLocalMic");

        MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_MICFWD_LOCAL_MIC, NULL, &theKymera->lock);
    }
}

void appKymeraScoUseRemoteMic(void)
{
    /* Only do something if both EBs support MIC forwarding */
    if (appConfigMicForwardingEnabled())
    {
        kymeraTaskData *theKymera = KymeraGetTaskData();

        DEBUG_LOG("appKymeraScoUseRemoteMic");

        MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_MICFWD_REMOTE_MIC, NULL, &theKymera->lock);
    }
}

static void kymera_dsp_msg_handler(MessageFromOperator *op_msg)
{
    PanicFalse(op_msg->len == KYMERA_OP_MSG_LEN);

    switch (op_msg->message[KYMERA_OP_MSG_WORD_MSG_ID])
    {
        case KYMERA_OP_MSG_ID_TONE_END:
            DEBUG_LOG("KYMERA_OP_MSG_ID_TONE_END");
            appKymeraTonePromptStop();
        break;

        default:
        break;
    }
}

void appKymeraProspectiveDspPowerOn(void)
{
    switch (appKymeraGetState())
    {
        case KYMERA_STATE_IDLE:
        case KYMERA_STATE_A2DP_STARTING_A:
        case KYMERA_STATE_A2DP_STARTING_B:
        case KYMERA_STATE_A2DP_STARTING_C:
        case KYMERA_STATE_A2DP_STARTING_SLAVE:
        case KYMERA_STATE_A2DP_STREAMING:
        case KYMERA_STATE_A2DP_STREAMING_WITH_FORWARDING:
        case KYMERA_STATE_SCO_ACTIVE:
        case KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING:
        case KYMERA_STATE_SCO_SLAVE_ACTIVE:
        case KYMERA_STATE_TONE_PLAYING:
            if (MessageCancelFirst(KymeraGetTask(), KYMERA_INTERNAL_PROSPECTIVE_POWER_OFF))
            {
                /* Already prospectively on, just re-start off timer */
                DEBUG_LOG("appKymeraProspectiveDspPowerOn already on, restart timer");
            }
            else
            {
                DEBUG_LOG("appKymeraProspectiveDspPowerOn starting");
                appPowerPerformanceProfileRequest();
                OperatorsFrameworkEnable();
                appPowerPerformanceProfileRelinquish();
            }
            MessageSendLater(KymeraGetTask(), KYMERA_INTERNAL_PROSPECTIVE_POWER_OFF, NULL,
                             appConfigProspectiveAudioOffTimeout());
        break;

        default:
        break;
    }
}

/* Handle KYMERA_INTERNAL_PROSPECTIVE_POWER_OFF - switch off DSP again */
static void appKymeraHandleProspectivePowerOff(void)
{
    DEBUG_LOG("appKymeraHandleProspectivePowerOff");
    OperatorsFrameworkDisable();
}

static void kymera_msg_handler(Task task, MessageId id, Message msg)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    UNUSED(task);
    switch (id)
    {
        case MESSAGE_FROM_OPERATOR:
            kymera_dsp_msg_handler((MessageFromOperator *)msg);
        break;

        case MESSAGE_SOURCE_EMPTY:
        break;

        case MESSAGE_STREAM_DISCONNECT:
            DEBUG_LOG("appKymera MESSAGE_STREAM_DISCONNECT");
            appKymeraTonePromptStop();
        break;

        case MESSAGE_USB_ENUMERATED:
        {
            const MESSAGE_USB_ENUMERATED_T *m = (const MESSAGE_USB_ENUMERATED_T *)msg;
            DEBUG_LOG("appkymera MESSAGE_USB_ENUMERATED.");
            KymeraAnc_TuningStart(m->sample_rate);
            break;
        }

        case MESSAGE_USB_SUSPENDED:
            DEBUG_LOG("appkymera MESSAGE_USB_SUSPENDED");
            KymeraAnc_TuningStop();
        break;

        case KYMERA_INTERNAL_A2DP_START:
        {
            const KYMERA_INTERNAL_A2DP_START_T *m = (const KYMERA_INTERNAL_A2DP_START_T *)msg;
            uint8 seid = m->codec_settings.seid;

            /* Check if we are busy (due to other chain in use) */
            if (!appA2dpIsSeidSource(seid) && theKymera->busy_lock)
            {
               /* Re-send message blocked on busy_lock */
                MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_A2DP_START);
                *message = *m;
                MessageSendConditionally(&theKymera->task, id, message, &theKymera->busy_lock);
                break;
            }

            /* If there is no pre-start delay, or during the pre-start delay, the
            start can be cancelled if there is a stop on the message queue */
            MessageId mid = appA2dpIsSeidSource(seid) ? KYMERA_INTERNAL_A2DP_STOP_FORWARDING :
                                                        KYMERA_INTERNAL_A2DP_STOP;
            if (MessageCancelFirst(&theKymera->task, mid))
            {
                /* A stop on the queue was cancelled, clear the starter's lock
                and stop starting */
                DEBUG_LOG("appKymera not starting due to queued stop, seid=%u", seid);
                if (m->lock)
                {
                    *m->lock &= ~m->lock_mask;
                }
                /* Also clear kymera's lock, since no longer starting */
                appKymeraClearStartingLock(theKymera);
                break;
            }
            if (m->master_pre_start_delay)
            {
                /* Send another message before starting kymera. */
                MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_A2DP_START);
                *message = *m;
                --message->master_pre_start_delay;
                MessageSend(&theKymera->task, id, message);
                appKymeraSetStartingLock(theKymera);
                break;
            }
        }
        // fallthrough (no message cancelled, zero master_pre_start_delay)
        case KYMERA_INTERNAL_A2DP_STARTING:
        {
            const KYMERA_INTERNAL_A2DP_START_T *m = (const KYMERA_INTERNAL_A2DP_START_T *)msg;
            if (appKymeraHandleInternalA2dpStart(m))
            {
                /* Start complete, clear locks. */
                appKymeraClearStartingLock(theKymera);
                if (m->lock)
                {
                    *m->lock &= ~m->lock_mask;
                }
            }
            else
            {
                /* Start incomplete, send another message. */
                MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_A2DP_START);
                *message = *m;
                MessageSend(&theKymera->task, KYMERA_INTERNAL_A2DP_STARTING, message);
                appKymeraSetStartingLock(theKymera);
            }
        }
        break;

        case KYMERA_INTERNAL_A2DP_STOP:
        case KYMERA_INTERNAL_A2DP_STOP_FORWARDING:
            appKymeraHandleInternalA2dpStop(msg);
        break;

        case KYMERA_INTERNAL_A2DP_SET_VOL:
        {
            KYMERA_INTERNAL_A2DP_SET_VOL_T *m = (KYMERA_INTERNAL_A2DP_SET_VOL_T *)msg;
            appKymeraHandleInternalA2dpSetVolume(m->volume_in_db);
        }
        break;

        case KYMERA_INTERNAL_SCO_START:
        {
            const KYMERA_INTERNAL_SCO_START_T *m = (const KYMERA_INTERNAL_SCO_START_T *)msg;

            if (theKymera->busy_lock)
            {
                MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_SCO_START);
                *message = *m;
               /* Another audio chain is active, re-send message blocked on busy_lock */
                MessageSendConditionally(&theKymera->task, id, message, &theKymera->busy_lock);
                break;
            }

            if (m->pre_start_delay)
            {
                /* Resends are sent unconditonally, but the lock is set blocking
                   other new messages */
                appKymeraSetStartingLock(KymeraGetTaskData());
                appKymeraScoStartHelper(m->audio_sink, m->sco_info, m->wesco, m->volume_in_db,
                                        m->pre_start_delay - 1, FALSE);
            }
            else
            {
                appKymeraHandleInternalScoStart(m->audio_sink, m->sco_info, m->wesco, m->volume_in_db);
                appKymeraClearStartingLock(KymeraGetTaskData());
            }
        }
        break;

        case KYMERA_INTERNAL_SCO_START_FORWARDING_TX:
        {
            const KYMERA_INTERNAL_SCO_START_FORWARDING_TX_T *m =
                    (const KYMERA_INTERNAL_SCO_START_FORWARDING_TX_T*)msg;
            appKymeraHandleInternalScoForwardingStartTx(m->forwarding_sink);
        }
        break;

        case KYMERA_INTERNAL_SCO_STOP_FORWARDING_TX:
        {
            appKymeraHandleInternalScoForwardingStopTx();
        }
        break;

        case KYMERA_INTERNAL_SCO_SET_VOL:
        {
            KYMERA_INTERNAL_SCO_SET_VOL_T *m = (KYMERA_INTERNAL_SCO_SET_VOL_T *)msg;
            appKymeraHandleInternalScoSetVolume(m->volume_in_db);
        }
        break;

        case KYMERA_INTERNAL_SCO_MIC_MUTE:
        {
            KYMERA_INTERNAL_SCO_MIC_MUTE_T *m = (KYMERA_INTERNAL_SCO_MIC_MUTE_T *)msg;
            appKymeraHandleInternalScoMicMute(m->mute);
        }
        break;


        case KYMERA_INTERNAL_SCO_STOP:
        {
            appKymeraHandleInternalScoStop();
        }
        break;

        case KYMERA_INTERNAL_SCO_SLAVE_START:
        {
            const KYMERA_INTERNAL_SCO_SLAVE_START_T *m = (const KYMERA_INTERNAL_SCO_SLAVE_START_T *)msg;
            if (theKymera->busy_lock)
            {
               /* Re-send message blocked on busy_lock */
                MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_SCO_SLAVE_START);
                *message = *m;
                MessageSendConditionally(&theKymera->task, id, message, &theKymera->busy_lock);
            }

#if 0
            /* If we are not idle (a pre-requisite) and this message can be delayed,
               then re-send it. The normal situation is message delays when stopping
               A2DP/AV. That is calls were issued in the right order to stop A2DP then
               start SCO receive but the number of messages required for each were
               different, leading the 2nd action to complete 1st. */
            if (   start_req->pre_start_delay
                && appKymeraGetState() != KYMERA_STATE_IDLE)
            {
                DEBUG_LOG("appKymeraHandleInternalScoForwardingStartRx, re-queueing.");
                appKymeraScoFwdStartReceiveHelper(start_req->link_source, start_req->volume,
                                                  start_req->sco_info,
                                                  start_req->pre_start_delay - 1);
                return;
            }
#endif

            else
            {
                appKymeraHandleInternalScoSlaveStart(m->link_source, m->sco_info, m->volume_in_db);
            }
        }
        break;

        case KYMERA_INTERNAL_SCOFWD_RX_STOP:
        {
            appKymeraHandleInternalScoSlaveStop();
        }
        break;

        case KYMERA_INTERNAL_TONE_PROMPT_PLAY:
            appKymeraHandleInternalTonePromptPlay(msg);
        break;

        case KYMERA_INTERNAL_MICFWD_LOCAL_MIC:
            appKymeraSwitchSelectMic(MIC_SELECTION_LOCAL);
            break;

        case KYMERA_INTERNAL_MICFWD_REMOTE_MIC:
            appKymeraSwitchSelectMic(MIC_SELECTION_REMOTE);
            break;

        case KYMERA_INTERNAL_ANC_TUNING_START:
        {
            const KYMERA_INTERNAL_ANC_TUNING_START_T *m = (const KYMERA_INTERNAL_ANC_TUNING_START_T *)msg;
            KymeraAnc_TuningCreateChain(m->usb_rate);
        }
        break;

        case KYMERA_INTERNAL_ANC_TUNING_STOP:
            KymeraAnc_TuningDestroyChain();
        break;

        case KYMERA_INTERNAL_PROSPECTIVE_POWER_OFF:
            appKymeraHandleProspectivePowerOff();
        break;

        case KYMERA_INTERNAL_AUDIO_SS_DISABLE:
            DEBUG_LOG("appKymera KYMERA_INTERNAL_AUDIO_SS_DISABLE");
            OperatorsFrameworkDisable();
            break;

#ifdef INCLUDE_MIRRORING
        case MESSAGE_SINK_AUDIO_SYNCHRONISED:
        case MESSAGE_SOURCE_AUDIO_SYNCHRONISED:
            appKymeraA2dpHandleAudioSyncStreamInd(id, msg);
        break;

        case KYMERA_INTERNAL_A2DP_DATA_SYNC_IND_TIMEOUT:
            appKymeraA2dpHandleDataSyncIndTimeout();
        break;

        case KYMERA_INTERNAL_A2DP_MESSAGE_MORE_DATA_TIMEOUT:
            appKymeraA2dpHandleMessageMoreDataTimeout();
        break;

        case KYMERA_INTERNAL_A2DP_AUDIO_SYNCHRONISED:
             appKymeraA2dpHandleAudioSynchronisedInd();
        break;

        case MESSAGE_MORE_DATA:
            appKymeraA2dpHandleMessageMoreData((const MessageMoreData *)msg);
        break;

#endif /* INCLUDE_MIRRORING */

        case KYMERA_INTERNAL_AEC_LEAKTHROUGH_CREATE_STANDALONE_CHAIN:
        {
            appKymeraSetState(KYMERA_STATE_STANDALONE_LEAKTHROUGH);
            Kymera_CreateLeakthroughChain();
        }
        break;
        case KYMERA_INTERNAL_AEC_LEAKTHROUGH_DESTROY_STANDALONE_CHAIN:
        {
            Kymera_DestroyLeakthroughChain();
            appKymeraSetState(KYMERA_STATE_IDLE);
        }
        break;
        case KYMERA_INTERNAL_LEAKTHROUGH_SIDETONE_ENABLE:
        {
            Kymera_SetMinLeakthroughSidetoneGain();
            Kymera_LeakthroughResetGainIndex();
            Kymera_LeakthroughEnableAecSideTonePath(TRUE);
            MessageSendLater(&theKymera->task,KYMERA_INTERNAL_LEAKTHROUGH_SIDETONE_GAIN_RAMPUP,NULL,ST_GAIN_RAMP_STEP_TIME_MS);
        }
        break;
        case KYMERA_INTERNAL_LEAKTHROUGH_SIDETONE_GAIN_RAMPUP:
        {
            /* Apply next step value of sidetone gain from look-up table */
            Kymera_LeakthroughStepupSTGain();
        }
        break;

        default:
            break;
    }
}

bool appKymeraInit(Task init_task)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    memset(theKymera, 0, sizeof(*theKymera));
    theKymera->task.handler = kymera_msg_handler;
    theKymera->state = KYMERA_STATE_IDLE;
    theKymera->output_rate = 0;
    theKymera->lock = theKymera->busy_lock = 0;
    theKymera->a2dp_seid = AV_SEID_INVALID;
    theKymera->tone_count = 0;
    theKymera->q2q_mode = 0;
    theKymera->enable_left_right_mix = TRUE;
    appKymeraExternalAmpSetup();
    DEBUG_LOG("appKymeraInit number of bundles %d", bundle_config->number_of_capability_bundles);
    if (bundle_config && bundle_config->number_of_capability_bundles > 0)
        ChainSetDownloadableCapabilityBundleConfig(bundle_config);
    theKymera->mic = MIC_SELECTION_LOCAL;
    Microphones_Init();

#ifdef INCLUDE_ANC_PASSTHROUGH_SUPPORT_CHAIN
    theKymera->anc_passthough_operator = INVALID_OPERATOR;
#endif

#ifdef ENABLE_AEC_LEAKTHROUGH
    Kymera_LeakthroughInit();
#endif

    UNUSED(init_task);
    return TRUE;
}

void Kymera_SetBundleConfig(const capability_bundle_config_t *config)
{
    bundle_config = config;
}

void Kymera_SetChainConfigs(const kymera_chain_configs_t *configs)
{
    chain_configs = configs;
}

const kymera_chain_configs_t *Kymera_GetChainConfigs(void)
{
    PanicNull((void *)chain_configs);
    return chain_configs;
}

void Kymera_SetScoChainTable(const appKymeraScoChainInfo *info)
{
    appKymeraScoChainTable = info;
}

void Kymera_SetScoSlaveChainTable(const appKymeraScoChainInfo *info)
{
    appKymeraScoSlaveChainTable = info;
}
