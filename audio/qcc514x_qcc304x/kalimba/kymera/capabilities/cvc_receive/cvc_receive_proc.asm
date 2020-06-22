// *****************************************************************************
// Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// $Change$  $DateTime$
// *****************************************************************************

#include "portability_macros.h"
#include "stack.h"

#include "cvc_receive_cap_asm.h"

#include "shared_memory_ids_asm_defs.h"
#include "frame_iir_resamplerv2_asm_defs.h"
#include "iir_resamplerv2_common_asm_defs.h"

#include "patch_library.h"

#define uses_RCV_FREQPROC  (uses_RCV_NS || uses_AEQ)
#define uses_RCV_VAD       (uses_RCV_AGC || uses_AEQ)


#if !defined(CAPABILITY_DOWNLOAD_BUILD)
#define $M.CVC_RCV_CAP.data.dyn_table_main   $M.CVC_RCV_CAP.data.DynTable_Main
#else
#define $M.CVC_RCV_CAP.data.dyn_table_main   $M.CVC_RCV_CAP.data.Downloadable.DynTable_Main
#endif

// *****************************************************************************
// MODULE:
//    unsigned CVC_RCV_CAP_Create(CVC_RECEIVE_OP_DATA *op_extra_data);
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r0 - Extended Data
//
// OUTPUTS:
//    - r0 - status
//
// TRASHED REGISTERS:
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.CVC_RCV_CAP.Create;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

$_CVC_RCV_CAP_Create:
   push rLink;
   pushm <r8,r9>;

   r8 = r0; // Extended Data

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($CVC_RCV_CAP_CREATE.PATCH_ID_0, r1)

   call $cvc_recv.register_component;

   // rcv_variant -> r9
   r9 = M[r8 + $CVC_RCV_CAP.ROOT.DATA_VARIANT];
   Null = r9 - RCV_VARIANT_BEX;
   if Z r9 = RCV_VARIANT_NB;

   //  Allocate Scratch Data
   r0 = r8;
   r1 = $M.CVC_RCV_CAP.data.dyn_table_main;
   r2 = M[r8 + $CVC_RCV_CAP.ROOT.DATA_VARIANT];
   call $_DynLoaderScratchReserve;
   NULL = r0;
   if Z CALL_PANIC_OR_JUMP(create_failed);

   //
   //  Allocate Shared Variables
   //

   // CVCLIB table
   r0 = r8 + $CVC_RCV_CAP.ROOT.CVCLIB_TABLE_FIELD;
   r1 = r9;
   call $_cvclib_table_alloc;
   r0 = M[r8 + $CVC_RCV_CAP.ROOT.CVCLIB_TABLE_FIELD];
   if Z CALL_PANIC_OR_JUMP(create_failed);


   // Allocate VAD & DC Blocker Resources
#if uses_RCV_VAD ||  uses_DCBLOCK
   r0 = r8 + $CVC_RCV_CAP.ROOT.VAD_DCB_COEFFS_PTR_FIELD;
   r1 = r9;
   call $_vad410_AllocMem;
   r0 = M[r8 + $CVC_RCV_CAP.ROOT.VAD_DCB_COEFFS_PTR_FIELD];
   if Z  CALL_PANIC_OR_JUMP(create_failed);
#endif

   // Allocate OMS Resources
#if uses_RCV_NS
   r0 = r8 + $CVC_RCV_CAP.ROOT.OMS_CONFIG_PTR_FIELD;
   r1 = r9;
   call $_oms280_AllocMem;
   r0 = M[r8 + $CVC_RCV_CAP.ROOT.OMS_CONFIG_PTR_FIELD];
   if Z CALL_PANIC_OR_JUMP(create_failed);
#endif

  // Allocate FB resources
#if uses_RCV_FREQPROC
   r0 = r8 + $CVC_RCV_CAP.ROOT.ANAL_FB_CONFIG_PTR_FIELD;
   r1 = r9;
   r2 = 0;
   call $_dyn_load_filter_bank_config;
   r0 = M[r8 + $CVC_RCV_CAP.ROOT.ANAL_FB_CONFIG_PTR_FIELD];
   if Z CALL_PANIC_OR_JUMP(create_failed);

   r0 = r8 + $CVC_RCV_CAP.ROOT.SYNTH_FB_CONFIG_PTR_FIELD;
   r1 = M[r8 + $CVC_RCV_CAP.ROOT.DATA_VARIANT];
   Null = r1 - RCV_VARIANT_BEX;
   if Z r1 = RCV_VARIANT_WB;
   r2 = 0;
   call $_dyn_load_filter_bank_config;
   r0 = M[r8 + $CVC_RCV_CAP.ROOT.SYNTH_FB_CONFIG_PTR_FIELD];
   if Z CALL_PANIC_OR_JUMP(create_failed);

   r0 = r8 + $CVC_RCV_CAP.ROOT.FB_SPLIT_PTR_FIELD;
   call $_dyn_load_filter_bank_split;
   r0 = M[r8 + $CVC_RCV_CAP.ROOT.FB_SPLIT_PTR_FIELD];
   if Z CALL_PANIC_OR_JUMP(create_failed);
#endif

   // Set the current mode pointer field

   r10 = r8 + $CVC_RCV_CAP.ROOT.CUR_MODE;
   M[r8 + $CVC_RCV_CAP.ROOT.CUR_MODE_PTR_FIELD] = r10;

   //  Allocate Persistent Data
   r0 = r8;
   r1 = $M.CVC_RCV_CAP.data.dyn_table_main;
   r2 = $M.CVC_RCV_CAP.data.DynTable_Linker;
   r3 = M[r8 + $CVC_RCV_CAP.ROOT.DATA_VARIANT];
   call $_DynLoaderProcessDynamicAllocations;
   NULL = r0;
   if Z CALL_PANIC_OR_JUMP(create_failed);

   // Setup upsampler
   r2 = M[r8 + $CVC_RCV_CAP.ROOT.DATA_VARIANT];
   Null = r2 - RCV_VARIANT_BEX;
   if NZ jump skip_rcv_fe_resampler_setup;
        r0 = $shared_memory_ids.IIRV2_RESAMPLER_Up_2_Down_1;
        call $_iir_resamplerv2_allocate_config_by_id;
        r1 = r0;
        if Z CALL_PANIC_OR_JUMP(create_failed);
        r0 = M[r8 + $CVC_RCV_CAP.ROOT.UPSAMPLE_PTR_FIELD];
        r0 = r0 + $frame_iir_resamplerv2.iir_resampler_op_struct.COMMON_FIELD;
        call $_iir_resamplerv2_set_config;

skip_rcv_fe_resampler_setup:
  

create_succeeded:
   r0 = 0;                                                         // create succeeded
   popm <r8,r9>;
   pop rLink;
   rts;
create_failed:
   r0 = 1;                                                         // create failed
   popm <r8,r9>;
   pop rLink;
   rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    void CVC_RCV_CAP_Destroy(CVC_RECEIVE_OP_DATA *op_extra_data);
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r0 - Extended Data
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.CVC_RCV_CAP.Destroy;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

$_CVC_RCV_CAP_Destroy:
   push rLink;
   push r8;

   r8 = r0;  // extended data

   // unregister component
   call $Security.UnregisterComponent;

   //  deallocate Scratch Data
   r0 = M[r8 + $CVC_RCV_CAP.ROOT.SCRATCH_ALLOC_PTR_FIELD];
   if NZ call $_DynLoaderDeregisterScratch;
   M[r8 + $CVC_RCV_CAP.ROOT.SCRATCH_ALLOC_PTR_FIELD]=NULL;


   r0 = M[r8 + $CVC_RCV_CAP.ROOT.CVCLIB_TABLE_FIELD];
   if NZ call $_cvclib_table_release;
   M[r8 + $CVC_RCV_CAP.ROOT.CVCLIB_TABLE_FIELD] = NULL;

   r0 = M[r8 + $CVC_RCV_CAP.ROOT.UPSAMPLE_PTR_FIELD];
   r0 = M[r0 + ($frame_iir_resamplerv2.iir_resampler_op_struct.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.FILTER_FIELD)];
   if NZ call $_iir_resamplerv2_release_config;
   
   r0 = M[r8 + $CVC_RCV_CAP.ROOT.VAD_DCB_COEFFS_PTR_FIELD];
   if NZ call $_vad410_ReleaseMem;
   M[r8 + $CVC_RCV_CAP.ROOT.VAD_DCB_COEFFS_PTR_FIELD]=NULL;

   r0 = M[r8 + $CVC_RCV_CAP.ROOT.OMS_CONFIG_PTR_FIELD];
   if NZ call $_oms280_ReleaseMem;
   M[r8 + $CVC_RCV_CAP.ROOT.OMS_CONFIG_PTR_FIELD]=NULL;

   r0 = M[r8 + $CVC_RCV_CAP.ROOT.ANAL_FB_CONFIG_PTR_FIELD];
   if NZ call $_dyn_free_filter_bank_config;
   M[r8 + $CVC_RCV_CAP.ROOT.ANAL_FB_CONFIG_PTR_FIELD]=NULL;

   r0 = M[r8 + $CVC_RCV_CAP.ROOT.SYNTH_FB_CONFIG_PTR_FIELD];
   if NZ call $_dyn_free_filter_bank_config;
   M[r8 + $CVC_RCV_CAP.ROOT.SYNTH_FB_CONFIG_PTR_FIELD]=NULL;

   r0 = M[r8 + $CVC_RCV_CAP.ROOT.FB_SPLIT_PTR_FIELD];
   if NZ call $_dyn_free_filter_bank_split;
   M[r8 + $CVC_RCV_CAP.ROOT.FB_SPLIT_PTR_FIELD]=NULL;

   //  deallocate Persistent Data
   r0 = M[r8 + $CVC_RCV_CAP.ROOT.INST_ALLOC_PTR_FIELD];
   if NZ call $_DynLoaderReleaseDynamicAllocations;
   M[r8 + $CVC_RCV_CAP.ROOT.INST_ALLOC_PTR_FIELD]=NULL;

   pop r8;
   pop rLink;
   rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    void CVC_RCV_CAP_Process(CVC_RECEIVE_OP_DATA *op_extra_data);
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r0 - Extended Data
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.CVC_RCV_CAP;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

$_CVC_RCV_CAP_Process:
   push rLink;
   call $_cvc_register_save;

   push r0;    // Save Root Object. As local variable

   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($CVC_RCV_CAP_PROCESS.PATCH_ID_0, r1)

   r1 = r0;
   r0 = M[r0 + $CVC_RCV_CAP.ROOT.INST_ALLOC_PTR_FIELD];
   r1 = M[r1 + $CVC_RCV_CAP.ROOT.SCRATCH_ALLOC_PTR_FIELD];
   if NZ call $_DynLoaderScratchCommit;

   r0 = M[SP - 1*ADDR_PER_WORD];  // extended data
   NULL = M[r0 + $CVC_RCV_CAP.ROOT.ALGREINIT];
   if NZ call ReInitialize;    // preserves FP

   // Input streaming: input_stream -> stream_rcvin
   r0 = M[SP - 1*ADDR_PER_WORD];  // extended data
   r3 = M[r0 + $CVC_RCV_CAP.ROOT.MODULES_PTR_FIELD];
   r7 = M[r0 + $CVC_RCV_CAP.ROOT.INPUT_STREAM_MAP_PTR_FIELD];
   r8 = M[r3 + $CVC_RCV_CAP.MODULE.RCVIN_PTR_FIELD];
   call $cvc.stream_transfer.peak.cbuffer_in;

   // Set Current Mode (r1)
   r0 = M[SP - 1*ADDR_PER_WORD];  // extended data
   r1 = M[r0 + $CVC_RCV_CAP.ROOT.HOST_MODE];
   r3 = M[r0 + $CVC_RCV_CAP.ROOT.OBPM_MODE];
   r2 = M[r0 + $CVC_RCV_CAP.ROOT.OVR_CONTROL];
   Null = r2 AND $M.CVC_RECV.CONTROL.MODE_OVERRIDE;
   if NZ r1 = r3;
   
   // Process Data and save current mode
   call $cvc_recv.process_data;

   // Output streaming: stream_rcvout -> output_stream
   r0 = M[SP - 1*ADDR_PER_WORD];  // extended data
   r3 = M[r0 + $CVC_RCV_CAP.ROOT.MODULES_PTR_FIELD];
   r7 = M[r3 + $CVC_RCV_CAP.MODULE.RCVOUT_PTR_FIELD];
   r8 = M[r0 + $CVC_RCV_CAP.ROOT.OUTPUT_STREAM_MAP_PTR_FIELD];
   call $cvc.stream_transfer.peak.cbuffer_out;

   // AA - Updated AGC gain value to be persisted, here
   r0 = M[SP - 1*ADDR_PER_WORD];  // extended data
   #if uses_RCV_AGC
   r3 = M[r0 + $CVC_RCV_CAP.ROOT.MODULES_PTR_FIELD];
   r1 = M[r3 + $CVC_RCV_CAP.MODULE.AGC_PTR_FIELD];
   r2 = M[r1 + $agc400.OFFSET_G_REAL_FIELD];
   M[r0 + $CVC_RCV_CAP.ROOT.AGC_STATE] = r2;
   #endif

   pop r0;     // remove local variables

   // Release committed scratch
   NULL = M[r0 + $CVC_RCV_CAP.ROOT.SCRATCH_ALLOC_PTR_FIELD];
   if NZ call $_scratch_free;

   call $_cvc_register_restore;
   pop rLink;
   rts;


ReInitialize:

   push rLink;
   push r9;
   push r0;    // root object

   M[r0 + $CVC_RCV_CAP.ROOT.ALGREINIT]  = Null;
   // AA - Zero frame counter here

   r9 = M[r0 + $CVC_RCV_CAP.ROOT.MODULES_PTR_FIELD];
   r3 = M[r0 + $CVC_RCV_CAP.ROOT.PARAMS_PTR_FIELD];
   r0=  M[r3 + $M.CVC_RECV.PARAMETERS.OFFSET_RCV_CONFIG];

#if uses_AEQ
   r2 = M[r9 + $CVC_RCV_CAP.MODULE.AEQ_PTR_FIELD];
   M[r2 + $M.AdapEq.CONTROL_WORD_FIELD]=r0;
#endif
#if uses_RCV_AGC
   // AGC module bypass control
   r2 = M[r9 + $CVC_RCV_CAP.MODULE.AGC_PTR_FIELD];
   r1 = r0 AND $M.CVC_RECV.CONFIG.RCVAGCBYP;
   M[r2 + $agc400.FLAG_BYPASS_AGC]=r1;

   // AGC persistent control
   r1 = r0 AND $M.CVC_RECV.CONFIG.BYPASS_AGCPERSIST;
   if NZ r1 = 1;
   r1 = 1 - r1;
   M[r2 + $agc400.FLAG_ENABLE_PERSIST] = r1;

   r1 = M[SP - 1*ADDR_PER_WORD];
   r1 = M[r1 + $CVC_RCV_CAP.ROOT.AGC_STATE];
   M[r2 + $agc400.OFFSET_PERSISTED_GAIN_FIELD] = r1;
#endif


   // -----------------------------------------------------------------------------
   // OMS rcv module bypass/hold  and High Resolution mode
   // -----------------------------------------------------------------------------
#if uses_RCV_NS
   r2 = M[r9 + $CVC_RCV_CAP.MODULE.HARM_PTR_FIELD];
   r1=M[r3 + $M.CVC_RECV.PARAMETERS.OFFSET_OMS_HI_RES_MODE];
   r1 = 1 - r1;
   M[r2 + $harm100.FLAG_BYPASS_FIELD]=r1;
   r2 = M[r9 + $CVC_RCV_CAP.MODULE.OMS_PTR_FIELD];
   r1 = r0 AND $M.CVC_RECV.CONFIG.RCVOMSBYP;
   M[r2 + $M.oms280.BYPASS_FIELD] = r1;

   //oms280rcv harm value pointer import
   r0 = M[r9 + $CVC_RCV_CAP.MODULE.HARM_PTR_FIELD];
   r1 = r0 + $harm100.HARM_VALUE_FIELD;
   Null = M[r0 + $harm100.FLAG_BYPASS_FIELD];
   if NZ r1 = 0;
   M[r2 + $M.oms280.PTR_HARM_VALUE_FIELD] = r1;
#endif

   r0 = M[SP - 1*ADDR_PER_WORD];  // root object   
   r0 = M[r0 + $CVC_RCV_CAP.ROOT.INIT_TABLE_PTR_FIELD];
   call $_cvc_run_frame_proc_function_table;

   pop r0;     // remove local variables
   pop r9;
   jump $pop_rLink_and_rts;



// -r7 = resampler
// -r8 = variant
$cvc.fe.frame_resample_process:
   Null = r8 - RCV_VARIANT_BEX;
   if NZ rts;
   jump $frame.iir_resamplev2.Process;


// -r8 = variant
$cvc.rcv_peq.process_nb_bex:
   Null = r8 - RCV_VARIANT_NB;
   if Z jump $cvc.peq.process;
   Null = r8 - RCV_VARIANT_BEX;
   if Z jump $cvc.peq.process;
   rts;

// -r8 = variant
$cvc.rcv_peq.process_wb:
   Null = r8 - RCV_VARIANT_NB;
   if Z rts;
   Null = r8 - RCV_VARIANT_BEX;
   if Z rts;
   jump $cvc.peq.process;
   
.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $M.CVC_RECV.passthrough.mute_and_gain_control
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r8 - mute_gain_ctrl_obj_dm2
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
.MODULE $M.CVC_RECV.passthrough.mute_and_gain_control;
   .CODESEGMENT PM;

$cvc_recv.passthrough.mute_and_gain_control:
   // if it is not in passthrough mode, then it must be in static or standby mode, so mute the stream
   r4 = M[r8];
   r4 = M[r4];
   NULL = r4 - $M.CVC_RECV.SYSMODE.PASS_THRU;
   if Z jump get_and_apply_gain;

   // get the output stream pointer;
   r8 = M[r8 + 3 * ADDR_PER_WORD];

   // mute the output
   jump $cvc.stream_mute;

get_and_apply_gain:
   // get the parameter pointer
   r1 = M[r8 + 1 * ADDR_PER_WORD];

   // get gain
   r5 = M[r1 + $M.CVC_RECV.PARAMETERS.OFFSET_PT_RCVGAIN_MANTISSA];
   r6 = M[r1 + $M.CVC_RECV.PARAMETERS.OFFSET_PT_RCVGAIN_EXPONENT];

   // set the source stream pointer
   r7 = M[r8 + 2 * ADDR_PER_WORD];

   // set the destination stream pointer
   r8 = M[r8 + 3 * ADDR_PER_WORD];

   // passthrough processing with gain in r5, r6, 
   // source in r7, and destination in r8
   jump $cvc.stream_gain;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $cvc_recv.stream_gain
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r7 - parameter pointer
//    - r8 - stream_rcvout (destination)
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
.MODULE $M.CVC_RECV.stream_gain;
   .CODESEGMENT PM;

$cvc_recv.stream_gain:
   // get gain
   r5 = M[r7 + $M.CVC_RECV.PARAMETERS.OFFSET_RCVGAIN_MANTISSA];
   r6 = M[r7 + $M.CVC_RECV.PARAMETERS.OFFSET_RCVGAIN_EXPONENT];
   r6 = r6 + $filter_bank.QBIT_Z;

   // set destination for in-place processing
   r7 = r8;

   // process with gain in r5, r6, 
   // source in r7, and destination in r8
   jump $cvc.stream_gain;

.ENDMODULE;
