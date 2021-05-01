/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: x86Gdt.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 05 ��
**
** ��        ��: x86 ��ϵ���� GDT.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86Segment.h"
/*********************************************************************************************************
  ���Ͷ���
*********************************************************************************************************/
/*********************************************************************************************************
  The sructure of a segment descriptor.

  @see Intel x86 doc, Vol 3, section 3.4.5, figure 3-8. For segment types, see section 3.5
*********************************************************************************************************/
struct x86_segment_descriptor {
    /*
     * Lowest dword
     */
    UINT16      limit_15_0;                                 /*  Segment limit, bits 15..0               */
    UINT16      base_paged_addr_15_0;                       /*  Base address, bits 15..0                */

    /*
     * Highest dword
     */
    UINT8       base_paged_addr_23_16;                      /*  Base address bits 23..16                */
    UINT8       segment_type:4;                             /*  Section 3.4.3.1 (code/data)             */
                                                            /*  and 3.5 (system) of Intel x86 vol 3     */
    UINT8       descriptor_type:1;                          /*  0=system, 1=Code/Data                   */
    UINT8       dpl:2;                                      /*  Descriptor privilege level              */
    UINT8       present:1;

    UINT8       limit_19_16:4;                              /*  Segment limit, bits 19..16              */
    UINT8       custom:1;
    UINT8       longmode:1;                                 /*  L: 1=long mode, 0=compatibility mode    */
    UINT8       op_size:1;                                  /*  0=16bits instructions, 1=32bits         */
    UINT8       granularity:1;                              /*  0=limit in bytes, 1=limit in pages      */

    UINT8       base_paged_addr_31_24;                      /*  Base address bits 31..24                */
} __attribute__ ((packed, aligned(8)));

typedef struct x86_segment_descriptor   X86_SEG_DESC;
/*********************************************************************************************************
  The GDT register, which stores the address and size of the GDT.

  @see Intel x86 doc vol 3, section 2.4.1; and section 3.5.1
*********************************************************************************************************/
struct x86_gdt_register {
    /*
     * Intel doc says that the real GDT register (ie the "limit" field)
     * should be odd-word aligned. That's why we add a padding here.
     * Credits to Romain for having signalled this to us.
     */
    UINT16      padding;

    /*
     * The maximum GDT offset allowed to access an entry in the GDT
     */
    UINT16      limit;

    /*
     * This is not exactly a "virtual" address, ie an adddress such as
     * those of instructions and data; this is a "linear" address, ie an
     * address in the paged memory. However, in X86 we configure the
     * segmented memory as a "flat" space: the 0-4GB segment-based (ie
     * "virtual") addresses directly map to the 0-4GB paged memory (ie
     * "linear"), so that the "linear" addresses are numerically equal
     * to the "virtual" addresses: this base_addr will thus be the same
     * as the address of the gdt array
     */
     addr_t      base_addr;
} __attribute__ ((packed, aligned(4)));

typedef struct x86_gdt_register     X86_GDT_REG;
/*********************************************************************************************************
  Structure of a Task State Segment on the x86 Architecture.

  @see Intel x86 spec vol 3, figure 6-2

  @note Such a data structure should not cross any page boundary (see
  end of section 6.2.1 of Intel spec vol 3). This is the reason why
  we tell gcc to align it on a 128B boundary (its size is 104B, which is <= 128).
*********************************************************************************************************/
struct x86_tss {
    /*
     * Intel provides a way for a task to switch to another in an
     * automatic way (call gates). In this case, the back_link field
     * stores the source TSS of the context switch. This allows to
     * easily implement coroutines, task backtracking, ... In SylixOS we
     * don't use TSS for the context switch purpouse, so we always
     * ignore this field.
     * (+0)
     */
    UINT16      back_link;

    UINT16      reserved1;

    /*
     * CPL0 saved context. (+4)
     */
    addr_t      esp0;
    UINT16      ss0;

    UINT16      reserved2;

    /*
     * CPL1 saved context. (+12)
     */
    addr_t      esp1;
    UINT16      ss1;

    UINT16      reserved3;

    /*
     * CPL2 saved context. (+20)
     */
    addr_t      esp2;
    UINT16      ss2;

    UINT16      reserved4;

    /*
     * Interrupted context's saved registers. (+28)
     */
    addr_t      cr3;
    addr_t      eip;
    UINT32      eflags;
    UINT32      eax;
    UINT32      ecx;
    UINT32      edx;
    UINT32      ebx;
    UINT32      esp;
    UINT32      ebp;
    UINT32      esi;
    UINT32      edi;

    /*
     * +72
     */
    UINT16      es;
    UINT16      reserved5;

    /*
     * +76
     */
    UINT16      cs;
    UINT16      reserved6;

    /*
     * +80
     */
    UINT16      ss;
    UINT16      reserved7;

    /*
     * +84
     */
    UINT16      ds;
    UINT16      reserved8;

    /*
     * +88
     */
    UINT16      fs;
    UINT16      reserved9;

    /*
     * +92
     */
    UINT16      gs;
    UINT16      reserved10;

    /*
     * +96
     */
    UINT16      ldtr;
    UINT16      reserved11;

    /*
     * +100
     */
    UINT16      debug_trap_flag :1;
    UINT16      reserved12      :15;
    UINT16      iomap_base_addr;

    /*
     * 104
     */
} __attribute__ ((packed, aligned(128)));

typedef struct x86_tss  X86_TSS;
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define IS_LONG_MODE    ((LW_CFG_CPU_WORD_LENGHT == 64) ? 1 : 0)
/*********************************************************************************************************
  Helper macro that builds a Segment descriptor for the virtual
  0..4GB addresses to be mapped to the linear 0..4GB linear addresses.
*********************************************************************************************************/
#define BUILD_GDTE(descr_privilege_level, is_code)                      \
        ((X86_SEG_DESC) {                                               \
            .limit_15_0             = 0xffff,                           \
            .base_paged_addr_15_0   = 0,                                \
            .base_paged_addr_23_16  = 0,                                \
            .segment_type           = ((is_code) ? 0xb : 0x3),          \
               /* With descriptor_type (below) = 1 (code/data),         \
                * see Figure 3-1 of section 3.4.3.1 in Intel            \
                * x86 vol 3:                                            \
                *   - Code (bit 3 = 1):                                 \
                *     bit 0: 1=Accessed                                 \
                *     bit 1: 1=Readable                                 \
                *     bit 2: 0=Non-Conforming                           \
                *   - Data (bit 3 = 0):                                 \
                *     bit 0: 1=Accessed                                 \
                *     bit 1: 1=Writable                                 \
                *     bit 2: 0=Expand up (stack-related)                \
                * For Conforming/non conforming segments, see           \
                * Intel x86 Vol 3 section 4.8.1.1                       \
                */                                                      \
            .descriptor_type        = 1,  /* 1=Code/Data */             \
            .dpl                    = ((descr_privilege_level) & 0x3),  \
            .present                = 1,                                \
            .limit_19_16            = 0xf,                              \
            .custom                 = 0,                                \
            .longmode               = IS_LONG_MODE,                     \
            .op_size                = 1,  /* 32 bits instr/data */      \
            .granularity            = 1,  /* limit is in 4kB Pages */   \
            .base_paged_addr_31_24  = 0                                 \
        })
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static X86_SEG_DESC _G_x86GDT[LW_CFG_MAX_PROCESSORS][X86_SEG_MAX];      /*  ȫ����������                */
static X86_TSS      _G_x86KernelTss[LW_CFG_MAX_PROCESSORS];             /*  �ں� TSS                    */

extern LW_STACK     _K_stkInterruptStack[LW_CFG_MAX_PROCESSORS][LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)];
/*********************************************************************************************************
** ��������: x86GdtInit
** ��������: ��ʼ�� GDT
** �䡡��  : ulCPUId       CPU ID
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __x86GdtInit (ULONG  ulCPUId)
{
    X86_GDT_REG  gdtr;

    _G_x86GDT[ulCPUId][X86_SEG_NULL]  = (X86_SEG_DESC){ 0, };
    _G_x86GDT[ulCPUId][X86_SEG_KCODE] = BUILD_GDTE(0, 1);
    _G_x86GDT[ulCPUId][X86_SEG_KDATA] = BUILD_GDTE(0, 0);
    _G_x86GDT[ulCPUId][X86_SEG_UCODE] = BUILD_GDTE(3, 1);
    _G_x86GDT[ulCPUId][X86_SEG_UDATA] = BUILD_GDTE(3, 0);
    _G_x86GDT[ulCPUId][X86_SEG_KERNEL_TSS] = (X86_SEG_DESC){ 0, };

    /*
     * Put some garbage in the padding field of the GDTR
     */
    gdtr.padding   = ~0;

    /*
     * Address of the GDT
     */
    gdtr.base_addr = (addr_t)_G_x86GDT[ulCPUId];

    /*
     * The limit is the maximum offset in bytes from the base address of the GDT
     */
    gdtr.limit     = X86_SEG_MAX * sizeof(X86_SEG_DESC) - 1;

    /*
     * Commit the GDT into the CPU, and update the segment
     * registers. The CS register may only be updated with a long jump
     * to an absolute address in the given segment (see Intel x86 doc
     * vol 3, section 4.8.1).
     */
    __asm__ __volatile__  ("lgdt %0          \n\
                            ljmp %1,    $1f  \n\
                            1:               \n\
                            movw %2,    %%ax \n\
                            movw %%ax,  %%ss \n\
                            movw %%ax,  %%ds \n\
                            movw %%ax,  %%es \n\
                            movw %%ax,  %%fs \n\
                            movw %%ax,  %%gs"
                          :
                          /*
                           * The real beginning of the GDT
                           * register is /after/ the "padding"
                           * field, ie at the "limit"
                           * field.
                           */
                          :"m"(gdtr.limit),
                           "i"(X86_CS_KERNEL),
                           "i"(X86_DS_KERNEL)
                          :"memory", "eax");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86GdtInit
** ��������: ��ʼ�� GDT
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86GdtInit (VOID)
{
    /*
     * ��ʱ CPU ���˽ṹ��û�г�ʼ����, ���� 0
     */

    /*
     * BSP BOOT CPU �е��� x86TssInit ����
     */
    return  (__x86GdtInit(0));
}
/*********************************************************************************************************
** ��������: x86GdtRegisterKernelTss
** ��������: ע�� KERNEL TSS
** �䡡��  : ulCPUId       CPU ID
**           ulTssVAddr    KERNEL TSS ��ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __x86GdtRegisterKernelTss (ULONG  ulCPUId, addr_t  ulTssVAddr)
{
    UINT16  usTssRegVal;

    /*
     * Initialize the GDT entry
     */
    _G_x86GDT[ulCPUId][X86_SEG_KERNEL_TSS] = (X86_SEG_DESC) {
        .limit_15_0             = 0x67,                     /*  See Intel x86 vol 3 section 6.2.2       */
        .base_paged_addr_15_0   = (ulTssVAddr) & 0xffff,
        .base_paged_addr_23_16  = (ulTssVAddr >> 16) & 0xff,
        .segment_type           = 0x9,                      /*  See Intel x86 vol 3 figure 6-3          */
        .descriptor_type        = 0,                        /*  (idem)                                  */
        .dpl                    = 3,                        /*  Allowed for CPL3 tasks                  */
        .present                = 1,
        .limit_19_16            = 0,                        /*  Size of a TSS is < 2^16 !               */
        .custom                 = 0,                        /*  Unused                                  */
        .longmode               = 0,
        .op_size                = 0,                        /*  See Intel x86 vol 3 figure 6-3          */
        .granularity            = 1,                        /*  limit is in Bytes                       */
        .base_paged_addr_31_24  = (ulTssVAddr >> 24) & 0xff
    };

    /*
     * Load the TSS register into the processor
     */
    usTssRegVal = X86_KERNEL_TSS;
    __asm__ __volatile__ ("ltr %0" :: "r"(usTssRegVal));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86TssInit
** ��������: ��ʼ�� TSS
** �䡡��  : ulCPUId       CPU ID
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __x86TssInit (ULONG  ulCPUId)
{
    X86_TSS  *pTss;
    addr_t    ulStackAddr;

    ulStackAddr = (addr_t)&_K_stkInterruptStack[ulCPUId][(LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)) - 1];
    ulStackAddr = ROUND_DOWN(ulStackAddr, ARCH_STK_ALIGN_SIZE);

    pTss = &_G_x86KernelTss[ulCPUId];

    /*
     * Reset the kernel TSS
     */
    lib_bzero(pTss, sizeof(X86_TSS));

    /*
     * Now setup the kernel TSS.
     *
     * Considering the privilege change method we choose (cpl3 -> cpl0
     * through a software interrupt), we don't need to initialize a
     * full-fledged TSS. See section 6.4.1 of Intel x86 vol 1. Actually,
     * only a correct value for the kernel esp and ss are required (aka
     * "ss0" and "esp0" fields). Since the esp0 will have to be updated
     * at privilege change time, we don't have to set it up now.
     */
    pTss->ss0  = X86_DS_KERNEL;
    pTss->esp0 = ulStackAddr;

    /*
     * Register this TSS into the gdt
     */
    __x86GdtRegisterKernelTss(ulCPUId, (addr_t)pTss);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86TssInit
** ��������: ��ʼ�� TSS
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86TssInit (VOID)
{
    /*
     * ��ʱ CPU ���˽ṹ��û�г�ʼ����, ���� 0
     */
    return  (__x86TssInit(0));
}
/*********************************************************************************************************
** ��������: x86GdtSecondaryInit
** ��������: Secondary CPU ��ʼ�� GDT
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86GdtSecondaryInit (VOID)
{
    /*
     * ��ʱ CPU ���˽ṹ�Ѿ���ʼ����, ���� LW_CPU_GET_CUR_ID()
     */
    ULONG  ulCPUId = LW_CPU_GET_CUR_ID();

    __x86GdtInit(ulCPUId);

    /*
     * BSP Secondary CPU ��û�е��� x86TssSecondaryInit ����, ����������� __x86TssInit ����
     */
    return  (__x86TssInit(ulCPUId));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
