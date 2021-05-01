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
** 文   件   名: tsync.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2020 年 06 月 11 日
**
** 描        述: 文件系统背景回写任务.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && ((LW_CFG_DISKCACHE_EN > 0) || (LW_CFG_YAFFS_EN > 0))
/*********************************************************************************************************
  回调类型
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        TSYNC_lineManage;
    VOIDFUNCPTR         TSYNC_pfuncSync;
    PVOID               TSYNC_pvArg;
} LW_TSYNC_NODE;
typedef LW_TSYNC_NODE  *PLW_TSYNC_NODE;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static PLW_LIST_LINE    _G_plineTSyncHeader = LW_NULL;                  /*  链表头                      */
static BOOL             _G_bTSyncOnce       = LW_FALSE;
/*********************************************************************************************************
  背景线程属性
*********************************************************************************************************/
static LW_CLASS_THREADATTR  _G_threadattrTSyncAttr = {
    LW_NULL,
    LW_CFG_THREAD_DEFAULT_GUARD_SIZE,
    LW_CFG_THREAD_DISKCACHE_STK_SIZE,
    LW_PRIO_T_CACHE,
    LW_CFG_TSYNC_OPTION | LW_OPTION_OBJECT_GLOBAL,                      /*  内核全局线程                */
    LW_NULL,
    LW_NULL,
};
/*********************************************************************************************************
** 函数名称: __tsyncThread
** 功能描述: 文件系统背景回写任务
** 输　入  : pvArg             启动参数
** 输　出  : NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  __tsyncThread (PVOID  pvArg)
{
    PLW_LIST_LINE   plineCache;
    PLW_TSYNC_NODE  ptsync;

    for (;;) {
        API_TimeSSleep(LW_CFG_TSYNC_PERIOD);                            /*  近似延时                    */

        for (plineCache  = _G_plineTSyncHeader;
             plineCache != LW_NULL;
             plineCache  = _list_line_get_next(plineCache)) {

            ptsync = _LIST_ENTRY(plineCache, LW_TSYNC_NODE, TSYNC_lineManage);
            ptsync->TSYNC_pfuncSync(ptsync->TSYNC_pvArg);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __tsyncInit
** 功能描述: 创建文件系统背景回写任务
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __tsyncInit (VOID)
{
    LW_OBJECT_HANDLE  ulTSync;

    ulTSync = API_ThreadCreate("t_sync", __tsyncThread, &_G_threadattrTSyncAttr, LW_NULL);
    if (!ulTSync) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "t_sync thread create error.\r\n");
    }
}
/*********************************************************************************************************
** 函数名称: API_TSyncAdd
** 功能描述: 安装文件系统背景回写任务回调函数
** 输　入  : pfuncSync         回写回调函数
**           pvArg             回写回调参数
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_TSyncAdd (VOIDFUNCPTR  pfuncSync, PVOID  pvArg)
{
    PLW_TSYNC_NODE  ptsync;

    if (!pfuncSync) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    API_ThreadOnce(&_G_bTSyncOnce, __tsyncInit);                        /*  创建 t_sync 任务            */

    ptsync = (PLW_TSYNC_NODE)__SHEAP_ALLOC(sizeof(LW_TSYNC_NODE));
    if (!ptsync) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    ptsync->TSYNC_pfuncSync = pfuncSync;
    ptsync->TSYNC_pvArg     = pvArg;

    __KERNEL_ENTER();
    _List_Line_Add_Ahead(&ptsync->TSYNC_lineManage, &_G_plineTSyncHeader);
    __KERNEL_EXIT();

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0) ||*/
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
