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
** ��   ��   ��: armMpuV7R.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 11 �� 08 ��
**
** ��        ��: ARM ��ϵ���� MPU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7R ��ϵ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_R__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern VOID     armMpuV7REnable(VOID);
extern VOID     armMpuV7RDisable(VOID);
extern VOID     armMpuV7RBgEnable(VOID);
extern VOID     armMpuV7RBgDisable(VOID);
extern UINT8    armMpuV7RGetRegionNum(VOID);
extern UINT8    armMpuV7RGetRegionNumIns(VOID);
extern UINT8    armMpuV7RAreRegionsSeparate(VOID);
extern VOID     armMpuV7RSelectRegion(UINT8);
extern VOID     armMpuV7RSetRegionBase(UINT32);
extern VOID     armMpuV7RSetRegionAP(UINT32);
extern VOID     armMpuV7RSetRegionSize(UINT32);
extern VOID     armMpuV7RSetRegionBaseIns(UINT32);
extern VOID     armMpuV7RSetRegionAPIns(UINT32);
extern VOID     armMpuV7RSetRegionSizeIns(UINT32);
/*********************************************************************************************************
  DMA Pool
*********************************************************************************************************/
static LW_CLASS_HEAP    _G_heapDmaPool;
/*********************************************************************************************************
** ��������: armMpuV7RGetAPDesc
** ��������: ת�� AP Ȩ��������
** �䡡��  : pmpuregion     �ڴ������벼�ֱ�
** �䡡��  : AP Ȩ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32  armMpuV7RGetAPDesc (const PARM_MPU_REGION   pmpuregion)
{
    UINT32  uiVal = 0;
    UINT32  uiAP  = pmpuregion->MPUD_uiAP;

    if (uiAP & ARM_MPU_AP_FLAG_EXEC) {
        uiAP &= ~ARM_MPU_AP_FLAG_EXEC;

    } else {
        uiVal |= 0x1000;
    }

    uiVal |= (uiAP << 8);

    switch (pmpuregion->MPUD_uiCPS) {

    case ARM_MPU_CPS_STRONG_ORDER_S:
        break;

    case ARM_MPU_CPS_DEVICE:
        uiVal |= 0x0010;
        break;

    case ARM_MPU_CPS_DEVICE_S:
        uiVal |= 0x0001;
        break;

    case ARM_MPU_CPS_NORMAL_WT:
        uiVal |= 0x0002;
        break;

    case ARM_MPU_CPS_NORMAL_WT_S:
        uiVal |= 0x0006;
        break;

    case ARM_MPU_CPS_NORMAL_WB:
        uiVal |= 0x0003;
        break;

    case ARM_MPU_CPS_NORMAL_WB_S:
        uiVal |= 0x0007;
        break;

    case ARM_MPU_CPS_NORMAL_NC:
        uiVal |= 0x0008;
        break;

    case ARM_MPU_CPS_NORMAL_NC_S:
        uiVal |= 0x000c;
        break;

    case ARM_MPU_CPS_NORMAL_WBA:
        uiVal |= 0x000b;
        break;

    case ARM_MPU_CPS_NORMAL_WBA_S:
        uiVal |= 0x000f;
        break;

    default:
        _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU CPS invalid.\r\n");
        return  (0);
    }

    return  (uiVal);
}
/*********************************************************************************************************
** ��������: armMpuV7RInit
** ��������: ��ʼ�� MPU
** �䡡��  : pcMachineName  ��������
**           mpuregion      �ڴ������벼�ֱ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armMpuV7RInit (CPCHAR  pcMachineName, const ARM_MPU_REGION  mpuregion[])
{
    UINT8             ucSeparate;
    UINT8             ucMaxRegions;
    UINT8             ucMaxRegionsIns;

    PARM_MPU_REGION   pmpuregion;
    UINT32            uiVal;
    size_t            stSize;

    if (mpuregion == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU mpuregion[] invalid.\r\n");
        return;
    }

    armMpuV7RDisable();
    armMpuV7RBgDisable();

    ucMaxRegions = armMpuV7RGetRegionNum();
    ucSeparate   = armMpuV7RAreRegionsSeparate() & 1;
    if (ucSeparate) {
        ucMaxRegionsIns = armMpuV7RGetRegionNumIns();
    } else {
        ucMaxRegionsIns = 0;
    }

    pmpuregion = (PARM_MPU_REGION)mpuregion;
    while (pmpuregion->MPUD_ucSize != ARM_MPU_REGION_SIZE_END) {
        if (ucSeparate) {
            if (pmpuregion->MPUD_ucIsIns) {
                if (pmpuregion->MPUD_ucNumber >= ucMaxRegionsIns) {
                    _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU region invalid.\r\n");
                    break;
                }

                armMpuV7RSelectRegion(pmpuregion->MPUD_ucNumber);
                armMpuV7RSetRegionBaseIns(pmpuregion->MPUD_uiAddr);

                uiVal = (pmpuregion->MPUD_ucSize << 1) | ((pmpuregion->MPUD_bEnable) ? 1 : 0);
                armMpuV7RSetRegionSizeIns(uiVal);

                uiVal = armMpuV7RGetAPDesc(pmpuregion);
                armMpuV7RSetRegionAPIns(uiVal);

            } else {
                if (pmpuregion->MPUD_ucNumber >= ucMaxRegions) {
                    _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU region invalid.\r\n");
                    break;
                }

                armMpuV7RSelectRegion(pmpuregion->MPUD_ucNumber);
                armMpuV7RSetRegionBase(pmpuregion->MPUD_uiAddr);

                uiVal = (pmpuregion->MPUD_ucSize << 1) | ((pmpuregion->MPUD_bEnable) ? 1 : 0);
                armMpuV7RSetRegionSize(uiVal);

                uiVal = armMpuV7RGetAPDesc(pmpuregion);
                armMpuV7RSetRegionAP(uiVal);
            }

        } else {
            if (pmpuregion->MPUD_ucNumber >= ucMaxRegions) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU region invalid.\r\n");
                break;
            }

            armMpuV7RSelectRegion(pmpuregion->MPUD_ucNumber);
            armMpuV7RSetRegionBase(pmpuregion->MPUD_uiAddr);

            uiVal = (pmpuregion->MPUD_ucSize << 1) | ((pmpuregion->MPUD_bEnable) ? 1 : 0);
            armMpuV7RSetRegionSize(uiVal);

            uiVal = armMpuV7RGetAPDesc(pmpuregion);
            armMpuV7RSetRegionAP(uiVal);
        }

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

        pmpuregion++;
    }

    armMpuV7REnable();
}
/*********************************************************************************************************
** ��������: armMpuV7RDmaAlloc
** ��������: �� DMA �ڴ���з����ڴ�
** �䡡��  : stSize     ��Ҫ���ڴ��С
** �䡡��  : �ڴ��׵�ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID  armMpuV7RDmaAlloc (size_t  stSize)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        return  (_HeapAllocate(&_G_heapDmaPool, stSize, __func__));

    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: armMpuV7RDmaAllocAlign
** ��������: �� DMA �ڴ���з����ڴ�
** �䡡��  : stSize     ��Ҫ���ڴ��С
**           stAlign    ����Ҫ��
** �䡡��  : �ڴ��׵�ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID  armMpuV7RDmaAllocAlign (size_t  stSize, size_t  stAlign)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        return  (_HeapAllocateAlign(&_G_heapDmaPool, stSize, stAlign, __func__));

    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: armMpuV7RDmaFree
** ��������: �ͷŴ� DMA �ڴ���з�����ڴ�
** �䡡��  : pvDmaMem   �ڴ��׵�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armMpuV7RDmaFree (PVOID  pvDmaMem)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        _HeapFree(&_G_heapDmaPool, pvDmaMem, LW_FALSE, __func__);
    }
}

#endif                                                                  /*  LW_CFG_ARM_MPU > 0          */
#endif                                                                  /*  __SYLIXOS_ARM_ARCH_R__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
