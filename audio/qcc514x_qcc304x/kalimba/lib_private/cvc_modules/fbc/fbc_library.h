// *****************************************************************************
// Copyright (c) 2007 - 2020 Qualcomm Technologies International, Ltd.
//
// %%version
//
// *****************************************************************************

#ifndef FBC_LIB_ASM_H_INCLUDED
#define FBC_LIB_ASM_H_INCLUDED

// *****************************************************************************
// FBC (Time Domain) algorithm matlab version
//
// VERSION HISTORY:
//    1.0.0 - initial version as sub-module in aec510/aec520
//    1.1.0 - stand-alone module
// *****************************************************************************

// -----------------------------------------------------------------------------
// FBC maximum filter length 10ms
// -----------------------------------------------------------------------------
.CONST $fbc.nb.FILTER_SIZE                80;
.CONST $fbc.wb.FILTER_SIZE                160;
.CONST $fbc.uwb.FILTER_SIZE               240;
.CONST $fbc.swb.FILTER_SIZE               320;
.CONST $fbc.fb.FILTER_SIZE                480;

// -----------------------------------------------------------------------------
// FBC external constants
// -----------------------------------------------------------------------------
.CONST $fbc.PERD                          1;
.CONST $fbc.HFP_B_SZIE                    6;

// -----------------------------------------------------------------------------
// FBC data object structure {TBD: generate from "C" header}
// -----------------------------------------------------------------------------

.CONST $fbc.STREAM_D_FIELD                MK1 * 0;
.CONST $fbc.STREAM_X_FIELD                MK1 * 1;
.CONST $fbc.G_FIELD                       MK1 * 2;
.CONST $fbc.SCRATCH_FIELD                 MK1 * 3;
.CONST $fbc.SAMPLING_RATE_FIELD           MK1 * 4;
.CONST $fbc.PERD_FIELD                    MK1 * 5;
.CONST $fbc.TAIL_LENGTH_FIELD             MK1 * 6;
.CONST $fbc.GAMMA_FIELD                   MK1 * 7;
.CONST $fbc.VADX_FIELD                    MK1 * 8;
.CONST $fbc.L2P_PWR_DIFFERENCE_FIELD      MK1 * 9;
.CONST $fbc.INT_VAR                       MK1 * 10;

// Internal fields
.CONST $fbc.FRAME_SIZE_FIELD              $fbc.INT_VAR + MK1 * 0;
.CONST $fbc.FILTER_LENGTH_FIELD           $fbc.INT_VAR + MK1 * 1;
.CONST $fbc.NIBBLE_FIELD                  $fbc.INT_VAR + MK1 * 2;
.CONST $fbc.ALFA_ERLE_FIELD               $fbc.INT_VAR + MK1 * 3;
.CONST $fbc.HD_TS_FIELD                   $fbc.INT_VAR + MK1 * 4;
.CONST $fbc.SIL_TS_FIELD                  $fbc.INT_VAR + MK1 * 5;
.CONST $fbc.G_B_FIELD                     $fbc.INT_VAR + MK1 * 6;
.CONST $fbc.SCRPTR_OBUF_Y_FIELD           $fbc.INT_VAR + MK1 * 7;
.CONST $fbc.SCRPTR_OBUF_D_FIELD           $fbc.INT_VAR + MK1 * 8;
.CONST $fbc.SCRPTR_OBUF_D_1_FIELD         $fbc.INT_VAR + MK1 * 9;
.CONST $fbc.SCRPTR_OBUF_D_2_FIELD         $fbc.INT_VAR + MK1 * 10;
.CONST $fbc.G_A_AMP_FIELD                 $fbc.INT_VAR + MK1 * 11;
.CONST $fbc.G_B_AMP_FIELD                 $fbc.INT_VAR + MK1 * 12;
.CONST $fbc.X_BUF_PWR_GW_FIELD            $fbc.INT_VAR + MK1 * 13;
.CONST $fbc.X_BUF_PWR_MSW_FIELD           $fbc.INT_VAR + MK1 * 14;
.CONST $fbc.X_BUF_PWR_LSW_FIELD           $fbc.INT_VAR + MK1 * 15;
.CONST $fbc.MU_MANTISA_FIELD              $fbc.INT_VAR + MK1 * 16;
.CONST $fbc.MU_EXP_FIELD                  $fbc.INT_VAR + MK1 * 17;
.CONST $fbc.MU2_MANTISA_FIELD             $fbc.INT_VAR + MK1 * 18;
.CONST $fbc.MU2_EXP_FIELD                 $fbc.INT_VAR + MK1 * 19;
.CONST $fbc.DERLE_AMP_FIELD               $fbc.INT_VAR + MK1 * 20;
.CONST $fbc.DERLE_FIL_FIELD               $fbc.INT_VAR + MK1 * 21;
.CONST $fbc.ERLE_FOLD_FIELD               $fbc.INT_VAR + MK1 * 22;
.CONST $fbc.HD_FOLD_FIELD                 $fbc.INT_VAR + MK1 * 23;
.CONST $fbc.HD_FLAG_FIELD                 $fbc.INT_VAR + MK1 * 24;
.CONST $fbc.HD_CNTR_FIELD                 $fbc.INT_VAR + MK1 * 25;
.CONST $fbc.TH_CNTR_FIELD                 $fbc.INT_VAR + MK1 * 26;
.CONST $fbc.SIL_CNTR_FIELD                $fbc.INT_VAR + MK1 * 27;
.CONST $fbc.DIVERGE_FLAG_FIELD            $fbc.INT_VAR + MK1 * 28;
.CONST $fbc.LRAT_0_FIELD                  $fbc.INT_VAR + MK1 * 29;
.CONST $fbc.LRAT_1_FIELD                  $fbc.INT_VAR + MK1 * 30;
.CONST $fbc.LRAT_2_FIELD                  $fbc.INT_VAR + MK1 * 31;
.CONST $fbc.L2P_IBUF_D_FIELD              $fbc.INT_VAR + MK1 * 32;
.CONST $fbc.L2P_OBUF_D_FIELD              $fbc.INT_VAR + MK1 * 33;
.CONST $fbc.L2P_OBUF_D_1_FIELD            $fbc.INT_VAR + MK1 * 34;
.CONST $fbc.L2P_OBUF_D_2_FIELD            $fbc.INT_VAR + MK1 * 35;
.CONST $fbc.IBUF_D_PRE_PWR_FIELD          $fbc.INT_VAR + MK1 * 36;
.CONST $fbc.OBUF_D_PRE_PWR_FIELD          $fbc.INT_VAR + MK1 * 37;
.CONST $fbc.STOP_ADAPT_FIELD              $fbc.INT_VAR + MK1 * 38;
.CONST $fbc.USER0_FIELD                   $fbc.INT_VAR + MK1 * 39;
.CONST $fbc.TEMP0_FIELD                   $fbc.INT_VAR + MK1 * 40;
.CONST $fbc.TEMP1_FIELD                   $fbc.INT_VAR + MK1 * 41;

.CONST $fbc.STRUCT_SIZE                   (10 + 42);

// Legacy: AEC support utility
.CONST $fbc.L2_HD_GAIN_FIELD              $fbc.INT_VAR + MK1 * 42;
.CONST $fbc.HD_GAIN_FIELD                 $fbc.INT_VAR + MK1 * 43;

// Legacy: High Pass Filter support
.CONST $fbc.FLAG_BYPASS_HPF_FIELD         $fbc.INT_VAR + MK1 * 44;
.CONST $fbc.HPF_STREAM_FIELD              $fbc.INT_VAR + MK1 * 45;
.CONST $fbc.HPF_FILTER_FIELD              $fbc.INT_VAR + MK1 * 46;
.CONST $fbc.STREAM_D_DLY_B_FIELD          $fbc.INT_VAR + MK1 * 47;
.CONST $fbc.STREAM_X_DLY_B_FIELD          $fbc.INT_VAR + MK1 * 48;
.CONST $fbc.STREAM_D_HI_FIELD             $fbc.INT_VAR + MK1 * 49;
.CONST $fbc.STREAM_X_HI_FIELD             $fbc.INT_VAR + MK1 * 50;

.CONST $fbc.STRUCT_SIZE.EXT               (10 + 51);

#endif // FBC_LIB_ASM_H_INCLUDED
