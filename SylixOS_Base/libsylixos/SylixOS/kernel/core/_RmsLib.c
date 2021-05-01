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
** ��   ��   ��: _RmsInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 22 ��
**
** ��        ��: RMS �����⡣(��Щ�������������ں�ʱ������)

** BUG
2007.11.04  �� 0xFFFFFFFF ��Ϊ __ARCH_ULONG_MAX.
2007.11.04  �������������ִ��ʱ����㺯��.
2007.11.11  �� _RmsInitExpire() ���ֳ�ʱʱ,Ӧ�ü�¼��ǰ��ϵͳʱ��,�����Ϳ��Ա���һ�γ�ʱ,�Ժ󶼴���.
2008.03.29  ��صĵط�����ر��жϵĺ���.
2010.08.03  ��ʱ��Ĳ���������ص�������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _RmsActive
** ��������: ��һ�μ��� RMS ����ʼ���߳�ִ��ʱ����в��� (�����ں˲����жϺ󱻵���)
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

VOID  _RmsActive (PLW_CLASS_RMS  prms)
{
    prms->RMS_ucStatus = LW_RMS_ACTIVE;
    __KERNEL_TIME_GET_IGNIRQ(prms->RMS_ulTickSave, ULONG);
}
/*********************************************************************************************************
** ��������: _RmsGetExecTime
** ��������: ��������ִ�е�ʱ�� (�����ں˲����жϺ󱻵���)
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG   _RmsGetExecTime (PLW_CLASS_RMS  prms)
{
    REGISTER ULONG            ulThreadExecTime;
             ULONG            ulKernelTime;
    
    __KERNEL_TIME_GET_IGNIRQ(ulKernelTime, ULONG);
    ulThreadExecTime = (ulKernelTime >= prms->RMS_ulTickSave) ? 
                       (ulKernelTime -  prms->RMS_ulTickSave) :
                       (ulKernelTime + (__ARCH_ULONG_MAX - prms->RMS_ulTickSave) + 1);
                          
    return  (ulThreadExecTime);
}
/*********************************************************************************************************
** ��������: _RmsInitExpire
** ��������: ��ʼ����ʱ��ȴ� (�����ں˲����жϺ󱻵���)
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _RmsInitExpire (PLW_CLASS_RMS  prms, ULONG  ulPeriod, ULONG  *pulWaitTick)
{
             PLW_CLASS_TCB    ptcbCur;
    REGISTER ULONG            ulThreadExecTime;                         /*  �����߳�ִ��ʱ��            */
             ULONG            ulKernelTime;
    
    LW_TCB_GET_CUR(ptcbCur);
    
    __KERNEL_TIME_GET_IGNIRQ(ulKernelTime, ULONG);
    ulThreadExecTime = (ulKernelTime >= prms->RMS_ulTickSave) ? 
                       (ulKernelTime -  prms->RMS_ulTickSave) :
                       (ulKernelTime + (__ARCH_ULONG_MAX - prms->RMS_ulTickSave) + 1);
                          
    if (ulThreadExecTime > ulPeriod) {
        __KERNEL_TIME_GET_IGNIRQ(prms->RMS_ulTickSave, ULONG);          /*  ���¼�¼ϵͳʱ��            */
        return  (ERROR_RMS_TICK);
    }
    
    if (ulThreadExecTime == ulPeriod) {
        *pulWaitTick = 0;
        __KERNEL_TIME_GET_IGNIRQ(prms->RMS_ulTickSave, ULONG);          /*  ���¼�¼ϵͳʱ��            */
        return  (ERROR_NONE);
    }
    
    *pulWaitTick = ulPeriod - ulThreadExecTime;                         /*  ����˯��ʱ��                */
    
    prms->RMS_ucStatus   = LW_RMS_EXPIRED;                              /*  �ı�״̬                    */
    prms->RMS_ptcbOwner  = ptcbCur;                                     /*  ��¼��ǰTCB                 */
    
    __KERNEL_TIME_GET_IGNIRQ(prms->RMS_ulTickNext, ULONG);
    prms->RMS_ulTickNext += *pulWaitTick;                               /*  �����´ε�ʱʱ��            */
                                                                        /*  ��Ȼ���                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _RmsEndExpire
** ��������: һ��������ϣ��ȴ���һ������ (�����ں˺󱻵���)
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _RmsEndExpire (PLW_CLASS_RMS  prms)
{
    if (prms->RMS_ucStatus != LW_RMS_EXPIRED) {                         /*  �� cancel �� ɾ����         */
        return  (ERROR_RMS_WAS_CHANGED);                                /*  ���ı���                    */
    }
    
    prms->RMS_ucStatus = LW_RMS_ACTIVE;                                 /*  �ı�״̬                    */
    __KERNEL_TIME_GET(prms->RMS_ulTickSave, ULONG);                     /*  ���¼�¼ϵͳʱ��            */
    
    if (prms->RMS_ulTickNext != prms->RMS_ulTickSave) {                 /*  �Ƿ� TIME OUT               */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    
    } else {
        return  (ERROR_NONE);
    }
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
