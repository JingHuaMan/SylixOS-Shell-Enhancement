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
** ��   ��   ��: cskyMpu.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 11 �� 12 ��
**
** ��        ��: C-SKY ��ϵ�ܹ� MPU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  C-SKY ��ϵ�ܹ�
*********************************************************************************************************/
#if defined(__SYLIXOS_CSKY_ARCH_CK803__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_VMM_EN == 0) && (LW_CFG_CSKY_MPU > 0)
#include "arch/csky/inc/cskyregs.h"
/*********************************************************************************************************
  �ɸ߻������üĴ�����ַ
*********************************************************************************************************/
#define CSKY_CACHE_CRCR(n)                (*(volatile UINT32 *)(0xe000f008 + 0x4 * (n)))
/*********************************************************************************************************
  CRCR �Ĵ�������
*********************************************************************************************************/
#define CSKY_CACHE_CRCR_OFF_EN            0
#define CSKY_CACHE_CRCR_MASK_EN           (0x1 << CSKY_CACHE_CRCR_OFF_EN)

#define CSKY_CACHE_CRCR_OFF_SIZE          1
#define CSKY_CACHE_CRCR_MASK_SIZE         (0x1f << CSKY_CACHE_CRCR_OFF_SIZE)

#define CSKY_CACHE_CRCR_OFF_BASE_ADDR     10
#define CSKY_CACHE_CRCR_MASK_BASE_ADDR    (0x3fffff << CSKY_CACHE_CRCR_OFF_BASE_ADDR)
/*********************************************************************************************************
  CAPR �Ĵ�������
*********************************************************************************************************/
#define CSKY_MPU_CAPR_OFF_NX              0
#define CSKY_MPU_CAPR_OFF_AP              8
#define CSKY_MPU_CAPR_OFF_S               24
/*********************************************************************************************************
  PACR �Ĵ�������
*********************************************************************************************************/
#define CSKY_MPU_PACR_OFF_E               0
#define CSKY_MPU_PACR_MASK_E              (0x1 << CSKY_MPU_PACR_OFF_E)

#define CSKY_MPU_PACR_OFF_SIZE            1
#define CSKY_MPU_PACR_MASK_SIZE           (0x1f << CSKY_MPU_PACR_OFF_SIZE)

#define CSKY_MPU_PACR_OFF_BASE_ADDR       12
#define CSKY_MPU_PACR_MASK_BASE_ADDR      (0xfffff << CSKY_MPU_PACR_OFF_BASE_ADDR)
/*********************************************************************************************************
  PRSR �Ĵ�������
*********************************************************************************************************/
#define CSKY_MPU_PRSR_OFF_RID             0
#define CSKY_MPU_PRSR_MASK_RID            (0x7 << CSKY_MPU_PRSR_OFF_RID)
/*********************************************************************************************************
  DMA Pool
*********************************************************************************************************/
static LW_CLASS_HEAP    _G_heapDmaPool;
/*********************************************************************************************************
** ��������: cskyMpuInit
** ��������: ��ʼ�� MPU
** �䡡��  : pcMachineName  ��������
**           mpuregion      �ڴ������벼�ֱ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  cskyMpuInit (CPCHAR  pcMachineName, const CSKY_MPU_REGION  mpuregion[])
{
    PCSKY_MPU_REGION   pmpuregion;
    UINT32             uiCAPR;
    UINT32             uiPACR;
    UINT32             uiCRCR;
    size_t             stSize;

    if (mpuregion == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU mpuregion[] invalid.\r\n");
        return;
    }

    cskyMpuSetCCR(cskyMpuGetCCR() & ~1);                                /*  ���� MPU                    */

    for (pmpuregion = (PCSKY_MPU_REGION)mpuregion;
         (pmpuregion->MPUD_ucSize      != CSKY_MPU_REGION_SIZE_END) &&
         (pmpuregion->MPUD_ucMpuNumber < LW_CFG_MPU_REGION_NUM);
         pmpuregion++) {

        cskyMpuSetPRSR((UINT32)pmpuregion->MPUD_ucMpuNumber);           /*  ѡ�񱣻���                  */

        if (pmpuregion->MPUD_bEnable) {
            uiCAPR  = cskyMpuGetCAPR();

            uiPACR  = cskyMpuGetPACR() & ~CSKY_MPU_PACR_MASK_BASE_ADDR; /*  ���û���ַ                  */
            uiPACR |= pmpuregion->MPUD_uiAddr & CSKY_MPU_PACR_MASK_BASE_ADDR;

            if (pmpuregion->MPUD_ucSize != CSKY_MPU_REGION_SIZE_4KB) {
                uiPACR &= ~(((1u << (pmpuregion->MPUD_ucSize - 11)) - 1) << 12);
            }

            uiPACR |= ((UINT32)(pmpuregion->MPUD_ucSize) << CSKY_MPU_PACR_OFF_SIZE) & CSKY_MPU_PACR_MASK_SIZE;
            uiPACR |= (1 << CSKY_MPU_PACR_OFF_E);

            uiCAPR &= ~((0x1 <<  pmpuregion->MPUD_ucMpuNumber) |
                        (0x3 << (pmpuregion->MPUD_ucMpuNumber * 2 + CSKY_MPU_CAPR_OFF_AP)) |
                        (0x1 << (pmpuregion->MPUD_ucMpuNumber + CSKY_MPU_CAPR_OFF_S)));
            uiCAPR |= (((pmpuregion->MPUD_uiAttr & CSKY_MPU_ATTR_NX) >>
                         CSKY_MPU_ATTR_NX_POS) << pmpuregion->MPUD_ucMpuNumber) |
                      (((pmpuregion->MPUD_uiAttr & CSKY_MPU_ATTR_AP) >>
                         CSKY_MPU_ATTR_AP_POS) << (pmpuregion->MPUD_ucMpuNumber * 2 + CSKY_MPU_CAPR_OFF_AP)) |
                      (((pmpuregion->MPUD_uiAttr & CSKY_MPU_ATTR_S)  >>
                         CSKY_MPU_ATTR_S_POS)  << (pmpuregion->MPUD_ucMpuNumber + CSKY_MPU_CAPR_OFF_AP));

            cskyMpuSetCAPR(uiCAPR);
            cskyMpuSetPACR(uiPACR);

            if (pmpuregion->MPUD_bDmaPool) {
                stSize = ((size_t)2 << pmpuregion->MPUD_ucSize);
                if (!_G_heapDmaPool.HEAP_stTotalByteSize) {
                    _HeapCtor(&_G_heapDmaPool,
                              (PVOID)pmpuregion->MPUD_uiAddr, stSize);

                } else {
                    _HeapAddMemory(&_G_heapDmaPool, (PVOID)pmpuregion->MPUD_uiAddr, stSize);
                }

            } else {
                uiCRCR  = pmpuregion->MPUD_uiAddr  & CSKY_CACHE_CRCR_MASK_BASE_ADDR;
                uiCRCR |= (pmpuregion->MPUD_ucSize << CSKY_CACHE_CRCR_OFF_SIZE) &
                           CSKY_CACHE_CRCR_MASK_SIZE;
                uiCRCR |= (1 << CSKY_CACHE_CRCR_OFF_EN);

                CSKY_CACHE_CRCR(pmpuregion->MPUD_ucMpuNumber) = uiCRCR;
            }

        } else {
            cskyMpuSetPACR(0);
        }
    }

    cskyMpuSetCCR(cskyMpuGetCCR() | 1);                                 /*  ʹ�� MPU                    */
}
/*********************************************************************************************************
** ��������: cskyMpuDmaAlloc
** ��������: �� DMA �ڴ���з����ڴ�
** �䡡��  : stSize     ��Ҫ���ڴ��С
** �䡡��  : �ڴ��׵�ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID  cskyMpuDmaAlloc (size_t  stSize)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        return  (_HeapAllocate(&_G_heapDmaPool, stSize, __func__));

    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: cskyMpuDmaAllocAlign
** ��������: �� DMA �ڴ���з����ڴ�
** �䡡��  : stSize     ��Ҫ���ڴ��С
**           stAlign    ����Ҫ��
** �䡡��  : �ڴ��׵�ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID  cskyMpuDmaAllocAlign (size_t  stSize, size_t  stAlign)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        return  (_HeapAllocateAlign(&_G_heapDmaPool, stSize, stAlign, __func__));

    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: cskyMpuDmaFree
** ��������: �ͷŴ� DMA �ڴ���з�����ڴ�
** �䡡��  : pvDmaMem   �ڴ��׵�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  cskyMpuDmaFree (PVOID  pvDmaMem)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        _HeapFree(&_G_heapDmaPool, pvDmaMem, LW_FALSE, __func__);
    }
}

#endif                                                                  /*  LW_CFG_CSKY_MPU > 0         */
#endif                                                                  /*  __SYLIXOS_CSKY_ARCH_CK803__ */
/*********************************************************************************************************
  END
*********************************************************************************************************/
