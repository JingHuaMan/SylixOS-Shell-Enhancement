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
# ��   ��   ��: bsp.mk
#
# ��   ��   ��: Jiao.JinXing(������)
#
# �ļ���������: 2016 �� 08 �� 24 ��
#
# ��        ��: bsp ��Ŀ�� makefile ģ��
#*********************************************************************************************************

#*********************************************************************************************************
# Copy symbol.c symbol.h
#*********************************************************************************************************
ifeq ($(BSP_SYMBOL_PATH),)
BSP_SYMBOL_PATH = SylixOS/bsp
endif

$(BSP_SYMBOL_PATH)/symbol.c: $(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)/symbol.c $(BSP_SYMBOL_PATH)/symbol.h
		cp "$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)/symbol.c" $(BSP_SYMBOL_PATH)/symbol.c

$(BSP_SYMBOL_PATH)/symbol.h: $(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)/symbol.h
		cp "$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)/symbol.h" $(BSP_SYMBOL_PATH)/symbol.h

ifeq (,$(findstring SylixOS, $(BSP_SYMBOL_PATH)))
BSP_OUT_BASE = ../
else
BSP_OUT_BASE = 
endif

#*********************************************************************************************************
# Add symbol.c to LOCAL_SRCS
#*********************************************************************************************************
LOCAL_SRCS := $(BSP_SYMBOL_PATH)/symbol.c $(LOCAL_SRCS)

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

$(target)_DSYMBOL     += $(TOOLCHAIN_DEF_SYMBOL)SYLIXOS_EXPORT_KSYMBOL

$(target)_CPUFLAGS                     := $(ARCH_CPUFLAGS_NOFPU) $(ARCH_KERNEL_CFLAGS)
$(target)_CPUFLAGS_WITHOUT_FPUFLAGS    := $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_KERNEL_CFLAGS)
$(target)_COMMONFLAGS                  := $($(target)_CPUFLAGS) $(ARCH_COMMONFLAGS) $(TOOLCHAIN_OPTIMIZE) $(TOOLCHAIN_COMMONFLAGS) $($(target)_GCOV_FLAGS) $($(target)_OMP_FLAGS)
$(target)_COMMONFLAGS_WITHOUT_FPUFLAGS := $($(target)_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_COMMONFLAGS) $(TOOLCHAIN_OPTIMIZE) $(TOOLCHAIN_COMMONFLAGS) $($(target)_GCOV_FLAGS) $($(target)_OMP_FLAGS)
$(target)_ASFLAGS_WITHOUT_FPUFLAGS     := $($(target)_COMMONFLAGS_WITHOUT_FPUFLAGS) $(TOOLCHAIN_ASFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH)
$(target)_ASFLAGS                      := $($(target)_COMMONFLAGS) $(TOOLCHAIN_ASFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH)
$(target)_CFLAGS_WITHOUT_FPUFLAGS      := $($(target)_COMMONFLAGS_WITHOUT_FPUFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CFLAGS)
$(target)_CFLAGS                       := $($(target)_COMMONFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CFLAGS)
$(target)_CXXFLAGS_WITHOUT_FPUFLAGS    := $($(target)_COMMONFLAGS_WITHOUT_FPUFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CXX_EXCEPT) $($(target)_CXXFLAGS)
$(target)_CXXFLAGS                     := $($(target)_COMMONFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CXX_EXCEPT) $($(target)_CXXFLAGS)

#*********************************************************************************************************
# Depend library search paths
#*********************************************************************************************************
$(target)_DEPEND_LIB_PATH := $(TOOLCHAIN_LIB_INC)"$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)"
$(target)_DEPEND_LIB_PATH += $(LOCAL_DEPEND_LIB_PATH)

#*********************************************************************************************************
# Depend libraries
#*********************************************************************************************************
$(target)_DEPEND_LIB := $(LOCAL_DEPEND_LIB)
$(target)_DEPEND_LIB += $(TOOLCHAIN_LINK_SYLIXOS)

ifeq ($($(target)_USE_CXX), yes)
$(target)_DEPEND_LIB += $(TOOLCHAIN_LINK_CXX)
endif

ifeq ($($(target)_USE_GCOV), yes)
endif

ifeq ($($(target)_USE_OMP), yes)
endif

$(target)_DEPEND_LIB    += $(TOOLCHAIN_LINK_M) $(TOOLCHAIN_LINK_GCC)
$(target)_DEPEND_TARGET += $(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)/libsylixos.a

#*********************************************************************************************************
# Targets
#*********************************************************************************************************
$(target)_IMG       := $(BSP_OUT_BASE)$(OUTPATH)/$(LOCAL_TARGET_NAME)
$(target)_STRIP_IMG := $(BSP_OUT_BASE)$(OUTPATH)/strip/$(LOCAL_TARGET_NAME)
ifeq ($(ARCH), c6x)
$(target)_BIN       := $(BSP_OUT_BASE)$(OUTPATH)/$(addsuffix .hex, $(basename $(LOCAL_TARGET_NAME)))
else
$(target)_BIN       := $(BSP_OUT_BASE)$(OUTPATH)/$(addsuffix .bin, $(basename $(LOCAL_TARGET_NAME)))
endif
$(target)_SIZ       := $(BSP_OUT_BASE)$(OUTPATH)/$(addsuffix .siz, $(basename $(LOCAL_TARGET_NAME)))
$(target)_LZO       := $(BSP_OUT_BASE)$(OUTPATH)/$(addsuffix .lzo, $(basename $(LOCAL_TARGET_NAME)))

#*********************************************************************************************************
# Link script files
#*********************************************************************************************************
LOCAL_LD_SCRIPT_NT := $(LOCAL_LD_SCRIPT) config.ld

#*********************************************************************************************************
# Link object files
#*********************************************************************************************************
ifeq ($(ARCH), c6x)
$($(target)_IMG): $($(target)_OBJS) $($(target)_DEPEND_TARGET)
		@rm -f $@
		$(__PRE_LINK_CMD)
		$(__LD) $(__CPUFLAGS) $(ARCH_KERNEL_LDFLAGS) $(__LINKFLAGS) --abi=eabi -z --dynamic --trampolines=off --dsbt_size=64 -o $@ $(LOCAL_LD_SCRIPT) $(__OBJS) $(__LIBRARIES) 
		@mv $@ $@.c6x
		@nm $@.c6x > $@_nm.txt
		@$(DIS) $(TOOLCHAIN_DIS_FLAGS) $@.c6x > $@_dis.txt
		@$(LINK) $@_nm.txt $@_dis.txt $@.c6x
		@mv $@.c6x $@
		@rm -f $@_nm.txt $@_dis.txt
		$(__POST_LINK_CMD)
else
$($(target)_IMG): $(LOCAL_LD_SCRIPT_NT) $($(target)_OBJS) $($(target)_DEPEND_TARGET)
		@rm -f $@
		$(__PRE_LINK_CMD)
		$(CPP) $(__CPUFLAGS) -E -P $(__DSYMBOL) config.ld -o config.lds
		$(__LD) $(__CPUFLAGS) $(ARCH_KERNEL_LDFLAGS) $(__LINKFLAGS) -nostdlib $(addprefix -T, $<) -o $@ $(__OBJS) $(__LIBRARIES)
		$(__POST_LINK_CMD)
endif

#*********************************************************************************************************
# Create bin
#*********************************************************************************************************
ifeq ($(ARCH), c6x)
$($(target)_BIN): $($(target)_IMG)
		@rm -f $@
		$(OC) -i $< -o=$@
else
$($(target)_BIN): $($(target)_IMG)
		@rm -f $@
		$(OC) -O binary $< $@
endif

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
# Add targets
#*********************************************************************************************************
ifeq ($(TOOLCHAIN_COMMERCIAL), 1)
TARGETS := $(TARGETS) $($(target)_IMG) $($(target)_BIN) $($(target)_SIZ) $($(target)_STRIP_IMG) $($(target)_LZO) $(BSP_SYMBOL_PATH)/symbol.c $(BSP_SYMBOL_PATH)/symbol.h
else
TARGETS := $(TARGETS) $($(target)_IMG) $($(target)_BIN) $($(target)_SIZ) $($(target)_STRIP_IMG) $(BSP_SYMBOL_PATH)/symbol.c $(BSP_SYMBOL_PATH)/symbol.h
endif

#*********************************************************************************************************
# End
#*********************************************************************************************************
