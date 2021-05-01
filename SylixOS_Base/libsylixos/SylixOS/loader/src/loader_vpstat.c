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
** 文   件   名: loader_vpstat.c
**
** 创   建   人: Han.hui (韩辉)
**
** 文件创建日期: 2019 年 06 月 03 日
**
** 描        述: 进程数据统计.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
/*********************************************************************************************************
  进程控制块
*********************************************************************************************************/
extern LW_LD_VPROC  *_G_pvprocTable[LW_CFG_MAX_THREADS];
/*********************************************************************************************************
** 函数名称: vprocMemInfo
** 功能描述: 获得进程内存使用情况
** 输　入  : pid           进程号
**           pstStatic     静态内存大小
**           pstHeap       内存堆消耗
**           pstMmap       mmap 大小
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocMemInfoNoLock (LW_LD_VPROC  *pvproc, size_t  *pstStatic, size_t  *pstHeap, size_t  *pstMmap)
{
    size_t        stStatic = 0;
    size_t        stHeap   = 0;
    size_t        stMmap   = 0;

    INT                 i, iNum;
    ULONG               ulPages;
    BOOL                bStart;
    LW_LIST_RING       *pringTemp;
    LW_LD_EXEC_MODULE  *pmodTemp;
    PVOID               pvVmem[LW_LD_VMEM_MAX];

#if LW_CFG_VMM_EN == 0
    PLW_CLASS_HEAP      pheapVpPatch;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */

    if (!pvproc) {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }

    LW_VP_LOCK(pvproc);
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

#if LW_CFG_VMM_EN > 0
        ulPages = 0;
        if (API_VmmPCountInArea(pmodTemp->EMOD_pvBaseAddr, &ulPages) == ERROR_NONE) {
            stStatic += (size_t)(ulPages << LW_CFG_VMM_PAGE_SHIFT);
        }
#else
        stStatic += pmodTemp->EMOD_stLen;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
        stStatic += __moduleSymbolBufferSize(pmodTemp);
    }

    if (stStatic) {                                                     /*  至少存在一个模块            */
        pmodTemp = _LIST_ENTRY(pvproc->VP_ringModules, LW_LD_EXEC_MODULE, EMOD_ringModules);
        ulPages  = 0;

#if LW_CFG_VMM_EN > 0
        iNum = __moduleVpPatchVmem(pmodTemp, pvVmem, LW_LD_VMEM_MAX);
        if (iNum > 0) {
            for (i = 0; i < iNum; i++) {
                if (API_VmmPCountInArea(pvVmem[i],
                                        &ulPages) == ERROR_NONE) {
                    stHeap += (size_t)(ulPages << LW_CFG_VMM_PAGE_SHIFT);
                }
            }
        }
#else
        pheapVpPatch = (PLW_CLASS_HEAP)__moduleVpPatchHeap(pmodTemp);
        if (pheapVpPatch) {                                             /*  获得 vp 进程私有 heap       */
            stHeap += (size_t)(pheapVpPatch->HEAP_stTotalByteSize);
        }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    }
    LW_VP_UNLOCK(pvproc);

#if LW_CFG_VMM_EN > 0
    API_VmmMmapPCount(pvproc->VP_pid, &stMmap);                         /*  计算 mmap 内存实际消耗量    */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (pstStatic) {
        *pstStatic = stStatic;
    }

    if (pstHeap) {
        *pstHeap = stHeap;
    }

    if (pstMmap) {
        *pstMmap = stMmap;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: vprocMemInfo
** 功能描述: 获得进程内存使用情况
** 输　入  : pid           进程号
**           pstStatic     静态内存大小
**           pstHeap       内存堆消耗
**           pstMmap       mmap 大小
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  vprocMemInfo (pid_t  pid, size_t  *pstStatic, size_t  *pstHeap, size_t  *pstMmap)
{
    INT           iRet;
    LW_LD_VPROC  *pvproc;

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (!pvproc) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }

    iRet = vprocMemInfoNoLock(pvproc, pstStatic, pstHeap, pstMmap);
    LW_LD_UNLOCK();

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: vprocListGet
** 功能描述: 获得进程列表
** 输　入  : pidTable      进程号表
**           uiMaxCnt      缓存个数
** 输　出  : 获取的进程个数
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  vprocListGet (pid_t  pidTable[], UINT  uiMaxCnt)
{
    INT  i, iNum = 0;

    if (!pidTable || !uiMaxCnt) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_LD_LOCK();
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
        if (_G_pvprocTable[i]) {
            pidTable[iNum] = _G_pvprocTable[i]->VP_pid;
            iNum++;
            if (iNum >= uiMaxCnt) {
                break;
            }
        }
    }
    LW_LD_UNLOCK();

    return  (iNum);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
