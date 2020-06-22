// -----------------------------------------------------------------------------
// Copyright (c) 2019                  Qualcomm Technologies International, Ltd.
//
#ifndef __USB_AUDIO_GEN_C_H__
#define __USB_AUDIO_GEN_C_H__

#ifndef ParamType
typedef unsigned ParamType;
#endif

// CodeBase IDs
#define USB_AUDIO_USB_AUDIO_RX_CAP_ID	0x009A
#define USB_AUDIO_USB_AUDIO_RX_ALT_CAP_ID_0	0x4068
#define USB_AUDIO_USB_AUDIO_RX_SAMPLE_RATE	0
#define USB_AUDIO_USB_AUDIO_RX_VERSION_MAJOR	 1

#define USB_AUDIO_USB_AUDIO_TX_CAP_ID	0x009B
#define USB_AUDIO_USB_AUDIO_TX_ALT_CAP_ID_0	0x4069
#define USB_AUDIO_USB_AUDIO_TX_SAMPLE_RATE	0
#define USB_AUDIO_USB_AUDIO_TX_VERSION_MAJOR	0

// Constant Definitions


// Runtime Config Parameter Bitfields


// Statistics Block
typedef struct _tag_USB_AUDIO_STATISTICS
{
	ParamType OFFSET_SAMPLE_RATE;
	ParamType OFFSET_NUM_CHANNELS;
	ParamType OFFSET_DATA_FORMAT;
	ParamType OFFSET_BITS_PER_SAMPLE;
	ParamType OFFSET_MODE;
}USB_AUDIO_STATISTICS;

typedef USB_AUDIO_STATISTICS* LP_USB_AUDIO_STATISTICS;

// Parameters Block
typedef struct _tag_USB_AUDIO_PARAMETERS
{
	ParamType OFFSET_CONFIG;
}USB_AUDIO_PARAMETERS;

typedef USB_AUDIO_PARAMETERS* LP_USB_AUDIO_PARAMETERS;

unsigned *USB_AUDIO_GetDefaults(unsigned capid);

#endif // __USB_AUDIO_GEN_C_H__
