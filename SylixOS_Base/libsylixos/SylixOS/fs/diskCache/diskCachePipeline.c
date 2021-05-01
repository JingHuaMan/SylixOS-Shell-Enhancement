/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: diskCachePipeline.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 07 月 25 日
**
** 描        述: 磁盘高速缓冲并发写管线.
**
** BUG:
2020.04.29  修正管线并行写重复扇区的错误.
            修正管线读取未写入扇区的错误.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "diskCacheLib.h"
#include "diskCache.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
/*********************************************************************************************************
  磁盘操作
*********************************************************************************************************/
#define __PHYDISK_READ      pblkd->BLKD_pfuncBlkRd
#define __PHYDISK_WRITE     pblkd->BLKD_pfuncBlkWrt
#define __PHYDISK_STATUS    pblkd->BLKD_pfuncBlkStatusChk
#define __PHYDISK_IOCTL     pblkd->BLKD_pfuncBlkIoctl
/*********************************************************************************************************
  管线缓存分配与释放
*********************************************************************************************************/
#define __PIPELINE_BUF_GET(pwp)         API_PartitionGet(pwp->DISKCWP_hPart)
#define __PIPELINE_BUF_PUT(pwp, buf)    API_PartitionPut(pwp->DISKCWP_hPart, buf)
#define __PIPELINE_BUF_WAIT(pwp)        API_SemaphoreCPend(pwp->DISKCWP_hCounter, LW_OPTION_WAIT_INFINITE)
#define __PIPELINE_BUF_NOTIFY(pwp)      API_SemaphoreCPost(pwp->DISKCWP_hCounter)
/*********************************************************************************************************
  管线锁
*********************************************************************************************************/
#define __PIPELINE_LOCK(pwp)    API_SemaphoreMPend(pwp->DISKCWP_hLock, LW_OPTION_WAIT_INFINITE)
#define __PIPELINE_UNLOCK(pwp)  API_SemaphoreMPost(pwp->DISKCWP_hLock)
/*********************************************************************************************************
  管线消息等待与通知
*********************************************************************************************************/
#define __PIPELINE_MSG_WAIT(pwp)    API_SemaphoreCPend(pwp->DISKCWP_hQueue, LW_OPTION_WAIT_INFINITE)
#define __PIPELINE_MSG_SIGNAL(pwp)  API_SemaphoreCPost(pwp->DISKCWP_hQueue)
/*********************************************************************************************************
  管线同步等待与通知
*********************************************************************************************************/
#define __PIPELINE_SYNC_WAIT(pwp)   API_SemaphoreBPend(pwp->DISKCWP_hSync, LW_OPTION_WAIT_INFINITE)
#define __PIPELINE_SYNC_SIGNAL(pwp) API_SemaphoreBPost(pwp->DISKCWP_hSync)
/*********************************************************************************************************
  扇区拷贝
*********************************************************************************************************/
extern VOID  __diskCacheMemcpy(PVOID  pvTo, CPVOID  pvFrom, size_t  stSize);
       VOID  __diskCacheWpSync(PLW_DISKCACHE_WP  pwp, ULONG  ulGetCnt);
/*********************************************************************************************************
** 函数名称: __diskCacheWpThread
** 功能描述: 写管线线程
** 输　入  : pvArg                 线程参数
** 输　出  : LW_NULL
** 全局变量: 
** 调用模块: 
** 注  意  : 这里在线程退出之前需要提前调用 detach 激活文件系统卸载任务, 因为卸载任务可能是 t_except 所以
             真正的线程删除操作可能得不到执行.
*********************************************************************************************************/
static PVOID  __diskCacheWpThread (PVOID  pvArg)
{
    REGISTER INT                 i;
             PLW_DISKCACHE_CB    pdiskc = (PLW_DISKCACHE_CB)pvArg;
             PLW_DISKCACHE_WP    pwp    = &pdiskc->DISKC_wpWrite;
    REGISTER PLW_BLK_DEV         pblkd  = pdiskc->DISKC_pblkdDisk;

             PLW_LIST_LINE       pline;
             PLW_DISKCACHE_WPMSG pdiskcwpm;
             PVOID               pvFree;
    
    for (;;) {
        if (__PIPELINE_MSG_WAIT(pwp)) {                                 /*  等待消息                    */
            break;
        }

        if (LW_UNLIKELY(pwp->DISKCWP_bExit)) {                          /*  线程退出                    */
            break;
        }

        __PIPELINE_LOCK(pwp);

        for (pline  = pwp->DISKCWP_plineHead;
             pline != LW_NULL;
             pline  = _list_line_get_prev(pline)) {

            pdiskcwpm = (PLW_DISKCACHE_WPMSG)pline;
            if (!pdiskcwpm->DISKCWPM_bWriting) {
                pdiskcwpm->DISKCWPM_bWriting = LW_TRUE;                 /*  准备操作此模块              */
                break;
            }
        }

        if (pline == LW_NULL) {
            __PIPELINE_UNLOCK(pwp);
            continue;
        }

        if (pwp->DISKCWP_bParallel) {                                   /*  并行处理这里可以释放锁      */
            __PIPELINE_UNLOCK(pwp);
        }

        i = 0;

        if (__PHYDISK_WRITE(pblkd, pdiskcwpm->DISKCWPM_pvBuffer,
                            pdiskcwpm->DISKCWPM_ulStartSector,
                            pdiskcwpm->DISKCWPM_ulNSector) < 0) {
            for (i = 0; i < pblkd->BLKD_iRetry; i++) {
                if (pblkd->BLKD_bRemovable) {
                    if (__PHYDISK_STATUS) {
                        if (__PHYDISK_STATUS(pblkd) != ERROR_NONE) {
                            continue;                                   /*  设备状态错误, 重新检测      */
                        }
                    }
                }
                if (__PHYDISK_WRITE(pblkd, pdiskcwpm->DISKCWPM_pvBuffer,
                                    pdiskcwpm->DISKCWPM_ulStartSector,
                                    pdiskcwpm->DISKCWPM_ulNSector) >= 0) {
                    break;
                }
            }
        }

        if (i >= pblkd->BLKD_iRetry) {
            if (pblkd->BLKD_bRemovable) {
                __PHYDISK_IOCTL(pblkd, FIOCANCEL, 0);
            }
            _DebugFormat(__ERRORMESSAGE_LEVEL, "can not write block: blk %s sector %lu [%ld].\r\n",
                         pblkd->BLKD_pcName,
                         pdiskcwpm->DISKCWPM_ulStartSector,
                         pdiskcwpm->DISKCWPM_ulNSector);
        }

        if (pwp->DISKCWP_bParallel) {
            __PIPELINE_LOCK(pwp);
        }

        pvFree = pdiskcwpm->DISKCWPM_pvBuffer;

        _List_Line_Del(&pdiskcwpm->DISKCWPM_lineLink, &pwp->DISKCWP_plineTail);

        if (pwp->DISKCWP_plineTail == LW_NULL) {
            pwp->DISKCWP_plineHead =  LW_NULL;
        } else if (pwp->DISKCWP_plineHead == pline) {
            pwp->DISKCWP_plineHead =  _list_line_get_prev(pline);
        }

        _List_Line_Add_Tail(&pdiskcwpm->DISKCWPM_lineLink, &pwp->DISKCWP_plineFree);

        __PIPELINE_UNLOCK(pwp);

        __PIPELINE_BUF_PUT(pwp, pvFree);                                /*  归还写缓存                  */
        __PIPELINE_BUF_NOTIFY(pwp);

        __PIPELINE_SYNC_SIGNAL(pwp);                                    /*  通知 SYNC                   */
    }
    
    API_ThreadDetach(API_ThreadIdSelf());                               /*  解除 Join 线程              */
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpTCreate
** 功能描述: 创建写管线线程组
** 输　入  : pdiskc                磁盘缓冲控制块
**           pwp                   并发写管线控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __diskCacheWpTCreate (PLW_DISKCACHE_CB  pdiskc, PLW_DISKCACHE_WP  pwp)
{
    INT                     i, j;
    LW_CLASS_THREADATTR     threadattr;
    
    API_ThreadAttrBuild(&threadattr,
                        LW_CFG_THREAD_DISKCACHE_STK_SIZE,
                        LW_PRIO_T_SERVICE,
                        LW_CFG_TSYNC_OPTION | LW_OPTION_OBJECT_GLOBAL,
                        pdiskc);
    
    for (i = 0; i < pwp->DISKCWP_iPipeline; i++) {
        pwp->DISKCWP_hWThread[i] = API_ThreadInit("t_dcwpipe", 
                                                  __diskCacheWpThread,
                                                  &threadattr,
                                                  LW_NULL);
        if (pwp->DISKCWP_hWThread[i] == LW_OBJECT_HANDLE_INVALID) {
            break;
        }
    }
    
    if (i < pwp->DISKCWP_iPipeline) {
        for (j = 0; j < i; j++) {
            API_ThreadDelete(&pwp->DISKCWP_hWThread[j], LW_NULL);
        }
        return  (PX_ERROR);
    
    } else {
        for (i = 0; i < pwp->DISKCWP_iPipeline; i++) {
            API_ThreadStart(pwp->DISKCWP_hWThread[i]);
        }
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpTDelete
** 功能描述: 删除写管线线程组
** 输　入  : pwp                   并发写管线控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __diskCacheWpTDelete (PLW_DISKCACHE_WP  pwp)
{
    INT   i;

    for (i = 0; i < pwp->DISKCWP_iPipeline; i++) {
        API_ThreadJoin(pwp->DISKCWP_hWThread[i], LW_NULL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpMemCreate
** 功能描述: 创建缓存
** 输　入  : pwp                   并发写管线控制块
**           iBurstOpt             缓存属性
**           iPipeline             写管线线程数
**           iMsgCount             消息队列长度
**           iMaxRBurstSector      读猝发长度
**           iMaxWBurstSector      写猝发长度
**           ulBytesPerSector      每扇区大小
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __diskCacheWpMemCreate (PLW_DISKCACHE_WP  pwp,
                                    INT               iBurstOpt,
                                    INT               iPipeline, 
                                    INT               iMsgCount, 
                                    INT               iMaxRBurstSector,
                                    INT               iMaxWBurstSector,
                                    ULONG             ulBytesPerSector)
{
    INT     iMaxBurst;
    size_t  stAlign = LW_CFG_CPU_ARCH_CACHE_LINE;
    PVOID   pvRBurstBuffer;
    PVOID   pvWBurstBuffer;
    
    pwp->DISKCWP_iBurstOpt = iBurstOpt;
    if (iBurstOpt & LW_DCATTR_BOPT_PAGE_ALIGN) {
        stAlign = LW_CFG_VMM_PAGE_SIZE;
    }
    
    if (iPipeline == 0) {                                               /*  不需要管线支持              */
        iMaxBurst = __MAX(iMaxRBurstSector, iMaxWBurstSector);
        
#if LW_CFG_CACHE_EN > 0
        if (iBurstOpt & LW_DCATTR_BOPT_CACHE_COHERENCE) {
            pvRBurstBuffer = API_CacheDmaMalloc((size_t)(iMaxBurst * ulBytesPerSector));
        } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        {
            pvRBurstBuffer = __SHEAP_ALLOC_ALIGN((size_t)(iMaxBurst * ulBytesPerSector), stAlign);
        }
        if (pvRBurstBuffer == LW_NULL) {
            return  (PX_ERROR);
        }
    
        pvWBurstBuffer = pvRBurstBuffer;
        
        pwp->DISKCWP_pvRBurstBuffer = pvRBurstBuffer;
        pwp->DISKCWP_pvWBurstBuffer = pvRBurstBuffer;
    
    } else {                                                            /*  需要管线写支持              */
#if LW_CFG_CACHE_EN > 0
        if (iBurstOpt & LW_DCATTR_BOPT_CACHE_COHERENCE) {
            pvRBurstBuffer = API_CacheDmaMalloc((size_t)(iMaxRBurstSector * ulBytesPerSector));
        } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        {
            pvRBurstBuffer = __SHEAP_ALLOC_ALIGN((size_t)(iMaxRBurstSector * ulBytesPerSector), stAlign);
        }
        if (pvRBurstBuffer == LW_NULL) {
            return  (PX_ERROR);
        }
    
#if LW_CFG_CACHE_EN > 0
        if (iBurstOpt & LW_DCATTR_BOPT_CACHE_COHERENCE) {
            pvWBurstBuffer = API_CacheDmaMalloc((size_t)(iMaxWBurstSector * ulBytesPerSector * iMsgCount));
        } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        {
            pvWBurstBuffer = __SHEAP_ALLOC_ALIGN((size_t)(iMaxWBurstSector * ulBytesPerSector * iMsgCount), stAlign);
        }
        if (pvWBurstBuffer == LW_NULL) {
#if LW_CFG_CACHE_EN > 0
            if (iBurstOpt & LW_DCATTR_BOPT_CACHE_COHERENCE) {
                API_CacheDmaFree(pvRBurstBuffer);
            } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
            {
                __SHEAP_FREE(pvRBurstBuffer);
            }
            return  (PX_ERROR);
        }
        
        pwp->DISKCWP_pvRBurstBuffer = pvRBurstBuffer;
        pwp->DISKCWP_pvWBurstBuffer = pvWBurstBuffer;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpMemDelete
** 功能描述: 删除缓存
** 输　入  : pwp                   并发写管线控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __diskCacheWpMemDelete (PLW_DISKCACHE_WP  pwp)
{
    if (pwp->DISKCWP_iPipeline > 0) {                                   /*  使用管线线程                */
#if LW_CFG_CACHE_EN > 0
        if (pwp->DISKCWP_iBurstOpt & LW_DCATTR_BOPT_CACHE_COHERENCE) {
            API_CacheDmaFree(pwp->DISKCWP_pvWBurstBuffer);
        } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        {
            __SHEAP_FREE(pwp->DISKCWP_pvWBurstBuffer);
        }
    }
    
#if LW_CFG_CACHE_EN > 0
    if (pwp->DISKCWP_iBurstOpt & LW_DCATTR_BOPT_CACHE_COHERENCE) {
        API_CacheDmaFree(pwp->DISKCWP_pvRBurstBuffer);
    } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    {
        __SHEAP_FREE(pwp->DISKCWP_pvRBurstBuffer);
    }
    
    pwp->DISKCWP_pvRBurstBuffer = LW_NULL;
    pwp->DISKCWP_pvWBurstBuffer = LW_NULL;
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpCreate
** 功能描述: 创建写管线
** 输　入  : pdiskc                磁盘缓冲控制块
**           pwp                   并发写管线控制块
**           bParallel             并发读写支持
**           iBurstOpt             缓存属性
**           iPipeline             写管线线程数
**           iMsgCount             管线总消息个数
**           iMaxRBurstSector      读猝发长度
**           iMaxWBurstSector      写猝发长度
**           ulBytesPerSector      每扇区大小
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 调用此函数前 pwp 已经被清零.
*********************************************************************************************************/
INT  __diskCacheWpCreate (PLW_DISKCACHE_CB  pdiskc,
                          PLW_DISKCACHE_WP  pwp, 
                          BOOL              bParallel,
                          INT               iBurstOpt,
                          INT               iPipeline, 
                          INT               iMsgCount,
                          INT               iMaxRBurstSector,
                          INT               iMaxWBurstSector,
                          ULONG             ulBytesPerSector)
{
    INT                 i, iErrLevel = 0;
    PLW_DISKCACHE_WPMSG pdiskcwpm;

    pwp->DISKCWP_bExit     = LW_FALSE;
    pwp->DISKCWP_bParallel = bParallel;
    pwp->DISKCWP_iPipeline = iPipeline;
    pwp->DISKCWP_iMsgCount = iMsgCount;
    
    if (iPipeline == 0) {
        return  (__diskCacheWpMemCreate(pwp, iBurstOpt, iPipeline, iMsgCount,
                                        iMaxRBurstSector, iMaxWBurstSector, ulBytesPerSector));
    
    } else {                                                            /*  需要并发写支持              */
        if (__diskCacheWpMemCreate(pwp, iBurstOpt, iPipeline, iMsgCount,
                                   iMaxRBurstSector, iMaxWBurstSector, ulBytesPerSector)) {
            return  (PX_ERROR);
        }

        pwp->DISKCWP_pvMsgBuffer = __SHEAP_ZALLOC(sizeof(LW_DISKCACHE_WPMSG) * iMsgCount);
        if (pwp->DISKCWP_pvMsgBuffer == LW_NULL) {
            iErrLevel = 1;
            goto    __error_handle;
        }

        pdiskcwpm = (PLW_DISKCACHE_WPMSG)pwp->DISKCWP_pvMsgBuffer;
        for (i = 0; i < iMsgCount; i++) {
            _List_Line_Add_Tail(&pdiskcwpm->DISKCWPM_lineLink, &pwp->DISKCWP_plineFree);
            pdiskcwpm++;
        }

        pwp->DISKCWP_hQueue = API_SemaphoreCCreate("dc_mq", 0, (ULONG)iMsgCount,
                                                   LW_OPTION_OBJECT_GLOBAL,
                                                   LW_NULL);
        if (pwp->DISKCWP_hQueue == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 2;
            goto    __error_handle;
        }
        
        pwp->DISKCWP_hCounter = API_SemaphoreCCreate("dc_wpcnt", (ULONG)iMsgCount, 
                                                     (ULONG)iMsgCount, 
                                                     LW_OPTION_OBJECT_GLOBAL | 
                                                     LW_OPTION_WAIT_PRIORITY,
                                                     LW_NULL);
        if (pwp->DISKCWP_hCounter == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 3;
            goto    __error_handle;
        }
        
        pwp->DISKCWP_hPart = API_PartitionCreate("dc_wppart", pwp->DISKCWP_pvWBurstBuffer,
                                                 (ULONG)iMsgCount, 
                                                 (size_t)(iMaxWBurstSector * ulBytesPerSector),
                                                 LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pwp->DISKCWP_hPart == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 4;
            goto    __error_handle;
        }
        
        pwp->DISKCWP_hSync = API_SemaphoreBCreate("dc_sync", LW_FALSE, 
                                                  LW_OPTION_OBJECT_GLOBAL,
                                                  LW_NULL);
        if (pwp->DISKCWP_hSync == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 5;
            goto    __error_handle;
        }
        
        pwp->DISKCWP_hLock = API_SemaphoreMCreate("dc_devlock",
                                                  LW_PRIO_DEF_CEILING,
                                                  LW_OPTION_INHERIT_PRIORITY |
                                                  LW_OPTION_DELETE_SAFE |
                                                  LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pwp->DISKCWP_hLock == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 6;
            goto    __error_handle;
        }
        
        if (__diskCacheWpTCreate(pdiskc, pwp)) {
            iErrLevel = 7;
            goto    __error_handle;
        }
    }
    
    return  (ERROR_NONE);
    
__error_handle:
    if (iErrLevel > 6) {
        API_SemaphoreMDelete(&pwp->DISKCWP_hLock);
    }
    if (iErrLevel > 5) {
        API_SemaphoreBDelete(&pwp->DISKCWP_hSync);
    }
    if (iErrLevel > 4) {
        API_PartitionDelete(&pwp->DISKCWP_hPart);
    }
    if (iErrLevel > 3) {
        API_SemaphoreCDelete(&pwp->DISKCWP_hCounter);
    }
    if (iErrLevel > 2) {
        API_SemaphoreCDelete(&pwp->DISKCWP_hQueue);
    }
    if (iErrLevel > 1) {
        __SHEAP_FREE(pwp->DISKCWP_pvMsgBuffer);
    }
    if (iErrLevel > 0) {
        __diskCacheWpMemDelete(pwp);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpDelete
** 功能描述: 删除写管线
** 输　入  : pwp                   并发写管线控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __diskCacheWpDelete (PLW_DISKCACHE_WP  pwp)
{
    pwp->DISKCWP_bExit = LW_TRUE;

    if (pwp->DISKCWP_iPipeline == 0) {
        __diskCacheWpMemDelete(pwp);
        return  (ERROR_NONE);
    }
    
    API_SemaphoreCDelete(&pwp->DISKCWP_hQueue);
    __diskCacheWpTDelete(pwp);
    
    API_PartitionDelete(&pwp->DISKCWP_hPart);
    API_SemaphoreCDelete(&pwp->DISKCWP_hCounter);
    API_SemaphoreBDelete(&pwp->DISKCWP_hSync);
    __diskCacheWpMemDelete(pwp);
    
    API_SemaphoreMDelete(&pwp->DISKCWP_hLock);
    __SHEAP_FREE(pwp->DISKCWP_pvMsgBuffer);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpGetBuffer
** 功能描述: 获得一个猝发操作缓冲
** 输　入  : pwp                   并发写管线控制块
**           bRead                 是否为读, 否则为写
** 输　出  : 缓冲区
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  __diskCacheWpGetBuffer (PLW_DISKCACHE_WP  pwp, BOOL bRead)
{
    PVOID  pvRet;

    if (bRead) {
        return  (pwp->DISKCWP_pvRBurstBuffer);
    }

    if (pwp->DISKCWP_iPipeline == 0) {
        return  (pwp->DISKCWP_pvWBurstBuffer);
    }
    
    if (__PIPELINE_BUF_WAIT(pwp)) {
        _BugHandle(LW_TRUE, LW_TRUE, "diskcache pipeline error!\r\n");
    }
    
    pvRet = __PIPELINE_BUF_GET(pwp);
    _BugHandle((pvRet == LW_NULL), LW_TRUE, "diskcache pipeline error!\r\n");
    
    return  (pvRet);
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpGetBuffer
** 功能描述: 交还一个未使用的猝发操作缓冲
** 输　入  : pwp                   并发写管线控制块
**           pvBuffer              缓冲区
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __diskCacheWpPutBuffer (PLW_DISKCACHE_WP  pwp, PVOID  pvBuffer)
{
    if ((pvBuffer == pwp->DISKCWP_pvRBurstBuffer) ||
        (pwp->DISKCWP_iPipeline == 0)) {
        return;
    }
    
    __PIPELINE_BUF_PUT(pwp, pvBuffer);
    __PIPELINE_BUF_NOTIFY(pwp);
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpOverlap
** 功能描述: 检查是否存在交叠
** 输　入  : pwp                控制块
**           ulStartSector      起始扇区
**           ulNSector          扇区数量
** 输　出  : 是否存在交叠
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __diskCacheWpOverlap (PLW_DISKCACHE_WP  pwp,
                                   ULONG             ulStartSector,
                                   ULONG             ulNSector)
{
    BOOL                bOverlap;
    ULONG               ulEndSector = ulStartSector + ulNSector - 1;
    PLW_LIST_LINE       pline;
    PLW_DISKCACHE_WPMSG pdiskcwpm;

    __PIPELINE_LOCK(pwp);

    for (pline  = pwp->DISKCWP_plineTail;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {

        pdiskcwpm = (PLW_DISKCACHE_WPMSG)pline;
        if ((ulStartSector < (pdiskcwpm->DISKCWPM_ulStartSector + pdiskcwpm->DISKCWPM_ulNSector)) &&
            (ulEndSector >= pdiskcwpm->DISKCWPM_ulStartSector)) {
            break;
        }
    }

    bOverlap = pline ? LW_TRUE : LW_FALSE;

    __PIPELINE_UNLOCK(pwp);

    return  (bOverlap);
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpSteal
** 功能描述: 从队列里快速读扇区
** 输　入  : pdiskc             CACHE 控制块
**           pvBuffer           缓冲区
**           ulSectorNo         扇区号
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  __diskCacheWpSteal (PLW_DISKCACHE_CB  pdiskc,
                          PVOID             pvBuffer,
                          ULONG             ulSectorNo)
{
    BOOL                bSteal;
    PCHAR               pcFrom;
    PLW_LIST_LINE       pline;
    PLW_DISKCACHE_WPMSG pdiskcwpm;
    PLW_DISKCACHE_WP    pwp = &pdiskc->DISKC_wpWrite;

    if ((pwp->DISKCWP_iPipeline == 0) || (pwp->DISKCWP_plineHead == LW_NULL)) {
        return  (LW_FALSE);
    }

    __PIPELINE_LOCK(pwp);

    for (pline  = pwp->DISKCWP_plineTail;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {

        pdiskcwpm = (PLW_DISKCACHE_WPMSG)pline;
        if ((ulSectorNo >= pdiskcwpm->DISKCWPM_ulStartSector) &&
            (ulSectorNo < (pdiskcwpm->DISKCWPM_ulStartSector + pdiskcwpm->DISKCWPM_ulNSector))) {
            break;
        }
    }

    if (pline) {
        bSteal = LW_TRUE;
        pcFrom = (PCHAR)pdiskcwpm->DISKCWPM_pvBuffer
               + ((ulSectorNo - pdiskcwpm->DISKCWPM_ulStartSector)
               * pdiskc->DISKC_ulBytesPerSector);
        __diskCacheMemcpy(pvBuffer, pcFrom, pdiskc->DISKC_ulBytesPerSector);

    } else {
        bSteal = LW_FALSE;
    }

    __PIPELINE_UNLOCK(pwp);

    return  (bSteal);
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpRead
** 功能描述: 读扇区
** 输　入  : pdiskc                磁盘缓冲控制块
**           pblkd                 磁盘物理设备
**           pvBuffer              缓冲区
**           ulStartSector         起始扇区
**           ulNSector             总扇区数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __diskCacheWpRead (PLW_DISKCACHE_CB  pdiskc,
                        PLW_BLK_DEV       pblkd,
                        PVOID             pvBuffer,
                        ULONG             ulStartSector,
                        ULONG             ulNSector)
{
             INT                iRet;
    REGISTER PLW_DISKCACHE_WP   pwp = &pdiskc->DISKC_wpWrite;
    
    if (pwp->DISKCWP_iPipeline == 0) {
        return  (__PHYDISK_READ(pblkd, pvBuffer, ulStartSector, ulNSector));
    }

    if (__diskCacheWpOverlap(pwp, ulStartSector, ulNSector)) {
        __diskCacheWpSync(pwp, 0);
    }

    if (!pwp->DISKCWP_bParallel) {
        __PIPELINE_LOCK(pwp);
    }
    
    iRet = __PHYDISK_READ(pblkd, pvBuffer, ulStartSector, ulNSector);
                                                    
    if (!pwp->DISKCWP_bParallel) {
        __PIPELINE_UNLOCK(pwp);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpWrite
** 功能描述: 提交一个写请求
** 输　入  : pdiskc                磁盘缓冲控制块
**           pblkdDisk             磁盘物理设备
**           pvBuffer              缓冲区
**           ulStartSector         起始扇区
**           ulNSector             总扇区数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __diskCacheWpWrite (PLW_DISKCACHE_CB  pdiskc,
                         PLW_BLK_DEV       pblkd,
                         PVOID             pvBuffer,
                         ULONG             ulStartSector,
                         ULONG             ulNSector)
{
    PLW_LIST_LINE        pline;
    PLW_DISKCACHE_WPMSG  pdiskcwpm;
    PLW_DISKCACHE_WP     pwp = &pdiskc->DISKC_wpWrite;

    if (pwp->DISKCWP_iPipeline == 0) {
        return  (__PHYDISK_WRITE(pblkd, pvBuffer, ulStartSector, ulNSector));
    }
    
    if (__diskCacheWpOverlap(pwp, ulStartSector, ulNSector)) {
        __diskCacheWpSync(pwp, 1);
    }

    __PIPELINE_LOCK(pwp);

    _BugHandle((pwp->DISKCWP_plineFree == LW_NULL), LW_TRUE, "diskcache pipeline msgq error!\r\n");

    pline = pwp->DISKCWP_plineFree;
    _List_Line_Del(pline, &pwp->DISKCWP_plineFree);

    pdiskcwpm = (PLW_DISKCACHE_WPMSG)pline;
    pdiskcwpm->DISKCWPM_bWriting      = LW_FALSE;
    pdiskcwpm->DISKCWPM_pvBuffer      = pvBuffer;
    pdiskcwpm->DISKCWPM_ulStartSector = ulStartSector;
    pdiskcwpm->DISKCWPM_ulNSector     = ulNSector;

    _List_Line_Add_Ahead(&pdiskcwpm->DISKCWPM_lineLink, &pwp->DISKCWP_plineTail);

    if (pwp->DISKCWP_plineHead == LW_NULL) {
        pwp->DISKCWP_plineHead =  pwp->DISKCWP_plineTail;
    }

    __PIPELINE_UNLOCK(pwp);

    __PIPELINE_MSG_SIGNAL(pwp);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __diskCacheWpSync
** 功能描述: 获得一个猝发操作缓冲
** 输　入  : pwp                   并发写管线控制块
**           ulGetCnt              当前已经占用的个数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __diskCacheWpSync (PLW_DISKCACHE_WP  pwp, ULONG  ulGetCnt)
{
    ULONG      ulError;
    ULONG      ulCounter;

    if (pwp->DISKCWP_iPipeline == 0) {
        return;
    }

    do {
        if (API_PartitionStatus(pwp->DISKCWP_hPart, LW_NULL, 
                                &ulCounter, LW_NULL)) {
            break;
        }
        if ((ulCounter + ulGetCnt) >= pwp->DISKCWP_iMsgCount) {
            __PIPELINE_SYNC_SIGNAL(pwp);
            break;
        } else {
            ulError = __PIPELINE_SYNC_WAIT(pwp);
        }
    } while (ulError == ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
