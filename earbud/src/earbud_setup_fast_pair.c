/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Scrambled APSK.

*/

#include "earbud_setup_fast_pair.h"

#include "fast_pair.h"

#define SCRAMBLED_ASPK {0x55d4, 0xd417, 0x32da, 0x81bb, 0xbde7, 0xe2ee, 0x8a44, 0x6fd3, 0xa181, 0xda60, 0xb9b8, 0x7b16, 0x445b,\
0x7c3c, 0xb224, 0x0c35}

#ifdef INCLUDE_FAST_PAIR
static const uint16 scrambled_apsk[] = SCRAMBLED_ASPK;
#endif

void Earbud_SetupFastPair(void)
{
#ifdef INCLUDE_FAST_PAIR
    FastPair_SetPrivateKey(scrambled_apsk, sizeof(scrambled_apsk));
#endif
}
