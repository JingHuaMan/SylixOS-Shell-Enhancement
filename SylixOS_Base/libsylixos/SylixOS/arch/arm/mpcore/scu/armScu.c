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
** ��   ��   ��: armScu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2014 �� 01 �� 03 ��
**
** ��        ��: ARM SNOOP CONTROL UNIT.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#include "armScu.h"
#include "../../common/cp15/armCp15.h"
/*********************************************************************************************************
  Snoop Control Unit �Ĵ���ƫ��
*********************************************************************************************************/
#define SCU_REGS_BASE_OFFSET    0                       /*  Snoop Control Unit �Ĵ���ƫ��               */
/*********************************************************************************************************
  Snoop Control Unit �Ĵ���
*********************************************************************************************************/
typedef struct {
    volatile UINT32     SCU_uiControl;                  /*  SCU Control Register.                       */
    volatile UINT32     SCU_uiConfigure;                /*  SCU Configuration Register.                 */
    volatile UINT32     SCU_uiCpuPowerStatus;           /*  SCU CPU Power Status Register.              */
    volatile UINT32     SCU_uiInvalidateAll;            /*  SCU Invalidate All Registers in Secure State*/
    volatile UINT32     SCU_uiReserves1[12];            /*  Reserves.                                   */
    volatile UINT32     SCU_uiFilteringStart;           /*  Filtering Start Address Register.           */
    volatile UINT32     SCU_uiFilteringEnd;             /*  Filtering End Address Register.             */
    volatile UINT32     SCU_uiReserves2[2];
    volatile UINT32     SCU_uiSAC;                      /*  SCU Access Control (SAC) Register.          */
    volatile UINT32     SCU_uiSNSAC;                    /*  SCU Non-secure Access Control (SNSAC) Reg.  */
} SCU_REGS;
/*********************************************************************************************************
** ��������: armScuGet
** ��������: ��� SNOOP CONTROL UNIT
** �䡡��  : NONE
** �䡡��  : SNOOP CONTROL UNIT ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE SCU_REGS  *armScuGet (VOID)
{
    REGISTER addr_t  ulBase = armPrivatePeriphBaseGet() + SCU_REGS_BASE_OFFSET;

    return  ((SCU_REGS *)ulBase);
}
/*********************************************************************************************************
** ��������: armScuFeatureEnable
** ��������: SCU ����ʹ��
** �䡡��  : uiFeatures        ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armScuFeatureEnable (UINT32  uiFeatures)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    write32(read32((addr_t)&pScu->SCU_uiControl) | uiFeatures, (addr_t)&pScu->SCU_uiControl);
}
/*********************************************************************************************************
** ��������: armScuFeatureDisable
** ��������: SCU ���Խ���
** �䡡��  : uiFeatures        ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armScuFeatureDisable (UINT32  uiFeatures)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    write32(read32((addr_t)&pScu->SCU_uiControl) & (~uiFeatures), (addr_t)&pScu->SCU_uiControl);
}
/*********************************************************************************************************
** ��������: armScuFeatureGet
** ��������: ��� SCU ����
** �䡡��  : NONE
** �䡡��  : SCU ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT32  armScuFeatureGet (VOID)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    return  (read32((addr_t)&pScu->SCU_uiControl));
}
/*********************************************************************************************************
** ��������: armScuTagRamSize
** ��������: ��� Tag Ram Size
** �䡡��  : NONE
** �䡡��  : Tag Ram Size
**
**      Bits [15:14] indicate Cortex-A9 processor CPU3 tag RAM size if present.
**      Bits [13:12] indicate Cortex-A9 processor CPU2 tag RAM size if present.
**      Bits [11:10] indicate Cortex-A9 processor CPU1 tag RAM size if present.
**      Bits [9:8]   indicate Cortex-A9 processor CPU0 tag RAM size.
**      The encoding is as follows:
**      b00   16KB cache,  64 indexes per tag RAM.
**      b01   32KB cache, 128 indexes per tag RAM.
**      b10   64KB cache, 256 indexes per tag RAM.
**      b11   Reserved
**
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT32  armScuTagRamSize (VOID)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    return  ((read32((addr_t)&pScu->SCU_uiConfigure) >> 8) & 0xFF);
}
/*********************************************************************************************************
** ��������: armScuCpuMpStatus
** ��������: ��� CPU MP ״̬
** �䡡��  : NONE
** �䡡��  : CPU MP ״̬
**
**      0  This Cortex-A9 processor is in AMP mode, not taking part in coherency, or not present.
**      1  This Cortex-A9 processor is in SMP mode, taking part in coherency.
**
**      Bit 3 is for CPU3
**      Bit 2 is for CPU2
**      Bit 1 is for CPU1
**      Bit 0 is for CPU0
**
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT32  armScuCpuMpStatus (VOID)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    return  ((read32((addr_t)&pScu->SCU_uiConfigure) >> 4) & 0xF);
}
/*********************************************************************************************************
** ��������: armScuCpuNumber
** ��������: ��� CPU ��Ŀ
** �䡡��  : NONE
** �䡡��  : CPU ��Ŀ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT32  armScuCpuNumber (VOID)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    return  (((read32((addr_t)&pScu->SCU_uiConfigure) >> 0) & 0x3) + 1);
}
/*********************************************************************************************************
** ��������: armScuSecureInvalidateAll
** ��������: Invalidates the SCU tag RAMs on a per Cortex-A9 processor and per way basis
** �䡡��  : uiCPUId       CPU ID
**           uiWays        Specifies the ways that must be invalidated for CPU(ID)
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armScuSecureInvalidateAll (UINT32  uiCPUId,  UINT32  uiWays)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    switch (uiCPUId) {

    case 0:
    case 1:
    case 2:
    case 3:
        uiWays &= 0xF;
        write32(uiWays << (uiCPUId * 4), (addr_t)&pScu->SCU_uiInvalidateAll);
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** ��������: armScuFilteringSet
** ��������: ���� Filtering Start and End Address
** �䡡��  : uiStart       Start Address
**           uiEnd         End Address
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armScuFilteringSet (UINT32  uiStart,  UINT32  uiEnd)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    write32(uiStart, (addr_t)&pScu->SCU_uiFilteringStart);
    write32(uiEnd,   (addr_t)&pScu->SCU_uiFilteringEnd);
}
/*********************************************************************************************************
** ��������: armScuAccessCtrlSet
** ��������: ���� SCU ���ʿ���
** �䡡��  : uiCpuBits         CPU λ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armScuAccessCtrlSet (UINT32  uiCpuBits)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    write32(uiCpuBits, (addr_t)&pScu->SCU_uiSAC);
}
/*********************************************************************************************************
** ��������: armScuNonAccessCtrlSet
** ��������: ���� SCU �ǰ�ȫģʽ���ʿ���
** �䡡��  : uiValue           ֵ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armScuNonAccessCtrlSet (UINT32  uiValue)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    write32(uiValue, (addr_t)&pScu->SCU_uiSNSAC);
}
/*********************************************************************************************************
** ��������: armScuCpuPowerStatusGet
** ��������: ��� CPU ��Դ״̬
** �䡡��  : NONE
** �䡡��  : CPU ��Դ״̬
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT32  armScuCpuPowerStatusGet (VOID)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    return  (read32((addr_t)&pScu->SCU_uiCpuPowerStatus));
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
