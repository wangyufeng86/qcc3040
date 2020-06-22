#ifndef AMA_CONFIG_H
#define AMA_CONFIG_H

#include "ama_data.h"

#define AMA_CONFIG_DEVICE_TYPE          "A32E8VQVU960EJ"
#define AMA_CONFIG_TEMP_SERIAL_NUMBER   "19348"             /* Arbitary - only used if problem retrieving serial number */
#define AMA_DEFAULT_CODEC_OVER_RFCOMM   ama_codec_msbc      /* Compile time choice between ama_codec_msbc or ama_codec_opus */
#define AMA_DEFAULT_CODEC_OVER_IAP2     ama_codec_msbc      /* Compile time choice between ama_codec_msbc or ama_codec_opus */
#define AMA_DEFAULT_OPUS_CODEC_BIT_RATE AMA_OPUS_16KBPS     /* Compile time choice between AMA_OPUS_16KBPS or AMA_OPUS_32KBPS */

#endif // AMA_CONFIG_H
