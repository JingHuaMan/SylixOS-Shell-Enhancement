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
** ��   ��   ��: RmsCreate.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 11 ��
**
** ��        ��: ����һ�����ȵ���������

** BUG
2007.11.04  ���� _DebugHandle() ����.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.05.31  ʹ�� __KERNEL_MODE_...().
2011.07.29  ������󴴽�/���ٻص�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ע�⣺
        ʹ�þ��ȵ���������������������ϵͳʱ�䣬���ǿ������� RTC ʱ�䡣
        
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: API_RmsCreate
** ��������: ����һ�����ȵ���������
** �䡡��  : 
**           pcName                        ����
**           ulOption                      ѡ��
**           pulId                         Id ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

LW_API  
LW_OBJECT_HANDLE  API_RmsCreate (CPCHAR             pcName,
                                 ULONG              ulOption,
                                 LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_RMS  prms;
    REGISTER ULONG          ulIdTemp;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        prms = _Allocate_Rms_Object();
    );                                                                  /*  �˳��ں�                    */
    
    if (!prms) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to build a RMS.\r\n");
        _ErrorHandle(ERROR_RMS_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    prms->RMS_ucType     = LW_RMS_USED;
    prms->RMS_ucStatus   = LW_RMS_INACTIVE;
    prms->RMS_ulTickNext = 0ul;
    prms->RMS_ulTickSave = 0ul;
    
    if (pcName) {                                                       /*  ��������                    */
        lib_strcpy(prms->RMS_cRmsName, pcName);
    } else {
        prms->RMS_cRmsName[0] = PX_EOS;                                 /*  �������                    */
    }
    
    ulIdTemp = _MakeObjectId(_OBJECT_RMS, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             prms->RMS_usIndex);                        /*  �������� id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "RMS \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
