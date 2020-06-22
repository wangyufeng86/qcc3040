// *****************************************************************************
// Copyright (c) 2007 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION:
//
//   This header files holds the offset values defined for the AGC-alg400
//   data structure.
//  
// MODIFICATIONS:
//
//      
//
// *****************************************************************************

// *****************************************************************************
// NAME:
//    AGC(Automatic Gain Control) Library
//
// DESCRIPTION:
//    This library provides AGC API function calls
//
//    The library provides the following routines:
//        $M.agc.Initialize.func
//        $M.agc.Process.func
//
// *****************************************************************************

#ifndef AGC400_LIB_H
#define AGC400_LIB_H

.CONST $AGC400_VERSION                           0x010000;

// *****************************************************************************
// AGC Data Structure Element Offset Definitions
// 
// The application using this library must include a data block
//  of size BLOCK_SIZE.  The associated parameters should be initialized
// as defined below.
//
//******************************************************************************  
// @DATA_OBJECT PARAMETERS

// @DOC_FIELD_TEXT   initial gain value
// @DOC_FIELD_FORMAT   Q7.17 format
.CONST $agc400.Parameter.OFFSET_INITIAL_GAIN_FIELD       0*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   Target Level for AGC (PARAMETER)
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.Parameter.OFFSET_AGC_TARGET_FIELD         1*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   TC for smoothing AGC gain when increasing
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.Parameter.OFFSET_ATTACK_TC_FIELD          2*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   TC for smoothing AGC gain when decreasing
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.Parameter.OFFSET_DECAY_TC_FIELD           3*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   TC for input level adjustment when increasing
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.Parameter.OFFSET_ALPHA_A_90_FIELD         4*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   TC for input level adjustment when decreasing
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.Parameter.OFFSET_ALPHA_D_90_FIELD         5*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   max AGC gain (PARAMETER)
// @DOC_FIELD_FORMAT   Q7.17 format
.CONST $agc400.Parameter.OFFSET_G_MAX_FIELD              6*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   Threshold for AGC to compress output sample
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.Parameter.OFFSET_START_COMP_FIELD         7*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   in/out compression ratio applied when exceeding AGC TARGET  (PARAMETER)
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.Parameter.OFFSET_COMP_FIELD               8*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   input threshold for forcing vad detection
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.Parameter.OFFSET_INPUT_THRESHOLD_FIELD    9*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   adaptation rate acceleration factor
// @DOC_FIELD_FORMAT   Q5.19 format
.CONST $agc400.Parameter.OFFSET_ATTACK_SPEED_FIELD      10*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   hysterisis threshold1 for agc gain adaptation
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.Parameter.OFFSET_ADAPT_THRESHOLD1_FIELD  11*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   hysterisis threshold2 for agc gain adaptation
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.Parameter.OFFSET_ADAPT_THRESHOLD2_FIELD  12*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   min AGC gain (PARAMETER)
// @DOC_FIELD_FORMAT   Q7.17 format
.CONST $agc400.Parameter.OFFSET_G_MIN_FIELD             13*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   AGC VAD echo hold time in seconds(PARAMETER)
// @DOC_FIELD_FORMAT   Q7.17 format
.CONST $agc400.Parameter.OFFSET_ECHO_HOLD_TIME_FIELD    14*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   AGC VAD noise hold time in seconds(PARAMETER)
// @DOC_FIELD_FORMAT   Q7.17 format
.CONST $agc400.Parameter.OFFSET_NOISE_HOLD_TIME_FIELD   15*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   Structure Size
// @DOC_FIELD_FORMAT   Integer
.CONST $agc400.Parameter.STRUC_SIZE                     16;

// @END  DATA_OBJECT PARAMETERS

// @DATA_OBJECT  AGCDATAOBJECT

// @DOC_FIELD_TEXT   Pointer to Parameters                             @DOC_LINK         @DATA_OBJECT  PARAMETERS
// @DOC_FIELD_FORMAT   Pointer
.CONST $agc400.OFFSET_PARAM_PTR_FIELD				 0*ADDR_PER_WORD;               
// @DOC_FIELD_TEXT   Pointer to input stream (new frame)  
// @DOC_FIELD_FORMAT   Pointer
.CONST $agc400.OFFSET_PTR_INPUT_FIELD              1*ADDR_PER_WORD;	
// @DOC_FIELD_TEXT   Pointer to output stream (new frame)  
// @DOC_FIELD_FORMAT   Pointer
.CONST $agc400.OFFSET_PTR_OUTPUT_FIELD             2*ADDR_PER_WORD;		
// @DOC_FIELD_TEXT  Pointer to the value of Voice Activity Detector (VAD)  . Value = 1 for 'voice activity detected'  Value = 0 for 'no voice activity detected'
// @DOC_FIELD_FORMAT   Pointer
.CONST $agc400.OFFSET_PTR_VAD_VALUE_FIELD          3*ADDR_PER_WORD;     
// @DOC_FIELD_TEXT Pointer to cVc Variant Variable
// @DOC_FIELD_FORMAT flag pointer
.CONST $agc400.PTR_VARIANT_FIELD                   4*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   pointer to tone flag to freeze adaptation
// @DOC_FIELD_FORMAT   Pointer
.CONST $agc400.OFFSET_PTR_TONE_FLAG_FIELD          5*ADDR_PER_WORD;
.CONST $agc400.FRAME_TH_FIELD                      6*ADDR_PER_WORD;	
// @DOC_FIELD_TEXT   running average of abs(input stream)
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.OFFSET_INPUT_LEVEL_FIELD            7*ADDR_PER_WORD;   
// @DOC_FIELD_TEXT   minimum for the running average of abs(input stream)
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.OFFSET_INPUT_LEVEL_MIN_FIELD        8*ADDR_PER_WORD;      
// @DOC_FIELD_TEXT   1 - TC for smoothing AGC gain when increasing
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.ATTACK_TC_FIELD                   9*ADDR_PER_WORD;      
// @DOC_FIELD_TEXT   1 - TC for smoothing AGC gain when decreasing
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.DECAY_TC_FIELD                    10*ADDR_PER_WORD;    
// @DOC_FIELD_TEXT   1 - TC for input level adjustment when increasing
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.ALPHA_A_90_FIELD                  11*ADDR_PER_WORD;   
// @DOC_FIELD_TEXT   TC for input level adjustment when decreasing
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.ALPHA_D_90_FIELD                  12*ADDR_PER_WORD;   
// @DOC_FIELD_TEXT   min AGC gain (PARAMETER) 
// @DOC_FIELD_FORMAT   Q7.17 format
.CONST $agc400.OFFSET_G_MIN_FIELD                13*ADDR_PER_WORD;   
// @DOC_FIELD_TEXT   estimated AGC gain value
// @DOC_FIELD_FORMAT   Q7.17 format
.CONST $agc400.OFFSET_G_FIELD                    14*ADDR_PER_WORD;   
// @DOC_FIELD_TEXT   applied AGC gain value
// @DOC_FIELD_FORMAT   Q7.17 format
.CONST $agc400.OFFSET_G_REAL_FIELD               15*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   1 - in/out compression ratio applied when exceeding AGC TARGET
// @DOC_FIELD_FORMAT   Q1.23 format
.CONST $agc400.OFFSET_ONE_M_COMP_FIELD           16*ADDR_PER_WORD;
// @DOC_FIELD_TEXT   power estimation scale factor -0.5*log2(2/frame_size) 
// @DOC_FIELD_FORMAT   Q8.16 format
.CONST $agc400.OFFSET_PWR_SCALE_FIELD            17*ADDR_PER_WORD;	
// @DOC_FIELD_TEXT   Frame counter before AGC gain update
// @DOC_FIELD_FORMAT   Integer 
.CONST $agc400.OFFSET_FRAME_COUNTER_FIELD        18*ADDR_PER_WORD;
// @DOC_FIELD_TEXT     last persisted gain 
// @DOC_FIELD_FORMAT   Q7.17 format
.CONST $agc400.OFFSET_PERSISTED_GAIN_FIELD       19*ADDR_PER_WORD;
// @DOC_FIELD_TEXT  Control word for AGC - Bypass
// @DOC_FIELD_FORMAT  Bit Flag 
.CONST $agc400.FLAG_BYPASS_AGC                   20*ADDR_PER_WORD;
// @DOC_FIELD_TEXT     Flag for enable agc persist feature
// @DOC_FIELD_FORMAT  Bit Flag 
.CONST $agc400.FLAG_ENABLE_PERSIST               21*ADDR_PER_WORD;

//
// Internal fields
//
.CONST $agc400.INPUT_AVE_FIELD                   22*ADDR_PER_WORD;
.CONST $agc400.INPUT_MAX_FIELD                   23*ADDR_PER_WORD;
.CONST $agc400.VAD_PP_FIELD                      24*ADDR_PER_WORD;
.CONST $agc400.ATTACK_MODE_FIELD                 25*ADDR_PER_WORD;
.CONST $agc400.FRAME_SIZE_FIELD                  26*ADDR_PER_WORD;
.CONST $agc400.VAD_HOLD_ENABLE_FIELD             27*ADDR_PER_WORD;

// @DOC_FIELD_TEXT   Structure Size   
// @DOC_FIELD_FORMAT   Integer                
.CONST $agc400.STRUCT_SIZE                         28;

// @END  DATA_OBJECT AGCDATAOBJECT

// -----------------------------------------------------------------------------
// AGC VAD hold structure
// -----------------------------------------------------------------------------
.CONST $agc400.vad.VAD_AGC_FIELD                   0;
.CONST $agc400.vad.ECHO_HOLD_FIELD                 1 * MK1;
.CONST $agc400.vad.ECHO_THRES_FIELD                2 * MK1;
.CONST $agc400.vad.NOISE_HOLD_FIELD                3 * MK1;
.CONST $agc400.vad.NOISE_THRES_FIELD               4 * MK1;
.CONST $agc400.vad.STRUC_SIZE                      5;

#endif   // AGC400_LIB_H
