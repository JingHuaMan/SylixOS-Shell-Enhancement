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
** ��   ��   ��: RmsGetName.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 11 ��
**
** ��        ��: ��þ��ȵ���������������

** BUG
2007.11.04  ���� _DebugHandle() ����.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ע�⣺
        ʹ�þ��ȵ���������������������ϵͳʱ�䣬���ǿ������� RTC ʱ�䡣
        
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: API_RmsGetName
** ��������: ��þ��ȵ���������������
** �䡡��  : 
**           ulId                          RMS ���
**           pcName                        ���ֻ�����
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

LW_API  
ULONG  API_RmsGetName (LW_OBJECT_HANDLE  ulId, PCHAR  pcName)
{
    REGISTER PLW_CLASS_RMS       prms;
    REGISTER UINT16              usIndex;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pcName) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_NULL);
        return  (ERROR_KERNEL_PNAME_NULL);
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
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Rms_Type_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_RMS_NULL);
        return  (ERROR_RMS_NULL);
    }
#else
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
#endif

    prms = &_K_rmsBuffer[usIndex];
    
    lib_strcpy(pcName, prms->RMS_cRmsName);
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
