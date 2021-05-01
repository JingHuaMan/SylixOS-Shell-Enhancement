/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: cskyregs.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 14 日
**
** 描        述: C-SKY 指令.
*********************************************************************************************************/

#ifndef __CSKY_REGS_H
#define __CSKY_REGS_H

/*********************************************************************************************************
  Configure language
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)
#define _ULCAST_
#else
#define _ULCAST_ (unsigned long)
#endif

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

#define cskyFpuIdRead()                     \
({  UINT32 __uiRes;                         \
    __asm__ __volatile__(                   \
        "mfcr      %0, cr<0, 2>"            \
        : "=r" (__uiRes)                    \
        :);                                 \
    __uiRes;                                \
})

#define cskyEntryHiWrite(value)             \
do {                                        \
    __asm__ __volatile__(                   \
        "mtcr      %0, cr<4, 15>"           \
        :: "r"(value));                     \
}while (0)

#define cskyEntryLo0Write(value)            \
do {                                        \
    __asm__ __volatile__(                   \
        "mtcr      %0, cr<2, 15>"           \
        :: "r"(value));                     \
}while (0)

#define cskyEntryLo1Write(value)            \
do {                                        \
    __asm__ __volatile__(                   \
        "mtcr      %0, cr<3, 15>"           \
        :: "r"(value));                     \
}while (0)

#define cskyIndexRead()                     \
({  UINT32   uiRes;                         \
    __asm__ __volatile__(                   \
        "mfcr      %0, cr<0, 15>"           \
        : "=r"(uiRes));                     \
    uiRes;                                  \
})

#define cskyIndexWrite(value)               \
do {                                        \
    __asm__ __volatile__(                   \
        "mtcr      %0, cr<0, 15>"           \
        :: "r"(value));                     \
}while (0)

#define cskyEntryHiRead()                   \
({  UINT32   uiRes;                         \
    __asm__ __volatile__(                   \
        "mfcr      %0, cr<4, 15>"           \
        : "=r"(uiRes));                     \
    uiRes;                                  \
})

#define cskyPgdRead()                       \
({  UINT32   uiRes;                         \
    __asm__ __volatile__(                   \
        "mfcr      %0, cr<29, 15>"          \
        : "=r"(uiRes));                     \
    uiRes;                                  \
})

#define cskyEntryLo0Read()                  \
({  UINT32   uiRes;                         \
    __asm__ __volatile__(                   \
        "mfcr      %0, cr<2, 15>"           \
        : "=r"(uiRes));                     \
    uiRes;                                  \
})

#define cskyEntryLo1Read()                  \
({  UINT32   uiRes;                         \
    __asm__ __volatile__(                   \
        "mfcr      %0, cr<3, 15>"           \
        : "=r"(uiRes));                     \
    uiRes;                                  \
})

#define SET_CIR(value)                      \
    __asm__ __volatile__(                   \
        "mtcr  %0 , cr22\n\t" ::"r"(value))

#define GET_CFR(value, rvalue, tmp)         \
    __asm__ __volatile__(                   \
        "mtcr  %2 , cr17\n\t"               \
        "mfcr  %0 , cr17\n\t"               \
        "bclri %1 , %0, 31\n\t"             \
        "mtcr  %1 , cr17\n\t"               \
        :"=r"(rvalue), "=r"(tmp)            \
        :"r"(value), "0"(rvalue), "1"(tmp))

#define SET_CFR(value)                      \
    __asm__ __volatile__(                   \
        "mtcr  %0 , cr17\n\t" ::"r"(value))

#define LDW_ADDR(addr, tmp)                 \
    __asm__ __volatile__(                   \
        "ldw   %0 , (%1, 0)\n\t"            \
        :"=r"(tmp)                          \
        :"r"(addr), "0"(tmp))

static inline void cskyTlbProbe(void)
{
    int value = 0x80000000;

    __asm__ __volatile__("mtcr %0,cr<8, 15>\n\t"
                    : :"r" (value));
}

static inline void cskyTlbRead(void)
{
    int value = 0x40000000;

    __asm__ __volatile__("mtcr %0,cr<8, 15>\n\t"
                    : :"r" (value));
}

static inline void cskyTlbWriteIndexed(void)
{
    int value = 0x20000000;

    __asm__ __volatile__("mtcr %0,cr<8,15>\n\t"
                    : :"r" (value));
}

static inline void cskyTlbWriteRandom(void)
{
    int value = 0x10000000;

    __asm__ __volatile__("mtcr %0,cr<8, 15>\n\t"
                    : :"r" (value));
}

static inline void cskyTlbInvalidAll(void)
{
    int value = 0x04000000;

    __asm__ __volatile__("mtcr %0,cr<8, 15>\n\t"
                    : :"r" (value));
}

static inline void cskyTlbInvalidIndexed(void)
{
    int value = 0x02000000;

    __asm__ __volatile__("mtcr %0,cr<8, 15>\n\t"
                    : :"r" (value));
}

static inline uint32_t cskyMpuGetCCR(void)
{
    register uint32_t result;

    __asm__ __volatile__("mfcr %0, cr<18, 0>\n"
                    : "=r"(result));

    return (result);
}

static inline void cskyMpuSetCCR(uint32_t ccr)
{
    __asm__ __volatile__("mtcr %0, cr<18, 0>\n"
                    : : "r"(ccr));
}

static inline uint32_t cskyMpuGetCAPR(void)
{
    register uint32_t result;

    __asm__ __volatile__("mfcr %0, cr<19, 0>\n"
                    : "=r"(result));

    return (result);
}

static inline void cskyMpuSetCAPR(uint32_t capr)
{
    __asm__ __volatile__("mtcr %0, cr<19, 0>\n"
                    : : "r"(capr));
}

static inline void cskyMpuSetPACR(uint32_t pacr)
{
    __asm__ __volatile__("mtcr %0, cr<20, 0>\n"
                    : : "r"(pacr));
}

static inline uint32_t cskyMpuGetPACR(void)
{
    uint32_t result;

    __asm__ __volatile__("mfcr %0, cr<20, 0>"
                    : "=r"(result));

    return (result);
}

static inline void cskyMpuSetPRSR(uint32_t prsr)
{
    __asm__ __volatile__("mtcr %0, cr<21, 0>\n"
                    : : "r"(prsr));
}

static inline uint32_t cskyMpuGetPRSR(void)
{
    uint32_t result;

    __asm__ __volatile__("mfcr %0, cr<21, 0>"
                    : "=r"(result));

    return (result);
}

#endif                                                                  /* !__ASSEMBLY__                */

#define CSKY_FPUID_IMPLEMENT               (1 << 31)

#define CSKY_ENTRYLO_PFN_SHIFT             12

#define ENTRYLO_V                          (_ULCAST_(1) << 1)
#define ENTRYLO_D                          (_ULCAST_(1) << 2)
#define ENTRYLO_C_SHIFT                    3
#define ENTRYLO_C                          (_ULCAST_(1) << ENTRYLO_C_SHIFT)

#define PM_4K                              0x00000000
#define PM_16K                             0x00006000
#define PM_64K                             0x0001e000
#define PM_256K                            0x0007e000
#define PM_1M                              0x001fe000
#define PM_4M                              0x007fe000
#define PM_16M                             0x01ffe000

#define TLBWI_TLB                          0x20000000

#define FESR_ILLE                          (1 << 16)
#define FESR_FEC                           (1 << 7)
#define FESR_IDC                           (1 << 5)
#define FESR_IXC                           (1 << 4)
#define FESR_UFC                           (1 << 3)
#define FESR_OFC                           (1 << 2)
#define FESR_DZC                           (1 << 1)
#define FESR_IOC                           (1 << 0)

#endif                                                                  /*  __CSKY_REGS_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
