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
** ��   ��   ��: armCacheV7.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARMv7 ��ϵ���� CACHE ����.
**
** BUG:
2014.11.12  L2 CACHE ֻ�� CPU 0 ���ܲ���.
2015.08.21  ���� Invalidate ����������ַ�������.
2015.11.25  Text Update ����Ҫ���֧Ԥ��.
            Text Update ʹ�� armDCacheV7FlushPoU() ��д DCACHE.
2016.04.29  ���� data update ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7A, R ��ϵ����
*********************************************************************************************************/
#if !defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../armCacheCommon.h"
#include "../../mmu/armMmuCommon.h"
#include "../../../common/cp15/armCp15.h"
#if defined(__SYLIXOS_ARM_ARCH_R__)
#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
#include "../../mpu/v7r/armMpuV7R.h"
#endif
#endif
/*********************************************************************************************************
  L2 CACHE ֧��
*********************************************************************************************************/
#if LW_CFG_ARM_CACHE_L2 > 0
#include "../l2/armL2.h"
/*********************************************************************************************************
  L1 CACHE ״̬
*********************************************************************************************************/
static INT      iCacheStatus = 0;
#define L1_CACHE_I_EN   0x01
#define L1_CACHE_D_EN   0x02
#define L1_CACHE_EN     (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS    0x00
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern VOID     armDCacheV7Disable(VOID);
extern VOID     armDCacheV7FlushPoU(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     armDCacheV7FlushAll(VOID);
extern VOID     armDCacheV7FlushAllPoU(VOID);
extern VOID     armDCacheV7ClearAll(VOID);
extern UINT32   armCacheV7CCSIDR(VOID);
/*********************************************************************************************************
  ѡ�� CACHE ����
*********************************************************************************************************/
#define ARMV7_CSSELR_IND_DATA_UNIFIED   0
#define ARMV7_CSSELR_IND_INSTRUCTION    1
extern VOID     armCacheV7SetCSSELR(UINT32  uiValue);
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
static UINT32                           uiArmV7ICacheLineSize;
static UINT32                           uiArmV7DCacheLineSize;
#define ARMv7_CACHE_LOOP_OP_MAX_SIZE    (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
** ��������: armCacheV7Enable
** ��������: ʹ�� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armCacheV7Enable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheEnable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
        armBranchPredictionEnable();

    } else {
        armDCacheEnable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }
    
#if LW_CFG_ARM_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) && 
        (iCacheStatus == L1_CACHE_EN)) {
        armL2Enable();
    }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7Disable
** ��������: ���� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armCacheV7Disable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheDisable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
        armBranchPredictionDisable();
        
    } else {
        armDCacheV7Disable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }
    
#if LW_CFG_ARM_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) && 
        (iCacheStatus == L1_CACHE_DIS)) {
        armL2Disable();
    }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7Flush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����дָ��.
*********************************************************************************************************/
static INT	armCacheV7Flush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);/*  ���ֻ�д                    */
        }
        
#if LW_CFG_ARM_CACHE_L2 > 0
        armL2FlushAll();
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7FlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV7FlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);/*  ���ֻ�д                    */
        }
        
#if LW_CFG_ARM_CACHE_L2 > 0
        armL2Flush(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7Invalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ (pvAdrs ������������ַ)
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺���������� DCACHE pvAdrs �����ַ�������ַ������ͬ.
*********************************************************************************************************/
static INT	armCacheV7Invalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV7ICacheLineSize);
        }

    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & ((addr_t)uiArmV7DCacheLineSize - 1)) {        /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)uiArmV7DCacheLineSize - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV7DCacheLineSize);
                ulStart += uiArmV7DCacheLineSize;
            }
            
            if (ulEnd & ((addr_t)uiArmV7DCacheLineSize - 1)) {          /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)uiArmV7DCacheLineSize - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV7DCacheLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV7DCacheLineSize);
            }
            
#if LW_CFG_ARM_CACHE_L2 > 0
            armL2Invalidate(pvAdrs, stBytes);                           /*  �����������ַ������ͬ      */
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7InvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV7InvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV7ICacheLineSize);
        }

    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
                    
            if (ulStart & ((addr_t)uiArmV7DCacheLineSize - 1)) {        /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)uiArmV7DCacheLineSize - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV7DCacheLineSize);
                ulStart += uiArmV7DCacheLineSize;
            }
            
            if (ulEnd & ((addr_t)uiArmV7DCacheLineSize - 1)) {          /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)uiArmV7DCacheLineSize - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV7DCacheLineSize);
            }

            if (ulStart < ulEnd) {                                      /*  ����Ч���벿��              */
                armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV7DCacheLineSize);
            }
            
#if LW_CFG_ARM_CACHE_L2 > 0
            armL2Invalidate(pvPdrs, stBytes);                           /*  �����������ַ������ͬ      */
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7Clear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����д����Чָ��.
*********************************************************************************************************/
static INT	armCacheV7Clear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV7ICacheLineSize);
        }

    } else {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7ClearAll();                                      /*  ȫ����д����Ч              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);/*  ���ֻ�д����Ч              */
        }
        
#if LW_CFG_ARM_CACHE_L2 > 0
        armL2ClearAll();
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7ClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV7ClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV7ICacheLineSize);
        }

    } else {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7ClearAll();                                      /*  ȫ����д����Ч              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);/*  ���ֻ�д����Ч              */
        }
        
#if LW_CFG_ARM_CACHE_L2 > 0
        armL2Clear(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7Lock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV7Lock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV7Unlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV7Unlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV7TextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	armCacheV7TextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
        armDCacheV7FlushAllPoU();                                       /*  DCACHE ȫ����д             */
        armICacheInvalidateAll();                                       /*  ICACHE ȫ����Ч             */
        
    } else {
        PVOID   pvAdrsBak = pvAdrs;

        ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
        armDCacheV7FlushPoU(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);

        ARM_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, uiArmV7ICacheLineSize);
        armICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, uiArmV7ICacheLineSize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7DataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	armCacheV7DataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;
    
    if (bInv == LW_FALSE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);/*  ���ֻ�д                    */
        }
    
    } else {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7ClearAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archCacheV7Init
** ��������: ��ʼ�� CACHE 
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armCacheV7Init (LW_CACHE_OP *pcacheop, 
                      CACHE_MODE   uiInstruction, 
                      CACHE_MODE   uiData, 
                      CPCHAR       pcMachineName)
{
    UINT32  uiICCSIDR;
    UINT32  uiDCCSIDR;

#define ARMv7_CCSIDR_LINESIZE_MASK      0x7
#define ARMv7_CCSIDR_LINESIZE(x)        ((x) & ARMv7_CCSIDR_LINESIZE_MASK)
#define ARMv7_CACHE_LINESIZE(x)         (16 << ARMv7_CCSIDR_LINESIZE(x))

#define ARMv7_CCSIDR_NUMSET_MASK        0xfffe000
#define ARMv7_CCSIDR_NUMSET(x)          ((x) & ARMv7_CCSIDR_NUMSET_MASK)
#define ARMv7_CACHE_NUMSET(x)           ((ARMv7_CCSIDR_NUMSET(x) >> 13) + 1)

#define ARMv7_CCSIDR_WAYNUM_MSK         0x1ff8
#define ARMv7_CCSIDR_WAYNUM(x)          ((x) & ARMv7_CCSIDR_WAYNUM_MSK)
#define ARMv7_CACHE_WAYNUM(x)           ((ARMv7_CCSIDR_NUMSET(x) >> 3) + 1)

#if LW_CFG_ARM_CACHE_L2 > 0
    armL2Init(uiInstruction, uiData, pcMachineName);
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */

#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    armCacheV7SetCSSELR(ARMV7_CSSELR_IND_INSTRUCTION);
    uiICCSIDR = armCacheV7CCSIDR();

    armCacheV7SetCSSELR(ARMV7_CSSELR_IND_DATA_UNIFIED);
    uiDCCSIDR = armCacheV7CCSIDR();

    pcacheop->CACHEOP_iICacheLine = ARMv7_CACHE_LINESIZE(uiICCSIDR);
    pcacheop->CACHEOP_iDCacheLine = ARMv7_CACHE_LINESIZE(uiDCCSIDR);
    
    uiArmV7ICacheLineSize = (UINT32)pcacheop->CACHEOP_iICacheLine;
    uiArmV7DCacheLineSize = (UINT32)pcacheop->CACHEOP_iDCacheLine;
    
    pcacheop->CACHEOP_iICacheWaySize = uiArmV7ICacheLineSize
                                     * ARMv7_CACHE_NUMSET(uiICCSIDR);   /*  ICACHE WaySize              */
    pcacheop->CACHEOP_iDCacheWaySize = uiArmV7DCacheLineSize
                                     * ARMv7_CACHE_NUMSET(uiDCCSIDR);   /*  DCACHE WaySize              */

    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv7 I-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iICacheLine, pcacheop->CACHEOP_iICacheWaySize);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv7 D-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iDCacheLine, pcacheop->CACHEOP_iDCacheWaySize);

    if ((lib_strcmp(pcMachineName, ARM_MACHINE_A5) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A7) == 0)) {
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
        
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A9) == 0) {
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
        armAuxControlFeatureEnable(AUX_CTRL_A9_L1_PREFETCH);            /*  Cortex-A9 ʹ�� L1 Ԥȡ      */
    
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A8) == 0) {
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
        armAuxControlFeatureEnable(AUX_CTRL_A8_FORCE_ETM_CLK |
                                   AUX_CTRL_A8_FORCE_MAIN_CLK |
                                   AUX_CTRL_A8_L1NEON |
                                   AUX_CTRL_A8_FORCE_NEON_CLK |
                                   AUX_CTRL_A8_FORCE_NEON_SIGNAL);
    
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A15) == 0) {
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
        armAuxControlFeatureEnable(AUX_CTRL_A15_FORCE_MAIN_CLK |
                                   AUX_CTRL_A15_FORCE_NEON_CLK);
    
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A17) == 0) {
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
        armAuxControlFeatureEnable(AUX_CTRL_A17_L1_PREFETCH |
                                   AUX_CTRL_A17_L2_PREFETCH);

    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_R4) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_R5) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_R7) == 0)) {
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
    }
    
    pcacheop->CACHEOP_pfuncEnable  = armCacheV7Enable;
    pcacheop->CACHEOP_pfuncDisable = armCacheV7Disable;
    
    pcacheop->CACHEOP_pfuncLock    = armCacheV7Lock;                    /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock  = armCacheV7Unlock;
    
    pcacheop->CACHEOP_pfuncFlush          = armCacheV7Flush;
    pcacheop->CACHEOP_pfuncFlushPage      = armCacheV7FlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = armCacheV7Invalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = armCacheV7InvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = armCacheV7Clear;
    pcacheop->CACHEOP_pfuncClearPage      = armCacheV7ClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = armCacheV7TextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = armCacheV7DataUpdate;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;

#elif (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
    pcacheop->CACHEOP_pfuncDmaMalloc      = armMpuV7RDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = armMpuV7RDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = armMpuV7RDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: archCacheV7Reset
** ��������: ��λ CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����� lockdown �������� unlock & invalidate ��������
*********************************************************************************************************/
VOID  armCacheV7Reset (CPCHAR  pcMachineName)
{
    armICacheInvalidateAll();
    armDCacheV7Disable();
    armICacheDisable();
    armBranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
