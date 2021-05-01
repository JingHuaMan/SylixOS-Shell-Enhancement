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
** ��   ��   ��: pmDev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 19 ��
**
** ��        ��: ��Դ�����豸�ӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0
/*********************************************************************************************************
** ��������: API_PowerMDevInit
** ��������: ��ʼ����Դ�����豸�ڵ�
** �䡡��  : pmdev         ��Դ����, �豸�ڵ�
**           pmadapter     ��Դ����������
**           uiChan        ͨ����
**           pmdfunc       �豸�ڵ��������
** �䡡��  : ERROR ok OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevInit (PLW_PM_DEV  pmdev,  PLW_PM_ADAPTER  pmadapter, 
                        UINT        uiChan, PLW_PMD_FUNCS   pmdfunc)
{
    if (!pmdev || !pmadapter) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (uiChan >= pmadapter->PMA_uiMaxChan) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    pmdev->PMD_pmadapter = pmadapter;
    pmdev->PMD_uiChannel = uiChan;
    pmdev->PMD_uiStatus  = LW_PMD_STAT_NOR;
    pmdev->PMD_pmdfunc   = pmdfunc;
    lib_bzero(&pmdev->PMD_wunTimer, sizeof(LW_CLASS_WAKEUP_NODE));
    
    __POWERM_LOCK();
    _List_Line_Add_Ahead(&pmdev->PMD_lineManage, &_G_plinePMDev);
    __POWERM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PowerMDevTerm
** ��������: ������Դ�����豸�ڵ�
** �䡡��  : pmdev         ��Դ����, �豸�ڵ�
** �䡡��  : ERROR ok OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevTerm (PLW_PM_DEV  pmdev)
{
    if (!pmdev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __POWERM_LOCK();
    _List_Line_Del(&pmdev->PMD_lineManage, &_G_plinePMDev);
    __POWERM_UNLOCK();

    return  (API_PowerMDevWatchDogOff(pmdev));
}
/*********************************************************************************************************
** ��������: API_PowerMDevOn
** ��������: ��Դ����, �豸�ڵ��һ�δ���Ҫ���ô˺���
** �䡡��  : pmdev         ��Դ����, �豸�ڵ�
** �䡡��  : ERROR ok OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevOn (PLW_PM_DEV  pmdev)
{
    PLW_PM_ADAPTER  pmadapter;
    INT             iRet = PX_ERROR;
    
    if (!pmdev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmadapter = pmdev->PMD_pmadapter;
    if (pmadapter && 
        pmadapter->PMA_pmafunc &&
        pmadapter->PMA_pmafunc->PMAF_pfuncOn) {
        iRet = pmadapter->PMA_pmafunc->PMAF_pfuncOn(pmadapter, pmdev);
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PowerMDevOff
** ��������: ��Դ����, �豸�ڵ����һ�ιر���Ҫ���ô˺���
** �䡡��  : pmdev         ��Դ����, �豸�ڵ�
** �䡡��  : ERROR ok OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevOff (PLW_PM_DEV  pmdev)
{
    PLW_PM_ADAPTER  pmadapter;
    INT             iRet = PX_ERROR;
    
    if (!pmdev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmadapter = pmdev->PMD_pmadapter;
    if (pmadapter && 
        pmadapter->PMA_pmafunc &&
        pmadapter->PMA_pmafunc->PMAF_pfuncOff) {
        iRet = pmadapter->PMA_pmafunc->PMAF_pfuncOff(pmadapter, pmdev);
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_POWERM_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
