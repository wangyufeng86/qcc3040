// -----------------------------------------------------------------------------
// Copyright (c) 2019                  Qualcomm Technologies International, Ltd.
//
#include "usb_audio_gen_c.h"

#ifndef __GNUC__ 
_Pragma("datasection CONST")
#endif /* __GNUC__ */

static unsigned defaults_usb_audioUSB_AUDIO_RX[] = {
   0x00002080u			// CONFIG
};

unsigned *USB_AUDIO_GetDefaults(unsigned capid){
	switch(capid){
		case 0x009A: return defaults_usb_audioUSB_AUDIO_RX;
		case 0x4068: return defaults_usb_audioUSB_AUDIO_RX;
		case 0x009B: return defaults_usb_audioUSB_AUDIO_RX;
		case 0x4069: return defaults_usb_audioUSB_AUDIO_RX;
	}
	return((unsigned *)0);
}
