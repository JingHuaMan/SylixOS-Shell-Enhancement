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
** ��   ��   ��: ioLicense.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 09 �� 25 ��
**
** ��        ��: ��ʾϵͳ IO �����������֤����ϵͳ
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
** ��������: API_IoSetDrvLicense
** ��������: ����ָ��������������֤��Ϣ
** �䡡��  : 
**           iDrvNum                       ������ʽ�� (���豸��)
**           pcLicense                     ���֤�ַ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
********************************************************************************************************/
LW_API  
INT   API_IoSetDrvLicense (INT  iDrvNum, PCHAR  pcLicense)
{
    REGISTER LW_DRV_LICENSE     *pdrvlic;
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS ||
        iDrvNum <  0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
        
    if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pdrvlic = &_S_deventryTbl[iDrvNum].DEVENTRY_drvlicLicense;
    
    pdrvlic->DRVLIC_pcLicense = pcLicense;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IoGetDrvLicense
** ��������: ���ָ��������������֤��Ϣ
** �䡡��  : 
**           iDrvNum                       ������ʽ�� (���豸��)
** �䡡��  : ���֤�ַ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
********************************************************************************************************/
LW_API  
PCHAR   API_IoGetDrvLicense (INT  iDrvNum)
{
    REGISTER LW_DRV_LICENSE     *pdrvlic;
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS ||
        iDrvNum <  0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (LW_NULL);
    }
        
    if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (LW_NULL);
    }
    
    pdrvlic = &_S_deventryTbl[iDrvNum].DEVENTRY_drvlicLicense;
    
    return  (pdrvlic->DRVLIC_pcLicense);
}
/*********************************************************************************************************
** ��������: API_IoSetDrvAuthor
** ��������: ����ָ�����������������Ϣ
** �䡡��  : 
**           iDrvNum                       ������ʽ�� (���豸��)
**           pcAuthor                      ������Ϣ�ַ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
********************************************************************************************************/
LW_API  
INT   API_IoSetDrvAuthor (INT  iDrvNum, PCHAR  pcAuthor)
{
    REGISTER LW_DRV_LICENSE     *pdrvlic;
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS ||
        iDrvNum <  0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
        
    if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pdrvlic = &_S_deventryTbl[iDrvNum].DEVENTRY_drvlicLicense;
    
    pdrvlic->DRVLIC_pcAuthor = pcAuthor;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IoGetDrvAuthor
** ��������: ���ָ�����������������Ϣ
** �䡡��  : 
**           iDrvNum                       ������ʽ�� (���豸��)
** �䡡��  : ������Ϣ�ַ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
********************************************************************************************************/
LW_API  
PCHAR   API_IoGetDrvAuthor (INT  iDrvNum)
{
    REGISTER LW_DRV_LICENSE     *pdrvlic;
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS ||
        iDrvNum <  0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (LW_NULL);
    }
        
    if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (LW_NULL);
    }
    
    pdrvlic = &_S_deventryTbl[iDrvNum].DEVENTRY_drvlicLicense;
    
    return  (pdrvlic->DRVLIC_pcAuthor);
}
/*********************************************************************************************************
** ��������: API_IoSetDrvDescroption
** ��������: ����ָ�����������������Ϣ
** �䡡��  : 
**           iDrvNum                       ������ʽ�� (���豸��)
**           pcDescription                 ������Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
********************************************************************************************************/
LW_API  
INT   API_IoSetDrvDescription (INT  iDrvNum, PCHAR  pcDescription)
{
    REGISTER LW_DRV_LICENSE     *pdrvlic;
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS ||
        iDrvNum <  0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
        
    if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pdrvlic = &_S_deventryTbl[iDrvNum].DEVENTRY_drvlicLicense;
    
    pdrvlic->DRVLIC_pcDescription = pcDescription;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IoGetDrvDescroption
** ��������: ���ָ�����������������Ϣ
** �䡡��  : 
**           iDrvNum                       ������ʽ�� (���豸��)
** �䡡��  : ������Ϣ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
********************************************************************************************************/
LW_API  
PCHAR   API_IoGetDrvDescription (INT  iDrvNum)
{
    REGISTER LW_DRV_LICENSE     *pdrvlic;
    
    if (iDrvNum >= LW_CFG_MAX_DRIVERS ||
        iDrvNum <  0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (LW_NULL);
    }
        
    if (_S_deventryTbl[iDrvNum].DEVENTRY_bInUse == LW_FALSE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (LW_NULL);
    }
    
    pdrvlic = &_S_deventryTbl[iDrvNum].DEVENTRY_drvlicLicense;
    
    return  (pdrvlic->DRVLIC_pcDescription);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
