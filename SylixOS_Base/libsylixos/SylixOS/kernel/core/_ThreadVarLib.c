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
** ��   ��   ��: _ThreadVarLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: �߳��ڲ�ȫ�ֱ����ָ�
**

** BUG
2007.03.20  �� FREE ����ʱʹ�� Lock() �� Unlok() ��߰�ȫ��
2007.11.13  ʹ���������������������ȫ��װ.
2007.11.18  ����ע��.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.05.31  ʹ�� __KERNEL_MODE_...().
2012.07.04  �ϲ� _ThreadVarSwitch() �������˴�.
2014.07.22  ���� _ThreadVarSave() ��Ϊ CPU ֹͣʱ�������е��߳�˽�б���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _ThreadVarDelete
** ��������: �߳��ڲ�ȫ�ֱ����ָ�
** �䡡��  : ptcb      �߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)

VOID  _ThreadVarDelete (PLW_CLASS_TCB  ptcb)
{
    REGISTER PLW_LIST_LINE          plineVar;
    REGISTER PLW_CLASS_TCB          ptcbCur;
    REGISTER PLW_CLASS_THREADVAR    pthreadvar;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (ptcb != ptcbCur) {                                              /*  ���ǵ�ǰ�߳�ɾ��            */
        for (plineVar  = ptcb->TCB_plinePrivateVars;                    /*  ����߳�˽�б���            */
             plineVar != LW_NULL; 
             plineVar  = _list_line_get_next(plineVar)) {
            
            pthreadvar = _LIST_ENTRY(plineVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
            
            __KERNEL_MODE_PROC(
                _Free_ThreadVar_Object(pthreadvar);                     /*  �ͷű�����                  */
            );
        }
    } else {                                                            /*  ��ǰ�߳�ɾ��                */
        for (plineVar  = ptcb->TCB_plinePrivateVars;                    /*  ����߳�˽�б���            */
             plineVar != LW_NULL; 
             plineVar  = _list_line_get_next(plineVar)) {
            
            pthreadvar = _LIST_ENTRY(plineVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
            *pthreadvar->PRIVATEVAR_pulAddress = pthreadvar->PRIVATEVAR_ulValueSave;
                                                                        /*  �ָ��ⲿ������ֵ            */
            __KERNEL_MODE_PROC(
                _Free_ThreadVar_Object(pthreadvar);                     /*  �ͷű�����                  */
            );
        }
    }
}
/*********************************************************************************************************
** ��������: _ThreadVarSwitch.
** ��������: �߳��ڲ�ȫ�ֱ����л�
** �䡡��  : ptcbOld       �������������߳�
**           ptcbNew       ����������߳�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadVarSwitch (PLW_CLASS_TCB  ptcbOld, PLW_CLASS_TCB  ptcbNew)
{
    REGISTER PLW_LIST_LINE          plineOldVar;
    REGISTER PLW_LIST_LINE          plineNewVar;
    REGISTER PLW_CLASS_THREADVAR    pthreadvar;
    
    REGISTER ULONG  ulSwitch;
    
    if (_Thread_Exist(ptcbOld)) {                                       /*  ���߳̿��ܲ�������          */
        for (plineOldVar  = ptcbOld->TCB_plinePrivateVars;              /*  ��þ��߳�˽�б���          */
             plineOldVar != LW_NULL; 
             plineOldVar  = _list_line_get_next(plineOldVar)) {
        
            pthreadvar = _LIST_ENTRY(plineOldVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
            ulSwitch   = pthreadvar->PRIVATEVAR_ulValueSave;
            pthreadvar->PRIVATEVAR_ulValueSave = *pthreadvar->PRIVATEVAR_pulAddress;
            *pthreadvar->PRIVATEVAR_pulAddress = ulSwitch;
        }
    }
    
    for (plineNewVar  = ptcbNew->TCB_plinePrivateVars;
         plineNewVar != LW_NULL; 
         plineNewVar  = _list_line_get_next(plineNewVar)) {             /*  ������߳�˽�б���          */
         
        pthreadvar = _LIST_ENTRY(plineNewVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
        ulSwitch   = pthreadvar->PRIVATEVAR_ulValueSave;
        pthreadvar->PRIVATEVAR_ulValueSave = *pthreadvar->PRIVATEVAR_pulAddress;
        *pthreadvar->PRIVATEVAR_pulAddress = ulSwitch;
    }
}
/*********************************************************************************************************
** ��������: _ThreadVarSave.
** ��������: �߳��ڲ�ȫ�ֱ����л�
** �䡡��  : ptcbCur       ��ǰ����ִ�е��߳�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

VOID  _ThreadVarSave (PLW_CLASS_TCB  ptcbCur)
{
    REGISTER PLW_LIST_LINE          plineCurVar;
    REGISTER PLW_CLASS_THREADVAR    pthreadvar;
    
    REGISTER ULONG  ulSwitch;
    
    if (_Thread_Exist(ptcbCur)) {                                       /*  ���߳̿��ܲ�������          */
        for (plineCurVar  = ptcbCur->TCB_plinePrivateVars;              /*  ��þ��߳�˽�б���          */
             plineCurVar != LW_NULL; 
             plineCurVar  = _list_line_get_next(plineCurVar)) {
        
            pthreadvar = _LIST_ENTRY(plineCurVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
            ulSwitch   = pthreadvar->PRIVATEVAR_ulValueSave;
            pthreadvar->PRIVATEVAR_ulValueSave = *pthreadvar->PRIVATEVAR_pulAddress;
            *pthreadvar->PRIVATEVAR_pulAddress = ulSwitch;
        }
    }
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
                                                                        /*  (LW_CFG_THREAD_PRIVATE_VA...*/
                                                                        /*  (LW_CFG_MAX_THREAD_GLB_VA...*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
