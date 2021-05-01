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
** ��   ��   ��: i2cLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 10 �� 20 ��
**
** ��        ��: i2c �豸������.

** BUG:
2009.10.27  ϵͳ�������߲�, �� i2c �����������������߲�.
2012.11.09  API_I2cAdapterCreate() ����Ҫ��������, �����ߴ���ʱ����.
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
static LW_OBJECT_HANDLE             _G_hI2cListLock = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __I2C_LIST_LOCK()           API_SemaphoreBPend(_G_hI2cListLock, LW_OPTION_WAIT_INFINITE)
#define __I2C_LIST_UNLOCK()         API_SemaphoreBPost(_G_hI2cListLock)
/*********************************************************************************************************
** ��������: __i2cAdapterFind
** ��������: ��ѯ i2c ������
** �䡡��  : pcName        ����������
** �䡡��  : ������ָ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_I2C_ADAPTER  __i2cAdapterFind (CPCHAR  pcName)
{
    REGISTER PLW_BUS_ADAPTER        pbusadapter;
    
    pbusadapter = __busAdapterGet(pcName);                              /*  �������߲�������            */

    return  ((PLW_I2C_ADAPTER)pbusadapter);                             /*  ���߽ṹΪ i2c ��������Ԫ�� */
}
/*********************************************************************************************************
** ��������: API_I2cLibInit
** ��������: ��ʼ�� i2c �����
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_I2cLibInit (VOID)
{
    if (_G_hI2cListLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hI2cListLock = API_SemaphoreBCreate("i2c_listlock", 
                                               LW_TRUE, LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                               LW_NULL);
    }
    
    if (_G_hI2cListLock) {
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_I2cAdapterCreate
** ��������: ����һ�� i2c ������
** �䡡��  : pcName        ����������
**           pi2cfunc      ����������
**           ulTimeout     ������ʱʱ�� (ticks)
**           iRetry        ���Դ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_I2cAdapterCreate (CPCHAR           pcName, 
                           PLW_I2C_FUNCS    pi2cfunc,
                           ULONG            ulTimeout,
                           INT              iRetry)
{
    REGISTER PLW_I2C_ADAPTER    pi2cadapter;

    if (!pcName || !pi2cfunc || (iRetry < 0)) {
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
    pi2cadapter = (PLW_I2C_ADAPTER)__SHEAP_ALLOC(sizeof(LW_I2C_ADAPTER));
    if (pi2cadapter == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pi2cadapter->I2CADAPTER_pi2cfunc       = pi2cfunc;
    pi2cadapter->I2CADAPTER_ulTimeout      = ulTimeout;
    pi2cadapter->I2CADAPTER_iRetry         = iRetry;
    pi2cadapter->I2CADAPTER_plineDevHeader = LW_NULL;                   /*  Ŀǰ�������� i2c �豸       */
    
    /*
     *  ���������
     */
    pi2cadapter->I2CADAPTER_hBusLock = API_SemaphoreBCreate("i2c_buslock", 
                                                            LW_TRUE, 
                                                            LW_OPTION_WAIT_FIFO |
                                                            LW_OPTION_OBJECT_GLOBAL,
                                                            LW_NULL);
    if (pi2cadapter->I2CADAPTER_hBusLock == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pi2cadapter);
        return  (PX_ERROR);
    }
    
    /*
     *  �������߲�
     */
    if (__busAdapterCreate(&pi2cadapter->I2CADAPTER_pbusadapter, pcName) != ERROR_NONE) {
        API_SemaphoreBDelete(&pi2cadapter->I2CADAPTER_hBusLock);
        __SHEAP_FREE(pi2cadapter);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_I2cAdapterDelete
** ��������: �Ƴ�һ�� i2c ������
** �䡡��  : pcName        ����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ں��е���������ȷ��������ʹ�������������, ����ɾ�����������.
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_I2cAdapterDelete (CPCHAR  pcName)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter = __i2cAdapterFind(pcName);
    
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    /*
     *  ����Ӳ����豸, �����е�����, ��û���κα���!
     */
    if (API_SemaphoreBPend(pi2cadapter->I2CADAPTER_hBusLock, 
                           LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (pi2cadapter->I2CADAPTER_plineDevHeader) {                       /*  ����Ƿ����豸���ӵ�������  */
        API_SemaphoreBPost(pi2cadapter->I2CADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    if (__busAdapterDelete(pcName) != ERROR_NONE) {                     /*  �����߲��Ƴ�                */
        API_SemaphoreBPost(pi2cadapter->I2CADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    API_SemaphoreBDelete(&pi2cadapter->I2CADAPTER_hBusLock);            /*  ɾ���������ź���            */
    __SHEAP_FREE(pi2cadapter);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_I2cAdapterGet
** ��������: ͨ�����ֻ�ȡһ�� i2c ������
** �䡡��  : pcName        ����������
** �䡡��  : i2c ���������ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_I2C_ADAPTER  API_I2cAdapterGet (CPCHAR  pcName)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter = __i2cAdapterFind(pcName);
    
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (LW_NULL);
    
    } else {
        return  (pi2cadapter);
    }
}
/*********************************************************************************************************
** ��������: API_I2cDeviceCreate
** ��������: ��ָ�� i2c ��������, ����һ�� i2c �豸
** �䡡��  : pcAdapterName ����������
**           pcDeviceName  �豸����
**           usAddr        �豸��ַ
**           usFlag        �豸��־
** �䡡��  : i2c �豸���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_I2C_DEVICE  API_I2cDeviceCreate (CPCHAR  pcAdapterName,
                                     CPCHAR  pcDeviceName,
                                     UINT16  usAddr,
                                     UINT16  usFlag)
{
    REGISTER PLW_I2C_ADAPTER    pi2cadapter = __i2cAdapterFind(pcAdapterName);
    REGISTER PLW_I2C_DEVICE     pi2cdevice;
    
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (LW_NULL);
    }
    
    if (pcDeviceName && _Object_Name_Invalid(pcDeviceName)) {           /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_NULL);
    }
    
    pi2cdevice = (PLW_I2C_DEVICE)__SHEAP_ALLOC(sizeof(LW_I2C_DEVICE));
    if (pi2cdevice == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    pi2cdevice->I2CDEV_usAddr      = usAddr;
    pi2cdevice->I2CDEV_usFlag      = usFlag;
    pi2cdevice->I2CDEV_pi2cadapter = pi2cadapter;
    pi2cdevice->I2CDEV_atomicUsageCnt.counter = 0;                      /*  Ŀǰ��û�б�ʹ�ù�          */
    lib_strcpy(pi2cdevice->I2CDEV_cName, pcDeviceName);
    
    __I2C_LIST_LOCK();                                                  /*  ���� i2c ����               */
    _List_Line_Add_Ahead(&pi2cdevice->I2CDEV_lineManage, 
                         &pi2cadapter->I2CADAPTER_plineDevHeader);      /*  �����������豸����          */
    __I2C_LIST_UNLOCK();                                                /*  ���� i2c ����               */
    
    LW_BUS_INC_DEV_COUNT(&pi2cadapter->I2CADAPTER_pbusadapter);         /*  �����豸++                  */
    
    return  (pi2cdevice);
}
/*********************************************************************************************************
** ��������: API_I2cDeviceDelete
** ��������: ɾ��ָ���� i2c �豸
** �䡡��  : pi2cdevice        ָ���� i2c �豸���ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceDelete (PLW_I2C_DEVICE   pi2cdevice)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter;

    if (pi2cdevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pi2cadapter = pi2cdevice->I2CDEV_pi2cadapter;
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (API_AtomicGet(&pi2cdevice->I2CDEV_atomicUsageCnt)) {
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    __I2C_LIST_LOCK();                                                  /*  ���� i2c ����               */
    _List_Line_Del(&pi2cdevice->I2CDEV_lineManage, 
                   &pi2cadapter->I2CADAPTER_plineDevHeader);            /*  �����������豸����          */
    __I2C_LIST_UNLOCK();                                                /*  ���� i2c ����               */
    
    LW_BUS_DEC_DEV_COUNT(&pi2cadapter->I2CADAPTER_pbusadapter);         /*  �����豸--                  */
    
    __SHEAP_FREE(pi2cdevice);                                           /*  �ͷ��ڴ�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_I2cDeviceUsageInc
** ��������: ��ָ�� i2c �豸ʹ�ü���++
** �䡡��  : pi2cdevice        ָ���� i2c �豸���ƿ�
** �䡡��  : ��ǰ��ʹ�ü���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceUsageInc (PLW_I2C_DEVICE   pi2cdevice)
{
    if (pi2cdevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicInc(&pi2cdevice->I2CDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** ��������: API_I2cDeviceUsageDec
** ��������: ��ָ�� i2c �豸ʹ�ü���--
** �䡡��  : pi2cdevice        ָ���� i2c �豸���ƿ�
** �䡡��  : ��ǰ��ʹ�ü���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceUsageDec (PLW_I2C_DEVICE   pi2cdevice)
{
    if (pi2cdevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicDec(&pi2cdevice->I2CDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** ��������: API_I2cDeviceUsageGet
** ��������: ���ָ�� i2c �豸ʹ�ü���
** �䡡��  : pi2cdevice        ָ���� i2c �豸���ƿ�
** �䡡��  : ��ǰ��ʹ�ü���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceUsageGet (PLW_I2C_DEVICE   pi2cdevice)
{
    if (pi2cdevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicGet(&pi2cdevice->I2CDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** ��������: API_I2cDeviceTransfer
** ��������: ʹ��ָ�� i2c �豸����һ�δ���
** �䡡��  : pi2cdevice        ָ���� i2c �豸���ƿ�
**           pi2cmsg           ������Ϣ���ƿ���
**           iNum              ������Ϣ������Ϣ������
** �䡡��  : ������ pi2cmsg ����, ���󷵻� -1
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceTransfer (PLW_I2C_DEVICE   pi2cdevice, 
                            PLW_I2C_MESSAGE  pi2cmsg,
                            INT              iNum)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter;
             INT                    iRet;

    if (!pi2cdevice || !pi2cmsg || (iNum < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pi2cadapter = pi2cdevice->I2CDEV_pi2cadapter;
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    
    if (pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterXfer) {
        if (API_SemaphoreBPend(pi2cadapter->I2CADAPTER_hBusLock, 
                               LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {/*  ��������                    */
            return  (PX_ERROR);
        }
        iRet = pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterXfer(pi2cadapter, pi2cmsg, iNum);
        API_SemaphoreBPost(pi2cadapter->I2CADAPTER_hBusLock);           /*  �ͷ�����                    */
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_I2cDeviceMasterSend
** ��������: ʹ��ָ�� i2c �豸����һ�η��ʹ���
** �䡡��  : pi2cdevice        ָ���� i2c �豸���ƿ�
**           pcBuffer          ���ͻ���
**           iCount            ��Ҫ���͵�����
** �䡡��  : ʵ�ʷ�������, ���󷵻� -1
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceMasterSend (PLW_I2C_DEVICE   pi2cdevice,
                              CPCHAR           pcBuffer,
                              INT              iCount)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter;
             LW_I2C_MESSAGE         i2cmsg;
             INT                    iRet;
    
    if (!pi2cdevice || !pcBuffer || (iCount < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pi2cadapter = pi2cdevice->I2CDEV_pi2cadapter;
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterXfer) {
        i2cmsg.I2CMSG_usAddr    = pi2cdevice->I2CDEV_usAddr;
        i2cmsg.I2CMSG_usFlag    = (UINT16)(pi2cdevice->I2CDEV_usFlag & LW_I2C_M_TEN);
        i2cmsg.I2CMSG_usLen     = (UINT16)iCount;
        i2cmsg.I2CMSG_pucBuffer = (UINT8 *)pcBuffer;

        if (API_SemaphoreBPend(pi2cadapter->I2CADAPTER_hBusLock, 
                               LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {/*  ��������                    */
            return  (PX_ERROR);
        }
        iRet = pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterXfer(pi2cadapter, &i2cmsg, 1);
        API_SemaphoreBPost(pi2cadapter->I2CADAPTER_hBusLock);           /*  �ͷ�����                    */
        
        iRet = (iRet == 1) ? (iCount) : (iRet);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_I2cDeviceMasterRecv
** ��������: ʹ��ָ�� i2c �豸����һ�ν��մ���
** �䡡��  : pi2cdevice        ָ���� i2c �豸���ƿ�
**           pcBuffer          ���ջ���
**           iCount            ���ջ����С
** �䡡��  : ʵ�ʽ�������, ���󷵻� -1
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceMasterRecv (PLW_I2C_DEVICE   pi2cdevice,
                              PCHAR            pcBuffer,
                              INT              iCount)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter;
             LW_I2C_MESSAGE         i2cmsg;
             INT                    iRet;
             
    if (!pi2cdevice || !pcBuffer || (iCount < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pi2cadapter = pi2cdevice->I2CDEV_pi2cadapter;
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterXfer) {
        i2cmsg.I2CMSG_usAddr    = pi2cdevice->I2CDEV_usAddr;
        i2cmsg.I2CMSG_usFlag    = (UINT16)(pi2cdevice->I2CDEV_usFlag & LW_I2C_M_TEN);
        i2cmsg.I2CMSG_usFlag    = (UINT16)(i2cmsg.I2CMSG_usFlag | LW_I2C_M_RD);
        i2cmsg.I2CMSG_usLen     = (UINT16)iCount;
        i2cmsg.I2CMSG_pucBuffer = (UINT8 *)pcBuffer;
        
        if (API_SemaphoreBPend(pi2cadapter->I2CADAPTER_hBusLock, 
                               LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {/*  ��������                    */
            return  (PX_ERROR);
        }
        iRet = pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterXfer(pi2cadapter, &i2cmsg, 1);
        API_SemaphoreBPost(pi2cadapter->I2CADAPTER_hBusLock);           /*  �ͷ�����                    */
        
        iRet = (iRet == 1) ? (iCount) : (iRet);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_I2cDeviceCtl
** ��������: ָ�� i2c �豸����ָ������
** �䡡��  : pi2cdevice        ָ���� i2c �豸���ƿ�
**           iCmd              ������
**           lArg              �������
** �䡡��  : ����ִ�н��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_I2cDeviceCtl (PLW_I2C_DEVICE   pi2cdevice, INT  iCmd, LONG  lArg)
{
    REGISTER PLW_I2C_ADAPTER        pi2cadapter;

    if (!pi2cdevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pi2cadapter = pi2cdevice->I2CDEV_pi2cadapter;
    if (pi2cadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterCtl) {
        return  (pi2cadapter->I2CADAPTER_pi2cfunc->I2CFUNC_pfuncMasterCtl(pi2cadapter, iCmd, lArg));
    } else {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
