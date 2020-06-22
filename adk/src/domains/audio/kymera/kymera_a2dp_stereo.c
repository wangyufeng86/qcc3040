/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_a2dp_stereo.c
\brief      Kymera A2DP for stereo
*/

#ifdef INCLUDE_STEREO
#include <operators.h>

#include "init.h"
#include "kymera_private.h"
#include "kymera_a2dp_private.h"
#include "kymera_config.h"
#include "av.h"
#include "a2dp_profile_config.h"

static void appKymeraPreStartSanity(kymeraTaskData *theKymera)
{
    /* Can only start streaming if we're currently idle */
    PanicFalse(appKymeraGetState() == KYMERA_STATE_IDLE);

    /* Ensure there are no audio chains already */
    PanicNotNull(theKymera->chain_input_handle);
    PanicNotNull(theKymera->chainu.output_vol_handle);
}

static void appKymeraCreateInputChain(kymeraTaskData *theKymera, uint8 seid)
{
    const chain_config_t *config = NULL;
    DEBUG_LOG("appKymeraCreateInputChain");

    switch (seid)
    {
        case AV_SEID_SBC_SNK:
            DEBUG_LOG("Create SBC input chain");
            config = Kymera_GetChainConfigs()->chain_input_sbc_stereo_config;
        break;

        default:
            Panic();
        break;
    }

    /* Create input chain */
    theKymera->chain_input_handle = PanicNull(ChainCreate(config));
}

static void appKymeraConfigureInputChain(kymeraTaskData *theKymera,
                                         uint8 seid, uint32 rate,
                                         bool cp_header_enabled)
{
    kymera_chain_handle_t chain_handle = theKymera->chain_input_handle;
    rtp_codec_type_t rtp_codec = -1;
    Operator op_rtp_decoder = ChainGetOperatorByRole(chain_handle, OPR_RTP_DECODER);
    DEBUG_LOG("appKymeraConfigureInputChain");

    switch (seid)
    {
        case AV_SEID_SBC_SNK:
            DEBUG_LOG("configure SBC input chain");
            rtp_codec = rtp_codec_type_sbc;
        break;

        default:
            Panic();
        break;
    }

    appKymeraConfigureRtpDecoder(op_rtp_decoder, rtp_codec, rtp_decode, rate, cp_header_enabled, PRE_DECODER_BUFFER_SIZE);
    ChainConnect(chain_handle);
}

static void appKymeraCreateAndConfigureOutputChain(uint8 seid, uint32 rate,
                                                   int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    unsigned kick_period = KICK_PERIOD_FAST;
    DEBUG_LOG("appKymeraCreateAndConfigureOutputChain, creating output chain, completing startup");

    switch (seid)
    {
        case AV_SEID_SBC_SNK:
            kick_period = KICK_PERIOD_MASTER_SBC;
            break;

        default :
            Panic();
            break;
    }

    theKymera->output_rate = rate;
    appKymeraCreateOutputChain(kick_period, 0, volume_in_db);
    appKymeraSetOperatorUcids(FALSE);
    appKymeraExternalAmpControl(TRUE);
}

static void appKymeraJoinInputOutputChains(kymeraTaskData *theKymera)
{
    DEBUG_LOG("appKymeraJoinInputOutputChains");
    /* Connect input and output chains together */
    PanicFalse(ChainConnectInput(theKymera->chainu.output_vol_handle,
                                 ChainGetOutput(theKymera->chain_input_handle,
                                 EPR_SOURCE_DECODED_PCM),
                                 EPR_SINK_STEREO_MIXER_L));

    PanicFalse(ChainConnectInput(theKymera->chainu.output_vol_handle,
                                 ChainGetOutput(theKymera->chain_input_handle,
                                 EPR_SOURCE_DECODED_PCM_RIGHT),
                                 EPR_SINK_STEREO_MIXER_R));
}

static void appKymeraStartChains(kymeraTaskData *theKymera, Source media_source)
{
    bool connected;

    DEBUG_LOG("appKymeraStartChains");
    /* Start the output chain regardless of whether the source was connected
    to the input chain. Failing to do so would mean audio would be unable
    to play a tone. This would cause kymera to lock, since it would never
    receive a KYMERA_OP_MSG_ID_TONE_END and the kymera lock would never
    be cleared. */
    ChainStart(theKymera->chainu.output_vol_handle);
    /* The media source may fail to connect to the input chain if the source
    disconnects between the time A2DP asks Kymera to start and this
    function being called. A2DP will subsequently ask Kymera to stop. */
    connected = ChainConnectInput(theKymera->chain_input_handle, media_source, EPR_SINK_MEDIA);
    if (connected)
    {
        ChainStart(theKymera->chain_input_handle);
    }
}

static bool appKymeraA2dpStartStereo(const a2dp_codec_settings *codec_settings, uint32 max_bitrate, int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    UNUSED(max_bitrate);
    bool cp_header_enabled;
    uint32 rate;
    uint8 seid;
    Source media_source;
    uint16 mtu;

    DEBUG_LOG("appKymeraA2dpStartHeadset");
    appKymeraGetA2dpCodecSettingsCore(codec_settings, &seid, &media_source, &rate, &cp_header_enabled, &mtu);

    switch (appKymeraGetState())
    {
        /* Headset audio chains are started in one step */
        case KYMERA_STATE_A2DP_STARTING_A:
        {
            appKymeraCreateAndConfigureOutputChain(seid, rate, volume_in_db);
            
            appKymeraCreateInputChain(theKymera, seid);
            appKymeraConfigureInputChain(theKymera, seid,
                                         rate, cp_header_enabled);
            appKymeraJoinInputOutputChains(theKymera);
            appKymeraConfigureDspPowerMode();
            /* Connect media source to chain */
            StreamDisconnect(media_source, 0);
            appKymeraStartChains(theKymera, media_source);
        }
        return TRUE;

        default:
            Panic();
        return FALSE;
    }
}

void appKymeraA2dpCommonStop(Source source)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraA2dpCommonStop, source(%p)", source);

    PanicNull(theKymera->chain_input_handle);
    PanicNull(theKymera->chainu.output_vol_handle);

    /* A tone still playing at this point must be interruptable */
    appKymeraTonePromptStop();

    /* Stop chains before disconnecting */
    ChainStop(theKymera->chain_input_handle);

    /* Disable external amplifier if required */
    appKymeraExternalAmpControl(FALSE);

    /* Disconnect A2DP source from the RTP operator then dispose */
    StreamDisconnect(source, 0);
    StreamConnectDispose(source);

    /* Stop and destroy the output chain */
    appKymeraDestroyOutputChain();

    /* Destroy chains now that input has been disconnected */
    ChainDestroy(theKymera->chain_input_handle);
    theKymera->chain_input_handle = NULL;

}

bool appKymeraHandleInternalA2dpStart(const KYMERA_INTERNAL_A2DP_START_T *msg)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    uint8 seid = msg->codec_settings.seid;
    uint32 rate = msg->codec_settings.rate;

    DEBUG_LOG("appKymeraHandleInternalA2dpStart, state %u, seid %u, rate %u", appKymeraGetState(), seid, rate);

    if (appKymeraGetState() == KYMERA_STATE_TONE_PLAYING)
    {
        /* If there is a tone still playing at this point,
         * it must be an interruptable tone, so cut it off */
        appKymeraTonePromptStop();
    }

    if (appA2dpIsSeidNonTwsSink(seid))
    {
        switch (appKymeraGetState())
        {
            case KYMERA_STATE_IDLE:
            {
                appKymeraPreStartSanity(theKymera);
                theKymera->output_rate = rate;
                theKymera->a2dp_seid = seid;
                appKymeraSetState(KYMERA_STATE_A2DP_STARTING_A);
            }
            // fall-through
            case KYMERA_STATE_A2DP_STARTING_A:
            {
                if (!appKymeraA2dpStartStereo(&msg->codec_settings, msg->max_bitrate, msg->volume_in_db))
                {
                    DEBUG_LOG("appKymeraHandleInternalA2dpStart, state %u, seid %u, rate %u", appKymeraGetState(), seid, rate);
                    Panic();
                }
                /* Startup is complete, now streaming */
                appKymeraSetState(KYMERA_STATE_A2DP_STREAMING);
            }
            break;

            default:
                Panic();
            break;
        }
    }
    else
    {
        /* Unsupported SEID, control should never reach here */
        Panic();
    }
    return TRUE;
}

void appKymeraHandleInternalA2dpStop(const KYMERA_INTERNAL_A2DP_STOP_T *msg)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    uint8 seid = msg->seid;

    DEBUG_LOG("appKymeraHandleInternalA2dpStop, state %u, seid %u", appKymeraGetState(), seid);

    if (appA2dpIsSeidNonTwsSink(seid))
    {
        switch (appKymeraGetState())
        {
            case KYMERA_STATE_A2DP_STREAMING:
                appKymeraA2dpCommonStop(msg->source);
                theKymera->output_rate = 0;
                theKymera->a2dp_seid = AV_SEID_INVALID;
                appKymeraSetState(KYMERA_STATE_IDLE);
            break;

            case KYMERA_STATE_IDLE:
            break;

            default:
                // Report, but ignore attempts to stop in invalid states
                DEBUG_LOG("appKymeraHandleInternalA2dpStop, invalid state %u", appKymeraGetState());
            break;
        }
    }
    else
    {
        /* Unsupported SEID, control should never reach here */
        Panic();
    }
}

void appKymeraHandleInternalA2dpSetVolume(int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraHandleInternalA2dpSetVolume, vol %d", volume_in_db);

    switch (appKymeraGetState())
    {
        case KYMERA_STATE_A2DP_STREAMING:
            appKymeraSetMainVolume(theKymera->chainu.output_vol_handle, volume_in_db);
            break;

        default:
            break;
    }
}

#endif /* INCLUDE_STEREO */
