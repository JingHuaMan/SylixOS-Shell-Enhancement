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
** ��   ��   ��: selectList.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 11 �� 07 ��
**
** ��        ��:  IO ϵͳ select ��ϵͳ WAKEUP �����������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
#include "select.h"
/*********************************************************************************************************
  WAKE UP LIST ������
*********************************************************************************************************/
#define __WAKEUPLIST_LOCK_OPTION        (LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |  \
                                         LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL)
/*********************************************************************************************************
** ��������: API_SelWakeupListInit
** ��������: ��ʼ�� WAKEUP LIST ������ƽṹ
** �䡡��  : pselwulList        select wake up list ���ƽṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeupListInit (PLW_SEL_WAKEUPLIST  pselwulList)
{
    if (!pselwulList) {                                                 /*  ���ָ��                    */
        return;
    }
    
    pselwulList->SELWUL_hListLock = API_SemaphoreMCreate("sellist_lock", 
                                    LW_PRIO_DEF_CEILING, __WAKEUPLIST_LOCK_OPTION, 
                                    LW_NULL);                           /*  �������ź���                */
                                    
    pselwulList->SELWUL_ulWakeCounter = 0;                              /*  û�еȴ����߳�              */
    pselwulList->SELWUL_plineHeader   = LW_NULL;                        /*  ��ʼ������ͷ                */
}
/*********************************************************************************************************
** ��������: API_SelWakeupListTerm
** ��������: ��ָֹ���� WAKEUP LIST ������ƽṹ
** �䡡��  : pselwulList        select wake up list ���ƽṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �������ȼ� 0 �� exc �������������, ���Բ���Ҫ��� exc �������е�����, ���ϵͳ���ܻ��뱾����
             ͬʱִ��, ����ȷ����ȫ�ڼ�, ��Ҫɾ���������ڲ����ڱ� select list �Ĳ���.
             
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeupListTerm (PLW_SEL_WAKEUPLIST  pselwulList)
{
    if (!pselwulList) {                                                 /*  ���ָ��                    */
        return;
    }
    
#if LW_CFG_SMP_EN > 0
    _excJobDel(2, (VOIDFUNCPTR)API_SelWakeupAll, (PVOID)pselwulList,
               0, 0, 0, 0, 0);
#endif                                                                  /*  LW_CFG_SMP_EN               */
               
    API_SelWakeupTerm(pselwulList);                                     /*  ɾ���ڲ�����Ľڵ�          */
    
    API_SemaphoreMDelete(&pselwulList->SELWUL_hListLock);               /*  ɾ���ź���                  */
}
/*********************************************************************************************************
** ��������: API_SelWakeupListLen
** ��������: ���ָ���� WAKEUP LIST ������ƽṹ�ȴ��̵߳�����
** �䡡��  : pselwulList        select wake up list ���ƽṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
UINT    API_SelWakeupListLen (PLW_SEL_WAKEUPLIST  pselwulList)
{
    if (!pselwulList) {                                                 /*  ���ָ��                    */
        return  (0);
    }
    
    return  (pselwulList->SELWUL_ulWakeCounter);                        /*  ���صȴ��̵߳�����          */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
