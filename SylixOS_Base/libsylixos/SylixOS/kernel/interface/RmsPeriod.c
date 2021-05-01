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
** ��   ��   ��: RmsPeriod.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 11 ��
**
** ��        ��: ָ�����ȵ�����������ʼ���̶����ڹ���

** BUG
2007.11.04  ���� _DebugHandle() ����.
2008.01.20  �Ľ�������ȫ����Ϊ������.
2008.03.29  �����µ� wake up ���ƵĴ���.
2008.03.30  ʹ���µľ���������.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.05.31  ʹ�� __KERNEL_MODE_...().
2010.08.03  ֧�� SMP ���.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ע�⣺
        ʹ�þ��ȵ���������������������ϵͳʱ�䣬���ǿ������� RTC ʱ�䡣
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: API_RmsPeriod
** ��������: ָ�����ȵ�����������ʼ���̶����ڹ���
** �䡡��  : 
**           ulId                          RMS ���
**           ulPeriod                      �����ִ������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

LW_API  
ULONG  API_RmsPeriod (LW_OBJECT_HANDLE  ulId, ULONG  ulPeriod)
{
             INTREG                    iregInterLevel;
             
             PLW_CLASS_TCB             ptcbCur;
	REGISTER PLW_CLASS_PCB             ppcb;

    REGISTER PLW_CLASS_RMS             prms;
    REGISTER UINT16                    usIndex;
    
    REGISTER ULONG                     ulErrorCode;
             ULONG                     ulWaitTime;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!ulPeriod) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulPeriod invalidate.\r\n");
        _ErrorHandle(ERROR_RMS_TICK);
        return  (ERROR_RMS_TICK);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_RMS)) {                           /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Rms_Index_Invalid(usIndex)) {                                  /*  �������������              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    if (_Rms_Type_Invalid(usIndex)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_RMS_NULL);
        return  (ERROR_RMS_NULL);
    }
#else
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
#endif

    prms = &_K_rmsBuffer[usIndex];
    
    switch (prms->RMS_ucStatus) {                                       /*  ״̬��                      */
    
    case LW_RMS_INACTIVE:
        _RmsActive(prms);                                               /*  ���� RMS                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        return  (ERROR_NONE);
        
    case LW_RMS_ACTIVE:                                                 /*  �ѽ���ʼ�����              */
        ulErrorCode = _RmsInitExpire(prms, ulPeriod, &ulWaitTime);      /*  ��ʼ�������ڡ�����          */
        if (ulErrorCode) {                                              /*  ��������                    */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
            _ErrorHandle(ulErrorCode);
            return  (ulErrorCode);
        }
        
        if (!ulWaitTime) {                                              /*  ʱ��ոպã�                */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
            return  (ERROR_NONE);
        }
        
        /*
         *  ��ǰ�߳̿�ʼ˯��
         */
        ppcb = _GetPcb(ptcbCur);
        __DEL_FROM_READY_RING(ptcbCur, ppcb);                           /*  �Ӿ�������ɾ��              */

        ptcbCur->TCB_ulDelay = ulWaitTime;
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  ����ȴ�ɨ����              */
        
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�, ��������          */
        
        /*
         *  ��ǰ�߳�˯�߽���
         */
         __KERNEL_MODE_PROC(
            ulErrorCode = _RmsEndExpire(prms);                          /*  �����ڵ� RMS              */
         );

        _ErrorHandle(ulErrorCode);
        return  (ulErrorCode);
        
    default:                                                            /*  ת̬������                  */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _ErrorHandle(ERROR_RMS_NULL);
        return  (ERROR_RMS_NULL);
    }
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
