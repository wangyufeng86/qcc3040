/**
* Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
 * \file cvc_send_cap.h
 */

#ifndef CVC_SEND_CAP_ASM_H
#define CVC_SEND_CAP_ASM_H

/*Temporary macro for debug purposes. */
#ifndef DEBUG_MODE
  #define CALL_PANIC_OR_JUMP(x) jump x
#else
  #define CALL_PANIC_OR_JUMP(x) call $error 
#endif

/* Description for Operator Data Structure */

/* Set From Dynamic Loader */
.CONST $cvc_send.cap.INST_ALLOC_PTR_FIELD                0*ADDR_PER_WORD;
.CONST $cvc_send.cap.SCRATCH_ALLOC_PTR_FIELD             1*ADDR_PER_WORD;

.CONST $cvc_send.cap.REF_INPUT_STREAM_MAP_PTR_FIELD      2*ADDR_PER_WORD;
.CONST $cvc_send.cap.LEFT_INPUT_STREAM_MAP_PTR_FIELD     3*ADDR_PER_WORD;
.CONST $cvc_send.cap.RIGHT_INPUT_STREAM_MAP_PTR_FIELD    4*ADDR_PER_WORD;
.CONST $cvc_send.cap.MIC3_INPUT_STREAM_MAP_PTR_FIELD     5*ADDR_PER_WORD;
.CONST $cvc_send.cap.MIC4_INPUT_STREAM_MAP_PTR_FIELD     6*ADDR_PER_WORD;
.CONST $cvc_send.cap.OUTPUT_STREAM_MAP_PTR_FIELD         7*ADDR_PER_WORD;
.CONST $cvc_send.cap.VA_BEAM1_STREAM_MAP_PTR_FIELD       8*ADDR_PER_WORD;
.CONST $cvc_send.cap.VA_BEAM2_STREAM_MAP_PTR_FIELD       9*ADDR_PER_WORD;
.CONST $cvc_send.cap.VA_BEAM3_STREAM_MAP_PTR_FIELD       10*ADDR_PER_WORD;
.CONST $cvc_send.cap.VA_BEAM4_STREAM_MAP_PTR_FIELD       11*ADDR_PER_WORD;

.CONST $cvc_send.cap.PTR_INPUT_STREAM_REF_FIELD          12*ADDR_PER_WORD;
.CONST $cvc_send.cap.PTR_INPUT_STREAM_MIC1_FIELD         13*ADDR_PER_WORD;
.CONST $cvc_send.cap.PTR_INPUT_STREAM_MIC2_FIELD         14*ADDR_PER_WORD;
.CONST $cvc_send.cap.PTR_INPUT_STREAM_MIC3_FIELD         15*ADDR_PER_WORD;
.CONST $cvc_send.cap.PTR_INPUT_STREAM_MIC4_FIELD         16*ADDR_PER_WORD;
.CONST $cvc_send.cap.PTR_OUTPUT_STREAM_SND_FIELD         17*ADDR_PER_WORD;
.CONST $cvc_send.cap.PTR_OUTPUT_STREAM_VA1_FIELD         18*ADDR_PER_WORD;
.CONST $cvc_send.cap.PTR_OUTPUT_STREAM_VA2_FIELD         19*ADDR_PER_WORD;
.CONST $cvc_send.cap.PTR_OUTPUT_STREAM_VA3_FIELD         20*ADDR_PER_WORD;
.CONST $cvc_send.cap.PTR_OUTPUT_STREAM_VA4_FIELD         21*ADDR_PER_WORD;

.CONST $cvc_send.cap.MODE_TABLE_PTR_FIELD                22*ADDR_PER_WORD;
.CONST $cvc_send.cap.INIT_TABLE_PTR_FIELD                23*ADDR_PER_WORD;
.CONST $cvc_send.cap.CVC_DATA_ROOT_FIELD                 24*ADDR_PER_WORD;
.CONST $cvc_send.cap.STATUS_TABLE_PTR_FIELD              25*ADDR_PER_WORD;
.CONST $cvc_send.cap.PARAMS_PTR_FIELD                    26*ADDR_PER_WORD;

/* Shared Variables (Set Before dynamic load) */              
.CONST $cvc_send.cap.CVCLIB_TABLE                        27*ADDR_PER_WORD;
.CONST $cvc_send.cap.FFTSPLIT_TABLE                      28*ADDR_PER_WORD;
.CONST $cvc_send.cap.OMS_CONST                           29*ADDR_PER_WORD;
.CONST $cvc_send.cap.DMS200_MODE                         30*ADDR_PER_WORD;  
.CONST $cvc_send.cap.ASF_MODE_TABLE                      31*ADDR_PER_WORD;
.CONST $cvc_send.cap.WNR_MODE_TABLE                      32*ADDR_PER_WORD;
.CONST $cvc_send.cap.VAD_DC_COEFFS                       33*ADDR_PER_WORD;
.CONST $cvc_send.cap.AEC_MODE                            34*ADDR_PER_WORD;
.CONST $cvc_send.cap.FILTERBANK_CONFIG                   35*ADDR_PER_WORD;
.CONST $cvc_send.cap.NDVC_SHARE_PTR_FIELD                36*ADDR_PER_WORD;

// State Info                                                 
.CONST $cvc_send.cap.MGDC_STATE_FIELD                    37*ADDR_PER_WORD; 
.CONST $cvc_send.cap.MGDC_STATE_PTR_FIELD                38*ADDR_PER_WORD;

// Misc Control Data                                          
.CONST $cvc_send.cap.OP_FEATURE_REQUESTED                39*ADDR_PER_WORD;
.CONST $cvc_send.cap.ASF_WNR_AVAILABLE                   40*ADDR_PER_WORD;
.CONST $cvc_send.cap.CAPABILITY_ID                       41*ADDR_PER_WORD;
.CONST $cvc_send.cap.CAP_ROOT_PTR_FIELD                  42*ADDR_PER_WORD;
.CONST $cvc_send.cap.HOST_MODE                           43*ADDR_PER_WORD;
.CONST $cvc_send.cap.OBPM_MODE                           44*ADDR_PER_WORD;
.CONST $cvc_send.cap.OVR_CONTROL                         45*ADDR_PER_WORD;
.CONST $cvc_send.cap.ALGREINIT                           46*ADDR_PER_WORD; 

.CONST $cvc_send.cap.SCRATCH_BUFFER                      47*ADDR_PER_WORD;  

.CONST $cvc_send.cap.CUR_MODE                            48*ADDR_PER_WORD;
.CONST $cvc_send.cap.DATA_VARIANT                        49*ADDR_PER_WORD;
.CONST $cvc_send.cap.MAJOR_CONFIG                        50*ADDR_PER_WORD;
.CONST $cvc_send.cap.NUM_MICS                            51*ADDR_PER_WORD;
.CONST $cvc_send.cap.MIC_CONFIG                          52*ADDR_PER_WORD;
.CONST $cvc_send.cap.CAP_DATA                            53*ADDR_PER_WORD;
.CONST $cvc_send.cap.DYN_MAIN                            54*ADDR_PER_WORD;
.CONST $cvc_send.cap.DYN_LINKER                          55*ADDR_PER_WORD;

.CONST $cvc_send.cap.MUTE_CONTROL_PTR                    56*ADDR_PER_WORD;
.CONST $cvc_send.cap.HOST_MUTE                           57*ADDR_PER_WORD;
.CONST $cvc_send.cap.OBPM_MUTE                           58*ADDR_PER_WORD;
.CONST $cvc_send.cap.CURRENT_MUTE                        59*ADDR_PER_WORD;

.CONST $cvc_send.cap.FRAME_SIZE_FIELD                    60*ADDR_PER_WORD;
.CONST $cvc_send.cap.SAMPLE_RATE_FIELD                   61*ADDR_PER_WORD;
.CONST $cvc_send.cap.NUM_VA_OUTPUTS                      62*ADDR_PER_WORD;
.CONST $cvc_send.cap.OMNI_MODE_HOST_FIELD                63*ADDR_PER_WORD;
.CONST $cvc_send.cap.OMNI_MODE_OBPM_FIELD                64*ADDR_PER_WORD;
.CONST $cvc_send.cap.OMNI_MODE_PTR_FIELD                 65*ADDR_PER_WORD;
.CONST $cvc_send.cap.SECURITY.KEY_FIELD                  66*ADDR_PER_WORD;

.CONST $cvc_send.cap.PARMS_DEF_FIELD                     68*ADDR_PER_WORD;//Secure Data is 2 words

// ---  Use cases --------------------------------------
.CONST $cvc_send.HEADSET                     0;
.CONST $cvc_send.SPEAKER                     1;
.CONST $cvc_send.AUTO                        2;

.CONST $cvc_send.REQUESTED_FEATURE_VOICE     0x1;
.CONST $cvc_send.REQUESTED_FEATURE_AEC       0x2;
.CONST $cvc_send.REQUESTED_FEATURE_VA        0x4;

#define CVC_SEND_MAX_NUM_INPUT               5
#define CVC_SEND_MAX_NUM_OUTPUT              5

#define DATA_VARIANT_NB                      0
#define DATA_VARIANT_WB                      1
#define DATA_VARIANT_UWB                     2
#define DATA_VARIANT_SWB                     3
#define DATA_VARIANT_FB                      4

#define MIC_CONFIG_DEFAULT                   0
#define MIC_CONFIG_ENDFIRE                   1
#define MIC_CONFIG_BROADSIDE                 2
#define MIC_CONFIG_CIRCULAR                  4

/* Voice quality metric constants */

.CONST $MAX_ROLLOFF_IDX_DIVIDED_BY_2   9; //max_rolloff_idx: 19

.CONST $SNR_MAX                      3.5;
.CONST $SNR_MIN                     -0.5;
.CONST $SNR_SCALE_LOWER_LIMIT        0.0;
.CONST $SNR_SCALE_HIGHER_LIMIT      15.0;

.CONST $SNR_MAX_FIXED_POINT                            Qfmt_($SNR_MAX, 8); 
.CONST $SNR_MIN_FIXED_POINT                            Qfmt_($SNR_MIN, 8); 
.CONST $SNR_SCALE_LOWER_LIMIT_FIXED_POINTXED_POINT     Qfmt_($SNR_SCALE_LOWER_LIMIT, 8);
.CONST $SNR_SCALE_HIGHER_LIMIT_FIXED_POINT             Qfmt_($SNR_SCALE_HIGHER_LIMIT, 8);

.CONST $SNR_FACTOR_FIXED_POINT                         Qfmt_(($SNR_SCALE_HIGHER_LIMIT - $SNR_SCALE_LOWER_LIMIT + 1.0)/($SNR_MAX - $SNR_MIN), 8);

.CONST $VOICE_QUALITY_METRIC_ERROR_CODE                0xFF;

#endif //CVC_SEND_CAP_ASM_H
