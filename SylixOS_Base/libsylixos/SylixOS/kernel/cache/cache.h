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
** ��   ��   ��: cache.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 09 �� 27 ��
**
** ��        ��: ƽ̨�޹صĸ��ٻ�����������. BSP �� ����������Ҫ #include "cache.h"

** BUG
2008.05.02 ������ص�ע��.
2008.12.05 ������� DMA �ڴ����.
2009.06.18 ȡ�� CACHE �Ե�ַӳ��Ĺ���, ��ȫ���� VMM ������.
2013.12.20 ȡ�� PipeFlush ����, ������ͳһ�� DCACHE �ʵ���λ�ý��й���.
*********************************************************************************************************/

#ifndef __CACHE_H
#define __CACHE_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
/*********************************************************************************************************
  CACHE MODE DEFINE 
*********************************************************************************************************/

#define CACHE_DISABLED                  0x00                            /*  CACHE ��·ģʽ              */
#define CACHE_WRITETHROUGH              0x01                            /*  CACHE дͨģʽ              */
#define CACHE_COPYBACK                  0x02                            /*  CACHE ��дģʽ              */

#define CACHE_SNOOP_DISABLE             0x00                            /*  CACHE �������ڴ����ݲ�һ��  */
#define CACHE_SNOOP_ENABLE              0x10                            /*  ���߼���, CACHE ���ڴ�һ��  */

/*********************************************************************************************************
  CACHE TYPE
*********************************************************************************************************/

typedef enum {
    INSTRUCTION_CACHE = 0,                                              /*  ָ�� CACHE                  */
    DATA_CACHE        = 1                                               /*  ���� CACHE                  */
} LW_CACHE_TYPE;

/*********************************************************************************************************
  CACHE MODE
*********************************************************************************************************/

typedef UINT    CACHE_MODE;                                             /*  CACHE ģʽ                  */

/*********************************************************************************************************
  CACHE OP STRUCT
  
  ע��: ����ϵͳ�ͷ�����ռ�ʱ, ����ô˺��� CACHEOP_pfuncVmmAreaInv, ������ռ��ڿ��ܴ�������ҳ��, ����
        ������, ������� CPU �ڰ��������ַ��д����Ч CACHE ʱ, ���������ڵ�����ҳ��, ���ܻᱨ��, ����
        ARM ������, ����������Ҫ���� DCACHE �����ݻ�д. ������� CACHEOP_pfuncClear ����. 
        
        TextUpdate �� DataUpdate ���� SMP ÿһ�� CPU ������ CACHE ����, SMP ���� CACHE û������.
*********************************************************************************************************/

typedef struct {
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    ULONG           CACHEOP_ulOption;                                   /*  cache ѡ��                  */
#define CACHE_TEXT_UPDATE_MP        0x01                                /*  ÿһ�����Ƿ�Ҫ text update*/
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

    INT             CACHEOP_iILoc;                                      /*  ָ�� cache λ��             */
    INT             CACHEOP_iDLoc;                                      /*  ���� cache λ��             */
#define CACHE_LOCATION_VIVT         0                                   /*  �����ַ CACHE              */
#define CACHE_LOCATION_PIPT         1                                   /*  �����ַ CACHE              */
#define CACHE_LOCATION_VIPT         2                                   /*  ���� INDEX ���� TAG         */

    INT             CACHEOP_iICacheLine;                                /*  cache line �ֽ���           */
    INT             CACHEOP_iDCacheLine;                                /*  cache line �ֽ���           */
    
    INT             CACHEOP_iICacheWaySize;                             /*  dcache ÿһ· �ֽ���        */
    INT             CACHEOP_iDCacheWaySize;                             /*  dcache ÿһ· �ֽ���        */
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    FUNCPTR         CACHEOP_pfuncEnable;                                /*  ���� CACHE                  */
    FUNCPTR         CACHEOP_pfuncDisable;                               /*  ֹͣ CACHE                  */
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

    FUNCPTR         CACHEOP_pfuncLock;                                  /*  ���� CACHE                  */
    FUNCPTR         CACHEOP_pfuncUnlock;                                /*  ���� CACHE                  */
    
    FUNCPTR         CACHEOP_pfuncFlush;                                 /*  �� CACHE ָ�����ݻ�д�ڴ�   */
    FUNCPTR         CACHEOP_pfuncFlushPage;                             /*  ��дָ��������ҳ��          */
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    FUNCPTR         CACHEOP_pfuncInvalidate;                            /*  ʹ CACHE ָ��������Ч       */
    FUNCPTR         CACHEOP_pfuncInvalidatePage;                        /*  ��Чָ��������ҳ��          */
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
    FUNCPTR         CACHEOP_pfuncClear;                                 /*  ��д����Ч���� CACHE ����   */
    FUNCPTR         CACHEOP_pfuncClearPage;                             /*  ��д����Чָ��������ҳ��    */
    
    FUNCPTR         CACHEOP_pfuncTextUpdate;                            /*  ��д D CACHE ��Ч I CACHE   */
    FUNCPTR         CACHEOP_pfuncDataUpdate;                            /*  ��д/��д��Ч L1 D CACHE    */
    
    PVOIDFUNCPTR    CACHEOP_pfuncDmaMalloc;                             /*  ����һ��ǻ�����ڴ�        */
    PVOIDFUNCPTR    CACHEOP_pfuncDmaMallocAlign;                        /*  ����һ��ǻ�����ڴ�(����)  */
    VOIDFUNCPTR     CACHEOP_pfuncDmaFree;                               /*  ����һ��ǻ�����ڴ�        */
} LW_CACHE_OP;                                                          /*  CACHE ������                */

/*********************************************************************************************************
  ͨ�� CACHE ���ʼ������, ����� SMP ϵͳ, ��ֻ��Ҫ������ API_KernelStart �ص��е��ü���
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
LW_API ULONG        API_CacheLibPrimaryInit(CACHE_MODE  uiInstruction, 
                                            CACHE_MODE  uiData, 
                                            CPCHAR      pcMachineName);

#define API_CacheLibInit    API_CacheLibPrimaryInit

#if LW_CFG_SMP_EN > 0
LW_API ULONG        API_CacheLibSecondaryInit(CPCHAR  pcMachineName);
#endif                                                                  /*  LW_CFG_SMP_EN               */
#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  ͨ�� CACHE �� API ����
*********************************************************************************************************/

LW_API LW_CACHE_OP *API_CacheGetLibBlock(VOID);
LW_API ULONG        API_CacheGetOption(VOID);
LW_API CACHE_MODE   API_CacheGetMode(LW_CACHE_TYPE  cachetype);
LW_API INT          API_CacheLocation(LW_CACHE_TYPE  cachetype);
LW_API INT          API_CacheLine(LW_CACHE_TYPE  cachetype);
LW_API size_t       API_CacheWaySize(LW_CACHE_TYPE  cachetype);
LW_API BOOL         API_CacheAliasProb(VOID);

LW_API INT          API_CacheEnable(LW_CACHE_TYPE  cachetype);
LW_API INT          API_CacheDisable(LW_CACHE_TYPE cachetype);

#if LW_CFG_SMP_EN > 0
LW_API INT          API_CacheBarrier(VOIDFUNCPTR pfuncHook, PVOID pvArg, size_t stSize, const PLW_CLASS_CPUSET pcpuset);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

LW_API INT          API_CacheLock(LW_CACHE_TYPE   cachetype, PVOID  pvAdrs, size_t  stBytes);
LW_API INT          API_CacheUnlock(LW_CACHE_TYPE cachetype, PVOID  pvAdrs, size_t  stBytes);

LW_API INT          API_CacheFlush(LW_CACHE_TYPE  cachetype, PVOID pvAdrs, size_t  stBytes);
LW_API INT          API_CacheFlushPage(LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs,size_t stBytes);
LW_API INT          API_CacheInvalidate(LW_CACHE_TYPE cachetype, PVOID pvAdrs, size_t  stBytes);
LW_API INT          API_CacheInvalidatePage(LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes);
LW_API INT          API_CacheClear(LW_CACHE_TYPE cachetype, PVOID  pvAdrs, size_t  stBytes);
LW_API INT          API_CacheClearPage(LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes);
LW_API INT          API_CacheTextUpdate(PVOID  pvAdrs, size_t  stBytes);
LW_API INT          API_CacheDataUpdate(PVOID  pvAdrs, size_t  stBytes, BOOL  bInv);
LW_API INT          API_CacheLocalTextUpdate(PVOID  pvAdrs, size_t  stBytes);

LW_API PVOID        API_CacheDmaMalloc(size_t   stBytes);
LW_API PVOID        API_CacheDmaMallocAlign(size_t   stBytes, size_t  stAlign);
LW_API VOID         API_CacheDmaFree(PVOID      pvBuf);

LW_API INT          API_CacheDmaFlush(PVOID  pvAdrs, size_t  stBytes);
LW_API INT          API_CacheDmaInvalidate(PVOID  pvAdrs, size_t  stBytes);
LW_API INT          API_CacheDmaClear(PVOID  pvAdrs, size_t  stBytes);

/*********************************************************************************************************
  ���� CACHE ����, �Ż���Ӧ�ĺ�������.
*********************************************************************************************************/

LW_API VOID         API_CacheFuncsSet(VOID);

/*********************************************************************************************************
  VxWorks ���� CACHE ������
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#define cacheGetLibBlock            API_CacheGetLibBlock
#define cacheGetOption              API_CacheGetOption
#define cacheGetMode                API_CacheGetMode
#define cacheLocation               API_CacheLocation
#define cacheLine                   API_CacheLine
#define cacheWaySize                API_CacheWaySize
#define cacheAliasProb              API_CacheAliasProb

#define cacheEnable                 API_CacheEnable
#define cacheDisable                API_CacheDisable

#if LW_CFG_SMP_EN > 0
#define cacheBarrier                API_CacheBarrier
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

#define cacheLock                   API_CacheLock
#define cacheUnlock                 API_CacheUnlock

#define cacheFlush                  API_CacheFlush
#define cacheFlushPage              API_CacheFlushPage
#define cacheInvalidate             API_CacheInvalidate
#define cacheInvalidatePage         API_CacheInvalidatePage
#define cacheClear                  API_CacheClear
#define cacheClearPage              API_CacheClearPage
#define cacheTextUpdate             API_CacheTextUpdate
#define cacheLocalTextUpdate        API_CacheLocalTextUpdate
#define cacheDataUpdate             API_CacheDataUpdate

#define cacheDmaMalloc              API_CacheDmaMalloc
#define cacheDmaMallocAlign         API_CacheDmaMallocAlign
#define cacheDmaFree                API_CacheDmaFree

#define cacheDmaFlush               API_CacheDmaFlush
#define cacheDmaInvalidate          API_CacheDmaInvalidate
#define cacheDmaClear               API_CacheDmaClear

#define cacheFuncsSet               API_CacheFuncsSet

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  __CACHE_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
