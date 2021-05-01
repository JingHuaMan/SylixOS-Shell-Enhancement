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
** ��   ��   ��: SemaphorePend.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 01 �� 16 ��
**
** ��        ��: �������ź������ݵȴ�����. Ϊ�˷�����ֲ��������ϵͳӦ���������д.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
ulTimeout ȡֵ��
    
    LW_OPTION_NOT_WAIT                       �����еȴ�
    LW_OPTION_WAIT_A_TICK                    �ȴ�һ��ϵͳʱ��
    LW_OPTION_WAIT_A_SECOND                  �ȴ�һ��
    LW_OPTION_WAIT_INFINITE                  ��Զ�ȴ���ֱ������Ϊֹ
    
ע�⣺�� ulTimeout == LW_OPTION_NOT_WAIT ʱ API_SemaphoreCPend ���ǲ�ͬ�� API_SemaphoreBTryPend

      API_SemaphoreBTryPend �������ж���ʹ�ã��� API_SemaphoreBPend ����
    
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: API_SemaphorePend
** ��������: �ȴ��ź���
** �䡡��  : 
**           ulId            �¼����
**           ulTimeout       �ȴ�ʱ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_SEMCBM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphorePend (LW_OBJECT_HANDLE  ulId, ULONG  ulTimeout)
{
    REGISTER ULONG      ulObjectClass;
    REGISTER ULONG      ulErrorCode;
    
    ulObjectClass = _ObjectGetClass(ulId);                              /*  ����ź������������        */
    
    switch (ulObjectClass) {
    
#if LW_CFG_SEMB_EN > 0
    case _OBJECT_SEM_B:
        ulErrorCode = API_SemaphoreBPend(ulId, ulTimeout);
        break;
#endif                                                                  /*  LW_CFG_SEMB_EN > 0          */

#if LW_CFG_SEMC_EN > 0
    case _OBJECT_SEM_C:
        ulErrorCode = API_SemaphoreCPend(ulId, ulTimeout);
        break;
#endif                                                                  /*  LW_CFG_SEMC_EN > 0          */

#if LW_CFG_SEMM_EN > 0
    case _OBJECT_SEM_M:
        ulErrorCode = API_SemaphoreMPend(ulId, ulTimeout);
        break;
#endif                                                                  /*  LW_CFG_SEMM_EN > 0          */
    
    default:
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  ������ʹ���                */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    return  (ulErrorCode);
}

#endif                                                                  /*  (LW_CFG_SEMCBM_EN > 0) &&   */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
