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
** ��   ��   ��: pmAdapter.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 19 ��
**
** ��        ��: ��Դ����������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#define  __POWERM_MAIN_FILE
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0
/*********************************************************************************************************
** ��������: API_PowerMAdapterCreate
** ��������: ����һ����Դ����������
** �䡡��  : pcName        ��Դ����������������
**           uiMaxChan     ��Դ�������������ͨ����
**           pmafuncs      ��Դ������������������
** �䡡��  : ��Դ����������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_PM_ADAPTER  API_PowerMAdapterCreate (CPCHAR  pcName, UINT  uiMaxChan, PLW_PMA_FUNCS  pmafuncs)
{
    PLW_PM_ADAPTER  pmadapter;
    
    if (!pcName || !uiMaxChan || !pmafuncs) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    pmadapter = (PLW_PM_ADAPTER)__SHEAP_ALLOC(sizeof(LW_PM_ADAPTER) + lib_strlen(pcName));
    if (pmadapter == LW_NULL) {
        _ErrorHandle(ERROR_POWERM_FULL);
        return  (LW_NULL);
    }
    
    pmadapter->PMA_uiMaxChan = uiMaxChan;
    pmadapter->PMA_pmafunc   = pmafuncs;
    lib_strcpy(pmadapter->PMA_cName, pcName);
    
    __POWERM_LOCK();
    _List_Line_Add_Ahead(&pmadapter->PMA_lineManage, &_G_plinePMAdapter);
    __POWERM_UNLOCK();
    
    return  (pmadapter);
}
/*********************************************************************************************************
** ��������: API_PowerMAdapterDelete
** ��������: ɾ��һ����Դ���������� (���Ƽ�ʹ�ô˺���)
** �䡡��  : pmadapter     ��Դ����������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_PowerMAdapterDelete (PLW_PM_ADAPTER  pmadapter)
{
    if (!pmadapter) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __POWERM_LOCK();
    _List_Line_Del(&pmadapter->PMA_lineManage, &_G_plinePMAdapter);
    __POWERM_UNLOCK();
    
    __SHEAP_FREE(pmadapter);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PowerMAdapterFind
** ��������: ��ѯһ����Դ����������
** �䡡��  : pcName        ��Դ����������������
** �䡡��  : ��Դ����������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_PM_ADAPTER  API_PowerMAdapterFind (CPCHAR  pcName)
{
    PLW_LIST_LINE   plineTemp;
    PLW_PM_ADAPTER  pmadapter;
    
    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    __POWERM_LOCK();
    for (plineTemp  = _G_plinePMAdapter;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmadapter = _LIST_ENTRY(plineTemp, LW_PM_ADAPTER, PMA_lineManage);
        if (lib_strcmp(pmadapter->PMA_cName, pcName) == 0) {
            break;
        }
    }
    __POWERM_UNLOCK();
    
    if (plineTemp) {
        return  (pmadapter);
    
    } else {
        return  (LW_NULL);
    }
}

#endif                                                                  /*  LW_CFG_POWERM_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
