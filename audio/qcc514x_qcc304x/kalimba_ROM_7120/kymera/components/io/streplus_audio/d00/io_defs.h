
/* *************************************************************************  *
   COMMERCIAL IN CONFIDENCE
   Copyright (C) 2018 Qualcomm Technologies International Ltd.

   Qualcomm Technologies International Ltd.
   Churchill House,
   Cambridge Business Park,
   Cowley Park,
   Cambridge, CB4 0WZ. UK
   http://www.qualcomm.com

   $Id: //depot/bg/Staging/QCC514x_QCC304x.SRC.1.0/audio/qcc514x_qcc304x/kalimba_ROM_7120/kymera/components/io/streplus_audio/d00/io_defs.h#34 $
   $Name$
   $Source$

   DESCRIPTION
      Hardware declarations header file (higher level).
      Lists masks and values for use when setting or interpreting
      the contents of memory mapped hardware registers.

   INTERFACE
      Entry   :-
      Exit    :-

   MODIFICATIONS
      1.0    24/06/99    RWY    First created.
      1.x    xx/xx/xx    RWY    Automatically generated.

*  *************************************************************************  */

#define __IO_DEFS_H__

#define CHIP_REGISTER_HASH 0x7825




#if !defined(__IO_DEFS_H__IO_DEFS_MODULE_K32_TRACE) 
#define __IO_DEFS_H__IO_DEFS_MODULE_K32_TRACE

/* -- k32_trace -- Kalimba 32-bit Trace Control registers. -- */

enum trace_cfg_enum_posn_enum
{
   TRACE_TRIGGER_CFG_END_TRIG_EN_POSN                 = (int)0,
   TRACE_CFG_ENUM_TRACE_TRIGGER_CFG_END_TRIG_EN_LSB_POSN = (int)0,
   TRACE_CFG_ENUM_TRACE_TRIGGER_CFG_END_TRIG_EN_MSB_POSN = (int)0,
   TRACE_TRIGGER_CFG_START_TRIG_EN_POSN               = (int)1,
   TRACE_CFG_ENUM_TRACE_TRIGGER_CFG_START_TRIG_EN_LSB_POSN = (int)1,
   TRACE_CFG_ENUM_TRACE_TRIGGER_CFG_START_TRIG_EN_MSB_POSN = (int)1,
   TRACE_TRIGGER_CFG_TRIGGER_LENGTH_LSB_POSN          = (int)2,
   TRACE_CFG_ENUM_TRACE_TRIGGER_CFG_TRIGGER_LENGTH_LSB_POSN = (int)2,
   TRACE_TRIGGER_CFG_TRIGGER_LENGTH_MSB_POSN          = (int)11,
   TRACE_CFG_ENUM_TRACE_TRIGGER_CFG_TRIGGER_LENGTH_MSB_POSN = (int)11
};
typedef enum trace_cfg_enum_posn_enum trace_cfg_enum_posn;

#define TRACE_TRIGGER_CFG_END_TRIG_EN_MASK       (0x00000001u)
#define TRACE_TRIGGER_CFG_START_TRIG_EN_MASK     (0x00000002u)
#define TRACE_TRIGGER_CFG_TRIGGER_LENGTH_LSB_MASK (0x00000004u)
#define TRACE_TRIGGER_CFG_TRIGGER_LENGTH_MSB_MASK (0x00000800u)

enum trace_trig_status_enum_posn_enum
{
   TRACE_TRIGGER_STATUS_START_FOUND_POSN              = (int)0,
   TRACE_TRIG_STATUS_ENUM_TRACE_TRIGGER_STATUS_START_FOUND_LSB_POSN = (int)0,
   TRACE_TRIG_STATUS_ENUM_TRACE_TRIGGER_STATUS_START_FOUND_MSB_POSN = (int)0,
   TRACE_TRIGGER_STATUS_START_COMPL_POSN              = (int)1,
   TRACE_TRIG_STATUS_ENUM_TRACE_TRIGGER_STATUS_START_COMPL_LSB_POSN = (int)1,
   TRACE_TRIG_STATUS_ENUM_TRACE_TRIGGER_STATUS_START_COMPL_MSB_POSN = (int)1,
   TRACE_TRIGGER_STATUS_END_FOUND_POSN                = (int)2,
   TRACE_TRIG_STATUS_ENUM_TRACE_TRIGGER_STATUS_END_FOUND_LSB_POSN = (int)2,
   TRACE_TRIG_STATUS_ENUM_TRACE_TRIGGER_STATUS_END_FOUND_MSB_POSN = (int)2,
   TRACE_TRIGGER_STATUS_END_COMPL_POSN                = (int)3,
   TRACE_TRIG_STATUS_ENUM_TRACE_TRIGGER_STATUS_END_COMPL_LSB_POSN = (int)3,
   TRACE_TRIG_STATUS_ENUM_TRACE_TRIGGER_STATUS_END_COMPL_MSB_POSN = (int)3,
   TRACE_TRIGGER_STATUS_SMDBG_LSB_POSN                = (int)4,
   TRACE_TRIG_STATUS_ENUM_TRACE_TRIGGER_STATUS_SMDBG_LSB_POSN = (int)4,
   TRACE_TRIGGER_STATUS_SMDBG_MSB_POSN                = (int)5,
   TRACE_TRIG_STATUS_ENUM_TRACE_TRIGGER_STATUS_SMDBG_MSB_POSN = (int)5
};
typedef enum trace_trig_status_enum_posn_enum trace_trig_status_enum_posn;

#define TRACE_TRIGGER_STATUS_START_FOUND_MASK    (0x00000001u)
#define TRACE_TRIGGER_STATUS_START_COMPL_MASK    (0x00000002u)
#define TRACE_TRIGGER_STATUS_END_FOUND_MASK      (0x00000004u)
#define TRACE_TRIGGER_STATUS_END_COMPL_MASK      (0x00000008u)
#define TRACE_TRIGGER_STATUS_SMDBG_LSB_MASK      (0x00000010u)
#define TRACE_TRIGGER_STATUS_SMDBG_MSB_MASK      (0x00000020u)

enum trace_0_cfg_posn_enum
{
   TRACE_CFG_0_ENABLE_POSN                            = (int)0,
   TRACE_0_CFG_TRACE_CFG_0_ENABLE_LSB_POSN            = (int)0,
   TRACE_0_CFG_TRACE_CFG_0_ENABLE_MSB_POSN            = (int)0,
   TRACE_CFG_0_SYNC_INTERVAL_LSB_POSN                 = (int)1,
   TRACE_0_CFG_TRACE_CFG_0_SYNC_INTERVAL_LSB_POSN     = (int)1,
   TRACE_CFG_0_SYNC_INTERVAL_MSB_POSN                 = (int)3,
   TRACE_0_CFG_TRACE_CFG_0_SYNC_INTERVAL_MSB_POSN     = (int)3,
   TRACE_CFG_0_CPU_SELECT_LSB_POSN                    = (int)4,
   TRACE_0_CFG_TRACE_CFG_0_CPU_SELECT_LSB_POSN        = (int)4,
   TRACE_CFG_0_CPU_SELECT_MSB_POSN                    = (int)5,
   TRACE_0_CFG_TRACE_CFG_0_CPU_SELECT_MSB_POSN        = (int)5,
   TRACE_CFG_0_FLUSH_FIFO_POSN                        = (int)6,
   TRACE_0_CFG_TRACE_CFG_0_FLUSH_FIFO_LSB_POSN        = (int)6,
   TRACE_0_CFG_TRACE_CFG_0_FLUSH_FIFO_MSB_POSN        = (int)6,
   TRACE_CFG_0_STALL_CORE_ON_TRACE_FULL_POSN          = (int)7,
   TRACE_0_CFG_TRACE_CFG_0_STALL_CORE_ON_TRACE_FULL_LSB_POSN = (int)7,
   TRACE_0_CFG_TRACE_CFG_0_STALL_CORE_ON_TRACE_FULL_MSB_POSN = (int)7,
   TRACE_CFG_0_CLR_STORED_ON_SYNC_POSN                = (int)8,
   TRACE_0_CFG_TRACE_CFG_0_CLR_STORED_ON_SYNC_LSB_POSN = (int)8,
   TRACE_0_CFG_TRACE_CFG_0_CLR_STORED_ON_SYNC_MSB_POSN = (int)8,
   TRACE_CFG_0_FLUSH_BITGEN_POSN                      = (int)9,
   TRACE_0_CFG_TRACE_CFG_0_FLUSH_BITGEN_LSB_POSN      = (int)9,
   TRACE_0_CFG_TRACE_CFG_0_FLUSH_BITGEN_MSB_POSN      = (int)9
};
typedef enum trace_0_cfg_posn_enum trace_0_cfg_posn;

#define TRACE_CFG_0_ENABLE_MASK                  (0x00000001u)
#define TRACE_CFG_0_SYNC_INTERVAL_LSB_MASK       (0x00000002u)
#define TRACE_CFG_0_SYNC_INTERVAL_MSB_MASK       (0x00000008u)
#define TRACE_CFG_0_CPU_SELECT_LSB_MASK          (0x00000010u)
#define TRACE_CFG_0_CPU_SELECT_MSB_MASK          (0x00000020u)
#define TRACE_CFG_0_FLUSH_FIFO_MASK              (0x00000040u)
#define TRACE_CFG_0_STALL_CORE_ON_TRACE_FULL_MASK (0x00000080u)
#define TRACE_CFG_0_CLR_STORED_ON_SYNC_MASK      (0x00000100u)
#define TRACE_CFG_0_FLUSH_BITGEN_MASK            (0x00000200u)

enum trace_0_dmem_cfg_posn_enum
{
   TRACE_0_DMEM_EN_POSN                               = (int)0,
   TRACE_0_DMEM_CFG_TRACE_0_DMEM_EN_LSB_POSN          = (int)0,
   TRACE_0_DMEM_CFG_TRACE_0_DMEM_EN_MSB_POSN          = (int)0,
   TRACE_0_DMEM_CFG_WRAP_POSN                         = (int)1,
   TRACE_0_DMEM_CFG_TRACE_0_DMEM_CFG_WRAP_LSB_POSN    = (int)1,
   TRACE_0_DMEM_CFG_TRACE_0_DMEM_CFG_WRAP_MSB_POSN    = (int)1,
   TRACE_0_DMEM_CFG_LENGTH_LSB_POSN                   = (int)2,
   TRACE_0_DMEM_CFG_TRACE_0_DMEM_CFG_LENGTH_LSB_POSN  = (int)2,
   TRACE_0_DMEM_CFG_LENGTH_MSB_POSN                   = (int)12,
   TRACE_0_DMEM_CFG_TRACE_0_DMEM_CFG_LENGTH_MSB_POSN  = (int)12
};
typedef enum trace_0_dmem_cfg_posn_enum trace_0_dmem_cfg_posn;

#define TRACE_0_DMEM_EN_MASK                     (0x00000001u)
#define TRACE_0_DMEM_CFG_WRAP_MASK               (0x00000002u)
#define TRACE_0_DMEM_CFG_LENGTH_LSB_MASK         (0x00000004u)
#define TRACE_0_DMEM_CFG_LENGTH_MSB_MASK         (0x00001000u)

enum trace_0_tbus_cfg_posn_enum
{
   TRACE_0_TBUS_EN_POSN                               = (int)0,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_EN_LSB_POSN          = (int)0,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_EN_MSB_POSN          = (int)0,
   TRACE_0_TBUS_CFG_TRAN_TYPE_POSN                    = (int)1,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_TRAN_TYPE_LSB_POSN = (int)1,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_TRAN_TYPE_MSB_POSN = (int)1,
   TRACE_0_TBUS_CFG_WRAP_POSN                         = (int)2,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_WRAP_LSB_POSN    = (int)2,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_WRAP_MSB_POSN    = (int)2,
   TRACE_0_TBUS_CFG_DEST_SYS_LSB_POSN                 = (int)3,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_DEST_SYS_LSB_POSN = (int)3,
   TRACE_0_TBUS_CFG_DEST_SYS_MSB_POSN                 = (int)6,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_DEST_SYS_MSB_POSN = (int)6,
   TRACE_0_TBUS_CFG_DEST_BLK_LSB_POSN                 = (int)7,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_DEST_BLK_LSB_POSN = (int)7,
   TRACE_0_TBUS_CFG_DEST_BLK_MSB_POSN                 = (int)10,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_DEST_BLK_MSB_POSN = (int)10,
   TRACE_0_TBUS_CFG_SRC_BLK_LSB_POSN                  = (int)11,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_SRC_BLK_LSB_POSN = (int)11,
   TRACE_0_TBUS_CFG_SRC_BLK_MSB_POSN                  = (int)14,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_SRC_BLK_MSB_POSN = (int)14,
   TRACE_0_TBUS_CFG_TAG_LSB_POSN                      = (int)15,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_TAG_LSB_POSN     = (int)15,
   TRACE_0_TBUS_CFG_TAG_MSB_POSN                      = (int)18,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_TAG_MSB_POSN     = (int)18,
   TRACE_0_TBUS_CFG_LENGTH_LSB_POSN                   = (int)19,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_LENGTH_LSB_POSN  = (int)19,
   TRACE_0_TBUS_CFG_LENGTH_MSB_POSN                   = (int)29,
   TRACE_0_TBUS_CFG_TRACE_0_TBUS_CFG_LENGTH_MSB_POSN  = (int)29
};
typedef enum trace_0_tbus_cfg_posn_enum trace_0_tbus_cfg_posn;

#define TRACE_0_TBUS_EN_MASK                     (0x00000001u)
#define TRACE_0_TBUS_CFG_TRAN_TYPE_MASK          (0x00000002u)
#define TRACE_0_TBUS_CFG_WRAP_MASK               (0x00000004u)
#define TRACE_0_TBUS_CFG_DEST_SYS_LSB_MASK       (0x00000008u)
#define TRACE_0_TBUS_CFG_DEST_SYS_MSB_MASK       (0x00000040u)
#define TRACE_0_TBUS_CFG_DEST_BLK_LSB_MASK       (0x00000080u)
#define TRACE_0_TBUS_CFG_DEST_BLK_MSB_MASK       (0x00000400u)
#define TRACE_0_TBUS_CFG_SRC_BLK_LSB_MASK        (0x00000800u)
#define TRACE_0_TBUS_CFG_SRC_BLK_MSB_MASK        (0x00004000u)
#define TRACE_0_TBUS_CFG_TAG_LSB_MASK            (0x00008000u)
#define TRACE_0_TBUS_CFG_TAG_MSB_MASK            (0x00040000u)
#define TRACE_0_TBUS_CFG_LENGTH_LSB_MASK         (0x00080000u)
#define TRACE_0_TBUS_CFG_LENGTH_MSB_MASK         (0x20000000u)

enum trace_1_cfg_posn_enum
{
   TRACE_CFG_1_ENABLE_POSN                            = (int)0,
   TRACE_1_CFG_TRACE_CFG_1_ENABLE_LSB_POSN            = (int)0,
   TRACE_1_CFG_TRACE_CFG_1_ENABLE_MSB_POSN            = (int)0,
   TRACE_CFG_1_SYNC_INTERVAL_LSB_POSN                 = (int)1,
   TRACE_1_CFG_TRACE_CFG_1_SYNC_INTERVAL_LSB_POSN     = (int)1,
   TRACE_CFG_1_SYNC_INTERVAL_MSB_POSN                 = (int)3,
   TRACE_1_CFG_TRACE_CFG_1_SYNC_INTERVAL_MSB_POSN     = (int)3,
   TRACE_CFG_1_CPU_SELECT_LSB_POSN                    = (int)4,
   TRACE_1_CFG_TRACE_CFG_1_CPU_SELECT_LSB_POSN        = (int)4,
   TRACE_CFG_1_CPU_SELECT_MSB_POSN                    = (int)5,
   TRACE_1_CFG_TRACE_CFG_1_CPU_SELECT_MSB_POSN        = (int)5,
   TRACE_CFG_1_FLUSH_FIFO_POSN                        = (int)6,
   TRACE_1_CFG_TRACE_CFG_1_FLUSH_FIFO_LSB_POSN        = (int)6,
   TRACE_1_CFG_TRACE_CFG_1_FLUSH_FIFO_MSB_POSN        = (int)6,
   TRACE_CFG_1_STALL_CORE_ON_TRACE_FULL_POSN          = (int)7,
   TRACE_1_CFG_TRACE_CFG_1_STALL_CORE_ON_TRACE_FULL_LSB_POSN = (int)7,
   TRACE_1_CFG_TRACE_CFG_1_STALL_CORE_ON_TRACE_FULL_MSB_POSN = (int)7,
   TRACE_CFG_1_CLR_STORED_ON_SYNC_POSN                = (int)8,
   TRACE_1_CFG_TRACE_CFG_1_CLR_STORED_ON_SYNC_LSB_POSN = (int)8,
   TRACE_1_CFG_TRACE_CFG_1_CLR_STORED_ON_SYNC_MSB_POSN = (int)8,
   TRACE_CFG_1_FLUSH_BITGEN_POSN                      = (int)9,
   TRACE_1_CFG_TRACE_CFG_1_FLUSH_BITGEN_LSB_POSN      = (int)9,
   TRACE_1_CFG_TRACE_CFG_1_FLUSH_BITGEN_MSB_POSN      = (int)9
};
typedef enum trace_1_cfg_posn_enum trace_1_cfg_posn;

#define TRACE_CFG_1_ENABLE_MASK                  (0x00000001u)
#define TRACE_CFG_1_SYNC_INTERVAL_LSB_MASK       (0x00000002u)
#define TRACE_CFG_1_SYNC_INTERVAL_MSB_MASK       (0x00000008u)
#define TRACE_CFG_1_CPU_SELECT_LSB_MASK          (0x00000010u)
#define TRACE_CFG_1_CPU_SELECT_MSB_MASK          (0x00000020u)
#define TRACE_CFG_1_FLUSH_FIFO_MASK              (0x00000040u)
#define TRACE_CFG_1_STALL_CORE_ON_TRACE_FULL_MASK (0x00000080u)
#define TRACE_CFG_1_CLR_STORED_ON_SYNC_MASK      (0x00000100u)
#define TRACE_CFG_1_FLUSH_BITGEN_MASK            (0x00000200u)

enum trace_1_dmem_cfg_posn_enum
{
   TRACE_1_DMEM_EN_POSN                               = (int)0,
   TRACE_1_DMEM_CFG_TRACE_1_DMEM_EN_LSB_POSN          = (int)0,
   TRACE_1_DMEM_CFG_TRACE_1_DMEM_EN_MSB_POSN          = (int)0,
   TRACE_1_DMEM_CFG_WRAP_POSN                         = (int)1,
   TRACE_1_DMEM_CFG_TRACE_1_DMEM_CFG_WRAP_LSB_POSN    = (int)1,
   TRACE_1_DMEM_CFG_TRACE_1_DMEM_CFG_WRAP_MSB_POSN    = (int)1,
   TRACE_1_DMEM_CFG_LENGTH_LSB_POSN                   = (int)2,
   TRACE_1_DMEM_CFG_TRACE_1_DMEM_CFG_LENGTH_LSB_POSN  = (int)2,
   TRACE_1_DMEM_CFG_LENGTH_MSB_POSN                   = (int)12,
   TRACE_1_DMEM_CFG_TRACE_1_DMEM_CFG_LENGTH_MSB_POSN  = (int)12
};
typedef enum trace_1_dmem_cfg_posn_enum trace_1_dmem_cfg_posn;

#define TRACE_1_DMEM_EN_MASK                     (0x00000001u)
#define TRACE_1_DMEM_CFG_WRAP_MASK               (0x00000002u)
#define TRACE_1_DMEM_CFG_LENGTH_LSB_MASK         (0x00000004u)
#define TRACE_1_DMEM_CFG_LENGTH_MSB_MASK         (0x00001000u)

enum trace_1_tbus_cfg_posn_enum
{
   TRACE_1_TBUS_EN_POSN                               = (int)0,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_EN_LSB_POSN          = (int)0,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_EN_MSB_POSN          = (int)0,
   TRACE_1_TBUS_CFG_TRAN_TYPE_POSN                    = (int)1,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_TRAN_TYPE_LSB_POSN = (int)1,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_TRAN_TYPE_MSB_POSN = (int)1,
   TRACE_1_TBUS_CFG_WRAP_POSN                         = (int)2,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_WRAP_LSB_POSN    = (int)2,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_WRAP_MSB_POSN    = (int)2,
   TRACE_1_TBUS_CFG_DEST_SYS_LSB_POSN                 = (int)3,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_DEST_SYS_LSB_POSN = (int)3,
   TRACE_1_TBUS_CFG_DEST_SYS_MSB_POSN                 = (int)6,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_DEST_SYS_MSB_POSN = (int)6,
   TRACE_1_TBUS_CFG_DEST_BLK_LSB_POSN                 = (int)7,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_DEST_BLK_LSB_POSN = (int)7,
   TRACE_1_TBUS_CFG_DEST_BLK_MSB_POSN                 = (int)10,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_DEST_BLK_MSB_POSN = (int)10,
   TRACE_1_TBUS_CFG_SRC_BLK_LSB_POSN                  = (int)11,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_SRC_BLK_LSB_POSN = (int)11,
   TRACE_1_TBUS_CFG_SRC_BLK_MSB_POSN                  = (int)14,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_SRC_BLK_MSB_POSN = (int)14,
   TRACE_1_TBUS_CFG_TAG_LSB_POSN                      = (int)15,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_TAG_LSB_POSN     = (int)15,
   TRACE_1_TBUS_CFG_TAG_MSB_POSN                      = (int)18,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_TAG_MSB_POSN     = (int)18,
   TRACE_1_TBUS_CFG_LENGTH_LSB_POSN                   = (int)19,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_LENGTH_LSB_POSN  = (int)19,
   TRACE_1_TBUS_CFG_LENGTH_MSB_POSN                   = (int)29,
   TRACE_1_TBUS_CFG_TRACE_1_TBUS_CFG_LENGTH_MSB_POSN  = (int)29
};
typedef enum trace_1_tbus_cfg_posn_enum trace_1_tbus_cfg_posn;

#define TRACE_1_TBUS_EN_MASK                     (0x00000001u)
#define TRACE_1_TBUS_CFG_TRAN_TYPE_MASK          (0x00000002u)
#define TRACE_1_TBUS_CFG_WRAP_MASK               (0x00000004u)
#define TRACE_1_TBUS_CFG_DEST_SYS_LSB_MASK       (0x00000008u)
#define TRACE_1_TBUS_CFG_DEST_SYS_MSB_MASK       (0x00000040u)
#define TRACE_1_TBUS_CFG_DEST_BLK_LSB_MASK       (0x00000080u)
#define TRACE_1_TBUS_CFG_DEST_BLK_MSB_MASK       (0x00000400u)
#define TRACE_1_TBUS_CFG_SRC_BLK_LSB_MASK        (0x00000800u)
#define TRACE_1_TBUS_CFG_SRC_BLK_MSB_MASK        (0x00004000u)
#define TRACE_1_TBUS_CFG_TAG_LSB_MASK            (0x00008000u)
#define TRACE_1_TBUS_CFG_TAG_MSB_MASK            (0x00040000u)
#define TRACE_1_TBUS_CFG_LENGTH_LSB_MASK         (0x00080000u)
#define TRACE_1_TBUS_CFG_LENGTH_MSB_MASK         (0x20000000u)

enum trace_dmem_status_posn_enum
{
   TRACE_DMEM_STATUS_CNTL_0_DUMP_DONE_POSN            = (int)0,
   TRACE_DMEM_STATUS_TRACE_DMEM_STATUS_CNTL_0_DUMP_DONE_LSB_POSN = (int)0,
   TRACE_DMEM_STATUS_TRACE_DMEM_STATUS_CNTL_0_DUMP_DONE_MSB_POSN = (int)0,
   TRACE_DMEM_STATUS_CNTL_1_DUMP_DONE_POSN            = (int)1,
   TRACE_DMEM_STATUS_TRACE_DMEM_STATUS_CNTL_1_DUMP_DONE_LSB_POSN = (int)1,
   TRACE_DMEM_STATUS_TRACE_DMEM_STATUS_CNTL_1_DUMP_DONE_MSB_POSN = (int)1,
   TRACE_DMEM_STATUS_CNTL_2_DUMP_DONE_POSN            = (int)2,
   TRACE_DMEM_STATUS_TRACE_DMEM_STATUS_CNTL_2_DUMP_DONE_LSB_POSN = (int)2,
   TRACE_DMEM_STATUS_TRACE_DMEM_STATUS_CNTL_2_DUMP_DONE_MSB_POSN = (int)2,
   TRACE_DMEM_STATUS_CNTL_3_DUMP_DONE_POSN            = (int)3,
   TRACE_DMEM_STATUS_TRACE_DMEM_STATUS_CNTL_3_DUMP_DONE_LSB_POSN = (int)3,
   TRACE_DMEM_STATUS_TRACE_DMEM_STATUS_CNTL_3_DUMP_DONE_MSB_POSN = (int)3
};
typedef enum trace_dmem_status_posn_enum trace_dmem_status_posn;

#define TRACE_DMEM_STATUS_CNTL_0_DUMP_DONE_MASK  (0x00000001u)
#define TRACE_DMEM_STATUS_CNTL_1_DUMP_DONE_MASK  (0x00000002u)
#define TRACE_DMEM_STATUS_CNTL_2_DUMP_DONE_MASK  (0x00000004u)
#define TRACE_DMEM_STATUS_CNTL_3_DUMP_DONE_MASK  (0x00000008u)

enum trace_tbus_status_posn_enum
{
   TRACE_TBUS_STATUS_CNTL_0_DUMP_DONE_POSN            = (int)0,
   TRACE_TBUS_STATUS_TRACE_TBUS_STATUS_CNTL_0_DUMP_DONE_LSB_POSN = (int)0,
   TRACE_TBUS_STATUS_TRACE_TBUS_STATUS_CNTL_0_DUMP_DONE_MSB_POSN = (int)0,
   TRACE_TBUS_STATUS_CNTL_1_DUMP_DONE_POSN            = (int)1,
   TRACE_TBUS_STATUS_TRACE_TBUS_STATUS_CNTL_1_DUMP_DONE_LSB_POSN = (int)1,
   TRACE_TBUS_STATUS_TRACE_TBUS_STATUS_CNTL_1_DUMP_DONE_MSB_POSN = (int)1,
   TRACE_TBUS_STATUS_CNTL_2_DUMP_DONE_POSN            = (int)2,
   TRACE_TBUS_STATUS_TRACE_TBUS_STATUS_CNTL_2_DUMP_DONE_LSB_POSN = (int)2,
   TRACE_TBUS_STATUS_TRACE_TBUS_STATUS_CNTL_2_DUMP_DONE_MSB_POSN = (int)2,
   TRACE_TBUS_STATUS_CNTL_3_DUMP_DONE_POSN            = (int)3,
   TRACE_TBUS_STATUS_TRACE_TBUS_STATUS_CNTL_3_DUMP_DONE_LSB_POSN = (int)3,
   TRACE_TBUS_STATUS_TRACE_TBUS_STATUS_CNTL_3_DUMP_DONE_MSB_POSN = (int)3
};
typedef enum trace_tbus_status_posn_enum trace_tbus_status_posn;

#define TRACE_TBUS_STATUS_CNTL_0_DUMP_DONE_MASK  (0x00000001u)
#define TRACE_TBUS_STATUS_CNTL_1_DUMP_DONE_MASK  (0x00000002u)
#define TRACE_TBUS_STATUS_CNTL_2_DUMP_DONE_MASK  (0x00000004u)
#define TRACE_TBUS_STATUS_CNTL_3_DUMP_DONE_MASK  (0x00000008u)

enum trace_0_dmem_base_addr_posn_enum
{
   TRACE_0_DMEM_BASE_ADDR_LSB_POSN                    = (int)0,
   TRACE_0_DMEM_BASE_ADDR_TRACE_0_DMEM_BASE_ADDR_LSB_POSN = (int)0,
   TRACE_0_DMEM_BASE_ADDR_MSB_POSN                    = (int)31,
   TRACE_0_DMEM_BASE_ADDR_TRACE_0_DMEM_BASE_ADDR_MSB_POSN = (int)31
};
typedef enum trace_0_dmem_base_addr_posn_enum trace_0_dmem_base_addr_posn;

#define TRACE_0_DMEM_BASE_ADDR_LSB_MASK          (0x00000001u)
#define TRACE_0_DMEM_BASE_ADDR_MSB_MASK          (0x80000000u)

enum trace_0_end_trigger_posn_enum
{
   TRACE_0_END_TRIGGER_LSB_POSN                       = (int)0,
   TRACE_0_END_TRIGGER_TRACE_0_END_TRIGGER_LSB_POSN   = (int)0,
   TRACE_0_END_TRIGGER_MSB_POSN                       = (int)31,
   TRACE_0_END_TRIGGER_TRACE_0_END_TRIGGER_MSB_POSN   = (int)31
};
typedef enum trace_0_end_trigger_posn_enum trace_0_end_trigger_posn;

#define TRACE_0_END_TRIGGER_LSB_MASK             (0x00000001u)
#define TRACE_0_END_TRIGGER_MSB_MASK             (0x80000000u)

enum trace_0_start_trigger_posn_enum
{
   TRACE_0_START_TRIGGER_LSB_POSN                     = (int)0,
   TRACE_0_START_TRIGGER_TRACE_0_START_TRIGGER_LSB_POSN = (int)0,
   TRACE_0_START_TRIGGER_MSB_POSN                     = (int)31,
   TRACE_0_START_TRIGGER_TRACE_0_START_TRIGGER_MSB_POSN = (int)31
};
typedef enum trace_0_start_trigger_posn_enum trace_0_start_trigger_posn;

#define TRACE_0_START_TRIGGER_LSB_MASK           (0x00000001u)
#define TRACE_0_START_TRIGGER_MSB_MASK           (0x80000000u)

enum trace_0_tbus_base_addr_posn_enum
{
   TRACE_0_TBUS_BASE_ADDR_LSB_POSN                    = (int)0,
   TRACE_0_TBUS_BASE_ADDR_TRACE_0_TBUS_BASE_ADDR_LSB_POSN = (int)0,
   TRACE_0_TBUS_BASE_ADDR_MSB_POSN                    = (int)31,
   TRACE_0_TBUS_BASE_ADDR_TRACE_0_TBUS_BASE_ADDR_MSB_POSN = (int)31
};
typedef enum trace_0_tbus_base_addr_posn_enum trace_0_tbus_base_addr_posn;

#define TRACE_0_TBUS_BASE_ADDR_LSB_MASK          (0x00000001u)
#define TRACE_0_TBUS_BASE_ADDR_MSB_MASK          (0x80000000u)

enum trace_1_dmem_base_addr_posn_enum
{
   TRACE_1_DMEM_BASE_ADDR_LSB_POSN                    = (int)0,
   TRACE_1_DMEM_BASE_ADDR_TRACE_1_DMEM_BASE_ADDR_LSB_POSN = (int)0,
   TRACE_1_DMEM_BASE_ADDR_MSB_POSN                    = (int)31,
   TRACE_1_DMEM_BASE_ADDR_TRACE_1_DMEM_BASE_ADDR_MSB_POSN = (int)31
};
typedef enum trace_1_dmem_base_addr_posn_enum trace_1_dmem_base_addr_posn;

#define TRACE_1_DMEM_BASE_ADDR_LSB_MASK          (0x00000001u)
#define TRACE_1_DMEM_BASE_ADDR_MSB_MASK          (0x80000000u)

enum trace_1_end_trigger_posn_enum
{
   TRACE_1_END_TRIGGER_LSB_POSN                       = (int)0,
   TRACE_1_END_TRIGGER_TRACE_1_END_TRIGGER_LSB_POSN   = (int)0,
   TRACE_1_END_TRIGGER_MSB_POSN                       = (int)31,
   TRACE_1_END_TRIGGER_TRACE_1_END_TRIGGER_MSB_POSN   = (int)31
};
typedef enum trace_1_end_trigger_posn_enum trace_1_end_trigger_posn;

#define TRACE_1_END_TRIGGER_LSB_MASK             (0x00000001u)
#define TRACE_1_END_TRIGGER_MSB_MASK             (0x80000000u)

enum trace_1_start_trigger_posn_enum
{
   TRACE_1_START_TRIGGER_LSB_POSN                     = (int)0,
   TRACE_1_START_TRIGGER_TRACE_1_START_TRIGGER_LSB_POSN = (int)0,
   TRACE_1_START_TRIGGER_MSB_POSN                     = (int)31,
   TRACE_1_START_TRIGGER_TRACE_1_START_TRIGGER_MSB_POSN = (int)31
};
typedef enum trace_1_start_trigger_posn_enum trace_1_start_trigger_posn;

#define TRACE_1_START_TRIGGER_LSB_MASK           (0x00000001u)
#define TRACE_1_START_TRIGGER_MSB_MASK           (0x80000000u)

enum trace_1_tbus_base_addr_posn_enum
{
   TRACE_1_TBUS_BASE_ADDR_LSB_POSN                    = (int)0,
   TRACE_1_TBUS_BASE_ADDR_TRACE_1_TBUS_BASE_ADDR_LSB_POSN = (int)0,
   TRACE_1_TBUS_BASE_ADDR_MSB_POSN                    = (int)31,
   TRACE_1_TBUS_BASE_ADDR_TRACE_1_TBUS_BASE_ADDR_MSB_POSN = (int)31
};
typedef enum trace_1_tbus_base_addr_posn_enum trace_1_tbus_base_addr_posn;

#define TRACE_1_TBUS_BASE_ADDR_LSB_MASK          (0x00000001u)
#define TRACE_1_TBUS_BASE_ADDR_MSB_MASK          (0x80000000u)

enum trace_debug_sel_posn_enum
{
   TRACE_DEBUG_SEL_LSB_POSN                           = (int)0,
   TRACE_DEBUG_SEL_TRACE_DEBUG_SEL_LSB_POSN           = (int)0,
   TRACE_DEBUG_SEL_MSB_POSN                           = (int)3,
   TRACE_DEBUG_SEL_TRACE_DEBUG_SEL_MSB_POSN           = (int)3
};
typedef enum trace_debug_sel_posn_enum trace_debug_sel_posn;

#define TRACE_DEBUG_SEL_LSB_MASK                 (0x00000001u)
#define TRACE_DEBUG_SEL_MSB_MASK                 (0x00000008u)

enum k32_trace__access_ctrl_enum_posn_enum
{
   K32_TRACE__P0_ACCESS_PERMISSION_POSN               = (int)0,
   K32_TRACE__ACCESS_CTRL_ENUM_K32_TRACE__P0_ACCESS_PERMISSION_LSB_POSN = (int)0,
   K32_TRACE__ACCESS_CTRL_ENUM_K32_TRACE__P0_ACCESS_PERMISSION_MSB_POSN = (int)0,
   K32_TRACE__P1_ACCESS_PERMISSION_POSN               = (int)1,
   K32_TRACE__ACCESS_CTRL_ENUM_K32_TRACE__P1_ACCESS_PERMISSION_LSB_POSN = (int)1,
   K32_TRACE__ACCESS_CTRL_ENUM_K32_TRACE__P1_ACCESS_PERMISSION_MSB_POSN = (int)1,
   K32_TRACE__P2_ACCESS_PERMISSION_POSN               = (int)2,
   K32_TRACE__ACCESS_CTRL_ENUM_K32_TRACE__P2_ACCESS_PERMISSION_LSB_POSN = (int)2,
   K32_TRACE__ACCESS_CTRL_ENUM_K32_TRACE__P2_ACCESS_PERMISSION_MSB_POSN = (int)2,
   K32_TRACE__P3_ACCESS_PERMISSION_POSN               = (int)3,
   K32_TRACE__ACCESS_CTRL_ENUM_K32_TRACE__P3_ACCESS_PERMISSION_LSB_POSN = (int)3,
   K32_TRACE__ACCESS_CTRL_ENUM_K32_TRACE__P3_ACCESS_PERMISSION_MSB_POSN = (int)3
};
typedef enum k32_trace__access_ctrl_enum_posn_enum k32_trace__access_ctrl_enum_posn;

#define K32_TRACE__P0_ACCESS_PERMISSION_MASK     (0x00000001u)
#define K32_TRACE__P1_ACCESS_PERMISSION_MASK     (0x00000002u)
#define K32_TRACE__P2_ACCESS_PERMISSION_MASK     (0x00000004u)
#define K32_TRACE__P3_ACCESS_PERMISSION_MASK     (0x00000008u)

enum k32_trace__p0_access_permission_enum
{
   K32_TRACE__P0_ACCESS_BLOCKED             = (int)0x0,
   K32_TRACE__P0_ACCESS_UNBLOCKED           = (int)0x1,
   MAX_K32_TRACE__P0_ACCESS_PERMISSION      = (int)0x1
};
#define NUM_K32_TRACE__P0_ACCESS_PERMISSION (0x2)
typedef enum k32_trace__p0_access_permission_enum k32_trace__p0_access_permission;


enum k32_trace__p1_access_permission_enum
{
   K32_TRACE__P1_ACCESS_BLOCKED             = (int)0x0,
   K32_TRACE__P1_ACCESS_UNBLOCKED           = (int)0x1,
   MAX_K32_TRACE__P1_ACCESS_PERMISSION      = (int)0x1
};
#define NUM_K32_TRACE__P1_ACCESS_PERMISSION (0x2)
typedef enum k32_trace__p1_access_permission_enum k32_trace__p1_access_permission;


enum k32_trace__p2_access_permission_enum
{
   K32_TRACE__P2_ACCESS_BLOCKED             = (int)0x0,
   K32_TRACE__P2_ACCESS_UNBLOCKED           = (int)0x1,
   MAX_K32_TRACE__P2_ACCESS_PERMISSION      = (int)0x1
};
#define NUM_K32_TRACE__P2_ACCESS_PERMISSION (0x2)
typedef enum k32_trace__p2_access_permission_enum k32_trace__p2_access_permission;


enum k32_trace__p3_access_permission_enum
{
   K32_TRACE__P3_ACCESS_BLOCKED             = (int)0x0,
   K32_TRACE__P3_ACCESS_UNBLOCKED           = (int)0x1,
   MAX_K32_TRACE__P3_ACCESS_PERMISSION      = (int)0x1
};
#define NUM_K32_TRACE__P3_ACCESS_PERMISSION (0x2)
typedef enum k32_trace__p3_access_permission_enum k32_trace__p3_access_permission;


enum k32_trace__mutex_lock_enum_enum
{
   K32_TRACE__MUTEX_AVAILABLE               = (int)0x0,
   K32_TRACE__MUTEX_CLAIMED_BY_P0           = (int)0x1,
   K32_TRACE__MUTEX_CLAIMED_BY_P1           = (int)0x2,
   K32_TRACE__MUTEX_CLAIMED_BY_P2           = (int)0x4,
   K32_TRACE__MUTEX_CLAIMED_BY_P3           = (int)0x8,
   K32_TRACE__MUTEX_DISABLED                = (int)0xF,
   MAX_K32_TRACE__MUTEX_LOCK_ENUM           = (int)0xF
};
typedef enum k32_trace__mutex_lock_enum_enum k32_trace__mutex_lock_enum;


enum trace_0_trigger_cfg_posn_enum
{
   TRACE_0_TRIGGER_CFG_TRACE_TRIGGER_CFG_END_TRIG_EN_LSB_POSN = (int)0,
   TRACE_0_TRIGGER_CFG_TRACE_TRIGGER_CFG_END_TRIG_EN_MSB_POSN = (int)0,
   TRACE_0_TRIGGER_CFG_TRACE_TRIGGER_CFG_START_TRIG_EN_LSB_POSN = (int)1,
   TRACE_0_TRIGGER_CFG_TRACE_TRIGGER_CFG_START_TRIG_EN_MSB_POSN = (int)1,
   TRACE_0_TRIGGER_CFG_TRACE_TRIGGER_CFG_TRIGGER_LENGTH_LSB_POSN = (int)2,
   TRACE_0_TRIGGER_CFG_TRACE_TRIGGER_CFG_TRIGGER_LENGTH_MSB_POSN = (int)11
};
typedef enum trace_0_trigger_cfg_posn_enum trace_0_trigger_cfg_posn;


enum trace_0_trigger_status_posn_enum
{
   TRACE_0_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_START_FOUND_LSB_POSN = (int)0,
   TRACE_0_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_START_FOUND_MSB_POSN = (int)0,
   TRACE_0_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_START_COMPL_LSB_POSN = (int)1,
   TRACE_0_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_START_COMPL_MSB_POSN = (int)1,
   TRACE_0_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_END_FOUND_LSB_POSN = (int)2,
   TRACE_0_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_END_FOUND_MSB_POSN = (int)2,
   TRACE_0_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_END_COMPL_LSB_POSN = (int)3,
   TRACE_0_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_END_COMPL_MSB_POSN = (int)3,
   TRACE_0_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_SMDBG_LSB_POSN = (int)4,
   TRACE_0_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_SMDBG_MSB_POSN = (int)5
};
typedef enum trace_0_trigger_status_posn_enum trace_0_trigger_status_posn;


enum trace_1_trigger_cfg_posn_enum
{
   TRACE_1_TRIGGER_CFG_TRACE_TRIGGER_CFG_END_TRIG_EN_LSB_POSN = (int)0,
   TRACE_1_TRIGGER_CFG_TRACE_TRIGGER_CFG_END_TRIG_EN_MSB_POSN = (int)0,
   TRACE_1_TRIGGER_CFG_TRACE_TRIGGER_CFG_START_TRIG_EN_LSB_POSN = (int)1,
   TRACE_1_TRIGGER_CFG_TRACE_TRIGGER_CFG_START_TRIG_EN_MSB_POSN = (int)1,
   TRACE_1_TRIGGER_CFG_TRACE_TRIGGER_CFG_TRIGGER_LENGTH_LSB_POSN = (int)2,
   TRACE_1_TRIGGER_CFG_TRACE_TRIGGER_CFG_TRIGGER_LENGTH_MSB_POSN = (int)11
};
typedef enum trace_1_trigger_cfg_posn_enum trace_1_trigger_cfg_posn;


enum trace_1_trigger_status_posn_enum
{
   TRACE_1_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_START_FOUND_LSB_POSN = (int)0,
   TRACE_1_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_START_FOUND_MSB_POSN = (int)0,
   TRACE_1_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_START_COMPL_LSB_POSN = (int)1,
   TRACE_1_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_START_COMPL_MSB_POSN = (int)1,
   TRACE_1_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_END_FOUND_LSB_POSN = (int)2,
   TRACE_1_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_END_FOUND_MSB_POSN = (int)2,
   TRACE_1_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_END_COMPL_LSB_POSN = (int)3,
   TRACE_1_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_END_COMPL_MSB_POSN = (int)3,
   TRACE_1_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_SMDBG_LSB_POSN = (int)4,
   TRACE_1_TRIGGER_STATUS_TRACE_TRIGGER_STATUS_SMDBG_MSB_POSN = (int)5
};
typedef enum trace_1_trigger_status_posn_enum trace_1_trigger_status_posn;


enum trace_access_ctrl_posn_enum
{
   TRACE_ACCESS_CTRL_K32_TRACE__P0_ACCESS_PERMISSION_LSB_POSN = (int)0,
   TRACE_ACCESS_CTRL_K32_TRACE__P0_ACCESS_PERMISSION_MSB_POSN = (int)0,
   TRACE_ACCESS_CTRL_K32_TRACE__P1_ACCESS_PERMISSION_LSB_POSN = (int)1,
   TRACE_ACCESS_CTRL_K32_TRACE__P1_ACCESS_PERMISSION_MSB_POSN = (int)1,
   TRACE_ACCESS_CTRL_K32_TRACE__P2_ACCESS_PERMISSION_LSB_POSN = (int)2,
   TRACE_ACCESS_CTRL_K32_TRACE__P2_ACCESS_PERMISSION_MSB_POSN = (int)2,
   TRACE_ACCESS_CTRL_K32_TRACE__P3_ACCESS_PERMISSION_LSB_POSN = (int)3,
   TRACE_ACCESS_CTRL_K32_TRACE__P3_ACCESS_PERMISSION_MSB_POSN = (int)3
};
typedef enum trace_access_ctrl_posn_enum trace_access_ctrl_posn;


#endif /* IO_DEFS_MODULE_K32_TRACE */


#if !defined(__IO_DEFS_H__IO_DEFS_MODULE_K32_DOLOOP_CACHE) 
#define __IO_DEFS_H__IO_DEFS_MODULE_K32_DOLOOP_CACHE

/* -- k32_doloop_cache -- Kalimba 32-bit DoLoop Cache Control registers. -- */

enum doloop_cache_config_posn_enum
{
   DOLOOP_CACHE_CONFIG_DOLOOP_EN_POSN                 = (int)0,
   DOLOOP_CACHE_CONFIG_DOLOOP_CACHE_CONFIG_DOLOOP_EN_LSB_POSN = (int)0,
   DOLOOP_CACHE_CONFIG_DOLOOP_CACHE_CONFIG_DOLOOP_EN_MSB_POSN = (int)0,
   DOLOOP_CACHE_CONFIG_COUNTERS_EN_POSN               = (int)1,
   DOLOOP_CACHE_CONFIG_DOLOOP_CACHE_CONFIG_COUNTERS_EN_LSB_POSN = (int)1,
   DOLOOP_CACHE_CONFIG_DOLOOP_CACHE_CONFIG_COUNTERS_EN_MSB_POSN = (int)1
};
typedef enum doloop_cache_config_posn_enum doloop_cache_config_posn;

#define DOLOOP_CACHE_CONFIG_DOLOOP_EN_MASK       (0x00000001u)
#define DOLOOP_CACHE_CONFIG_COUNTERS_EN_MASK     (0x00000002u)

enum doloop_cache_fill_count_posn_enum
{
   DOLOOP_CACHE_FILL_COUNT_LSB_POSN                   = (int)0,
   DOLOOP_CACHE_FILL_COUNT_DOLOOP_CACHE_FILL_COUNT_LSB_POSN = (int)0,
   DOLOOP_CACHE_FILL_COUNT_MSB_POSN                   = (int)31,
   DOLOOP_CACHE_FILL_COUNT_DOLOOP_CACHE_FILL_COUNT_MSB_POSN = (int)31
};
typedef enum doloop_cache_fill_count_posn_enum doloop_cache_fill_count_posn;

#define DOLOOP_CACHE_FILL_COUNT_LSB_MASK         (0x00000001u)
#define DOLOOP_CACHE_FILL_COUNT_MSB_MASK         (0x80000000u)

enum doloop_cache_hit_count_posn_enum
{
   DOLOOP_CACHE_HIT_COUNT_LSB_POSN                    = (int)0,
   DOLOOP_CACHE_HIT_COUNT_DOLOOP_CACHE_HIT_COUNT_LSB_POSN = (int)0,
   DOLOOP_CACHE_HIT_COUNT_MSB_POSN                    = (int)31,
   DOLOOP_CACHE_HIT_COUNT_DOLOOP_CACHE_HIT_COUNT_MSB_POSN = (int)31
};
typedef enum doloop_cache_hit_count_posn_enum doloop_cache_hit_count_posn;

#define DOLOOP_CACHE_HIT_COUNT_LSB_MASK          (0x00000001u)
#define DOLOOP_CACHE_HIT_COUNT_MSB_MASK          (0x80000000u)

#endif /* IO_DEFS_MODULE_K32_DOLOOP_CACHE */





#if !defined(__IO_DEFS_H__IO_DEFS_MODULE_K32_TIMERS) 
#define __IO_DEFS_H__IO_DEFS_MODULE_K32_TIMERS

/* -- k32_timers -- Kalimba 32-bit Timers Control registers -- */

enum timer1_en_posn_enum
{
   TIMER1_EN_POSN                                     = (int)0,
   TIMER1_EN_TIMER1_EN_LSB_POSN                       = (int)0,
   TIMER1_EN_TIMER1_EN_MSB_POSN                       = (int)0
};
typedef enum timer1_en_posn_enum timer1_en_posn;

#define TIMER1_EN_MASK                           (0x00000001u)

enum timer1_trigger_posn_enum
{
   TIMER1_TRIGGER_LSB_POSN                            = (int)0,
   TIMER1_TRIGGER_TIMER1_TRIGGER_LSB_POSN             = (int)0,
   TIMER1_TRIGGER_MSB_POSN                            = (int)31,
   TIMER1_TRIGGER_TIMER1_TRIGGER_MSB_POSN             = (int)31
};
typedef enum timer1_trigger_posn_enum timer1_trigger_posn;

#define TIMER1_TRIGGER_LSB_MASK                  (0x00000001u)
#define TIMER1_TRIGGER_MSB_MASK                  (0x80000000u)

enum timer2_en_posn_enum
{
   TIMER2_EN_POSN                                     = (int)0,
   TIMER2_EN_TIMER2_EN_LSB_POSN                       = (int)0,
   TIMER2_EN_TIMER2_EN_MSB_POSN                       = (int)0
};
typedef enum timer2_en_posn_enum timer2_en_posn;

#define TIMER2_EN_MASK                           (0x00000001u)

enum timer2_trigger_posn_enum
{
   TIMER2_TRIGGER_LSB_POSN                            = (int)0,
   TIMER2_TRIGGER_TIMER2_TRIGGER_LSB_POSN             = (int)0,
   TIMER2_TRIGGER_MSB_POSN                            = (int)31,
   TIMER2_TRIGGER_TIMER2_TRIGGER_MSB_POSN             = (int)31
};
typedef enum timer2_trigger_posn_enum timer2_trigger_posn;

#define TIMER2_TRIGGER_LSB_MASK                  (0x00000001u)
#define TIMER2_TRIGGER_MSB_MASK                  (0x80000000u)

enum timer_time_posn_enum
{
   TIMER_TIME_LSB_POSN                                = (int)0,
   TIMER_TIME_TIMER_TIME_LSB_POSN                     = (int)0,
   TIMER_TIME_MSB_POSN                                = (int)31,
   TIMER_TIME_TIMER_TIME_MSB_POSN                     = (int)31
};
typedef enum timer_time_posn_enum timer_time_posn;

#define TIMER_TIME_LSB_MASK                      (0x00000001u)
#define TIMER_TIME_MSB_MASK                      (0x80000000u)

#endif /* IO_DEFS_MODULE_K32_TIMERS */




#if defined(IO_DEFS_MODULE_FREQ_COUNTER) 

#ifndef __IO_DEFS_H__IO_DEFS_MODULE_FREQ_COUNTER
#define __IO_DEFS_H__IO_DEFS_MODULE_FREQ_COUNTER

/* -- freq_counter -- Digital frequency counter -- */

enum freq_count_control_bt_posn_enum
{
   FREQ_COUNT_MON_BT_CLK_SEL_LSB_POSN                 = (int)0,
   FREQ_COUNT_CONTROL_BT_FREQ_COUNT_MON_BT_CLK_SEL_LSB_POSN = (int)0,
   FREQ_COUNT_MON_BT_CLK_SEL_MSB_POSN                 = (int)3,
   FREQ_COUNT_CONTROL_BT_FREQ_COUNT_MON_BT_CLK_SEL_MSB_POSN = (int)3
};
typedef enum freq_count_control_bt_posn_enum freq_count_control_bt_posn;

#define FREQ_COUNT_MON_BT_CLK_SEL_LSB_MASK       (0x00000001u)
#define FREQ_COUNT_MON_BT_CLK_SEL_MSB_MASK       (0x00000008u)

enum freq_count_mon_bt_clk_sel_enum
{
   FREQ_COUNT_MON_BT_LO_REF_CLK             = (int)0x0,
   FREQ_COUNT_MON_BT_RX_ADC_CLK             = (int)0x1,
   FREQ_COUNT_MON_BT_RX_ADC_TEST_CLK        = (int)0x2,
   FREQ_COUNT_MON_BT_SLOW_CLK               = (int)0x3,
   MAX_FREQ_COUNT_MON_BT_CLK_SEL            = (int)0x3
};
#define NUM_FREQ_COUNT_MON_BT_CLK_SEL (0x4)
typedef enum freq_count_mon_bt_clk_sel_enum freq_count_mon_bt_clk_sel;


enum freq_count_ring_osc_sel_nand_nor_inv_gate_enum_enum
{
   FREQ_COUNT_RING_OSC_SEL_NAND             = (int)0x0,
   FREQ_COUNT_RING_OSC_SEL_NOR              = (int)0x1,
   FREQ_COUNT_RING_OSC_SEL_INV              = (int)0x2,
   MAX_FREQ_COUNT_RING_OSC_SEL_NAND_NOR_INV_GATE_ENUM = (int)0x2
};
#define NUM_FREQ_COUNT_RING_OSC_SEL_NAND_NOR_INV_GATE_ENUM (0x3)
typedef enum freq_count_ring_osc_sel_nand_nor_inv_gate_enum_enum freq_count_ring_osc_sel_nand_nor_inv_gate_enum;


enum freq_count_control_posn_enum
{
   FREQ_COUNT_MON_CLK_SEL_LSB_POSN                    = (int)0,
   FREQ_COUNT_CONTROL_FREQ_COUNT_MON_CLK_SEL_LSB_POSN = (int)0,
   FREQ_COUNT_MON_CLK_SEL_MSB_POSN                    = (int)3,
   FREQ_COUNT_CONTROL_FREQ_COUNT_MON_CLK_SEL_MSB_POSN = (int)3,
   FREQ_COUNT_PRE_SCALE_SEL_LSB_POSN                  = (int)4,
   FREQ_COUNT_CONTROL_FREQ_COUNT_PRE_SCALE_SEL_LSB_POSN = (int)4,
   FREQ_COUNT_PRE_SCALE_SEL_MSB_POSN                  = (int)5,
   FREQ_COUNT_CONTROL_FREQ_COUNT_PRE_SCALE_SEL_MSB_POSN = (int)5,
   FREQ_COUNT_ENABLE_POSN                             = (int)6,
   FREQ_COUNT_CONTROL_FREQ_COUNT_ENABLE_LSB_POSN      = (int)6,
   FREQ_COUNT_CONTROL_FREQ_COUNT_ENABLE_MSB_POSN      = (int)6,
   FREQ_COUNT_SWAP_COUNTER_CLOCKS_POSN                = (int)7,
   FREQ_COUNT_CONTROL_FREQ_COUNT_SWAP_COUNTER_CLOCKS_LSB_POSN = (int)7,
   FREQ_COUNT_CONTROL_FREQ_COUNT_SWAP_COUNTER_CLOCKS_MSB_POSN = (int)7,
   FREQ_COUNT_RING_OSC_EN_POSN                        = (int)8,
   FREQ_COUNT_CONTROL_FREQ_COUNT_RING_OSC_EN_LSB_POSN = (int)8,
   FREQ_COUNT_CONTROL_FREQ_COUNT_RING_OSC_EN_MSB_POSN = (int)8,
   FREQ_COUNT_RING_OSC_SEL_POSN                       = (int)9,
   FREQ_COUNT_CONTROL_FREQ_COUNT_RING_OSC_SEL_LSB_POSN = (int)9,
   FREQ_COUNT_CONTROL_FREQ_COUNT_RING_OSC_SEL_MSB_POSN = (int)9,
   FREQ_COUNT_RING_OSC_SEL_NAND_NOR_INV_GATE_LSB_POSN = (int)10,
   FREQ_COUNT_CONTROL_FREQ_COUNT_RING_OSC_SEL_NAND_NOR_INV_GATE_LSB_POSN = (int)10,
   FREQ_COUNT_RING_OSC_SEL_NAND_NOR_INV_GATE_MSB_POSN = (int)11,
   FREQ_COUNT_CONTROL_FREQ_COUNT_RING_OSC_SEL_NAND_NOR_INV_GATE_MSB_POSN = (int)11
};
typedef enum freq_count_control_posn_enum freq_count_control_posn;

#define FREQ_COUNT_MON_CLK_SEL_LSB_MASK          (0x00000001u)
#define FREQ_COUNT_MON_CLK_SEL_MSB_MASK          (0x00000008u)
#define FREQ_COUNT_PRE_SCALE_SEL_LSB_MASK        (0x00000010u)
#define FREQ_COUNT_PRE_SCALE_SEL_MSB_MASK        (0x00000020u)
#define FREQ_COUNT_ENABLE_MASK                   (0x00000040u)
#define FREQ_COUNT_SWAP_COUNTER_CLOCKS_MASK      (0x00000080u)
#define FREQ_COUNT_RING_OSC_EN_MASK              (0x00000100u)
#define FREQ_COUNT_RING_OSC_SEL_MASK             (0x00000200u)
#define FREQ_COUNT_RING_OSC_SEL_NAND_NOR_INV_GATE_LSB_MASK (0x00000400u)
#define FREQ_COUNT_RING_OSC_SEL_NAND_NOR_INV_GATE_MSB_MASK (0x00000800u)

enum freq_count_pre_scale_sel_enum
{
   FREQ_COUNT_PRE_SCALE_RAW                 = (int)0x0,
   FREQ_COUNT_PRE_SCALE_DIV_2               = (int)0x1,
   FREQ_COUNT_PRE_SCALE_DIV_4               = (int)0x2,
   FREQ_COUNT_PRE_SCALE_DIV_8               = (int)0x3,
   MAX_FREQ_COUNT_PRE_SCALE_SEL             = (int)0x3
};
#define NUM_FREQ_COUNT_PRE_SCALE_SEL (0x4)
typedef enum freq_count_pre_scale_sel_enum freq_count_pre_scale_sel;


enum freq_count_state_enum
{
   FREQ_COUNT_STATE_IDLE                    = (int)0x0,
   FREQ_COUNT_STATE_PREPARING               = (int)0x1,
   FREQ_COUNT_STATE_MEASURING_FREQ          = (int)0x2,
   MAX_FREQ_COUNT_STATE                     = (int)0x2
};
#define NUM_FREQ_COUNT_STATE (0x3)
typedef enum freq_count_state_enum freq_count_state;


enum freq_count_mon_clk_edge_count_posn_enum
{
   FREQ_COUNT_MON_CLK_EDGE_COUNT_LSB_POSN             = (int)0,
   FREQ_COUNT_MON_CLK_EDGE_COUNT_FREQ_COUNT_MON_CLK_EDGE_COUNT_LSB_POSN = (int)0,
   FREQ_COUNT_MON_CLK_EDGE_COUNT_MSB_POSN             = (int)31,
   FREQ_COUNT_MON_CLK_EDGE_COUNT_FREQ_COUNT_MON_CLK_EDGE_COUNT_MSB_POSN = (int)31
};
typedef enum freq_count_mon_clk_edge_count_posn_enum freq_count_mon_clk_edge_count_posn;

#define FREQ_COUNT_MON_CLK_EDGE_COUNT_LSB_MASK   (0x00000001u)
#define FREQ_COUNT_MON_CLK_EDGE_COUNT_MSB_MASK   (0x80000000u)

enum freq_count_ref_clk_target_edge_count_posn_enum
{
   FREQ_COUNT_REF_CLK_TARGET_EDGE_COUNT_LSB_POSN      = (int)0,
   FREQ_COUNT_REF_CLK_TARGET_EDGE_COUNT_FREQ_COUNT_REF_CLK_TARGET_EDGE_COUNT_LSB_POSN = (int)0,
   FREQ_COUNT_REF_CLK_TARGET_EDGE_COUNT_MSB_POSN      = (int)31,
   FREQ_COUNT_REF_CLK_TARGET_EDGE_COUNT_FREQ_COUNT_REF_CLK_TARGET_EDGE_COUNT_MSB_POSN = (int)31
};
typedef enum freq_count_ref_clk_target_edge_count_posn_enum freq_count_ref_clk_target_edge_count_posn;

#define FREQ_COUNT_REF_CLK_TARGET_EDGE_COUNT_LSB_MASK (0x00000001u)
#define FREQ_COUNT_REF_CLK_TARGET_EDGE_COUNT_MSB_MASK (0x80000000u)

#endif /* IO_DEFS_MODULE_FREQ_COUNTER */
#endif /* __IO_DEFS_H__IO_DEFS_MODULE_FREQ_COUNTER */


#if !defined(__IO_DEFS_H__IO_DEFS_MODULE_K32_PREFETCH) 
#define __IO_DEFS_H__IO_DEFS_MODULE_K32_PREFETCH

/* -- k32_prefetch -- Kalimba 32-bit Prefetch Control registers. -- */

enum prefetch_config_posn_enum
{
   PREFETCH_CONFIG_COUNTERS_EN_POSN                   = (int)0,
   PREFETCH_CONFIG_PREFETCH_CONFIG_COUNTERS_EN_LSB_POSN = (int)0,
   PREFETCH_CONFIG_PREFETCH_CONFIG_COUNTERS_EN_MSB_POSN = (int)0
};
typedef enum prefetch_config_posn_enum prefetch_config_posn;

#define PREFETCH_CONFIG_COUNTERS_EN_MASK         (0x00000001u)

enum prefetch_debug_posn_enum
{
   PREFETCH_DEBUG_READ_EN_IN_POSN                     = (int)0,
   PREFETCH_DEBUG_PREFETCH_DEBUG_READ_EN_IN_LSB_POSN  = (int)0,
   PREFETCH_DEBUG_PREFETCH_DEBUG_READ_EN_IN_MSB_POSN  = (int)0,
   PREFETCH_DEBUG_WAIT_OUT_POSN                       = (int)1,
   PREFETCH_DEBUG_PREFETCH_DEBUG_WAIT_OUT_LSB_POSN    = (int)1,
   PREFETCH_DEBUG_PREFETCH_DEBUG_WAIT_OUT_MSB_POSN    = (int)1,
   PREFETCH_DEBUG_READ_EN_OUT_POSN                    = (int)2,
   PREFETCH_DEBUG_PREFETCH_DEBUG_READ_EN_OUT_LSB_POSN = (int)2,
   PREFETCH_DEBUG_PREFETCH_DEBUG_READ_EN_OUT_MSB_POSN = (int)2,
   PREFETCH_DEBUG_WAIT_IN_POSN                        = (int)3,
   PREFETCH_DEBUG_PREFETCH_DEBUG_WAIT_IN_LSB_POSN     = (int)3,
   PREFETCH_DEBUG_PREFETCH_DEBUG_WAIT_IN_MSB_POSN     = (int)3,
   PREFETCH_DEBUG_MEM_REQUEST_POSN                    = (int)4,
   PREFETCH_DEBUG_PREFETCH_DEBUG_MEM_REQUEST_LSB_POSN = (int)4,
   PREFETCH_DEBUG_PREFETCH_DEBUG_MEM_REQUEST_MSB_POSN = (int)4,
   PREFETCH_DEBUG_NEXT_MEMREQUEST_POSN                = (int)5,
   PREFETCH_DEBUG_PREFETCH_DEBUG_NEXT_MEMREQUEST_LSB_POSN = (int)5,
   PREFETCH_DEBUG_PREFETCH_DEBUG_NEXT_MEMREQUEST_MSB_POSN = (int)5,
   PREFETCH_DEBUG_PMEM_REQUEST_POSN                   = (int)6,
   PREFETCH_DEBUG_PREFETCH_DEBUG_PMEM_REQUEST_LSB_POSN = (int)6,
   PREFETCH_DEBUG_PREFETCH_DEBUG_PMEM_REQUEST_MSB_POSN = (int)6,
   PREFETCH_DEBUG_PM_WAIT_IN_PREV_POSN                = (int)7,
   PREFETCH_DEBUG_PREFETCH_DEBUG_PM_WAIT_IN_PREV_LSB_POSN = (int)7,
   PREFETCH_DEBUG_PREFETCH_DEBUG_PM_WAIT_IN_PREV_MSB_POSN = (int)7,
   PREFETCH_DEBUG_MEM_REQUEST_REG_POSN                = (int)8,
   PREFETCH_DEBUG_PREFETCH_DEBUG_MEM_REQUEST_REG_LSB_POSN = (int)8,
   PREFETCH_DEBUG_PREFETCH_DEBUG_MEM_REQUEST_REG_MSB_POSN = (int)8,
   PREFETCH_DEBUG_ALOW_PREFETCHING_LSB_POSN           = (int)9,
   PREFETCH_DEBUG_PREFETCH_DEBUG_ALOW_PREFETCHING_LSB_POSN = (int)9,
   PREFETCH_DEBUG_ALOW_PREFETCHING_MSB_POSN           = (int)10,
   PREFETCH_DEBUG_PREFETCH_DEBUG_ALOW_PREFETCHING_MSB_POSN = (int)10,
   PREFETCH_DEBUG_MISS_SEQ_T1_POSN                    = (int)11,
   PREFETCH_DEBUG_PREFETCH_DEBUG_MISS_SEQ_T1_LSB_POSN = (int)11,
   PREFETCH_DEBUG_PREFETCH_DEBUG_MISS_SEQ_T1_MSB_POSN = (int)11,
   PREFETCH_DEBUG_MISS_SEQ_T2_POSN                    = (int)12,
   PREFETCH_DEBUG_PREFETCH_DEBUG_MISS_SEQ_T2_LSB_POSN = (int)12,
   PREFETCH_DEBUG_PREFETCH_DEBUG_MISS_SEQ_T2_MSB_POSN = (int)12,
   PREFETCH_DEBUG_VALID_PREFETCH_DATA_POSN            = (int)13,
   PREFETCH_DEBUG_PREFETCH_DEBUG_VALID_PREFETCH_DATA_LSB_POSN = (int)13,
   PREFETCH_DEBUG_PREFETCH_DEBUG_VALID_PREFETCH_DATA_MSB_POSN = (int)13,
   PREFETCH_DEBUG_PREFETCH_VALID_POSN                 = (int)14,
   PREFETCH_DEBUG_PREFETCH_DEBUG_PREFETCH_VALID_LSB_POSN = (int)14,
   PREFETCH_DEBUG_PREFETCH_DEBUG_PREFETCH_VALID_MSB_POSN = (int)14,
   PREFETCH_DEBUG_PREFETCH_HIT_LSB_POSN               = (int)15,
   PREFETCH_DEBUG_PREFETCH_DEBUG_PREFETCH_HIT_LSB_POSN = (int)15,
   PREFETCH_DEBUG_PREFETCH_HIT_MSB_POSN               = (int)19,
   PREFETCH_DEBUG_PREFETCH_DEBUG_PREFETCH_HIT_MSB_POSN = (int)19,
   PREFETCH_DEBUG_PREFETCH_VALIDS_LSB_POSN            = (int)20,
   PREFETCH_DEBUG_PREFETCH_DEBUG_PREFETCH_VALIDS_LSB_POSN = (int)20,
   PREFETCH_DEBUG_PREFETCH_VALIDS_MSB_POSN            = (int)24,
   PREFETCH_DEBUG_PREFETCH_DEBUG_PREFETCH_VALIDS_MSB_POSN = (int)24
};
typedef enum prefetch_debug_posn_enum prefetch_debug_posn;

#define PREFETCH_DEBUG_READ_EN_IN_MASK           (0x00000001u)
#define PREFETCH_DEBUG_WAIT_OUT_MASK             (0x00000002u)
#define PREFETCH_DEBUG_READ_EN_OUT_MASK          (0x00000004u)
#define PREFETCH_DEBUG_WAIT_IN_MASK              (0x00000008u)
#define PREFETCH_DEBUG_MEM_REQUEST_MASK          (0x00000010u)
#define PREFETCH_DEBUG_NEXT_MEMREQUEST_MASK      (0x00000020u)
#define PREFETCH_DEBUG_PMEM_REQUEST_MASK         (0x00000040u)
#define PREFETCH_DEBUG_PM_WAIT_IN_PREV_MASK      (0x00000080u)
#define PREFETCH_DEBUG_MEM_REQUEST_REG_MASK      (0x00000100u)
#define PREFETCH_DEBUG_ALOW_PREFETCHING_LSB_MASK (0x00000200u)
#define PREFETCH_DEBUG_ALOW_PREFETCHING_MSB_MASK (0x00000400u)
#define PREFETCH_DEBUG_MISS_SEQ_T1_MASK          (0x00000800u)
#define PREFETCH_DEBUG_MISS_SEQ_T2_MASK          (0x00001000u)
#define PREFETCH_DEBUG_VALID_PREFETCH_DATA_MASK  (0x00002000u)
#define PREFETCH_DEBUG_PREFETCH_VALID_MASK       (0x00004000u)
#define PREFETCH_DEBUG_PREFETCH_HIT_LSB_MASK     (0x00008000u)
#define PREFETCH_DEBUG_PREFETCH_HIT_MSB_MASK     (0x00080000u)
#define PREFETCH_DEBUG_PREFETCH_VALIDS_LSB_MASK  (0x00100000u)
#define PREFETCH_DEBUG_PREFETCH_VALIDS_MSB_MASK  (0x01000000u)

enum prefetch_debug_addr_posn_enum
{
   PREFETCH_DEBUG_PMADDRIN_LSB_POSN                   = (int)0,
   PREFETCH_DEBUG_ADDR_PREFETCH_DEBUG_PMADDRIN_LSB_POSN = (int)0,
   PREFETCH_DEBUG_PMADDRIN_MSB_POSN                   = (int)15,
   PREFETCH_DEBUG_ADDR_PREFETCH_DEBUG_PMADDRIN_MSB_POSN = (int)15,
   PREFETCH_DEBUG_PMADDROUT_LSB_POSN                  = (int)16,
   PREFETCH_DEBUG_ADDR_PREFETCH_DEBUG_PMADDROUT_LSB_POSN = (int)16,
   PREFETCH_DEBUG_PMADDROUT_MSB_POSN                  = (int)31,
   PREFETCH_DEBUG_ADDR_PREFETCH_DEBUG_PMADDROUT_MSB_POSN = (int)31
};
typedef enum prefetch_debug_addr_posn_enum prefetch_debug_addr_posn;

#define PREFETCH_DEBUG_PMADDRIN_LSB_MASK         (0x00000001u)
#define PREFETCH_DEBUG_PMADDRIN_MSB_MASK         (0x00008000u)
#define PREFETCH_DEBUG_PMADDROUT_LSB_MASK        (0x00010000u)
#define PREFETCH_DEBUG_PMADDROUT_MSB_MASK        (0x80000000u)

enum prefetch_flush_posn_enum
{
   PREFETCH_FLUSH_POSN                                = (int)0,
   PREFETCH_FLUSH_PREFETCH_FLUSH_LSB_POSN             = (int)0,
   PREFETCH_FLUSH_PREFETCH_FLUSH_MSB_POSN             = (int)0
};
typedef enum prefetch_flush_posn_enum prefetch_flush_posn;

#define PREFETCH_FLUSH_MASK                      (0x00000001u)

enum prefetch_prefetch_count_posn_enum
{
   PREFETCH_PREFETCH_COUNT_LSB_POSN                   = (int)0,
   PREFETCH_PREFETCH_COUNT_PREFETCH_PREFETCH_COUNT_LSB_POSN = (int)0,
   PREFETCH_PREFETCH_COUNT_MSB_POSN                   = (int)31,
   PREFETCH_PREFETCH_COUNT_PREFETCH_PREFETCH_COUNT_MSB_POSN = (int)31
};
typedef enum prefetch_prefetch_count_posn_enum prefetch_prefetch_count_posn;

#define PREFETCH_PREFETCH_COUNT_LSB_MASK         (0x00000001u)
#define PREFETCH_PREFETCH_COUNT_MSB_MASK         (0x80000000u)

enum prefetch_request_count_posn_enum
{
   PREFETCH_REQUEST_COUNT_LSB_POSN                    = (int)0,
   PREFETCH_REQUEST_COUNT_PREFETCH_REQUEST_COUNT_LSB_POSN = (int)0,
   PREFETCH_REQUEST_COUNT_MSB_POSN                    = (int)31,
   PREFETCH_REQUEST_COUNT_PREFETCH_REQUEST_COUNT_MSB_POSN = (int)31
};
typedef enum prefetch_request_count_posn_enum prefetch_request_count_posn;

#define PREFETCH_REQUEST_COUNT_LSB_MASK          (0x00000001u)
#define PREFETCH_REQUEST_COUNT_MSB_MASK          (0x80000000u)

enum prefetch_wait_out_count_posn_enum
{
   PREFETCH_WAIT_OUT_COUNT_LSB_POSN                   = (int)0,
   PREFETCH_WAIT_OUT_COUNT_PREFETCH_WAIT_OUT_COUNT_LSB_POSN = (int)0,
   PREFETCH_WAIT_OUT_COUNT_MSB_POSN                   = (int)31,
   PREFETCH_WAIT_OUT_COUNT_PREFETCH_WAIT_OUT_COUNT_MSB_POSN = (int)31
};
typedef enum prefetch_wait_out_count_posn_enum prefetch_wait_out_count_posn;

#define PREFETCH_WAIT_OUT_COUNT_LSB_MASK         (0x00000001u)
#define PREFETCH_WAIT_OUT_COUNT_MSB_MASK         (0x80000000u)

#endif /* IO_DEFS_MODULE_K32_PREFETCH */








#if defined(IO_DEFS_MODULE_ANC) 

#ifndef __IO_DEFS_H__IO_DEFS_MODULE_ANC
#define __IO_DEFS_H__IO_DEFS_MODULE_ANC

/* -- anc -- Control registers for ANC block -- */

enum anc_control0_posn_enum
{
   CDC_DEBUG_CLK_EN_POSN                              = (int)0,
   ANC_CONTROL0_CDC_DEBUG_CLK_EN_LSB_POSN             = (int)0,
   ANC_CONTROL0_CDC_DEBUG_CLK_EN_MSB_POSN             = (int)0,
   CDC_DEBUG_FS_SEL_LSB_POSN                          = (int)1,
   ANC_CONTROL0_CDC_DEBUG_FS_SEL_LSB_POSN             = (int)1,
   CDC_DEBUG_FS_SEL_MSB_POSN                          = (int)4,
   ANC_CONTROL0_CDC_DEBUG_FS_SEL_MSB_POSN             = (int)4,
   CDC_DEBUG_ANC_EN_POSN                              = (int)5,
   ANC_CONTROL0_CDC_DEBUG_ANC_EN_LSB_POSN             = (int)5,
   ANC_CONTROL0_CDC_DEBUG_ANC_EN_MSB_POSN             = (int)5,
   CDC_DEBUG_ANC_SEL_LSB_POSN                         = (int)6,
   ANC_CONTROL0_CDC_DEBUG_ANC_SEL_LSB_POSN            = (int)6,
   CDC_DEBUG_ANC_SEL_MSB_POSN                         = (int)9,
   ANC_CONTROL0_CDC_DEBUG_ANC_SEL_MSB_POSN            = (int)9
};
typedef enum anc_control0_posn_enum anc_control0_posn;

#define CDC_DEBUG_CLK_EN_MASK                    (0x00000001u)
#define CDC_DEBUG_FS_SEL_LSB_MASK                (0x00000002u)
#define CDC_DEBUG_FS_SEL_MSB_MASK                (0x00000010u)
#define CDC_DEBUG_ANC_EN_MASK                    (0x00000020u)
#define CDC_DEBUG_ANC_SEL_LSB_MASK               (0x00000040u)
#define CDC_DEBUG_ANC_SEL_MSB_MASK               (0x00000200u)

enum anc_control1_posn_enum
{
   CDC_CLK_RST_CTRL_MCLK_CONTROL_LSB_POSN             = (int)0,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_MCLK_CONTROL_LSB_POSN = (int)0,
   CDC_CLK_RST_CTRL_MCLK_CONTROL_MSB_POSN             = (int)7,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_MCLK_CONTROL_MSB_POSN = (int)7,
   CDC_CLK_RST_CTRL_MCLK_EN_POSN                      = (int)0,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_MCLK_EN_LSB_POSN     = (int)0,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_MCLK_EN_MSB_POSN     = (int)0,
   CDC_CLK_RST_CTRL_MCLK_SEL_POSN                     = (int)2,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_MCLK_SEL_LSB_POSN    = (int)2,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_MCLK_SEL_MSB_POSN    = (int)2,
   CDC_CLK_RST_CTRL_RESERVED_BITS7_3_LSB_POSN         = (int)3,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_RESERVED_BITS7_3_LSB_POSN = (int)3,
   CDC_CLK_RST_CTRL_RESERVED_BITS7_3_MSB_POSN         = (int)7,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_RESERVED_BITS7_3_MSB_POSN = (int)7,
   CDC_CLK_RST_CTRL_FS_CNT_CONTROL_LSB_POSN           = (int)8,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_FS_CNT_CONTROL_LSB_POSN = (int)8,
   CDC_CLK_RST_CTRL_FS_CNT_CONTROL_MSB_POSN           = (int)15,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_FS_CNT_CONTROL_MSB_POSN = (int)15,
   CDC_CLK_RST_CTRL_FS_CNT_EN_POSN                    = (int)8,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_FS_CNT_EN_LSB_POSN   = (int)8,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_FS_CNT_EN_MSB_POSN   = (int)8,
   CDC_CLK_RST_CTRL_FS_CLR_EN_POSN                    = (int)9,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_FS_CLR_EN_LSB_POSN   = (int)9,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_FS_CLR_EN_MSB_POSN   = (int)9,
   CDC_CLK_RST_CTRL_FS_X2_SEL_POSN                    = (int)12,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_FS_X2_SEL_LSB_POSN   = (int)12,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_FS_X2_SEL_MSB_POSN   = (int)12,
   CDC_CLK_RST_CTRL_FS_CNT_RESERVED_BITS7_5_LSB_POSN  = (int)13,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_FS_CNT_RESERVED_BITS7_5_LSB_POSN = (int)13,
   CDC_CLK_RST_CTRL_FS_CNT_RESERVED_BITS7_5_MSB_POSN  = (int)15,
   ANC_CONTROL1_CDC_CLK_RST_CTRL_FS_CNT_RESERVED_BITS7_5_MSB_POSN = (int)15
};
typedef enum anc_control1_posn_enum anc_control1_posn;

#define CDC_CLK_RST_CTRL_MCLK_CONTROL_LSB_MASK   (0x00000001u)
#define CDC_CLK_RST_CTRL_MCLK_CONTROL_MSB_MASK   (0x00000080u)
#define CDC_CLK_RST_CTRL_MCLK_EN_MASK            (0x00000001u)
#define CDC_CLK_RST_CTRL_MCLK_SEL_MASK           (0x00000004u)
#define CDC_CLK_RST_CTRL_RESERVED_BITS7_3_LSB_MASK (0x00000008u)
#define CDC_CLK_RST_CTRL_RESERVED_BITS7_3_MSB_MASK (0x00000080u)
#define CDC_CLK_RST_CTRL_FS_CNT_CONTROL_LSB_MASK (0x00000100u)
#define CDC_CLK_RST_CTRL_FS_CNT_CONTROL_MSB_MASK (0x00008000u)
#define CDC_CLK_RST_CTRL_FS_CNT_EN_MASK          (0x00000100u)
#define CDC_CLK_RST_CTRL_FS_CLR_EN_MASK          (0x00000200u)
#define CDC_CLK_RST_CTRL_FS_X2_SEL_MASK          (0x00001000u)
#define CDC_CLK_RST_CTRL_FS_CNT_RESERVED_BITS7_5_LSB_MASK (0x00002000u)
#define CDC_CLK_RST_CTRL_FS_CNT_RESERVED_BITS7_5_MSB_MASK (0x00008000u)

enum anc_control10_posn_enum
{
   CDC_ANC1_VOLUME_EN_POSN                            = (int)0,
   ANC_CONTROL10_CDC_ANC1_VOLUME_EN_LSB_POSN          = (int)0,
   ANC_CONTROL10_CDC_ANC1_VOLUME_EN_MSB_POSN          = (int)0,
   CDC_ANC1_VOLUME_SHIFT_MAIN_LSB_POSN                = (int)1,
   ANC_CONTROL10_CDC_ANC1_VOLUME_SHIFT_MAIN_LSB_POSN  = (int)1,
   CDC_ANC1_VOLUME_SHIFT_MAIN_MSB_POSN                = (int)2,
   ANC_CONTROL10_CDC_ANC1_VOLUME_SHIFT_MAIN_MSB_POSN  = (int)2,
   CDC_ANC1_VOLUME_SHIFT_1_LSB_POSN                   = (int)3,
   ANC_CONTROL10_CDC_ANC1_VOLUME_SHIFT_1_LSB_POSN     = (int)3,
   CDC_ANC1_VOLUME_SHIFT_1_MSB_POSN                   = (int)4,
   ANC_CONTROL10_CDC_ANC1_VOLUME_SHIFT_1_MSB_POSN     = (int)4,
   CDC_ANC1_VOLUME_SHIFT_2_LSB_POSN                   = (int)5,
   ANC_CONTROL10_CDC_ANC1_VOLUME_SHIFT_2_LSB_POSN     = (int)5,
   CDC_ANC1_VOLUME_SHIFT_2_MSB_POSN                   = (int)6,
   ANC_CONTROL10_CDC_ANC1_VOLUME_SHIFT_2_MSB_POSN     = (int)6
};
typedef enum anc_control10_posn_enum anc_control10_posn;

#define CDC_ANC1_VOLUME_EN_MASK                  (0x00000001u)
#define CDC_ANC1_VOLUME_SHIFT_MAIN_LSB_MASK      (0x00000002u)
#define CDC_ANC1_VOLUME_SHIFT_MAIN_MSB_MASK      (0x00000004u)
#define CDC_ANC1_VOLUME_SHIFT_1_LSB_MASK         (0x00000008u)
#define CDC_ANC1_VOLUME_SHIFT_1_MSB_MASK         (0x00000010u)
#define CDC_ANC1_VOLUME_SHIFT_2_LSB_MASK         (0x00000020u)
#define CDC_ANC1_VOLUME_SHIFT_2_MSB_MASK         (0x00000040u)

enum anc_control11_posn_enum
{
   CDC_ANC0_MAX_OUTPUT_AMPL_LSB_POSN                  = (int)0,
   ANC_CONTROL11_CDC_ANC0_MAX_OUTPUT_AMPL_LSB_POSN    = (int)0,
   CDC_ANC0_MAX_OUTPUT_AMPL_MSB_POSN                  = (int)15,
   ANC_CONTROL11_CDC_ANC0_MAX_OUTPUT_AMPL_MSB_POSN    = (int)15
};
typedef enum anc_control11_posn_enum anc_control11_posn;

#define CDC_ANC0_MAX_OUTPUT_AMPL_LSB_MASK        (0x00000001u)
#define CDC_ANC0_MAX_OUTPUT_AMPL_MSB_MASK        (0x00008000u)

enum anc_control12_posn_enum
{
   CDC_ANC1_MAX_OUTPUT_AMPL_LSB_POSN                  = (int)0,
   ANC_CONTROL12_CDC_ANC1_MAX_OUTPUT_AMPL_LSB_POSN    = (int)0,
   CDC_ANC1_MAX_OUTPUT_AMPL_MSB_POSN                  = (int)15,
   ANC_CONTROL12_CDC_ANC1_MAX_OUTPUT_AMPL_MSB_POSN    = (int)15
};
typedef enum anc_control12_posn_enum anc_control12_posn;

#define CDC_ANC1_MAX_OUTPUT_AMPL_LSB_MASK        (0x00000001u)
#define CDC_ANC1_MAX_OUTPUT_AMPL_MSB_MASK        (0x00008000u)

enum anc_control13_posn_enum
{
   CDC_TX_INP_MUX_ADC_MUX10_CFG0_LSB_POSN             = (int)0,
   ANC_CONTROL13_CDC_TX_INP_MUX_ADC_MUX10_CFG0_LSB_POSN = (int)0,
   CDC_TX_INP_MUX_ADC_MUX10_CFG0_MSB_POSN             = (int)6,
   ANC_CONTROL13_CDC_TX_INP_MUX_ADC_MUX10_CFG0_MSB_POSN = (int)6,
   CDC_TX_INP_MUX_AMIC_MUX_10_SEL_LSB_POSN            = (int)0,
   ANC_CONTROL13_CDC_TX_INP_MUX_AMIC_MUX_10_SEL_LSB_POSN = (int)0,
   CDC_TX_INP_MUX_AMIC_MUX_10_SEL_MSB_POSN            = (int)1,
   ANC_CONTROL13_CDC_TX_INP_MUX_AMIC_MUX_10_SEL_MSB_POSN = (int)1,
   CDC_TX_INP_MUX_DMIC_MUX_10_SEL_LSB_POSN            = (int)3,
   ANC_CONTROL13_CDC_TX_INP_MUX_DMIC_MUX_10_SEL_LSB_POSN = (int)3,
   CDC_TX_INP_MUX_DMIC_MUX_10_SEL_MSB_POSN            = (int)5,
   ANC_CONTROL13_CDC_TX_INP_MUX_DMIC_MUX_10_SEL_MSB_POSN = (int)5,
   CDC_TX_INP_MUX_ADC_MUX_10_SEL_LSB_POSN             = (int)6,
   ANC_CONTROL13_CDC_TX_INP_MUX_ADC_MUX_10_SEL_LSB_POSN = (int)6,
   CDC_TX_INP_MUX_ADC_MUX_10_SEL_MSB_POSN             = (int)7,
   ANC_CONTROL13_CDC_TX_INP_MUX_ADC_MUX_10_SEL_MSB_POSN = (int)7
};
typedef enum anc_control13_posn_enum anc_control13_posn;

#define CDC_TX_INP_MUX_ADC_MUX10_CFG0_LSB_MASK   (0x00000001u)
#define CDC_TX_INP_MUX_ADC_MUX10_CFG0_MSB_MASK   (0x00000040u)
#define CDC_TX_INP_MUX_AMIC_MUX_10_SEL_LSB_MASK  (0x00000001u)
#define CDC_TX_INP_MUX_AMIC_MUX_10_SEL_MSB_MASK  (0x00000002u)
#define CDC_TX_INP_MUX_DMIC_MUX_10_SEL_LSB_MASK  (0x00000008u)
#define CDC_TX_INP_MUX_DMIC_MUX_10_SEL_MSB_MASK  (0x00000020u)
#define CDC_TX_INP_MUX_ADC_MUX_10_SEL_LSB_MASK   (0x00000040u)
#define CDC_TX_INP_MUX_ADC_MUX_10_SEL_MSB_MASK   (0x00000080u)

enum anc_control14_posn_enum
{
   CDC_TX_INP_MUX_ADC_MUX11_CFG0_LSB_POSN             = (int)0,
   ANC_CONTROL14_CDC_TX_INP_MUX_ADC_MUX11_CFG0_LSB_POSN = (int)0,
   CDC_TX_INP_MUX_ADC_MUX11_CFG0_MSB_POSN             = (int)6,
   ANC_CONTROL14_CDC_TX_INP_MUX_ADC_MUX11_CFG0_MSB_POSN = (int)6,
   CDC_TX_INP_MUX_AMIC_MUX_11_SEL_LSB_POSN            = (int)0,
   ANC_CONTROL14_CDC_TX_INP_MUX_AMIC_MUX_11_SEL_LSB_POSN = (int)0,
   CDC_TX_INP_MUX_AMIC_MUX_11_SEL_MSB_POSN            = (int)1,
   ANC_CONTROL14_CDC_TX_INP_MUX_AMIC_MUX_11_SEL_MSB_POSN = (int)1,
   CDC_TX_INP_MUX_DMIC_MUX_11_SEL_LSB_POSN            = (int)3,
   ANC_CONTROL14_CDC_TX_INP_MUX_DMIC_MUX_11_SEL_LSB_POSN = (int)3,
   CDC_TX_INP_MUX_DMIC_MUX_11_SEL_MSB_POSN            = (int)5,
   ANC_CONTROL14_CDC_TX_INP_MUX_DMIC_MUX_11_SEL_MSB_POSN = (int)5,
   CDC_TX_INP_MUX_ADC_MUX_11_SEL_LSB_POSN             = (int)6,
   ANC_CONTROL14_CDC_TX_INP_MUX_ADC_MUX_11_SEL_LSB_POSN = (int)6,
   CDC_TX_INP_MUX_ADC_MUX_11_SEL_MSB_POSN             = (int)7,
   ANC_CONTROL14_CDC_TX_INP_MUX_ADC_MUX_11_SEL_MSB_POSN = (int)7
};
typedef enum anc_control14_posn_enum anc_control14_posn;

#define CDC_TX_INP_MUX_ADC_MUX11_CFG0_LSB_MASK   (0x00000001u)
#define CDC_TX_INP_MUX_ADC_MUX11_CFG0_MSB_MASK   (0x00000040u)
#define CDC_TX_INP_MUX_AMIC_MUX_11_SEL_LSB_MASK  (0x00000001u)
#define CDC_TX_INP_MUX_AMIC_MUX_11_SEL_MSB_MASK  (0x00000002u)
#define CDC_TX_INP_MUX_DMIC_MUX_11_SEL_LSB_MASK  (0x00000008u)
#define CDC_TX_INP_MUX_DMIC_MUX_11_SEL_MSB_MASK  (0x00000020u)
#define CDC_TX_INP_MUX_ADC_MUX_11_SEL_LSB_MASK   (0x00000040u)
#define CDC_TX_INP_MUX_ADC_MUX_11_SEL_MSB_MASK   (0x00000080u)

enum anc_control15_posn_enum
{
   CDC_TX_INP_MUX_ADC_MUX12_CFG0_LSB_POSN             = (int)0,
   ANC_CONTROL15_CDC_TX_INP_MUX_ADC_MUX12_CFG0_LSB_POSN = (int)0,
   CDC_TX_INP_MUX_ADC_MUX12_CFG0_MSB_POSN             = (int)6,
   ANC_CONTROL15_CDC_TX_INP_MUX_ADC_MUX12_CFG0_MSB_POSN = (int)6,
   CDC_TX_INP_MUX_AMIC_MUX_12_SEL_LSB_POSN            = (int)0,
   ANC_CONTROL15_CDC_TX_INP_MUX_AMIC_MUX_12_SEL_LSB_POSN = (int)0,
   CDC_TX_INP_MUX_AMIC_MUX_12_SEL_MSB_POSN            = (int)1,
   ANC_CONTROL15_CDC_TX_INP_MUX_AMIC_MUX_12_SEL_MSB_POSN = (int)1,
   CDC_TX_INP_MUX_DMIC_MUX_12_SEL_LSB_POSN            = (int)3,
   ANC_CONTROL15_CDC_TX_INP_MUX_DMIC_MUX_12_SEL_LSB_POSN = (int)3,
   CDC_TX_INP_MUX_DMIC_MUX_12_SEL_MSB_POSN            = (int)5,
   ANC_CONTROL15_CDC_TX_INP_MUX_DMIC_MUX_12_SEL_MSB_POSN = (int)5,
   CDC_TX_INP_MUX_ADC_MUX_12_SEL_LSB_POSN             = (int)6,
   ANC_CONTROL15_CDC_TX_INP_MUX_ADC_MUX_12_SEL_LSB_POSN = (int)6,
   CDC_TX_INP_MUX_ADC_MUX_12_SEL_MSB_POSN             = (int)7,
   ANC_CONTROL15_CDC_TX_INP_MUX_ADC_MUX_12_SEL_MSB_POSN = (int)7
};
typedef enum anc_control15_posn_enum anc_control15_posn;

#define CDC_TX_INP_MUX_ADC_MUX12_CFG0_LSB_MASK   (0x00000001u)
#define CDC_TX_INP_MUX_ADC_MUX12_CFG0_MSB_MASK   (0x00000040u)
#define CDC_TX_INP_MUX_AMIC_MUX_12_SEL_LSB_MASK  (0x00000001u)
#define CDC_TX_INP_MUX_AMIC_MUX_12_SEL_MSB_MASK  (0x00000002u)
#define CDC_TX_INP_MUX_DMIC_MUX_12_SEL_LSB_MASK  (0x00000008u)
#define CDC_TX_INP_MUX_DMIC_MUX_12_SEL_MSB_MASK  (0x00000020u)
#define CDC_TX_INP_MUX_ADC_MUX_12_SEL_LSB_MASK   (0x00000040u)
#define CDC_TX_INP_MUX_ADC_MUX_12_SEL_MSB_MASK   (0x00000080u)

enum anc_control16_posn_enum
{
   CDC_TX_INP_MUX_ADC_MUX13_CFG0_LSB_POSN             = (int)0,
   ANC_CONTROL16_CDC_TX_INP_MUX_ADC_MUX13_CFG0_LSB_POSN = (int)0,
   CDC_TX_INP_MUX_ADC_MUX13_CFG0_MSB_POSN             = (int)6,
   ANC_CONTROL16_CDC_TX_INP_MUX_ADC_MUX13_CFG0_MSB_POSN = (int)6,
   CDC_TX_INP_MUX_AMIC_MUX_13_SEL_LSB_POSN            = (int)0,
   ANC_CONTROL16_CDC_TX_INP_MUX_AMIC_MUX_13_SEL_LSB_POSN = (int)0,
   CDC_TX_INP_MUX_AMIC_MUX_13_SEL_MSB_POSN            = (int)1,
   ANC_CONTROL16_CDC_TX_INP_MUX_AMIC_MUX_13_SEL_MSB_POSN = (int)1,
   CDC_TX_INP_MUX_DMIC_MUX_13_SEL_LSB_POSN            = (int)3,
   ANC_CONTROL16_CDC_TX_INP_MUX_DMIC_MUX_13_SEL_LSB_POSN = (int)3,
   CDC_TX_INP_MUX_DMIC_MUX_13_SEL_MSB_POSN            = (int)5,
   ANC_CONTROL16_CDC_TX_INP_MUX_DMIC_MUX_13_SEL_MSB_POSN = (int)5,
   CDC_TX_INP_MUX_ADC_MUX_13_SEL_LSB_POSN             = (int)6,
   ANC_CONTROL16_CDC_TX_INP_MUX_ADC_MUX_13_SEL_LSB_POSN = (int)6,
   CDC_TX_INP_MUX_ADC_MUX_13_SEL_MSB_POSN             = (int)7,
   ANC_CONTROL16_CDC_TX_INP_MUX_ADC_MUX_13_SEL_MSB_POSN = (int)7
};
typedef enum anc_control16_posn_enum anc_control16_posn;

#define CDC_TX_INP_MUX_ADC_MUX13_CFG0_LSB_MASK   (0x00000001u)
#define CDC_TX_INP_MUX_ADC_MUX13_CFG0_MSB_MASK   (0x00000040u)
#define CDC_TX_INP_MUX_AMIC_MUX_13_SEL_LSB_MASK  (0x00000001u)
#define CDC_TX_INP_MUX_AMIC_MUX_13_SEL_MSB_MASK  (0x00000002u)
#define CDC_TX_INP_MUX_DMIC_MUX_13_SEL_LSB_MASK  (0x00000008u)
#define CDC_TX_INP_MUX_DMIC_MUX_13_SEL_MSB_MASK  (0x00000020u)
#define CDC_TX_INP_MUX_ADC_MUX_13_SEL_LSB_MASK   (0x00000040u)
#define CDC_TX_INP_MUX_ADC_MUX_13_SEL_MSB_MASK   (0x00000080u)

enum anc_control17_posn_enum
{
   CDC_ANC0_CLK_RESET_CTL_LSB_POSN                    = (int)0,
   ANC_CONTROL17_CDC_ANC0_CLK_RESET_CTL_LSB_POSN      = (int)0,
   CDC_ANC0_CLK_RESET_CTL_MSB_POSN                    = (int)7,
   ANC_CONTROL17_CDC_ANC0_CLK_RESET_CTL_MSB_POSN      = (int)7,
   CDC_ANC0_FFa_CLK_EN_POSN                           = (int)0,
   ANC_CONTROL17_CDC_ANC0_FFa_CLK_EN_LSB_POSN         = (int)0,
   ANC_CONTROL17_CDC_ANC0_FFa_CLK_EN_MSB_POSN         = (int)0,
   CDC_ANC0_FFb_CLK_EN_POSN                           = (int)1,
   ANC_CONTROL17_CDC_ANC0_FFb_CLK_EN_LSB_POSN         = (int)1,
   ANC_CONTROL17_CDC_ANC0_FFb_CLK_EN_MSB_POSN         = (int)1,
   CDC_ANC0_FB_CLK_EN_POSN                            = (int)2,
   ANC_CONTROL17_CDC_ANC0_FB_CLK_EN_LSB_POSN          = (int)2,
   ANC_CONTROL17_CDC_ANC0_FB_CLK_EN_MSB_POSN          = (int)2,
   CDC_ANC0_FFa_RESET_EN_POSN                         = (int)3,
   ANC_CONTROL17_CDC_ANC0_FFa_RESET_EN_LSB_POSN       = (int)3,
   ANC_CONTROL17_CDC_ANC0_FFa_RESET_EN_MSB_POSN       = (int)3,
   CDC_ANC0_FFb_RESET_EN_POSN                         = (int)4,
   ANC_CONTROL17_CDC_ANC0_FFb_RESET_EN_LSB_POSN       = (int)4,
   ANC_CONTROL17_CDC_ANC0_FFb_RESET_EN_MSB_POSN       = (int)4,
   CDC_ANC0_FB_RESET_EN_POSN                          = (int)5,
   ANC_CONTROL17_CDC_ANC0_FB_RESET_EN_LSB_POSN        = (int)5,
   ANC_CONTROL17_CDC_ANC0_FB_RESET_EN_MSB_POSN        = (int)5,
   CDC_ANC0_SMARTLNQ_A_EN_POSN                        = (int)6,
   ANC_CONTROL17_CDC_ANC0_SMARTLNQ_A_EN_LSB_POSN      = (int)6,
   ANC_CONTROL17_CDC_ANC0_SMARTLNQ_A_EN_MSB_POSN      = (int)6,
   CDC_ANC0_SMARTLNQ_B_EN_POSN                        = (int)7,
   ANC_CONTROL17_CDC_ANC0_SMARTLNQ_B_EN_LSB_POSN      = (int)7,
   ANC_CONTROL17_CDC_ANC0_SMARTLNQ_B_EN_MSB_POSN      = (int)7
};
typedef enum anc_control17_posn_enum anc_control17_posn;

#define CDC_ANC0_CLK_RESET_CTL_LSB_MASK          (0x00000001u)
#define CDC_ANC0_CLK_RESET_CTL_MSB_MASK          (0x00000080u)
#define CDC_ANC0_FFa_CLK_EN_MASK                 (0x00000001u)
#define CDC_ANC0_FFb_CLK_EN_MASK                 (0x00000002u)
#define CDC_ANC0_FB_CLK_EN_MASK                  (0x00000004u)
#define CDC_ANC0_FFa_RESET_EN_MASK               (0x00000008u)
#define CDC_ANC0_FFb_RESET_EN_MASK               (0x00000010u)
#define CDC_ANC0_FB_RESET_EN_MASK                (0x00000020u)
#define CDC_ANC0_SMARTLNQ_A_EN_MASK              (0x00000040u)
#define CDC_ANC0_SMARTLNQ_B_EN_MASK              (0x00000080u)

enum anc_control18_posn_enum
{
   CDC_ANC0_MODE_1_CTL_LSB_POSN                       = (int)0,
   ANC_CONTROL18_CDC_ANC0_MODE_1_CTL_LSB_POSN         = (int)0,
   CDC_ANC0_MODE_1_CTL_MSB_POSN                       = (int)7,
   ANC_CONTROL18_CDC_ANC0_MODE_1_CTL_MSB_POSN         = (int)7,
   CDC_ANC0_ANC_OUT_EN_POSN                           = (int)0,
   ANC_CONTROL18_CDC_ANC0_ANC_OUT_EN_LSB_POSN         = (int)0,
   ANC_CONTROL18_CDC_ANC0_ANC_OUT_EN_MSB_POSN         = (int)0,
   CDC_ANC0_ANC_ADC_DMIC_A_SEL_POSN                   = (int)1,
   ANC_CONTROL18_CDC_ANC0_ANC_ADC_DMIC_A_SEL_LSB_POSN = (int)1,
   ANC_CONTROL18_CDC_ANC0_ANC_ADC_DMIC_A_SEL_MSB_POSN = (int)1,
   CDC_ANC0_ANC_ADC_DMIC_B_SEL_POSN                   = (int)2,
   ANC_CONTROL18_CDC_ANC0_ANC_ADC_DMIC_B_SEL_LSB_POSN = (int)2,
   ANC_CONTROL18_CDC_ANC0_ANC_ADC_DMIC_B_SEL_MSB_POSN = (int)2,
   CDC_ANC0_ANC_OUTMIX_EN_POSN                        = (int)3,
   ANC_CONTROL18_CDC_ANC0_ANC_OUTMIX_EN_LSB_POSN      = (int)3,
   ANC_CONTROL18_CDC_ANC0_ANC_OUTMIX_EN_MSB_POSN      = (int)3,
   CDC_ANC0_FFb_IN_EN_POSN                            = (int)4,
   ANC_CONTROL18_CDC_ANC0_FFb_IN_EN_LSB_POSN          = (int)4,
   ANC_CONTROL18_CDC_ANC0_FFb_IN_EN_MSB_POSN          = (int)4,
   CDC_ANC0_FFa_IN_EN_POSN                            = (int)5,
   ANC_CONTROL18_CDC_ANC0_FFa_IN_EN_LSB_POSN          = (int)5,
   ANC_CONTROL18_CDC_ANC0_FFa_IN_EN_MSB_POSN          = (int)5,
   CDC_ANC0_ANC_SMLPF_EN_POSN                         = (int)6,
   ANC_CONTROL18_CDC_ANC0_ANC_SMLPF_EN_LSB_POSN       = (int)6,
   ANC_CONTROL18_CDC_ANC0_ANC_SMLPF_EN_MSB_POSN       = (int)6,
   CDC_ANC0_FB_ON_FBMON_IS_TRUE_POSN                  = (int)7,
   ANC_CONTROL18_CDC_ANC0_FB_ON_FBMON_IS_TRUE_LSB_POSN = (int)7,
   ANC_CONTROL18_CDC_ANC0_FB_ON_FBMON_IS_TRUE_MSB_POSN = (int)7,
   CDC_ANC0_MODE_2_CTL_LSB_POSN                       = (int)8,
   ANC_CONTROL18_CDC_ANC0_MODE_2_CTL_LSB_POSN         = (int)8,
   CDC_ANC0_MODE_2_CTL_MSB_POSN                       = (int)15,
   ANC_CONTROL18_CDC_ANC0_MODE_2_CTL_MSB_POSN         = (int)15,
   CDC_ANC0_ANC_FFLE_EN_POSN                          = (int)8,
   ANC_CONTROL18_CDC_ANC0_ANC_FFLE_EN_LSB_POSN        = (int)8,
   ANC_CONTROL18_CDC_ANC0_ANC_FFLE_EN_MSB_POSN        = (int)8,
   CDC_ANC0_ANC_FFGAIN_B_EN_POSN                      = (int)9,
   ANC_CONTROL18_CDC_ANC0_ANC_FFGAIN_B_EN_LSB_POSN    = (int)9,
   ANC_CONTROL18_CDC_ANC0_ANC_FFGAIN_B_EN_MSB_POSN    = (int)9,
   CDC_ANC0_ANC_FFGAIN_A_EN_POSN                      = (int)10,
   ANC_CONTROL18_CDC_ANC0_ANC_FFGAIN_A_EN_LSB_POSN    = (int)10,
   ANC_CONTROL18_CDC_ANC0_ANC_FFGAIN_A_EN_MSB_POSN    = (int)10,
   CDC_ANC0_ANC_DCFILT_B_EN_POSN                      = (int)11,
   ANC_CONTROL18_CDC_ANC0_ANC_DCFILT_B_EN_LSB_POSN    = (int)11,
   ANC_CONTROL18_CDC_ANC0_ANC_DCFILT_B_EN_MSB_POSN    = (int)11,
   CDC_ANC0_ANC_DCFILT_A_EN_POSN                      = (int)12,
   ANC_CONTROL18_CDC_ANC0_ANC_DCFILT_A_EN_LSB_POSN    = (int)12,
   ANC_CONTROL18_CDC_ANC0_ANC_DCFILT_A_EN_MSB_POSN    = (int)12,
   CDC_ANC0_ANC_DMIC_X2_B_SEL_POSN                    = (int)13,
   ANC_CONTROL18_CDC_ANC0_ANC_DMIC_X2_B_SEL_LSB_POSN  = (int)13,
   ANC_CONTROL18_CDC_ANC0_ANC_DMIC_X2_B_SEL_MSB_POSN  = (int)13,
   CDC_ANC0_ANC_DMIC_X2_A_SEL_POSN                    = (int)14,
   ANC_CONTROL18_CDC_ANC0_ANC_DMIC_X2_A_SEL_LSB_POSN  = (int)14,
   ANC_CONTROL18_CDC_ANC0_ANC_DMIC_X2_A_SEL_MSB_POSN  = (int)14,
   CDC_ANC0_ANC_FBGAIN_EN_POSN                        = (int)15,
   ANC_CONTROL18_CDC_ANC0_ANC_FBGAIN_EN_LSB_POSN      = (int)15,
   ANC_CONTROL18_CDC_ANC0_ANC_FBGAIN_EN_MSB_POSN      = (int)15
};
typedef enum anc_control18_posn_enum anc_control18_posn;

#define CDC_ANC0_MODE_1_CTL_LSB_MASK             (0x00000001u)
#define CDC_ANC0_MODE_1_CTL_MSB_MASK             (0x00000080u)
#define CDC_ANC0_ANC_OUT_EN_MASK                 (0x00000001u)
#define CDC_ANC0_ANC_ADC_DMIC_A_SEL_MASK         (0x00000002u)
#define CDC_ANC0_ANC_ADC_DMIC_B_SEL_MASK         (0x00000004u)
#define CDC_ANC0_ANC_OUTMIX_EN_MASK              (0x00000008u)
#define CDC_ANC0_FFb_IN_EN_MASK                  (0x00000010u)
#define CDC_ANC0_FFa_IN_EN_MASK                  (0x00000020u)
#define CDC_ANC0_ANC_SMLPF_EN_MASK               (0x00000040u)
#define CDC_ANC0_FB_ON_FBMON_IS_TRUE_MASK        (0x00000080u)
#define CDC_ANC0_MODE_2_CTL_LSB_MASK             (0x00000100u)
#define CDC_ANC0_MODE_2_CTL_MSB_MASK             (0x00008000u)
#define CDC_ANC0_ANC_FFLE_EN_MASK                (0x00000100u)
#define CDC_ANC0_ANC_FFGAIN_B_EN_MASK            (0x00000200u)
#define CDC_ANC0_ANC_FFGAIN_A_EN_MASK            (0x00000400u)
#define CDC_ANC0_ANC_DCFILT_B_EN_MASK            (0x00000800u)
#define CDC_ANC0_ANC_DCFILT_A_EN_MASK            (0x00001000u)
#define CDC_ANC0_ANC_DMIC_X2_B_SEL_MASK          (0x00002000u)
#define CDC_ANC0_ANC_DMIC_X2_A_SEL_MASK          (0x00004000u)
#define CDC_ANC0_ANC_FBGAIN_EN_MASK              (0x00008000u)

enum anc_control19_posn_enum
{
   CDC_ANC0_FF_SHIFT_LSB_POSN                         = (int)0,
   ANC_CONTROL19_CDC_ANC0_FF_SHIFT_LSB_POSN           = (int)0,
   CDC_ANC0_FF_SHIFT_MSB_POSN                         = (int)7,
   ANC_CONTROL19_CDC_ANC0_FF_SHIFT_MSB_POSN           = (int)7,
   CDC_ANC0_FFb_SHIFT_LSB_POSN                        = (int)0,
   ANC_CONTROL19_CDC_ANC0_FFb_SHIFT_LSB_POSN          = (int)0,
   CDC_ANC0_FFb_SHIFT_MSB_POSN                        = (int)3,
   ANC_CONTROL19_CDC_ANC0_FFb_SHIFT_MSB_POSN          = (int)3,
   CDC_ANC0_FFa_SHIFT_LSB_POSN                        = (int)4,
   ANC_CONTROL19_CDC_ANC0_FFa_SHIFT_LSB_POSN          = (int)4,
   CDC_ANC0_FFa_SHIFT_MSB_POSN                        = (int)7,
   ANC_CONTROL19_CDC_ANC0_FFa_SHIFT_MSB_POSN          = (int)7,
   CDC_ANC0_FB_SHIFT_LSB_POSN                         = (int)8,
   ANC_CONTROL19_CDC_ANC0_FB_SHIFT_LSB_POSN           = (int)8,
   CDC_ANC0_FB_SHIFT_MSB_POSN                         = (int)15,
   ANC_CONTROL19_CDC_ANC0_FB_SHIFT_MSB_POSN           = (int)15,
   CDC_ANC0_FB_LPF_SHIFT_LSB_POSN                     = (int)8,
   ANC_CONTROL19_CDC_ANC0_FB_LPF_SHIFT_LSB_POSN       = (int)8,
   CDC_ANC0_FB_LPF_SHIFT_MSB_POSN                     = (int)11,
   ANC_CONTROL19_CDC_ANC0_FB_LPF_SHIFT_MSB_POSN       = (int)11,
   CDC_ANC0_RESERVED7_4_LSB_POSN                      = (int)12,
   ANC_CONTROL19_CDC_ANC0_RESERVED7_4_LSB_POSN        = (int)12,
   CDC_ANC0_RESERVED7_4_MSB_POSN                      = (int)15,
   ANC_CONTROL19_CDC_ANC0_RESERVED7_4_MSB_POSN        = (int)15
};
typedef enum anc_control19_posn_enum anc_control19_posn;

#define CDC_ANC0_FF_SHIFT_LSB_MASK               (0x00000001u)
#define CDC_ANC0_FF_SHIFT_MSB_MASK               (0x00000080u)
#define CDC_ANC0_FFb_SHIFT_LSB_MASK              (0x00000001u)
#define CDC_ANC0_FFb_SHIFT_MSB_MASK              (0x00000008u)
#define CDC_ANC0_FFa_SHIFT_LSB_MASK              (0x00000010u)
#define CDC_ANC0_FFa_SHIFT_MSB_MASK              (0x00000080u)
#define CDC_ANC0_FB_SHIFT_LSB_MASK               (0x00000100u)
#define CDC_ANC0_FB_SHIFT_MSB_MASK               (0x00008000u)
#define CDC_ANC0_FB_LPF_SHIFT_LSB_MASK           (0x00000100u)
#define CDC_ANC0_FB_LPF_SHIFT_MSB_MASK           (0x00000800u)
#define CDC_ANC0_RESERVED7_4_LSB_MASK            (0x00001000u)
#define CDC_ANC0_RESERVED7_4_MSB_MASK            (0x00008000u)

enum anc_control20_posn_enum
{
   CDC_ANC0_LPF_FF_A_CTL_LSB_POSN                     = (int)0,
   ANC_CONTROL20_CDC_ANC0_LPF_FF_A_CTL_LSB_POSN       = (int)0,
   CDC_ANC0_LPF_FF_A_CTL_MSB_POSN                     = (int)7,
   ANC_CONTROL20_CDC_ANC0_LPF_FF_A_CTL_MSB_POSN       = (int)7,
   CDC_ANC0_LPF_FFa_S1_LSB_POSN                       = (int)0,
   ANC_CONTROL20_CDC_ANC0_LPF_FFa_S1_LSB_POSN         = (int)0,
   CDC_ANC0_LPF_FFa_S1_MSB_POSN                       = (int)3,
   ANC_CONTROL20_CDC_ANC0_LPF_FFa_S1_MSB_POSN         = (int)3,
   CDC_ANC0_LPF_FFa_S2_LSB_POSN                       = (int)4,
   ANC_CONTROL20_CDC_ANC0_LPF_FFa_S2_LSB_POSN         = (int)4,
   CDC_ANC0_LPF_FFa_S2_MSB_POSN                       = (int)7,
   ANC_CONTROL20_CDC_ANC0_LPF_FFa_S2_MSB_POSN         = (int)7,
   CDC_ANC0_LPF_FF_B_CTL_LSB_POSN                     = (int)8,
   ANC_CONTROL20_CDC_ANC0_LPF_FF_B_CTL_LSB_POSN       = (int)8,
   CDC_ANC0_LPF_FF_B_CTL_MSB_POSN                     = (int)15,
   ANC_CONTROL20_CDC_ANC0_LPF_FF_B_CTL_MSB_POSN       = (int)15,
   CDC_ANC0_LPF_FFb_S1_LSB_POSN                       = (int)8,
   ANC_CONTROL20_CDC_ANC0_LPF_FFb_S1_LSB_POSN         = (int)8,
   CDC_ANC0_LPF_FFb_S1_MSB_POSN                       = (int)11,
   ANC_CONTROL20_CDC_ANC0_LPF_FFb_S1_MSB_POSN         = (int)11,
   CDC_ANC0_LPF_FFb_S2_LSB_POSN                       = (int)12,
   ANC_CONTROL20_CDC_ANC0_LPF_FFb_S2_LSB_POSN         = (int)12,
   CDC_ANC0_LPF_FFb_S2_MSB_POSN                       = (int)15,
   ANC_CONTROL20_CDC_ANC0_LPF_FFb_S2_MSB_POSN         = (int)15
};
typedef enum anc_control20_posn_enum anc_control20_posn;

#define CDC_ANC0_LPF_FF_A_CTL_LSB_MASK           (0x00000001u)
#define CDC_ANC0_LPF_FF_A_CTL_MSB_MASK           (0x00000080u)
#define CDC_ANC0_LPF_FFa_S1_LSB_MASK             (0x00000001u)
#define CDC_ANC0_LPF_FFa_S1_MSB_MASK             (0x00000008u)
#define CDC_ANC0_LPF_FFa_S2_LSB_MASK             (0x00000010u)
#define CDC_ANC0_LPF_FFa_S2_MSB_MASK             (0x00000080u)
#define CDC_ANC0_LPF_FF_B_CTL_LSB_MASK           (0x00000100u)
#define CDC_ANC0_LPF_FF_B_CTL_MSB_MASK           (0x00008000u)
#define CDC_ANC0_LPF_FFb_S1_LSB_MASK             (0x00000100u)
#define CDC_ANC0_LPF_FFb_S1_MSB_MASK             (0x00000800u)
#define CDC_ANC0_LPF_FFb_S2_LSB_MASK             (0x00001000u)
#define CDC_ANC0_LPF_FFb_S2_MSB_MASK             (0x00008000u)

enum anc_control21_posn_enum
{
   CDC_ANC0_LPF_FB_CTL_LSB_POSN                       = (int)0,
   ANC_CONTROL21_CDC_ANC0_LPF_FB_CTL_LSB_POSN         = (int)0,
   CDC_ANC0_LPF_FB_CTL_MSB_POSN                       = (int)7,
   ANC_CONTROL21_CDC_ANC0_LPF_FB_CTL_MSB_POSN         = (int)7,
   CDC_ANC0_LPF_FB_S1_LSB_POSN                        = (int)0,
   ANC_CONTROL21_CDC_ANC0_LPF_FB_S1_LSB_POSN          = (int)0,
   CDC_ANC0_LPF_FB_S1_MSB_POSN                        = (int)3,
   ANC_CONTROL21_CDC_ANC0_LPF_FB_S1_MSB_POSN          = (int)3,
   CDC_ANC0_LPF_FB_S2_LSB_POSN                        = (int)4,
   ANC_CONTROL21_CDC_ANC0_LPF_FB_S2_LSB_POSN          = (int)4,
   CDC_ANC0_LPF_FB_S2_MSB_POSN                        = (int)7,
   ANC_CONTROL21_CDC_ANC0_LPF_FB_S2_MSB_POSN          = (int)7
};
typedef enum anc_control21_posn_enum anc_control21_posn;

#define CDC_ANC0_LPF_FB_CTL_LSB_MASK             (0x00000001u)
#define CDC_ANC0_LPF_FB_CTL_MSB_MASK             (0x00000080u)
#define CDC_ANC0_LPF_FB_S1_LSB_MASK              (0x00000001u)
#define CDC_ANC0_LPF_FB_S1_MSB_MASK              (0x00000008u)
#define CDC_ANC0_LPF_FB_S2_LSB_MASK              (0x00000010u)
#define CDC_ANC0_LPF_FB_S2_MSB_MASK              (0x00000080u)

enum anc_control22_posn_enum
{
   CDC_ANC0_SMLPF_CTL_LSB_POSN                        = (int)0,
   ANC_CONTROL22_CDC_ANC0_SMLPF_CTL_LSB_POSN          = (int)0,
   CDC_ANC0_SMLPF_CTL_MSB_POSN                        = (int)7,
   ANC_CONTROL22_CDC_ANC0_SMLPF_CTL_MSB_POSN          = (int)7,
   CDC_ANC0_SHIFT_LSB_POSN                            = (int)0,
   ANC_CONTROL22_CDC_ANC0_SHIFT_LSB_POSN              = (int)0,
   CDC_ANC0_SHIFT_MSB_POSN                            = (int)3,
   ANC_CONTROL22_CDC_ANC0_SHIFT_MSB_POSN              = (int)3,
   CDC_ANC0_ZCD_SHIFT_LSB_POSN                        = (int)4,
   ANC_CONTROL22_CDC_ANC0_ZCD_SHIFT_LSB_POSN          = (int)4,
   CDC_ANC0_ZCD_SHIFT_MSB_POSN                        = (int)6,
   ANC_CONTROL22_CDC_ANC0_ZCD_SHIFT_MSB_POSN          = (int)6,
   CDC_ANC0_DCFLT_SHIFT_CTL_LSB_POSN                  = (int)8,
   ANC_CONTROL22_CDC_ANC0_DCFLT_SHIFT_CTL_LSB_POSN    = (int)8,
   CDC_ANC0_DCFLT_SHIFT_CTL_MSB_POSN                  = (int)15,
   ANC_CONTROL22_CDC_ANC0_DCFLT_SHIFT_CTL_MSB_POSN    = (int)15,
   CDC_ANC0_A_SHIFT_LSB_POSN                          = (int)8,
   ANC_CONTROL22_CDC_ANC0_A_SHIFT_LSB_POSN            = (int)8,
   CDC_ANC0_A_SHIFT_MSB_POSN                          = (int)11,
   ANC_CONTROL22_CDC_ANC0_A_SHIFT_MSB_POSN            = (int)11,
   CDC_ANC0_B_SHIFT_LSB_POSN                          = (int)12,
   ANC_CONTROL22_CDC_ANC0_B_SHIFT_LSB_POSN            = (int)12,
   CDC_ANC0_B_SHIFT_MSB_POSN                          = (int)15,
   ANC_CONTROL22_CDC_ANC0_B_SHIFT_MSB_POSN            = (int)15
};
typedef enum anc_control22_posn_enum anc_control22_posn;

#define CDC_ANC0_SMLPF_CTL_LSB_MASK              (0x00000001u)
#define CDC_ANC0_SMLPF_CTL_MSB_MASK              (0x00000080u)
#define CDC_ANC0_SHIFT_LSB_MASK                  (0x00000001u)
#define CDC_ANC0_SHIFT_MSB_MASK                  (0x00000008u)
#define CDC_ANC0_ZCD_SHIFT_LSB_MASK              (0x00000010u)
#define CDC_ANC0_ZCD_SHIFT_MSB_MASK              (0x00000040u)
#define CDC_ANC0_DCFLT_SHIFT_CTL_LSB_MASK        (0x00000100u)
#define CDC_ANC0_DCFLT_SHIFT_CTL_MSB_MASK        (0x00008000u)
#define CDC_ANC0_A_SHIFT_LSB_MASK                (0x00000100u)
#define CDC_ANC0_A_SHIFT_MSB_MASK                (0x00000800u)
#define CDC_ANC0_B_SHIFT_LSB_MASK                (0x00001000u)
#define CDC_ANC0_B_SHIFT_MSB_MASK                (0x00008000u)

enum anc_control23_posn_enum
{
   CDC_ANC0_IIR_ADAPT_CTL_LSB_POSN                    = (int)0,
   ANC_CONTROL23_CDC_ANC0_IIR_ADAPT_CTL_LSB_POSN      = (int)0,
   CDC_ANC0_IIR_ADAPT_CTL_MSB_POSN                    = (int)7,
   ANC_CONTROL23_CDC_ANC0_IIR_ADAPT_CTL_MSB_POSN      = (int)7,
   CDC_ANC0_COEF_ADAPTIVE_POSN                        = (int)0,
   ANC_CONTROL23_CDC_ANC0_COEF_ADAPTIVE_LSB_POSN      = (int)0,
   ANC_CONTROL23_CDC_ANC0_COEF_ADAPTIVE_MSB_POSN      = (int)0,
   CDC_ANC0_COEF_UPDATE_EN_POSN                       = (int)1,
   ANC_CONTROL23_CDC_ANC0_COEF_UPDATE_EN_LSB_POSN     = (int)1,
   ANC_CONTROL23_CDC_ANC0_COEF_UPDATE_EN_MSB_POSN     = (int)1,
   CDC_ANC0_FFGAIN_ADAPTIVE_POSN                      = (int)2,
   ANC_CONTROL23_CDC_ANC0_FFGAIN_ADAPTIVE_LSB_POSN    = (int)2,
   ANC_CONTROL23_CDC_ANC0_FFGAIN_ADAPTIVE_MSB_POSN    = (int)2,
   CDC_ANC0_FFGAIN_UPDATE_EN_POSN                     = (int)3,
   ANC_CONTROL23_CDC_ANC0_FFGAIN_UPDATE_EN_LSB_POSN   = (int)3,
   ANC_CONTROL23_CDC_ANC0_FFGAIN_UPDATE_EN_MSB_POSN   = (int)3,
   CDC_ANC0_FFGAIN_ZCD_EN_POSN                        = (int)4,
   ANC_CONTROL23_CDC_ANC0_FFGAIN_ZCD_EN_LSB_POSN      = (int)4,
   ANC_CONTROL23_CDC_ANC0_FFGAIN_ZCD_EN_MSB_POSN      = (int)4,
   CDC_ANC0_COEF_SMP_EN_POSN                          = (int)5,
   ANC_CONTROL23_CDC_ANC0_COEF_SMP_EN_LSB_POSN        = (int)5,
   ANC_CONTROL23_CDC_ANC0_COEF_SMP_EN_MSB_POSN        = (int)5,
   CDC_ANC0_FFGAIN_SMP_EN_POSN                        = (int)6,
   ANC_CONTROL23_CDC_ANC0_FFGAIN_SMP_EN_LSB_POSN      = (int)6,
   ANC_CONTROL23_CDC_ANC0_FFGAIN_SMP_EN_MSB_POSN      = (int)6
};
typedef enum anc_control23_posn_enum anc_control23_posn;

#define CDC_ANC0_IIR_ADAPT_CTL_LSB_MASK          (0x00000001u)
#define CDC_ANC0_IIR_ADAPT_CTL_MSB_MASK          (0x00000080u)
#define CDC_ANC0_COEF_ADAPTIVE_MASK              (0x00000001u)
#define CDC_ANC0_COEF_UPDATE_EN_MASK             (0x00000002u)
#define CDC_ANC0_FFGAIN_ADAPTIVE_MASK            (0x00000004u)
#define CDC_ANC0_FFGAIN_UPDATE_EN_MASK           (0x00000008u)
#define CDC_ANC0_FFGAIN_ZCD_EN_MASK              (0x00000010u)
#define CDC_ANC0_COEF_SMP_EN_MASK                (0x00000020u)
#define CDC_ANC0_FFGAIN_SMP_EN_MASK              (0x00000040u)

enum anc_control26_posn_enum
{
   CDC_ANC0_FF_A_GAIN_CTL_LSB_POSN                    = (int)0,
   ANC_CONTROL26_CDC_ANC0_FF_A_GAIN_CTL_LSB_POSN      = (int)0,
   CDC_ANC0_FF_A_GAIN_CTL_MSB_POSN                    = (int)7,
   ANC_CONTROL26_CDC_ANC0_FF_A_GAIN_CTL_MSB_POSN      = (int)7,
   CDC_ANC0_FF_B_GAIN_CTL_LSB_POSN                    = (int)8,
   ANC_CONTROL26_CDC_ANC0_FF_B_GAIN_CTL_LSB_POSN      = (int)8,
   CDC_ANC0_FF_B_GAIN_CTL_MSB_POSN                    = (int)15,
   ANC_CONTROL26_CDC_ANC0_FF_B_GAIN_CTL_MSB_POSN      = (int)15
};
typedef enum anc_control26_posn_enum anc_control26_posn;

#define CDC_ANC0_FF_A_GAIN_CTL_LSB_MASK          (0x00000001u)
#define CDC_ANC0_FF_A_GAIN_CTL_MSB_MASK          (0x00000080u)
#define CDC_ANC0_FF_B_GAIN_CTL_LSB_MASK          (0x00000100u)
#define CDC_ANC0_FF_B_GAIN_CTL_MSB_MASK          (0x00008000u)

enum anc_control27_posn_enum
{
   CDC_ANC0_FB_GAIN_CTL_LSB_POSN                      = (int)0,
   ANC_CONTROL27_CDC_ANC0_FB_GAIN_CTL_LSB_POSN        = (int)0,
   CDC_ANC0_FB_GAIN_CTL_MSB_POSN                      = (int)7,
   ANC_CONTROL27_CDC_ANC0_FB_GAIN_CTL_MSB_POSN        = (int)7
};
typedef enum anc_control27_posn_enum anc_control27_posn;

#define CDC_ANC0_FB_GAIN_CTL_LSB_MASK            (0x00000001u)
#define CDC_ANC0_FB_GAIN_CTL_MSB_MASK            (0x00000080u)

enum anc_control28_posn_enum
{
   CDC_ANC0_ANC_DMIC_X0P5_B_SEL_POSN                  = (int)0,
   ANC_CONTROL28_CDC_ANC0_ANC_DMIC_X0P5_B_SEL_LSB_POSN = (int)0,
   ANC_CONTROL28_CDC_ANC0_ANC_DMIC_X0P5_B_SEL_MSB_POSN = (int)0,
   CDC_ANC0_ANC_DMIC_X0P5_A_SEL_POSN                  = (int)1,
   ANC_CONTROL28_CDC_ANC0_ANC_DMIC_X0P5_A_SEL_LSB_POSN = (int)1,
   ANC_CONTROL28_CDC_ANC0_ANC_DMIC_X0P5_A_SEL_MSB_POSN = (int)1,
   CDC_ANC0_ANC_FB_TUNE_DSM_EN_POSN                   = (int)2,
   ANC_CONTROL28_CDC_ANC0_ANC_FB_TUNE_DSM_EN_LSB_POSN = (int)2,
   ANC_CONTROL28_CDC_ANC0_ANC_FB_TUNE_DSM_EN_MSB_POSN = (int)2,
   CDC_ANC0_ANC_FB_TUNE_DSM_CLR_POSN                  = (int)3,
   ANC_CONTROL28_CDC_ANC0_ANC_FB_TUNE_DSM_CLR_LSB_POSN = (int)3,
   ANC_CONTROL28_CDC_ANC0_ANC_FB_TUNE_DSM_CLR_MSB_POSN = (int)3
};
typedef enum anc_control28_posn_enum anc_control28_posn;

#define CDC_ANC0_ANC_DMIC_X0P5_B_SEL_MASK        (0x00000001u)
#define CDC_ANC0_ANC_DMIC_X0P5_A_SEL_MASK        (0x00000002u)
#define CDC_ANC0_ANC_FB_TUNE_DSM_EN_MASK         (0x00000004u)
#define CDC_ANC0_ANC_FB_TUNE_DSM_CLR_MASK        (0x00000008u)

enum anc_control29_posn_enum
{
   CDC_ANC1_CLK_RESET_CTL_LSB_POSN                    = (int)0,
   ANC_CONTROL29_CDC_ANC1_CLK_RESET_CTL_LSB_POSN      = (int)0,
   CDC_ANC1_CLK_RESET_CTL_MSB_POSN                    = (int)7,
   ANC_CONTROL29_CDC_ANC1_CLK_RESET_CTL_MSB_POSN      = (int)7,
   CDC_ANC1_FFa_CLK_EN_POSN                           = (int)0,
   ANC_CONTROL29_CDC_ANC1_FFa_CLK_EN_LSB_POSN         = (int)0,
   ANC_CONTROL29_CDC_ANC1_FFa_CLK_EN_MSB_POSN         = (int)0,
   CDC_ANC1_FFb_CLK_EN_POSN                           = (int)1,
   ANC_CONTROL29_CDC_ANC1_FFb_CLK_EN_LSB_POSN         = (int)1,
   ANC_CONTROL29_CDC_ANC1_FFb_CLK_EN_MSB_POSN         = (int)1,
   CDC_ANC1_FB_CLK_EN_POSN                            = (int)2,
   ANC_CONTROL29_CDC_ANC1_FB_CLK_EN_LSB_POSN          = (int)2,
   ANC_CONTROL29_CDC_ANC1_FB_CLK_EN_MSB_POSN          = (int)2,
   CDC_ANC1_FFa_RESET_EN_POSN                         = (int)3,
   ANC_CONTROL29_CDC_ANC1_FFa_RESET_EN_LSB_POSN       = (int)3,
   ANC_CONTROL29_CDC_ANC1_FFa_RESET_EN_MSB_POSN       = (int)3,
   CDC_ANC1_FFb_RESET_EN_POSN                         = (int)4,
   ANC_CONTROL29_CDC_ANC1_FFb_RESET_EN_LSB_POSN       = (int)4,
   ANC_CONTROL29_CDC_ANC1_FFb_RESET_EN_MSB_POSN       = (int)4,
   CDC_ANC1_FB_RESET_EN_POSN                          = (int)5,
   ANC_CONTROL29_CDC_ANC1_FB_RESET_EN_LSB_POSN        = (int)5,
   ANC_CONTROL29_CDC_ANC1_FB_RESET_EN_MSB_POSN        = (int)5,
   CDC_ANC1_SMARTLNQ_A_EN_POSN                        = (int)6,
   ANC_CONTROL29_CDC_ANC1_SMARTLNQ_A_EN_LSB_POSN      = (int)6,
   ANC_CONTROL29_CDC_ANC1_SMARTLNQ_A_EN_MSB_POSN      = (int)6,
   CDC_ANC1_SMARTLNQ_B_EN_POSN                        = (int)7,
   ANC_CONTROL29_CDC_ANC1_SMARTLNQ_B_EN_LSB_POSN      = (int)7,
   ANC_CONTROL29_CDC_ANC1_SMARTLNQ_B_EN_MSB_POSN      = (int)7
};
typedef enum anc_control29_posn_enum anc_control29_posn;

#define CDC_ANC1_CLK_RESET_CTL_LSB_MASK          (0x00000001u)
#define CDC_ANC1_CLK_RESET_CTL_MSB_MASK          (0x00000080u)
#define CDC_ANC1_FFa_CLK_EN_MASK                 (0x00000001u)
#define CDC_ANC1_FFb_CLK_EN_MASK                 (0x00000002u)
#define CDC_ANC1_FB_CLK_EN_MASK                  (0x00000004u)
#define CDC_ANC1_FFa_RESET_EN_MASK               (0x00000008u)
#define CDC_ANC1_FFb_RESET_EN_MASK               (0x00000010u)
#define CDC_ANC1_FB_RESET_EN_MASK                (0x00000020u)
#define CDC_ANC1_SMARTLNQ_A_EN_MASK              (0x00000040u)
#define CDC_ANC1_SMARTLNQ_B_EN_MASK              (0x00000080u)

enum anc_control30_posn_enum
{
   CDC_ANC1_MODE_1_CTL_LSB_POSN                       = (int)0,
   ANC_CONTROL30_CDC_ANC1_MODE_1_CTL_LSB_POSN         = (int)0,
   CDC_ANC1_MODE_1_CTL_MSB_POSN                       = (int)7,
   ANC_CONTROL30_CDC_ANC1_MODE_1_CTL_MSB_POSN         = (int)7,
   CDC_ANC1_ANC_OUT_EN_POSN                           = (int)0,
   ANC_CONTROL30_CDC_ANC1_ANC_OUT_EN_LSB_POSN         = (int)0,
   ANC_CONTROL30_CDC_ANC1_ANC_OUT_EN_MSB_POSN         = (int)0,
   CDC_ANC1_ANC_ADC_DMIC_A_SEL_POSN                   = (int)1,
   ANC_CONTROL30_CDC_ANC1_ANC_ADC_DMIC_A_SEL_LSB_POSN = (int)1,
   ANC_CONTROL30_CDC_ANC1_ANC_ADC_DMIC_A_SEL_MSB_POSN = (int)1,
   CDC_ANC1_ANC_ADC_DMIC_B_SEL_POSN                   = (int)2,
   ANC_CONTROL30_CDC_ANC1_ANC_ADC_DMIC_B_SEL_LSB_POSN = (int)2,
   ANC_CONTROL30_CDC_ANC1_ANC_ADC_DMIC_B_SEL_MSB_POSN = (int)2,
   CDC_ANC1_ANC_OUTMIX_EN_POSN                        = (int)3,
   ANC_CONTROL30_CDC_ANC1_ANC_OUTMIX_EN_LSB_POSN      = (int)3,
   ANC_CONTROL30_CDC_ANC1_ANC_OUTMIX_EN_MSB_POSN      = (int)3,
   CDC_ANC1_FFb_IN_EN_POSN                            = (int)4,
   ANC_CONTROL30_CDC_ANC1_FFb_IN_EN_LSB_POSN          = (int)4,
   ANC_CONTROL30_CDC_ANC1_FFb_IN_EN_MSB_POSN          = (int)4,
   CDC_ANC1_FFa_IN_EN_POSN                            = (int)5,
   ANC_CONTROL30_CDC_ANC1_FFa_IN_EN_LSB_POSN          = (int)5,
   ANC_CONTROL30_CDC_ANC1_FFa_IN_EN_MSB_POSN          = (int)5,
   CDC_ANC1_ANC_SMLPF_EN_POSN                         = (int)6,
   ANC_CONTROL30_CDC_ANC1_ANC_SMLPF_EN_LSB_POSN       = (int)6,
   ANC_CONTROL30_CDC_ANC1_ANC_SMLPF_EN_MSB_POSN       = (int)6,
   CDC_ANC1_FB_ON_FBMON_IS_TRUE_POSN                  = (int)7,
   ANC_CONTROL30_CDC_ANC1_FB_ON_FBMON_IS_TRUE_LSB_POSN = (int)7,
   ANC_CONTROL30_CDC_ANC1_FB_ON_FBMON_IS_TRUE_MSB_POSN = (int)7,
   CDC_ANC1_MODE_2_CTL_LSB_POSN                       = (int)8,
   ANC_CONTROL30_CDC_ANC1_MODE_2_CTL_LSB_POSN         = (int)8,
   CDC_ANC1_MODE_2_CTL_MSB_POSN                       = (int)15,
   ANC_CONTROL30_CDC_ANC1_MODE_2_CTL_MSB_POSN         = (int)15,
   CDC_ANC1_ANC_FFLE_EN_POSN                          = (int)8,
   ANC_CONTROL30_CDC_ANC1_ANC_FFLE_EN_LSB_POSN        = (int)8,
   ANC_CONTROL30_CDC_ANC1_ANC_FFLE_EN_MSB_POSN        = (int)8,
   CDC_ANC1_ANC_FFGAIN_B_EN_POSN                      = (int)9,
   ANC_CONTROL30_CDC_ANC1_ANC_FFGAIN_B_EN_LSB_POSN    = (int)9,
   ANC_CONTROL30_CDC_ANC1_ANC_FFGAIN_B_EN_MSB_POSN    = (int)9,
   CDC_ANC1_ANC_FFGAIN_A_EN_POSN                      = (int)10,
   ANC_CONTROL30_CDC_ANC1_ANC_FFGAIN_A_EN_LSB_POSN    = (int)10,
   ANC_CONTROL30_CDC_ANC1_ANC_FFGAIN_A_EN_MSB_POSN    = (int)10,
   CDC_ANC1_ANC_DCFILT_B_EN_POSN                      = (int)11,
   ANC_CONTROL30_CDC_ANC1_ANC_DCFILT_B_EN_LSB_POSN    = (int)11,
   ANC_CONTROL30_CDC_ANC1_ANC_DCFILT_B_EN_MSB_POSN    = (int)11,
   CDC_ANC1_ANC_DCFILT_A_EN_POSN                      = (int)12,
   ANC_CONTROL30_CDC_ANC1_ANC_DCFILT_A_EN_LSB_POSN    = (int)12,
   ANC_CONTROL30_CDC_ANC1_ANC_DCFILT_A_EN_MSB_POSN    = (int)12,
   CDC_ANC1_ANC_DMIC_X2_B_SEL_POSN                    = (int)13,
   ANC_CONTROL30_CDC_ANC1_ANC_DMIC_X2_B_SEL_LSB_POSN  = (int)13,
   ANC_CONTROL30_CDC_ANC1_ANC_DMIC_X2_B_SEL_MSB_POSN  = (int)13,
   CDC_ANC1_ANC_DMIC_X2_A_SEL_POSN                    = (int)14,
   ANC_CONTROL30_CDC_ANC1_ANC_DMIC_X2_A_SEL_LSB_POSN  = (int)14,
   ANC_CONTROL30_CDC_ANC1_ANC_DMIC_X2_A_SEL_MSB_POSN  = (int)14,
   CDC_ANC1_ANC_FBGAIN_EN_POSN                        = (int)15,
   ANC_CONTROL30_CDC_ANC1_ANC_FBGAIN_EN_LSB_POSN      = (int)15,
   ANC_CONTROL30_CDC_ANC1_ANC_FBGAIN_EN_MSB_POSN      = (int)15
};
typedef enum anc_control30_posn_enum anc_control30_posn;

#define CDC_ANC1_MODE_1_CTL_LSB_MASK             (0x00000001u)
#define CDC_ANC1_MODE_1_CTL_MSB_MASK             (0x00000080u)
#define CDC_ANC1_ANC_OUT_EN_MASK                 (0x00000001u)
#define CDC_ANC1_ANC_ADC_DMIC_A_SEL_MASK         (0x00000002u)
#define CDC_ANC1_ANC_ADC_DMIC_B_SEL_MASK         (0x00000004u)
#define CDC_ANC1_ANC_OUTMIX_EN_MASK              (0x00000008u)
#define CDC_ANC1_FFb_IN_EN_MASK                  (0x00000010u)
#define CDC_ANC1_FFa_IN_EN_MASK                  (0x00000020u)
#define CDC_ANC1_ANC_SMLPF_EN_MASK               (0x00000040u)
#define CDC_ANC1_FB_ON_FBMON_IS_TRUE_MASK        (0x00000080u)
#define CDC_ANC1_MODE_2_CTL_LSB_MASK             (0x00000100u)
#define CDC_ANC1_MODE_2_CTL_MSB_MASK             (0x00008000u)
#define CDC_ANC1_ANC_FFLE_EN_MASK                (0x00000100u)
#define CDC_ANC1_ANC_FFGAIN_B_EN_MASK            (0x00000200u)
#define CDC_ANC1_ANC_FFGAIN_A_EN_MASK            (0x00000400u)
#define CDC_ANC1_ANC_DCFILT_B_EN_MASK            (0x00000800u)
#define CDC_ANC1_ANC_DCFILT_A_EN_MASK            (0x00001000u)
#define CDC_ANC1_ANC_DMIC_X2_B_SEL_MASK          (0x00002000u)
#define CDC_ANC1_ANC_DMIC_X2_A_SEL_MASK          (0x00004000u)
#define CDC_ANC1_ANC_FBGAIN_EN_MASK              (0x00008000u)

enum anc_control31_posn_enum
{
   CDC_ANC1_FF_SHIFT_LSB_POSN                         = (int)0,
   ANC_CONTROL31_CDC_ANC1_FF_SHIFT_LSB_POSN           = (int)0,
   CDC_ANC1_FF_SHIFT_MSB_POSN                         = (int)7,
   ANC_CONTROL31_CDC_ANC1_FF_SHIFT_MSB_POSN           = (int)7,
   CDC_ANC1_FFb_SHIFT_LSB_POSN                        = (int)0,
   ANC_CONTROL31_CDC_ANC1_FFb_SHIFT_LSB_POSN          = (int)0,
   CDC_ANC1_FFb_SHIFT_MSB_POSN                        = (int)3,
   ANC_CONTROL31_CDC_ANC1_FFb_SHIFT_MSB_POSN          = (int)3,
   CDC_ANC1_FFa_SHIFT_LSB_POSN                        = (int)4,
   ANC_CONTROL31_CDC_ANC1_FFa_SHIFT_LSB_POSN          = (int)4,
   CDC_ANC1_FFa_SHIFT_MSB_POSN                        = (int)7,
   ANC_CONTROL31_CDC_ANC1_FFa_SHIFT_MSB_POSN          = (int)7,
   CDC_ANC1_FB_SHIFT_LSB_POSN                         = (int)8,
   ANC_CONTROL31_CDC_ANC1_FB_SHIFT_LSB_POSN           = (int)8,
   CDC_ANC1_FB_SHIFT_MSB_POSN                         = (int)15,
   ANC_CONTROL31_CDC_ANC1_FB_SHIFT_MSB_POSN           = (int)15,
   CDC_ANC1_FB_LPF_SHIFT_LSB_POSN                     = (int)8,
   ANC_CONTROL31_CDC_ANC1_FB_LPF_SHIFT_LSB_POSN       = (int)8,
   CDC_ANC1_FB_LPF_SHIFT_MSB_POSN                     = (int)11,
   ANC_CONTROL31_CDC_ANC1_FB_LPF_SHIFT_MSB_POSN       = (int)11,
   CDC_ANC1_RESERVED7_4_LSB_POSN                      = (int)12,
   ANC_CONTROL31_CDC_ANC1_RESERVED7_4_LSB_POSN        = (int)12,
   CDC_ANC1_RESERVED7_4_MSB_POSN                      = (int)15,
   ANC_CONTROL31_CDC_ANC1_RESERVED7_4_MSB_POSN        = (int)15
};
typedef enum anc_control31_posn_enum anc_control31_posn;

#define CDC_ANC1_FF_SHIFT_LSB_MASK               (0x00000001u)
#define CDC_ANC1_FF_SHIFT_MSB_MASK               (0x00000080u)
#define CDC_ANC1_FFb_SHIFT_LSB_MASK              (0x00000001u)
#define CDC_ANC1_FFb_SHIFT_MSB_MASK              (0x00000008u)
#define CDC_ANC1_FFa_SHIFT_LSB_MASK              (0x00000010u)
#define CDC_ANC1_FFa_SHIFT_MSB_MASK              (0x00000080u)
#define CDC_ANC1_FB_SHIFT_LSB_MASK               (0x00000100u)
#define CDC_ANC1_FB_SHIFT_MSB_MASK               (0x00008000u)
#define CDC_ANC1_FB_LPF_SHIFT_LSB_MASK           (0x00000100u)
#define CDC_ANC1_FB_LPF_SHIFT_MSB_MASK           (0x00000800u)
#define CDC_ANC1_RESERVED7_4_LSB_MASK            (0x00001000u)
#define CDC_ANC1_RESERVED7_4_MSB_MASK            (0x00008000u)

enum anc_control32_posn_enum
{
   CDC_ANC1_LPF_FF_A_CTL_LSB_POSN                     = (int)0,
   ANC_CONTROL32_CDC_ANC1_LPF_FF_A_CTL_LSB_POSN       = (int)0,
   CDC_ANC1_LPF_FF_A_CTL_MSB_POSN                     = (int)7,
   ANC_CONTROL32_CDC_ANC1_LPF_FF_A_CTL_MSB_POSN       = (int)7,
   CDC_ANC1_LPF_FFa_S1_LSB_POSN                       = (int)0,
   ANC_CONTROL32_CDC_ANC1_LPF_FFa_S1_LSB_POSN         = (int)0,
   CDC_ANC1_LPF_FFa_S1_MSB_POSN                       = (int)3,
   ANC_CONTROL32_CDC_ANC1_LPF_FFa_S1_MSB_POSN         = (int)3,
   CDC_ANC1_LPF_FFa_S2_LSB_POSN                       = (int)4,
   ANC_CONTROL32_CDC_ANC1_LPF_FFa_S2_LSB_POSN         = (int)4,
   CDC_ANC1_LPF_FFa_S2_MSB_POSN                       = (int)7,
   ANC_CONTROL32_CDC_ANC1_LPF_FFa_S2_MSB_POSN         = (int)7,
   CDC_ANC1_LPF_FF_B_CTL_LSB_POSN                     = (int)8,
   ANC_CONTROL32_CDC_ANC1_LPF_FF_B_CTL_LSB_POSN       = (int)8,
   CDC_ANC1_LPF_FF_B_CTL_MSB_POSN                     = (int)15,
   ANC_CONTROL32_CDC_ANC1_LPF_FF_B_CTL_MSB_POSN       = (int)15,
   CDC_ANC1_LPF_FFb_S1_LSB_POSN                       = (int)8,
   ANC_CONTROL32_CDC_ANC1_LPF_FFb_S1_LSB_POSN         = (int)8,
   CDC_ANC1_LPF_FFb_S1_MSB_POSN                       = (int)11,
   ANC_CONTROL32_CDC_ANC1_LPF_FFb_S1_MSB_POSN         = (int)11,
   CDC_ANC1_LPF_FFb_S2_LSB_POSN                       = (int)12,
   ANC_CONTROL32_CDC_ANC1_LPF_FFb_S2_LSB_POSN         = (int)12,
   CDC_ANC1_LPF_FFb_S2_MSB_POSN                       = (int)15,
   ANC_CONTROL32_CDC_ANC1_LPF_FFb_S2_MSB_POSN         = (int)15
};
typedef enum anc_control32_posn_enum anc_control32_posn;

#define CDC_ANC1_LPF_FF_A_CTL_LSB_MASK           (0x00000001u)
#define CDC_ANC1_LPF_FF_A_CTL_MSB_MASK           (0x00000080u)
#define CDC_ANC1_LPF_FFa_S1_LSB_MASK             (0x00000001u)
#define CDC_ANC1_LPF_FFa_S1_MSB_MASK             (0x00000008u)
#define CDC_ANC1_LPF_FFa_S2_LSB_MASK             (0x00000010u)
#define CDC_ANC1_LPF_FFa_S2_MSB_MASK             (0x00000080u)
#define CDC_ANC1_LPF_FF_B_CTL_LSB_MASK           (0x00000100u)
#define CDC_ANC1_LPF_FF_B_CTL_MSB_MASK           (0x00008000u)
#define CDC_ANC1_LPF_FFb_S1_LSB_MASK             (0x00000100u)
#define CDC_ANC1_LPF_FFb_S1_MSB_MASK             (0x00000800u)
#define CDC_ANC1_LPF_FFb_S2_LSB_MASK             (0x00001000u)
#define CDC_ANC1_LPF_FFb_S2_MSB_MASK             (0x00008000u)

enum anc_control33_posn_enum
{
   CDC_ANC1_LPF_FB_CTL_LSB_POSN                       = (int)0,
   ANC_CONTROL33_CDC_ANC1_LPF_FB_CTL_LSB_POSN         = (int)0,
   CDC_ANC1_LPF_FB_CTL_MSB_POSN                       = (int)7,
   ANC_CONTROL33_CDC_ANC1_LPF_FB_CTL_MSB_POSN         = (int)7,
   CDC_ANC1_LPF_FB_S1_LSB_POSN                        = (int)0,
   ANC_CONTROL33_CDC_ANC1_LPF_FB_S1_LSB_POSN          = (int)0,
   CDC_ANC1_LPF_FB_S1_MSB_POSN                        = (int)3,
   ANC_CONTROL33_CDC_ANC1_LPF_FB_S1_MSB_POSN          = (int)3,
   CDC_ANC1_LPF_FB_S2_LSB_POSN                        = (int)4,
   ANC_CONTROL33_CDC_ANC1_LPF_FB_S2_LSB_POSN          = (int)4,
   CDC_ANC1_LPF_FB_S2_MSB_POSN                        = (int)7,
   ANC_CONTROL33_CDC_ANC1_LPF_FB_S2_MSB_POSN          = (int)7
};
typedef enum anc_control33_posn_enum anc_control33_posn;

#define CDC_ANC1_LPF_FB_CTL_LSB_MASK             (0x00000001u)
#define CDC_ANC1_LPF_FB_CTL_MSB_MASK             (0x00000080u)
#define CDC_ANC1_LPF_FB_S1_LSB_MASK              (0x00000001u)
#define CDC_ANC1_LPF_FB_S1_MSB_MASK              (0x00000008u)
#define CDC_ANC1_LPF_FB_S2_LSB_MASK              (0x00000010u)
#define CDC_ANC1_LPF_FB_S2_MSB_MASK              (0x00000080u)

enum anc_control34_posn_enum
{
   CDC_ANC1_SMLPF_CTL_LSB_POSN                        = (int)0,
   ANC_CONTROL34_CDC_ANC1_SMLPF_CTL_LSB_POSN          = (int)0,
   CDC_ANC1_SMLPF_CTL_MSB_POSN                        = (int)7,
   ANC_CONTROL34_CDC_ANC1_SMLPF_CTL_MSB_POSN          = (int)7,
   CDC_ANC1_SHIFT_LSB_POSN                            = (int)0,
   ANC_CONTROL34_CDC_ANC1_SHIFT_LSB_POSN              = (int)0,
   CDC_ANC1_SHIFT_MSB_POSN                            = (int)3,
   ANC_CONTROL34_CDC_ANC1_SHIFT_MSB_POSN              = (int)3,
   CDC_ANC1_ZCD_SHIFT_LSB_POSN                        = (int)4,
   ANC_CONTROL34_CDC_ANC1_ZCD_SHIFT_LSB_POSN          = (int)4,
   CDC_ANC1_ZCD_SHIFT_MSB_POSN                        = (int)6,
   ANC_CONTROL34_CDC_ANC1_ZCD_SHIFT_MSB_POSN          = (int)6,
   CDC_ANC1_DCFLT_SHIFT_CTL_LSB_POSN                  = (int)8,
   ANC_CONTROL34_CDC_ANC1_DCFLT_SHIFT_CTL_LSB_POSN    = (int)8,
   CDC_ANC1_DCFLT_SHIFT_CTL_MSB_POSN                  = (int)15,
   ANC_CONTROL34_CDC_ANC1_DCFLT_SHIFT_CTL_MSB_POSN    = (int)15,
   CDC_ANC1_A_SHIFT_LSB_POSN                          = (int)8,
   ANC_CONTROL34_CDC_ANC1_A_SHIFT_LSB_POSN            = (int)8,
   CDC_ANC1_A_SHIFT_MSB_POSN                          = (int)11,
   ANC_CONTROL34_CDC_ANC1_A_SHIFT_MSB_POSN            = (int)11,
   CDC_ANC1_B_SHIFT_LSB_POSN                          = (int)12,
   ANC_CONTROL34_CDC_ANC1_B_SHIFT_LSB_POSN            = (int)12,
   CDC_ANC1_B_SHIFT_MSB_POSN                          = (int)15,
   ANC_CONTROL34_CDC_ANC1_B_SHIFT_MSB_POSN            = (int)15
};
typedef enum anc_control34_posn_enum anc_control34_posn;

#define CDC_ANC1_SMLPF_CTL_LSB_MASK              (0x00000001u)
#define CDC_ANC1_SMLPF_CTL_MSB_MASK              (0x00000080u)
#define CDC_ANC1_SHIFT_LSB_MASK                  (0x00000001u)
#define CDC_ANC1_SHIFT_MSB_MASK                  (0x00000008u)
#define CDC_ANC1_ZCD_SHIFT_LSB_MASK              (0x00000010u)
#define CDC_ANC1_ZCD_SHIFT_MSB_MASK              (0x00000040u)
#define CDC_ANC1_DCFLT_SHIFT_CTL_LSB_MASK        (0x00000100u)
#define CDC_ANC1_DCFLT_SHIFT_CTL_MSB_MASK        (0x00008000u)
#define CDC_ANC1_A_SHIFT_LSB_MASK                (0x00000100u)
#define CDC_ANC1_A_SHIFT_MSB_MASK                (0x00000800u)
#define CDC_ANC1_B_SHIFT_LSB_MASK                (0x00001000u)
#define CDC_ANC1_B_SHIFT_MSB_MASK                (0x00008000u)

enum anc_control35_posn_enum
{
   CDC_ANC1_IIR_ADAPT_CTL_LSB_POSN                    = (int)0,
   ANC_CONTROL35_CDC_ANC1_IIR_ADAPT_CTL_LSB_POSN      = (int)0,
   CDC_ANC1_IIR_ADAPT_CTL_MSB_POSN                    = (int)7,
   ANC_CONTROL35_CDC_ANC1_IIR_ADAPT_CTL_MSB_POSN      = (int)7,
   CDC_ANC1_COEF_ADAPTIVE_POSN                        = (int)0,
   ANC_CONTROL35_CDC_ANC1_COEF_ADAPTIVE_LSB_POSN      = (int)0,
   ANC_CONTROL35_CDC_ANC1_COEF_ADAPTIVE_MSB_POSN      = (int)0,
   CDC_ANC1_COEF_UPDATE_EN_POSN                       = (int)1,
   ANC_CONTROL35_CDC_ANC1_COEF_UPDATE_EN_LSB_POSN     = (int)1,
   ANC_CONTROL35_CDC_ANC1_COEF_UPDATE_EN_MSB_POSN     = (int)1,
   CDC_ANC1_FFGAIN_ADAPTIVE_POSN                      = (int)2,
   ANC_CONTROL35_CDC_ANC1_FFGAIN_ADAPTIVE_LSB_POSN    = (int)2,
   ANC_CONTROL35_CDC_ANC1_FFGAIN_ADAPTIVE_MSB_POSN    = (int)2,
   CDC_ANC1_FFGAIN_UPDATE_EN_POSN                     = (int)3,
   ANC_CONTROL35_CDC_ANC1_FFGAIN_UPDATE_EN_LSB_POSN   = (int)3,
   ANC_CONTROL35_CDC_ANC1_FFGAIN_UPDATE_EN_MSB_POSN   = (int)3,
   CDC_ANC1_FFGAIN_ZCD_EN_POSN                        = (int)4,
   ANC_CONTROL35_CDC_ANC1_FFGAIN_ZCD_EN_LSB_POSN      = (int)4,
   ANC_CONTROL35_CDC_ANC1_FFGAIN_ZCD_EN_MSB_POSN      = (int)4,
   CDC_ANC1_COEF_SMP_EN_POSN                          = (int)5,
   ANC_CONTROL35_CDC_ANC1_COEF_SMP_EN_LSB_POSN        = (int)5,
   ANC_CONTROL35_CDC_ANC1_COEF_SMP_EN_MSB_POSN        = (int)5,
   CDC_ANC1_FFGAIN_SMP_EN_POSN                        = (int)6,
   ANC_CONTROL35_CDC_ANC1_FFGAIN_SMP_EN_LSB_POSN      = (int)6,
   ANC_CONTROL35_CDC_ANC1_FFGAIN_SMP_EN_MSB_POSN      = (int)6
};
typedef enum anc_control35_posn_enum anc_control35_posn;

#define CDC_ANC1_IIR_ADAPT_CTL_LSB_MASK          (0x00000001u)
#define CDC_ANC1_IIR_ADAPT_CTL_MSB_MASK          (0x00000080u)
#define CDC_ANC1_COEF_ADAPTIVE_MASK              (0x00000001u)
#define CDC_ANC1_COEF_UPDATE_EN_MASK             (0x00000002u)
#define CDC_ANC1_FFGAIN_ADAPTIVE_MASK            (0x00000004u)
#define CDC_ANC1_FFGAIN_UPDATE_EN_MASK           (0x00000008u)
#define CDC_ANC1_FFGAIN_ZCD_EN_MASK              (0x00000010u)
#define CDC_ANC1_COEF_SMP_EN_MASK                (0x00000020u)
#define CDC_ANC1_FFGAIN_SMP_EN_MASK              (0x00000040u)

enum anc_control38_posn_enum
{
   CDC_ANC1_FF_A_GAIN_CTL_LSB_POSN                    = (int)0,
   ANC_CONTROL38_CDC_ANC1_FF_A_GAIN_CTL_LSB_POSN      = (int)0,
   CDC_ANC1_FF_A_GAIN_CTL_MSB_POSN                    = (int)7,
   ANC_CONTROL38_CDC_ANC1_FF_A_GAIN_CTL_MSB_POSN      = (int)7,
   CDC_ANC1_FF_B_GAIN_CTL_LSB_POSN                    = (int)8,
   ANC_CONTROL38_CDC_ANC1_FF_B_GAIN_CTL_LSB_POSN      = (int)8,
   CDC_ANC1_FF_B_GAIN_CTL_MSB_POSN                    = (int)15,
   ANC_CONTROL38_CDC_ANC1_FF_B_GAIN_CTL_MSB_POSN      = (int)15
};
typedef enum anc_control38_posn_enum anc_control38_posn;

#define CDC_ANC1_FF_A_GAIN_CTL_LSB_MASK          (0x00000001u)
#define CDC_ANC1_FF_A_GAIN_CTL_MSB_MASK          (0x00000080u)
#define CDC_ANC1_FF_B_GAIN_CTL_LSB_MASK          (0x00000100u)
#define CDC_ANC1_FF_B_GAIN_CTL_MSB_MASK          (0x00008000u)

enum anc_control39_posn_enum
{
   CDC_ANC1_FB_GAIN_CTL_LSB_POSN                      = (int)0,
   ANC_CONTROL39_CDC_ANC1_FB_GAIN_CTL_LSB_POSN        = (int)0,
   CDC_ANC1_FB_GAIN_CTL_MSB_POSN                      = (int)7,
   ANC_CONTROL39_CDC_ANC1_FB_GAIN_CTL_MSB_POSN        = (int)7
};
typedef enum anc_control39_posn_enum anc_control39_posn;

#define CDC_ANC1_FB_GAIN_CTL_LSB_MASK            (0x00000001u)
#define CDC_ANC1_FB_GAIN_CTL_MSB_MASK            (0x00000080u)

enum anc_control40_posn_enum
{
   CDC_ANC1_ANC_DMIC_X0P5_B_SEL_POSN                  = (int)0,
   ANC_CONTROL40_CDC_ANC1_ANC_DMIC_X0P5_B_SEL_LSB_POSN = (int)0,
   ANC_CONTROL40_CDC_ANC1_ANC_DMIC_X0P5_B_SEL_MSB_POSN = (int)0,
   CDC_ANC1_ANC_DMIC_X0P5_A_SEL_POSN                  = (int)1,
   ANC_CONTROL40_CDC_ANC1_ANC_DMIC_X0P5_A_SEL_LSB_POSN = (int)1,
   ANC_CONTROL40_CDC_ANC1_ANC_DMIC_X0P5_A_SEL_MSB_POSN = (int)1,
   CDC_ANC1_ANC_FB_TUNE_DSM_EN_POSN                   = (int)2,
   ANC_CONTROL40_CDC_ANC1_ANC_FB_TUNE_DSM_EN_LSB_POSN = (int)2,
   ANC_CONTROL40_CDC_ANC1_ANC_FB_TUNE_DSM_EN_MSB_POSN = (int)2,
   CDC_ANC1_ANC_FB_TUNE_DSM_CLR_POSN                  = (int)3,
   ANC_CONTROL40_CDC_ANC1_ANC_FB_TUNE_DSM_CLR_LSB_POSN = (int)3,
   ANC_CONTROL40_CDC_ANC1_ANC_FB_TUNE_DSM_CLR_MSB_POSN = (int)3
};
typedef enum anc_control40_posn_enum anc_control40_posn;

#define CDC_ANC1_ANC_DMIC_X0P5_B_SEL_MASK        (0x00000001u)
#define CDC_ANC1_ANC_DMIC_X0P5_A_SEL_MASK        (0x00000002u)
#define CDC_ANC1_ANC_FB_TUNE_DSM_EN_MASK         (0x00000004u)
#define CDC_ANC1_ANC_FB_TUNE_DSM_CLR_MASK        (0x00000008u)

enum anc_control9_posn_enum
{
   CDC_ANC0_VOLUME_EN_POSN                            = (int)0,
   ANC_CONTROL9_CDC_ANC0_VOLUME_EN_LSB_POSN           = (int)0,
   ANC_CONTROL9_CDC_ANC0_VOLUME_EN_MSB_POSN           = (int)0,
   CDC_ANC0_VOLUME_SHIFT_MAIN_LSB_POSN                = (int)1,
   ANC_CONTROL9_CDC_ANC0_VOLUME_SHIFT_MAIN_LSB_POSN   = (int)1,
   CDC_ANC0_VOLUME_SHIFT_MAIN_MSB_POSN                = (int)2,
   ANC_CONTROL9_CDC_ANC0_VOLUME_SHIFT_MAIN_MSB_POSN   = (int)2,
   CDC_ANC0_VOLUME_SHIFT_1_LSB_POSN                   = (int)3,
   ANC_CONTROL9_CDC_ANC0_VOLUME_SHIFT_1_LSB_POSN      = (int)3,
   CDC_ANC0_VOLUME_SHIFT_1_MSB_POSN                   = (int)4,
   ANC_CONTROL9_CDC_ANC0_VOLUME_SHIFT_1_MSB_POSN      = (int)4,
   CDC_ANC0_VOLUME_SHIFT_2_LSB_POSN                   = (int)5,
   ANC_CONTROL9_CDC_ANC0_VOLUME_SHIFT_2_LSB_POSN      = (int)5,
   CDC_ANC0_VOLUME_SHIFT_2_MSB_POSN                   = (int)6,
   ANC_CONTROL9_CDC_ANC0_VOLUME_SHIFT_2_MSB_POSN      = (int)6
};
typedef enum anc_control9_posn_enum anc_control9_posn;

#define CDC_ANC0_VOLUME_EN_MASK                  (0x00000001u)
#define CDC_ANC0_VOLUME_SHIFT_MAIN_LSB_MASK      (0x00000002u)
#define CDC_ANC0_VOLUME_SHIFT_MAIN_MSB_MASK      (0x00000004u)
#define CDC_ANC0_VOLUME_SHIFT_1_LSB_MASK         (0x00000008u)
#define CDC_ANC0_VOLUME_SHIFT_1_MSB_MASK         (0x00000010u)
#define CDC_ANC0_VOLUME_SHIFT_2_LSB_MASK         (0x00000020u)
#define CDC_ANC0_VOLUME_SHIFT_2_MSB_MASK         (0x00000040u)

enum anc_status0_posn_enum
{
   CDC_ANC0_FFa_GAIN_UPDATED_POSN                     = (int)0,
   ANC_STATUS0_CDC_ANC0_FFa_GAIN_UPDATED_LSB_POSN     = (int)0,
   ANC_STATUS0_CDC_ANC0_FFa_GAIN_UPDATED_MSB_POSN     = (int)0,
   CDC_ANC0_FFb_GAIN_UPDATED_POSN                     = (int)1,
   ANC_STATUS0_CDC_ANC0_FFb_GAIN_UPDATED_LSB_POSN     = (int)1,
   ANC_STATUS0_CDC_ANC0_FFb_GAIN_UPDATED_MSB_POSN     = (int)1,
   CDC_ANC0_FB_GAIN_UPDATED_POSN                      = (int)2,
   ANC_STATUS0_CDC_ANC0_FB_GAIN_UPDATED_LSB_POSN      = (int)2,
   ANC_STATUS0_CDC_ANC0_FB_GAIN_UPDATED_MSB_POSN      = (int)2
};
typedef enum anc_status0_posn_enum anc_status0_posn;

#define CDC_ANC0_FFa_GAIN_UPDATED_MASK           (0x00000001u)
#define CDC_ANC0_FFb_GAIN_UPDATED_MASK           (0x00000002u)
#define CDC_ANC0_FB_GAIN_UPDATED_MASK            (0x00000004u)

enum anc_status1_posn_enum
{
   CDC_ANC1_FFa_GAIN_UPDATED_POSN                     = (int)0,
   ANC_STATUS1_CDC_ANC1_FFa_GAIN_UPDATED_LSB_POSN     = (int)0,
   ANC_STATUS1_CDC_ANC1_FFa_GAIN_UPDATED_MSB_POSN     = (int)0,
   CDC_ANC1_FFb_GAIN_UPDATED_POSN                     = (int)1,
   ANC_STATUS1_CDC_ANC1_FFb_GAIN_UPDATED_LSB_POSN     = (int)1,
   ANC_STATUS1_CDC_ANC1_FFb_GAIN_UPDATED_MSB_POSN     = (int)1,
   CDC_ANC1_FB_GAIN_UPDATED_POSN                      = (int)2,
   ANC_STATUS1_CDC_ANC1_FB_GAIN_UPDATED_LSB_POSN      = (int)2,
   ANC_STATUS1_CDC_ANC1_FB_GAIN_UPDATED_MSB_POSN      = (int)2
};
typedef enum anc_status1_posn_enum anc_status1_posn;

#define CDC_ANC1_FFa_GAIN_UPDATED_MASK           (0x00000001u)
#define CDC_ANC1_FFb_GAIN_UPDATED_MASK           (0x00000002u)
#define CDC_ANC1_FB_GAIN_UPDATED_MASK            (0x00000004u)

enum cdc_anc0_iir_coeff_1_ctl_posn_enum
{
   CDC_ANC0_IIR_COEFF_1_CTL_LSB_POSN                  = (int)0,
   CDC_ANC0_IIR_COEFF_1_CTL_CDC_ANC0_IIR_COEFF_1_CTL_LSB_POSN = (int)0,
   CDC_ANC0_IIR_COEFF_1_CTL_MSB_POSN                  = (int)7,
   CDC_ANC0_IIR_COEFF_1_CTL_CDC_ANC0_IIR_COEFF_1_CTL_MSB_POSN = (int)7,
   CDC_ANC0_COEF_PTR_LSB_POSN                         = (int)0,
   CDC_ANC0_IIR_COEFF_1_CTL_CDC_ANC0_COEF_PTR_LSB_POSN = (int)0,
   CDC_ANC0_COEF_PTR_MSB_POSN                         = (int)6,
   CDC_ANC0_IIR_COEFF_1_CTL_CDC_ANC0_COEF_PTR_MSB_POSN = (int)6,
   CDC_ANC0_AUTO_INC_POSN                             = (int)7,
   CDC_ANC0_IIR_COEFF_1_CTL_CDC_ANC0_AUTO_INC_LSB_POSN = (int)7,
   CDC_ANC0_IIR_COEFF_1_CTL_CDC_ANC0_AUTO_INC_MSB_POSN = (int)7
};
typedef enum cdc_anc0_iir_coeff_1_ctl_posn_enum cdc_anc0_iir_coeff_1_ctl_posn;

#define CDC_ANC0_IIR_COEFF_1_CTL_LSB_MASK        (0x00000001u)
#define CDC_ANC0_IIR_COEFF_1_CTL_MSB_MASK        (0x00000080u)
#define CDC_ANC0_COEF_PTR_LSB_MASK               (0x00000001u)
#define CDC_ANC0_COEF_PTR_MSB_MASK               (0x00000040u)
#define CDC_ANC0_AUTO_INC_MASK                   (0x00000080u)

enum cdc_anc0_iir_coeff_2_ctl_posn_enum
{
   CDC_ANC0_IIR_COEFF_2_CTL_LSB_POSN                  = (int)0,
   CDC_ANC0_IIR_COEFF_2_CTL_CDC_ANC0_IIR_COEFF_2_CTL_LSB_POSN = (int)0,
   CDC_ANC0_IIR_COEFF_2_CTL_MSB_POSN                  = (int)7,
   CDC_ANC0_IIR_COEFF_2_CTL_CDC_ANC0_IIR_COEFF_2_CTL_MSB_POSN = (int)7,
   CDC_ANC0_COEF_LSB_POSN                             = (int)0,
   CDC_ANC0_IIR_COEFF_2_CTL_CDC_ANC0_COEF_LSB_POSN    = (int)0,
   CDC_ANC0_COEF_MSB_POSN                             = (int)7,
   CDC_ANC0_IIR_COEFF_2_CTL_CDC_ANC0_COEF_MSB_POSN    = (int)7
};
typedef enum cdc_anc0_iir_coeff_2_ctl_posn_enum cdc_anc0_iir_coeff_2_ctl_posn;

#define CDC_ANC0_IIR_COEFF_2_CTL_LSB_MASK        (0x00000001u)
#define CDC_ANC0_IIR_COEFF_2_CTL_MSB_MASK        (0x00000080u)
#define CDC_ANC0_COEF_LSB_MASK                   (0x00000001u)
#define CDC_ANC0_COEF_MSB_MASK                   (0x00000080u)

enum cdc_anc1_iir_coeff_1_ctl_posn_enum
{
   CDC_ANC1_IIR_COEFF_1_CTL_LSB_POSN                  = (int)0,
   CDC_ANC1_IIR_COEFF_1_CTL_CDC_ANC1_IIR_COEFF_1_CTL_LSB_POSN = (int)0,
   CDC_ANC1_IIR_COEFF_1_CTL_MSB_POSN                  = (int)7,
   CDC_ANC1_IIR_COEFF_1_CTL_CDC_ANC1_IIR_COEFF_1_CTL_MSB_POSN = (int)7,
   CDC_ANC1_COEF_PTR_LSB_POSN                         = (int)0,
   CDC_ANC1_IIR_COEFF_1_CTL_CDC_ANC1_COEF_PTR_LSB_POSN = (int)0,
   CDC_ANC1_COEF_PTR_MSB_POSN                         = (int)6,
   CDC_ANC1_IIR_COEFF_1_CTL_CDC_ANC1_COEF_PTR_MSB_POSN = (int)6,
   CDC_ANC1_AUTO_INC_POSN                             = (int)7,
   CDC_ANC1_IIR_COEFF_1_CTL_CDC_ANC1_AUTO_INC_LSB_POSN = (int)7,
   CDC_ANC1_IIR_COEFF_1_CTL_CDC_ANC1_AUTO_INC_MSB_POSN = (int)7
};
typedef enum cdc_anc1_iir_coeff_1_ctl_posn_enum cdc_anc1_iir_coeff_1_ctl_posn;

#define CDC_ANC1_IIR_COEFF_1_CTL_LSB_MASK        (0x00000001u)
#define CDC_ANC1_IIR_COEFF_1_CTL_MSB_MASK        (0x00000080u)
#define CDC_ANC1_COEF_PTR_LSB_MASK               (0x00000001u)
#define CDC_ANC1_COEF_PTR_MSB_MASK               (0x00000040u)
#define CDC_ANC1_AUTO_INC_MASK                   (0x00000080u)

enum cdc_anc1_iir_coeff_2_ctl_posn_enum
{
   CDC_ANC1_IIR_COEFF_2_CTL_LSB_POSN                  = (int)0,
   CDC_ANC1_IIR_COEFF_2_CTL_CDC_ANC1_IIR_COEFF_2_CTL_LSB_POSN = (int)0,
   CDC_ANC1_IIR_COEFF_2_CTL_MSB_POSN                  = (int)7,
   CDC_ANC1_IIR_COEFF_2_CTL_CDC_ANC1_IIR_COEFF_2_CTL_MSB_POSN = (int)7,
   CDC_ANC1_COEF_LSB_POSN                             = (int)0,
   CDC_ANC1_IIR_COEFF_2_CTL_CDC_ANC1_COEF_LSB_POSN    = (int)0,
   CDC_ANC1_COEF_MSB_POSN                             = (int)7,
   CDC_ANC1_IIR_COEFF_2_CTL_CDC_ANC1_COEF_MSB_POSN    = (int)7
};
typedef enum cdc_anc1_iir_coeff_2_ctl_posn_enum cdc_anc1_iir_coeff_2_ctl_posn;

#define CDC_ANC1_IIR_COEFF_2_CTL_LSB_MASK        (0x00000001u)
#define CDC_ANC1_IIR_COEFF_2_CTL_MSB_MASK        (0x00000080u)
#define CDC_ANC1_COEF_LSB_MASK                   (0x00000001u)
#define CDC_ANC1_COEF_MSB_MASK                   (0x00000080u)

#endif /* IO_DEFS_MODULE_ANC */
#endif /* __IO_DEFS_H__IO_DEFS_MODULE_ANC */







#if !defined(__IO_DEFS_H__IO_DEFS_MODULE_K32_MC_INTER_PROC_KEYHOLE) 
#define __IO_DEFS_H__IO_DEFS_MODULE_K32_MC_INTER_PROC_KEYHOLE

/* -- k32_mc_inter_proc_keyhole -- Kalimba 32-bit Multicore inter-processor communication keyhole register block -- */

enum inter_proc_keyhole_ctrl_posn_enum
{
   INTER_PROC_KEYHOLE_CTRL_BYTE_SEL_LSB_POSN          = (int)0,
   INTER_PROC_KEYHOLE_CTRL_INTER_PROC_KEYHOLE_CTRL_BYTE_SEL_LSB_POSN = (int)0,
   INTER_PROC_KEYHOLE_CTRL_BYTE_SEL_MSB_POSN          = (int)3,
   INTER_PROC_KEYHOLE_CTRL_INTER_PROC_KEYHOLE_CTRL_BYTE_SEL_MSB_POSN = (int)3,
   INTER_PROC_KEYHOLE_CTRL_READ_WRITE_SEL_POSN        = (int)4,
   INTER_PROC_KEYHOLE_CTRL_INTER_PROC_KEYHOLE_CTRL_READ_WRITE_SEL_LSB_POSN = (int)4,
   INTER_PROC_KEYHOLE_CTRL_INTER_PROC_KEYHOLE_CTRL_READ_WRITE_SEL_MSB_POSN = (int)4,
   INTER_PROC_KEYHOLE_CTRL_CPU_SEL_LSB_POSN           = (int)5,
   INTER_PROC_KEYHOLE_CTRL_INTER_PROC_KEYHOLE_CTRL_CPU_SEL_LSB_POSN = (int)5,
   INTER_PROC_KEYHOLE_CTRL_CPU_SEL_MSB_POSN           = (int)6,
   INTER_PROC_KEYHOLE_CTRL_INTER_PROC_KEYHOLE_CTRL_CPU_SEL_MSB_POSN = (int)6,
   INTER_PROC_KEYHOLE_CTRL_PM_DM_SEL_POSN             = (int)7,
   INTER_PROC_KEYHOLE_CTRL_INTER_PROC_KEYHOLE_CTRL_PM_DM_SEL_LSB_POSN = (int)7,
   INTER_PROC_KEYHOLE_CTRL_INTER_PROC_KEYHOLE_CTRL_PM_DM_SEL_MSB_POSN = (int)7
};
typedef enum inter_proc_keyhole_ctrl_posn_enum inter_proc_keyhole_ctrl_posn;

#define INTER_PROC_KEYHOLE_CTRL_BYTE_SEL_LSB_MASK (0x00000001u)
#define INTER_PROC_KEYHOLE_CTRL_BYTE_SEL_MSB_MASK (0x00000008u)
#define INTER_PROC_KEYHOLE_CTRL_READ_WRITE_SEL_MASK (0x00000010u)
#define INTER_PROC_KEYHOLE_CTRL_CPU_SEL_LSB_MASK (0x00000020u)
#define INTER_PROC_KEYHOLE_CTRL_CPU_SEL_MSB_MASK (0x00000040u)
#define INTER_PROC_KEYHOLE_CTRL_PM_DM_SEL_MASK   (0x00000080u)

enum inter_proc_keyhole_ctrl_read_write_sel_enum
{
   INTER_PROC_KEYHOLE_CTRL_READ             = (int)0x0,
   INTER_PROC_KEYHOLE_CTRL_WRITE            = (int)0x1,
   MAX_INTER_PROC_KEYHOLE_CTRL_READ_WRITE_SEL = (int)0x1
};
#define NUM_INTER_PROC_KEYHOLE_CTRL_READ_WRITE_SEL (0x2)
typedef enum inter_proc_keyhole_ctrl_read_write_sel_enum inter_proc_keyhole_ctrl_read_write_sel;


enum inter_proc_keyhole_ctrl_pm_dm_sel_enum
{
   INTER_PROC_KEYHOLE_CTRL_DM               = (int)0x0,
   INTER_PROC_KEYHOLE_CTRL_PM               = (int)0x1,
   MAX_INTER_PROC_KEYHOLE_CTRL_PM_DM_SEL    = (int)0x1
};
#define NUM_INTER_PROC_KEYHOLE_CTRL_PM_DM_SEL (0x2)
typedef enum inter_proc_keyhole_ctrl_pm_dm_sel_enum inter_proc_keyhole_ctrl_pm_dm_sel;


enum inter_proc_keyhole_status_enum
{
   INTER_PROC_KEYHOLE_FREE                  = (int)0x0,
   INTER_PROC_KEYHOLE_BUSY                  = (int)0x1,
   MAX_INTER_PROC_KEYHOLE_STATUS            = (int)0x1
};
#define NUM_INTER_PROC_KEYHOLE_STATUS (0x2)
typedef enum inter_proc_keyhole_status_enum inter_proc_keyhole_status;


enum inter_proc_keyhole_addr_posn_enum
{
   INTER_PROC_KEYHOLE_ADDR_LSB_POSN                   = (int)0,
   INTER_PROC_KEYHOLE_ADDR_INTER_PROC_KEYHOLE_ADDR_LSB_POSN = (int)0,
   INTER_PROC_KEYHOLE_ADDR_MSB_POSN                   = (int)31,
   INTER_PROC_KEYHOLE_ADDR_INTER_PROC_KEYHOLE_ADDR_MSB_POSN = (int)31
};
typedef enum inter_proc_keyhole_addr_posn_enum inter_proc_keyhole_addr_posn;

#define INTER_PROC_KEYHOLE_ADDR_LSB_MASK         (0x00000001u)
#define INTER_PROC_KEYHOLE_ADDR_MSB_MASK         (0x80000000u)

enum inter_proc_keyhole_read_data_posn_enum
{
   INTER_PROC_KEYHOLE_READ_DATA_LSB_POSN              = (int)0,
   INTER_PROC_KEYHOLE_READ_DATA_INTER_PROC_KEYHOLE_READ_DATA_LSB_POSN = (int)0,
   INTER_PROC_KEYHOLE_READ_DATA_MSB_POSN              = (int)31,
   INTER_PROC_KEYHOLE_READ_DATA_INTER_PROC_KEYHOLE_READ_DATA_MSB_POSN = (int)31
};
typedef enum inter_proc_keyhole_read_data_posn_enum inter_proc_keyhole_read_data_posn;

#define INTER_PROC_KEYHOLE_READ_DATA_LSB_MASK    (0x00000001u)
#define INTER_PROC_KEYHOLE_READ_DATA_MSB_MASK    (0x80000000u)

enum inter_proc_keyhole_write_data_posn_enum
{
   INTER_PROC_KEYHOLE_WRITE_DATA_LSB_POSN             = (int)0,
   INTER_PROC_KEYHOLE_WRITE_DATA_INTER_PROC_KEYHOLE_WRITE_DATA_LSB_POSN = (int)0,
   INTER_PROC_KEYHOLE_WRITE_DATA_MSB_POSN             = (int)31,
   INTER_PROC_KEYHOLE_WRITE_DATA_INTER_PROC_KEYHOLE_WRITE_DATA_MSB_POSN = (int)31
};
typedef enum inter_proc_keyhole_write_data_posn_enum inter_proc_keyhole_write_data_posn;

#define INTER_PROC_KEYHOLE_WRITE_DATA_LSB_MASK   (0x00000001u)
#define INTER_PROC_KEYHOLE_WRITE_DATA_MSB_MASK   (0x80000000u)

enum k32_mc_inter_proc_keyhole__access_ctrl_enum_posn_enum
{
   K32_MC_INTER_PROC_KEYHOLE__P0_ACCESS_PERMISSION_POSN = (int)0,
   K32_MC_INTER_PROC_KEYHOLE__ACCESS_CTRL_ENUM_K32_MC_INTER_PROC_KEYHOLE__P0_ACCESS_PERMISSION_LSB_POSN = (int)0,
   K32_MC_INTER_PROC_KEYHOLE__ACCESS_CTRL_ENUM_K32_MC_INTER_PROC_KEYHOLE__P0_ACCESS_PERMISSION_MSB_POSN = (int)0,
   K32_MC_INTER_PROC_KEYHOLE__P1_ACCESS_PERMISSION_POSN = (int)1,
   K32_MC_INTER_PROC_KEYHOLE__ACCESS_CTRL_ENUM_K32_MC_INTER_PROC_KEYHOLE__P1_ACCESS_PERMISSION_LSB_POSN = (int)1,
   K32_MC_INTER_PROC_KEYHOLE__ACCESS_CTRL_ENUM_K32_MC_INTER_PROC_KEYHOLE__P1_ACCESS_PERMISSION_MSB_POSN = (int)1,
   K32_MC_INTER_PROC_KEYHOLE__P2_ACCESS_PERMISSION_POSN = (int)2,
   K32_MC_INTER_PROC_KEYHOLE__ACCESS_CTRL_ENUM_K32_MC_INTER_PROC_KEYHOLE__P2_ACCESS_PERMISSION_LSB_POSN = (int)2,
   K32_MC_INTER_PROC_KEYHOLE__ACCESS_CTRL_ENUM_K32_MC_INTER_PROC_KEYHOLE__P2_ACCESS_PERMISSION_MSB_POSN = (int)2,
   K32_MC_INTER_PROC_KEYHOLE__P3_ACCESS_PERMISSION_POSN = (int)3,
   K32_MC_INTER_PROC_KEYHOLE__ACCESS_CTRL_ENUM_K32_MC_INTER_PROC_KEYHOLE__P3_ACCESS_PERMISSION_LSB_POSN = (int)3,
   K32_MC_INTER_PROC_KEYHOLE__ACCESS_CTRL_ENUM_K32_MC_INTER_PROC_KEYHOLE__P3_ACCESS_PERMISSION_MSB_POSN = (int)3
};
typedef enum k32_mc_inter_proc_keyhole__access_ctrl_enum_posn_enum k32_mc_inter_proc_keyhole__access_ctrl_enum_posn;

#define K32_MC_INTER_PROC_KEYHOLE__P0_ACCESS_PERMISSION_MASK (0x00000001u)
#define K32_MC_INTER_PROC_KEYHOLE__P1_ACCESS_PERMISSION_MASK (0x00000002u)
#define K32_MC_INTER_PROC_KEYHOLE__P2_ACCESS_PERMISSION_MASK (0x00000004u)
#define K32_MC_INTER_PROC_KEYHOLE__P3_ACCESS_PERMISSION_MASK (0x00000008u)

enum k32_mc_inter_proc_keyhole__p0_access_permission_enum
{
   K32_MC_INTER_PROC_KEYHOLE__P0_ACCESS_BLOCKED = (int)0x0,
   K32_MC_INTER_PROC_KEYHOLE__P0_ACCESS_UNBLOCKED = (int)0x1,
   MAX_K32_MC_INTER_PROC_KEYHOLE__P0_ACCESS_PERMISSION = (int)0x1
};
#define NUM_K32_MC_INTER_PROC_KEYHOLE__P0_ACCESS_PERMISSION (0x2)
typedef enum k32_mc_inter_proc_keyhole__p0_access_permission_enum k32_mc_inter_proc_keyhole__p0_access_permission;


enum k32_mc_inter_proc_keyhole__p1_access_permission_enum
{
   K32_MC_INTER_PROC_KEYHOLE__P1_ACCESS_BLOCKED = (int)0x0,
   K32_MC_INTER_PROC_KEYHOLE__P1_ACCESS_UNBLOCKED = (int)0x1,
   MAX_K32_MC_INTER_PROC_KEYHOLE__P1_ACCESS_PERMISSION = (int)0x1
};
#define NUM_K32_MC_INTER_PROC_KEYHOLE__P1_ACCESS_PERMISSION (0x2)
typedef enum k32_mc_inter_proc_keyhole__p1_access_permission_enum k32_mc_inter_proc_keyhole__p1_access_permission;


enum k32_mc_inter_proc_keyhole__p2_access_permission_enum
{
   K32_MC_INTER_PROC_KEYHOLE__P2_ACCESS_BLOCKED = (int)0x0,
   K32_MC_INTER_PROC_KEYHOLE__P2_ACCESS_UNBLOCKED = (int)0x1,
   MAX_K32_MC_INTER_PROC_KEYHOLE__P2_ACCESS_PERMISSION = (int)0x1
};
#define NUM_K32_MC_INTER_PROC_KEYHOLE__P2_ACCESS_PERMISSION (0x2)
typedef enum k32_mc_inter_proc_keyhole__p2_access_permission_enum k32_mc_inter_proc_keyhole__p2_access_permission;


enum k32_mc_inter_proc_keyhole__p3_access_permission_enum
{
   K32_MC_INTER_PROC_KEYHOLE__P3_ACCESS_BLOCKED = (int)0x0,
   K32_MC_INTER_PROC_KEYHOLE__P3_ACCESS_UNBLOCKED = (int)0x1,
   MAX_K32_MC_INTER_PROC_KEYHOLE__P3_ACCESS_PERMISSION = (int)0x1
};
#define NUM_K32_MC_INTER_PROC_KEYHOLE__P3_ACCESS_PERMISSION (0x2)
typedef enum k32_mc_inter_proc_keyhole__p3_access_permission_enum k32_mc_inter_proc_keyhole__p3_access_permission;


enum k32_mc_inter_proc_keyhole__mutex_lock_enum_enum
{
   K32_MC_INTER_PROC_KEYHOLE__MUTEX_AVAILABLE = (int)0x0,
   K32_MC_INTER_PROC_KEYHOLE__MUTEX_CLAIMED_BY_P0 = (int)0x1,
   K32_MC_INTER_PROC_KEYHOLE__MUTEX_CLAIMED_BY_P1 = (int)0x2,
   K32_MC_INTER_PROC_KEYHOLE__MUTEX_CLAIMED_BY_P2 = (int)0x4,
   K32_MC_INTER_PROC_KEYHOLE__MUTEX_CLAIMED_BY_P3 = (int)0x8,
   K32_MC_INTER_PROC_KEYHOLE__MUTEX_DISABLED = (int)0xF,
   MAX_K32_MC_INTER_PROC_KEYHOLE__MUTEX_LOCK_ENUM = (int)0xF
};
typedef enum k32_mc_inter_proc_keyhole__mutex_lock_enum_enum k32_mc_inter_proc_keyhole__mutex_lock_enum;


enum inter_proc_keyhole_access_ctrl_posn_enum
{
   INTER_PROC_KEYHOLE_ACCESS_CTRL_K32_MC_INTER_PROC_KEYHOLE__P0_ACCESS_PERMISSION_LSB_POSN = (int)0,
   INTER_PROC_KEYHOLE_ACCESS_CTRL_K32_MC_INTER_PROC_KEYHOLE__P0_ACCESS_PERMISSION_MSB_POSN = (int)0,
   INTER_PROC_KEYHOLE_ACCESS_CTRL_K32_MC_INTER_PROC_KEYHOLE__P1_ACCESS_PERMISSION_LSB_POSN = (int)1,
   INTER_PROC_KEYHOLE_ACCESS_CTRL_K32_MC_INTER_PROC_KEYHOLE__P1_ACCESS_PERMISSION_MSB_POSN = (int)1,
   INTER_PROC_KEYHOLE_ACCESS_CTRL_K32_MC_INTER_PROC_KEYHOLE__P2_ACCESS_PERMISSION_LSB_POSN = (int)2,
   INTER_PROC_KEYHOLE_ACCESS_CTRL_K32_MC_INTER_PROC_KEYHOLE__P2_ACCESS_PERMISSION_MSB_POSN = (int)2,
   INTER_PROC_KEYHOLE_ACCESS_CTRL_K32_MC_INTER_PROC_KEYHOLE__P3_ACCESS_PERMISSION_LSB_POSN = (int)3,
   INTER_PROC_KEYHOLE_ACCESS_CTRL_K32_MC_INTER_PROC_KEYHOLE__P3_ACCESS_PERMISSION_MSB_POSN = (int)3
};
typedef enum inter_proc_keyhole_access_ctrl_posn_enum inter_proc_keyhole_access_ctrl_posn;


#endif /* IO_DEFS_MODULE_K32_MC_INTER_PROC_KEYHOLE */





#if !defined(__IO_DEFS_H__IO_DEFS_MODULE_CHIP) 
#define __IO_DEFS_H__IO_DEFS_MODULE_CHIP

/* -- chip -- Crescendo chip-level enumerations -- */

enum system_bus_posn_enum
{
   SYSTEM_BUS_HOST_SYS_POSN                           = (int)1,
   SYSTEM_BUS_SYSTEM_BUS_HOST_SYS_LSB_POSN            = (int)1,
   SYSTEM_BUS_SYSTEM_BUS_HOST_SYS_MSB_POSN            = (int)1,
   SYSTEM_BUS_BT_SYS_POSN                             = (int)2,
   SYSTEM_BUS_SYSTEM_BUS_BT_SYS_LSB_POSN              = (int)2,
   SYSTEM_BUS_SYSTEM_BUS_BT_SYS_MSB_POSN              = (int)2,
   SYSTEM_BUS_APPS_SYS_POSN                           = (int)4,
   SYSTEM_BUS_SYSTEM_BUS_APPS_SYS_LSB_POSN            = (int)4,
   SYSTEM_BUS_SYSTEM_BUS_APPS_SYS_MSB_POSN            = (int)4,
   SYSTEM_BUS_CURATOR_POSN                            = (int)0,
   SYSTEM_BUS_SYSTEM_BUS_CURATOR_LSB_POSN             = (int)0,
   SYSTEM_BUS_SYSTEM_BUS_CURATOR_MSB_POSN             = (int)0,
   SYSTEM_BUS_PIO_CTRL_SYS_POSN                       = (int)5,
   SYSTEM_BUS_SYSTEM_BUS_PIO_CTRL_SYS_LSB_POSN        = (int)5,
   SYSTEM_BUS_SYSTEM_BUS_PIO_CTRL_SYS_MSB_POSN        = (int)5,
   SYSTEM_BUS_AUDIO_SYS_POSN                          = (int)3,
   SYSTEM_BUS_SYSTEM_BUS_AUDIO_SYS_LSB_POSN           = (int)3,
   SYSTEM_BUS_SYSTEM_BUS_AUDIO_SYS_MSB_POSN           = (int)3
};
typedef enum system_bus_posn_enum system_bus_posn;

#define SYSTEM_BUS_HOST_SYS_MASK                 (0x00000002u)
#define SYSTEM_BUS_BT_SYS_MASK                   (0x00000004u)
#define SYSTEM_BUS_APPS_SYS_MASK                 (0x00000010u)
#define SYSTEM_BUS_CURATOR_MASK                  (0x00000001u)
#define SYSTEM_BUS_PIO_CTRL_SYS_MASK             (0x00000020u)
#define SYSTEM_BUS_AUDIO_SYS_MASK                (0x00000008u)

enum system_bus_enum
{
   SYSTEM_BUS_HOST_SYS                      = (int)0x1,
   SYSTEM_BUS_BT_SYS                        = (int)0x2,
   SYSTEM_BUS_APPS_SYS                      = (int)0x4,
   SYSTEM_BUS_CURATOR                       = (int)0x0,
   SYSTEM_BUS_PIO_CTRL_SYS                  = (int)0x5,
   SYSTEM_BUS_AUDIO_SYS                     = (int)0x3,
   SYSTEM_BUS_NUM_SUBSYSTEMS                = (int)0x6,
   SYSTEM_BUS_RANGE_LSB                     = (int)0x0,
   SYSTEM_BUS_RANGE_MSB                     = (int)0x5,
   MAX_SYSTEM_BUS                           = (int)0x6
};
typedef enum system_bus_enum system_bus;


enum deep_sleep_wakeup_posn_enum
{
   DEEP_SLEEP_WAKEUP_TIMER_POSN                       = (int)0,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_TIMER_LSB_POSN = (int)0,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_TIMER_MSB_POSN = (int)0,
   DEEP_SLEEP_WAKEUP_XTAL_POSN                        = (int)1,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_XTAL_LSB_POSN  = (int)1,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_XTAL_MSB_POSN  = (int)1,
   DEEP_SLEEP_WAKEUP_SPIB_POSN                        = (int)2,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_SPIB_LSB_POSN  = (int)2,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_SPIB_MSB_POSN  = (int)2,
   DEEP_SLEEP_WAKEUP_PIO_POSN                         = (int)3,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_PIO_LSB_POSN   = (int)3,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_PIO_MSB_POSN   = (int)3,
   DEEP_SLEEP_WAKEUP_TBRIDGE2_POSN                    = (int)11,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_TBRIDGE2_LSB_POSN = (int)11,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_TBRIDGE2_MSB_POSN = (int)11,
   DEEP_SLEEP_WAKEUP_PMU_TS_POSN                      = (int)12,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_PMU_TS_LSB_POSN = (int)12,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_PMU_TS_MSB_POSN = (int)12,
   DEEP_SLEEP_WAKEUP_AUDIO_VAD_POSN                   = (int)13,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_AUDIO_VAD_LSB_POSN = (int)13,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_AUDIO_VAD_MSB_POSN = (int)13,
   DEEP_SLEEP_WAKEUP_USB_POSN                         = (int)8,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_USB_LSB_POSN   = (int)8,
   DEEP_SLEEP_WAKEUP_DEEP_SLEEP_WAKEUP_USB_MSB_POSN   = (int)8
};
typedef enum deep_sleep_wakeup_posn_enum deep_sleep_wakeup_posn;

#define DEEP_SLEEP_WAKEUP_TIMER_MASK             (0x00000001u)
#define DEEP_SLEEP_WAKEUP_XTAL_MASK              (0x00000002u)
#define DEEP_SLEEP_WAKEUP_SPIB_MASK              (0x00000004u)
#define DEEP_SLEEP_WAKEUP_PIO_MASK               (0x00000008u)
#define DEEP_SLEEP_WAKEUP_TBRIDGE2_MASK          (0x00000800u)
#define DEEP_SLEEP_WAKEUP_PMU_TS_MASK            (0x00001000u)
#define DEEP_SLEEP_WAKEUP_AUDIO_VAD_MASK         (0x00002000u)
#define DEEP_SLEEP_WAKEUP_USB_MASK               (0x00000100u)

enum deep_sleep_wakeup_enum
{
   DEEP_SLEEP_WAKEUP_TOTAL_NUM_SOURCES      = (int)0xE,
   MAX_DEEP_SLEEP_WAKEUP                    = (int)0xE
};
typedef enum deep_sleep_wakeup_enum deep_sleep_wakeup;


enum scan_config_enum
{
   ATPG_NUM_SHIFT_CLK                       = (int)0x6,
   ATPG_NUM_COMPRESSOR_CHANNEL              = (int)0x5,
   ATPG_NUM_DECOMPRESSOR_CHANNEL            = (int)0x2EE,
   ATPG_NUM_STUCKATONLY_CHANNEL             = (int)0x0,
   MAX_SCAN_CONFIG                          = (int)0x2EE
};
typedef enum scan_config_enum scan_config;


#endif /* IO_DEFS_MODULE_CHIP */





#if defined(IO_DEFS_MODULE_K32_MC_CONTROL) 

#ifndef __IO_DEFS_H__IO_DEFS_MODULE_K32_MC_CONTROL
#define __IO_DEFS_H__IO_DEFS_MODULE_K32_MC_CONTROL

/* -- k32_mc_control -- Kalimba general control registers for various functions that don't relate to the other registers at this level -- */

enum kalimba_debug_select_enum
{
   KALIMBA_DEBUG_SEL_NONE                   = (int)0x0,
   KALIMBA_DEBUG_SEL_TRACE                  = (int)0x1,
   KALIMBA_DEBUG_SEL_P0_PC                  = (int)0x2,
   KALIMBA_DEBUG_SEL_P1_PC                  = (int)0x3,
   KALIMBA_DEBUG_SEL_PM_SEQ0                = (int)0x4,
   KALIMBA_DEBUG_SEL_PM_SEQ1                = (int)0x5,
   KALIMBA_DEBUG_SEL_PM_SEQ2                = (int)0x6,
   KALIMBA_DEBUG_SEL_PM_SEQ3                = (int)0x7,
   KALIMBA_DEBUG_SEL_PM_SEQ4                = (int)0x8,
   KALIMBA_DEBUG_SEL_PM_SEQ5                = (int)0x9,
   KALIMBA_DEBUG_SEL_PM_SEQ6                = (int)0xA,
   KALIMBA_DEBUG_SEL_PM_SEQ7                = (int)0xB,
   KALIMBA_DEBUG_SEL_PM_SEQ8                = (int)0xC,
   KALIMBA_DEBUG_SEL_PM_SEQ9                = (int)0xD,
   KALIMBA_DEBUG_SEL_PM_SEQ10               = (int)0xE,
   KALIMBA_DEBUG_SEL_PM_SEQ11               = (int)0xF,
   KALIMBA_DEBUG_SEL_PM_SEQ12               = (int)0x10,
   KALIMBA_DEBUG_SEL_PM_SEQ13               = (int)0x11,
   KALIMBA_DEBUG_SEL_DM_SEQ0                = (int)0x12,
   KALIMBA_DEBUG_SEL_DM_SEQ1                = (int)0x13,
   KALIMBA_DEBUG_SEL_DM_SEQ2                = (int)0x14,
   KALIMBA_DEBUG_SEL_DM_SEQ3                = (int)0x15,
   KALIMBA_DEBUG_SEL_DM_SEQ4                = (int)0x16,
   KALIMBA_DEBUG_SEL_DM_SEQ5                = (int)0x17,
   KALIMBA_DEBUG_SEL_DM_SEQ6                = (int)0x18,
   KALIMBA_DEBUG_SEL_DM_SEQ7                = (int)0x19,
   KALIMBA_DEBUG_SEL_DM_SEQ8                = (int)0x1A,
   KALIMBA_DEBUG_SEL_DM_SEQ9                = (int)0x1B,
   KALIMBA_DEBUG_SEL_DM_SEQ10               = (int)0x1C,
   KALIMBA_DEBUG_SEL_DM_SEQ11               = (int)0x1D,
   KALIMBA_DEBUG_SEL_DM_SEQ12               = (int)0x1E,
   KALIMBA_DEBUG_SEL_DM_SEQ13               = (int)0x1F,
   MAX_KALIMBA_DEBUG_SELECT                 = (int)0x1F
};
#define NUM_KALIMBA_DEBUG_SELECT (0x20)
typedef enum kalimba_debug_select_enum kalimba_debug_select;


enum k32_mc_control__access_ctrl_enum_posn_enum
{
   K32_MC_CONTROL__P0_ACCESS_PERMISSION_POSN          = (int)0,
   K32_MC_CONTROL__ACCESS_CTRL_ENUM_K32_MC_CONTROL__P0_ACCESS_PERMISSION_LSB_POSN = (int)0,
   K32_MC_CONTROL__ACCESS_CTRL_ENUM_K32_MC_CONTROL__P0_ACCESS_PERMISSION_MSB_POSN = (int)0,
   K32_MC_CONTROL__P1_ACCESS_PERMISSION_POSN          = (int)1,
   K32_MC_CONTROL__ACCESS_CTRL_ENUM_K32_MC_CONTROL__P1_ACCESS_PERMISSION_LSB_POSN = (int)1,
   K32_MC_CONTROL__ACCESS_CTRL_ENUM_K32_MC_CONTROL__P1_ACCESS_PERMISSION_MSB_POSN = (int)1,
   K32_MC_CONTROL__P2_ACCESS_PERMISSION_POSN          = (int)2,
   K32_MC_CONTROL__ACCESS_CTRL_ENUM_K32_MC_CONTROL__P2_ACCESS_PERMISSION_LSB_POSN = (int)2,
   K32_MC_CONTROL__ACCESS_CTRL_ENUM_K32_MC_CONTROL__P2_ACCESS_PERMISSION_MSB_POSN = (int)2,
   K32_MC_CONTROL__P3_ACCESS_PERMISSION_POSN          = (int)3,
   K32_MC_CONTROL__ACCESS_CTRL_ENUM_K32_MC_CONTROL__P3_ACCESS_PERMISSION_LSB_POSN = (int)3,
   K32_MC_CONTROL__ACCESS_CTRL_ENUM_K32_MC_CONTROL__P3_ACCESS_PERMISSION_MSB_POSN = (int)3
};
typedef enum k32_mc_control__access_ctrl_enum_posn_enum k32_mc_control__access_ctrl_enum_posn;

#define K32_MC_CONTROL__P0_ACCESS_PERMISSION_MASK (0x00000001u)
#define K32_MC_CONTROL__P1_ACCESS_PERMISSION_MASK (0x00000002u)
#define K32_MC_CONTROL__P2_ACCESS_PERMISSION_MASK (0x00000004u)
#define K32_MC_CONTROL__P3_ACCESS_PERMISSION_MASK (0x00000008u)

enum k32_mc_control__p0_access_permission_enum
{
   K32_MC_CONTROL__P0_ACCESS_BLOCKED        = (int)0x0,
   K32_MC_CONTROL__P0_ACCESS_UNBLOCKED      = (int)0x1,
   MAX_K32_MC_CONTROL__P0_ACCESS_PERMISSION = (int)0x1
};
#define NUM_K32_MC_CONTROL__P0_ACCESS_PERMISSION (0x2)
typedef enum k32_mc_control__p0_access_permission_enum k32_mc_control__p0_access_permission;


enum k32_mc_control__p1_access_permission_enum
{
   K32_MC_CONTROL__P1_ACCESS_BLOCKED        = (int)0x0,
   K32_MC_CONTROL__P1_ACCESS_UNBLOCKED      = (int)0x1,
   MAX_K32_MC_CONTROL__P1_ACCESS_PERMISSION = (int)0x1
};
#define NUM_K32_MC_CONTROL__P1_ACCESS_PERMISSION (0x2)
typedef enum k32_mc_control__p1_access_permission_enum k32_mc_control__p1_access_permission;


enum k32_mc_control__p2_access_permission_enum
{
   K32_MC_CONTROL__P2_ACCESS_BLOCKED        = (int)0x0,
   K32_MC_CONTROL__P2_ACCESS_UNBLOCKED      = (int)0x1,
   MAX_K32_MC_CONTROL__P2_ACCESS_PERMISSION = (int)0x1
};
#define NUM_K32_MC_CONTROL__P2_ACCESS_PERMISSION (0x2)
typedef enum k32_mc_control__p2_access_permission_enum k32_mc_control__p2_access_permission;


enum k32_mc_control__p3_access_permission_enum
{
   K32_MC_CONTROL__P3_ACCESS_BLOCKED        = (int)0x0,
   K32_MC_CONTROL__P3_ACCESS_UNBLOCKED      = (int)0x1,
   MAX_K32_MC_CONTROL__P3_ACCESS_PERMISSION = (int)0x1
};
#define NUM_K32_MC_CONTROL__P3_ACCESS_PERMISSION (0x2)
typedef enum k32_mc_control__p3_access_permission_enum k32_mc_control__p3_access_permission;


enum k32_mc_control__mutex_lock_enum_enum
{
   K32_MC_CONTROL__MUTEX_AVAILABLE          = (int)0x0,
   K32_MC_CONTROL__MUTEX_CLAIMED_BY_P0      = (int)0x1,
   K32_MC_CONTROL__MUTEX_CLAIMED_BY_P1      = (int)0x2,
   K32_MC_CONTROL__MUTEX_CLAIMED_BY_P2      = (int)0x4,
   K32_MC_CONTROL__MUTEX_CLAIMED_BY_P3      = (int)0x8,
   K32_MC_CONTROL__MUTEX_DISABLED           = (int)0xF,
   MAX_K32_MC_CONTROL__MUTEX_LOCK_ENUM      = (int)0xF
};
typedef enum k32_mc_control__mutex_lock_enum_enum k32_mc_control__mutex_lock_enum;


enum kalimba_control_access_ctrl_posn_enum
{
   KALIMBA_CONTROL_ACCESS_CTRL_K32_MC_CONTROL__P0_ACCESS_PERMISSION_LSB_POSN = (int)0,
   KALIMBA_CONTROL_ACCESS_CTRL_K32_MC_CONTROL__P0_ACCESS_PERMISSION_MSB_POSN = (int)0,
   KALIMBA_CONTROL_ACCESS_CTRL_K32_MC_CONTROL__P1_ACCESS_PERMISSION_LSB_POSN = (int)1,
   KALIMBA_CONTROL_ACCESS_CTRL_K32_MC_CONTROL__P1_ACCESS_PERMISSION_MSB_POSN = (int)1,
   KALIMBA_CONTROL_ACCESS_CTRL_K32_MC_CONTROL__P2_ACCESS_PERMISSION_LSB_POSN = (int)2,
   KALIMBA_CONTROL_ACCESS_CTRL_K32_MC_CONTROL__P2_ACCESS_PERMISSION_MSB_POSN = (int)2,
   KALIMBA_CONTROL_ACCESS_CTRL_K32_MC_CONTROL__P3_ACCESS_PERMISSION_LSB_POSN = (int)3,
   KALIMBA_CONTROL_ACCESS_CTRL_K32_MC_CONTROL__P3_ACCESS_PERMISSION_MSB_POSN = (int)3
};
typedef enum kalimba_control_access_ctrl_posn_enum kalimba_control_access_ctrl_posn;


#endif /* IO_DEFS_MODULE_K32_MC_CONTROL */
#endif /* __IO_DEFS_H__IO_DEFS_MODULE_K32_MC_CONTROL */








#if !defined(__IO_DEFS_H__IO_DEFS_MODULE_K32_CORE) 
#define __IO_DEFS_H__IO_DEFS_MODULE_K32_CORE

/* -- k32_core -- Kalimba 32-bit Core Control registers -- */

enum arithmetic_mode_posn_enum
{
   ADDSUB_SATURATE_ON_OVERFLOW_POSN                   = (int)0,
   ARITHMETIC_MODE_ADDSUB_SATURATE_ON_OVERFLOW_LSB_POSN = (int)0,
   ARITHMETIC_MODE_ADDSUB_SATURATE_ON_OVERFLOW_MSB_POSN = (int)0,
   ARITHMETIC_16BIT_MODE_POSN                         = (int)1,
   ARITHMETIC_MODE_ARITHMETIC_16BIT_MODE_LSB_POSN     = (int)1,
   ARITHMETIC_MODE_ARITHMETIC_16BIT_MODE_MSB_POSN     = (int)1,
   DISABLE_UNBIASED_ROUNDING_POSN                     = (int)2,
   ARITHMETIC_MODE_DISABLE_UNBIASED_ROUNDING_LSB_POSN = (int)2,
   ARITHMETIC_MODE_DISABLE_UNBIASED_ROUNDING_MSB_POSN = (int)2,
   DISABLE_FRAC_MULT_ROUNDING_POSN                    = (int)3,
   ARITHMETIC_MODE_DISABLE_FRAC_MULT_ROUNDING_LSB_POSN = (int)3,
   ARITHMETIC_MODE_DISABLE_FRAC_MULT_ROUNDING_MSB_POSN = (int)3,
   DISABLE_RMAC_STORE_ROUNDING_POSN                   = (int)4,
   ARITHMETIC_MODE_DISABLE_RMAC_STORE_ROUNDING_LSB_POSN = (int)4,
   ARITHMETIC_MODE_DISABLE_RMAC_STORE_ROUNDING_MSB_POSN = (int)4
};
typedef enum arithmetic_mode_posn_enum arithmetic_mode_posn;

#define ADDSUB_SATURATE_ON_OVERFLOW_MASK         (0x00000001u)
#define ARITHMETIC_16BIT_MODE_MASK               (0x00000002u)
#define DISABLE_UNBIASED_ROUNDING_MASK           (0x00000004u)
#define DISABLE_FRAC_MULT_ROUNDING_MASK          (0x00000008u)
#define DISABLE_RMAC_STORE_ROUNDING_MASK         (0x00000010u)

enum bitreverse_addr_posn_enum
{
   BITREVERSE_ADDR_LSB_POSN                           = (int)0,
   BITREVERSE_ADDR_BITREVERSE_ADDR_LSB_POSN           = (int)0,
   BITREVERSE_ADDR_MSB_POSN                           = (int)31,
   BITREVERSE_ADDR_BITREVERSE_ADDR_MSB_POSN           = (int)31
};
typedef enum bitreverse_addr_posn_enum bitreverse_addr_posn;

#define BITREVERSE_ADDR_LSB_MASK                 (0x00000001u)
#define BITREVERSE_ADDR_MSB_MASK                 (0x80000000u)

enum bitreverse_data_posn_enum
{
   BITREVERSE_DATA_LSB_POSN                           = (int)0,
   BITREVERSE_DATA_BITREVERSE_DATA_LSB_POSN           = (int)0,
   BITREVERSE_DATA_MSB_POSN                           = (int)31,
   BITREVERSE_DATA_BITREVERSE_DATA_MSB_POSN           = (int)31
};
typedef enum bitreverse_data_posn_enum bitreverse_data_posn;

#define BITREVERSE_DATA_LSB_MASK                 (0x00000001u)
#define BITREVERSE_DATA_MSB_MASK                 (0x80000000u)

enum bitreverse_data16_posn_enum
{
   BITREVERSE_DATA16_LSB_POSN                         = (int)0,
   BITREVERSE_DATA16_BITREVERSE_DATA16_LSB_POSN       = (int)0,
   BITREVERSE_DATA16_MSB_POSN                         = (int)31,
   BITREVERSE_DATA16_BITREVERSE_DATA16_MSB_POSN       = (int)31
};
typedef enum bitreverse_data16_posn_enum bitreverse_data16_posn;

#define BITREVERSE_DATA16_LSB_MASK               (0x00000001u)
#define BITREVERSE_DATA16_MSB_MASK               (0x80000000u)

enum bitreverse_val_posn_enum
{
   BITREVERSE_VAL_LSB_POSN                            = (int)0,
   BITREVERSE_VAL_BITREVERSE_VAL_LSB_POSN             = (int)0,
   BITREVERSE_VAL_MSB_POSN                            = (int)31,
   BITREVERSE_VAL_BITREVERSE_VAL_MSB_POSN             = (int)31
};
typedef enum bitreverse_val_posn_enum bitreverse_val_posn;

#define BITREVERSE_VAL_LSB_MASK                  (0x00000001u)
#define BITREVERSE_VAL_MSB_MASK                  (0x80000000u)

enum dbg_counters_en_posn_enum
{
   DBG_COUNTERS_EN_POSN                               = (int)0,
   DBG_COUNTERS_EN_DBG_COUNTERS_EN_LSB_POSN           = (int)0,
   DBG_COUNTERS_EN_DBG_COUNTERS_EN_MSB_POSN           = (int)0
};
typedef enum dbg_counters_en_posn_enum dbg_counters_en_posn;

#define DBG_COUNTERS_EN_MASK                     (0x00000001u)

enum frame_pointer_posn_enum
{
   FRAME_POINTER_LSB_POSN                             = (int)0,
   FRAME_POINTER_FRAME_POINTER_LSB_POSN               = (int)0,
   FRAME_POINTER_MSB_POSN                             = (int)31,
   FRAME_POINTER_FRAME_POINTER_MSB_POSN               = (int)31
};
typedef enum frame_pointer_posn_enum frame_pointer_posn;

#define FRAME_POINTER_LSB_MASK                   (0x00000001u)
#define FRAME_POINTER_MSB_MASK                   (0x80000000u)

enum mm_doloop_end_posn_enum
{
   MM_DOLOOP_END_LSB_POSN                             = (int)0,
   MM_DOLOOP_END_MM_DOLOOP_END_LSB_POSN               = (int)0,
   MM_DOLOOP_END_MSB_POSN                             = (int)31,
   MM_DOLOOP_END_MM_DOLOOP_END_MSB_POSN               = (int)31
};
typedef enum mm_doloop_end_posn_enum mm_doloop_end_posn;

#define MM_DOLOOP_END_LSB_MASK                   (0x00000001u)
#define MM_DOLOOP_END_MSB_MASK                   (0x80000000u)

enum mm_doloop_start_posn_enum
{
   MM_DOLOOP_START_LSB_POSN                           = (int)0,
   MM_DOLOOP_START_MM_DOLOOP_START_LSB_POSN           = (int)0,
   MM_DOLOOP_START_MSB_POSN                           = (int)31,
   MM_DOLOOP_START_MM_DOLOOP_START_MSB_POSN           = (int)31
};
typedef enum mm_doloop_start_posn_enum mm_doloop_start_posn;

#define MM_DOLOOP_START_LSB_MASK                 (0x00000001u)
#define MM_DOLOOP_START_MSB_MASK                 (0x80000000u)

enum mm_quotient_posn_enum
{
   MM_QUOTIENT_LSB_POSN                               = (int)0,
   MM_QUOTIENT_MM_QUOTIENT_LSB_POSN                   = (int)0,
   MM_QUOTIENT_MSB_POSN                               = (int)31,
   MM_QUOTIENT_MM_QUOTIENT_MSB_POSN                   = (int)31
};
typedef enum mm_quotient_posn_enum mm_quotient_posn;

#define MM_QUOTIENT_LSB_MASK                     (0x00000001u)
#define MM_QUOTIENT_MSB_MASK                     (0x80000000u)

enum mm_rem_posn_enum
{
   MM_REM_LSB_POSN                                    = (int)0,
   MM_REM_MM_REM_LSB_POSN                             = (int)0,
   MM_REM_MSB_POSN                                    = (int)31,
   MM_REM_MM_REM_MSB_POSN                             = (int)31
};
typedef enum mm_rem_posn_enum mm_rem_posn;

#define MM_REM_LSB_MASK                          (0x00000001u)
#define MM_REM_MSB_MASK                          (0x80000000u)

enum mm_rintlink_posn_enum
{
   MM_RINTLINK_LSB_POSN                               = (int)0,
   MM_RINTLINK_MM_RINTLINK_LSB_POSN                   = (int)0,
   MM_RINTLINK_MSB_POSN                               = (int)31,
   MM_RINTLINK_MM_RINTLINK_MSB_POSN                   = (int)31
};
typedef enum mm_rintlink_posn_enum mm_rintlink_posn;

#define MM_RINTLINK_LSB_MASK                     (0x00000001u)
#define MM_RINTLINK_MSB_MASK                     (0x80000000u)

enum num_core_stalls_posn_enum
{
   NUM_CORE_STALLS_LSB_POSN                           = (int)0,
   NUM_CORE_STALLS_NUM_CORE_STALLS_LSB_POSN           = (int)0,
   NUM_CORE_STALLS_MSB_POSN                           = (int)31,
   NUM_CORE_STALLS_NUM_CORE_STALLS_MSB_POSN           = (int)31
};
typedef enum num_core_stalls_posn_enum num_core_stalls_posn;

#define NUM_CORE_STALLS_LSB_MASK                 (0x00000001u)
#define NUM_CORE_STALLS_MSB_MASK                 (0x80000000u)

enum num_instrs_posn_enum
{
   NUM_INSTRS_LSB_POSN                                = (int)0,
   NUM_INSTRS_NUM_INSTRS_LSB_POSN                     = (int)0,
   NUM_INSTRS_MSB_POSN                                = (int)31,
   NUM_INSTRS_NUM_INSTRS_MSB_POSN                     = (int)31
};
typedef enum num_instrs_posn_enum num_instrs_posn;

#define NUM_INSTRS_LSB_MASK                      (0x00000001u)
#define NUM_INSTRS_MSB_MASK                      (0x80000000u)

enum num_instr_expand_stalls_posn_enum
{
   NUM_INSTR_EXPAND_STALLS_LSB_POSN                   = (int)0,
   NUM_INSTR_EXPAND_STALLS_NUM_INSTR_EXPAND_STALLS_LSB_POSN = (int)0,
   NUM_INSTR_EXPAND_STALLS_MSB_POSN                   = (int)31,
   NUM_INSTR_EXPAND_STALLS_NUM_INSTR_EXPAND_STALLS_MSB_POSN = (int)31
};
typedef enum num_instr_expand_stalls_posn_enum num_instr_expand_stalls_posn;

#define NUM_INSTR_EXPAND_STALLS_LSB_MASK         (0x00000001u)
#define NUM_INSTR_EXPAND_STALLS_MSB_MASK         (0x80000000u)

enum num_mem_access_stalls_posn_enum
{
   NUM_MEM_ACCESS_STALLS_LSB_POSN                     = (int)0,
   NUM_MEM_ACCESS_STALLS_NUM_MEM_ACCESS_STALLS_LSB_POSN = (int)0,
   NUM_MEM_ACCESS_STALLS_MSB_POSN                     = (int)31,
   NUM_MEM_ACCESS_STALLS_NUM_MEM_ACCESS_STALLS_MSB_POSN = (int)31
};
typedef enum num_mem_access_stalls_posn_enum num_mem_access_stalls_posn;

#define NUM_MEM_ACCESS_STALLS_LSB_MASK           (0x00000001u)
#define NUM_MEM_ACCESS_STALLS_MSB_MASK           (0x80000000u)

enum num_run_clks_posn_enum
{
   NUM_RUN_CLKS_LSB_POSN                              = (int)0,
   NUM_RUN_CLKS_NUM_RUN_CLKS_LSB_POSN                 = (int)0,
   NUM_RUN_CLKS_MSB_POSN                              = (int)31,
   NUM_RUN_CLKS_NUM_RUN_CLKS_MSB_POSN                 = (int)31
};
typedef enum num_run_clks_posn_enum num_run_clks_posn;

#define NUM_RUN_CLKS_LSB_MASK                    (0x00000001u)
#define NUM_RUN_CLKS_MSB_MASK                    (0x80000000u)

enum pc_status_posn_enum
{
   PC_STATUS_LSB_POSN                                 = (int)0,
   PC_STATUS_PC_STATUS_LSB_POSN                       = (int)0,
   PC_STATUS_MSB_POSN                                 = (int)31,
   PC_STATUS_PC_STATUS_MSB_POSN                       = (int)31
};
typedef enum pc_status_posn_enum pc_status_posn;

#define PC_STATUS_LSB_MASK                       (0x00000001u)
#define PC_STATUS_MSB_MASK                       (0x80000000u)

enum stack_end_addr_posn_enum
{
   STACK_END_ADDR_LSB_POSN                            = (int)0,
   STACK_END_ADDR_STACK_END_ADDR_LSB_POSN             = (int)0,
   STACK_END_ADDR_MSB_POSN                            = (int)31,
   STACK_END_ADDR_STACK_END_ADDR_MSB_POSN             = (int)31
};
typedef enum stack_end_addr_posn_enum stack_end_addr_posn;

#define STACK_END_ADDR_LSB_MASK                  (0x00000001u)
#define STACK_END_ADDR_MSB_MASK                  (0x80000000u)

enum stack_overflow_pc_posn_enum
{
   STACK_OVERFLOW_PC_LSB_POSN                         = (int)0,
   STACK_OVERFLOW_PC_STACK_OVERFLOW_PC_LSB_POSN       = (int)0,
   STACK_OVERFLOW_PC_MSB_POSN                         = (int)31,
   STACK_OVERFLOW_PC_STACK_OVERFLOW_PC_MSB_POSN       = (int)31
};
typedef enum stack_overflow_pc_posn_enum stack_overflow_pc_posn;

#define STACK_OVERFLOW_PC_LSB_MASK               (0x00000001u)
#define STACK_OVERFLOW_PC_MSB_MASK               (0x80000000u)

enum stack_pointer_posn_enum
{
   STACK_POINTER_LSB_POSN                             = (int)0,
   STACK_POINTER_STACK_POINTER_LSB_POSN               = (int)0,
   STACK_POINTER_MSB_POSN                             = (int)31,
   STACK_POINTER_STACK_POINTER_MSB_POSN               = (int)31
};
typedef enum stack_pointer_posn_enum stack_pointer_posn;

#define STACK_POINTER_LSB_MASK                   (0x00000001u)
#define STACK_POINTER_MSB_MASK                   (0x80000000u)

enum stack_start_addr_posn_enum
{
   STACK_START_ADDR_LSB_POSN                          = (int)0,
   STACK_START_ADDR_STACK_START_ADDR_LSB_POSN         = (int)0,
   STACK_START_ADDR_MSB_POSN                          = (int)31,
   STACK_START_ADDR_STACK_START_ADDR_MSB_POSN         = (int)31
};
typedef enum stack_start_addr_posn_enum stack_start_addr_posn;

#define STACK_START_ADDR_LSB_MASK                (0x00000001u)
#define STACK_START_ADDR_MSB_MASK                (0x80000000u)

enum test_reg_0_posn_enum
{
   TEST_REG_0_LSB_POSN                                = (int)0,
   TEST_REG_0_TEST_REG_0_LSB_POSN                     = (int)0,
   TEST_REG_0_MSB_POSN                                = (int)31,
   TEST_REG_0_TEST_REG_0_MSB_POSN                     = (int)31
};
typedef enum test_reg_0_posn_enum test_reg_0_posn;

#define TEST_REG_0_LSB_MASK                      (0x00000001u)
#define TEST_REG_0_MSB_MASK                      (0x80000000u)

enum test_reg_1_posn_enum
{
   TEST_REG_1_LSB_POSN                                = (int)0,
   TEST_REG_1_TEST_REG_1_LSB_POSN                     = (int)0,
   TEST_REG_1_MSB_POSN                                = (int)31,
   TEST_REG_1_TEST_REG_1_MSB_POSN                     = (int)31
};
typedef enum test_reg_1_posn_enum test_reg_1_posn;

#define TEST_REG_1_LSB_MASK                      (0x00000001u)
#define TEST_REG_1_MSB_MASK                      (0x80000000u)

enum test_reg_2_posn_enum
{
   TEST_REG_2_LSB_POSN                                = (int)0,
   TEST_REG_2_TEST_REG_2_LSB_POSN                     = (int)0,
   TEST_REG_2_MSB_POSN                                = (int)31,
   TEST_REG_2_TEST_REG_2_MSB_POSN                     = (int)31
};
typedef enum test_reg_2_posn_enum test_reg_2_posn;

#define TEST_REG_2_LSB_MASK                      (0x00000001u)
#define TEST_REG_2_MSB_MASK                      (0x80000000u)

enum test_reg_3_posn_enum
{
   TEST_REG_3_LSB_POSN                                = (int)0,
   TEST_REG_3_TEST_REG_3_LSB_POSN                     = (int)0,
   TEST_REG_3_MSB_POSN                                = (int)31,
   TEST_REG_3_TEST_REG_3_MSB_POSN                     = (int)31
};
typedef enum test_reg_3_posn_enum test_reg_3_posn;

#define TEST_REG_3_LSB_MASK                      (0x00000001u)
#define TEST_REG_3_MSB_MASK                      (0x80000000u)

#endif /* IO_DEFS_MODULE_K32_CORE */





#if !defined(__IO_DEFS_H__IO_DEFS_MODULE_K32_MONITOR) 
#define __IO_DEFS_H__IO_DEFS_MODULE_K32_MONITOR

/* -- k32_monitor -- Kalimba 32-bit Monitor Control registers. -- */

enum exception_en_posn_enum
{
   EXCEPTION_EN_BREAK_POSN                            = (int)0,
   EXCEPTION_EN_EXCEPTION_EN_BREAK_LSB_POSN           = (int)0,
   EXCEPTION_EN_EXCEPTION_EN_BREAK_MSB_POSN           = (int)0,
   EXCEPTION_EN_IRQ_POSN                              = (int)1,
   EXCEPTION_EN_EXCEPTION_EN_IRQ_LSB_POSN             = (int)1,
   EXCEPTION_EN_EXCEPTION_EN_IRQ_MSB_POSN             = (int)1
};
typedef enum exception_en_posn_enum exception_en_posn;

#define EXCEPTION_EN_BREAK_MASK                  (0x00000001u)
#define EXCEPTION_EN_IRQ_MASK                    (0x00000002u)

enum exception_type_enum
{
   EXCEPTION_TYPE_NONE                      = (int)0x0,
   EXCEPTION_TYPE_DM1_UNALIGNED_32BIT       = (int)0x1,
   EXCEPTION_TYPE_DM1_UNALIGNED_16BIT       = (int)0x2,
   EXCEPTION_TYPE_DM1_UNMAPPED              = (int)0x3,
   EXCEPTION_TYPE_DM2_UNALIGNED_32BIT       = (int)0x4,
   EXCEPTION_TYPE_DM2_UNALIGNED_16BIT       = (int)0x5,
   EXCEPTION_TYPE_DM2_UNMAPPED              = (int)0x6,
   EXCEPTION_TYPE_PM_UNMAPPED               = (int)0x7,
   EXCEPTION_TYPE_PM_PROG_REGION            = (int)0x8,
   EXCEPTION_TYPE_DM1_PROG_REGION           = (int)0x9,
   EXCEPTION_TYPE_DM2_PROG_REGION           = (int)0xA,
   EXCEPTION_TYPE_STACK_OVERFLOW            = (int)0xB,
   EXCEPTION_TYPE_OTHER                     = (int)0xC,
   EXCEPTION_TYPE_PM_OUT_OF_BOUNDS          = (int)0xD,
   MAX_EXCEPTION_TYPE                       = (int)0xD
};
#define NUM_EXCEPTION_TYPE (0xE)
typedef enum exception_type_enum exception_type;


enum prog_exception_region_enable_posn_enum
{
   PM_PROG_EXCEPTION_REGION_ENABLE_POSN               = (int)0,
   PROG_EXCEPTION_REGION_ENABLE_PM_PROG_EXCEPTION_REGION_ENABLE_LSB_POSN = (int)0,
   PROG_EXCEPTION_REGION_ENABLE_PM_PROG_EXCEPTION_REGION_ENABLE_MSB_POSN = (int)0,
   DM1_PROG_EXCEPTION_REGION_ENABLE_POSN              = (int)1,
   PROG_EXCEPTION_REGION_ENABLE_DM1_PROG_EXCEPTION_REGION_ENABLE_LSB_POSN = (int)1,
   PROG_EXCEPTION_REGION_ENABLE_DM1_PROG_EXCEPTION_REGION_ENABLE_MSB_POSN = (int)1,
   DM2_PROG_EXCEPTION_REGION_ENABLE_POSN              = (int)2,
   PROG_EXCEPTION_REGION_ENABLE_DM2_PROG_EXCEPTION_REGION_ENABLE_LSB_POSN = (int)2,
   PROG_EXCEPTION_REGION_ENABLE_DM2_PROG_EXCEPTION_REGION_ENABLE_MSB_POSN = (int)2,
   PM_PROG_EXCEPTION_OOB_ENABLE_POSN                  = (int)3,
   PROG_EXCEPTION_REGION_ENABLE_PM_PROG_EXCEPTION_OOB_ENABLE_LSB_POSN = (int)3,
   PROG_EXCEPTION_REGION_ENABLE_PM_PROG_EXCEPTION_OOB_ENABLE_MSB_POSN = (int)3
};
typedef enum prog_exception_region_enable_posn_enum prog_exception_region_enable_posn;

#define PM_PROG_EXCEPTION_REGION_ENABLE_MASK     (0x00000001u)
#define DM1_PROG_EXCEPTION_REGION_ENABLE_MASK    (0x00000002u)
#define DM2_PROG_EXCEPTION_REGION_ENABLE_MASK    (0x00000004u)
#define PM_PROG_EXCEPTION_OOB_ENABLE_MASK        (0x00000008u)

enum dm1_prog_exception_region_end_addr_posn_enum
{
   DM1_PROG_EXCEPTION_REGION_END_ADDR_LSB_POSN        = (int)0,
   DM1_PROG_EXCEPTION_REGION_END_ADDR_DM1_PROG_EXCEPTION_REGION_END_ADDR_LSB_POSN = (int)0,
   DM1_PROG_EXCEPTION_REGION_END_ADDR_MSB_POSN        = (int)31,
   DM1_PROG_EXCEPTION_REGION_END_ADDR_DM1_PROG_EXCEPTION_REGION_END_ADDR_MSB_POSN = (int)31
};
typedef enum dm1_prog_exception_region_end_addr_posn_enum dm1_prog_exception_region_end_addr_posn;

#define DM1_PROG_EXCEPTION_REGION_END_ADDR_LSB_MASK (0x00000001u)
#define DM1_PROG_EXCEPTION_REGION_END_ADDR_MSB_MASK (0x80000000u)

enum dm1_prog_exception_region_start_addr_posn_enum
{
   DM1_PROG_EXCEPTION_REGION_START_ADDR_LSB_POSN      = (int)0,
   DM1_PROG_EXCEPTION_REGION_START_ADDR_DM1_PROG_EXCEPTION_REGION_START_ADDR_LSB_POSN = (int)0,
   DM1_PROG_EXCEPTION_REGION_START_ADDR_MSB_POSN      = (int)31,
   DM1_PROG_EXCEPTION_REGION_START_ADDR_DM1_PROG_EXCEPTION_REGION_START_ADDR_MSB_POSN = (int)31
};
typedef enum dm1_prog_exception_region_start_addr_posn_enum dm1_prog_exception_region_start_addr_posn;

#define DM1_PROG_EXCEPTION_REGION_START_ADDR_LSB_MASK (0x00000001u)
#define DM1_PROG_EXCEPTION_REGION_START_ADDR_MSB_MASK (0x80000000u)

enum dm2_prog_exception_region_end_addr_posn_enum
{
   DM2_PROG_EXCEPTION_REGION_END_ADDR_LSB_POSN        = (int)0,
   DM2_PROG_EXCEPTION_REGION_END_ADDR_DM2_PROG_EXCEPTION_REGION_END_ADDR_LSB_POSN = (int)0,
   DM2_PROG_EXCEPTION_REGION_END_ADDR_MSB_POSN        = (int)31,
   DM2_PROG_EXCEPTION_REGION_END_ADDR_DM2_PROG_EXCEPTION_REGION_END_ADDR_MSB_POSN = (int)31
};
typedef enum dm2_prog_exception_region_end_addr_posn_enum dm2_prog_exception_region_end_addr_posn;

#define DM2_PROG_EXCEPTION_REGION_END_ADDR_LSB_MASK (0x00000001u)
#define DM2_PROG_EXCEPTION_REGION_END_ADDR_MSB_MASK (0x80000000u)

enum dm2_prog_exception_region_start_addr_posn_enum
{
   DM2_PROG_EXCEPTION_REGION_START_ADDR_LSB_POSN      = (int)0,
   DM2_PROG_EXCEPTION_REGION_START_ADDR_DM2_PROG_EXCEPTION_REGION_START_ADDR_LSB_POSN = (int)0,
   DM2_PROG_EXCEPTION_REGION_START_ADDR_MSB_POSN      = (int)31,
   DM2_PROG_EXCEPTION_REGION_START_ADDR_DM2_PROG_EXCEPTION_REGION_START_ADDR_MSB_POSN = (int)31
};
typedef enum dm2_prog_exception_region_start_addr_posn_enum dm2_prog_exception_region_start_addr_posn;

#define DM2_PROG_EXCEPTION_REGION_START_ADDR_LSB_MASK (0x00000001u)
#define DM2_PROG_EXCEPTION_REGION_START_ADDR_MSB_MASK (0x80000000u)

enum exception_pc_posn_enum
{
   EXCEPTION_PC_LSB_POSN                              = (int)0,
   EXCEPTION_PC_EXCEPTION_PC_LSB_POSN                 = (int)0,
   EXCEPTION_PC_MSB_POSN                              = (int)31,
   EXCEPTION_PC_EXCEPTION_PC_MSB_POSN                 = (int)31
};
typedef enum exception_pc_posn_enum exception_pc_posn;

#define EXCEPTION_PC_LSB_MASK                    (0x00000001u)
#define EXCEPTION_PC_MSB_MASK                    (0x80000000u)

enum pm_prog_exception_region_end_addr_posn_enum
{
   PM_PROG_EXCEPTION_REGION_END_ADDR_LSB_POSN         = (int)0,
   PM_PROG_EXCEPTION_REGION_END_ADDR_PM_PROG_EXCEPTION_REGION_END_ADDR_LSB_POSN = (int)0,
   PM_PROG_EXCEPTION_REGION_END_ADDR_MSB_POSN         = (int)31,
   PM_PROG_EXCEPTION_REGION_END_ADDR_PM_PROG_EXCEPTION_REGION_END_ADDR_MSB_POSN = (int)31
};
typedef enum pm_prog_exception_region_end_addr_posn_enum pm_prog_exception_region_end_addr_posn;

#define PM_PROG_EXCEPTION_REGION_END_ADDR_LSB_MASK (0x00000001u)
#define PM_PROG_EXCEPTION_REGION_END_ADDR_MSB_MASK (0x80000000u)

enum pm_prog_exception_region_start_addr_posn_enum
{
   PM_PROG_EXCEPTION_REGION_START_ADDR_LSB_POSN       = (int)0,
   PM_PROG_EXCEPTION_REGION_START_ADDR_PM_PROG_EXCEPTION_REGION_START_ADDR_LSB_POSN = (int)0,
   PM_PROG_EXCEPTION_REGION_START_ADDR_MSB_POSN       = (int)31,
   PM_PROG_EXCEPTION_REGION_START_ADDR_PM_PROG_EXCEPTION_REGION_START_ADDR_MSB_POSN = (int)31
};
typedef enum pm_prog_exception_region_start_addr_posn_enum pm_prog_exception_region_start_addr_posn;

#define PM_PROG_EXCEPTION_REGION_START_ADDR_LSB_MASK (0x00000001u)
#define PM_PROG_EXCEPTION_REGION_START_ADDR_MSB_MASK (0x80000000u)

#endif /* IO_DEFS_MODULE_K32_MONITOR */



#if defined(IO_DEFS_MODULE_AOV) 

#ifndef __IO_DEFS_H__IO_DEFS_MODULE_AOV
#define __IO_DEFS_H__IO_DEFS_MODULE_AOV

/* -- aov -- AoV registers -- */

enum aov_events_posn_enum
{
   AOV_EVENT_RAM0_FULL_POSN                           = (int)0,
   AOV_EVENTS_AOV_EVENT_RAM0_FULL_LSB_POSN            = (int)0,
   AOV_EVENTS_AOV_EVENT_RAM0_FULL_MSB_POSN            = (int)0,
   AOV_EVENT_RAM1_FULL_POSN                           = (int)1,
   AOV_EVENTS_AOV_EVENT_RAM1_FULL_LSB_POSN            = (int)1,
   AOV_EVENTS_AOV_EVENT_RAM1_FULL_MSB_POSN            = (int)1,
   AOV_EVENT_RAM0_EMPTY_POSN                          = (int)2,
   AOV_EVENTS_AOV_EVENT_RAM0_EMPTY_LSB_POSN           = (int)2,
   AOV_EVENTS_AOV_EVENT_RAM0_EMPTY_MSB_POSN           = (int)2,
   AOV_EVENT_RAM1_EMPTY_POSN                          = (int)3,
   AOV_EVENTS_AOV_EVENT_RAM1_EMPTY_LSB_POSN           = (int)3,
   AOV_EVENTS_AOV_EVENT_RAM1_EMPTY_MSB_POSN           = (int)3,
   AOV_EVENT_RAM_OVERRUN_POSN                         = (int)4,
   AOV_EVENTS_AOV_EVENT_RAM_OVERRUN_LSB_POSN          = (int)4,
   AOV_EVENTS_AOV_EVENT_RAM_OVERRUN_MSB_POSN          = (int)4,
   AOV_EVENT_VAD_POS_EVENT_POSN                       = (int)5,
   AOV_EVENTS_AOV_EVENT_VAD_POS_EVENT_LSB_POSN        = (int)5,
   AOV_EVENTS_AOV_EVENT_VAD_POS_EVENT_MSB_POSN        = (int)5,
   AOV_EVENT_VAD_NEG_EVENT_POSN                       = (int)6,
   AOV_EVENTS_AOV_EVENT_VAD_NEG_EVENT_LSB_POSN        = (int)6,
   AOV_EVENTS_AOV_EVENT_VAD_NEG_EVENT_MSB_POSN        = (int)6
};
typedef enum aov_events_posn_enum aov_events_posn;

#define AOV_EVENT_RAM0_FULL_MASK                 (0x00000001u)
#define AOV_EVENT_RAM1_FULL_MASK                 (0x00000002u)
#define AOV_EVENT_RAM0_EMPTY_MASK                (0x00000004u)
#define AOV_EVENT_RAM1_EMPTY_MASK                (0x00000008u)
#define AOV_EVENT_RAM_OVERRUN_MASK               (0x00000010u)
#define AOV_EVENT_VAD_POS_EVENT_MASK             (0x00000020u)
#define AOV_EVENT_VAD_NEG_EVENT_MASK             (0x00000040u)

enum aov_ctl_posn_enum
{
   AOV_CTL_ENABLE_POSN                                = (int)0,
   AOV_CTL_AOV_CTL_ENABLE_LSB_POSN                    = (int)0,
   AOV_CTL_AOV_CTL_ENABLE_MSB_POSN                    = (int)0,
   AOV_CTL_VAD_ENABLE_POSN                            = (int)1,
   AOV_CTL_AOV_CTL_VAD_ENABLE_LSB_POSN                = (int)1,
   AOV_CTL_AOV_CTL_VAD_ENABLE_MSB_POSN                = (int)1,
   AOV_CTL_DRAIN_BUFFER_ENABLE_POSN                   = (int)2,
   AOV_CTL_AOV_CTL_DRAIN_BUFFER_ENABLE_LSB_POSN       = (int)2,
   AOV_CTL_AOV_CTL_DRAIN_BUFFER_ENABLE_MSB_POSN       = (int)2,
   AOV_CTL_FREEZE_ON_ERROR_POSN                       = (int)3,
   AOV_CTL_AOV_CTL_FREEZE_ON_ERROR_LSB_POSN           = (int)3,
   AOV_CTL_AOV_CTL_FREEZE_ON_ERROR_MSB_POSN           = (int)3,
   AOV_CTL_CLR_VAD_FIRED_POSN                         = (int)4,
   AOV_CTL_AOV_CTL_CLR_VAD_FIRED_LSB_POSN             = (int)4,
   AOV_CTL_AOV_CTL_CLR_VAD_FIRED_MSB_POSN             = (int)4,
   AOV_CTL_PAUSE_RAM_FILL_POSN                        = (int)5,
   AOV_CTL_AOV_CTL_PAUSE_RAM_FILL_LSB_POSN            = (int)5,
   AOV_CTL_AOV_CTL_PAUSE_RAM_FILL_MSB_POSN            = (int)5,
   AOV_CTL_KCODEC_SEL_LSB_POSN                        = (int)6,
   AOV_CTL_AOV_CTL_KCODEC_SEL_LSB_POSN                = (int)6,
   AOV_CTL_KCODEC_SEL_MSB_POSN                        = (int)8,
   AOV_CTL_AOV_CTL_KCODEC_SEL_MSB_POSN                = (int)8
};
typedef enum aov_ctl_posn_enum aov_ctl_posn;

#define AOV_CTL_ENABLE_MASK                      (0x00000001u)
#define AOV_CTL_VAD_ENABLE_MASK                  (0x00000002u)
#define AOV_CTL_DRAIN_BUFFER_ENABLE_MASK         (0x00000004u)
#define AOV_CTL_FREEZE_ON_ERROR_MASK             (0x00000008u)
#define AOV_CTL_CLR_VAD_FIRED_MASK               (0x00000010u)
#define AOV_CTL_PAUSE_RAM_FILL_MASK              (0x00000020u)
#define AOV_CTL_KCODEC_SEL_LSB_MASK              (0x00000040u)
#define AOV_CTL_KCODEC_SEL_MSB_MASK              (0x00000100u)

enum aov_keyhole_posn_enum
{
   AOV_KEYHOLE_ADDR_LSB_POSN                          = (int)0,
   AOV_KEYHOLE_AOV_KEYHOLE_ADDR_LSB_POSN              = (int)0,
   AOV_KEYHOLE_ADDR_MSB_POSN                          = (int)10,
   AOV_KEYHOLE_AOV_KEYHOLE_ADDR_MSB_POSN              = (int)10,
   AOV_KEYHOLE_FORCE_RAM_SEL_POSN                     = (int)11,
   AOV_KEYHOLE_AOV_KEYHOLE_FORCE_RAM_SEL_LSB_POSN     = (int)11,
   AOV_KEYHOLE_AOV_KEYHOLE_FORCE_RAM_SEL_MSB_POSN     = (int)11,
   AOV_KEYHOLE_RAM_SEL_POSN                           = (int)12,
   AOV_KEYHOLE_AOV_KEYHOLE_RAM_SEL_LSB_POSN           = (int)12,
   AOV_KEYHOLE_AOV_KEYHOLE_RAM_SEL_MSB_POSN           = (int)12
};
typedef enum aov_keyhole_posn_enum aov_keyhole_posn;

#define AOV_KEYHOLE_ADDR_LSB_MASK                (0x00000001u)
#define AOV_KEYHOLE_ADDR_MSB_MASK                (0x00000400u)
#define AOV_KEYHOLE_FORCE_RAM_SEL_MASK           (0x00000800u)
#define AOV_KEYHOLE_RAM_SEL_MASK                 (0x00001000u)

enum aov_rams_ds_en_posn_enum
{
   AOV_RAMS_DS_RAM0_POSN                              = (int)0,
   AOV_RAMS_DS_EN_AOV_RAMS_DS_RAM0_LSB_POSN           = (int)0,
   AOV_RAMS_DS_EN_AOV_RAMS_DS_RAM0_MSB_POSN           = (int)0,
   AOV_RAMS_DS_RAM1_POSN                              = (int)1,
   AOV_RAMS_DS_EN_AOV_RAMS_DS_RAM1_LSB_POSN           = (int)1,
   AOV_RAMS_DS_EN_AOV_RAMS_DS_RAM1_MSB_POSN           = (int)1
};
typedef enum aov_rams_ds_en_posn_enum aov_rams_ds_en_posn;

#define AOV_RAMS_DS_RAM0_MASK                    (0x00000001u)
#define AOV_RAMS_DS_RAM1_MASK                    (0x00000002u)

enum aov_rams_ema_posn_enum
{
   AOV_RAM0_EMAW_LSB_POSN                             = (int)0,
   AOV_RAMS_EMA_AOV_RAM0_EMAW_LSB_POSN                = (int)0,
   AOV_RAM0_EMAW_MSB_POSN                             = (int)1,
   AOV_RAMS_EMA_AOV_RAM0_EMAW_MSB_POSN                = (int)1,
   AOV_RAM0_EMA_LSB_POSN                              = (int)2,
   AOV_RAMS_EMA_AOV_RAM0_EMA_LSB_POSN                 = (int)2,
   AOV_RAM0_EMA_MSB_POSN                              = (int)4,
   AOV_RAMS_EMA_AOV_RAM0_EMA_MSB_POSN                 = (int)4,
   AOV_RAM1_EMAW_LSB_POSN                             = (int)5,
   AOV_RAMS_EMA_AOV_RAM1_EMAW_LSB_POSN                = (int)5,
   AOV_RAM1_EMAW_MSB_POSN                             = (int)6,
   AOV_RAMS_EMA_AOV_RAM1_EMAW_MSB_POSN                = (int)6,
   AOV_RAM1_EMA_LSB_POSN                              = (int)7,
   AOV_RAMS_EMA_AOV_RAM1_EMA_LSB_POSN                 = (int)7,
   AOV_RAM1_EMA_MSB_POSN                              = (int)9,
   AOV_RAMS_EMA_AOV_RAM1_EMA_MSB_POSN                 = (int)9
};
typedef enum aov_rams_ema_posn_enum aov_rams_ema_posn;

#define AOV_RAM0_EMAW_LSB_MASK                   (0x00000001u)
#define AOV_RAM0_EMAW_MSB_MASK                   (0x00000002u)
#define AOV_RAM0_EMA_LSB_MASK                    (0x00000004u)
#define AOV_RAM0_EMA_MSB_MASK                    (0x00000010u)
#define AOV_RAM1_EMAW_LSB_MASK                   (0x00000020u)
#define AOV_RAM1_EMAW_MSB_MASK                   (0x00000040u)
#define AOV_RAM1_EMA_LSB_MASK                    (0x00000080u)
#define AOV_RAM1_EMA_MSB_MASK                    (0x00000200u)

enum aov_rams_ls_en_posn_enum
{
   AOV_RAMS_LS_RAM0_POSN                              = (int)0,
   AOV_RAMS_LS_EN_AOV_RAMS_LS_RAM0_LSB_POSN           = (int)0,
   AOV_RAMS_LS_EN_AOV_RAMS_LS_RAM0_MSB_POSN           = (int)0,
   AOV_RAMS_LS_RAM1_POSN                              = (int)1,
   AOV_RAMS_LS_EN_AOV_RAMS_LS_RAM1_LSB_POSN           = (int)1,
   AOV_RAMS_LS_EN_AOV_RAMS_LS_RAM1_MSB_POSN           = (int)1
};
typedef enum aov_rams_ls_en_posn_enum aov_rams_ls_en_posn;

#define AOV_RAMS_LS_RAM0_MASK                    (0x00000001u)
#define AOV_RAMS_LS_RAM1_MASK                    (0x00000002u)

enum aov_rams_rawl_posn_enum
{
   AOV_RAM0_RAWLM_LSB_POSN                            = (int)0,
   AOV_RAMS_RAWL_AOV_RAM0_RAWLM_LSB_POSN              = (int)0,
   AOV_RAM0_RAWLM_MSB_POSN                            = (int)1,
   AOV_RAMS_RAWL_AOV_RAM0_RAWLM_MSB_POSN              = (int)1,
   AOV_RAM0_RAWL_POSN                                 = (int)2,
   AOV_RAMS_RAWL_AOV_RAM0_RAWL_LSB_POSN               = (int)2,
   AOV_RAMS_RAWL_AOV_RAM0_RAWL_MSB_POSN               = (int)2,
   AOV_RAM1_RAWLM_LSB_POSN                            = (int)3,
   AOV_RAMS_RAWL_AOV_RAM1_RAWLM_LSB_POSN              = (int)3,
   AOV_RAM1_RAWLM_MSB_POSN                            = (int)4,
   AOV_RAMS_RAWL_AOV_RAM1_RAWLM_MSB_POSN              = (int)4,
   AOV_RAM1_RAWL_POSN                                 = (int)5,
   AOV_RAMS_RAWL_AOV_RAM1_RAWL_LSB_POSN               = (int)5,
   AOV_RAMS_RAWL_AOV_RAM1_RAWL_MSB_POSN               = (int)5
};
typedef enum aov_rams_rawl_posn_enum aov_rams_rawl_posn;

#define AOV_RAM0_RAWLM_LSB_MASK                  (0x00000001u)
#define AOV_RAM0_RAWLM_MSB_MASK                  (0x00000002u)
#define AOV_RAM0_RAWL_MASK                       (0x00000004u)
#define AOV_RAM1_RAWLM_LSB_MASK                  (0x00000008u)
#define AOV_RAM1_RAWLM_MSB_MASK                  (0x00000010u)
#define AOV_RAM1_RAWL_MASK                       (0x00000020u)

enum aov_rams_sd_en_posn_enum
{
   AOV_RAMS_SD_RAM0_POSN                              = (int)0,
   AOV_RAMS_SD_EN_AOV_RAMS_SD_RAM0_LSB_POSN           = (int)0,
   AOV_RAMS_SD_EN_AOV_RAMS_SD_RAM0_MSB_POSN           = (int)0,
   AOV_RAMS_SD_RAM1_POSN                              = (int)1,
   AOV_RAMS_SD_EN_AOV_RAMS_SD_RAM1_LSB_POSN           = (int)1,
   AOV_RAMS_SD_EN_AOV_RAMS_SD_RAM1_MSB_POSN           = (int)1
};
typedef enum aov_rams_sd_en_posn_enum aov_rams_sd_en_posn;

#define AOV_RAMS_SD_RAM0_MASK                    (0x00000001u)
#define AOV_RAMS_SD_RAM1_MASK                    (0x00000002u)

enum aov_rams_wabl_posn_enum
{
   AOV_RAM0_WABLM_LSB_POSN                            = (int)0,
   AOV_RAMS_WABL_AOV_RAM0_WABLM_LSB_POSN              = (int)0,
   AOV_RAM0_WABLM_MSB_POSN                            = (int)1,
   AOV_RAMS_WABL_AOV_RAM0_WABLM_MSB_POSN              = (int)1,
   AOV_RAM0_WABL_POSN                                 = (int)2,
   AOV_RAMS_WABL_AOV_RAM0_WABL_LSB_POSN               = (int)2,
   AOV_RAMS_WABL_AOV_RAM0_WABL_MSB_POSN               = (int)2,
   AOV_RAM1_WABLM_LSB_POSN                            = (int)3,
   AOV_RAMS_WABL_AOV_RAM1_WABLM_LSB_POSN              = (int)3,
   AOV_RAM1_WABLM_MSB_POSN                            = (int)4,
   AOV_RAMS_WABL_AOV_RAM1_WABLM_MSB_POSN              = (int)4,
   AOV_RAM1_WABL_POSN                                 = (int)5,
   AOV_RAMS_WABL_AOV_RAM1_WABL_LSB_POSN               = (int)5,
   AOV_RAMS_WABL_AOV_RAM1_WABL_MSB_POSN               = (int)5
};
typedef enum aov_rams_wabl_posn_enum aov_rams_wabl_posn;

#define AOV_RAM0_WABLM_LSB_MASK                  (0x00000001u)
#define AOV_RAM0_WABLM_MSB_MASK                  (0x00000002u)
#define AOV_RAM0_WABL_MASK                       (0x00000004u)
#define AOV_RAM1_WABLM_LSB_MASK                  (0x00000008u)
#define AOV_RAM1_WABLM_MSB_MASK                  (0x00000010u)
#define AOV_RAM1_WABL_MASK                       (0x00000020u)

enum aov_ram_cfg_posn_enum
{
   AOV_RAM_CFG_MAX_ENTRIES_LSB_POSN                   = (int)0,
   AOV_RAM_CFG_AOV_RAM_CFG_MAX_ENTRIES_LSB_POSN       = (int)0,
   AOV_RAM_CFG_MAX_ENTRIES_MSB_POSN                   = (int)10,
   AOV_RAM_CFG_AOV_RAM_CFG_MAX_ENTRIES_MSB_POSN       = (int)10,
   AOV_RAM_CFG_THRESHOLD_LSB_POSN                     = (int)11,
   AOV_RAM_CFG_AOV_RAM_CFG_THRESHOLD_LSB_POSN         = (int)11,
   AOV_RAM_CFG_THRESHOLD_MSB_POSN                     = (int)14,
   AOV_RAM_CFG_AOV_RAM_CFG_THRESHOLD_MSB_POSN         = (int)14,
   AOV_RAM_CFG_READ_LIMIT_LSB_POSN                    = (int)15,
   AOV_RAM_CFG_AOV_RAM_CFG_READ_LIMIT_LSB_POSN        = (int)15,
   AOV_RAM_CFG_READ_LIMIT_MSB_POSN                    = (int)18,
   AOV_RAM_CFG_AOV_RAM_CFG_READ_LIMIT_MSB_POSN        = (int)18,
   AOV_RAM_CFG_STALL_LIMIT_LSB_POSN                   = (int)19,
   AOV_RAM_CFG_AOV_RAM_CFG_STALL_LIMIT_LSB_POSN       = (int)19,
   AOV_RAM_CFG_STALL_LIMIT_MSB_POSN                   = (int)22,
   AOV_RAM_CFG_AOV_RAM_CFG_STALL_LIMIT_MSB_POSN       = (int)22
};
typedef enum aov_ram_cfg_posn_enum aov_ram_cfg_posn;

#define AOV_RAM_CFG_MAX_ENTRIES_LSB_MASK         (0x00000001u)
#define AOV_RAM_CFG_MAX_ENTRIES_MSB_MASK         (0x00000400u)
#define AOV_RAM_CFG_THRESHOLD_LSB_MASK           (0x00000800u)
#define AOV_RAM_CFG_THRESHOLD_MSB_MASK           (0x00004000u)
#define AOV_RAM_CFG_READ_LIMIT_LSB_MASK          (0x00008000u)
#define AOV_RAM_CFG_READ_LIMIT_MSB_MASK          (0x00040000u)
#define AOV_RAM_CFG_STALL_LIMIT_LSB_MASK         (0x00080000u)
#define AOV_RAM_CFG_STALL_LIMIT_MSB_MASK         (0x00400000u)

enum aov_ram_if_status1_posn_enum
{
   RAM0_FULL_POSN                                     = (int)0,
   AOV_RAM_IF_STATUS1_RAM0_FULL_LSB_POSN              = (int)0,
   AOV_RAM_IF_STATUS1_RAM0_FULL_MSB_POSN              = (int)0,
   RAM1_FULL_POSN                                     = (int)1,
   AOV_RAM_IF_STATUS1_RAM1_FULL_LSB_POSN              = (int)1,
   AOV_RAM_IF_STATUS1_RAM1_FULL_MSB_POSN              = (int)1,
   OVERRUN_ERR_POSN                                   = (int)2,
   AOV_RAM_IF_STATUS1_OVERRUN_ERR_LSB_POSN            = (int)2,
   AOV_RAM_IF_STATUS1_OVERRUN_ERR_MSB_POSN            = (int)2,
   RAM_PAUSED_POSN                                    = (int)3,
   AOV_RAM_IF_STATUS1_RAM_PAUSED_LSB_POSN             = (int)3,
   AOV_RAM_IF_STATUS1_RAM_PAUSED_MSB_POSN             = (int)3,
   WRITE_STATE_LSB_POSN                               = (int)4,
   AOV_RAM_IF_STATUS1_WRITE_STATE_LSB_POSN            = (int)4,
   WRITE_STATE_MSB_POSN                               = (int)6,
   AOV_RAM_IF_STATUS1_WRITE_STATE_MSB_POSN            = (int)6,
   READ_STATE_LSB_POSN                                = (int)7,
   AOV_RAM_IF_STATUS1_READ_STATE_LSB_POSN             = (int)7,
   READ_STATE_MSB_POSN                                = (int)8,
   AOV_RAM_IF_STATUS1_READ_STATE_MSB_POSN             = (int)8
};
typedef enum aov_ram_if_status1_posn_enum aov_ram_if_status1_posn;

#define RAM0_FULL_MASK                           (0x00000001u)
#define RAM1_FULL_MASK                           (0x00000002u)
#define OVERRUN_ERR_MASK                         (0x00000004u)
#define RAM_PAUSED_MASK                          (0x00000008u)
#define WRITE_STATE_LSB_MASK                     (0x00000010u)
#define WRITE_STATE_MSB_MASK                     (0x00000040u)
#define READ_STATE_LSB_MASK                      (0x00000080u)
#define READ_STATE_MSB_MASK                      (0x00000100u)

enum aov_ram_if_status2_posn_enum
{
   RAM_WRITE_ADDR_LSB_POSN                            = (int)0,
   AOV_RAM_IF_STATUS2_RAM_WRITE_ADDR_LSB_POSN         = (int)0,
   RAM_WRITE_ADDR_MSB_POSN                            = (int)10,
   AOV_RAM_IF_STATUS2_RAM_WRITE_ADDR_MSB_POSN         = (int)10,
   RAM_WR_SEL_POSN                                    = (int)11,
   AOV_RAM_IF_STATUS2_RAM_WR_SEL_LSB_POSN             = (int)11,
   AOV_RAM_IF_STATUS2_RAM_WR_SEL_MSB_POSN             = (int)11,
   RAM_RD_ADR_LSB_POSN                                = (int)12,
   AOV_RAM_IF_STATUS2_RAM_RD_ADR_LSB_POSN             = (int)12,
   RAM_RD_ADR_MSB_POSN                                = (int)22,
   AOV_RAM_IF_STATUS2_RAM_RD_ADR_MSB_POSN             = (int)22
};
typedef enum aov_ram_if_status2_posn_enum aov_ram_if_status2_posn;

#define RAM_WRITE_ADDR_LSB_MASK                  (0x00000001u)
#define RAM_WRITE_ADDR_MSB_MASK                  (0x00000400u)
#define RAM_WR_SEL_MASK                          (0x00000800u)
#define RAM_RD_ADR_LSB_MASK                      (0x00001000u)
#define RAM_RD_ADR_MSB_MASK                      (0x00400000u)

enum aov_sequencer_active_posn_enum
{
   AOV_RAM0_SEQ_ACT_POSN                              = (int)0,
   AOV_SEQUENCER_ACTIVE_AOV_RAM0_SEQ_ACT_LSB_POSN     = (int)0,
   AOV_SEQUENCER_ACTIVE_AOV_RAM0_SEQ_ACT_MSB_POSN     = (int)0,
   AOV_RAM1_SEQ_ACT_POSN                              = (int)1,
   AOV_SEQUENCER_ACTIVE_AOV_RAM1_SEQ_ACT_LSB_POSN     = (int)1,
   AOV_SEQUENCER_ACTIVE_AOV_RAM1_SEQ_ACT_MSB_POSN     = (int)1
};
typedef enum aov_sequencer_active_posn_enum aov_sequencer_active_posn;

#define AOV_RAM0_SEQ_ACT_MASK                    (0x00000001u)
#define AOV_RAM1_SEQ_ACT_MASK                    (0x00000002u)

enum aov_debug_select_posn_enum
{
   AOV_DEBUG_SELECT_LSB_POSN                          = (int)0,
   AOV_DEBUG_SELECT_AOV_DEBUG_SELECT_LSB_POSN         = (int)0,
   AOV_DEBUG_SELECT_MSB_POSN                          = (int)3,
   AOV_DEBUG_SELECT_AOV_DEBUG_SELECT_MSB_POSN         = (int)3
};
typedef enum aov_debug_select_posn_enum aov_debug_select_posn;

#define AOV_DEBUG_SELECT_LSB_MASK                (0x00000001u)
#define AOV_DEBUG_SELECT_MSB_MASK                (0x00000008u)

enum aov_keyhole_read_data_posn_enum
{
   AOV_KEYHOLE_READ_DATA_LSB_POSN                     = (int)0,
   AOV_KEYHOLE_READ_DATA_AOV_KEYHOLE_READ_DATA_LSB_POSN = (int)0,
   AOV_KEYHOLE_READ_DATA_MSB_POSN                     = (int)23,
   AOV_KEYHOLE_READ_DATA_AOV_KEYHOLE_READ_DATA_MSB_POSN = (int)23
};
typedef enum aov_keyhole_read_data_posn_enum aov_keyhole_read_data_posn;

#define AOV_KEYHOLE_READ_DATA_LSB_MASK           (0x00000001u)
#define AOV_KEYHOLE_READ_DATA_MSB_MASK           (0x00800000u)

enum aov__access_ctrl_enum_posn_enum
{
   AOV__P0_ACCESS_PERMISSION_POSN                     = (int)0,
   AOV__ACCESS_CTRL_ENUM_AOV__P0_ACCESS_PERMISSION_LSB_POSN = (int)0,
   AOV__ACCESS_CTRL_ENUM_AOV__P0_ACCESS_PERMISSION_MSB_POSN = (int)0,
   AOV__P1_ACCESS_PERMISSION_POSN                     = (int)1,
   AOV__ACCESS_CTRL_ENUM_AOV__P1_ACCESS_PERMISSION_LSB_POSN = (int)1,
   AOV__ACCESS_CTRL_ENUM_AOV__P1_ACCESS_PERMISSION_MSB_POSN = (int)1,
   AOV__P2_ACCESS_PERMISSION_POSN                     = (int)2,
   AOV__ACCESS_CTRL_ENUM_AOV__P2_ACCESS_PERMISSION_LSB_POSN = (int)2,
   AOV__ACCESS_CTRL_ENUM_AOV__P2_ACCESS_PERMISSION_MSB_POSN = (int)2,
   AOV__P3_ACCESS_PERMISSION_POSN                     = (int)3,
   AOV__ACCESS_CTRL_ENUM_AOV__P3_ACCESS_PERMISSION_LSB_POSN = (int)3,
   AOV__ACCESS_CTRL_ENUM_AOV__P3_ACCESS_PERMISSION_MSB_POSN = (int)3
};
typedef enum aov__access_ctrl_enum_posn_enum aov__access_ctrl_enum_posn;

#define AOV__P0_ACCESS_PERMISSION_MASK           (0x00000001u)
#define AOV__P1_ACCESS_PERMISSION_MASK           (0x00000002u)
#define AOV__P2_ACCESS_PERMISSION_MASK           (0x00000004u)
#define AOV__P3_ACCESS_PERMISSION_MASK           (0x00000008u)

enum aov__p0_access_permission_enum
{
   AOV__P0_ACCESS_BLOCKED                   = (int)0x0,
   AOV__P0_ACCESS_UNBLOCKED                 = (int)0x1,
   MAX_AOV__P0_ACCESS_PERMISSION            = (int)0x1
};
#define NUM_AOV__P0_ACCESS_PERMISSION (0x2)
typedef enum aov__p0_access_permission_enum aov__p0_access_permission;


enum aov__p1_access_permission_enum
{
   AOV__P1_ACCESS_BLOCKED                   = (int)0x0,
   AOV__P1_ACCESS_UNBLOCKED                 = (int)0x1,
   MAX_AOV__P1_ACCESS_PERMISSION            = (int)0x1
};
#define NUM_AOV__P1_ACCESS_PERMISSION (0x2)
typedef enum aov__p1_access_permission_enum aov__p1_access_permission;


enum aov__p2_access_permission_enum
{
   AOV__P2_ACCESS_BLOCKED                   = (int)0x0,
   AOV__P2_ACCESS_UNBLOCKED                 = (int)0x1,
   MAX_AOV__P2_ACCESS_PERMISSION            = (int)0x1
};
#define NUM_AOV__P2_ACCESS_PERMISSION (0x2)
typedef enum aov__p2_access_permission_enum aov__p2_access_permission;


enum aov__p3_access_permission_enum
{
   AOV__P3_ACCESS_BLOCKED                   = (int)0x0,
   AOV__P3_ACCESS_UNBLOCKED                 = (int)0x1,
   MAX_AOV__P3_ACCESS_PERMISSION            = (int)0x1
};
#define NUM_AOV__P3_ACCESS_PERMISSION (0x2)
typedef enum aov__p3_access_permission_enum aov__p3_access_permission;


enum aov__mutex_lock_enum_enum
{
   AOV__MUTEX_AVAILABLE                     = (int)0x0,
   AOV__MUTEX_CLAIMED_BY_P0                 = (int)0x1,
   AOV__MUTEX_CLAIMED_BY_P1                 = (int)0x2,
   AOV__MUTEX_CLAIMED_BY_P2                 = (int)0x4,
   AOV__MUTEX_CLAIMED_BY_P3                 = (int)0x8,
   AOV__MUTEX_DISABLED                      = (int)0xF,
   MAX_AOV__MUTEX_LOCK_ENUM                 = (int)0xF
};
typedef enum aov__mutex_lock_enum_enum aov__mutex_lock_enum;


enum aov_event_clear_posn_enum
{
   AOV_EVENT_CLEAR_AOV_EVENT_RAM0_FULL_LSB_POSN       = (int)0,
   AOV_EVENT_CLEAR_AOV_EVENT_RAM0_FULL_MSB_POSN       = (int)0,
   AOV_EVENT_CLEAR_AOV_EVENT_RAM1_FULL_LSB_POSN       = (int)1,
   AOV_EVENT_CLEAR_AOV_EVENT_RAM1_FULL_MSB_POSN       = (int)1,
   AOV_EVENT_CLEAR_AOV_EVENT_RAM0_EMPTY_LSB_POSN      = (int)2,
   AOV_EVENT_CLEAR_AOV_EVENT_RAM0_EMPTY_MSB_POSN      = (int)2,
   AOV_EVENT_CLEAR_AOV_EVENT_RAM1_EMPTY_LSB_POSN      = (int)3,
   AOV_EVENT_CLEAR_AOV_EVENT_RAM1_EMPTY_MSB_POSN      = (int)3,
   AOV_EVENT_CLEAR_AOV_EVENT_RAM_OVERRUN_LSB_POSN     = (int)4,
   AOV_EVENT_CLEAR_AOV_EVENT_RAM_OVERRUN_MSB_POSN     = (int)4,
   AOV_EVENT_CLEAR_AOV_EVENT_VAD_POS_EVENT_LSB_POSN   = (int)5,
   AOV_EVENT_CLEAR_AOV_EVENT_VAD_POS_EVENT_MSB_POSN   = (int)5,
   AOV_EVENT_CLEAR_AOV_EVENT_VAD_NEG_EVENT_LSB_POSN   = (int)6,
   AOV_EVENT_CLEAR_AOV_EVENT_VAD_NEG_EVENT_MSB_POSN   = (int)6
};
typedef enum aov_event_clear_posn_enum aov_event_clear_posn;


enum aov_event_mask_posn_enum
{
   AOV_EVENT_MASK_AOV_EVENT_RAM0_FULL_LSB_POSN        = (int)0,
   AOV_EVENT_MASK_AOV_EVENT_RAM0_FULL_MSB_POSN        = (int)0,
   AOV_EVENT_MASK_AOV_EVENT_RAM1_FULL_LSB_POSN        = (int)1,
   AOV_EVENT_MASK_AOV_EVENT_RAM1_FULL_MSB_POSN        = (int)1,
   AOV_EVENT_MASK_AOV_EVENT_RAM0_EMPTY_LSB_POSN       = (int)2,
   AOV_EVENT_MASK_AOV_EVENT_RAM0_EMPTY_MSB_POSN       = (int)2,
   AOV_EVENT_MASK_AOV_EVENT_RAM1_EMPTY_LSB_POSN       = (int)3,
   AOV_EVENT_MASK_AOV_EVENT_RAM1_EMPTY_MSB_POSN       = (int)3,
   AOV_EVENT_MASK_AOV_EVENT_RAM_OVERRUN_LSB_POSN      = (int)4,
   AOV_EVENT_MASK_AOV_EVENT_RAM_OVERRUN_MSB_POSN      = (int)4,
   AOV_EVENT_MASK_AOV_EVENT_VAD_POS_EVENT_LSB_POSN    = (int)5,
   AOV_EVENT_MASK_AOV_EVENT_VAD_POS_EVENT_MSB_POSN    = (int)5,
   AOV_EVENT_MASK_AOV_EVENT_VAD_NEG_EVENT_LSB_POSN    = (int)6,
   AOV_EVENT_MASK_AOV_EVENT_VAD_NEG_EVENT_MSB_POSN    = (int)6
};
typedef enum aov_event_mask_posn_enum aov_event_mask_posn;


enum aov_event_status_posn_enum
{
   AOV_EVENT_STATUS_AOV_EVENT_RAM0_FULL_LSB_POSN      = (int)0,
   AOV_EVENT_STATUS_AOV_EVENT_RAM0_FULL_MSB_POSN      = (int)0,
   AOV_EVENT_STATUS_AOV_EVENT_RAM1_FULL_LSB_POSN      = (int)1,
   AOV_EVENT_STATUS_AOV_EVENT_RAM1_FULL_MSB_POSN      = (int)1,
   AOV_EVENT_STATUS_AOV_EVENT_RAM0_EMPTY_LSB_POSN     = (int)2,
   AOV_EVENT_STATUS_AOV_EVENT_RAM0_EMPTY_MSB_POSN     = (int)2,
   AOV_EVENT_STATUS_AOV_EVENT_RAM1_EMPTY_LSB_POSN     = (int)3,
   AOV_EVENT_STATUS_AOV_EVENT_RAM1_EMPTY_MSB_POSN     = (int)3,
   AOV_EVENT_STATUS_AOV_EVENT_RAM_OVERRUN_LSB_POSN    = (int)4,
   AOV_EVENT_STATUS_AOV_EVENT_RAM_OVERRUN_MSB_POSN    = (int)4,
   AOV_EVENT_STATUS_AOV_EVENT_VAD_POS_EVENT_LSB_POSN  = (int)5,
   AOV_EVENT_STATUS_AOV_EVENT_VAD_POS_EVENT_MSB_POSN  = (int)5,
   AOV_EVENT_STATUS_AOV_EVENT_VAD_NEG_EVENT_LSB_POSN  = (int)6,
   AOV_EVENT_STATUS_AOV_EVENT_VAD_NEG_EVENT_MSB_POSN  = (int)6
};
typedef enum aov_event_status_posn_enum aov_event_status_posn;


enum aov_wakeup_mask_posn_enum
{
   AOV_WAKEUP_MASK_AOV_EVENT_RAM0_FULL_LSB_POSN       = (int)0,
   AOV_WAKEUP_MASK_AOV_EVENT_RAM0_FULL_MSB_POSN       = (int)0,
   AOV_WAKEUP_MASK_AOV_EVENT_RAM1_FULL_LSB_POSN       = (int)1,
   AOV_WAKEUP_MASK_AOV_EVENT_RAM1_FULL_MSB_POSN       = (int)1,
   AOV_WAKEUP_MASK_AOV_EVENT_RAM0_EMPTY_LSB_POSN      = (int)2,
   AOV_WAKEUP_MASK_AOV_EVENT_RAM0_EMPTY_MSB_POSN      = (int)2,
   AOV_WAKEUP_MASK_AOV_EVENT_RAM1_EMPTY_LSB_POSN      = (int)3,
   AOV_WAKEUP_MASK_AOV_EVENT_RAM1_EMPTY_MSB_POSN      = (int)3,
   AOV_WAKEUP_MASK_AOV_EVENT_RAM_OVERRUN_LSB_POSN     = (int)4,
   AOV_WAKEUP_MASK_AOV_EVENT_RAM_OVERRUN_MSB_POSN     = (int)4,
   AOV_WAKEUP_MASK_AOV_EVENT_VAD_POS_EVENT_LSB_POSN   = (int)5,
   AOV_WAKEUP_MASK_AOV_EVENT_VAD_POS_EVENT_MSB_POSN   = (int)5,
   AOV_WAKEUP_MASK_AOV_EVENT_VAD_NEG_EVENT_LSB_POSN   = (int)6,
   AOV_WAKEUP_MASK_AOV_EVENT_VAD_NEG_EVENT_MSB_POSN   = (int)6
};
typedef enum aov_wakeup_mask_posn_enum aov_wakeup_mask_posn;


enum aov_access_ctrl_posn_enum
{
   AOV_ACCESS_CTRL_AOV__P0_ACCESS_PERMISSION_LSB_POSN = (int)0,
   AOV_ACCESS_CTRL_AOV__P0_ACCESS_PERMISSION_MSB_POSN = (int)0,
   AOV_ACCESS_CTRL_AOV__P1_ACCESS_PERMISSION_LSB_POSN = (int)1,
   AOV_ACCESS_CTRL_AOV__P1_ACCESS_PERMISSION_MSB_POSN = (int)1,
   AOV_ACCESS_CTRL_AOV__P2_ACCESS_PERMISSION_LSB_POSN = (int)2,
   AOV_ACCESS_CTRL_AOV__P2_ACCESS_PERMISSION_MSB_POSN = (int)2,
   AOV_ACCESS_CTRL_AOV__P3_ACCESS_PERMISSION_LSB_POSN = (int)3,
   AOV_ACCESS_CTRL_AOV__P3_ACCESS_PERMISSION_MSB_POSN = (int)3
};
typedef enum aov_access_ctrl_posn_enum aov_access_ctrl_posn;


#endif /* IO_DEFS_MODULE_AOV */
#endif /* __IO_DEFS_H__IO_DEFS_MODULE_AOV */




#if defined(IO_DEFS_MODULE_AUDIO_DMAC) 

#if !defined(__IO_DEFS_H__IO_DEFS_MODULE_AUDIO_DMAC) 
#define __IO_DEFS_H__IO_DEFS_MODULE_AUDIO_DMAC

/* -- audio_dmac -- Registers in Audio DMAC  -- */

enum audio_dma_channel_type_enum
{
   AUDIO_DMA_VM_CH                          = (int)0x1,
   MAX_AUDIO_DMA_CHANNEL_TYPE               = (int)0x1
};
typedef enum audio_dma_channel_type_enum audio_dma_channel_type;


enum audio_dma_descriptor_structure_enum
{
   AUDIO_DMA_DESCRPTR_L0_SRC_ADDR           = (int)0x0,
   AUDIO_DMA_DESCRPTR_L1_DES_ADDR           = (int)0x1,
   AUDIO_DMA_DESCRPTR_L2_TYPE_BUFF_HANDLE   = (int)0x2,
   AUDIO_DMA_DESCRPTR_L3_FLAGS_TAG_LEN      = (int)0x3,
   MAX_AUDIO_DMA_DESCRIPTOR_STRUCTURE       = (int)0x3
};
#define NUM_AUDIO_DMA_DESCRIPTOR_STRUCTURE (0x4)
typedef enum audio_dma_descriptor_structure_enum audio_dma_descriptor_structure;


enum audio_dma_descriptor_structure_l2_enum
{
   AUDIO_DMA_L2_SRC_BUFF_HANDLE_LSB         = (int)0x0,
   AUDIO_DMA_L2_SRC_BUFF_HANDLE_MSB         = (int)0xB,
   AUDIO_DMA_L2_SRC_BUFF_TYPE_LSB           = (int)0xC,
   AUDIO_DMA_L2_SRC_BUFF_TYPE_MSB           = (int)0xF,
   AUDIO_DMA_L2_DES_BUFF_HANDLE_LSB         = (int)0x10,
   AUDIO_DMA_L2_DES_BUFF_HANDLE_MSB         = (int)0x1B,
   AUDIO_DMA_L2_DES_BUFF_TYPE_LSB           = (int)0x1C,
   AUDIO_DMA_L2_DES_BUFF_TYPE_MSB           = (int)0x1F,
   MAX_AUDIO_DMA_DESCRIPTOR_STRUCTURE_L2    = (int)0x1F
};
typedef enum audio_dma_descriptor_structure_l2_enum audio_dma_descriptor_structure_l2;


enum audio_dma_descriptor_structure_l3_enum
{
   AUDIO_DMA_L3_TRANSACTION_LEN_LSB         = (int)0x0,
   AUDIO_DMA_L3_TRANSACTION_LEN_MSB         = (int)0xF,
   AUDIO_DMA_L3_TAG_LSB                     = (int)0x10,
   AUDIO_DMA_L3_TAG_MSB                     = (int)0x17,
   MAX_AUDIO_DMA_DESCRIPTOR_STRUCTURE_L3    = (int)0x17
};
typedef enum audio_dma_descriptor_structure_l3_enum audio_dma_descriptor_structure_l3;


enum audio_dma_descriptor_structure_l3_posn_enum
{
   AUDIO_DMA_L3_FLAG_INT_LOC_POSN                     = (int)24,
   AUDIO_DMA_DESCRIPTOR_STRUCTURE_L3_AUDIO_DMA_L3_FLAG_INT_LOC_LSB_POSN = (int)24,
   AUDIO_DMA_DESCRIPTOR_STRUCTURE_L3_AUDIO_DMA_L3_FLAG_INT_LOC_MSB_POSN = (int)24,
   AUDIO_DMA_L3_FLAG_STATUS_LOC_POSN                  = (int)25,
   AUDIO_DMA_DESCRIPTOR_STRUCTURE_L3_AUDIO_DMA_L3_FLAG_STATUS_LOC_LSB_POSN = (int)25,
   AUDIO_DMA_DESCRIPTOR_STRUCTURE_L3_AUDIO_DMA_L3_FLAG_STATUS_LOC_MSB_POSN = (int)25
};
typedef enum audio_dma_descriptor_structure_l3_posn_enum audio_dma_descriptor_structure_l3_posn;

#define AUDIO_DMA_L3_FLAG_INT_LOC_MASK           (0x01000000u)
#define AUDIO_DMA_L3_FLAG_STATUS_LOC_MASK        (0x02000000u)

enum audio_dma_error_source_enum
{
   AUDIO_DMA_INTAKE_ERR                     = (int)0x0,
   AUDIO_DMA_FETCH_ERR                      = (int)0x1,
   AUDIO_DMA_SINK_ERR                       = (int)0x2,
   AUDIO_DMA_RESULT_ERR                     = (int)0x3,
   MAX_AUDIO_DMA_ERROR_SOURCE               = (int)0x3
};
#define NUM_AUDIO_DMA_ERROR_SOURCE (0x4)
typedef enum audio_dma_error_source_enum audio_dma_error_source;


enum audio_dma_error_type_enum
{
   AUDIO_DMA_UNMMAPED_MMU_READ              = (int)0x0,
   AUDIO_DMA_UNSUCCESSFULY_BUFER_SET        = (int)0x1,
   AUDIO_DMA_SRC_OR_DEST_NOT_EXIST          = (int)0x2,
   AUDIO_DMA_ABORTED                        = (int)0x3,
   MAX_AUDIO_DMA_ERROR_TYPE                 = (int)0x3
};
#define NUM_AUDIO_DMA_ERROR_TYPE (0x4)
typedef enum audio_dma_error_type_enum audio_dma_error_type;


enum audio_dma_immediate_halt_enum_enum
{
   AUDIO_DMAC_IMMEDIATE_HALT_IMMEDIATE      = (int)0x0,
   AUDIO_DMAC_IMMEDIATE_HALT_CLEAR          = (int)0x1,
   MAX_AUDIO_DMA_IMMEDIATE_HALT_ENUM        = (int)0x1
};
#define NUM_AUDIO_DMA_IMMEDIATE_HALT_ENUM (0x2)
typedef enum audio_dma_immediate_halt_enum_enum audio_dma_immediate_halt_enum;


enum audio_dma_main_state_enum
{
   AUDIO_DMA_STATE_IDLE                     = (int)0x0,
   AUDIO_DMA_STATE_INTAKE                   = (int)0x1,
   AUDIO_DMA_STATE_DMA                      = (int)0x2,
   AUDIO_DMA_STATE_RESULT                   = (int)0x3,
   AUDIO_DMA_STATE_DONE                     = (int)0x4,
   AUDIO_DMA_STATE_ERROR                    = (int)0x5,
   AUDIO_DMA_STATE_ABORT                    = (int)0x6,
   MAX_AUDIO_DMA_MAIN_STATE                 = (int)0x6
};
#define NUM_AUDIO_DMA_MAIN_STATE (0x7)
typedef enum audio_dma_main_state_enum audio_dma_main_state;


enum audio_dma_queue_halt_enum_enum
{
   AUDIO_DMAC_QUEUE_HALT_ON_END_TRANSFER    = (int)0x0,
   AUDIO_DMAC_QUEUE_HALT_IMMEDIATE          = (int)0x1,
   AUDIO_DMAC_QUEUE_HALT_CLEAR              = (int)0x2,
   MAX_AUDIO_DMA_QUEUE_HALT_ENUM            = (int)0x2
};
#define NUM_AUDIO_DMA_QUEUE_HALT_ENUM (0x3)
typedef enum audio_dma_queue_halt_enum_enum audio_dma_queue_halt_enum;


enum audio_dma_transaction_type_enum
{
   AUDIO_DMA_TRAN_TYPE_VM_NO_OFFSET         = (int)0x1,
   AUDIO_DMA_TRAN_TYPE_VM_OFFSET            = (int)0x2,
   AUDIO_DMA_TRAN_TYPE_NULL                 = (int)0x3,
   AUDIO_DMA_TRAN_TYPE_ROOT_ERROR           = (int)0xF,
   MAX_AUDIO_DMA_TRANSACTION_TYPE           = (int)0xF
};
typedef enum audio_dma_transaction_type_enum audio_dma_transaction_type;


enum audio_dma_debug_select_posn_enum
{
   AUDIO_DMA_DEBUG_WORD_SELECT_LSB_POSN               = (int)0,
   AUDIO_DMA_DEBUG_SELECT_AUDIO_DMA_DEBUG_WORD_SELECT_LSB_POSN = (int)0,
   AUDIO_DMA_DEBUG_WORD_SELECT_MSB_POSN               = (int)3,
   AUDIO_DMA_DEBUG_SELECT_AUDIO_DMA_DEBUG_WORD_SELECT_MSB_POSN = (int)3,
   AUDIO_DMA_DEBUG_UNIT_SELECT_LSB_POSN               = (int)4,
   AUDIO_DMA_DEBUG_SELECT_AUDIO_DMA_DEBUG_UNIT_SELECT_LSB_POSN = (int)4,
   AUDIO_DMA_DEBUG_UNIT_SELECT_MSB_POSN               = (int)7,
   AUDIO_DMA_DEBUG_SELECT_AUDIO_DMA_DEBUG_UNIT_SELECT_MSB_POSN = (int)7,
   AUDIO_DMA_DEBUG_EN_POSN                            = (int)8,
   AUDIO_DMA_DEBUG_SELECT_AUDIO_DMA_DEBUG_EN_LSB_POSN = (int)8,
   AUDIO_DMA_DEBUG_SELECT_AUDIO_DMA_DEBUG_EN_MSB_POSN = (int)8
};
typedef enum audio_dma_debug_select_posn_enum audio_dma_debug_select_posn;

#define AUDIO_DMA_DEBUG_WORD_SELECT_LSB_MASK     (0x00000001u)
#define AUDIO_DMA_DEBUG_WORD_SELECT_MSB_MASK     (0x00000008u)
#define AUDIO_DMA_DEBUG_UNIT_SELECT_LSB_MASK     (0x00000010u)
#define AUDIO_DMA_DEBUG_UNIT_SELECT_MSB_MASK     (0x00000080u)
#define AUDIO_DMA_DEBUG_EN_MASK                  (0x00000100u)

enum audio_dma_debug_unit_select_enum
{
   AUDIO_DMA_DEBUG_UNIT_INTAKE              = (int)0x0,
   AUDIO_DMA_DEBUG_UNIT_FETCH               = (int)0x1,
   AUDIO_DMA_DEBUG_UNIT_SINK                = (int)0x2,
   AUDIO_DMA_DEBUG_UNIT_MAIN                = (int)0x3,
   AUDIO_DMA_DEBUG_UNIT_ADDITINAL_FUNC      = (int)0x4,
   MAX_AUDIO_DMA_DEBUG_UNIT_SELECT          = (int)0x4
};
#define NUM_AUDIO_DMA_DEBUG_UNIT_SELECT (0x5)
typedef enum audio_dma_debug_unit_select_enum audio_dma_debug_unit_select;


enum audio_dma_queue_halt_on_exception_posn_enum
{
   AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_END_TR_POSN      = (int)0,
   AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_END_TR_LSB_POSN = (int)0,
   AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_END_TR_MSB_POSN = (int)0,
   AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_MIDDLE_TR_POSN   = (int)1,
   AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_MIDDLE_TR_LSB_POSN = (int)1,
   AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_MIDDLE_TR_MSB_POSN = (int)1,
   AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_CLEAR_TR_POSN    = (int)2,
   AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_CLEAR_TR_LSB_POSN = (int)2,
   AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_CLEAR_TR_MSB_POSN = (int)2
};
typedef enum audio_dma_queue_halt_on_exception_posn_enum audio_dma_queue_halt_on_exception_posn;

#define AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_END_TR_MASK (0x00000001u)
#define AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_MIDDLE_TR_MASK (0x00000002u)
#define AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION_CLEAR_TR_MASK (0x00000004u)

enum audio_dma_queue_status_posn_enum
{
   AUDIO_DMA_QUEUE_STATUS_IDLE_POSN                   = (int)0,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_IDLE_LSB_POSN = (int)0,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_IDLE_MSB_POSN = (int)0,
   AUDIO_DMA_QUEUE_STATUS_ERROR_POSN                  = (int)1,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_ERROR_LSB_POSN = (int)1,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_ERROR_MSB_POSN = (int)1,
   AUDIO_DMA_QUEUE_STATUS_HALT_POSN                   = (int)2,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_HALT_LSB_POSN = (int)2,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_HALT_MSB_POSN = (int)2,
   AUDIO_DMA_QUEUE_STATUS_DUMMY0_POSN                 = (int)3,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_DUMMY0_LSB_POSN = (int)3,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_DUMMY0_MSB_POSN = (int)3,
   AUDIO_DMA_QUEUE_STATUS_ERROR_SRC_LSB_POSN          = (int)4,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_ERROR_SRC_LSB_POSN = (int)4,
   AUDIO_DMA_QUEUE_STATUS_ERROR_SRC_MSB_POSN          = (int)5,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_ERROR_SRC_MSB_POSN = (int)5,
   AUDIO_DMA_QUEUE_STATUS_ERROR_TYPE_LSB_POSN         = (int)6,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_ERROR_TYPE_LSB_POSN = (int)6,
   AUDIO_DMA_QUEUE_STATUS_ERROR_TYPE_MSB_POSN         = (int)7,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_ERROR_TYPE_MSB_POSN = (int)7,
   AUDIO_DMA_QUEUE_STATUS_TRANS_TAG_LSB_POSN          = (int)8,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_TRANS_TAG_LSB_POSN = (int)8,
   AUDIO_DMA_QUEUE_STATUS_TRANS_TAG_MSB_POSN          = (int)15,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_TRANS_TAG_MSB_POSN = (int)15,
   AUDIO_DMA_QUEUE_STATUS_TRANS_STATE_LSB_POSN        = (int)16,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_TRANS_STATE_LSB_POSN = (int)16,
   AUDIO_DMA_QUEUE_STATUS_TRANS_STATE_MSB_POSN        = (int)18,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_TRANS_STATE_MSB_POSN = (int)18,
   AUDIO_DMA_QUEUE_STATUS_DUMMY1_POSN                 = (int)19,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_DUMMY1_LSB_POSN = (int)19,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_DUMMY1_MSB_POSN = (int)19,
   AUDIO_DMA_QUEUE_STATUS_TRANS_CNTR_LSB_POSN         = (int)20,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_TRANS_CNTR_LSB_POSN = (int)20,
   AUDIO_DMA_QUEUE_STATUS_TRANS_CNTR_MSB_POSN         = (int)23,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_TRANS_CNTR_MSB_POSN = (int)23,
   AUDIO_DMA_QUEUE_STATUS_CH_TYPE_LSB_POSN            = (int)24,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_CH_TYPE_LSB_POSN = (int)24,
   AUDIO_DMA_QUEUE_STATUS_CH_TYPE_MSB_POSN            = (int)25,
   AUDIO_DMA_QUEUE_STATUS_AUDIO_DMA_QUEUE_STATUS_CH_TYPE_MSB_POSN = (int)25
};
typedef enum audio_dma_queue_status_posn_enum audio_dma_queue_status_posn;

#define AUDIO_DMA_QUEUE_STATUS_IDLE_MASK         (0x00000001u)
#define AUDIO_DMA_QUEUE_STATUS_ERROR_MASK        (0x00000002u)
#define AUDIO_DMA_QUEUE_STATUS_HALT_MASK         (0x00000004u)
#define AUDIO_DMA_QUEUE_STATUS_DUMMY0_MASK       (0x00000008u)
#define AUDIO_DMA_QUEUE_STATUS_ERROR_SRC_LSB_MASK (0x00000010u)
#define AUDIO_DMA_QUEUE_STATUS_ERROR_SRC_MSB_MASK (0x00000020u)
#define AUDIO_DMA_QUEUE_STATUS_ERROR_TYPE_LSB_MASK (0x00000040u)
#define AUDIO_DMA_QUEUE_STATUS_ERROR_TYPE_MSB_MASK (0x00000080u)
#define AUDIO_DMA_QUEUE_STATUS_TRANS_TAG_LSB_MASK (0x00000100u)
#define AUDIO_DMA_QUEUE_STATUS_TRANS_TAG_MSB_MASK (0x00008000u)
#define AUDIO_DMA_QUEUE_STATUS_TRANS_STATE_LSB_MASK (0x00010000u)
#define AUDIO_DMA_QUEUE_STATUS_TRANS_STATE_MSB_MASK (0x00040000u)
#define AUDIO_DMA_QUEUE_STATUS_DUMMY1_MASK       (0x00080000u)
#define AUDIO_DMA_QUEUE_STATUS_TRANS_CNTR_LSB_MASK (0x00100000u)
#define AUDIO_DMA_QUEUE_STATUS_TRANS_CNTR_MSB_MASK (0x00800000u)
#define AUDIO_DMA_QUEUE_STATUS_CH_TYPE_LSB_MASK  (0x01000000u)
#define AUDIO_DMA_QUEUE_STATUS_CH_TYPE_MSB_MASK  (0x02000000u)

enum audio_dma_debug_status_posn_enum
{
   AUDIO_DMA_DEBUG_STATUS_LSB_POSN                    = (int)0,
   AUDIO_DMA_DEBUG_STATUS_AUDIO_DMA_DEBUG_STATUS_LSB_POSN = (int)0,
   AUDIO_DMA_DEBUG_STATUS_MSB_POSN                    = (int)31,
   AUDIO_DMA_DEBUG_STATUS_AUDIO_DMA_DEBUG_STATUS_MSB_POSN = (int)31
};
typedef enum audio_dma_debug_status_posn_enum audio_dma_debug_status_posn;

#define AUDIO_DMA_DEBUG_STATUS_LSB_MASK          (0x00000001u)
#define AUDIO_DMA_DEBUG_STATUS_MSB_MASK          (0x80000000u)

enum audio_dma_queue_abort_transfer_posn_enum
{
   AUDIO_DMA_QUEUE_ABORT_TRANSFER_POSN                = (int)0,
   AUDIO_DMA_QUEUE_ABORT_TRANSFER_AUDIO_DMA_QUEUE_ABORT_TRANSFER_LSB_POSN = (int)0,
   AUDIO_DMA_QUEUE_ABORT_TRANSFER_AUDIO_DMA_QUEUE_ABORT_TRANSFER_MSB_POSN = (int)0
};
typedef enum audio_dma_queue_abort_transfer_posn_enum audio_dma_queue_abort_transfer_posn;

#define AUDIO_DMA_QUEUE_ABORT_TRANSFER_MASK      (0x00000001u)

enum audio_dma_queue_buffer_handle_posn_enum
{
   AUDIO_DMA_QUEUE_BUFFER_HANDLE_LSB_POSN             = (int)0,
   AUDIO_DMA_QUEUE_BUFFER_HANDLE_AUDIO_DMA_QUEUE_BUFFER_HANDLE_LSB_POSN = (int)0,
   AUDIO_DMA_QUEUE_BUFFER_HANDLE_MSB_POSN             = (int)11,
   AUDIO_DMA_QUEUE_BUFFER_HANDLE_AUDIO_DMA_QUEUE_BUFFER_HANDLE_MSB_POSN = (int)11
};
typedef enum audio_dma_queue_buffer_handle_posn_enum audio_dma_queue_buffer_handle_posn;

#define AUDIO_DMA_QUEUE_BUFFER_HANDLE_LSB_MASK   (0x00000001u)
#define AUDIO_DMA_QUEUE_BUFFER_HANDLE_MSB_MASK   (0x00000800u)

enum audio_dma_queue_clear_request_queue_posn_enum
{
   AUDIO_DMA_QUEUE_CLEAR_REQUEST_QUEUE_POSN           = (int)0,
   AUDIO_DMA_QUEUE_CLEAR_REQUEST_QUEUE_AUDIO_DMA_QUEUE_CLEAR_REQUEST_QUEUE_LSB_POSN = (int)0,
   AUDIO_DMA_QUEUE_CLEAR_REQUEST_QUEUE_AUDIO_DMA_QUEUE_CLEAR_REQUEST_QUEUE_MSB_POSN = (int)0
};
typedef enum audio_dma_queue_clear_request_queue_posn_enum audio_dma_queue_clear_request_queue_posn;

#define AUDIO_DMA_QUEUE_CLEAR_REQUEST_QUEUE_MASK (0x00000001u)

enum audio_dma_queue_clocks_enable_posn_enum
{
   AUDIO_DMA_QUEUE_CLOCKS_ENABLE_POSN                 = (int)0,
   AUDIO_DMA_QUEUE_CLOCKS_ENABLE_AUDIO_DMA_QUEUE_CLOCKS_ENABLE_LSB_POSN = (int)0,
   AUDIO_DMA_QUEUE_CLOCKS_ENABLE_AUDIO_DMA_QUEUE_CLOCKS_ENABLE_MSB_POSN = (int)0
};
typedef enum audio_dma_queue_clocks_enable_posn_enum audio_dma_queue_clocks_enable_posn;

#define AUDIO_DMA_QUEUE_CLOCKS_ENABLE_MASK       (0x00000001u)

enum audio_dma_queue_enable_posn_enum
{
   AUDIO_DMA_QUEUE_ENABLE_POSN                        = (int)0,
   AUDIO_DMA_QUEUE_ENABLE_AUDIO_DMA_QUEUE_ENABLE_LSB_POSN = (int)0,
   AUDIO_DMA_QUEUE_ENABLE_AUDIO_DMA_QUEUE_ENABLE_MSB_POSN = (int)0
};
typedef enum audio_dma_queue_enable_posn_enum audio_dma_queue_enable_posn;

#define AUDIO_DMA_QUEUE_ENABLE_MASK              (0x00000001u)

enum audio_dma_queue_request_count_posn_enum
{
   AUDIO_DMA_QUEUE_REQUEST_COUNT_LSB_POSN             = (int)0,
   AUDIO_DMA_QUEUE_REQUEST_COUNT_AUDIO_DMA_QUEUE_REQUEST_COUNT_LSB_POSN = (int)0,
   AUDIO_DMA_QUEUE_REQUEST_COUNT_MSB_POSN             = (int)7,
   AUDIO_DMA_QUEUE_REQUEST_COUNT_AUDIO_DMA_QUEUE_REQUEST_COUNT_MSB_POSN = (int)7
};
typedef enum audio_dma_queue_request_count_posn_enum audio_dma_queue_request_count_posn;

#define AUDIO_DMA_QUEUE_REQUEST_COUNT_LSB_MASK   (0x00000001u)
#define AUDIO_DMA_QUEUE_REQUEST_COUNT_MSB_MASK   (0x00000080u)

enum audio_dma_queue_request_fetch_disable_posn_enum
{
   AUDIO_DMA_QUEUE_REQUEST_FETCH_DISABLE_POSN         = (int)0,
   AUDIO_DMA_QUEUE_REQUEST_FETCH_DISABLE_AUDIO_DMA_QUEUE_REQUEST_FETCH_DISABLE_LSB_POSN = (int)0,
   AUDIO_DMA_QUEUE_REQUEST_FETCH_DISABLE_AUDIO_DMA_QUEUE_REQUEST_FETCH_DISABLE_MSB_POSN = (int)0
};
typedef enum audio_dma_queue_request_fetch_disable_posn_enum audio_dma_queue_request_fetch_disable_posn;

#define AUDIO_DMA_QUEUE_REQUEST_FETCH_DISABLE_MASK (0x00000001u)

enum audio_dma_queue_trigger_request_posn_enum
{
   AUDIO_DMA_QUEUE_TRIGGER_REQUEST_POSN               = (int)0,
   AUDIO_DMA_QUEUE_TRIGGER_REQUEST_AUDIO_DMA_QUEUE_TRIGGER_REQUEST_LSB_POSN = (int)0,
   AUDIO_DMA_QUEUE_TRIGGER_REQUEST_AUDIO_DMA_QUEUE_TRIGGER_REQUEST_MSB_POSN = (int)0
};
typedef enum audio_dma_queue_trigger_request_posn_enum audio_dma_queue_trigger_request_posn;

#define AUDIO_DMA_QUEUE_TRIGGER_REQUEST_MASK     (0x00000001u)

enum audio_dma_soft_reset_posn_enum
{
   AUDIO_DMA_SOFT_RESET_POSN                          = (int)0,
   AUDIO_DMA_SOFT_RESET_AUDIO_DMA_SOFT_RESET_LSB_POSN = (int)0,
   AUDIO_DMA_SOFT_RESET_AUDIO_DMA_SOFT_RESET_MSB_POSN = (int)0
};
typedef enum audio_dma_soft_reset_posn_enum audio_dma_soft_reset_posn;

#define AUDIO_DMA_SOFT_RESET_MASK                (0x00000001u)

enum audio_dmac__access_ctrl_enum_posn_enum
{
   AUDIO_DMAC__P0_ACCESS_PERMISSION_POSN              = (int)0,
   AUDIO_DMAC__ACCESS_CTRL_ENUM_AUDIO_DMAC__P0_ACCESS_PERMISSION_LSB_POSN = (int)0,
   AUDIO_DMAC__ACCESS_CTRL_ENUM_AUDIO_DMAC__P0_ACCESS_PERMISSION_MSB_POSN = (int)0,
   AUDIO_DMAC__P1_ACCESS_PERMISSION_POSN              = (int)1,
   AUDIO_DMAC__ACCESS_CTRL_ENUM_AUDIO_DMAC__P1_ACCESS_PERMISSION_LSB_POSN = (int)1,
   AUDIO_DMAC__ACCESS_CTRL_ENUM_AUDIO_DMAC__P1_ACCESS_PERMISSION_MSB_POSN = (int)1,
   AUDIO_DMAC__P2_ACCESS_PERMISSION_POSN              = (int)2,
   AUDIO_DMAC__ACCESS_CTRL_ENUM_AUDIO_DMAC__P2_ACCESS_PERMISSION_LSB_POSN = (int)2,
   AUDIO_DMAC__ACCESS_CTRL_ENUM_AUDIO_DMAC__P2_ACCESS_PERMISSION_MSB_POSN = (int)2,
   AUDIO_DMAC__P3_ACCESS_PERMISSION_POSN              = (int)3,
   AUDIO_DMAC__ACCESS_CTRL_ENUM_AUDIO_DMAC__P3_ACCESS_PERMISSION_LSB_POSN = (int)3,
   AUDIO_DMAC__ACCESS_CTRL_ENUM_AUDIO_DMAC__P3_ACCESS_PERMISSION_MSB_POSN = (int)3
};
typedef enum audio_dmac__access_ctrl_enum_posn_enum audio_dmac__access_ctrl_enum_posn;

#define AUDIO_DMAC__P0_ACCESS_PERMISSION_MASK    (0x00000001u)
#define AUDIO_DMAC__P1_ACCESS_PERMISSION_MASK    (0x00000002u)
#define AUDIO_DMAC__P2_ACCESS_PERMISSION_MASK    (0x00000004u)
#define AUDIO_DMAC__P3_ACCESS_PERMISSION_MASK    (0x00000008u)

enum audio_dmac__p0_access_permission_enum
{
   AUDIO_DMAC__P0_ACCESS_BLOCKED            = (int)0x0,
   AUDIO_DMAC__P0_ACCESS_UNBLOCKED          = (int)0x1,
   MAX_AUDIO_DMAC__P0_ACCESS_PERMISSION     = (int)0x1
};
#define NUM_AUDIO_DMAC__P0_ACCESS_PERMISSION (0x2)
typedef enum audio_dmac__p0_access_permission_enum audio_dmac__p0_access_permission;


enum audio_dmac__p1_access_permission_enum
{
   AUDIO_DMAC__P1_ACCESS_BLOCKED            = (int)0x0,
   AUDIO_DMAC__P1_ACCESS_UNBLOCKED          = (int)0x1,
   MAX_AUDIO_DMAC__P1_ACCESS_PERMISSION     = (int)0x1
};
#define NUM_AUDIO_DMAC__P1_ACCESS_PERMISSION (0x2)
typedef enum audio_dmac__p1_access_permission_enum audio_dmac__p1_access_permission;


enum audio_dmac__p2_access_permission_enum
{
   AUDIO_DMAC__P2_ACCESS_BLOCKED            = (int)0x0,
   AUDIO_DMAC__P2_ACCESS_UNBLOCKED          = (int)0x1,
   MAX_AUDIO_DMAC__P2_ACCESS_PERMISSION     = (int)0x1
};
#define NUM_AUDIO_DMAC__P2_ACCESS_PERMISSION (0x2)
typedef enum audio_dmac__p2_access_permission_enum audio_dmac__p2_access_permission;


enum audio_dmac__p3_access_permission_enum
{
   AUDIO_DMAC__P3_ACCESS_BLOCKED            = (int)0x0,
   AUDIO_DMAC__P3_ACCESS_UNBLOCKED          = (int)0x1,
   MAX_AUDIO_DMAC__P3_ACCESS_PERMISSION     = (int)0x1
};
#define NUM_AUDIO_DMAC__P3_ACCESS_PERMISSION (0x2)
typedef enum audio_dmac__p3_access_permission_enum audio_dmac__p3_access_permission;


enum audio_dmac__mutex_lock_enum_enum
{
   AUDIO_DMAC__MUTEX_AVAILABLE              = (int)0x0,
   AUDIO_DMAC__MUTEX_CLAIMED_BY_P0          = (int)0x1,
   AUDIO_DMAC__MUTEX_CLAIMED_BY_P1          = (int)0x2,
   AUDIO_DMAC__MUTEX_CLAIMED_BY_P2          = (int)0x4,
   AUDIO_DMAC__MUTEX_CLAIMED_BY_P3          = (int)0x8,
   AUDIO_DMAC__MUTEX_DISABLED               = (int)0xF,
   MAX_AUDIO_DMAC__MUTEX_LOCK_ENUM          = (int)0xF
};
typedef enum audio_dmac__mutex_lock_enum_enum audio_dmac__mutex_lock_enum;


enum audio_dmac_cpu0_access_ctrl_posn_enum
{
   AUDIO_DMAC_CPU0_ACCESS_CTRL_AUDIO_DMAC__P0_ACCESS_PERMISSION_LSB_POSN = (int)0,
   AUDIO_DMAC_CPU0_ACCESS_CTRL_AUDIO_DMAC__P0_ACCESS_PERMISSION_MSB_POSN = (int)0,
   AUDIO_DMAC_CPU0_ACCESS_CTRL_AUDIO_DMAC__P1_ACCESS_PERMISSION_LSB_POSN = (int)1,
   AUDIO_DMAC_CPU0_ACCESS_CTRL_AUDIO_DMAC__P1_ACCESS_PERMISSION_MSB_POSN = (int)1,
   AUDIO_DMAC_CPU0_ACCESS_CTRL_AUDIO_DMAC__P2_ACCESS_PERMISSION_LSB_POSN = (int)2,
   AUDIO_DMAC_CPU0_ACCESS_CTRL_AUDIO_DMAC__P2_ACCESS_PERMISSION_MSB_POSN = (int)2,
   AUDIO_DMAC_CPU0_ACCESS_CTRL_AUDIO_DMAC__P3_ACCESS_PERMISSION_LSB_POSN = (int)3,
   AUDIO_DMAC_CPU0_ACCESS_CTRL_AUDIO_DMAC__P3_ACCESS_PERMISSION_MSB_POSN = (int)3
};
typedef enum audio_dmac_cpu0_access_ctrl_posn_enum audio_dmac_cpu0_access_ctrl_posn;


#endif /* IO_DEFS_MODULE_AUDIO_DMAC */
#endif /* __IO_DEFS_H__IO_DEFS_MODULE_AUDIO_DMAC */

#if defined(IO_DEFS_MODULE_AUDIO_SYS_ADPLL) 

#if !defined(__IO_DEFS_H__IO_DEFS_MODULE_AUDIO_SYS_ADPLL) 
#define __IO_DEFS_H__IO_DEFS_MODULE_AUDIO_SYS_ADPLL

/* -- audio_sys_adpll -- Audio subsystem clock control registers for the ADPLL -- */

enum clkgen_clk_switch_posn_enum
{
   CLKGEN_CLK_SWITCH_DELAY_LSB_POSN                   = (int)0,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_DELAY_LSB_POSN = (int)0,
   CLKGEN_CLK_SWITCH_DELAY_MSB_POSN                   = (int)6,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_DELAY_MSB_POSN = (int)6,
   CLKGEN_CLK_SWITCH_EN_POSN                          = (int)8,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_EN_LSB_POSN    = (int)8,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_EN_MSB_POSN    = (int)8,
   CLKGEN_CLK_SWITCH_VCO_OVR_POSN                     = (int)9,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_VCO_OVR_LSB_POSN = (int)9,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_VCO_OVR_MSB_POSN = (int)9,
   CLKGEN_CLK_SWITCH_SEL_OVR_POSN                     = (int)10,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_SEL_OVR_LSB_POSN = (int)10,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_SEL_OVR_MSB_POSN = (int)10,
   CLKGEN_CLK_SWITCH_STATUS_CLR_POSN                  = (int)11,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_STATUS_CLR_LSB_POSN = (int)11,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_STATUS_CLR_MSB_POSN = (int)11,
   CLKGEN_CLK_SWITCH_IGNORE_LOCK_POSN                 = (int)12,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_IGNORE_LOCK_LSB_POSN = (int)12,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_IGNORE_LOCK_MSB_POSN = (int)12,
   CLKGEN_CLK_SWITCH_ADPLL_EN_POSN                    = (int)13,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_ADPLL_EN_LSB_POSN = (int)13,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_ADPLL_EN_MSB_POSN = (int)13,
   CLKGEN_CLK_SWITCH_SEL_XTAL_AOV_POSN                = (int)14,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_SEL_XTAL_AOV_LSB_POSN = (int)14,
   CLKGEN_CLK_SWITCH_CLKGEN_CLK_SWITCH_SEL_XTAL_AOV_MSB_POSN = (int)14
};
typedef enum clkgen_clk_switch_posn_enum clkgen_clk_switch_posn;

#define CLKGEN_CLK_SWITCH_DELAY_LSB_MASK         (0x00000001u)
#define CLKGEN_CLK_SWITCH_DELAY_MSB_MASK         (0x00000040u)
#define CLKGEN_CLK_SWITCH_EN_MASK                (0x00000100u)
#define CLKGEN_CLK_SWITCH_VCO_OVR_MASK           (0x00000200u)
#define CLKGEN_CLK_SWITCH_SEL_OVR_MASK           (0x00000400u)
#define CLKGEN_CLK_SWITCH_STATUS_CLR_MASK        (0x00000800u)
#define CLKGEN_CLK_SWITCH_IGNORE_LOCK_MASK       (0x00001000u)
#define CLKGEN_CLK_SWITCH_ADPLL_EN_MASK          (0x00002000u)
#define CLKGEN_CLK_SWITCH_SEL_XTAL_AOV_MASK      (0x00004000u)

enum clkgen_clk_switch_status_posn_enum
{
   CLKGEN_CLK_SWITCH_STATUS_LOCK_ERROR_POSN           = (int)0,
   CLKGEN_CLK_SWITCH_STATUS_CLKGEN_CLK_SWITCH_STATUS_LOCK_ERROR_LSB_POSN = (int)0,
   CLKGEN_CLK_SWITCH_STATUS_CLKGEN_CLK_SWITCH_STATUS_LOCK_ERROR_MSB_POSN = (int)0,
   CLKGEN_CLK_SWITCH_STATUS_SEL_POSN                  = (int)1,
   CLKGEN_CLK_SWITCH_STATUS_CLKGEN_CLK_SWITCH_STATUS_SEL_LSB_POSN = (int)1,
   CLKGEN_CLK_SWITCH_STATUS_CLKGEN_CLK_SWITCH_STATUS_SEL_MSB_POSN = (int)1
};
typedef enum clkgen_clk_switch_status_posn_enum clkgen_clk_switch_status_posn;

#define CLKGEN_CLK_SWITCH_STATUS_LOCK_ERROR_MASK (0x00000001u)
#define CLKGEN_CLK_SWITCH_STATUS_SEL_MASK        (0x00000002u)

enum nco_pll_ctl_posn_enum
{
   NCO_PLL_CTL_CAL_REQ_POSN                           = (int)0,
   NCO_PLL_CTL_NCO_PLL_CTL_CAL_REQ_LSB_POSN           = (int)0,
   NCO_PLL_CTL_NCO_PLL_CTL_CAL_REQ_MSB_POSN           = (int)0,
   NCO_PLL_CTL_PLL_STEP_SEL_POSN                      = (int)1,
   NCO_PLL_CTL_NCO_PLL_CTL_PLL_STEP_SEL_LSB_POSN      = (int)1,
   NCO_PLL_CTL_NCO_PLL_CTL_PLL_STEP_SEL_MSB_POSN      = (int)1,
   NCO_PLL_CTL_LOCK_CHECK_PERIOD_REG_LSB_POSN         = (int)2,
   NCO_PLL_CTL_NCO_PLL_CTL_LOCK_CHECK_PERIOD_REG_LSB_POSN = (int)2,
   NCO_PLL_CTL_LOCK_CHECK_PERIOD_REG_MSB_POSN         = (int)5,
   NCO_PLL_CTL_NCO_PLL_CTL_LOCK_CHECK_PERIOD_REG_MSB_POSN = (int)5,
   NCO_PLL_CTL_CLOCK_48_EN_POSN                       = (int)6,
   NCO_PLL_CTL_NCO_PLL_CTL_CLOCK_48_EN_LSB_POSN       = (int)6,
   NCO_PLL_CTL_NCO_PLL_CTL_CLOCK_48_EN_MSB_POSN       = (int)6,
   NCO_PLL_CTL_PLL_DIV_REG_LSB_POSN                   = (int)8,
   NCO_PLL_CTL_NCO_PLL_CTL_PLL_DIV_REG_LSB_POSN       = (int)8,
   NCO_PLL_CTL_PLL_DIV_REG_MSB_POSN                   = (int)9,
   NCO_PLL_CTL_NCO_PLL_CTL_PLL_DIV_REG_MSB_POSN       = (int)9,
   NCO_PLL_CTL_REF_CLK_EN_POSN                        = (int)10,
   NCO_PLL_CTL_NCO_PLL_CTL_REF_CLK_EN_LSB_POSN        = (int)10,
   NCO_PLL_CTL_NCO_PLL_CTL_REF_CLK_EN_MSB_POSN        = (int)10
};
typedef enum nco_pll_ctl_posn_enum nco_pll_ctl_posn;

#define NCO_PLL_CTL_CAL_REQ_MASK                 (0x00000001u)
#define NCO_PLL_CTL_PLL_STEP_SEL_MASK            (0x00000002u)
#define NCO_PLL_CTL_LOCK_CHECK_PERIOD_REG_LSB_MASK (0x00000004u)
#define NCO_PLL_CTL_LOCK_CHECK_PERIOD_REG_MSB_MASK (0x00000020u)
#define NCO_PLL_CTL_CLOCK_48_EN_MASK             (0x00000040u)
#define NCO_PLL_CTL_PLL_DIV_REG_LSB_MASK         (0x00000100u)
#define NCO_PLL_CTL_PLL_DIV_REG_MSB_MASK         (0x00000200u)
#define NCO_PLL_CTL_REF_CLK_EN_MASK              (0x00000400u)

enum nco_pll_debug_posn_enum
{
   NCO_PLL_DEBUG_TEST_FREQ_REQ_POSN                   = (int)0,
   NCO_PLL_DEBUG_NCO_PLL_DEBUG_TEST_FREQ_REQ_LSB_POSN = (int)0,
   NCO_PLL_DEBUG_NCO_PLL_DEBUG_TEST_FREQ_REQ_MSB_POSN = (int)0,
   NCO_PLL_DEBUG_DEBUG_EN_POSN                        = (int)1,
   NCO_PLL_DEBUG_NCO_PLL_DEBUG_DEBUG_EN_LSB_POSN      = (int)1,
   NCO_PLL_DEBUG_NCO_PLL_DEBUG_DEBUG_EN_MSB_POSN      = (int)1,
   NCO_PLL_DEBUG_CTL_REG_LSB_POSN                     = (int)2,
   NCO_PLL_DEBUG_NCO_PLL_DEBUG_CTL_REG_LSB_POSN       = (int)2,
   NCO_PLL_DEBUG_CTL_REG_MSB_POSN                     = (int)6,
   NCO_PLL_DEBUG_NCO_PLL_DEBUG_CTL_REG_MSB_POSN       = (int)6
};
typedef enum nco_pll_debug_posn_enum nco_pll_debug_posn;

#define NCO_PLL_DEBUG_TEST_FREQ_REQ_MASK         (0x00000001u)
#define NCO_PLL_DEBUG_DEBUG_EN_MASK              (0x00000002u)
#define NCO_PLL_DEBUG_CTL_REG_LSB_MASK           (0x00000004u)
#define NCO_PLL_DEBUG_CTL_REG_MSB_MASK           (0x00000040u)

enum nco_pll_gain_posn_enum
{
   NCO_PLL_GAIN_INTEG_GAIN_REG_LSB_POSN               = (int)0,
   NCO_PLL_GAIN_NCO_PLL_GAIN_INTEG_GAIN_REG_LSB_POSN  = (int)0,
   NCO_PLL_GAIN_INTEG_GAIN_REG_MSB_POSN               = (int)3,
   NCO_PLL_GAIN_NCO_PLL_GAIN_INTEG_GAIN_REG_MSB_POSN  = (int)3,
   NCO_PLL_GAIN_PROP_GAIN_REG_LSB_POSN                = (int)4,
   NCO_PLL_GAIN_NCO_PLL_GAIN_PROP_GAIN_REG_LSB_POSN   = (int)4,
   NCO_PLL_GAIN_PROP_GAIN_REG_MSB_POSN                = (int)6,
   NCO_PLL_GAIN_NCO_PLL_GAIN_PROP_GAIN_REG_MSB_POSN   = (int)6
};
typedef enum nco_pll_gain_posn_enum nco_pll_gain_posn;

#define NCO_PLL_GAIN_INTEG_GAIN_REG_LSB_MASK     (0x00000001u)
#define NCO_PLL_GAIN_INTEG_GAIN_REG_MSB_MASK     (0x00000008u)
#define NCO_PLL_GAIN_PROP_GAIN_REG_LSB_MASK      (0x00000010u)
#define NCO_PLL_GAIN_PROP_GAIN_REG_MSB_MASK      (0x00000040u)

enum nco_pll_status_posn_enum
{
   NCO_PLL_STATUS_FINE_TRIM_OUT_OF_RANGE_POSN         = (int)0,
   NCO_PLL_STATUS_NCO_PLL_STATUS_FINE_TRIM_OUT_OF_RANGE_LSB_POSN = (int)0,
   NCO_PLL_STATUS_NCO_PLL_STATUS_FINE_TRIM_OUT_OF_RANGE_MSB_POSN = (int)0,
   NCO_PLL_STATUS_CAL_BUSY_POSN                       = (int)1,
   NCO_PLL_STATUS_NCO_PLL_STATUS_CAL_BUSY_LSB_POSN    = (int)1,
   NCO_PLL_STATUS_NCO_PLL_STATUS_CAL_BUSY_MSB_POSN    = (int)1,
   NCO_PLL_STATUS_TEST_FREQ_VALID_POSN                = (int)2,
   NCO_PLL_STATUS_NCO_PLL_STATUS_TEST_FREQ_VALID_LSB_POSN = (int)2,
   NCO_PLL_STATUS_NCO_PLL_STATUS_TEST_FREQ_VALID_MSB_POSN = (int)2,
   NCO_PLL_STATUS_PLL_LOCKED_POSN                     = (int)3,
   NCO_PLL_STATUS_NCO_PLL_STATUS_PLL_LOCKED_LSB_POSN  = (int)3,
   NCO_PLL_STATUS_NCO_PLL_STATUS_PLL_LOCKED_MSB_POSN  = (int)3
};
typedef enum nco_pll_status_posn_enum nco_pll_status_posn;

#define NCO_PLL_STATUS_FINE_TRIM_OUT_OF_RANGE_MASK (0x00000001u)
#define NCO_PLL_STATUS_CAL_BUSY_MASK             (0x00000002u)
#define NCO_PLL_STATUS_TEST_FREQ_VALID_MASK      (0x00000004u)
#define NCO_PLL_STATUS_PLL_LOCKED_MASK           (0x00000008u)

enum nco_pll_vco_ctl_posn_enum
{
   NCO_PLL_VCO_CTL_EN_FILT_LOWPASS_IN_POSN            = (int)0,
   NCO_PLL_VCO_CTL_NCO_PLL_VCO_CTL_EN_FILT_LOWPASS_IN_LSB_POSN = (int)0,
   NCO_PLL_VCO_CTL_NCO_PLL_VCO_CTL_EN_FILT_LOWPASS_IN_MSB_POSN = (int)0,
   NCO_PLL_VCO_CTL_EN_IN_POSN                         = (int)1,
   NCO_PLL_VCO_CTL_NCO_PLL_VCO_CTL_EN_IN_LSB_POSN     = (int)1,
   NCO_PLL_VCO_CTL_NCO_PLL_VCO_CTL_EN_IN_MSB_POSN     = (int)1
};
typedef enum nco_pll_vco_ctl_posn_enum nco_pll_vco_ctl_posn;

#define NCO_PLL_VCO_CTL_EN_FILT_LOWPASS_IN_MASK  (0x00000001u)
#define NCO_PLL_VCO_CTL_EN_IN_MASK               (0x00000002u)

enum nco_pll_acc_value_posn_enum
{
   NCO_PLL_ACC_VALUE_LSB_POSN                         = (int)0,
   NCO_PLL_ACC_VALUE_NCO_PLL_ACC_VALUE_LSB_POSN       = (int)0,
   NCO_PLL_ACC_VALUE_MSB_POSN                         = (int)15,
   NCO_PLL_ACC_VALUE_NCO_PLL_ACC_VALUE_MSB_POSN       = (int)15
};
typedef enum nco_pll_acc_value_posn_enum nco_pll_acc_value_posn;

#define NCO_PLL_ACC_VALUE_LSB_MASK               (0x00000001u)
#define NCO_PLL_ACC_VALUE_MSB_MASK               (0x00008000u)

enum nco_pll_clk_vco_count_delta_posn_enum
{
   NCO_PLL_CLK_VCO_COUNT_DELTA_LSB_POSN               = (int)0,
   NCO_PLL_CLK_VCO_COUNT_DELTA_NCO_PLL_CLK_VCO_COUNT_DELTA_LSB_POSN = (int)0,
   NCO_PLL_CLK_VCO_COUNT_DELTA_MSB_POSN               = (int)15,
   NCO_PLL_CLK_VCO_COUNT_DELTA_NCO_PLL_CLK_VCO_COUNT_DELTA_MSB_POSN = (int)15
};
typedef enum nco_pll_clk_vco_count_delta_posn_enum nco_pll_clk_vco_count_delta_posn;

#define NCO_PLL_CLK_VCO_COUNT_DELTA_LSB_MASK     (0x00000001u)
#define NCO_PLL_CLK_VCO_COUNT_DELTA_MSB_MASK     (0x00008000u)

enum nco_pll_coarse_trim_posn_enum
{
   NCO_PLL_COARSE_TRIM_LSB_POSN                       = (int)0,
   NCO_PLL_COARSE_TRIM_NCO_PLL_COARSE_TRIM_LSB_POSN   = (int)0,
   NCO_PLL_COARSE_TRIM_MSB_POSN                       = (int)6,
   NCO_PLL_COARSE_TRIM_NCO_PLL_COARSE_TRIM_MSB_POSN   = (int)6
};
typedef enum nco_pll_coarse_trim_posn_enum nco_pll_coarse_trim_posn;

#define NCO_PLL_COARSE_TRIM_LSB_MASK             (0x00000001u)
#define NCO_PLL_COARSE_TRIM_MSB_MASK             (0x00000040u)

enum nco_pll_coarse_trim_reg_posn_enum
{
   NCO_PLL_COARSE_TRIM_REG_LSB_POSN                   = (int)0,
   NCO_PLL_COARSE_TRIM_REG_NCO_PLL_COARSE_TRIM_REG_LSB_POSN = (int)0,
   NCO_PLL_COARSE_TRIM_REG_MSB_POSN                   = (int)6,
   NCO_PLL_COARSE_TRIM_REG_NCO_PLL_COARSE_TRIM_REG_MSB_POSN = (int)6
};
typedef enum nco_pll_coarse_trim_reg_posn_enum nco_pll_coarse_trim_reg_posn;

#define NCO_PLL_COARSE_TRIM_REG_LSB_MASK         (0x00000001u)
#define NCO_PLL_COARSE_TRIM_REG_MSB_MASK         (0x00000040u)

enum nco_pll_en_posn_enum
{
   NCO_PLL_EN_POSN                                    = (int)0,
   NCO_PLL_EN_NCO_PLL_EN_LSB_POSN                     = (int)0,
   NCO_PLL_EN_NCO_PLL_EN_MSB_POSN                     = (int)0
};
typedef enum nco_pll_en_posn_enum nco_pll_en_posn;

#define NCO_PLL_EN_MASK                          (0x00000001u)

enum nco_pll_frac_inc_reg_posn_enum
{
   NCO_PLL_FRAC_INC_REG_LSB_POSN                      = (int)0,
   NCO_PLL_FRAC_INC_REG_NCO_PLL_FRAC_INC_REG_LSB_POSN = (int)0,
   NCO_PLL_FRAC_INC_REG_MSB_POSN                      = (int)23,
   NCO_PLL_FRAC_INC_REG_NCO_PLL_FRAC_INC_REG_MSB_POSN = (int)23
};
typedef enum nco_pll_frac_inc_reg_posn_enum nco_pll_frac_inc_reg_posn;

#define NCO_PLL_FRAC_INC_REG_LSB_MASK            (0x00000001u)
#define NCO_PLL_FRAC_INC_REG_MSB_MASK            (0x00800000u)

enum nco_pll_init_acc_value_reg_posn_enum
{
   NCO_PLL_INIT_ACC_VALUE_REG_LSB_POSN                = (int)0,
   NCO_PLL_INIT_ACC_VALUE_REG_NCO_PLL_INIT_ACC_VALUE_REG_LSB_POSN = (int)0,
   NCO_PLL_INIT_ACC_VALUE_REG_MSB_POSN                = (int)15,
   NCO_PLL_INIT_ACC_VALUE_REG_NCO_PLL_INIT_ACC_VALUE_REG_MSB_POSN = (int)15
};
typedef enum nco_pll_init_acc_value_reg_posn_enum nco_pll_init_acc_value_reg_posn;

#define NCO_PLL_INIT_ACC_VALUE_REG_LSB_MASK      (0x00000001u)
#define NCO_PLL_INIT_ACC_VALUE_REG_MSB_MASK      (0x00008000u)

enum audio_sys_adpll__access_ctrl_enum_posn_enum
{
   AUDIO_SYS_ADPLL__P0_ACCESS_PERMISSION_POSN         = (int)0,
   AUDIO_SYS_ADPLL__ACCESS_CTRL_ENUM_AUDIO_SYS_ADPLL__P0_ACCESS_PERMISSION_LSB_POSN = (int)0,
   AUDIO_SYS_ADPLL__ACCESS_CTRL_ENUM_AUDIO_SYS_ADPLL__P0_ACCESS_PERMISSION_MSB_POSN = (int)0,
   AUDIO_SYS_ADPLL__P1_ACCESS_PERMISSION_POSN         = (int)1,
   AUDIO_SYS_ADPLL__ACCESS_CTRL_ENUM_AUDIO_SYS_ADPLL__P1_ACCESS_PERMISSION_LSB_POSN = (int)1,
   AUDIO_SYS_ADPLL__ACCESS_CTRL_ENUM_AUDIO_SYS_ADPLL__P1_ACCESS_PERMISSION_MSB_POSN = (int)1,
   AUDIO_SYS_ADPLL__P2_ACCESS_PERMISSION_POSN         = (int)2,
   AUDIO_SYS_ADPLL__ACCESS_CTRL_ENUM_AUDIO_SYS_ADPLL__P2_ACCESS_PERMISSION_LSB_POSN = (int)2,
   AUDIO_SYS_ADPLL__ACCESS_CTRL_ENUM_AUDIO_SYS_ADPLL__P2_ACCESS_PERMISSION_MSB_POSN = (int)2,
   AUDIO_SYS_ADPLL__P3_ACCESS_PERMISSION_POSN         = (int)3,
   AUDIO_SYS_ADPLL__ACCESS_CTRL_ENUM_AUDIO_SYS_ADPLL__P3_ACCESS_PERMISSION_LSB_POSN = (int)3,
   AUDIO_SYS_ADPLL__ACCESS_CTRL_ENUM_AUDIO_SYS_ADPLL__P3_ACCESS_PERMISSION_MSB_POSN = (int)3
};
typedef enum audio_sys_adpll__access_ctrl_enum_posn_enum audio_sys_adpll__access_ctrl_enum_posn;

#define AUDIO_SYS_ADPLL__P0_ACCESS_PERMISSION_MASK (0x00000001u)
#define AUDIO_SYS_ADPLL__P1_ACCESS_PERMISSION_MASK (0x00000002u)
#define AUDIO_SYS_ADPLL__P2_ACCESS_PERMISSION_MASK (0x00000004u)
#define AUDIO_SYS_ADPLL__P3_ACCESS_PERMISSION_MASK (0x00000008u)

enum audio_sys_adpll__p0_access_permission_enum
{
   AUDIO_SYS_ADPLL__P0_ACCESS_BLOCKED       = (int)0x0,
   AUDIO_SYS_ADPLL__P0_ACCESS_UNBLOCKED     = (int)0x1,
   MAX_AUDIO_SYS_ADPLL__P0_ACCESS_PERMISSION = (int)0x1
};
#define NUM_AUDIO_SYS_ADPLL__P0_ACCESS_PERMISSION (0x2)
typedef enum audio_sys_adpll__p0_access_permission_enum audio_sys_adpll__p0_access_permission;


enum audio_sys_adpll__p1_access_permission_enum
{
   AUDIO_SYS_ADPLL__P1_ACCESS_BLOCKED       = (int)0x0,
   AUDIO_SYS_ADPLL__P1_ACCESS_UNBLOCKED     = (int)0x1,
   MAX_AUDIO_SYS_ADPLL__P1_ACCESS_PERMISSION = (int)0x1
};
#define NUM_AUDIO_SYS_ADPLL__P1_ACCESS_PERMISSION (0x2)
typedef enum audio_sys_adpll__p1_access_permission_enum audio_sys_adpll__p1_access_permission;


enum audio_sys_adpll__p2_access_permission_enum
{
   AUDIO_SYS_ADPLL__P2_ACCESS_BLOCKED       = (int)0x0,
   AUDIO_SYS_ADPLL__P2_ACCESS_UNBLOCKED     = (int)0x1,
   MAX_AUDIO_SYS_ADPLL__P2_ACCESS_PERMISSION = (int)0x1
};
#define NUM_AUDIO_SYS_ADPLL__P2_ACCESS_PERMISSION (0x2)
typedef enum audio_sys_adpll__p2_access_permission_enum audio_sys_adpll__p2_access_permission;


enum audio_sys_adpll__p3_access_permission_enum
{
   AUDIO_SYS_ADPLL__P3_ACCESS_BLOCKED       = (int)0x0,
   AUDIO_SYS_ADPLL__P3_ACCESS_UNBLOCKED     = (int)0x1,
   MAX_AUDIO_SYS_ADPLL__P3_ACCESS_PERMISSION = (int)0x1
};
#define NUM_AUDIO_SYS_ADPLL__P3_ACCESS_PERMISSION (0x2)
typedef enum audio_sys_adpll__p3_access_permission_enum audio_sys_adpll__p3_access_permission;


enum audio_sys_adpll__mutex_lock_enum_enum
{
   AUDIO_SYS_ADPLL__MUTEX_AVAILABLE         = (int)0x0,
   AUDIO_SYS_ADPLL__MUTEX_CLAIMED_BY_P0     = (int)0x1,
   AUDIO_SYS_ADPLL__MUTEX_CLAIMED_BY_P1     = (int)0x2,
   AUDIO_SYS_ADPLL__MUTEX_CLAIMED_BY_P2     = (int)0x4,
   AUDIO_SYS_ADPLL__MUTEX_CLAIMED_BY_P3     = (int)0x8,
   AUDIO_SYS_ADPLL__MUTEX_DISABLED          = (int)0xF,
   MAX_AUDIO_SYS_ADPLL__MUTEX_LOCK_ENUM     = (int)0xF
};
typedef enum audio_sys_adpll__mutex_lock_enum_enum audio_sys_adpll__mutex_lock_enum;


enum adpll_access_ctrl_posn_enum
{
   ADPLL_ACCESS_CTRL_AUDIO_SYS_ADPLL__P0_ACCESS_PERMISSION_LSB_POSN = (int)0,
   ADPLL_ACCESS_CTRL_AUDIO_SYS_ADPLL__P0_ACCESS_PERMISSION_MSB_POSN = (int)0,
   ADPLL_ACCESS_CTRL_AUDIO_SYS_ADPLL__P1_ACCESS_PERMISSION_LSB_POSN = (int)1,
   ADPLL_ACCESS_CTRL_AUDIO_SYS_ADPLL__P1_ACCESS_PERMISSION_MSB_POSN = (int)1,
   ADPLL_ACCESS_CTRL_AUDIO_SYS_ADPLL__P2_ACCESS_PERMISSION_LSB_POSN = (int)2,
   ADPLL_ACCESS_CTRL_AUDIO_SYS_ADPLL__P2_ACCESS_PERMISSION_MSB_POSN = (int)2,
   ADPLL_ACCESS_CTRL_AUDIO_SYS_ADPLL__P3_ACCESS_PERMISSION_LSB_POSN = (int)3,
   ADPLL_ACCESS_CTRL_AUDIO_SYS_ADPLL__P3_ACCESS_PERMISSION_MSB_POSN = (int)3
};
typedef enum adpll_access_ctrl_posn_enum adpll_access_ctrl_posn;


#endif /* IO_DEFS_MODULE_AUDIO_SYS_ADPLL */
#endif /* __IO_DEFS_H__IO_DEFS_MODULE_AUDIO_SYS_ADPLL */








#if !defined(__IO_DEFS_H__IO_DEFS_MODULE_K32_MISC) 
#define __IO_DEFS_H__IO_DEFS_MODULE_K32_MISC

/* -- k32_misc -- Kalimba 32-bit Misc Control registers -- */

enum clock_divide_rate_enum_enum
{
   CLOCK_STOPPED                            = (int)0x0,
   CLOCK_RATE_MAX                           = (int)0x1,
   CLOCK_RATE_HALF                          = (int)0x2,
   CLOCK_RATE_RESERVED                      = (int)0x3,
   MAX_CLOCK_DIVIDE_RATE_ENUM               = (int)0x3
};
#define NUM_CLOCK_DIVIDE_RATE_ENUM (0x4)
typedef enum clock_divide_rate_enum_enum clock_divide_rate_enum;


enum allow_goto_shallow_sleep_posn_enum
{
   ALLOW_GOTO_SHALLOW_SLEEP_POSN                      = (int)0,
   ALLOW_GOTO_SHALLOW_SLEEP_ALLOW_GOTO_SHALLOW_SLEEP_LSB_POSN = (int)0,
   ALLOW_GOTO_SHALLOW_SLEEP_ALLOW_GOTO_SHALLOW_SLEEP_MSB_POSN = (int)0
};
typedef enum allow_goto_shallow_sleep_posn_enum allow_goto_shallow_sleep_posn;

#define ALLOW_GOTO_SHALLOW_SLEEP_MASK            (0x00000001u)

enum clock_cont_shallow_sleep_rate_posn_enum
{
   CLOCK_CONT_SHALLOW_SLEEP_RATE_LSB_POSN             = (int)0,
   CLOCK_CONT_SHALLOW_SLEEP_RATE_CLOCK_CONT_SHALLOW_SLEEP_RATE_LSB_POSN = (int)0,
   CLOCK_CONT_SHALLOW_SLEEP_RATE_MSB_POSN             = (int)7,
   CLOCK_CONT_SHALLOW_SLEEP_RATE_CLOCK_CONT_SHALLOW_SLEEP_RATE_MSB_POSN = (int)7
};
typedef enum clock_cont_shallow_sleep_rate_posn_enum clock_cont_shallow_sleep_rate_posn;

#define CLOCK_CONT_SHALLOW_SLEEP_RATE_LSB_MASK   (0x00000001u)
#define CLOCK_CONT_SHALLOW_SLEEP_RATE_MSB_MASK   (0x00000080u)

enum clock_stop_wind_down_sequence_en_posn_enum
{
   CLOCK_STOP_WIND_DOWN_SEQUENCE_EN_POSN              = (int)0,
   CLOCK_STOP_WIND_DOWN_SEQUENCE_EN_CLOCK_STOP_WIND_DOWN_SEQUENCE_EN_LSB_POSN = (int)0,
   CLOCK_STOP_WIND_DOWN_SEQUENCE_EN_CLOCK_STOP_WIND_DOWN_SEQUENCE_EN_MSB_POSN = (int)0
};
typedef enum clock_stop_wind_down_sequence_en_posn_enum clock_stop_wind_down_sequence_en_posn;

#define CLOCK_STOP_WIND_DOWN_SEQUENCE_EN_MASK    (0x00000001u)

enum disable_mutex_and_access_immunity_posn_enum
{
   DISABLE_MUTEX_AND_ACCESS_IMMUNITY_POSN             = (int)0,
   DISABLE_MUTEX_AND_ACCESS_IMMUNITY_DISABLE_MUTEX_AND_ACCESS_IMMUNITY_LSB_POSN = (int)0,
   DISABLE_MUTEX_AND_ACCESS_IMMUNITY_DISABLE_MUTEX_AND_ACCESS_IMMUNITY_MSB_POSN = (int)0
};
typedef enum disable_mutex_and_access_immunity_posn_enum disable_mutex_and_access_immunity_posn;

#define DISABLE_MUTEX_AND_ACCESS_IMMUNITY_MASK   (0x00000001u)

enum goto_shallow_sleep_posn_enum
{
   GOTO_SHALLOW_SLEEP_POSN                            = (int)0,
   GOTO_SHALLOW_SLEEP_GOTO_SHALLOW_SLEEP_LSB_POSN     = (int)0,
   GOTO_SHALLOW_SLEEP_GOTO_SHALLOW_SLEEP_MSB_POSN     = (int)0
};
typedef enum goto_shallow_sleep_posn_enum goto_shallow_sleep_posn;

#define GOTO_SHALLOW_SLEEP_MASK                  (0x00000001u)

enum pmwin_enable_posn_enum
{
   PMWIN_ENABLE_POSN                                  = (int)0,
   PMWIN_ENABLE_PMWIN_ENABLE_LSB_POSN                 = (int)0,
   PMWIN_ENABLE_PMWIN_ENABLE_MSB_POSN                 = (int)0
};
typedef enum pmwin_enable_posn_enum pmwin_enable_posn;

#define PMWIN_ENABLE_MASK                        (0x00000001u)

enum processor_id_posn_enum
{
   PROCESSOR_ID_POSN                                  = (int)0,
   PROCESSOR_ID_PROCESSOR_ID_LSB_POSN                 = (int)0,
   PROCESSOR_ID_PROCESSOR_ID_MSB_POSN                 = (int)0
};
typedef enum processor_id_posn_enum processor_id_posn;

#define PROCESSOR_ID_MASK                        (0x00000001u)

enum proc_deep_sleep_en_posn_enum
{
   PROC_DEEP_SLEEP_EN_POSN                            = (int)0,
   PROC_DEEP_SLEEP_EN_PROC_DEEP_SLEEP_EN_LSB_POSN     = (int)0,
   PROC_DEEP_SLEEP_EN_PROC_DEEP_SLEEP_EN_MSB_POSN     = (int)0
};
typedef enum proc_deep_sleep_en_posn_enum proc_deep_sleep_en_posn;

#define PROC_DEEP_SLEEP_EN_MASK                  (0x00000001u)

enum shallow_sleep_status_posn_enum
{
   SHALLOW_SLEEP_STATUS_POSN                          = (int)0,
   SHALLOW_SLEEP_STATUS_SHALLOW_SLEEP_STATUS_LSB_POSN = (int)0,
   SHALLOW_SLEEP_STATUS_SHALLOW_SLEEP_STATUS_MSB_POSN = (int)0
};
typedef enum shallow_sleep_status_posn_enum shallow_sleep_status_posn;

#define SHALLOW_SLEEP_STATUS_MASK                (0x00000001u)

#endif /* IO_DEFS_MODULE_K32_MISC */


