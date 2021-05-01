/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: ppcMmuE500Tlb1.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 06 ��
**
** ��        ��: PowerPC E500 ��ϵ���� MMU TLB1 ������.
**
** ע        ��: ���������κβ���ϵͳ����, �����ڲ���ϵͳ��ʼ��ǰ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/arch_e500.h"
#include "arch/ppc/common/e500/ppcSprE500.h"
#include "./ppcMmuE500Reg.h"
#include "alloca.h"
/*********************************************************************************************************
  The E500 MMU has two levels.  In level 1, instruction and data are
  split, while they are unified in level 2.  Level 1 is maintained by
  the hardware and level 2 is maintained by the OS.  The number of entries
  are as follows:

  Name      Level   Type No of pg sizes Assoc     #entries     filled by
  I-L1VSP     L1    Instr     9         Full         4         TLB1 hit
  I-L1TLB4K   L1    Instr   1(4k)       4-way        64        TLB0 hit
  D-L1VSP     L1    Data      9         Full         4         TLB1 hit
  D-L1TLB4K   L1    Data    1(4k)       4-way        64        TLB0 hit
  TLB1        L2    I/D       9         Full         16        s/w tlbwe
  TLB0        L2    I/D     1(4k)       2-way        256       s/w tlbwe

  The VSP (variable sized page) are used as static entries like the BATs,
  while the 4k page are dynamic entries that gets loaded with the PTEs.
  When a TLB miss occur in TLB0, an exception occurs and the OS walks the
  data structure and copies a PTE into a TLB0 entry.  Hence, TLB1 is
  filled with mapping from the _G_tlb1StaticMapDesc[] array(in bspMap.h), and TLB0 is
  filled with mapping from the ppcMmuE500.c.

  Note that the E500 MMU cannot be turned off.
*********************************************************************************************************/
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static UINT             _G_uiTlb1Size = 0;                              /*  TLB1 �����С               */
       BOOL             _G_bMas2MBit  = LW_FALSE;                       /*  ���һ����                  */
       BOOL             _G_bHasMAS7   = LW_FALSE;                       /*  �Ƿ��� MAS7 �Ĵ���          */
       BOOL             _G_bHasHID1   = LW_FALSE;                       /*  �Ƿ��� HID1 �Ĵ���          */
/*********************************************************************************************************
  MAS �Ĵ�������
*********************************************************************************************************/
typedef struct {
    ULONG               MASR_ulFlag;                                    /*  ӳ���־                    */
    MAS1_REG            MASR_uiMAS1;                                    /*  MAS1                        */
    MAS2_REG            MASR_uiMAS2;                                    /*  MAS2                        */
    MAS3_REG            MASR_uiMAS3;                                    /*  MAS3                        */
    MAS7_REG            MASR_uiMAS7;                                    /*  MAS7                        */
} MAS_REGS;
/*********************************************************************************************************
** ��������: ppcE500MmuGlobalInit
** ��������: ���� BSP �� MMU TLB1 ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcE500MmuTLB1GlobalInit (CPCHAR  pcMachineName)
{
    MMUCFG_REG  uiMMUCFG;
    MAS4_REG    uiMAS4;
    UINT32      uiHID1;

    /*
     * ���� PID
     */
    uiMMUCFG.MMUCFG_uiValue = ppcE500MmuGetMMUCFG();
    ppcE500MmuSetPID0(0);
    if (uiMMUCFG.MMUCFG_ucNPIDS > 1) {
        ppcE500MmuSetPID1(0);
        if (uiMMUCFG.MMUCFG_ucNPIDS > 2) {
            ppcE500MmuSetPID2(0);
        }
    }

    /*
     * ���� MAS4
     */
    uiMAS4.MAS4_uiValue   = 0;
    uiMAS4.MAS4_ucTLBSELD = 0;                                          /*  Ĭ��ѡ�� TLB0               */
    uiMAS4.MAS4_ucTSIZED  = MMU_TSIZED;                                 /*  Ĭ��ҳ���С                */
    uiMAS4.MAS4_bX0D      = MMU_MAS4_X0D;
    uiMAS4.MAS4_bX1D      = MMU_MAS4_X1D;
    uiMAS4.MAS4_bWD       = LW_TRUE;                                    /*  Ĭ��д��͸ CACHE            */
    uiMAS4.MAS4_bID       = LW_TRUE;                                    /*  Ĭ�ϲ��� CACHE              */
    uiMAS4.MAS4_bMD       = LW_TRUE;                                    /*  Ĭ��һ����                  */
    uiMAS4.MAS4_bGD       = LW_TRUE;                                    /*  Ĭ����ֹ�²����            */
    uiMAS4.MAS4_bED       = LW_FALSE;                                   /*  Ĭ�ϴ��                    */

    ppcE500MmuSetMAS4(uiMAS4.MAS4_uiValue);

    /*
     * ʹ�ܵ�ַ�㲥
     */
    if (_G_bHasHID1) {
        uiHID1 = ppcE500GetHID1();
        if (MMU_MAS2_M) {
            uiHID1 |=  ARCH_PPC_HID1_ABE;
        } else {
            uiHID1 &= ~ARCH_PPC_HID1_ABE;
        }
        ppcE500SetHID1(uiHID1);
    }

    /*
     * �� MAS7 �Ĵ���, ��ʹ�� MAS7 �Ĵ����ķ���
     */
    if (_G_bHasMAS7) {
        UINT32  uiHID0;

        uiHID0  = ppcE500GetHID0();
        uiHID0 |= ARCH_PPC_HID0_MAS7EN;
        ppcE500SetHID0(uiHID0);
    }

    ppcE500MmuInvalidateTLB();                                          /*  ��Ч TLB0                   */
    ppcE500MmuTLB1Invalidate();                                         /*  ��Ч TLB1                   */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcE500MmuTLB1Init
** ��������: MMU TLB1 ϵͳ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcE500MmuTLB1Init (CPCHAR  pcMachineName)
{
    TLBCFG_REG  uiTLB1CFG;

    /*
     * ���һ������ʹ�� HID1[ABE] λ
     */
    MMU_MAS2_M = (LW_CFG_MAX_PROCESSORS > 1) ? 1 : 0;                   /*  ���һ����λ����            */

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500V2) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        _G_bHasMAS7 = LW_TRUE;
    } else {
        _G_bHasMAS7 = LW_FALSE;
    }

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        _G_bHasHID1 = LW_FALSE;
    } else {
        _G_bHasHID1 = LW_TRUE;
    }

    /*
     * ��� TLB1 ��Ŀ��
     */
    uiTLB1CFG.TLBCFG_uiValue = ppcE500MmuGetTLB1CFG();
    _G_uiTlb1Size = uiTLB1CFG.TLBCFG_usNENTRY;
}
/*********************************************************************************************************
** ��������: archE500MmuTLB1GlobalMap
** ��������: MMU TLB1 ȫ��ӳ��
** �䡡��  : pcMachineName          ʹ�õĻ�������
**           pdesc                  ӳ���ϵ����
**           pfuncPreRemoveTempMap  �Ƴ���ʱӳ��ǰ�Ļص�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �����ڹ��ж�����µ���
*********************************************************************************************************/
INT  archE500MmuTLB1GlobalMap (CPCHAR               pcMachineName,
                               PE500_TLB1_MAP_DESC  pdesc,
                               VOID                 (*pfuncPreRemoveTempMap)(VOID))
{
    E500_TLB1_MAP_DESC      desc;
    MAS0_REG                uiMAS0;
    MAS1_REG                uiMAS1;
    MAS2_REG                uiMAS2;
    MAS3_REG                uiMAS3;
    MAS7_REG                uiMAS7;
    UINT                    i;
    size_t                  stRemainSize;
    MAS_REGS               *masRegs;

    if (!pdesc) {
        return  (PX_ERROR);
    }

    if (LW_CPU_GET_CUR_ID() == 0) {
        ppcE500MmuTLB1Init(pcMachineName);
    }

    ppcE500MmuTLB1GlobalInit(pcMachineName);

    /*
     * ��һ��: ���������ڴ���Ϣ����
     */
    masRegs = (MAS_REGS *)alloca(sizeof(MAS_REGS) * _G_uiTlb1Size);     /*  ��ջ�����                  */
    lib_bzero(masRegs, sizeof(MAS_REGS) * _G_uiTlb1Size);

    desc         = *pdesc;                                              /*  �ӵ�һ����ʼ����            */
    stRemainSize = desc.TLB1D_stSize;

    for (i = 0; (i < _G_uiTlb1Size) && stRemainSize;) {
        if (!(desc.TLB1D_ulFlag & E500_TLB1_FLAG_VALID)) {              /*  ��Ч��ӳ���ϵ              */
            pdesc++;
            desc         = *pdesc;
            stRemainSize = desc.TLB1D_stSize;
            continue;
        }

        /*
         * MAS1
         */
        uiMAS1.MAS1_uiValue = 0;
        uiMAS1.MAS1_bVaild  = LW_TRUE;                                  /*   TLB ��Ч                   */
        uiMAS1.MAS1_bIPROT  = LW_TRUE;                                  /*   TLB ��Ч����               */
        uiMAS1.MAS1_usTID   = 0;                                        /*   ȫ��ӳ��                   */
        uiMAS1.MAS1_bTS     = 0;                                        /*   ��ַ�ռ� 0                 */

        if ((desc.TLB1D_stSize >= 1 * LW_CFG_GB_SIZE) &&
           !(desc.TLB1D_ui64PhyAddr & (1 * LW_CFG_GB_SIZE - 1))) {
            desc.TLB1D_stSize   = 1 * LW_CFG_GB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_1G;

        } else if ((desc.TLB1D_stSize >= 256 * LW_CFG_MB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (256 * LW_CFG_MB_SIZE - 1))) {
            desc.TLB1D_stSize   = 256 * LW_CFG_MB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_256M;

        } else if ((desc.TLB1D_stSize >= 64 * LW_CFG_MB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (64 * LW_CFG_MB_SIZE - 1))) {
            desc.TLB1D_stSize   = 64 * LW_CFG_MB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_64M;

        } else if ((desc.TLB1D_stSize >= 16 * LW_CFG_MB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (16 * LW_CFG_MB_SIZE - 1))) {
            desc.TLB1D_stSize   = 16 * LW_CFG_MB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_16M;

        } else if ((desc.TLB1D_stSize >= 4 * LW_CFG_MB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (4 * LW_CFG_MB_SIZE - 1))) {
            desc.TLB1D_stSize   = 4 * LW_CFG_MB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_4M;

        } else if ((desc.TLB1D_stSize >= 1 * LW_CFG_MB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (1 * LW_CFG_MB_SIZE - 1))) {
            desc.TLB1D_stSize   = 1 * LW_CFG_MB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_1M;

        } else if ((desc.TLB1D_stSize >= 256 * LW_CFG_KB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (256 * LW_CFG_KB_SIZE - 1))) {
            desc.TLB1D_stSize   = 256 * LW_CFG_KB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_256K;

        } else if ((desc.TLB1D_stSize >= 64 * LW_CFG_KB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (64 * LW_CFG_KB_SIZE - 1))) {
            desc.TLB1D_stSize   = 64 * LW_CFG_KB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_64K;

        } else if ((desc.TLB1D_stSize >= 16 * LW_CFG_KB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (16 * LW_CFG_KB_SIZE - 1))) {
            desc.TLB1D_stSize   = 16 * LW_CFG_KB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_16K;

        } else if ((desc.TLB1D_stSize >= 4 * LW_CFG_KB_SIZE) &&
                  !(desc.TLB1D_ui64PhyAddr & (4 * LW_CFG_KB_SIZE - 1))) {
            desc.TLB1D_stSize   = 4 * LW_CFG_KB_SIZE;
            uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_4K;

        } else {
            _BugFormat(LW_TRUE, LW_TRUE, "map size 0x%x is NOT 4K align!\r\n", pdesc->TLB1D_stSize);
        }

        /*
         * MAS2
         */
        uiMAS2.MAS2_uiValue = 0;
        uiMAS2.MAS2_uiEPN   = desc.TLB1D_ulVirMap >> MMU_RPN_SHIFT;
        uiMAS2.MAS2_bLittleEndian = LW_FALSE;                           /*  ���                        */

        if (desc.TLB1D_ulFlag & E500_TLB1_FLAG_CACHEABLE) {             /*  ��д CACHE                  */
            uiMAS2.MAS2_bUnCache = LW_FALSE;
            uiMAS2.MAS2_bWT      = LW_FALSE;
            uiMAS2.MAS2_bGuarded = LW_FALSE;

            if (MMU_MAS2_M) {
                uiMAS2.MAS2_bMemCoh = LW_TRUE;                          /*  ���һ����                  */
            }

        } else if (desc.TLB1D_ulFlag & E500_TLB1_FLAG_WRITETHROUGH) {   /*  д��͸ CACHE                */
            uiMAS2.MAS2_bUnCache = LW_FALSE;
            uiMAS2.MAS2_bWT      = LW_TRUE;
            uiMAS2.MAS2_bGuarded = LW_FALSE;

            if (MMU_MAS2_M) {
                uiMAS2.MAS2_bMemCoh = LW_TRUE;                          /*  ���һ����                  */
            }

        } else {                                                        /*  ������ CACHE                */
            uiMAS2.MAS2_bUnCache = LW_TRUE;
            uiMAS2.MAS2_bWT      = LW_TRUE;
            uiMAS2.MAS2_bGuarded = LW_TRUE;                             /*  ��ֹ�²����                */
        }

        /*
         * MAS3
         */
        uiMAS3.MAS3_uiValue = 0;                                        /*  MAS3                        */
        uiMAS3.MAS3_uiRPN   = (desc.TLB1D_ui64PhyAddr >> MMU_RPN_SHIFT) & 0xfffff;

        if (desc.TLB1D_ulFlag & E500_TLB1_FLAG_ACCESS) {
            uiMAS3.MAS3_bSuperRead = LW_TRUE;                           /*  ��Ȩ̬�ɶ�                  */
            uiMAS3.MAS3_bUserRead  = LW_FALSE;                          /*  �û�̬���ɶ�                */
        }

        if (desc.TLB1D_ulFlag & E500_TLB1_FLAG_WRITABLE) {
            uiMAS3.MAS3_bSuperWrite = LW_TRUE;                          /*  ��Ȩ̬��д                  */
            uiMAS3.MAS3_bUserWrite  = LW_FALSE;                         /*  �û�̬����д                */
        }

        if (desc.TLB1D_ulFlag & E500_TLB1_FLAG_EXECABLE) {
            uiMAS3.MAS3_bSuperExec = LW_TRUE;                           /*  ��Ȩ̬��ִ��                */
            uiMAS3.MAS3_bUserExec  = LW_FALSE;                          /*  �û�̬����ִ��              */
        }

        /*
         * MAS7
         */
        uiMAS7.MAS7_uiValue    = 0;
        uiMAS7.MAS7_uiHigh4RPN = desc.TLB1D_ui64PhyAddr >> 32;

        /*
         * �����������¼�� masRegs ����
         */
        masRegs[i].MASR_ulFlag = desc.TLB1D_ulFlag;
        masRegs[i].MASR_uiMAS1 = uiMAS1;
        masRegs[i].MASR_uiMAS2 = uiMAS2;
        masRegs[i].MASR_uiMAS3 = uiMAS3;
        masRegs[i].MASR_uiMAS7 = uiMAS7;

        i++;

        stRemainSize = stRemainSize - desc.TLB1D_stSize;
        if (stRemainSize > 0) {                                         /*  ��δ"ӳ��"�Ĳ���            */
            desc.TLB1D_ui64PhyAddr += desc.TLB1D_stSize;                /*  ����"ӳ��"ʣ��Ĳ���        */
            desc.TLB1D_ulVirMap    += desc.TLB1D_stSize;
            desc.TLB1D_stSize       = stRemainSize;

        } else {
            pdesc++;                                                    /*  "ӳ��"��һ��                */
            desc         = *pdesc;
            stRemainSize = desc.TLB1D_stSize;
        }
    }

    _BugHandle(i == _G_uiTlb1Size, LW_TRUE, "to many map desc!\r\n");

    /*
     * �ڶ���: �� masRegs �����¼��ֵ������������ӳ��
     */
    PPC_EXEC_INS("SYNC");

    for (i = 0; i < _G_uiTlb1Size; i++) {
        uiMAS0.MAS0_uiValue  = 0;                                       /*  MAS0                        */
        uiMAS0.MAS0_ucTLBSEL = 1;                                       /*  ѡ�� TLB1                   */
        uiMAS0.MAS0_ucESEL   = i;                                       /*  ѡ�� TLB1 �ĵ� i entry      */
        uiMAS0.MAS0_usNV     = 0;
        ppcE500MmuSetMAS0(uiMAS0.MAS0_uiValue);

        ppcE500MmuSetMAS0(uiMAS0.MAS0_uiValue);
        ppcE500MmuSetMAS1(masRegs[i].MASR_uiMAS1.MAS1_uiValue);
        ppcE500MmuSetMAS2(masRegs[i].MASR_uiMAS2.MAS2_uiValue);
        ppcE500MmuSetMAS3(masRegs[i].MASR_uiMAS3.MAS3_uiValue);

        if (_G_bHasMAS7) {
            ppcE500MmuSetMAS7(masRegs[i].MASR_uiMAS7.MAS7_uiValue);
        }

        PPC_EXEC_INS("ISYNC");
        PPC_EXEC_INS("SYNC");
        PPC_EXEC_INS("TLBWE");
        PPC_EXEC_INS("TLBSYNC");
        PPC_EXEC_INS("ISYNC");
    }

    if (pfuncPreRemoveTempMap) {
        pfuncPreRemoveTempMap();
    }

    /*
     * ������: ɾ����ʱӳ��
     */
    for (i = 0; i < _G_uiTlb1Size; i++) {
        if (masRegs[i].MASR_ulFlag & E500_TLB1_FLAG_TEMP) {
            uiMAS0.MAS0_uiValue  = 0;                                   /*  MAS0                        */
            uiMAS0.MAS0_ucTLBSEL = 1;                                   /*  ѡ�� TLB1                   */
            uiMAS0.MAS0_ucESEL   = i;                                   /*  ѡ�� TLB1 �ĵ� i entry      */
            uiMAS0.MAS0_usNV     = 0;
            ppcE500MmuSetMAS0(uiMAS0.MAS0_uiValue);

            ppcE500MmuSetMAS1(0);
            ppcE500MmuSetMAS2(0);
            ppcE500MmuSetMAS3(0);

            if (_G_bHasMAS7) {
                ppcE500MmuSetMAS7(0);
            }

            PPC_EXEC_INS("ISYNC");
            PPC_EXEC_INS("SYNC");
            PPC_EXEC_INS("TLBWE");
            PPC_EXEC_INS("TLBSYNC");
            PPC_EXEC_INS("ISYNC");
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
