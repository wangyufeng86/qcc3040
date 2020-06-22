/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup patch Patch
 *
 * \file patch_loader.h
 * \ingroup patch
 *
 * Public header file for the patch_loader component of the audio subsystem
 *
 * \section sec1 Contains
 *
 *       patch_loader<br>
 *
 */

#ifndef PATCHLOADER_H
#define PATCHLOADER_H

/****************************************************************************
Include Files
*/

#include "types.h"

/**
* \brief  Installs the patch code and enables the patches.
*
* This function is called once the patch file data has been completely written
* to pram starting with pramAddress
*
* On completion, all of the patch code is in place in PM and any specified
* hardware and software patches have been set up and enabled.
*/
#define RAW_PATCH_FILE_HDR_SIZE 14
#define RAW_PATCH_FILE_HDR_SIZE_BYTES 14*2

#ifdef __KCC__
#define SECTION_PM_PATCH _Pragma("codesection CODE_HEAP")
#else /* __KCC__ */
#define SECTION_PM_PATCH
#endif /* __KCC__ */

/* This is where the PM Patch code starts */
SECTION_PM_PATCH void section_patch(void);

uint32 initialize_patches(unsigned pramAddress);


#endif /* PATCHLOADER_H */
