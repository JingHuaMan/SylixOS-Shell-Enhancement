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
** 文   件   名: _ThreadJoinLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 18 日
**
** 描        述: 线程合并和解锁CORE函数库

** BUG
2007.11.13  使用链表库对链表操作进行完全封装.
2007.11.13  加入 TCB_ptcbJoin 信息记录与判断.
2008.03.30  使用新的就绪环操作.
2010.08.03  将外部不使用的函数改为 static 类型, 
            这里的函数都是在内核锁定模式下调用的, 所以只需关闭中断就可避免 SMP 抢占.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2014.12.03  不再使用 suspend 操作, 转而使用 LW_THREAD_STATUS_JOIN.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _ThreadJoinWait
** 功能描述: 线程合并后阻塞自己 (在进入内核并关中断后被调用)
** 输　入  : ptcbCur   当前任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _ThreadJoinWait (PLW_CLASS_TCB  ptcbCur)
{
    REGISTER PLW_CLASS_PCB  ppcbMe;
             
    ppcbMe = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcbMe);                             /*  从就绪环中删除              */
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_JOIN;                     /*  设置为 join 状态            */
}
/*********************************************************************************************************
** 函数名称: _ThreadJoinWakeup
** 功能描述: 就绪其他线程 (在进入内核后被调用)
** 输　入  : ptcb      任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _ThreadJoinWakeup (PLW_CLASS_TCB  ptcb)
{
    REGISTER PLW_CLASS_PCB  ppcb;
    
    ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_JOIN);
    if (__LW_THREAD_IS_READY(ptcb)) {
       ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;              /*  中断激活方式                */
       ppcb = _GetPcb(ptcb);
       __ADD_TO_READY_RING(ptcb, ppcb);                                 /*  加入就绪环                  */
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadJoin
** 功能描述: 将当前线程合并到其他线程 (在进入内核后被调用)
** 输　入  : ptcbDes          要合并的线程
**           ptwj             Wait Join 表项
**           ppvRetValSave    目的线程结束时的返回值存放地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadJoin (PLW_CLASS_TCB  ptcbDes, PLW_CLASS_WAITJOIN  ptwj, PVOID  *ppvRetValSave)
{
    INTREG                iregInterLevel;
    PLW_CLASS_TCB         ptcbCur;
    
    LW_TCB_GET_CUR(ptcbCur);                                            /*  当前任务控制块              */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_JOIN, 
                      ptcbCur->TCB_ulId, ptcbDes->TCB_ulId, LW_NULL);
    
    if (ptcbDes) {
        ptcbCur->TCB_ppvJoinRetValSave = ppvRetValSave;                 /*  保存存放返回值的地址        */
                                                                        /*  加入等待队列                */
        _List_Line_Add_Ahead(&ptcbCur->TCB_lineJoin, &ptcbDes->TCB_plineJoinHeader);

        ptcbCur->TCB_ptcbJoin = ptcbDes;                                /*  记录目标线程                */

        iregInterLevel = KN_INT_DISABLE();
        _ThreadJoinWait(ptcbCur);                                       /*  阻塞自己等待对方激活        */
        KN_INT_ENABLE(iregInterLevel);

    } else if (ptwj) {                                                  /*  等待回收 ID 资源            */
        if (ppvRetValSave) {
            *ppvRetValSave = ptwj->TWJ_pvReturn;
        }
        _Free_Tcb_Object(ptwj->TWJ_ptcb);                               /*  释放 ID                     */
        ptwj->TWJ_ptcb = LW_NULL;
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadDisjoin
** 功能描述: 指定线程解除合并的线程 (在进入内核后被调用)
** 输　入  : ptcbDes          合并的目标线程
**           ptcbDisjoin      需要解除合并的线程
**           bWakeup          是否唤醒 (FALSE 时不操作就绪表)
**           pvArg            唤醒参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadDisjoin (PLW_CLASS_TCB  ptcbDes, PLW_CLASS_TCB  ptcbDisjoin, BOOL  bWakeup, PVOID  pvArg)
{
    INTREG  iregInterLevel;
    
    if (ptcbDisjoin->TCB_ppvJoinRetValSave) {                           /*  等待返回值                  */
        *ptcbDisjoin->TCB_ppvJoinRetValSave = pvArg;                    /*  保存返回值                  */
    }
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_DETACH, 
                      ptcbDisjoin->TCB_ulId, pvArg, LW_NULL);
                      
    _List_Line_Del(&ptcbDisjoin->TCB_lineJoin, &ptcbDes->TCB_plineJoinHeader);
    
    ptcbDisjoin->TCB_ptcbJoin = LW_NULL;                                /*  清除记录的等待线程 tcb      */
    
    iregInterLevel = KN_INT_DISABLE();
    if (bWakeup) {
        _ThreadJoinWakeup(ptcbDisjoin);                                 /*  释放合并的线程              */
    } else {
        ptcbDisjoin->TCB_usStatus &= (~LW_THREAD_STATUS_JOIN);          /*  仅修改状态即可              */
    }
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: _ThreadDetach
** 功能描述: 指定线程解除合并的所有其他线程并不允许其他线程合并自己 (在进入内核后被调用)
** 输　入  : ptcbDes          合并的目标线程
**           ptwj             Wait Join 表项
**           pvRetVal         返回值
** 输　出  : 唤醒数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  _ThreadDetach (PLW_CLASS_TCB  ptcbDes, PLW_CLASS_WAITJOIN  ptwj, PVOID  pvRetVal)
{
             INT            iCnt = 0;
    REGISTER PLW_CLASS_TCB  ptcbJoin;

    if (ptcbDes) {
        while (ptcbDes->TCB_plineJoinHeader) {
            iCnt++;
            ptcbJoin = _LIST_ENTRY(ptcbDes->TCB_plineJoinHeader, LW_CLASS_TCB, TCB_lineJoin);
            _ThreadDisjoin(ptcbDes, ptcbJoin, LW_TRUE, pvRetVal);
        }
        ptcbDes->TCB_bDetachFlag = LW_TRUE;                             /*  严禁合并自己                */

    } else if (ptwj) {
        _Free_Tcb_Object(ptwj->TWJ_ptcb);                               /*  释放 ID                     */
        ptwj->TWJ_ptcb = LW_NULL;
    }

    return  (iCnt);
}
/*********************************************************************************************************
** 函数名称: _ThreadWjAdd
** 功能描述: 将指定线程放在 Wait Join 表中 (在进入内核后被调用)
** 输　入  : ptcbDes          指定线程
**           ptwj             Wait Join 表项
**           pvRetVal         返回值
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _ThreadWjAdd (PLW_CLASS_TCB  ptcbDes, PLW_CLASS_WAITJOIN  ptwj, PVOID  pvRetVal)
{
    ptwj->TWJ_ptcb     = ptcbDes;
    ptwj->TWJ_pvReturn = pvRetVal;
}
/*********************************************************************************************************
** 函数名称: _ThreadWjClear
** 功能描述: 清除 Wait Join 表 (在进入内核后被调用)
** 输　入  : pvVProc   进程控制块
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _ThreadWjClear (PVOID  pvVProc)
{
    INT  i;

    for (i = LW_NCPUS; i < LW_CFG_MAX_THREADS; i++) {
        if (_K_twjTable[i].TWJ_ptcb) {
            if (_K_twjTable[i].TWJ_ptcb->TCB_pvVProcessContext == pvVProc) {
                _Free_Tcb_Object(_K_twjTable[i].TWJ_ptcb);              /*  释放 ID                     */
                _K_twjTable[i].TWJ_ptcb = LW_NULL;
            }
        }
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
