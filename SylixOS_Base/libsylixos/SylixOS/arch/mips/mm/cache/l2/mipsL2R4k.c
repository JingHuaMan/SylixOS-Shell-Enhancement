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
** ��   ��   ��: mipsL2R4k.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 07 �� 18 ��
**
** ��        ��: MIPS R4K ��ϵ���� L2-CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_MIPS_CACHE_L2 > 0
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/common/mipsCpuProbe.h"
#include "arch/mips/inc/addrspace.h"
#include "arch/mips/mm/cache/mipsCacheCommon.h"
#include "arch/mips/mm/cache/l2/mipsL2R4k.h"
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  mipsL2R4kLineFlush(PCHAR       pcAddr);
extern VOID  mipsL2R4kLineClear(PCHAR       pcAddr);
extern VOID  mipsL2R4kLineInvalidate(PCHAR  pcAddr);
extern VOID  mipsL2R4kIndexClear(PCHAR      pcAddr);
extern VOID  mipsL2R4kIndexStoreTag(PCHAR   pcAddr);
/*********************************************************************************************************
  CACHE ѭ������ʱ���������С, ���ڸô�Сʱ��ʹ�� All ����
*********************************************************************************************************/
#define MIPS_L2_CACHE_LOOP_OP_MAX_SIZE  (64 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
** ��������: __mipsL2R4kClear
** ��������: SCACHE ��������Ч
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __mipsL2R4kClear (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{
    REGISTER PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mipsL2R4kLineClear(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: __mipsL2R4kFlush
** ��������: SCACHE �����ݻ�д
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __mipsL2R4kFlush (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{
    REGISTER PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mipsL2R4kLineFlush(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mipsL2R4kClearAll
** ��������: SCACHE �������ݻ�д����Ч
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kClearAll (VOID)
{
    if (MIPS_CACHE_HAS_L2) {
        REGISTER INT     iWay;
        REGISTER PCHAR   pcLineAddr;
        REGISTER PCHAR   pcBaseAddr = (PCHAR)MIPS_CACHE_INDEX_BASE;
        REGISTER PCHAR   pcEndAddr  = (PCHAR)(MIPS_CACHE_INDEX_BASE + _G_SCache.CACHE_uiWaySize);

        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_SCache.CACHE_uiLineSize) {
            for (iWay = 0; iWay < _G_SCache.CACHE_uiWayNr; iWay++) {
                mipsL2R4kIndexClear(pcLineAddr + (iWay << _G_SCache.CACHE_uiWayBit));
            }
        }
        MIPS_PIPE_FLUSH();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsL2R4kFlushAll
** ��������: SCACHE �������ݻ�д
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kFlushAll (VOID)
{
    return  (mipsL2R4kClearAll());
}
/*********************************************************************************************************
** ��������: mipsL2R4kClear
** ��������: SCACHE ��������Ч
** �䡡��  : pvStart        ��ʼ�����ַ
**           stBytes        �ֽ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kClear (PVOID  pvAdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= MIPS_L2_CACHE_LOOP_OP_MAX_SIZE) {
        mipsL2R4kClearAll();                                            /*  ȫ����д����Ч              */

    } else {
        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_SCache.CACHE_uiLineSize);
        __mipsL2R4kClear(pvAdrs, (PVOID)ulEnd,
                         _G_SCache.CACHE_uiLineSize);                   /*  ���ֻ�д����Ч              */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsL2R4kFlush
** ��������: SCACHE �����ݻ�д
** �䡡��  : pvStart        ��ʼ�����ַ
**           stBytes        �ֽ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kFlush (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= MIPS_L2_CACHE_LOOP_OP_MAX_SIZE) {
        mipsL2R4kFlushAll();                                            /*  ȫ����д                    */

    } else {
        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_SCache.CACHE_uiLineSize);
        if (MIPS_CACHE_HAS_HIT_WB_S) {
            __mipsL2R4kFlush(pvAdrs, (PVOID)ulEnd,                      /*  ���ֻ�д                    */
                             _G_SCache.CACHE_uiLineSize);
        } else {
            __mipsL2R4kClear(pvAdrs, (PVOID)ulEnd,
                             _G_SCache.CACHE_uiLineSize);               /*  ���ֻ�д����Ч              */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsL2R4kInvalidate
** ��������: SCACHE ��������Ч
** �䡡��  : pvStart        ��ʼ�����ַ
**           stBytes        �ֽ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kInvalidate (PVOID  pvStart, size_t stBytes)
{
    REGISTER PCHAR   pcAddr;
             addr_t  ulStart = (addr_t)pvStart;
             addr_t  ulEnd   = ulStart + stBytes;

    if (ulStart & ((addr_t)_G_SCache.CACHE_uiLineSize - 1)) {           /*  ��ʼ��ַ�� cache line ����  */
        ulStart &= ~((addr_t)_G_SCache.CACHE_uiLineSize - 1);
        __mipsL2R4kClear((PVOID)ulStart, (PVOID)ulStart, _G_SCache.CACHE_uiLineSize);
        ulStart += _G_SCache.CACHE_uiLineSize;
    }

    if (ulEnd & ((addr_t)_G_SCache.CACHE_uiLineSize - 1)) {             /*  ������ַ�� cache line ����  */
        ulEnd &= ~((addr_t)_G_SCache.CACHE_uiLineSize - 1);
        __mipsL2R4kClear((PVOID)ulEnd, (PVOID)ulEnd, _G_SCache.CACHE_uiLineSize);
    }

    for (pcAddr = (PCHAR)ulStart; pcAddr < (PCHAR)ulEnd; pcAddr += _G_SCache.CACHE_uiLineSize) {
        mipsL2R4kLineInvalidate(pcAddr);                                /*  ����Ч���벿��              */
    }
    MIPS_PIPE_FLUSH();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsL2R4kInitHw
** ��������: SCACHE Ӳ����ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kInitHw (VOID)
{
    REGISTER PCHAR   pcLineAddr;
    REGISTER PCHAR   pcBaseAddr = (PCHAR)MIPS_CACHE_INDEX_BASE;
    REGISTER PCHAR   pcEndAddr;
    REGISTER INT32   iWay;
    REGISTER CHAR    cTemp;

    (VOID)cTemp;

    /*
     * Init L2
     */
    if (MIPS_CACHE_HAS_L2) {
        pcEndAddr = (PCHAR)(MIPS_CACHE_INDEX_BASE + _G_SCache.CACHE_uiSize);

        /*
         * Clear all tags
         */
        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_SCache.CACHE_uiLineSize) {
            for (iWay = 0; iWay < _G_SCache.CACHE_uiWayNr; iWay ++) {
                mipsL2R4kIndexStoreTag(pcLineAddr + (iWay << _G_SCache.CACHE_uiWayBit));
            }
        }

        /*
         * Load from each line (in cached space)
         */
        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_SCache.CACHE_uiLineSize) {
            cTemp = *pcLineAddr;
        }

        /*
         * Clear all tags again
         */
        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_SCache.CACHE_uiLineSize) {
            for (iWay = 0; iWay < _G_SCache.CACHE_uiWayNr; iWay ++) {
                mipsL2R4kIndexStoreTag(pcLineAddr + (iWay << _G_SCache.CACHE_uiWayBit));
            }
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
