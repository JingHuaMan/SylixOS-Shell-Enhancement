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
** ��   ��   ��: x86Topology.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 04 �� 13 ��
**
** ��        ��: x86 ��ϵ���ܴ����� CPU ����.
*********************************************************************************************************/

#ifndef __X86_TOPOLOGY_H
#define __X86_TOPOLOGY_H

/*********************************************************************************************************
  ͷ�ļ�
*********************************************************************************************************/
#include "arch/x86/common/x86CpuId.h"
#include "arch/x86/apic/x86IoApic.h"
#include "arch/x86/apic/x86LocalApic.h"
/*********************************************************************************************************
  CPU �����ͼ�����صĺ�
*********************************************************************************************************/
typedef UINT    X86_CPUSET_T;

#define X86_CPUSET_SET(cpuset, n)       ((cpuset) |= (1 << (n)))
#define X86_CPUSET_ISSET(cpuset, n)     ((cpuset) & (1 << (n)))
#define X86_CPUSET_CLR(cpuset, n)       ((cpuset) &= ~(1 << (n)))
#define X86_CPUSET_ZERO(cpuset)         ((cpuset) = 0)
#define X86_CPUSET_ISZERO(cpuset)       ((cpuset) == 0)
/*********************************************************************************************************
  CPU ���˽ṹ
*********************************************************************************************************/
typedef struct {
    UINT            TOP_uiMaxLProcsCore;                    /*  Maximum Logical Processors per Core     */
    UINT            TOP_uiMaxCoresPkg;                      /*  Maximum Number of Cores per Package     */
    UINT            TOP_uiMaxLProcsPkg;                     /*  Maximum Logical Processors per Package  */

    UINT            TOP_uiSmtMaskWidth;                     /*  SMT Bit Width                           */
    UINT            TOP_uiCoreMaskWidth;                    /*  Core Bit Width                          */
    UINT            TOP_uiPkgMaskWidth;                     /*  Package Bit Width                       */

    UINT            TOP_uiNumCores;                         /*  Number of Cores                         */
    UINT            TOP_uiNumPkgs;                          /*  Number of Packages                      */
    UINT            TOP_uiNumLProcsEnabled;                 /*  Number of Logical Processors Enabled    */

    UCHAR           TOP_uiPkgIdMask;                        /*  Package ID Mask                         */

    UCHAR           TOP_aucSmtIds[X86_CPUID_MAX_NUM_CPUS];  /*  SMT Ids                                 */
    UCHAR           TOP_aucCoreIds[X86_CPUID_MAX_NUM_CPUS]; /*  Core Ids                                */
    UCHAR           TOP_aucPkgIds[X86_CPUID_MAX_NUM_CPUS];  /*  Package Ids                             */

    UINT            TOP_auiPhysIdx[X86_CPUID_MAX_NUM_CPUS]; /*  CPU ID table                            */

    X86_CPUSET_T    TOP_smtSet[X86_CPUID_MAX_NUM_CPUS];     /*  SMT CPU set                             */

    UINT8           TOP_ucCpuBSP;                           /*  BSP Local APIC ID                       */
} X86_CPU_TOPOLOGY;
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
extern X86_CPU_TOPOLOGY _G_x86CpuTopology;                  /*  CPU ����                                */
                                                            /*  Local APIC ID->�߼�������ID             */
extern UINT8            _G_aucX86CpuIndexTable[X86_CPUID_MAX_NUM_CPUS];   
extern UINT             _G_uiX86BaseCpuPhysIndex;           /*  Base CPU Phy index                      */
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
/*
 * �߼� Processor ��Ŀ
 */
#define X86_LPROC_NR                    (_G_x86CpuTopology.TOP_uiNumLProcsEnabled)

/*
 * ���� Processor ��Ŀ
 */
#define X86_PPROC_NR                    (_G_x86CpuTopology.TOP_uiNumCores)

/*
 * �ж��Ƿ��� SMT ����
 */
#define X86_HAS_SMT                     (X86_LPROC_NR != X86_PPROC_NR)

/*
 * APICID -> LOGICID(0 ~ LW_NCPUS-1)
 */
#define X86_APICID_TO_LOGICID(apicid)   (_G_aucX86CpuIndexTable[(apicid)] - _G_uiX86BaseCpuPhysIndex)

/*
 * LOGICID(0 ~ LW_NCPUS-1) -> APICID
 */
#define X86_LOGICID_TO_APICID(logicid)  (_G_x86CpuTopology.TOP_auiPhysIdx[(logicid) + _G_uiX86BaseCpuPhysIndex])

/*
 * LOGICID -> PHYID
 */
#define X86_LOGICID_TO_PHYID(logicid)   (X86_HAS_SMT ? \
                                        ((logicid) >> _G_x86CpuTopology.TOP_uiSmtMaskWidth) : (logicid))

/*
 * �߼� Processor �Ƿ����
 */
#define X86_APICID_PRESEND(apicid)      ((apicid) == _G_x86CpuTopology.TOP_ucCpuBSP ? \
                                        LW_TRUE : (BOOL)X86_APICID_TO_LOGICID(apicid))

/*
 * �߼� Processor �Ƿ�Ϊ HT ��
 */
#define X86_APICID_IS_HT(apicid)        (X86_HAS_SMT && ((apicid) & _G_x86CpuTopology.TOP_uiSmtMaskWidth))

/*
 * ��� BSP �� APICID
 */
#define X86_BSP_APICID                  (_G_x86CpuTopology.TOP_ucCpuBSP)
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LW_INLINE UINT  x86CpuPhysIndexGet (VOID)
{
    return  (_G_aucX86CpuIndexTable[x86LocalApicId()]);
}

#endif                                                                  /*  __X86_TOPOLOGY_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
