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
** ��   ��   ��: spiLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 11 �� 06 ��
**
** ��        ��: spi �豸������.

** BUG:
2011.04.02  ��������ߵ��������ͷ� API. ��Ƭѡ�Ĳ��������豸������������ɶ�������������.
            SPI ���߻������ʹ�� mutex, ��֧�ֶ�εݹ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE             _G_hSpiListLock = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __SPI_LIST_LOCK()           API_SemaphoreBPend(_G_hSpiListLock, LW_OPTION_WAIT_INFINITE)
#define __SPI_LIST_UNLOCK()         API_SemaphoreBPost(_G_hSpiListLock)
/*********************************************************************************************************
** ��������: __spiAdapterFind
** ��������: ��ѯ spi ������
** �䡡��  : pcName        ����������
** �䡡��  : ������ָ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_SPI_ADAPTER  __spiAdapterFind (CPCHAR  pcName)
{
    REGISTER PLW_BUS_ADAPTER        pbusadapter;
    
    pbusadapter = __busAdapterGet(pcName);                              /*  �������߲�������            */

    return  ((PLW_SPI_ADAPTER)pbusadapter);                             /*  ���߽ṹΪ SPI ��������Ԫ�� */
}
/*********************************************************************************************************
** ��������: API_SpiLibInit
** ��������: ��ʼ�� spi �����
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpiLibInit (VOID)
{
    if (_G_hSpiListLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hSpiListLock = API_SemaphoreBCreate("spi_listlock", 
                                               LW_TRUE, LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                               LW_NULL);
    }
    
    if (_G_hSpiListLock) {
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SpiAdapterCreate
** ��������: ����һ�� spi ������
** �䡡��  : pcName        ����������
**           pspifunc      ����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpiAdapterCreate (CPCHAR           pcName, 
                           PLW_SPI_FUNCS    pspifunc)
{
    REGISTER PLW_SPI_ADAPTER    pspiadapter;

    if (!pcName || !pspifunc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pcName && _Object_Name_Invalid(pcName)) {                       /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (PX_ERROR);
    }
    
    /*
     *  �������ƿ�
     */
    pspiadapter = (PLW_SPI_ADAPTER)__SHEAP_ALLOC(sizeof(LW_SPI_ADAPTER));
    if (pspiadapter == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pspiadapter->SPIADAPTER_pspifunc       = pspifunc;
    pspiadapter->SPIADAPTER_plineDevHeader = LW_NULL;                   /*  Ŀǰ�������� spi �豸       */
    
    /*
     *  ���������
     */
    pspiadapter->SPIADAPTER_hBusLock = API_SemaphoreMCreate("spi_buslock", LW_PRIO_DEF_CEILING,
                                                            LW_OPTION_WAIT_PRIORITY |
                                                            LW_OPTION_INHERIT_PRIORITY |
                                                            LW_OPTION_DELETE_SAFE |
                                                            LW_OPTION_OBJECT_GLOBAL,
                                                            LW_NULL);
    if (pspiadapter->SPIADAPTER_hBusLock == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pspiadapter);
        return  (PX_ERROR);
    }
    
    /*
     *  �������߲�
     */
    if (__busAdapterCreate(&pspiadapter->SPIADAPTER_pbusadapter, pcName) != ERROR_NONE) {
        API_SemaphoreMDelete(&pspiadapter->SPIADAPTER_hBusLock);
        __SHEAP_FREE(pspiadapter);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SpiAdapterDelete
** ��������: �Ƴ�һ�� spi ������
** �䡡��  : pcName        ����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ں��е���������ȷ��������ʹ�������������, ����ɾ�����������.
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpiAdapterDelete (CPCHAR  pcName)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter = __spiAdapterFind(pcName);
    
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    /*
     *  ����Ӳ����豸, �����е�����, ��û���κα���!
     */
    if (API_SemaphoreMPend(pspiadapter->SPIADAPTER_hBusLock, 
                           LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (pspiadapter->SPIADAPTER_plineDevHeader) {                       /*  ����Ƿ����豸���ӵ�������  */
        API_SemaphoreMPost(pspiadapter->SPIADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    if (__busAdapterDelete(pcName) != ERROR_NONE) {                     /*  �����߲��Ƴ�                */
        API_SemaphoreMPost(pspiadapter->SPIADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    API_SemaphoreMDelete(&pspiadapter->SPIADAPTER_hBusLock);            /*  ɾ���������ź���            */
    __SHEAP_FREE(pspiadapter);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SpiAdapterGet
** ��������: ͨ�����ֻ�ȡһ�� spi ������
** �䡡��  : pcName        ����������
** �䡡��  : spi ���������ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_SPI_ADAPTER  API_SpiAdapterGet (CPCHAR  pcName)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter = __spiAdapterFind(pcName);
    
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (LW_NULL);
    
    } else {
        return  (pspiadapter);
    }
}
/*********************************************************************************************************
** ��������: API_SpiDeviceCreate
** ��������: ��ָ�� spi ��������, ����һ�� spi �豸
** �䡡��  : pcAdapterName       ����������
**           pcDeviceName        �豸����
** �䡡��  : spi �豸���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_SPI_DEVICE  API_SpiDeviceCreate (CPCHAR  pcAdapterName,
                                     CPCHAR  pcDeviceName)
{
    REGISTER PLW_SPI_ADAPTER    pspiadapter = __spiAdapterFind(pcAdapterName);
    REGISTER PLW_SPI_DEVICE     pspidevice;
    
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (LW_NULL);
    }
    
    if (pcDeviceName && _Object_Name_Invalid(pcDeviceName)) {           /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_NULL);
    }
    
    pspidevice = (PLW_SPI_DEVICE)__SHEAP_ALLOC(sizeof(LW_SPI_DEVICE));
    if (pspidevice == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    pspidevice->SPIDEV_pspiadapter = pspiadapter;
    pspidevice->SPIDEV_atomicUsageCnt.counter = 0;                      /*  Ŀǰ��û�б�ʹ�ù�          */
    lib_strcpy(pspidevice->SPIDEV_cName, pcDeviceName);
    
    __SPI_LIST_LOCK();                                                  /*  ���� spi ����               */
    _List_Line_Add_Ahead(&pspidevice->SPIDEV_lineManage, 
                         &pspiadapter->SPIADAPTER_plineDevHeader);      /*  �����������豸����          */
    __SPI_LIST_UNLOCK();                                                /*  ���� spi ����               */
    
    LW_BUS_INC_DEV_COUNT(&pspiadapter->SPIADAPTER_pbusadapter);         /*  �����豸++                  */
    
    return  (pspidevice);
}
/*********************************************************************************************************
** ��������: API_SpiDeviceDelete
** ��������: ɾ��ָ���� spi �豸
** �䡡��  : pspidevice        ָ���� spi �豸���ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceDelete (PLW_SPI_DEVICE   pspidevice)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter;

    if (pspidevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pspiadapter = pspidevice->SPIDEV_pspiadapter;
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (API_AtomicGet(&pspidevice->SPIDEV_atomicUsageCnt)) {
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    __SPI_LIST_LOCK();                                                  /*  ���� spi ����               */
    _List_Line_Del(&pspidevice->SPIDEV_lineManage, 
                   &pspiadapter->SPIADAPTER_plineDevHeader);            /*  �����������豸����          */
    __SPI_LIST_UNLOCK();                                                /*  ���� spi ����               */
    
    LW_BUS_DEC_DEV_COUNT(&pspiadapter->SPIADAPTER_pbusadapter);         /*  �����豸--                  */
    
    __SHEAP_FREE(pspidevice);                                           /*  �ͷ��ڴ�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SpiDeviceUsageInc
** ��������: ��ָ�� spi �豸ʹ�ü���++
** �䡡��  : pspidevice        ָ���� spi �豸���ƿ�
** �䡡��  : ��ǰ��ʹ�ü���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceUsageInc (PLW_SPI_DEVICE   pspidevice)
{
    if (pspidevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicInc(&pspidevice->SPIDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** ��������: API_SpiDeviceUsageDec
** ��������: ��ָ�� spi �豸ʹ�ü���--
** �䡡��  : pspidevice        ָ���� spi �豸���ƿ�
** �䡡��  : ��ǰ��ʹ�ü���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceUsageDec (PLW_SPI_DEVICE   pspidevice)
{
    if (pspidevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicDec(&pspidevice->SPIDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** ��������: API_SpiDeviceUsageGet
** ��������: ���ָ�� spi �豸ʹ�ü���
** �䡡��  : pspidevice        ָ���� spi �豸���ƿ�
** �䡡��  : ��ǰ��ʹ�ü���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceUsageGet (PLW_SPI_DEVICE   pspidevice)
{
    if (pspidevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicGet(&pspidevice->SPIDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** ��������: API_SpiDeviceBusRequest
** ��������: ���ָ�� SPI �豸������ʹ��Ȩ
** �䡡��  : pspidevice        ָ���� spi �豸���ƿ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceBusRequest (PLW_SPI_DEVICE   pspidevice)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter;

    if (!pspidevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pspiadapter = pspidevice->SPIDEV_pspiadapter;
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (API_SemaphoreMPend(pspiadapter->SPIADAPTER_hBusLock, 
                           LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {    /*  ��������                    */
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SpiDeviceBusRelease
** ��������: �ͷ�ָ�� SPI �豸������ʹ��Ȩ
** �䡡��  : pspidevice        ָ���� spi �豸���ƿ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceBusRelease (PLW_SPI_DEVICE   pspidevice)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter;

    if (!pspidevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pspiadapter = pspidevice->SPIDEV_pspiadapter;
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (API_SemaphoreMPost(pspiadapter->SPIADAPTER_hBusLock) != 
        ERROR_NONE) {                                                   /*  ��������                    */
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SpiDeviceTransfer
** ��������: ʹ��ָ�� spi �豸����һ�δ���
** �䡡��  : pspidevice        ָ���� spi �豸���ƿ�
**           pspimsg           ������Ϣ���ƿ���
**           iNum              ������Ϣ������Ϣ������
** �䡡��  : ������ pspimsg ����, ���󷵻� -1
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceTransfer (PLW_SPI_DEVICE   pspidevice, 
                            PLW_SPI_MESSAGE  pspimsg,
                            INT              iNum)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter;
             INT                    iRet;

    if (!pspidevice || !pspimsg || (iNum < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pspiadapter = pspidevice->SPIDEV_pspiadapter;
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    
    if (pspiadapter->SPIADAPTER_pspifunc->SPIFUNC_pfuncMasterXfer) {
        if (API_SemaphoreMPend(pspiadapter->SPIADAPTER_hBusLock, 
                               LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {/*  ��������                    */
            return  (PX_ERROR);
        }
        iRet = pspiadapter->SPIADAPTER_pspifunc->SPIFUNC_pfuncMasterXfer(pspiadapter, pspimsg, iNum);
        API_SemaphoreMPost(pspiadapter->SPIADAPTER_hBusLock);           /*  �ͷ�����                    */
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_SpiDeviceCtl
** ��������: ָ�� spi �豸����ָ������
** �䡡��  : pspidevice        ָ���� spi �豸���ƿ�
**           iCmd              ������
**           lArg              �������
** �䡡��  : ����ִ�н��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_SpiDeviceCtl (PLW_SPI_DEVICE   pspidevice, INT  iCmd, LONG  lArg)
{
    REGISTER PLW_SPI_ADAPTER        pspiadapter;

    if (!pspidevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pspiadapter = pspidevice->SPIDEV_pspiadapter;
    if (pspiadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (pspiadapter->SPIADAPTER_pspifunc->SPIFUNC_pfuncMasterCtl) {
        return  (pspiadapter->SPIADAPTER_pspifunc->SPIFUNC_pfuncMasterCtl(pspiadapter, iCmd, lArg));
    } else {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
