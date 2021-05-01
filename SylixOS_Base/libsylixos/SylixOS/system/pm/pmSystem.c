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
** ��   ��   ��: pmSystem.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 19 ��
**
** ��        ��: ��Դ����ϵͳ�ӿ�.
**
** ע        ��: ���� API ��������, ��Ҫ���̹߳���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0
/*********************************************************************************************************
  ϵͳ״̬��ѯ
*********************************************************************************************************/
BOOL    _G_bPowerSavingMode = LW_FALSE;
/*********************************************************************************************************
** ��������: API_PowerMSuspend
** ��������: ϵͳ��������ģʽ.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  API_PowerMSuspend (VOID)
{
    PLW_LIST_LINE   plineTemp;
    PLW_PM_DEV      pmdev;
    
    __POWERM_LOCK();
    for (plineTemp  = _G_plinePMDev;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmdev = _LIST_ENTRY(plineTemp, LW_PM_DEV, PMD_lineManage);
        if (pmdev->PMD_pmdfunc && 
            pmdev->PMD_pmdfunc->PMDF_pfuncSuspend) {
            pmdev->PMD_pmdfunc->PMDF_pfuncSuspend(pmdev);
        }
    }
    __POWERM_UNLOCK();
    
    API_KernelSuspend();
}
/*********************************************************************************************************
** ��������: API_PowerMResume
** ��������: ϵͳ������ģʽ����.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  API_PowerMResume (VOID)
{
    PLW_LIST_LINE   plineTemp;
    PLW_PM_DEV      pmdev;
    
    __POWERM_LOCK();
    for (plineTemp  = _G_plinePMDev;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmdev = _LIST_ENTRY(plineTemp, LW_PM_DEV, PMD_lineManage);
        if (pmdev->PMD_pmdfunc && 
            pmdev->PMD_pmdfunc->PMDF_pfuncResume) {
            pmdev->PMD_pmdfunc->PMDF_pfuncResume(pmdev);
        }
    }
    __POWERM_UNLOCK();
    
    API_KernelResume();
}
/*********************************************************************************************************
** ��������: API_PowerMCpuSet
** ��������: ���� CPU �ڵ����.
** �䡡��  : ulNCpus       ���е� CPU ���� (���� >= 1)
**           uiPowerLevel  CPU ���е��ܼ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  API_PowerMCpuSet (ULONG  ulNCpus, UINT  uiPowerLevel)
{
    PLW_LIST_LINE   plineTemp;
    PLW_PM_DEV      pmdev;
    UINT            uiOldPowerLevel;

#if LW_CFG_SMP_EN > 0
    ULONG           i;
    ULONG           ulActCnt = 0;
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    if (ulNCpus == 0) {
        _ErrorHandle(EINVAL);
        return;
    }

    if (ulNCpus > LW_NCPUS) {
        ulNCpus = LW_NCPUS;
    }

#if LW_CFG_SMP_EN > 0
    LW_CPU_FOREACH_ACTIVE (i) {
        ulActCnt++;
    }
    if (ulActCnt > ulNCpus) {                                           /*  ��Ҫ�ر�һЩ CPU            */
#if LW_CFG_SMP_CPU_DOWN_EN > 0
        ULONG   ulDownCnt = ulActCnt - ulNCpus;
        LW_CPU_FOREACH_EXCEPT (i, 0) {
            if (API_CpuIsUp(i)) {
                API_CpuDown(i);
                ulDownCnt--;
            }
            if (ulDownCnt == 0) {
                break;
            }
        }
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */

    } else if (ulActCnt < ulNCpus) {                                    /*  ��Ҫ��һЩ CPU            */
        ULONG   ulUpCnt = ulNCpus - ulActCnt;
        LW_CPU_FOREACH_EXCEPT (i, 0) {
            if (!API_CpuIsUp(i)) {
                API_CpuUp(i);
                ulUpCnt--;
            }
            if (ulUpCnt == 0) {
                break;
            }
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    API_CpuPowerGet(&uiOldPowerLevel);
    if (uiOldPowerLevel != uiPowerLevel) {
        API_CpuPowerSet(uiPowerLevel);
        
        __POWERM_LOCK();
        for (plineTemp  = _G_plinePMDev;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            pmdev = _LIST_ENTRY(plineTemp, LW_PM_DEV, PMD_lineManage);
            if (pmdev->PMD_pmdfunc && 
                pmdev->PMD_pmdfunc->PMDF_pfuncCpuPower) {
                pmdev->PMD_pmdfunc->PMDF_pfuncCpuPower(pmdev);
            }
        }
        __POWERM_UNLOCK();
    }
}
/*********************************************************************************************************
** ��������: API_PowerMCpuGet
** ��������: ��ȡ CPU �ڵ����.
** �䡡��  : pulNCpus       ���е� CPU ����
**           puiPowerLevel  CPU ���е��ܼ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  API_PowerMCpuGet (ULONG  *pulNCpus, UINT  *puiPowerLevel)
{
#if LW_CFG_SMP_EN > 0
    ULONG   i;
    ULONG   ulActCnt = 0;

    if (pulNCpus) {
        LW_CPU_FOREACH_ACTIVE (i) {
            ulActCnt++;
        }
        *pulNCpus = ulActCnt;
    }
#else
    if (pulNCpus) {
        *pulNCpus = 1ul;
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    if (puiPowerLevel) {
        API_CpuPowerGet(puiPowerLevel);
    }
}
/*********************************************************************************************************
** ��������: API_PowerMSavingEnter
** ��������: ϵͳ����ʡ��ģʽ.
** �䡡��  : ulNCpus       �������е� CPU ���� (���� >= 1)
**           uiPowerLevel  CPU ���е��ܼ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  API_PowerMSavingEnter (ULONG  ulNCpus, UINT  uiPowerLevel)
{
    PLW_LIST_LINE   plineTemp;
    PLW_PM_DEV      pmdev;
    
    if (ulNCpus == 0) {
        _ErrorHandle(EINVAL);
        return;
    }
    
    __POWERM_LOCK();
    for (plineTemp  = _G_plinePMDev;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmdev = _LIST_ENTRY(plineTemp, LW_PM_DEV, PMD_lineManage);
        if (pmdev->PMD_pmdfunc && 
            pmdev->PMD_pmdfunc->PMDF_pfuncPowerSavingEnter) {
            pmdev->PMD_pmdfunc->PMDF_pfuncPowerSavingEnter(pmdev);
        }
    }
    _G_bPowerSavingMode = LW_TRUE;
    __POWERM_UNLOCK();
    
    API_PowerMCpuSet(ulNCpus, uiPowerLevel);
}
/*********************************************************************************************************
** ��������: API_PowerMSavingExit
** ��������: ϵͳ�˳�ʡ��ģʽ.
** �䡡��  : ulNCpus       ���е� CPU ���� (���� >= 1)
**           uiPowerLevel  CPU ���е��ܼ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  API_PowerMSavingExit (ULONG  ulNCpus, UINT  uiPowerLevel)
{
    PLW_LIST_LINE   plineTemp;
    PLW_PM_DEV      pmdev;
    
    if (ulNCpus == 0) {
        _ErrorHandle(EINVAL);
        return;
    }
    
    __POWERM_LOCK();
    for (plineTemp  = _G_plinePMDev;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmdev = _LIST_ENTRY(plineTemp, LW_PM_DEV, PMD_lineManage);
        if (pmdev->PMD_pmdfunc && 
            pmdev->PMD_pmdfunc->PMDF_pfuncPowerSavingExit) {
            pmdev->PMD_pmdfunc->PMDF_pfuncPowerSavingExit(pmdev);
        }
    }
    _G_bPowerSavingMode = LW_FALSE;
    __POWERM_UNLOCK();
    
    API_PowerMCpuSet(ulNCpus, uiPowerLevel);
}

#endif                                                                  /*  LW_CFG_POWERM_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
