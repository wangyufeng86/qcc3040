// *****************************************************************************
// Copyright (c) 2007 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef _STREAM_LIB_H_INCLUDED
#define _STREAM_LIB_H_INCLUDED

// -----------------------------------------------------------------------------
// CVC stream frmbuffer data structure
// -----------------------------------------------------------------------------
.CONST $cvc.stream.frmbuffer.FRAME_PTR_FIELD                MK1 * 0;
.CONST $cvc.stream.frmbuffer.FRAME_SIZE_FIELD               MK1 * 1;
.CONST $cvc.stream.frmbuffer.BUFFER_SIZE_FIELD              MK1 * 2;
.CONST $cvc.stream.frmbuffer.BUFFER_START_ADDRESS_FIELD     MK1 * 3;
.CONST $cvc.stream.frmbuffer.PEAK_VALUE_FIELD               MK1 * 4;
.CONST $cvc.stream.frmbuffer.STRUC_SIZE.NO_PEAK             4;
.CONST $cvc.stream.frmbuffer.STRUC_SIZE                     5;

#endif // _STREAM_LIB_H_INCLUDED
