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
# ��   ��   ��: gcc.mk
#
# ��   ��   ��: Jiao.JinXing(������)
#
# �ļ���������: 2017 �� 05 �� 19 ��
#
# ��        ��: RealEvo-Compiler GCC ��ر�������
#*********************************************************************************************************

#*********************************************************************************************************
# Toolchain select
#*********************************************************************************************************
CC      = $(TOOLCHAIN_PREFIX)gcc
CXX     = $(TOOLCHAIN_PREFIX)g++
AS      = $(TOOLCHAIN_PREFIX)gcc
AR      = $(TOOLCHAIN_PREFIX)ar
C_LD    = $(TOOLCHAIN_PREFIX)gcc
CXX_LD  = $(TOOLCHAIN_PREFIX)g++
OC      = $(TOOLCHAIN_PREFIX)objcopy
SZ      = $(TOOLCHAIN_PREFIX)size
CPP     = $(TOOLCHAIN_PREFIX)cpp
LZOCOM  = $(TOOLCHAIN_PREFIX)lzocom
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
STRIP   = $(TOOLCHAIN_PREFIX)strip
CERT    = $(TOOLCHAIN_PREFIX)cert

#*********************************************************************************************************
# Commercial toolchain check
#*********************************************************************************************************
TOOLCHAIN_COMMERCIAL = $(shell $(LZOCOM) 1>null 2>null && \
			(rm -rf null; echo 1) || \
			(rm -rf null; echo 0))

#*********************************************************************************************************
# Toolchain version
#*********************************************************************************************************
ifeq ($(TOOLCHAIN_COMMERCIAL), 1)
TOOLCHAIN_VERSION_STR   := $(shell $(CC) -dumpversion)
TOOLCHAIN_VERSION_MAJOR := $(shell echo $(TOOLCHAIN_VERSION_STR) | cut -f1 -d.)
TOOLCHAIN_VERSION_MINOR := $(shell echo $(TOOLCHAIN_VERSION_STR) | cut -f2 -d.)
TOOLCHAIN_VERSION_PATCH := $(shell echo $(TOOLCHAIN_VERSION_STR) | cut -f3 -d.)
TOOLCHAIN_VERSION       := $(TOOLCHAIN_VERSION_MAJOR)$(TOOLCHAIN_VERSION_MINOR)$(TOOLCHAIN_VERSION_PATCH)
endif

#*********************************************************************************************************
# Compiler optimize flag
# Do NOT use -O3 and -Os, -Os is not align for function loop and jump.
#                         -O3 default use inline function.
#*********************************************************************************************************
ifeq ($(DEBUG_LEVEL), debug)
TOOLCHAIN_OPTIMIZE = -O0 -g3 -gdwarf-2
else
TOOLCHAIN_OPTIMIZE = -O2 -g1 -gdwarf-2
endif

#*********************************************************************************************************
# Toolchain flag
#*********************************************************************************************************
TOOLCHAIN_CXX_EXCEPT_CFLAGS    = -fexceptions -frtti
TOOLCHAIN_NO_CXX_EXCEPT_CFLAGS = -fno-exceptions -fno-rtti
TOOLCHAIN_GCOV_CFLAGS          = -fprofile-arcs -ftest-coverage
TOOLCHAIN_OMP_CFLAGS           = -fopenmp
TOOLCHAIN_COMMONFLAGS          = -Wall -fmessage-length=0 -fsigned-char -fno-short-enums -fno-strict-aliasing
TOOLCHAIN_ASFLAGS              = -x assembler-with-cpp
TOOLCHAIN_NO_UNDEF_SYM_FLAGS   = @$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)/symbol.ld

TOOLCHAIN_AR_FLAGS             = -r
TOOLCHAIN_STRIP_FLAGS          = 
TOOLCHAIN_STRIP_KO_FLAGS       = --strip-unneeded

#*********************************************************************************************************
# Toolchain link library
#*********************************************************************************************************
TOOLCHAIN_LINK_VPMPDM    = -lvpmpdm
TOOLCHAIN_LINK_CEXTERN   = -lcextern
TOOLCHAIN_LINK_DSOHANDLE = -ldsohandle
TOOLCHAIN_LINK_SYLIXOS   = -lsylixos
TOOLCHAIN_LINK_GCOV      = -lgcov
TOOLCHAIN_LINK_GCOV_KO   = -lgcov_ko
TOOLCHAIN_LINK_OMP       = -lgomp
TOOLCHAIN_LINK_CXX       = -lstdc++
TOOLCHAIN_LINK_M         = -lm
TOOLCHAIN_LINK_GCC       = -lgcc
TOOLCHAIN_LINK_GTEST     = -lgtestx

TOOLCHAIN_LINK_PIC_GCOV  = -lgcov
TOOLCHAIN_LINK_PIC_OMP   = -lgomp
TOOLCHAIN_LINK_PIC_CXX   = -lstdc++
TOOLCHAIN_LINK_PIC_M     = -lm
TOOLCHAIN_LINK_PIC_GCC   = -lgcc

#*********************************************************************************************************
# Toolchain include & define
#*********************************************************************************************************
TOOLCHAIN_HEADER_INC     = -I
TOOLCHAIN_LIB_INC        = -L
TOOLCHAIN_DEF_SYMBOL     = -D

#*********************************************************************************************************
# End
#*********************************************************************************************************
