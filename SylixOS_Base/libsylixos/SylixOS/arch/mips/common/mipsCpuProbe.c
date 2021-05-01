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
** ��   ��   ��: mipsCpuProbe.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 07 �� 18 ��
**
** ��        ��: MIPS ��ϵ�ܹ� CPU ̽��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/mips/common/cp0/mipsCp0.h"
#include "mipsCpuProbe.h"
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
UINT32         _G_uiMipsProcessorId = 0;                                /*  ������ ID                   */
UINT32         _G_uiMipsPridRev     = 0;                                /*  ������ ID Revision          */
UINT32         _G_uiMipsPridImp     = 0;                                /*  ������ ID Implementation    */
UINT32         _G_uiMipsPridComp    = 0;                                /*  ������ ID Company           */
UINT32         _G_uiMipsPridOpt     = 0;                                /*  ������ ID Option            */
MIPS_CPU_TYPE  _G_uiMipsCpuType     = CPU_UNKNOWN;                      /*  ����������                  */
UINT32         _G_uiMipsMachineType = MIPS_MACHINE_TYPE_24KF;           /*  ��������                    */
/*********************************************************************************************************
** ��������: mispCpuProbeLegacy
** ��������: ̽�� Legacy CPU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mispCpuProbeLegacy (VOID)
{
    switch (_G_uiMipsPridImp) {

    case PRID_IMP_R4000:
        _G_uiMipsCpuType = CPU_R4000PC;
        break;

    case PRID_IMP_LOONGSON_64:                                          /*  Loongson-2x/3x              */
        switch (_G_uiMipsPridRev) {

        case PRID_REV_LOONGSON2E:                                       /*  Loongson-2E/2F              */
        case PRID_REV_LOONGSON2F:
            _G_uiMipsCpuType = CPU_LOONGSON2;
            break;

        case PRID_REV_LOONGSON3A_R1:                                    /*  Loongson-3x/2G/2H           */
        case PRID_REV_LOONGSON3A_R2:
        case PRID_REV_LOONGSON3A_R3_0:
        case PRID_REV_LOONGSON3A_R3_1:
        case PRID_REV_LOONGSON3B_R1:
        case PRID_REV_LOONGSON3B_R2:
            _G_uiMipsCpuType = CPU_LOONGSON3;
            break;
        }
        break;

    case PRID_IMP_LOONGSON2K:
        switch (_G_uiMipsPridRev) {

        case PRID_REV_LOONGSON2K_R1:
        case PRID_REV_LOONGSON2K_R2:
            _G_uiMipsCpuType = CPU_LOONGSON2K;                          /*  Loongson-2K                 */
            break;
        }
        break;

    case PRID_IMP_LOONGSON_32:                                          /*  Loongson-1x                 */
        _G_uiMipsCpuType = CPU_LOONGSON1;
        break;

    case PRID_IMP_CETC_HR2:                                             /*  CETC-HR2                    */
        _G_uiMipsCpuType = CPU_CETC_HR2;
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** ��������: mispCpuProbeIngenic
** ��������: ̽����� CPU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mispCpuProbeIngenic (VOID)
{
    switch (_G_uiMipsPridImp) {

    case PRID_IMP_JZRISC:
        _G_uiMipsCpuType = CPU_JZRISC;
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** ��������: mispCpuProbeMips
** ��������: ̽�� MIPS CPU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mispCpuProbeMips (VOID)
{
    switch (_G_uiMipsPridImp) {

    case PRID_IMP_QEMU_GENERIC:
        _G_uiMipsCpuType = CPU_QEMU_GENERIC;
        break;

    case PRID_IMP_24K:
    case PRID_IMP_24KE:
        _G_uiMipsCpuType = CPU_24K;
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** ��������: mipsCpuProbe
** ��������: MIPS CPU ̽��
** �䡡��  : pcMachineName     ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsCpuProbe (CPCHAR  pcMachineName)
{
    static  BOOL  bIsProbed = LW_FALSE;

    if (bIsProbed) {
        return;
    }
    bIsProbed = LW_TRUE;

    if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS1X) == 0)) {
        _G_uiMipsMachineType = MIPS_MACHINE_TYPE_LS1X;

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS2X) == 0)) {
        _G_uiMipsMachineType = MIPS_MACHINE_TYPE_LS2X;

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS3X) == 0)) {
        _G_uiMipsMachineType = MIPS_MACHINE_TYPE_LS3X;

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_JZ47XX) == 0)) {
        _G_uiMipsMachineType = MIPS_MACHINE_TYPE_JZ47XX;

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_HR2) == 0)) {
        _G_uiMipsMachineType = MIPS_MACHINE_TYPE_HR2;
    }

    _G_uiMipsProcessorId = mipsCp0PRIdRead();
    _G_uiMipsPridRev     = _G_uiMipsProcessorId & PRID_REV_MASK;
    _G_uiMipsPridImp     = _G_uiMipsProcessorId & PRID_IMP_MASK;
    _G_uiMipsPridComp    = _G_uiMipsProcessorId & PRID_COMP_MASK;
    _G_uiMipsPridOpt     = _G_uiMipsProcessorId & PRID_OPT_MASK;

    switch (_G_uiMipsPridComp) {

    case PRID_COMP_CETC:
    case PRID_COMP_LOONGSON:
    case PRID_COMP_LEGACY:
        mispCpuProbeLegacy();
        break;

    case PRID_COMP_MIPS:
        mispCpuProbeMips();
        break;

    case PRID_COMP_INGENIC_D0:
    case PRID_COMP_INGENIC_D1:
    case PRID_COMP_INGENIC_E1:
        mispCpuProbeIngenic();
        break;
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
