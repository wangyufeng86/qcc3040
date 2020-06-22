/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * /file
 *
 * Provide macros to allow the chip model and variant to be read at runtime.
 */

#ifndef _CHIP_H_
#define _CHIP_H_

/*
 * Define some macros that allow run-time determination of the chip.
 */
#define CHIPVSN_MAJOR  (0x00ff)
#define CHIPVSN_VARIANT  (0x0f00)
#define CHIPVSN_MINOR  (0xf000)

/* Main chips */
#define ISCHIP_CSRA6810X        (0x0046)
#define ISCHIP_CSRA68100_MASK       CHIPVSN_MAJOR
#define ISCHIP_QCC512X_QCC302X  (0x0049)
#define ISCHIP_QCC512X_QCC302X_MASK CHIPVSN_MAJOR
#define ISCHIP_QCC514X_QCC304X  (0x004b)
#define ISCHIP_QCC514X_QCC304X_MASK CHIPVSN_MAJOR

/* Variant chips */
#define ISCHIP_CSRA68105        (0x3146)
#define ISCHIP_CSRA68105_MASK       (CHIPVSN_MAJOR|CHIPVSN_VARIANT|CHIPVSN_MINOR)
#define ISCHIP_QCC5126_QCC5127  (0x2149)
#define ISCHIP_QCC5126_QCC5127_MASK (CHIPVSN_MAJOR|CHIPVSN_VARIANT|CHIPVSN_MINOR)

/* Accessor functions */
#define IsChip(x) ((hal_get_reg_sub_sys_chip_version() & ISCHIP_##x##_MASK) == ISCHIP_##x)

#define GetChip(x) (hal_get_reg_sub_sys_chip_version())

#endif /* _CHIP_H_ */
