/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version
\file       main.c
\brief      Main application task
*/

#include <os.h>
#include <panic.h>
#include <sink.h>
#include <source.h>
#include <stream.h>
#include <vmal.h>
#include <operator.h>
#include <operators.h>
#include <pio.h>
#include <cap_id_prim.h>
#include <opmsg_prim.h>

#define DAC_SAMPLE_RATE         48000

/* Create defines for volume manipulation.
 * The passthrough gain is the log2 of the required linear gain in Q6N format.
 * Convert a dB gain to Q6N as follows: 2^(32-6) * gain_db / 20log(2)
 * This can be simplified to a scaling of 2^26 / 20log2 = 67108864 / 6.0206
 */
/* Operator applies unity gain (0dB) */
#define INITIAL_OPERATOR_GAIN    GAIN_DB(0)
#define GAIN_DB_TO_Q6N_SF (11146541)
#define GAIN_DB(x)      ((int32)(GAIN_DB_TO_Q6N_SF * (x)))

#define INITIAL_GAIN    GAIN_DB(-6)

/* Local function */
static void setup_loopback(void);

#define AUDIO_CORE_0 0
#define AMP_PIO 32
#define AMP_PIO_BANK        1
#define AMP_PIO_ENABLE      1<<0

//#define USE_DOWNLOADABLE

static Operator passthrough;

#ifdef USE_DOWNLOADABLE
static BundleID bID;
static FILE_INDEX index=FILE_NONE;
static const char operator_file[] = "download_passthrough.dkcs";
#endif

int main(void)
{
    OsInit();

    /* permit audio to stream */
    OperatorFrameworkEnable(1);

    setup_loopback();

    // Enable audio amp
    PioSetFunction(AMP_PIO, PIO);
    PioSetDir32Bank(AMP_PIO_BANK, AMP_PIO_ENABLE, AMP_PIO_ENABLE);
    PioSet32Bank(AMP_PIO_BANK, AMP_PIO_ENABLE, 0);

    /* Start the message scheduler loop */
    MessageLoop();

    /* We should never get here, keep compiler happy */
    return 0;
}


static void setup_loopback(void)
{
    /* Get the input endpoints */

    Source source_line_in_left  = PanicNull(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));
    Source source_line_in_right = PanicNull(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B));

    /* ...and configure the sample rate and gain for each channel */
    PanicFalse(SourceConfigure(source_line_in_left,  STREAM_CODEC_INPUT_RATE, DAC_SAMPLE_RATE));
    PanicFalse(SourceConfigure(source_line_in_right, STREAM_CODEC_INPUT_RATE, DAC_SAMPLE_RATE));
    PanicFalse(SourceConfigure(source_line_in_left,  STREAM_CODEC_INPUT_GAIN, 9));
    PanicFalse(SourceConfigure(source_line_in_right, STREAM_CODEC_INPUT_GAIN, 9));

    PanicFalse(SourceSynchronise(source_line_in_left,source_line_in_right));

    /* Get the output endpoints */
    Sink sink_line_out_left  = PanicNull(StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));
    Sink sink_line_out_right = PanicNull(StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B));

    PanicFalse(SinkConfigure(sink_line_out_left,  STREAM_CODEC_OUTPUT_RATE, DAC_SAMPLE_RATE));
    PanicFalse(SinkConfigure(sink_line_out_right, STREAM_CODEC_OUTPUT_RATE, DAC_SAMPLE_RATE));
    PanicFalse(SinkConfigure(sink_line_out_left,  STREAM_CODEC_OUTPUT_GAIN, 15));
    PanicFalse(SinkConfigure(sink_line_out_right, STREAM_CODEC_OUTPUT_GAIN, 15));

#ifdef USE_DOWNLOADABLE
    /* Check whether the bundle file actually exists */
    index = FileFind(FILE_ROOT, operator_file, strlen(operator_file));
    if(index == FILE_NONE)Panic();
    bID = PanicZero(OperatorBundleLoad(index,AUDIO_CORE_0));
    passthrough = PanicZero(VmalOperatorCreate( CAP_ID_DOWNLOAD_PASSTHROUGH));
#else
    passthrough = PanicZero(VmalOperatorCreate( CAP_ID_BASIC_PASS ));
#endif
    uint16 set_gain[] = { OPMSG_COMMON_ID_SET_PARAMS, 1, 1, 1,
                          UINT32_MSW(INITIAL_OPERATOR_GAIN),
                          UINT32_LSW(INITIAL_OPERATOR_GAIN) };
    PanicZero(VmalOperatorMessage(passthrough, set_gain,
                                  sizeof(set_gain)/sizeof(set_gain[0]),
                                  NULL, 0));

    //audio inputs
    PanicNull(StreamConnect(source_line_in_left,  StreamSinkFromOperatorTerminal(passthrough, 0)));
    PanicNull(StreamConnect(source_line_in_right, StreamSinkFromOperatorTerminal(passthrough, 1)));


    //outputs
    PanicNull(StreamConnect(StreamSourceFromOperatorTerminal(passthrough, 0),
                            sink_line_out_left));
    PanicNull(StreamConnect(StreamSourceFromOperatorTerminal(passthrough, 1),
                            sink_line_out_right));

    /* Finally start the operator */
    Operator op_list[] = {
        passthrough,
    };
    PanicFalse(OperatorStartMultiple(sizeof(op_list)/sizeof(op_list[0]),op_list,NULL));
}
