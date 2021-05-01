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
** ��   ��   ��: RmsDelete.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 25 ��
**
** ��        ��: ɾ�����ȵ���������

** BUG
2007.11.04  ���� _DebugHandle() ����.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2011.07.29  ������󴴽�/���ٻص�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ע�⣺
        ʹ�þ��ȵ���������������������ϵͳʱ�䣬���ǿ������� RTC ʱ�䡣
        
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: API_RmsDeleteEx
** ��������: ɾ�����ȵ���������
** �䡡��  : 
**           pulId                         RMS ���ָ��
**           bForce                        ǿ��ɾ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

LW_API  
ULONG  API_RmsDeleteEx (LW_OBJECT_HANDLE   *pulId, BOOL  bForce)
{
             INTREG             iregInterLevel;
    REGISTER PLW_CLASS_RMS      prms;
    REGISTER UINT16             usIndex;
    REGISTER LW_OBJECT_HANDLE   ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
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
    
    if (!bForce) {
        if (prms->RMS_ucStatus == LW_RMS_EXPIRED) {                     /*  ״̬����                    */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں�                    */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS status error.\r\n");
            _ErrorHandle(ERROR_RMS_STATUS);
            return  (ERROR_RMS_STATUS);
        }
    }
    
    prms->RMS_ucType = LW_RMS_UNUSED;                                   /*  û��ʹ��                    */
    
    _ObjectCloseId(pulId);
    
    _Free_Rms_Object(prms);
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�                    */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "RMS \"%s\" has been delete.\r\n", prms->RMS_cRmsName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_RmsDelete
** ��������: ɾ�����ȵ���������
** �䡡��  : 
**           pulId                         RMS ���ָ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_RmsDelete (LW_OBJECT_HANDLE   *pulId)
{
    return  (API_RmsDeleteEx(pulId, LW_FALSE));
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
