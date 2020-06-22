/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Production test mode
*/

#ifdef PRODUCTION_TEST_MODE

/* local includes */
#include "app_task.h"
#include "earbud_sm.h"
#include "init.h"
#include "earbud_sm_private.h"
#include "earbud_led.h"
#include "adk_log.h"
#include "earbud_config.h"
#include "earbud_tones.h"
#include "earbud_production_test.h"

/* framework includes */
#include <connection_manager.h>
#include <test.h>
#include <kymera.h>

/* dut modes - tx and rx */
typedef enum
{
    TEST_TX, 
    TEST_RX,
    TEST_DUT,
    TEST_DUT_AUDIO
} TestMode;

typedef struct
{
    TestMode    mode;
    uint16      channel;
    uint16      level;
    uint16      modFreq;
    uint16      packetType;
    uint16      length;
} DutControl;

/* Channel frequencies */
#define CHANNEL0  2402
#define CHANNEL39 2441
#define CHANNEL78 2480

/* Packet types */
#define TDH5  15
#define T2DH5 30
#define T3DH5 31

/* Packet lengths */
#define LDH5  339
#define L2DH5 679
#define L3DH5 1021

#define MAX_DUT_MODES 11

DutControl DutModes [MAX_DUT_MODES] =
{
    {TEST_DUT,0,0,0,0,0},
    {TEST_DUT_AUDIO,0,0,0,0,0},
    {TEST_TX,CHANNEL0 ,10,0,TDH5 ,LDH5},
    {TEST_TX,CHANNEL0 ,10,0,T2DH5,L2DH5},
    {TEST_TX,CHANNEL0 ,10,0,T3DH5,L3DH5},
    {TEST_TX,CHANNEL39,10,0,TDH5 ,LDH5},
    {TEST_TX,CHANNEL39,10,0,T2DH5,L2DH5},
    {TEST_TX,CHANNEL39,10,0,T3DH5,L3DH5},
    {TEST_TX,CHANNEL78,10,0,TDH5 ,LDH5},
    {TEST_TX,CHANNEL78,10,0,T2DH5,L2DH5},
    {TEST_TX,CHANNEL78,10,0,T3DH5,L3DH5},
};

static uint8 dutMode = 0;


/*! \brief Handle request to enter DUT test mode. */
void appSmHandleInternalEnterDUTTestMode(void)
{
    static uint8 audio = 0;

    DEBUG_LOG_VERBOSE("DUT Mode test audio %d", audio);

    if (audio == 0)
    {
        appSmClearUserPairing();
        appSmSetState(APP_STATE_OUT_OF_CASE_IDLE);
        ConManagerAllowHandsetConnect(TRUE);
        ConnectionEnterDutMode();
        audio = 1;
    }
    else
    {
        appKymeraTonePlay(dut_mode_tone, 0, TRUE, NULL, 0);
        MessageSendLater(SmGetTask(), SM_INTERNAL_ENTER_DUT_TEST_MODE, NULL, 1000);
    }
}

/*! \brief Handle request to enter FCC test mode. */
void appSmHandleInternalEnterProductionTestMode(void)
{
    DEBUG_LOG_VERBOSE("appSmHandleInternalEnterProductionTestMode");

    if (dutMode)
    { 
        if (dutMode > MAX_DUT_MODES)
        {
            dutMode = 0; 
			/*do nothing*/
			return;
        }
        DutControl * mode = &DutModes[dutMode-1];
        DEBUG_LOG_VERBOSE("Mode %d", mode->mode);
        switch (mode->mode)
        {
            case TEST_DUT:
                DEBUG_LOG_VERBOSE("Going to DUT mode");
                MessageSendLater(SmGetTask(), SM_INTERNAL_ENTER_DUT_TEST_MODE, NULL, 10);
                break;

            case TEST_DUT_AUDIO:
                DEBUG_LOG_VERBOSE("Going to DUT mode with audio");
                MessageSendLater(SmGetTask(), SM_INTERNAL_ENTER_DUT_TEST_MODE, NULL, 10);
                break;

            case TEST_TX:
                DEBUG_LOG_VERBOSE("Going to FCC mode");
                MessageCancelAll(SmGetTask(), SM_INTERNAL_ENTER_DUT_TEST_MODE);
                switch(mode->packetType)
                {
                    case TDH5: /* One fast flash */
                        appUiFCCDH5();
                        DEBUG_LOG_VERBOSE("DH5 Tx test");
                        break;
                    case T2DH5: /* Two fast flashes */
                        appUiFCC2DH5();
                        DEBUG_LOG_VERBOSE("2-DH5 Tx test");
                        break;
                    case T3DH5:
                    default: /* 3 fast flashes */
                        appUiFCC3DH5();
                        DEBUG_LOG_VERBOSE("3-DH5 Tx test");
                        break;
                }
                DEBUG_LOG_VERBOSE("channel %d", mode->channel);
#ifdef QCC3020_FF_ENTRY_LEVEL_AA
                TestCfgPkt(mode->packetType,mode->length);
                TestTxPower(mode->level);
                TestTxStart(mode->channel, 0, 0);
                TestTxData1(mode->channel, mode->level);
                TestCfgPkt(mode->packetType,mode->length);
#endif
                DEBUG_LOG_VERBOSE("Finish TEST_TX", mode->channel);
                /* start tx */
                break;

            default:
                break;
        }
        /* go to the next mode */
        dutMode++;
    }
    else
    {
        dutMode = 1;
    }
}

/*! \brief Request To enter Production Test mode */
void appSmEnterProductionTestMode(void)
{
    MessageSend(SmGetTask(), SM_INTERNAL_ENTER_PRODUCTION_TEST_MODE, NULL);
}

#endif /*PRODUCTION_TEST_MODE*/
