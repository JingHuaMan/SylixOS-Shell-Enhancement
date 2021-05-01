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
# ��   ��   ��: extension.mk
#
# ��   ��   ��: Jiao.JinXing(������)
#
# �ļ���������: 2017 �� 02 �� 20 ��
#
# ��        ��: lite �汾 bsp ��չ��Ŀ�� makefile ģ��
#*********************************************************************************************************

#*********************************************************************************************************
# Check configure
#*********************************************************************************************************
$(call check_defined, SYLIXOS_LITE_BSP_PATH, SylixOS lite bsp project path)

#*********************************************************************************************************
# Include common.mk
#*********************************************************************************************************
include $(MKTEMP)/common.mk

#*********************************************************************************************************
# Depend and compiler parameter (cplusplus in kernel MUST NOT use exceptions and rtti)
#*********************************************************************************************************
ifeq ($($(target)_USE_CXX_EXCEPT), yes)
$(target)_CXX_EXCEPT  := $(TOOLCHAIN_NO_CXX_EXCEPT_CFLAGS)
else
$(target)_CXX_EXCEPT  := $(TOOLCHAIN_NO_CXX_EXCEPT_CFLAGS)
endif

ifeq ($($(target)_USE_GCOV), yes)
$(target)_GCOV_FLAGS  := $(TOOLCHAIN_NO_GCOV_CFLAGS)
else
$(target)_GCOV_FLAGS  := $(TOOLCHAIN_NO_GCOV_CFLAGS)
endif

ifeq ($($(target)_USE_OMP), yes)
$(target)_OMP_FLAGS   := $(TOOLCHAIN_NO_OMP_CFLAGS)
else
$(target)_OMP_FLAGS   := $(TOOLCHAIN_NO_OMP_CFLAGS)
endif

$(target)_DSYMBOL     += $(TOOLCHAIN_DEF_SYMBOL)SYLIXOS_EXTENSION
$(target)_CPUFLAGS    := $(ARCH_CPUFLAGS) $(ARCH_KERNEL_CFLAGS)
$(target)_COMMONFLAGS := $($(target)_CPUFLAGS) $(ARCH_COMMONFLAGS) $(TOOLCHAIN_OPTIMIZE) $(TOOLCHAIN_COMMONFLAGS) $($(target)_GCOV_FLAGS) $($(target)_OMP_FLAGS)
$(target)_ASFLAGS     := $($(target)_COMMONFLAGS) $(TOOLCHAIN_ASFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH)
$(target)_CFLAGS      := $($(target)_COMMONFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CFLAGS)
$(target)_CXXFLAGS    := $($(target)_COMMONFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CXX_EXCEPT) $($(target)_CXXFLAGS)

#*********************************************************************************************************
# Add extension certificate file to source file list
#*********************************************************************************************************
EXT_CERT_SRC    = SylixOS/bsp/bspCert.c
ifeq (,$(findstring $(EXT_CERT_SRC),$(LOCAL_SRCS)))
LOCAL_SRCS     += $(EXT_CERT_SRC)
$(target)_OBJS += $(addprefix $(OBJPATH)/$(target)/, $(addsuffix .o, $(basename $(EXT_CERT_SRC))))
endif

#*********************************************************************************************************
# Depend library search paths
#*********************************************************************************************************
$(target)_DEPEND_LIB_PATH += $(LOCAL_DEPEND_LIB_PATH)

#*********************************************************************************************************
# Depend libraries
#*********************************************************************************************************
$(target)_DEPEND_LIB := $(LOCAL_DEPEND_LIB)

ifeq ($($(target)_USE_CXX), yes)
$(target)_DEPEND_LIB += $(TOOLCHAIN_LINK_CXX)
endif

ifeq ($($(target)_USE_GCOV), yes)
endif

ifeq ($($(target)_USE_OMP), yes)
endif

$(target)_DEPEND_LIB += $(TOOLCHAIN_LINK_M) $(TOOLCHAIN_LINK_GCC)

#*********************************************************************************************************
# Targets
#*********************************************************************************************************
$(target)_IMG       := $(OUTPATH)/$(LOCAL_TARGET_NAME)
$(target)_STRIP_IMG := $(OUTPATH)/strip/$(LOCAL_TARGET_NAME)
$(target)_BIN       := $(OUTPATH)/$(addsuffix .bin, $(basename $(LOCAL_TARGET_NAME)))
$(target)_SIZ       := $(OUTPATH)/$(addsuffix .siz, $(basename $(LOCAL_TARGET_NAME)))
$(target)_LZO       := $(OUTPATH)/$(addsuffix .lzo, $(basename $(LOCAL_TARGET_NAME)))

#*********************************************************************************************************
# Link script files
#*********************************************************************************************************
LOCAL_LD_SCRIPT_NT := $(LOCAL_LD_SCRIPT) config.ld

#*********************************************************************************************************
# Link object files
#*********************************************************************************************************
$($(target)_IMG): $(LOCAL_LD_SCRIPT_NT) $($(target)_OBJS) $($(target)_DEPEND_TARGET) SylixOSBSPSymbol.ld
		@rm -f $@
		$(__PRE_LINK_CMD)
		$(CPP) $(__CPUFLAGS) -E -P $(__DSYMBOL) config.ld -o config.lds
		$(__LD) $(__CPUFLAGS) $(ARCH_KERNEL_LDFLAGS) $(__LINKFLAGS) -nostdlib $(addprefix -T, $<) -o $@ $(__OBJS) $(__LIBRARIES)
		$(__POST_LINK_CMD)

#*********************************************************************************************************
# Create bin
#*********************************************************************************************************
$($(target)_BIN): $($(target)_IMG)
		@rm -f $@
		$(OC) -O binary $< $@

#*********************************************************************************************************
# Create siz
#*********************************************************************************************************
$($(target)_SIZ): $($(target)_IMG)
		@rm -f $@
		$(SZ) --format=berkeley $< > $@

#*********************************************************************************************************
# Create lzo
#*********************************************************************************************************
$($(target)_LZO): $($(target)_BIN)
		@rm -f $@
		$(LZOCOM) -c $< $@

#*********************************************************************************************************
# Strip image
#*********************************************************************************************************
$($(target)_STRIP_IMG): $($(target)_IMG)
		@if [ ! -d "$(dir $@)" ]; then mkdir -p "$(dir $@)"; fi
		@rm -f $@
		$(__PRE_STRIP_CMD)
		$(STRIP) $(TOOLCHAIN_STRIP_FLAGS) $< -o $@
		$(__POST_STRIP_CMD)

#*********************************************************************************************************
# Copy SylixOSBSPSymbol.ld
#*********************************************************************************************************
SylixOSBSPSymbol.ld: $(subst $(SPACE),\ ,$(SYLIXOS_LITE_BSP_PATH))/$(OUTDIR)/SylixOSBSPSymbol.ld
		cp $< $@

#*********************************************************************************************************
# Copy extension certificate file
#*********************************************************************************************************
$(EXT_CERT_SRC): $(subst $(SPACE),\ ,$(SYLIXOS_LITE_BSP_PATH))/$(OUTDIR)/bspCert.c
		cp $< $@

#*********************************************************************************************************
# Add targets
#*********************************************************************************************************
ifeq ($(TOOLCHAIN_COMMERCIAL), 1)
TARGETS := $(TARGETS) $($(target)_IMG) $($(target)_BIN) $($(target)_SIZ) $($(target)_STRIP_IMG) $($(target)_LZO) SylixOSBSPSymbol.ld $(EXT_CERT_SRC)
else
TARGETS := $(TARGETS) $($(target)_IMG) $($(target)_BIN) $($(target)_SIZ) $($(target)_STRIP_IMG) SylixOSBSPSymbol.ld $(EXT_CERT_SRC)
endif

#*********************************************************************************************************
# End
#*********************************************************************************************************
