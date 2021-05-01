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
** ��   ��   ��: CoroutineResume.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 06 �� 19 ��
**
** ��        ��: ����Э�̹����(Э����һ���������Ĳ���ִ�е�λ). 
                 ��ǰЭ�������ó� CPU. ������ָ����Э��.
                 ע��! ���õ�Э��һ���ǵ�ǰ�̴߳�����Э��.
                 
** BUG:
2010.08.03  ���ע��.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_COROUTINE_EN > 0
/*********************************************************************************************************
** ��������: API_CoroutineResume
** ��������: ��ǰЭ�������ó� CPU. ������ָ����Э��.
** �䡡��  : pvCrcb            ��Ҫ���ȵ�Э�̾��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_CoroutineResume (PVOID  pvCrcb)
{
             INTREG                 iregInterLevel;
             
             PLW_CLASS_CPU          pcpuCur;
             PLW_CLASS_TCB          ptcbCur;
    REGISTER PLW_CLASS_COROUTINE    pcrcbNow;
    REGISTER PLW_CLASS_COROUTINE    pcrcbNext = (PLW_CLASS_COROUTINE)pvCrcb;

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }

    if (!pcrcbNext) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine handle invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (pcrcbNext->COROUTINE_ulThread != ptcbCur->TCB_ulId) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine handle invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    pcrcbNow = _LIST_ENTRY(ptcbCur->TCB_pringCoroutineHeader,
                           LW_CLASS_COROUTINE,
                           COROUTINE_ringRoutine);                      /*  ��ǰЭ��                    */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_COROUTINE, MONITOR_EVENT_COROUTINE_RESUME, 
                      ptcbCur->TCB_ulId, pcrcbNext, LW_NULL);
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    ptcbCur->TCB_pringCoroutineHeader = &pcrcbNext->COROUTINE_ringRoutine;
    
    pcpuCur = LW_CPU_GET_CUR();
    pcpuCur->CPU_pcrcbCur  = pcrcbNow;
    pcpuCur->CPU_pcrcbNext = pcrcbNext;
    archCrtCtxSwitch(pcpuCur);                                          /*  Э���л�                    */
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
    
    _CoroutineReclaim(ptcbCur);                                         /*  ���Ի����Ѿ�ɾ����Э��      */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
