# Define the list of features added to a product when building for QCC300x
CONFIG_FEATURES+=CONFIG_QCC300X

# Pull in the non-Kymera build configuration
path = $(dir $(lastword $(MAKEFILE_LIST)))
include $(path)/no_kymera.mak

# remove unsupported libs
CONFIG_DIRS_FILTER := $(CONFIG_DIRS_FILTER) anc
