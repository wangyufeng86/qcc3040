/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  cvsd.h
 *
 * CVSD public header file.
 *
 */

#ifndef CVSD_LIB_H
#define CVSD_LIB_H

#include "cvsd_struct.h"

#define SCRATCH_SIZE_WORDS 256*8

void cvsd_receive_asm(sCvsdState_t* cvsd_struct, tCbuffer* bufIn, tCbuffer* bufOut, int* ptScratch, int knSamples);
void cvsd_send_asm(sCvsdState_t* cvsd_struct, tCbuffer* bufIn, tCbuffer* bufOut, int* ptScratch, int knSamples);

#endif /* CVSD_LIB_H */
