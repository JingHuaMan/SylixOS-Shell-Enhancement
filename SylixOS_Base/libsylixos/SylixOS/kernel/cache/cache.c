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
** ��   ��   ��: cache.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 09 �� 27 ��
**
** ��        ��: ƽ̨�޹صĸ��ٻ����������� (��� L1-cache).

** BUG
2007.12.22 ����ע��.
2008.05.02 �޸��� API_CacheFuncsSet() �ڲ����������ش���.
2008.06.06 �� D-CACHE Ϊ��д����ʱ, ʹ CACHE ������֮ǰ, �������Ƚ��л�д.
2008.12.05 ������� DMA �ڴ����.
2009.03.09 �޸��ڴ���뿪�ٵĴ���.
2009.06.18 ȡ�� CACHE �Ե�ַӳ��Ĺ���, ��ȫ���� VMM ������.
2009.07.11 API_CacheLibInit() ֻ�ܳ�ʼ��һ��.
2011.05.20 API_CacheInvalidate() �����������ȷ����д�� cache �Ȼ�д����Ч.
2011.12.10 �� cache �ؼ��Բ���ʱ, ��Ҫ���ж�! һ�����ܷ�����ǰ��������ռ!
2012.01.18 ����Ҫ�� API_CacheTextUpdate() ���е�����Ч�Ե��ж�.
2012.01.18 cache �ؼ��Բ���ʱ��ֹ�����ռ. (�����������)
2013.07.20 TextUpdate ֧�� SMP ϵͳ.
2013.12.19 API_CacheLibPrimaryInit() ���� CACHE �����ִ�, arch ����ͨ�����ִ��ṩ��Ӧ�� CACHE ��������.
2014.01.03 �� cache api ���.
2014.07.27 ��������ҳ�����.
2018.07.18 ���� API_CacheBarrier() ��Զ������ͬʱ���� CACHE ��ͬ�� CACHE ������װ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
   
   ������ֻʹ�� CACHE ��ϵͳ�ṹͼ
   
   +---------------+     +-----------------+     +--------------+
   |               |     |                 |     |              |
   |  INSTRUCTION  |---->|    PROCESSOR    |<--->|  DATA CACHE  | (3)
   |     CACHE     |     |                 |     |  (copyback)  |
   |               |     |                 |     |              |
   +---------------+     +-----------------+     +--------------+
           ^                     (2)                     ^
           |                                             |
           |                                     +--------------+
           |                                     |              |
           |                                     | WRITE BUFFER |
           |                                     |              |
           |                                     +--------------+
           |             +-----------------+             |
           |             |                 |         (1) |
           +-------------|       RAM       |<------------+
                         |                 |
                         +-----------------+
                             ^         ^
                             |         |
           +-------------+   |         |   +-------------+
           |             |   |         |   |             |
           | DMA Devices |<--+         +-->| VMEbus, etc.|
           |             |                 |             |
           +-------------+                 +-------------+

*********************************************************************************************************/
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
/*********************************************************************************************************
  ȫ�ֱ������� 
*********************************************************************************************************/
static CACHE_MODE   _G_uiICacheMode = CACHE_DISABLED;
static CACHE_MODE   _G_uiDCacheMode = CACHE_DISABLED;
/*********************************************************************************************************
  cache ��������������
*********************************************************************************************************/
#define __CACHE_OP_ENTER(iregInterLevel)    {   iregInterLevel = KN_INT_DISABLE(); KN_SMP_MB(); }
#define __CACHE_OP_EXIT(iregInterLevel)     {   KN_INT_ENABLE(iregInterLevel);  }
/*********************************************************************************************************
  ȫ�ֽṹ�������� 
*********************************************************************************************************/
LW_CACHE_OP     _G_cacheopLib = {                                       /*  the cache primitives        */
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    0,
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

    CACHE_LOCATION_VIVT,
    CACHE_LOCATION_VIVT,
    32,
    32,
    LW_CFG_VMM_PAGE_SIZE,
    LW_CFG_VMM_PAGE_SIZE,                                               /*  def: No Cache Alias problem */
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    LW_NULL,                                                            /*  cacheEnable()               */
    LW_NULL,                                                            /*  cacheDisable()              */
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

    LW_NULL,                                                            /*  cacheLock()                 */
    LW_NULL,                                                            /*  cacheUnlock()               */
    LW_NULL,                                                            /*  cacheFlush()                */
    LW_NULL,                                                            /*  cacheFlushPage()            */

#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    LW_NULL,                                                            /*  cacheInvalidate()           */
    LW_NULL,                                                            /*  cacheInvalidatePage()       */
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

    LW_NULL,                                                            /*  cacheClear()                */
    LW_NULL,                                                            /*  cacheClearPage()            */
    LW_NULL,                                                            /*  cacheTextUpdate()           */
    LW_NULL,                                                            /*  cacheDataUpdate()           */
    LW_NULL,                                                            /*  cacheDmaMalloc()            */
    LW_NULL,                                                            /*  cacheDmaMallocAlign()       */
    LW_NULL,                                                            /*  cacheDmaFree()              */
};
/*********************************************************************************************************
  TextUpdate Parameter
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

typedef struct {
    PVOID       TUA_pvAddr;
    size_t      TUA_stSize;
} LW_CACHE_TU_ARG;

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  ��ϵ�ṹ��������
*********************************************************************************************************/
extern VOID   __ARCH_CACHE_INIT(CACHE_MODE  uiInstruction, CACHE_MODE  uiData, CPCHAR  pcMachineName);
/*********************************************************************************************************
** ��������: API_CacheGetLibBlock
** ��������: ���ϵͳ�� LW_CACHE_OP �ṹ, (���� BSP �����ʼ�� CACHE ϵͳ)
** �䡡��  : NONE
** �䡡��  : &_G_cacheopLib
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LW_CACHE_OP *API_CacheGetLibBlock (VOID)
{
    return  (&_G_cacheopLib);
}
/*********************************************************************************************************
** ��������: API_CacheGetOption
** ��������: ���ϵͳ�� LW_CACHE_OP �ṹ OPTION �ֶ�.
** �䡡��  : NONE
** �䡡��  : &_G_cacheopLib
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_CacheGetOption (VOID)
{
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    return  (0);
#else
    return  (_G_cacheopLib.CACHEOP_ulOption);
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
}
/*********************************************************************************************************
** ��������: API_CacheGetMode
** ��������: ��� CACHE ģʽ.
** �䡡��  : cachetype     cache ����
** �䡡��  : CACHE ģʽ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
CACHE_MODE  API_CacheGetMode (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        return  (_G_uiICacheMode);
    } else {
        return  (_G_uiDCacheMode);
    }
}
/*********************************************************************************************************
** ��������: API_CacheLibPrimaryInit
** ��������: ��ʼ�� CACHE ����, CPU �������.
** �䡡��  : uiInstruction                 ��ʼָ�� CACHE ģʽ
**           uiData                        ��ʼ���� CACHE ģʽ
**           pcMachineName                 ��ǰ���л�������.
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG   API_CacheLibPrimaryInit (CACHE_MODE  uiInstruction, CACHE_MODE  uiData, CPCHAR  pcMachineName)
{
    static BOOL bIsInit = LW_FALSE;

    if (bIsInit) {
        return  (ERROR_NONE);
    }

    _G_uiICacheMode = uiInstruction;
    _G_uiDCacheMode = uiData;                                           /*  ���� D-CACHE ģʽ           */
    
    __ARCH_CACHE_INIT(uiInstruction, uiData, pcMachineName);            /*  ��ʼ�� CACHE                */
    API_CacheFuncsSet();
    
    bIsInit = LW_TRUE;
    
    _BugFormat((_G_cacheopLib.CACHEOP_iICacheLine & (_G_cacheopLib.CACHEOP_iICacheLine - 1)),
               LW_TRUE, "I-CACHE line size: %s error!\r\n", _G_cacheopLib.CACHEOP_iICacheLine);
    _BugFormat((_G_cacheopLib.CACHEOP_iDCacheLine & (_G_cacheopLib.CACHEOP_iDCacheLine - 1)),
               LW_TRUE, "D-CACHE line size: %s error!\r\n", _G_cacheopLib.CACHEOP_iDCacheLine);

    _DebugHandle(__LOGMESSAGE_LEVEL, "CACHE initilaized.\r\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_CacheLibSInit
** ��������: ��ʼ���Ӻ� CACHE ����, CPU ������ء�
** �䡡��  : pcMachineName                 ��ǰ���л�������.
** �䡡��  : OK 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

LW_API  
ULONG   API_CacheLibSecondaryInit (CPCHAR  pcMachineName)
{
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: API_CacheLocation
** ��������: ��ȡ CACHE ��λ��
** �䡡��  : cachetype     cache ����
** �䡡��  : BSP �ṩ�� CACHE λ��������Ϣ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_CacheLocation (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        return  (_G_cacheopLib.CACHEOP_iILoc);
    
    } else {
        return  (_G_cacheopLib.CACHEOP_iDLoc);
    }
}
/*********************************************************************************************************
** ��������: API_CacheLine
** ��������: ��ȡ CACHE line ��С
** �䡡��  : cachetype     cache ����
** �䡡��  : CACHE �д�С
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_CacheLine (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        return  (_G_cacheopLib.CACHEOP_iICacheLine);
    
    } else {
        return  (_G_cacheopLib.CACHEOP_iDCacheLine);
    }
}
/*********************************************************************************************************
** ��������: API_CacheWaySize
** ��������: ��ȡһ· DCACHE ��С
** �䡡��  : cachetype     cache ����
** �䡡��  : һ· CACHE ��С
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
size_t  API_CacheWaySize (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        return  (_G_cacheopLib.CACHEOP_iICacheWaySize);
    
    } else {
        return  (_G_cacheopLib.CACHEOP_iDCacheWaySize);
    }
}
/*********************************************************************************************************
** ��������: API_CacheAliasProb
** ��������: VIPT CACHE �Ƿ���� alias ����. (���� DCACHE ��ӳ��ͬһ�����ַ)
** �䡡��  : NONE
** �䡡��  : LW_TRUE  �� alias ����
**           LW_FALSE �� alias ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
BOOL  API_CacheAliasProb (VOID)
{
    if (_G_cacheopLib.CACHEOP_iDLoc == CACHE_LOCATION_VIVT) {
        return  (LW_TRUE);
    }
    
    if ((_G_cacheopLib.CACHEOP_iDLoc == CACHE_LOCATION_VIPT) &&
        (_G_cacheopLib.CACHEOP_iDCacheWaySize > LW_CFG_VMM_PAGE_SIZE)) {
        return  (LW_TRUE);
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: API_CacheEnable
** ��������: ʹ��ָ�����͵� CACHE
** �䡡��  : cachetype                     CACHE ����
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheEnable (LW_CACHE_TYPE  cachetype)
{
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    return  (ERROR_NONE);
    
#else
    INTREG  iregInterLevel;
    INT     iError;

    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncEnable == LW_NULL) ? PX_ERROR : 
              (_G_cacheopLib.CACHEOP_pfuncEnable)(cachetype));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "%sCACHE enable.\r\n",
                 (cachetype == INSTRUCTION_CACHE) ? "I-" : "D-");
                 
#if (defined(LW_CFG_CPU_ARCH_ARM) || defined(LW_CFG_CPU_ARCH_ARM64)) && (LW_CFG_SMP_EN > 0)
    if (cachetype == DATA_CACHE) {
        __ARCH_SPIN_WORK();                                             /*  spin lock ʹ��              */
    }
#endif

    return  (iError);
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
}
/*********************************************************************************************************
** ��������: API_CacheDisable
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype                     CACHE ����
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : dcache �ڹر�ǰ, ��������������������.

                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheDisable (LW_CACHE_TYPE  cachetype)
{
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    return  (ERROR_NONE);
    
#else
    INTREG  iregInterLevel;
    INT     iError;

    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncDisable == LW_NULL) ? PX_ERROR : 
              (_G_cacheopLib.CACHEOP_pfuncDisable)(cachetype));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "%sCACHE disable.\r\n",
                 (cachetype == INSTRUCTION_CACHE) ? "I-" : "D-");
    
    return  (iError);
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
}
/*********************************************************************************************************
** ��������: API_CacheBarrier
** ��������: ��� CACHE ͬ������
** �䡡��  : pfuncHook             ͬ��������
**           pvArg                 ͬ������������
**           stSize                CPU ���ϴ�С
**           pcpuset               ��Ҫͬ���� CPU ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˲���һ�����ڶ��ͬ�� CACHE. �˺�����֧��Ƕ�׵��� (hook �в����ٵ��ô˺���)

                                           API ����
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

LW_API  
INT    API_CacheBarrier (VOIDFUNCPTR             pfuncHook, 
                         PVOID                   pvArg, 
                         size_t                  stSize, 
                         const PLW_CLASS_CPUSET  pcpuset)
{
    BOOL            bLock;
    ULONG           i, ulCPUId, ulNumChk;
    PLW_CLASS_CPU   pcpu, pcpuCur;
    
    if (!pcpuset || !pfuncHook) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    ulNumChk = ((ULONG)stSize << 3);
    ulNumChk = (ulNumChk > LW_NCPUS) ? LW_NCPUS : ulNumChk;

    bLock = __SMP_CPU_LOCK();

    ulCPUId = LW_CPU_GET_CUR_ID();
    pcpuCur = LW_CPU_GET(ulCPUId);
    
    pcpuCur->CPU_bCacheBarStart = LW_TRUE;                              /*  ���õ�ǰ����ʼͬ����        */
    pcpuCur->CPU_bCacheBarEnd   = LW_FALSE;                             /*  δ��ʼ�ڶ��׶�ͬ��          */
    KN_SMP_MB();
    
    for (i = 0; i < ulNumChk; i++) {
        if (LW_CPU_ISSET(i, pcpuset)) {
            pcpu = LW_CPU_GET(i);
            while (!pcpu->CPU_bCacheBarStart);                          /*  �����ȴ����� CPU            */
        }
    }
    
    pfuncHook(pvArg);                                                   /*  ִ�лص�����                */
    
    pcpuCur->CPU_bCacheBarEnd = LW_TRUE;                                /*  ���õ�ǰ�˽���ͬ����        */
    KN_SMP_MB();
    
    for (i = 0; i < ulNumChk; i++) {
        if (LW_CPU_ISSET(i, pcpuset)) {
            pcpu = LW_CPU_GET(i);
            while (!pcpu->CPU_bCacheBarEnd);                            /*  �����ȴ����� CPU            */
        }
    }

    __SMP_CPU_UNLOCK(bLock);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: API_CacheLock
** ��������: ָ�����͵� CACHE ����һ����ַ
** �䡡��  : cachetype                     CACHE ����
**           pvAdrs                        ��Ҫ�����������ַ
**           stBytes                       ����
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheLock (LW_CACHE_TYPE   cachetype, 
                      PVOID           pvAdrs, 
                      size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncLock == LW_NULL) ? PX_ERROR : 
              (_G_cacheopLib.CACHEOP_pfuncLock)(cachetype, pvAdrs, stBytes));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_CacheUnlock
** ��������: ָ�����͵� CACHE ����һ����ַ
** �䡡��  : cachetype                     CACHE ����
**           pvAdrs                        ��Ҫ�����������ַ
**           stBytes                       ����
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheUnlock (LW_CACHE_TYPE   cachetype, 
                        PVOID           pvAdrs, 
                        size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncUnlock == LW_NULL) ? PX_ERROR : 
              (_G_cacheopLib.CACHEOP_pfuncUnlock)(cachetype, pvAdrs, stBytes));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_CacheFlush
** ��������: ָ�����͵� CACHE �����л���ָ�������������(��д���ڴ�)
** �䡡��  : cachetype                     CACHE ����
**           pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheFlush (LW_CACHE_TYPE   cachetype, 
                       PVOID           pvAdrs, 
                       size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncFlush == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncFlush)(cachetype, pvAdrs, stBytes));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_CacheFlushPage
** ��������: ָ�����͵� CACHE ʹָ����ҳ���д
** �䡡��  : cachetype                     CACHE ����
**           pvAdrs                        �����ַ
**           pvPdrs                        �����ַ
**           stBytes                       ���� (ҳ�泤��������)
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheFlushPage (LW_CACHE_TYPE   cachetype, 
                           PVOID           pvAdrs, 
                           PVOID           pvPdrs,
                           size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
#if LW_CFG_VMM_EN > 0
    iError = ((_G_cacheopLib.CACHEOP_pfuncFlushPage == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncFlushPage)(cachetype, pvAdrs, pvPdrs, stBytes));
#else
    iError = ((_G_cacheopLib.CACHEOP_pfuncFlush == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncFlush)(cachetype, pvAdrs, stBytes));
#endif
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_CacheInvalidate
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ��������Ч(���ʲ�����)
** �䡡��  : cachetype                     CACHE ����
**           pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : invalidate ��������ȷ����صĻ�д�� cache �ж���.

                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheInvalidate (LW_CACHE_TYPE   cachetype, 
                            PVOID           pvAdrs, 
                            size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    iError = ((_G_cacheopLib.CACHEOP_pfuncClear == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncClear)(cachetype, pvAdrs, stBytes));
#else
    iError = ((_G_cacheopLib.CACHEOP_pfuncInvalidate == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncInvalidate)(cachetype, pvAdrs, stBytes));
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_CacheInvalidatePage
** ��������: ָ�����͵� CACHE ʹָ����ҳ����Ч(���ʲ�����)
** �䡡��  : cachetype                     CACHE ����
**           pvAdrs                        �����ַ
**           pvPdrs                        �����ַ
**           stBytes                       ���� (ҳ�泤��������)
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : invalidate ��������ȷ����صĻ�д�� cache �ж���.

                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheInvalidatePage (LW_CACHE_TYPE   cachetype, 
                                PVOID           pvAdrs, 
                                PVOID           pvPdrs,
                                size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
#if LW_CFG_VMM_EN > 0
    iError = ((_G_cacheopLib.CACHEOP_pfuncClearPage == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncClearPage)(cachetype, pvAdrs, pvPdrs, stBytes));
#else
    iError = ((_G_cacheopLib.CACHEOP_pfuncClear == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncClear)(cachetype, pvAdrs, stBytes));
#endif
#else
#if LW_CFG_VMM_EN > 0
    iError = ((_G_cacheopLib.CACHEOP_pfuncInvalidatePage == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncInvalidatePage)(cachetype, pvAdrs, pvPdrs, stBytes));
#else
    iError = ((_G_cacheopLib.CACHEOP_pfuncInvalidate == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncInvalidate)(cachetype, pvAdrs, stBytes));
#endif
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_CacheClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype                     CACHE ����
**           pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheClear (LW_CACHE_TYPE   cachetype, 
                       PVOID           pvAdrs, 
                       size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;

    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncClear == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncClear)(cachetype, pvAdrs, stBytes));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_CacheClearPage
** ��������: ָ�����͵� CACHE ʹָ����ҳ���д����Ч
** �䡡��  : cachetype                     CACHE ����
**           pvAdrs                        �����ַ
**           pvPdrs                        �����ַ
**           stBytes                       ���� (ҳ�泤��������)
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheClearPage (LW_CACHE_TYPE   cachetype, 
                           PVOID           pvAdrs, 
                           PVOID           pvPdrs,
                           size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;

    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
#if LW_CFG_VMM_EN > 0
    iError = ((_G_cacheopLib.CACHEOP_pfuncClearPage == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncClearPage)(cachetype, pvAdrs, pvPdrs, stBytes));
#else
    iError = ((_G_cacheopLib.CACHEOP_pfuncClear == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncClear)(cachetype, pvAdrs, stBytes));
#endif
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __cacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_VMM_L4_HYPERVISOR_EN == 0)

static INT __cacheTextUpdate (LW_CACHE_TU_ARG *ptuarg)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncTextUpdate == LW_NULL) ? ERROR_NONE :
              (_G_cacheopLib.CACHEOP_pfuncTextUpdate)(ptuarg->TUA_pvAddr, ptuarg->TUA_stSize));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    return  (iError);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
                                                                        /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
/*********************************************************************************************************
** ��������: API_CacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    INTREG          iregInterLevel;
    INT             iError;

#if (LW_CFG_SMP_EN > 0) && (LW_CFG_VMM_L4_HYPERVISOR_EN == 0)
    if ((_G_cacheopLib.CACHEOP_ulOption & CACHE_TEXT_UPDATE_MP) && 
        (LW_NCPUS > 1)) {                                               /*  ��Ҫ֪ͨ���� CPU            */
        ULONG           ulNesting;
        LW_CACHE_TU_ARG tuarg;
        BOOL            bLock = LW_FALSE;                               /*  For no warning              */

        tuarg.TUA_pvAddr = pvAdrs;
        tuarg.TUA_stSize = stBytes;
        
        ulNesting = LW_CPU_GET_CUR_NESTING();
        if (!ulNesting) {
            bLock = __SMP_CPU_LOCK();                                   /*  ������ǰ CPU ִ��           */
        }

        __CACHE_OP_ENTER(iregInterLevel);                               /*  ��ʼ���� cache              */
        iError = ((_G_cacheopLib.CACHEOP_pfuncTextUpdate == LW_NULL) ? ERROR_NONE :
                  (_G_cacheopLib.CACHEOP_pfuncTextUpdate)(pvAdrs, stBytes));
        __CACHE_OP_EXIT(iregInterLevel);                                /*  �������� cache              */

        _SmpCallFuncAllOther(__cacheTextUpdate, &tuarg,
                             LW_NULL, LW_NULL, IPIM_OPT_NORMAL);        /*  ֪ͨ������ CPU              */

        if (!ulNesting) {
            __SMP_CPU_UNLOCK(bLock);                                    /*  ������ǰ CPU ִ��           */
        }

    } else
#endif                                                                  /*  LW_CFG_SMP_EN               */
    {
        __CACHE_OP_ENTER(iregInterLevel);                               /*  ��ʼ���� cache              */
        iError = ((_G_cacheopLib.CACHEOP_pfuncTextUpdate == LW_NULL) ? ERROR_NONE :
                  (_G_cacheopLib.CACHEOP_pfuncTextUpdate)(pvAdrs, stBytes));
        __CACHE_OP_EXIT(iregInterLevel);                                /*  �������� cache              */
    }
                                                                        /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_CacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncDataUpdate == LW_NULL) ? ERROR_NONE :
              (_G_cacheopLib.CACHEOP_pfuncDataUpdate)(pvAdrs, stBytes, bInv));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_CacheLocalTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE (���Ե�ǰ CPU ��Ч)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : BSP ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheLocalTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  ��ʼ���� cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncTextUpdate == LW_NULL) ? ERROR_NONE :
              (_G_cacheopLib.CACHEOP_pfuncTextUpdate)(pvAdrs, stBytes));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  �������� cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_CacheDmaMalloc
** ��������: ����һ��ǻ�����ڴ�
** �䡡��  : 
**           stBytes                       ����
** �䡡��  : ���ٵĿռ��׵�ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID    API_CacheDmaMalloc (size_t   stBytes)
{
    return  ((_G_cacheopLib.CACHEOP_pfuncDmaMalloc == LW_NULL) ?
             (__KHEAP_ALLOC_ALIGN(stBytes, LW_CFG_CPU_ARCH_CACHE_LINE)) :
             (_G_cacheopLib.CACHEOP_pfuncDmaMalloc(stBytes)));
}
/*********************************************************************************************************
** ��������: API_CacheDmaMallocAlign
** ��������: ����һ��ǻ�����ڴ�, ָ���ڴ�����ϵ
** �䡡��  : 
**           stBytes                       ����
**           stAlign                       �����ֽ���
** �䡡��  : ���ٵĿռ��׵�ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID    API_CacheDmaMallocAlign (size_t   stBytes, size_t  stAlign)
{
    size_t  stAlignMax;

    if (stAlign & (stAlign - 1)) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    stAlignMax = (stAlign > LW_CFG_CPU_ARCH_CACHE_LINE)
               ? stAlign : LW_CFG_CPU_ARCH_CACHE_LINE;

    return  ((_G_cacheopLib.CACHEOP_pfuncDmaMallocAlign == LW_NULL) ?
             (__KHEAP_ALLOC_ALIGN(stBytes, stAlignMax)) :
             (_G_cacheopLib.CACHEOP_pfuncDmaMallocAlign(stBytes, stAlign)));
}
/*********************************************************************************************************
** ��������: API_CacheDmaFree
** ��������: �黹һ��ǻ�����ڴ�
** �䡡��  : 
**           pvBuf                         ����ʱ���׵�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_CacheDmaFree (PVOID  pvBuf)
{
    if (_G_cacheopLib.CACHEOP_pfuncDmaFree) {
        _G_cacheopLib.CACHEOP_pfuncDmaFree(pvBuf);
    
    } else {
        __KHEAP_FREE(pvBuf);
    }
}
/*********************************************************************************************************
** ��������: API_CacheDmaFlush
** ��������: DMA ���� CACHE �����л���ָ�������������(��д���ڴ�)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheDmaFlush (PVOID  pvAdrs, size_t  stBytes)
{
    return  ((_G_cacheopLib.CACHEOP_pfuncDmaMalloc == LW_NULL) ?
             (API_CacheFlush(DATA_CACHE, pvAdrs, stBytes)) : 
             (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_CacheDmaInvalidate
** ��������: DMA ���� CACHE ʹ���ֻ�ȫ��������Ч(���ʲ�����)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheDmaInvalidate (PVOID  pvAdrs, size_t  stBytes)
{
    return  ((_G_cacheopLib.CACHEOP_pfuncDmaMalloc == LW_NULL) ?
             (API_CacheInvalidate(DATA_CACHE, pvAdrs, stBytes)) : 
             (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_CacheDmaClear
** ��������: DMA ���� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT    API_CacheDmaClear (PVOID  pvAdrs, size_t  stBytes)
{
    return  ((_G_cacheopLib.CACHEOP_pfuncDmaMalloc == LW_NULL) ?
             (API_CacheClear(DATA_CACHE, pvAdrs, stBytes)) : 
             (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_CacheFuncsSet
** ��������: ����������Ϣ���� CACHE ������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_CacheFuncsSet (VOID)
{
    if (_G_uiDCacheMode & CACHE_WRITETHROUGH) {                         /*  ����дͨģʽ D CACHE        */
        _G_cacheopLib.CACHEOP_pfuncFlush      = LW_NULL;
        _G_cacheopLib.CACHEOP_pfuncFlushPage  = LW_NULL;
    }
    
    if (_G_uiDCacheMode & CACHE_SNOOP_ENABLE) {                         /*  D CACHE ʼ�����ڴ�һ��      */
        _G_cacheopLib.CACHEOP_pfuncFlush          = LW_NULL;
        _G_cacheopLib.CACHEOP_pfuncFlushPage      = LW_NULL;
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
        _G_cacheopLib.CACHEOP_pfuncInvalidate     = LW_NULL;
        _G_cacheopLib.CACHEOP_pfuncInvalidatePage = LW_NULL;
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
        _G_cacheopLib.CACHEOP_pfuncClear          = LW_NULL;
        _G_cacheopLib.CACHEOP_pfuncClearPage      = LW_NULL;
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
