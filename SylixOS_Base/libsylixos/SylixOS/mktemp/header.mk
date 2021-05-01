#*********************************************************************************************************
#
#                                    �й������Դ��֯
#
#                                   Ƕ��ʽʵʱ����ϵͳ
#
#                                SylixOS(TM)  LW : long wing
#
#                               Copyright All Rights Reserved
#
#--------------�ļ���Ϣ--------------------------------------------------------------------------------
#
# ��   ��   ��: header.mk
#
# ��   ��   ��: Jiao.JinXing(������)
#
# �ļ���������: 2016 �� 08 �� 24 ��
#
# ��        ��: makefile ģ���ײ�
#*********************************************************************************************************

#*********************************************************************************************************
# Check configure
#*********************************************************************************************************
check_defined = \
    $(foreach 1,$1,$(__check_defined))
__check_defined = \
    $(if $(value $1),, \
      $(error Undefined $1$(if $(value 2), ($(strip $2)))))

$(call check_defined, CONFIG_MK_EXIST, Please configure this project in RealEvo-IDE or create a config.mk file!)
$(call check_defined, SYLIXOS_BASE_PATH, SylixOS base project path)
$(call check_defined, TOOLCHAIN_PREFIX, the prefix name of toolchain)
$(call check_defined, DEBUG_LEVEL, debug level(debug or release))

#*********************************************************************************************************
# All *.mk files
#*********************************************************************************************************
APPLICATION_MK    = $(MKTEMP)/application.mk
LIBRARY_MK        = $(MKTEMP)/library.mk
STATIC_LIBRARY_MK = $(MKTEMP)/static-library.mk
KERNEL_MODULE_MK  = $(MKTEMP)/kernel-module.mk
KERNEL_LIBRARY_MK = $(MKTEMP)/kernel-library.mk
UNIT_TEST_MK      = $(MKTEMP)/unit-test.mk
GTEST_MK          = $(MKTEMP)/gtest.mk
LIBSYLIXOS_MK     = $(MKTEMP)/libsylixos.mk
DUMMY_MK          = $(MKTEMP)/dummy.mk
BSP_MK            = $(MKTEMP)/bsp.mk
BARE_METAL_MK     = $(MKTEMP)/bare-metal.mk
EXTENSION_MK      = $(MKTEMP)/extension.mk
LITE_BSP_MK       = $(MKTEMP)/lite-bsp.mk
END_MK            = $(MKTEMP)/end.mk
CLEAR_VARS_MK     = $(MKTEMP)/clear-vars.mk

#*********************************************************************************************************
# Build paths
#*********************************************************************************************************
ifeq ($(DEBUG_LEVEL), debug)
OUTDIR = Debug
else
OUTDIR = Release
endif

ifeq ($(CUSTOM_OUT_BASE),)
TARGET_WORD_POS = 3
OUTPATH = ./$(OUTDIR)
OBJPATH = $(OUTPATH)/obj
DEPPATH = $(OUTPATH)/dep
else
TARGET_WORD_POS = 4
OUTPATH = $(OUTDIR)
OBJPATH = $(CUSTOM_OUT_BASE)$(OUTPATH)/obj
DEPPATH = $(CUSTOM_OUT_BASE)$(OUTPATH)/dep
endif

#*********************************************************************************************************
# Define some useful variables
#*********************************************************************************************************
BIAS  = /
EMPTY =
SPACE = $(EMPTY) $(EMPTY)

SYLIXOS_BASE_PATH := $(subst \,/,$(subst $(SPACE),\ ,$(SYLIXOS_BASE_PATH)))

__TARGET    = $(word $(TARGET_WORD_POS),$(subst $(BIAS),$(SPACE),$(@)))
__DEP       = $(addprefix $(DEPPATH)/$(__TARGET)/, $(addsuffix .d, $(basename $(<))))
ifneq (,$(findstring cl6x,$(TOOLCHAIN_PREFIX)))
__PP        = $(addprefix $(DEPPATH)/$(__TARGET)/, $(addsuffix .pp, $(basename $(<))))
endif
__LIBRARIES = $($(@F)_DEPEND_LIB_PATH) $($(@F)_DEPEND_LIB)
__OBJS      = $($(@F)_OBJS)
__CPUFLAGS  = $($(@F)_CPUFLAGS)
__DSYMBOL   = $($(@F)_DSYMBOL)
__LINKFLAGS = $($(@F)_LINKFLAGS)
__LD        = $($(@F)_LD)

__PRE_LINK_CMD   = $($(@F)_PRE_LINK_CMD)
__POST_LINK_CMD  = $($(@F)_POST_LINK_CMD)

__PRE_STRIP_CMD  = $($(@F)_PRE_STRIP_CMD)
__POST_STRIP_CMD = $($(@F)_POST_STRIP_CMD)

#*********************************************************************************************************
# Do not export the following environment variables 
#*********************************************************************************************************
unexport CPATH
unexport C_INCLUDE_PATH
unexport CPLUS_INCLUDE_PATH
unexport OBJC_INCLUDE_PATH
unexport LIBRARY_PATH
unexport LD_LIBRARY_PATH

#*********************************************************************************************************
# Include toolchain mk
#*********************************************************************************************************
ifneq (,$(findstring cl6x,$(TOOLCHAIN_PREFIX)))
include $(MKTEMP)/cl6x.mk
else
include $(MKTEMP)/gcc.mk
endif

#*********************************************************************************************************
# Include arch.mk
#*********************************************************************************************************
include $(MKTEMP)/arch.mk

#*********************************************************************************************************
# End
#*********************************************************************************************************
