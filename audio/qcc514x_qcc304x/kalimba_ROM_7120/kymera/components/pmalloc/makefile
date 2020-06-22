############################################################################
# Copyright (c) 2008 - 2018 Qualcomm Technologies International, Ltd.
############################################################################
#
# COMPONENT:    makefile
# MODULE:       pmalloc
# DESCRIPTION:  Library makefile.
#
############################################################################


#########################################################################
# Define root directory (relative so we can be installed anywhere)
#########################################################################

BUILD_ROOT = ../../build
include $(BUILD_ROOT)/roots.mkf

#########################################################################
# Enter source files and directories and header directories here.
#
# Makerules will add the standard interface paths
#########################################################################

C_PATH += $(call myabspath,./$(CHIP_NAME))
C_SRC +=    pl_malloc.c
C_SRC +=    heap_alloc_$(CHIP_ARCH).c
C_SRC +=    $(if $(EXTERNAL_MEM), pl_ext_malloc.c)


S_SRC +=    pl_malloc_debug.asm

#########################################################################
# Enter final target file here (only 1 target should be specified)
#########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = pmalloc

#########################################################################
# Build tools extra switches
#########################################################################

#########################################################################
# Include the standard definitions and rules
#########################################################################

include $(BUILD_ROOT)/makerules.mkf
