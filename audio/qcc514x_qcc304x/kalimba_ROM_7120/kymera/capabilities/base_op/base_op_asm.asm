/****************************************************************************
 * Copyright (c) 2014 - 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
#include "opmgr_operator_data_asm_defs.h"

.MODULE $M.base_op.base_op_get_class_ext;
    .CODESEGMENT PM;
// *****************************************************************************
// MODULE:
//    $M.base_op.base_op_get_class_ext
//
// DESCRIPTION:
//    This function is equivalent $_base_op_set_class_ext but guarantees 
//    to affect only r0.
//
// INPUTS:
//    - r0 = address to the OPERATOR_DATA structure
//
// OUTPUTS:
//    - r0 = address to the class-specific per-instance data
//
// TRASHED REGISTERS:
//    None.
// *****************************************************************************
$base_op.base_op_get_class_ext:
    r0 = M[r0 + $opmgr_operator_data.OPERATOR_DATA_struct.CAP_CLASS_EXT_FIELD];
    rts;
.ENDMODULE;

.MODULE $M.base_op.base_op_get_instance_data;
    .CODESEGMENT PM;
// *****************************************************************************
// MODULE:
//    $M.base_op.base_op_get_instance_data
//
// DESCRIPTION:
//    This function is equivalent $_base_op_get_instance_data but guarantees 
//    to affect only r0.
//
// INPUTS:
//    - r0 = address to the OPERATOR_DATA structure
//
// OUTPUTS:
//    - r0 = address to the capability-specific per-instance data
//
// TRASHED REGISTERS:
//    None.
// *****************************************************************************
$base_op.base_op_get_instance_data:
    r0 = M[r0 + $opmgr_operator_data.OPERATOR_DATA_struct.EXTRA_OP_DATA_FIELD];
    rts;
.ENDMODULE;