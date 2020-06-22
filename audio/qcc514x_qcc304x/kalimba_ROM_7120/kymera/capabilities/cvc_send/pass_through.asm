// *****************************************************************************
// Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// $Change$  $DateTime$
// *****************************************************************************

#include "cvc_send_data.h"

// -----------------------------------------------------------------------------
// inputs:
//    -r7 = &cvc_streams
//    -r8 = &stream_z0
//    -r9 = $root 
// 
// outputs:
//    -None
// -----------------------------------------------------------------------------
.MODULE $M.cvc_send.passthrough.voice;
   .CODESEGMENT PM;

$cvc_send.passthrough.voice:  
   // check requested feature
   // if voice feature is not requested, return   
   r2 = M[r9 + $cvc_send.data.cap_root_ptr];
   r1 = M[r2 + $cvc_send.cap.OP_FEATURE_REQUESTED];
   Null = r1 AND $cvc_send.REQUESTED_FEATURE_VOICE;
   if Z rts;

   r6 = $M.GEN.CVC_SEND.PARAMETERS.OFFSET_PT_SNDGAIN_MANTISSA;

$cvc_send.passthrough:   
   // check system mode
   r0 = M[r2 + $cvc_send.cap.CUR_MODE];   
   NULL = r0 - $M.GEN.CVC_SEND.SYSMODE.STANDBY;
   if LE jump $cvc.stream_mute;
  
   // get voice input source offset  
   r1 = $cvc_send.stream.sndin_left;
   Null = r0 - $M.GEN.CVC_SEND.SYSMODE.PASS_THRU_LEFT;
   if Z jump get_source;   

   r1 = $cvc_send.stream.sndin_right;
   Null = r0 - $M.GEN.CVC_SEND.SYSMODE.PASS_THRU_RIGHT;
   if Z jump get_source;
  
   r1 = $cvc_send.stream.sndin_mic3;
   Null = r0 - $M.GEN.CVC_SEND.SYSMODE.PASS_THRU_MIC3;
   if Z jump get_source;

   r1 = $cvc_send.stream.sndin_mic4;
   Null = r0 - $M.GEN.CVC_SEND.SYSMODE.PASS_THRU_MIC4;
   if Z jump get_source;
   
   r1 = $cvc_send.stream.refin;
   Null = r0 - $M.GEN.CVC_SEND.SYSMODE.PASS_THRU_AEC_REF;
   if Z jump get_source;

   // Default connect to mic1
   r1 = $cvc_send.stream.sndin_left;

get_source:
   // get input source pointer
   r7 = M[r7 + r1];

   // get and apply gain with input source in r7, destination in r8, cvc root object in r9
   jump $cvc_send.stream_gain.passthrough;

.ENDMODULE;

// -----------------------------------------------------------------------------
// inputs:
//    -r7 = &cvc_streams
//    -r8 = &stream_z1
//    -r9 = $root 
// 
// outputs:
//    -None
// -----------------------------------------------------------------------------
.MODULE $M.cvc_send.passthrough.va;
   .CODESEGMENT PM;
   
$cvc_send.passthrough.va:   
   // check requested feature
   // if VA feature is not requested, return  
   r2 = M[r9 + $cvc_send.data.cap_root_ptr];
   r1 = M[r2 + $cvc_send.cap.OP_FEATURE_REQUESTED];
   Null = r1 AND $cvc_send.REQUESTED_FEATURE_VA;
   if Z rts;

   r6 = $M.GEN.CVC_SEND.PARAMETERS.OFFSET_VA_GAIN_MANTISSA;
   jump $cvc_send.passthrough;

.ENDMODULE;

//------------------------------------------------------------------------------
// inputs:
//    -r7 = ADC input stream pointer
//    -r8 = VA output stream pointer
//    -r9 = $root
// 
// outputs:
//    None 
// -----------------------------------------------------------------------------  
.MODULE $M.cvc_send.passthrough.speaker_va;
   .CODESEGMENT PM;   
   
$cvc_send.passthrough.speaker_va:
   // check requested feature
   // if voice feature is not requested, return
   r2 = M[r9 + $cvc_send.data.cap_root_ptr];
   r1 = M[r2 + $cvc_send.cap.OP_FEATURE_REQUESTED];
   Null = r1 AND $cvc_send.REQUESTED_FEATURE_VA;
   if Z rts;
 
   // check system mode 
   r0 = M[r2 + $cvc_send.cap.CUR_MODE];   
   NULL = r0 - $M.GEN.CVC_SEND.SYSMODE.STANDBY;
   if LE jump $cvc.stream_mute;

   // get and apply gain with input source in r7, destination in r8, cvc root object in r9
   r6 = $M.GEN.CVC_SEND.PARAMETERS.OFFSET_VA_GAIN_MANTISSA;
   jump $cvc_send.stream_gain.passthrough;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $cvc_send.stream_gain
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r6 - point to gain (mantisa/exp)
//    - r7 - source stream pointer
//    - r8 - destination stream pointer
//    - r9 - cvc root object
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// CPU USAGE:
//
// NOTE:
// *****************************************************************************
.MODULE $M.CVC_SEND.stream_gain;

   .CODESEGMENT PM;

$cvc_send.stream_gain.passthrough:
   // r6 from caller
   // no extra scaling
   r0 = 0;
   jump $cvc_send.stream_gain;

$cvc_send.stream_gain.process.va:
   r6 = $M.GEN.CVC_SEND.PARAMETERS.OFFSET_VA_GAIN_MANTISSA;
   jump $cvc_send.stream_gain.process;

$cvc_send.stream_gain.process.voice:
   r6 = $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SNDGAIN_MANTISSA;

$cvc_send.stream_gain.process:
   // extra scaling
   r0 = $filter_bank.QBIT_Z;

$cvc_send.stream_gain:
   // get gain
   r1 = M[r9 + $cvc_send.data.param];
   r6 = r6 + r1;
   // gain mantisa
   r5 = M[r6];
   // gain exp
   r6 = M[r6 + MK1];
   r6 = r6 + r0;

   // processing with gain in r5 (mantisa), r6 (exp), 
   // source in r7, and destination in r8
   jump $cvc.stream_gain;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $M.CVC_SEND.mute_control
//
// DESCRIPTION:
//    Mute control
//
// MODIFICATIONS:
//
// INPUTS:
//    r7 - mute control pointer
//    I8 - source & target stream for in-place processing
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.CVC_SEND.mute_control;
   .CODESEGMENT PM;

$cvc_send.mute_control:
   r7 = M[r7];
   Null = 1 - r7;
   if NZ rts;
   jump $cvc.stream_mute;

.ENDMODULE;
