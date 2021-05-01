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
** ��   ��   ��: InterDefer.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 05 �� 09 ��
**
** ��        ��: �ж��ӳٶ��д���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_ISR_DEFER_EN > 0
/*********************************************************************************************************
  ÿһ�� CPU �� DEFER ISR QUEUE
*********************************************************************************************************/
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_ISR_DEFER_PER_CPU > 0)
static LW_OBJECT_HANDLE _K_ulIsrDefer[LW_CFG_MAX_PROCESSORS];
static LW_JOB_QUEUE     _K_jobqIsrDefer[LW_CFG_MAX_PROCESSORS];
static LW_JOB_MSG       _K_jobmsgIsrDefer[LW_CFG_MAX_PROCESSORS][LW_CFG_ISR_DEFER_SIZE];
#else
static LW_OBJECT_HANDLE _K_ulIsrDefer[1];
static LW_JOB_QUEUE     _K_jobqIsrDefer[1];
static LW_JOB_MSG       _K_jobmsgIsrDefer[LW_CFG_ISR_DEFER_SIZE];
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: __interDeferTask
** ��������: ����ж϶�ջʹ����
** �䡡��  : ulCPUId                       CPU ��
**           pstFreeByteSize               ���ж�ջ��С   (��Ϊ LW_NULL)
**           pstUsedByteSize               ʹ�ö�ջ��С   (��Ϊ LW_NULL)
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  _interDeferTask (PVOID  pvArg)
{
    PLW_JOB_QUEUE   pjobq = (PLW_JOB_QUEUE)pvArg;
    
    for (;;) {
        _jobQueueExec(pjobq, LW_OPTION_WAIT_INFINITE);
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: _interDeferInit
** ��������: ��ʼ���ж��ӳٴ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _interDeferInit (VOID)
{
    CHAR                  cDefer[LW_CFG_OBJECT_NAME_SIZE] = "t_isrdefer";
    LW_CLASS_THREADATTR   threadattr;
    
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_ISR_DEFER_PER_CPU > 0)
    INT                   i;
    LW_CLASS_CPUSET       cpuset;
    
    LW_CPU_ZERO(&cpuset);
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_DEFER_STK_SIZE, 
                        LW_CFG_ISR_DEFER_PRIO, 
                        (LW_OPTION_THREAD_STK_CHK | 
                        LW_OPTION_THREAD_SAFE | 
                        LW_OPTION_OBJECT_GLOBAL |
                        LW_OPTION_THREAD_DETACHED |
                        LW_OPTION_THREAD_AFFINITY_ALWAYS), 
                        LW_NULL);
    
    LW_CPU_FOREACH (i) {
        if (_jobQueueInit(&_K_jobqIsrDefer[i], 
                          &_K_jobmsgIsrDefer[i][0], 
                          LW_CFG_ISR_DEFER_SIZE, LW_FALSE)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create ISR defer queue.\r\n");
            return;
        }
        
        lib_itoa(i, &cDefer[10], 10);
        API_ThreadAttrSetArg(&threadattr, &_K_jobqIsrDefer[i]);
        _K_ulIsrDefer[i] = API_ThreadInit(cDefer, _interDeferTask, &threadattr, LW_NULL);
        if (_K_ulIsrDefer[i] == LW_OBJECT_HANDLE_INVALID) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create ISR defer task.\r\n");
            return;
        }
        
        LW_CPU_SET(i, &cpuset);                                         /*  ������ָ�� CPU              */
        API_ThreadSetAffinity(_K_ulIsrDefer[i], sizeof(LW_CLASS_CPUSET), &cpuset);
        LW_CPU_CLR(i, &cpuset);
        
        API_ThreadStart(_K_ulIsrDefer[i]);
    }
    
#else
    if (_jobQueueInit(&_K_jobqIsrDefer[0], 
                      &_K_jobmsgIsrDefer[0], 
                      LW_CFG_ISR_DEFER_SIZE, LW_FALSE)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create ISR defer queue.\r\n");
        return;
    }
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_DEFER_STK_SIZE, 
                        LW_CFG_ISR_DEFER_PRIO, 
                        (LW_OPTION_THREAD_STK_CHK | 
                        LW_OPTION_THREAD_SAFE | 
                        LW_OPTION_OBJECT_GLOBAL |
                        LW_OPTION_THREAD_DETACHED |
                        LW_OPTION_THREAD_AFFINITY_ALWAYS), 
                        &_K_jobqIsrDefer[0]);
    
    _K_ulIsrDefer[0] = API_ThreadInit(cDefer, _interDeferTask, &threadattr, LW_NULL);
    if (_K_ulIsrDefer[0] == LW_OBJECT_HANDLE_INVALID) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create ISR defer task.\r\n");
        return;
    }
    
    API_ThreadStart(_K_ulIsrDefer[0]);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */ 
}                                                                       /*  LW_CFG_ISR_DEFER_PER_CPU    */
/*********************************************************************************************************
** ��������: API_InterDeferGet
** ��������: ��ö�Ӧ CPU ���ж��ӳٶ���
** �䡡��  : ulCPUId       CPU ��
** �䡡��  : �ж��ӳٶ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_JOB_QUEUE  API_InterDeferGet (ULONG  ulCPUId)
{
    if (ulCPUId >= LW_NCPUS) {
        _ErrorHandle(ERANGE);
        return  (LW_NULL);
    }
    
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_ISR_DEFER_PER_CPU > 0)
    return  (&_K_jobqIsrDefer[ulCPUId]);
#else
    return  (&_K_jobqIsrDefer[0]);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */ 
}                                                                       /*  LW_CFG_ISR_DEFER_PER_CPU    */
/*********************************************************************************************************
** ��������: API_InterDeferContext
** ��������: �Ƿ����ж��ӳٶ�������������
** �䡡��  : NONE
** �䡡��  : 0: �����ж�������Ҳ�����ж��ӳٶ�������������
**           1: ���ж�������
**           2: �ж��ӳٶ�������������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_InterDeferContext (VOID)
{
    INTREG          iregInterLevel;
    ULONG           ulNesting;
    PLW_CLASS_TCB   ptcbCur;

    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */

    ulNesting = LW_CPU_GET_CUR()->CPU_ulInterNesting;
    if (ulNesting) {
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
        return  (1);
    }

    LW_TCB_GET_CUR(ptcbCur);

    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */

#if (LW_CFG_SMP_EN > 0) && (LW_CFG_ISR_DEFER_PER_CPU > 0)
    {
        INT  i;

        LW_CPU_FOREACH_ACTIVE (i) {
            if (_K_ulIsrDefer[i] == ptcbCur->TCB_ulId) {
                return  (2);
            }
        }
    }

#else
    if (_K_ulIsrDefer[0] == ptcbCur->TCB_ulId) {
        return  (2);
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    return  (0);
}
/*********************************************************************************************************
** ��������: API_InterDeferJobAdd
** ��������: ���ж��ӳٴ�����м���һ������
** �䡡��  : pjobq         ����
**           pfunc         ������
**           pvArg         �������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_InterDeferJobAdd (PLW_JOB_QUEUE  pjobq, VOIDFUNCPTR  pfunc, PVOID  pvArg)
{
    if (!pjobq) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    return  (_jobQueueAdd(pjobq, pfunc, pvArg, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL));
}
/*********************************************************************************************************
** ��������: API_InterDeferJobDelete
** ��������: ���ж��ӳٴ������ɾ������
** �䡡��  : pjobq         ����
**           bMatchArg     �Ƿ���в���ƥ���ж�
**           pfunc         ������
**           pvArg         �������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_InterDeferJobDelete (PLW_JOB_QUEUE  pjobq, BOOL  bMatchArg, VOIDFUNCPTR  pfunc, PVOID  pvArg)
{
    if (!pjobq) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    _jobQueueDel(pjobq, (bMatchArg) ? 1 : 0,
                 pfunc, pvArg, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_ISR_DEFER_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
