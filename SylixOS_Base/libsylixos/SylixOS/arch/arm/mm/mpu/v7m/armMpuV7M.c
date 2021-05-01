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
** ��   ��   ��: armMpuV7M.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 11 �� 08 ��
**
** ��        ��: ARM ��ϵ���� MPU ����.
**
** BUG:
2018.1.8 Jiao.JinXing (������) ���� REGION ��ʹ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M ��ϵ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
/*********************************************************************************************************
  �Ĵ�����ַ
*********************************************************************************************************/
#define ARM_MPU_TYPE        (*(volatile UINT32 *)0xe000ed90)    /*  MPU Type Register                   */
#define ARM_MPU_CTRL        (*(volatile UINT32 *)0xe000ed94)    /*  MPU Control Register                */
#define ARM_MPU_RNR         (*(volatile UINT32 *)0xe000ed98)    /*  MPU Region Number Register          */
#define ARM_MPU_RBAR        (*(volatile UINT32 *)0xe000ed9c)    /*  MPU Region Base Address Register    */
#define ARM_MPU_RASR        (*(volatile UINT32 *)0xe000eda0)    /*  MPU Region Attr and Size Register   */
#define ARM_MPU_RBAR_A1     (*(volatile UINT32 *)0xe000eda4)    /*  MPU alias registers                 */
#define ARM_MPU_RASR_A1     (*(volatile UINT32 *)0xe000eda8)
#define ARM_MPU_RBAR_A2     (*(volatile UINT32 *)0xe000edac)
#define ARM_MPU_RASR_A2     (*(volatile UINT32 *)0xe000edb0)
#define ARM_MPU_RBAR_A3     (*(volatile UINT32 *)0xe000edb4)
#define ARM_MPU_RASR_A3     (*(volatile UINT32 *)0xe000edb8)
/*********************************************************************************************************
  TYPE �Ĵ�������
*********************************************************************************************************/
#define ARM_MPU_TYPE_OFF_DREGION        8
#define ARM_MPU_TYPE_MASK_DREGION       (0xff << ARM_MPU_TYPE_OFF_DREGION)
/*********************************************************************************************************
  CTRL �Ĵ�������
*********************************************************************************************************/
#define ARM_MPU_CTRL_OFT_PRIVDEFENA     2
#define ARM_MPU_CTRL_OFT_HFNMIENA       1
#define ARM_MPU_CTRL_OFT_ENABLE         0
/*********************************************************************************************************
  RASR �Ĵ�������
*********************************************************************************************************/
#define ARM_MPU_RASR_OFT_XN             28
#define ARM_MPU_RASR_OFT_AP             24
#define ARM_MPU_RASR_OFT_TEX            19
#define ARM_MPU_RASR_OFT_S              18
#define ARM_MPU_RASR_OFT_C              17
#define ARM_MPU_RASR_OFT_B              16
#define ARM_MPU_RASR_OFT_SRD            8
#define ARM_MPU_RASR_OFT_SIZE           1
#define ARM_MPU_RASR_OFT_EN             0
/*********************************************************************************************************
  DMA Pool
*********************************************************************************************************/
static LW_CLASS_HEAP    _G_heapDmaPool;
/*********************************************************************************************************
** ��������: armMpuV7MInit
** ��������: ��ʼ�� MPU
** �䡡��  : pcMachineName  ��������
**           mpuregion      �ڴ������벼�ֱ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armMpuV7MInit (CPCHAR  pcMachineName, const ARM_MPU_REGION  mpuregion[])
{
    PARM_MPU_REGION   pmpuregion;
    UINT32            uiVal;
    UINT32            uiAP;
    size_t            stSize;
    UINT32            uiMaxRegions;

    if (mpuregion == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU mpuregion[] invalid.\r\n");
        return;
    }

    ARM_MPU_CTRL &= ~1;

    uiMaxRegions = (ARM_MPU_TYPE & ARM_MPU_TYPE_MASK_DREGION) >> ARM_MPU_TYPE_OFF_DREGION;

    for (pmpuregion = (PARM_MPU_REGION)mpuregion;
         (pmpuregion->MPUD_ucSize  != ARM_MPU_REGION_SIZE_END) &&
         (pmpuregion->MPUD_ucNumber < uiMaxRegions);
         pmpuregion++) {

        ARM_MPU_RNR = (UINT32)pmpuregion->MPUD_ucNumber;

        if (pmpuregion->MPUD_bEnable) {
            ARM_MPU_RBAR = pmpuregion->MPUD_uiAddr;

            uiVal = (1 << ARM_MPU_RASR_OFT_EN);
            uiAP  = pmpuregion->MPUD_uiAP;

            if (uiAP & ARM_MPU_AP_FLAG_EXEC) {
                uiAP &= ~ARM_MPU_AP_FLAG_EXEC;

            } else {
                uiVal |= (1 << ARM_MPU_RASR_OFT_XN);
            }

            uiVal |= (uiAP << ARM_MPU_RASR_OFT_AP);

            switch (pmpuregion->MPUD_uiCPS) {

            case ARM_MPU_CPS_STRONG_ORDER_S:
                uiVal |= (1 << ARM_MPU_RASR_OFT_S);
                break;

            case ARM_MPU_CPS_DEVICE:
                uiVal |= (2 << ARM_MPU_RASR_OFT_TEX);
                break;

            case ARM_MPU_CPS_DEVICE_S:
                uiVal |= (1 << ARM_MPU_RASR_OFT_S);
                uiVal |= (1 << ARM_MPU_RASR_OFT_B);
                break;

            case ARM_MPU_CPS_NORMAL_WT:
                uiVal |= (1 << ARM_MPU_RASR_OFT_C);
                break;

            case ARM_MPU_CPS_NORMAL_WT_S:
                uiVal |= (1 << ARM_MPU_RASR_OFT_S);
                uiVal |= (1 << ARM_MPU_RASR_OFT_C);
                break;

            case ARM_MPU_CPS_NORMAL_WB:
                uiVal |= (1 << ARM_MPU_RASR_OFT_C);
                uiVal |= (1 << ARM_MPU_RASR_OFT_B);
                break;

            case ARM_MPU_CPS_NORMAL_WB_S:
                uiVal |= (1 << ARM_MPU_RASR_OFT_S);
                uiVal |= (1 << ARM_MPU_RASR_OFT_C);
                uiVal |= (1 << ARM_MPU_RASR_OFT_B);
                break;

            case ARM_MPU_CPS_NORMAL_NC:
                uiVal |= (1 << ARM_MPU_RASR_OFT_TEX);
                break;

            case ARM_MPU_CPS_NORMAL_NC_S:
                uiVal |= (1 << ARM_MPU_RASR_OFT_TEX);
                uiVal |= (1 << ARM_MPU_RASR_OFT_S);
                break;

            case ARM_MPU_CPS_NORMAL_WBA:
                uiVal |= (1 << ARM_MPU_RASR_OFT_TEX);
                uiVal |= (1 << ARM_MPU_RASR_OFT_C);
                uiVal |= (1 << ARM_MPU_RASR_OFT_B);
                break;

            case ARM_MPU_CPS_NORMAL_WBA_S:
                uiVal |= (1 << ARM_MPU_RASR_OFT_TEX);
                uiVal |= (1 << ARM_MPU_RASR_OFT_S);
                uiVal |= (1 << ARM_MPU_RASR_OFT_C);
                uiVal |= (1 << ARM_MPU_RASR_OFT_B);
                break;

            default:
                _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU CPS invalid.\r\n");
                return;
            }

            uiVal |= (pmpuregion->MPUD_ucSize << ARM_MPU_RASR_OFT_SIZE);

            ARM_MPU_RASR = uiVal;

            if (pmpuregion->MPUD_bDmaPool &&
                (pmpuregion->MPUD_ucSize >= ARM_MPU_REGION_SIZE_512B)) {
                stSize = ((size_t)2 << pmpuregion->MPUD_ucSize);
                if (!_G_heapDmaPool.HEAP_stTotalByteSize) {
                    _HeapCtor(&_G_heapDmaPool,
                              (PVOID)pmpuregion->MPUD_uiAddr, stSize);

                } else {
                    _HeapAddMemory(&_G_heapDmaPool, (PVOID)pmpuregion->MPUD_uiAddr, stSize);
                }
            }

        } else {
            ARM_MPU_RBAR = 0;
            ARM_MPU_RASR = 0;
        }
    }

    ARM_MPU_CTRL |= 1;
}
/*********************************************************************************************************
** ��������: armMpuV7MDmaAlloc
** ��������: �� DMA �ڴ���з����ڴ�
** �䡡��  : stSize     ��Ҫ���ڴ��С
** �䡡��  : �ڴ��׵�ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID  armMpuV7MDmaAlloc (size_t  stSize)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        return  (_HeapAllocate(&_G_heapDmaPool, stSize, __func__));

    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: armMpuV7MDmaAllocAlign
** ��������: �� DMA �ڴ���з����ڴ�
** �䡡��  : stSize     ��Ҫ���ڴ��С
**           stAlign    ����Ҫ��
** �䡡��  : �ڴ��׵�ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID  armMpuV7MDmaAllocAlign (size_t  stSize, size_t  stAlign)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        return  (_HeapAllocateAlign(&_G_heapDmaPool, stSize, stAlign, __func__));

    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: armMpuV7MDmaFree
** ��������: �ͷŴ� DMA �ڴ���з�����ڴ�
** �䡡��  : pvDmaMem   �ڴ��׵�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armMpuV7MDmaFree (PVOID  pvDmaMem)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        _HeapFree(&_G_heapDmaPool, pvDmaMem, LW_FALSE, __func__);
    }
}

#endif                                                                  /*  LW_CFG_ARM_MPU > 0          */
#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
