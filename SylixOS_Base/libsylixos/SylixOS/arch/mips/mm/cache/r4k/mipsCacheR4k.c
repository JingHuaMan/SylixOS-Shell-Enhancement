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
** ��   ��   ��: mipsCacheR4k.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 01 ��
**
** ��        ��: MIPS R4K ��ϵ���� CACHE ����.
**
** BUG:
2016.04.06  Add Cache Init �� CP0_ECC Register Init(Loongson-2H ֧��)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/common/mipsCpuProbe.h"
#include "arch/mips/inc/addrspace.h"
#include "arch/mips/mm/cache/mipsCacheCommon.h"
#if LW_CFG_MIPS_CACHE_L2 > 0
#include "arch/mips/mm/cache/l2/mipsL2R4k.h"
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  mipsCacheR4kDisableHw(VOID);
extern VOID  mipsCacheR4kEnableHw(VOID);

extern VOID  mipsDCacheR4kLineFlush(PCHAR       pcAddr);
extern VOID  mipsDCacheR4kLineClear(PCHAR       pcAddr);
extern VOID  mipsDCacheR4kLineInvalidate(PCHAR  pcAddr);
extern VOID  mipsDCacheR4kIndexClear(PCHAR      pcAddr);
extern VOID  mipsDCacheR4kIndexStoreTag(PCHAR   pcAddr);

extern VOID  mipsICacheR4kLineInvalidate(PCHAR  pcAddr);
extern VOID  mipsICacheR4kIndexInvalidate(PCHAR pcAddr);
extern VOID  mipsICacheR4kFill(PCHAR            pcAddr);
extern VOID  mipsICacheR4kIndexStoreTag(PCHAR   pcAddr);
/*********************************************************************************************************
  CACHE ״̬
*********************************************************************************************************/
static INT      _G_iCacheStatus = L1_CACHE_DIS;
/*********************************************************************************************************
  CACHE ѭ������ʱ���������С, ���ڸô�Сʱ��ʹ�� All ����
*********************************************************************************************************/
#define MIPS_CACHE_LOOP_OP_MAX_SIZE     (8 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
/*********************************************************************************************************
  ����о 2F ����������Ҫ�ر�С�ĵ�ʹ�� Likely ��ת��ָ����� Likely ��
  ת��ָ��Ҳ���˳������������ļ򵥵ľ�̬���Ⱥ���Ч�� ���������ִ�������
  ������������ͬ����Ч����Ϊ�ִ������ܴ�������ת��Ԥ��Ӳ���ǱȽϸ��ӣ���
  ��ͨ���� 90%���ϵ���ȷԤ���ʡ� ������˵����о 2F �ܹ���ȷԤ�� 85%-100%��
  ƽ�� 95%������ת�Ƶ�ת�Ʒ��� ����������£� ��������Ӧ��ʹ��Ԥ���ʲ�̫
  �ߵ� Likely ��ת��ָ���ʵ�ϣ����Ƿ��ִ���-mno-branch-likely ѡ��� GCC
  ��3.3 �棩ͨ���Ṥ���ø���
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: mipsBranchPredictionDisable
** ��������: ���ܷ�֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  mipsBranchPredictionDisable (VOID)
{
    if (_G_uiMipsCpuType == CPU_LOONGSON2) {
        UINT32  uiDiag = mipsCp0DiagRead();

        uiDiag |= 1 << 0;                                               /*  �� 1 ʱ���� RAS             */
        mipsCp0DiagWrite(uiDiag);

    } else if (_G_uiMipsCpuType == CPU_JZRISC) {
        UINT32  uiConfig7 = mipsCp0Config7Read();

        uiConfig7 &= ~(1 << 0);
        mipsCp0DiagWrite(uiConfig7);
    }
}
/*********************************************************************************************************
** ��������: mipsBranchPredictionEnable
** ��������: ʹ�ܷ�֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  mipsBranchPredictionEnable (VOID)
{
    if (_G_uiMipsCpuType == CPU_LOONGSON2) {
        UINT32  uiDiag = mipsCp0DiagRead();

        uiDiag &= ~(1 << 0);
        mipsCp0DiagWrite(uiDiag);

    } else if (_G_uiMipsCpuType == CPU_LOONGSON1) {
        UINT32  uiConfig6 = mipsCp0GSConfigRead();

        uiConfig6 &= ~(3 << 0);                                         /*  ��֧Ԥ�ⷽʽ:Gshare ���� BHT*/
        mipsCp0GSConfigWrite(uiConfig6);

    } else if (_G_uiMipsCpuType == CPU_JZRISC) {
        UINT32  uiConfig7 = mipsCp0Config7Read();

        uiConfig7 |= (1 << 0);
        mipsCp0DiagWrite(uiConfig7);
    }
}
/*********************************************************************************************************
** ��������: mipsBranchPredictorInvalidate
** ��������: ��Ч��֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  mipsBranchPredictorInvalidate (VOID)
{
    if (_G_uiMipsCpuType == CPU_LOONGSON2) {
        UINT32  uiDiag = mipsCp0DiagRead();

        uiDiag |= 1 << 1;                                               /*  д�� 1 ��� BTB             */
        mipsCp0DiagWrite(uiDiag);

    } else if (_G_uiMipsCpuType == CPU_JZRISC) {
        UINT32  uiConfig7 = mipsCp0Config7Read();

        uiConfig7 |= (1 << 1);                                          /*  д�� 1 ��� BTB             */
        mipsCp0DiagWrite(uiConfig7);
    }
}
/*********************************************************************************************************
** ��������: mipsDCacheR4kClear
** ��������: D-CACHE �����ݻ�д����Ч
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsDCacheR4kClear (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    REGISTER PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mipsDCacheR4kLineClear(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mipsDCacheR4kFlush
** ��������: D-CACHE �����ݻ�д
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsDCacheR4kFlush (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    REGISTER PCHAR   pcAddr;

    if (MIPS_CACHE_HAS_HIT_WB_D) {
        for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
            mipsDCacheR4kLineFlush(pcAddr);
        }
        MIPS_PIPE_FLUSH();

    } else {
        mipsDCacheR4kClear(pvStart, pvEnd, uiStep);
    }
}
/*********************************************************************************************************
** ��������: mipsDCacheR4kInvalidate
** ��������: D-CACHE ��������Ч
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsDCacheR4kInvalidate (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    REGISTER PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mipsDCacheR4kLineInvalidate(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mipsICacheR4kInvalidate
** ��������: I-CACHE ��������Ч
** �䡡��  : pvStart       ��ʼ��ַ
**           pvEnd         ������ַ
**           uiStep        ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsICacheR4kInvalidate (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    REGISTER PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mipsICacheR4kLineInvalidate(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mipsDCacheR4kClearAll
** ��������: D-CACHE �������ݻ�д����Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsDCacheR4kClearAll (VOID)
{
    REGISTER INT     iWay;
    REGISTER PCHAR   pcLineAddr;
    REGISTER PCHAR   pcBaseAddr = (PCHAR)MIPS_CACHE_INDEX_BASE;
    REGISTER PCHAR   pcEndAddr  = (PCHAR)(MIPS_CACHE_INDEX_BASE + _G_DCache.CACHE_uiWaySize);

    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        for (iWay = 0; iWay < _G_DCache.CACHE_uiWayNr; iWay++) {
            mipsDCacheR4kIndexClear(pcLineAddr + (iWay << _G_DCache.CACHE_uiWayBit));
        }
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mipsDCacheR4kFlushAll
** ��������: D-CACHE �������ݻ�д
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsDCacheR4kFlushAll (VOID)
{
    mipsDCacheR4kClearAll();
}
/*********************************************************************************************************
** ��������: mipsICacheR4kInvalidateAll
** ��������: I-CACHE ����������Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID    mipsICacheR4kInvalidateAll (VOID)
{
    REGISTER INT     iWay;
    REGISTER PCHAR   pcLineAddr;
    REGISTER PCHAR   pcBaseAddr = (PCHAR)MIPS_CACHE_INDEX_BASE;
    REGISTER PCHAR   pcEndAddr  = (PCHAR)(MIPS_CACHE_INDEX_BASE + _G_ICache.CACHE_uiSize);

    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_ICache.CACHE_uiLineSize) {
        for (iWay = 0; iWay < _G_ICache.CACHE_uiWayNr; iWay++) {
            mipsICacheR4kIndexInvalidate(pcLineAddr + (iWay << _G_ICache.CACHE_uiWayBit));
        }
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mipsCacheR4kEnable
** ��������: ʹ�� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus |= L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus |= L1_CACHE_D_EN;
        }
    }
    
    if (_G_iCacheStatus == L1_CACHE_EN) {
        mipsCacheR4kEnableHw();
        mipsBranchPredictionEnable();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kDisable
** ��������: ���� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus &= ~L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus &= ~L1_CACHE_D_EN;
        }
    }
    
    if (_G_iCacheStatus == L1_CACHE_DIS) {
        mipsCacheR4kDisableHw();
        mipsBranchPredictionDisable();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kFlush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes == 0) {
        return  (ERROR_NONE);
    }

    if (cachetype == DATA_CACHE) {
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (MIPS_CACHE_HAS_L2) {
            /*
             * Loongson2F_UM_CN_V1.5 PAGE91
             * ���� CACHE ������ CACHE ��ָ�� CACHE ���ְ�����ϵ, ���Ի�д���� CACHE ����, ��ͬ
             *
             * TODO: ������ CPU �Ƿ�Ҳ����?
             */
            return  (mipsL2R4kFlush(pvAdrs, stBytes));
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mipsDCacheR4kFlushAll();                                    /*  ȫ����д                    */

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mipsDCacheR4kFlush(pvAdrs, (PVOID)ulEnd,                    /*  ���ֻ�д                    */
                               _G_DCache.CACHE_uiLineSize);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kFlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��:
*********************************************************************************************************/
static INT  mipsCacheR4kFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (stBytes == 0) {
        return  (ERROR_NONE);
    }

    if (cachetype == DATA_CACHE) {
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (MIPS_CACHE_HAS_L2) {
            return  (mipsL2R4kFlush(pvAdrs, stBytes));
        }
#endif
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mipsDCacheR4kFlushAll();                                    /*  ȫ����д                    */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mipsDCacheR4kFlush(pvAdrs, (PVOID)ulEnd,                    /*  ���ֻ�д                    */
                               _G_DCache.CACHE_uiLineSize);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kInvalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes == 0) {
        return  (ERROR_NONE);
    }

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mipsICacheR4kInvalidateAll();                               /*  ICACHE ȫ����Ч             */

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mipsICacheR4kInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }

    } else {
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (MIPS_CACHE_HAS_L2) {
            return  (mipsL2R4kInvalidate(pvAdrs, stBytes));
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        addr_t  ulStart = (addr_t)pvAdrs;
                ulEnd   = ulStart + stBytes;
            
        if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {       /*  ��ʼ��ַ�� cache line ����  */
            ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
            mipsDCacheR4kClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
            ulStart += _G_DCache.CACHE_uiLineSize;
        }
            
        if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {         /*  ������ַ�� cache line ����  */
            ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
            mipsDCacheR4kClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
        }

        if (ulStart < ulEnd) {                                          /*  ����Ч���벿��              */
            mipsDCacheR4kInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kInvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��:
*********************************************************************************************************/
static INT  mipsCacheR4kInvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (stBytes == 0) {
        return  (ERROR_NONE);
    }

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mipsICacheR4kInvalidateAll();                               /*  ICACHE ȫ����Ч             */

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mipsICacheR4kInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }

    } else {
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (MIPS_CACHE_HAS_L2) {
            return  (mipsL2R4kInvalidate(pvAdrs, stBytes));
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        addr_t  ulStart = (addr_t)pvAdrs;
                ulEnd   = ulStart + stBytes;
                    
        if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {       /*  ��ʼ��ַ�� cache line ����  */
            ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
            mipsDCacheR4kClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
            ulStart += _G_DCache.CACHE_uiLineSize;
        }
            
        if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {         /*  ������ַ�� cache line ����  */
            ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
            mipsDCacheR4kClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
        }

        if (ulStart < ulEnd) {                                          /*  ����Ч���벿��              */
            mipsDCacheR4kInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes == 0) {
        return  (ERROR_NONE);
    }

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mipsICacheR4kInvalidateAll();                               /*  ICACHE ȫ����Ч             */

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mipsICacheR4kInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }

    } else {
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (MIPS_CACHE_HAS_L2) {
            return  (mipsL2R4kClear(pvAdrs, stBytes));
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mipsDCacheR4kClearAll();                                    /*  ȫ����д����Ч              */

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mipsDCacheR4kClear(pvAdrs, (PVOID)ulEnd,
                               _G_DCache.CACHE_uiLineSize);             /*  ���ֻ�д����Ч              */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (stBytes == 0) {
        return  (ERROR_NONE);
    }

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mipsICacheR4kInvalidateAll();                               /*  ICACHE ȫ����Ч             */
            
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mipsICacheR4kInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }

    } else {
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (MIPS_CACHE_HAS_L2) {
            return  (mipsL2R4kClear(pvAdrs, stBytes));
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mipsDCacheR4kClearAll();                                    /*  ȫ����д����Ч              */

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mipsDCacheR4kClear(pvAdrs, (PVOID)ulEnd,
                               _G_DCache.CACHE_uiLineSize);             /*  ���ֻ�д����Ч              */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kLock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kUnlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  mipsCacheR4kTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (stBytes == 0) {
        return  (ERROR_NONE);
    }

    if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
        mipsDCacheR4kFlushAll();                                        /*  DCACHE ȫ����д             */
        mipsICacheR4kInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
        
    } else {
        PVOID   pvAdrsBak = pvAdrs;

        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
        mipsDCacheR4kFlush(pvAdrs, (PVOID)ulEnd,
                           _G_DCache.CACHE_uiLineSize);                 /*  ���ֻ�д                    */

        MIPS_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
        mipsICacheR4kInvalidate(pvAdrsBak, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
INT  mipsCacheR4kDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;

    if (stBytes == 0) {
        return  (ERROR_NONE);
    }

    if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {                       /*  ȫ����д                    */
        if (bInv) {
            mipsDCacheR4kClearAll();
        } else {
            mipsDCacheR4kFlushAll();
        }

    } else {                                                            /*  ���ֻ�д                    */
        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
        if (bInv) {
            mipsDCacheR4kClear(pvAdrs, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
        } else {
            mipsDCacheR4kFlush(pvAdrs, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kInitHw
** ��������: CACHE Ӳ����ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsCacheR4kInitHw (VOID)
{
    REGISTER PCHAR   pcLineAddr;
    REGISTER PCHAR   pcBaseAddr = (PCHAR)MIPS_CACHE_INDEX_BASE;
    REGISTER PCHAR   pcEndAddr;
    REGISTER PCHAR   pcWayEndAddr;
    REGISTER CHAR    cTemp;
    REGISTER INT32   iWay;

    (VOID)cTemp;

    mipsCp0TagLoWrite(0);

    if (MIPS_CACHE_HAS_TAG_HI) {
        mipsCp0TagHiWrite(0);
    }

    if (MIPS_CACHE_HAS_ECC) {
        mipsCp0ECCWrite(0);
    }

    pcEndAddr    = (PCHAR)(MIPS_CACHE_INDEX_BASE + _G_ICache.CACHE_uiSize);
    pcWayEndAddr = (PCHAR)(MIPS_CACHE_INDEX_BASE + _G_ICache.CACHE_uiWaySize);

    /*
     * Clear tag to invalidate
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcWayEndAddr; pcLineAddr += _G_ICache.CACHE_uiLineSize) {
        for (iWay = 0; iWay < _G_ICache.CACHE_uiWayNr; iWay ++) {
            mipsICacheR4kIndexStoreTag(pcLineAddr + (iWay << _G_ICache.CACHE_uiWayBit));
        }
    }

    if (MIPS_CACHE_HAS_FILL_I) {
        /*
         * Fill so data field parity is correct
         */
        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcWayEndAddr; pcLineAddr += _G_ICache.CACHE_uiLineSize) {
            mipsICacheR4kFill(pcLineAddr);
        }
        /*
         * Invalidate again �C prudent but not strictly necessay
         */
        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcWayEndAddr; pcLineAddr += _G_ICache.CACHE_uiLineSize) {
            for (iWay = 0; iWay < _G_ICache.CACHE_uiWayNr; iWay ++) {
                mipsICacheR4kIndexStoreTag(pcLineAddr + (iWay << _G_ICache.CACHE_uiWayBit));
            }
        }
    }

    if (MIPS_CACHE_HAS_ECC) {
        mipsCp0ECCWrite(MIPS_CACHE_ECC_VALUE);
    }

    pcEndAddr    = (PCHAR)(MIPS_CACHE_INDEX_BASE + _G_DCache.CACHE_uiSize);
    pcWayEndAddr = (PCHAR)(MIPS_CACHE_INDEX_BASE + _G_DCache.CACHE_uiWaySize);

    /*
     * Clear all tags
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcWayEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        for (iWay = 0; iWay < _G_DCache.CACHE_uiWayNr; iWay ++) {
            mipsDCacheR4kIndexStoreTag(pcLineAddr + (iWay << _G_DCache.CACHE_uiWayBit));
        }
    }

    /*
     * Load from each line (in cached space)
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        cTemp = *pcLineAddr;
    }

    /*
     * Clear all tags again
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcWayEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        for (iWay = 0; iWay < _G_DCache.CACHE_uiWayNr; iWay ++) {
            mipsDCacheR4kIndexStoreTag(pcLineAddr + (iWay << _G_DCache.CACHE_uiWayBit));
        }
    }

#if LW_CFG_MIPS_CACHE_L2 > 0
    mipsL2R4kInitHw();
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
}
/*********************************************************************************************************
** ��������: mipsCacheR4kInit
** ��������: ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsCacheR4kInit (LW_CACHE_OP  *pcacheop,
                        CACHE_MODE    uiInstruction,
                        CACHE_MODE    uiData,
                        CPCHAR        pcMachineName)
{
    mipsCacheProbe(pcMachineName);                                      /*  CACHE ̽��                  */
    mipsCacheInfoShow();                                                /*  ��ӡ CACHE ��Ϣ             */
    mipsCacheR4kDisableHw();                                            /*  �ر� CACHE                  */
    mipsBranchPredictorInvalidate();                                    /*  ��Ч��֧Ԥ��                */
    mipsCacheR4kInitHw();                                               /*  ��ʼ�� CACHE                */

#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIPT;

    pcacheop->CACHEOP_iICacheLine = _G_ICache.CACHE_uiLineSize;
    pcacheop->CACHEOP_iDCacheLine = _G_DCache.CACHE_uiLineSize;

    pcacheop->CACHEOP_iICacheWaySize = _G_ICache.CACHE_uiWaySize;
    pcacheop->CACHEOP_iDCacheWaySize = _G_DCache.CACHE_uiWaySize;

    pcacheop->CACHEOP_pfuncEnable  = mipsCacheR4kEnable;
    pcacheop->CACHEOP_pfuncDisable = mipsCacheR4kDisable;
    
    pcacheop->CACHEOP_pfuncLock   = mipsCacheR4kLock;                   /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock = mipsCacheR4kUnlock;

    pcacheop->CACHEOP_pfuncFlush          = mipsCacheR4kFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = mipsCacheR4kFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = mipsCacheR4kInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = mipsCacheR4kInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = mipsCacheR4kClear;
    pcacheop->CACHEOP_pfuncClearPage      = mipsCacheR4kClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = mipsCacheR4kTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = mipsCacheR4kDataUpdate;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: mipsCacheR4kReset
** ��������: ��λ CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����� lockdown �������� unlock & invalidate ��������
*********************************************************************************************************/
VOID  mipsCacheR4kReset (CPCHAR  pcMachineName)
{
    mipsCacheProbe(pcMachineName);                                      /*  CACHE ̽��                  */
    mipsCacheR4kDisableHw();                                            /*  �ر� CACHE                  */
    mipsBranchPredictorInvalidate();                                    /*  ��Ч��֧Ԥ��                */
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
