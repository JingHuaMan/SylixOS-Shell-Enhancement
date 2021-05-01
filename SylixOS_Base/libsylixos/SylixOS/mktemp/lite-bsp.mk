#*********************************************************************************************************
#
#                                    中国软件开源组织
#
#                                   嵌入式实时操作系统
#
#                                SylixOS(TM)  LW : long wing
#
#                               Copyright All Rights Reserved
#
#--------------文件信息--------------------------------------------------------------------------------
#
# 文   件   名: lite-bsp.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2017 年 02 月 20 日
#
# 描        述: lite 版本 bsp 类目标 makefile 模板
#*********************************************************************************************************

#*********************************************************************************************************
# Include common.mk
#*********************************************************************************************************
include $(MKTEMP)/common.mk

#*********************************************************************************************************
# Opensource toolchain CAN NOT support extension!
#*********************************************************************************************************
ifeq ($(TOOLCHAIN_COMMERCIAL), 0)
$(target)_USE_EXTENSION) = no
endif

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
# Bsp certificate file and object
#*********************************************************************************************************
BSP_SYMBOL_LD = $(OUTDIR)/SylixOSBSPSymbol.ld
BSP_CERT_SRC  = $(OUTDIR)/bspCert.c
BSP_CERT_OBJ  = $(addprefix $(OBJPATH)/$(target)/, $(addsuffix .o, $(basename $(BSP_CERT_SRC))))
BSP_CERT_DEP  = $(addprefix $(DEPPATH)/$(target)/, $(addsuffix .d, $(basename $(BSP_CERT_SRC))))

ifneq (,$(findstring $(BSP_CERT_SRC),$(LOCAL_SRCS)))
$(target)_OBJS := $(filter-out $(BSP_CERT_OBJ),$($(target)_OBJS))
endif

#*********************************************************************************************************
# Depend library search paths
#*********************************************************************************************************
$(target)_DEPEND_LIB_PATH := $(TOOLCHAIN_LIB_INC)"$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)"
$(target)_DEPEND_LIB_PATH += $(LOCAL_DEPEND_LIB_PATH)

#*********************************************************************************************************
# Depend libraries
#*********************************************************************************************************
ifeq ($($(target)_USE_EXTENSION), yes)
$(target)_DEPEND_LIB := -Wl,--whole-archive $(LOCAL_DEPEND_LIB)
$(target)_DEPEND_LIB += $(TOOLCHAIN_LINK_SYLIXOS) -Wl,--no-whole-archive
else
$(target)_DEPEND_LIB := $(LOCAL_DEPEND_LIB)
$(target)_DEPEND_LIB += $(TOOLCHAIN_LINK_SYLIXOS)
endif

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
$(target)_IMG       := $(OUTPATH)/$(LOCAL_TARGET_NAME)
$(target)_STRIP_IMG := $(OUTPATH)/strip/$(LOCAL_TARGET_NAME)
ifeq ($(ARCH), c6x)
$(target)_BIN       := $(OUTPATH)/$(addsuffix .hex, $(basename $(LOCAL_TARGET_NAME)))
else
$(target)_BIN       := $(OUTPATH)/$(addsuffix .bin, $(basename $(LOCAL_TARGET_NAME)))
endif
$(target)_SIZ       := $(OUTPATH)/$(addsuffix .siz, $(basename $(LOCAL_TARGET_NAME)))
$(target)_LZO       := $(OUTPATH)/$(addsuffix .lzo, $(basename $(LOCAL_TARGET_NAME)))

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
ifeq ($($(target)_USE_EXTENSION), yes)
$($(target)_IMG): $(LOCAL_LD_SCRIPT_NT) $($(target)_OBJS) $($(target)_DEPEND_TARGET)
		@echo Link $@ first time
			@rm -f $@
			$(__PRE_LINK_CMD)
			$(CPP) $(__CPUFLAGS) -E -P $(__DSYMBOL) config.ld -o config.lds
			$(__LD) $(__CPUFLAGS) $(ARCH_KERNEL_LDFLAGS) $(__LINKFLAGS) -nostdlib $(addprefix -T, $<) -o $@ $(__OBJS) $(__LIBRARIES)
		@echo Create $(BSP_SYMBOL_LD)
			@rm -f $(BSP_SYMBOL_LD)
			cp $(subst $(SPACE),\ ,$(SYLIXOS_BASE_PATH))/libsylixos/SylixOS/hosttools/makelitesymbol/Makefile $(OUTDIR)
			cp $(subst $(SPACE),\ ,$(SYLIXOS_BASE_PATH))/libsylixos/SylixOS/hosttools/makelitesymbol/makelitesymbol.bat $(OUTDIR)
			cp $(subst $(SPACE),\ ,$(SYLIXOS_BASE_PATH))/libsylixos/SylixOS/hosttools/makelitesymbol/makelitesymbol.sh $(OUTDIR)
			cp $(subst $(SPACE),\ ,$(SYLIXOS_BASE_PATH))/libsylixos/SylixOS/hosttools/makesymbol/nm.exe $(OUTDIR)
			make -C $(OUTDIR) SRCFILE=$(@F) DESTFILE=$(notdir $(BSP_SYMBOL_LD))
		@echo Create $(BSP_CERT_SRC)
			@rm -f $(BSP_CERT_SRC)
			$(CERT) $(BSP_SYMBOL_LD) $(BSP_CERT_SRC)
		@echo Compile $(BSP_CERT_SRC)
			@if [ ! -d "$(dir $(BSP_CERT_OBJ))" ]; then \
				mkdir -p "$(dir $(BSP_CERT_OBJ))"; fi
			@if [ ! -d "$(dir $(BSP_CERT_DEP))" ]; then \
				mkdir -p "$(dir $(BSP_CERT_DEP))"; fi
			$(CC) $($(notdir $@)_CFLAGS) -MMD -MP -MF $(BSP_CERT_DEP) -c $(BSP_CERT_SRC) -o $(BSP_CERT_OBJ)
		@echo Link $@
			@rm -f $@
			$(__LD) $(__CPUFLAGS) $(ARCH_KERNEL_LDFLAGS) $(__LINKFLAGS) -nostdlib $(addprefix -T, $<) -o $@ $(__OBJS) $(BSP_CERT_OBJ) $(__LIBRARIES)
			$(__POST_LINK_CMD)
else
$($(target)_IMG): $(LOCAL_LD_SCRIPT_NT) $($(target)_OBJS) $($(target)_DEPEND_TARGET)
		@rm -f $@
		$(__PRE_LINK_CMD)
		$(CPP) $(__CPUFLAGS) -E -P $(__DSYMBOL) config.ld -o config.lds
		$(__LD) $(__CPUFLAGS) $(ARCH_KERNEL_LDFLAGS) $(__LINKFLAGS) -nostdlib $(addprefix -T, $<) -o $@ $(__OBJS) $(__LIBRARIES)
		$(__POST_LINK_CMD)
endif
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
ifeq ($($(target)_USE_EXTENSION), yes)
TARGETS := $(TARGETS) $($(target)_IMG) $($(target)_BIN) $($(target)_SIZ) $($(target)_STRIP_IMG) $($(target)_LZO) $(BSP_SYMBOL_LD) $(BSP_CERT_SRC) $(BSP_CERT_OBJ)
else
TARGETS := $(TARGETS) $($(target)_IMG) $($(target)_BIN) $($(target)_SIZ) $($(target)_STRIP_IMG) $($(target)_LZO)
endif
else
TARGETS := $(TARGETS) $($(target)_IMG) $($(target)_BIN) $($(target)_SIZ) $($(target)_STRIP_IMG)
endif

#*********************************************************************************************************
# End
#*********************************************************************************************************
