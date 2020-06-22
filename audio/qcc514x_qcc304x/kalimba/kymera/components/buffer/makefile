############################################################################
# CONFIDENTIAL
# Copyright (c) 2008 - 2017 Qualcomm Technologies International, Ltd.
#
############################################################################
#
# COMPONENT:    makefile
# MODULE:       Buffer
# DESCRIPTION:  Library makefile.
#
############################################################################


#########################################################################
# Define root directory (relative so we can be installed anywhere)
#########################################################################

BUILD_ROOT = ../../build
include $(BUILD_ROOT)/roots.mkf

#########################################################################
# Patching
#########################################################################

PATCH_SRC += cbuffer_asm.asm
PATCH_SRC += $(if $(findstring $(CHIP_ARCH), hydra), hydra/mmu_buff_asm.asm)
PATCH_SRC += cbuffer_ex_asm.asm
PATCH_SRC += $(if $(findstring $(CHIP_ARCH), hydra), hydra/mmu_buff_ex_asm.asm)

GEN_PIDS = $(PATCH_DIR)/buffer_asm_patch_ids.txt

#########################################################################
# Enter source files and directories and header directories here.
#
# Makerules will add the standard interface paths
#########################################################################

# C files we include in all builds
C_SRC += cbuffer.c
C_SRC += buffer_metadata.c
C_SRC += buffer_metadata_eof.c
C_SRC += cbuffer_ex.c
C_SRC += $(if $(findstring $(BUILD_METADATA_DUALCORE), true), buffer_metadata_kip.c, )


# Assembler files we need for a normal kalimba build
S_SRC += cbuffer_asm.asm
S_SRC += cbuffer_ex_asm.asm

# choose a target
S_PATH += $(if $(findstring $(CHIP_ARCH), bluecore), $(call myabspath,./bc), )
S_PATH += $(if $(findstring $(CHIP_ARCH), hydra), $(call myabspath,./hydra), )
S_SRC += $(if $(findstring $(CHIP_ARCH), bluecore), mmu_buff_asm.asm,)
S_SRC += $(if $(findstring $(CHIP_ARCH), hydra), mmu_buff_asm.asm,)
S_SRC += $(if $(findstring $(CHIP_ARCH), hydra), mmu_buff_ex_asm.asm,)

# build either the BC specific...
C_PATH += $(if $(findstring $(CHIP_ARCH), bluecore), $(call myabspath,./bc), )
C_SRC += $(if $(findstring $(CHIP_ARCH), bluecore), cbuff.c,)
# ... or build the Hydra specific
C_PATH += $(if $(findstring $(CHIP_ARCH), hydra), $(call myabspath,./hydra), )
C_SRC += $(if $(findstring $(CHIP_ARCH), hydra), cbuff.c,)
C_SRC += $(if $(findstring $(CHIP_ARCH), hydra), mmu_buff.c,)

# path for platform specific private headers
H_PATH += $(if $(findstring $(CHIP_ARCH), hydra), $(call myabspath,./hydra), )

#########################################################################
# Enter final target file here (only 1 target should be specified)
#########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = buffer

#########################################################################
# Include the standard definitions and rules
#########################################################################

include $(BUILD_ROOT)/makerules.mkf
