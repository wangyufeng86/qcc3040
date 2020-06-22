// *****************************************************************************
// Copyright (c) 2007 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef _HARM100_LIB_H
#define _HARM100_LIB_H

// -----------------------------------------------------------------------------
// Constants - internal
// -----------------------------------------------------------------------------
.CONST $harm100.DELAY_LENGTH                 27;

// -----------------------------------------------------------------------------
// Data Object
// -----------------------------------------------------------------------------

// @DATA_OBJECT HARMONICITY_OBJECT

// @DOC_FIELD_TEXT Pointer to input frame of time signal 'x'.
// @DOC_FIELD_FORMAT Pointer
.CONST $harm100.INP_X_FIELD                  MK1 * 0;

// @DOC_FIELD_TEXT Pointer to Variant Flag (0/1/2/3/4 - nb/wb/uwb/sb/fb)
// @DOC_FIELD_FORMAT Flag Pointer
.CONST $harm100.VARIANT_FIELD                MK1 * 1;

// @DOC_FIELD_TEXT Pointer to history buffer, nb/wb require 3 frames, ub 4 frames
// @DOC_FIELD_FORMAT Pointer
.CONST $harm100.HISTORY_FIELD                MK1 * 2;

// @DOC_FIELD_TEXT Flag to turn on/off the process, flag set is on, flag clear is off.
// @DOC_FIELD_FORMAT Flag
.CONST $harm100.FLAG_BYPASS_FIELD            MK1 * 3;

// Internal fields
.CONST $harm100.HARM_VALUE_FIELD             MK1 * 4;
.CONST $harm100.FRAME_SIZE_FIELD             MK1 * 5;
.CONST $harm100.DELAY_START_FIELD            MK1 * 6;
.CONST $harm100.DECIMATION_FIELD             MK1 * 7;
.CONST $harm100.NUM_FRAME_FIELD              MK1 * 8;
.CONST $harm100.CURRENT_FRAME_FIELD          MK1 * 9;
.CONST $harm100.WINDOW_LENGTH_FIELD          MK1 * 10;
.CONST $harm100.AMDF_MEM_START_FIELD         MK1 * 11;

.CONST $harm100.STRUC_SIZE                   ($harm100.AMDF_MEM_START_FIELD  >> LOG2_ADDR_PER_WORD) + $harm100.DELAY_LENGTH;

// @END  DATA_OBJECT HARMONICITY_OBJECT

#endif // _HARM100_LIB_H
