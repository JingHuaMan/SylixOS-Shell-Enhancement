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
** ��   ��   ��: SemaphoreFlush.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 01 �� 16 ��
**
** ��        ��: �ͷŵȴ��ź����������߳�. Ϊ�˷�����ֲ��������ϵͳӦ���������д.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_SemaphoreFlush
** ��������: �ͷŵȴ��ź����������߳�
** �䡡��  : 
**           ulId                   �¼����
**           pulThreadUnblockNum    ���������߳�����   ����ΪNULL
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_SEMCBM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreFlush (LW_OBJECT_HANDLE  ulId, ULONG  *pulThreadUnblockNum)
{
    REGISTER ULONG      ulObjectClass;
    REGISTER ULONG      ulErrorCode;
    
    ulObjectClass = _ObjectGetClass(ulId);                              /*  ����ź������������        */
    
    switch (ulObjectClass) {
    
#if LW_CFG_SEMB_EN > 0
    case _OBJECT_SEM_B:
        ulErrorCode = API_SemaphoreBFlush(ulId, pulThreadUnblockNum);
        break;
#endif                                                                  /*  LW_CFG_SEMB_EN > 0          */

#if LW_CFG_SEMC_EN > 0
    case _OBJECT_SEM_C:
        ulErrorCode = API_SemaphoreCFlush(ulId, pulThreadUnblockNum);
        break;
#endif                                                                  /*  LW_CFG_SEMC_EN > 0          */

    default:
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  ������ʹ���                */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }

    return  (ulErrorCode);
}

#endif                                                                  /*  (LW_CFG_SEMCBM_EN  > 0) &&  */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
