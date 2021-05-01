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
# ��   ��   ��: common.mk
#
# ��   ��   ��: Jiao.JinXing(������)
#
# �ļ���������: 2016 �� 08 �� 24 ��
#
# ��        ��: makefile ģ�幫������
#*********************************************************************************************************

#*********************************************************************************************************
# Target name
#*********************************************************************************************************
target := $(LOCAL_TARGET_NAME)

#*********************************************************************************************************
# Objects
#*********************************************************************************************************
ifeq ($(ARCH), arm)
LOCAL_ARCH_SRCS := $(LOCAL_ARM_SRCS)
endif

ifeq ($(ARCH), arm64)
LOCAL_ARCH_SRCS := $(LOCAL_ARM64_SRCS)
endif

ifeq ($(ARCH), mips)
LOCAL_ARCH_SRCS := $(LOCAL_MIPS_SRCS)
endif

ifeq ($(ARCH), mips64)
LOCAL_ARCH_SRCS := $(LOCAL_MIPS64_SRCS)
endif

ifeq ($(ARCH), ppc)
LOCAL_ARCH_SRCS := $(LOCAL_PPC_SRCS)
endif

ifeq ($(ARCH), x86)
LOCAL_ARCH_SRCS := $(LOCAL_X86_SRCS)
endif

ifeq ($(ARCH), x64)
LOCAL_ARCH_SRCS := $(LOCAL_X64_SRCS)
endif

ifeq ($(ARCH), c6x)
LOCAL_ARCH_SRCS := $(LOCAL_C6X_SRCS)
endif

ifeq ($(ARCH), sparc)
LOCAL_ARCH_SRCS := $(LOCAL_SPARC_SRCS)
endif

ifeq ($(ARCH), riscv)
LOCAL_ARCH_SRCS := $(LOCAL_RISCV_SRCS)
endif

ifeq ($(ARCH), csky)
LOCAL_ARCH_SRCS := $(LOCAL_CSKY_SRCS)
endif

LOCAL_SRCS := $(LOCAL_SRCS) $(LOCAL_ARCH_SRCS)
LOCAL_SRCS := $(filter-out $(LOCAL_EXCLUDE_SRCS),$(LOCAL_SRCS))

$(target)_OBJS := $(addprefix $(OBJPATH)/$(target)/, $(addsuffix .o, $(basename $(LOCAL_SRCS))))
$(target)_DEPS := $(addprefix $(DEPPATH)/$(target)/, $(addsuffix .d, $(basename $(LOCAL_SRCS))))
ifeq ($(ARCH), c6x)
$(target)_PPS  := $(addprefix $(DEPPATH)/$(target)/, $(addsuffix .pp, $(basename $(LOCAL_SRCS))))
endif

#*********************************************************************************************************
# Include depend files
#*********************************************************************************************************
ifneq ($(MAKECMDGOALS), clean)
sinclude $($(target)_DEPS)
endif

#*********************************************************************************************************
# Include paths
#*********************************************************************************************************
$(target)_INC_PATH := $(LOCAL_INC_PATH)
$(target)_INC_PATH += $(TOOLCHAIN_HEADER_INC)"$(SYLIXOS_BASE_PATH)/libsylixos/SylixOS"
$(target)_INC_PATH += $(TOOLCHAIN_HEADER_INC)"$(SYLIXOS_BASE_PATH)/libsylixos/SylixOS/include"
$(target)_INC_PATH += $(TOOLCHAIN_HEADER_INC)"$(SYLIXOS_BASE_PATH)/libsylixos/SylixOS/include/network"

ifeq ($(ARCH), c6x)
$(target)_INC_PATH    += $(TOOLCHAIN_HEADER_INC)"$(TOOLCHAIN_PATH)/include"
LOCAL_DEPEND_LIB_PATH += $(TOOLCHAIN_LIB_INC)"$(TOOLCHAIN_PATH)/lib/lib"
endif

#*********************************************************************************************************
# Compiler preprocess
#*********************************************************************************************************
$(target)_DSYMBOL := $(TOOLCHAIN_DEF_SYMBOL)SYLIXOS
$(target)_DSYMBOL += $(LOCAL_DSYMBOL)

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
$(target)_CFLAGS    := $(LOCAL_CFLAGS)
$(target)_CXXFLAGS  := $(LOCAL_CXXFLAGS)
$(target)_LINKFLAGS := $(LOCAL_LINKFLAGS)

#*********************************************************************************************************
# Define some useful variables
#*********************************************************************************************************
$(target)_USE_CXX         := $(LOCAL_USE_CXX)
$(target)_USE_CXX_EXCEPT  := $(LOCAL_USE_CXX_EXCEPT)
$(target)_USE_GCOV        := $(LOCAL_USE_GCOV)
$(target)_USE_OMP         := $(LOCAL_USE_OMP)
$(target)_USE_EXTENSION   := $(LOCAL_USE_EXTENSION)

$(target)_NO_UNDEF_SYM    := $(LOCAL_NO_UNDEF_SYM)

$(target)_PRE_LINK_CMD    := $(LOCAL_PRE_LINK_CMD)
$(target)_POST_LINK_CMD   := $(LOCAL_POST_LINK_CMD)

$(target)_PRE_STRIP_CMD   := $(LOCAL_PRE_STRIP_CMD)
$(target)_POST_STRIP_CMD  := $(LOCAL_POST_STRIP_CMD)

$(target)_DEPEND_TARGET   := $(LOCAL_DEPEND_TARGET)
$(target)_SHARED_LIB_ONLY := $(LOCAL_SHARED_LIB_ONLY)

ifeq ($($(target)_USE_CXX), yes)
$(target)_LD := $(CXX_LD)
else
$(target)_LD := $(C_LD)
endif

#*********************************************************************************************************
# Compile source files
#*********************************************************************************************************
ifeq ($(ARCH), c6x)
$(OBJPATH)/$(target)/%.o: %.sa
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__PP))" ]; then \
			mkdir -p "$(dir $(__PP))"; fi
		@-rm -rf $(__DEP)
		$(AS) $($(__TARGET)_ASFLAGS) --preproc_with_compile --preproc_dependency=$(__PP) $< -fe=$@
		@-$(DEPFIX) $(__PP) $(__DEP)
		@-rm -rf $(__PP)

$(OBJPATH)/$(target)/%.o: %.s
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__PP))" ]; then \
			mkdir -p "$(dir $(__PP))"; fi
		@-rm -rf $(__DEP)
		$(AS) $($(__TARGET)_ASFLAGS) --preproc_with_compile --preproc_dependency=$(__PP) $< -fe=$@
		@-$(DEPFIX) $(__PP) $(__DEP)
		@-rm -rf $(__PP)

$(OBJPATH)/$(target)/%.o: %.abs
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__PP))" ]; then \
			mkdir -p "$(dir $(__PP))"; fi
		@-rm -rf $(__DEP)
		$(AS) $($(__TARGET)_ASFLAGS) --preproc_with_compile --preproc_dependency=$(__PP) $< -fe=$@
		@-$(DEPFIX) $(__PP) $(__DEP)
		@-rm -rf $(__PP)

$(OBJPATH)/$(target)/%.o: %.asm
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__PP))" ]; then \
			mkdir -p "$(dir $(__PP))"; fi
		@-rm -rf $(__DEP)
		$(AS) $($(__TARGET)_ASFLAGS) --preproc_with_compile --preproc_dependency=$(__PP) $< -fe=$@
		@-$(DEPFIX) $(__PP) $(__DEP)
		@-rm -rf $(__PP)

ifeq ($($(target)_USE_CXX), yes)
$(OBJPATH)/$(target)/%.o: %.c
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__PP))" ]; then \
			mkdir -p "$(dir $(__PP))"; fi
		@-rm -rf $(__DEP)
		$(CC) $($(__TARGET)_CFLAGS) --preproc_with_compile --cplusplus --preproc_dependency=$(__PP) $< -fe=$@
		@-$(DEPFIX) $(__PP) $(__DEP)
		@-rm -rf $(__PP)
else
$(OBJPATH)/$(target)/%.o: %.c
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__PP))" ]; then \
			mkdir -p "$(dir $(__PP))"; fi
		@-rm -rf $(__DEP)
		$(CC) $($(__TARGET)_CFLAGS) --preproc_with_compile --preproc_dependency=$(__PP) $< -fe=$@
		@-$(DEPFIX) $(__PP) $(__DEP)
		@-rm -rf $(__PP)
endif

$(OBJPATH)/$(target)/%.o: %.cpp
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__PP))" ]; then \
			mkdir -p "$(dir $(__PP))"; fi
		@-rm -rf $(__DEP)
		$(CXX) $($(__TARGET)_CXXFLAGS) --preproc_with_compile --preproc_dependency=$(__PP) $< -fe=$@
		@-$(DEPFIX) $(__PP) $(__DEP)
		@-rm -rf $(__PP)

$(OBJPATH)/$(target)/%.o: %.cxx
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__PP))" ]; then \
			mkdir -p "$(dir $(__PP))"; fi
		@-rm -rf $(__DEP)
		$(CXX) $($(__TARGET)_CXXFLAGS) --preproc_with_compile --preproc_dependency=$(__PP) $< -fe=$@
		@-$(DEPFIX) $(__PP) $(__DEP)
		@-rm -rf $(__PP)

$(OBJPATH)/$(target)/%.o: %.cc
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__PP))" ]; then \
			mkdir -p "$(dir $(__PP))"; fi
		@-rm -rf $(__DEP)
		$(CXX) $($(__TARGET)_CXXFLAGS) --preproc_with_compile --preproc_dependency=$(__PP) $< -fe=$@
		@-$(DEPFIX) $(__PP) $(__DEP)
		@-rm -rf $(__PP)
else
$(OBJPATH)/$(target)/%.o: %.S
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(AS) $($(__TARGET)_ASFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

$(OBJPATH)/$(target)/%.o: %.c
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(CC) $($(__TARGET)_CFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

$(OBJPATH)/$(target)/%.o: %.cpp
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(CXX) $($(__TARGET)_CXXFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

$(OBJPATH)/$(target)/%.o: %.cxx
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(CXX) $($(__TARGET)_CXXFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

$(OBJPATH)/$(target)/%.o: %.cc
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(CXX) $($(__TARGET)_CXXFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@
endif

#*********************************************************************************************************
# End
#*********************************************************************************************************
