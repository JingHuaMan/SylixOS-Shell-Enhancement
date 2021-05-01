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
** ��   ��   ��: pmAdapter.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 19 ��
**
** ��        ��: ��Դ����������.
*********************************************************************************************************/

#ifndef __PMADAPTER_H
#define __PMADAPTER_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0

#ifdef  __POWERM_MAIN_FILE
       LW_OBJECT_HANDLE     _G_ulPowerMLock;
       LW_LIST_LINE_HEADER  _G_plinePMAdapter;
#else
extern LW_OBJECT_HANDLE     _G_ulPowerMLock;
extern LW_LIST_LINE_HEADER  _G_plinePMAdapter;
#endif                                                                  /*  __POWERM_MAIN_FILE          */

#define __POWERM_LOCK()     API_SemaphoreMPend(_G_ulPowerMLock, LW_OPTION_WAIT_INFINITE)
#define __POWERM_UNLOCK()   API_SemaphoreMPost(_G_ulPowerMLock)

/*********************************************************************************************************
  ����������ýӿ�
*********************************************************************************************************/

LW_API PLW_PM_ADAPTER  API_PowerMAdapterCreate(CPCHAR  pcName, UINT  uiMaxChan, PLW_PMA_FUNCS  pmafuncs);
LW_API INT             API_PowerMAdapterDelete(PLW_PM_ADAPTER  pmadapter);
LW_API PLW_PM_ADAPTER  API_PowerMAdapterFind(CPCHAR  pcName);

#define pmAdapterCreate     API_PowerMAdapterCreate
#define pmAdapterDelete     API_PowerMAdapterDelete
#define pmAdapterFind       API_PowerMAdapterFind

#endif                                                                  /*  LW_CFG_POWERM_EN            */
#endif                                                                  /*  __PMADAPTER_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
