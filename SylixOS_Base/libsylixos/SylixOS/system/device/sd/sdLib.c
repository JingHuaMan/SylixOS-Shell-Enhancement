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
** ��   ��   ��: sdLib.c
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2010 �� 11 �� 22 ��
**
** ��        ��: sd�豸������

** BUG:
**
2011.01.18  ����SD�豸��ȡ����.
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
static LW_OBJECT_HANDLE    _G_hSdListLock = LW_OBJECT_HANDLE_INVALID;   /* ���������                   */
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __SD_LIST_LOCK()           API_SemaphoreBPend(_G_hSdListLock, LW_OPTION_WAIT_INFINITE)
#define __SD_LIST_UNLOCK()         API_SemaphoreBPost(_G_hSdListLock)

#define __SD_ADAPTER_LOCK(pAda)    API_SemaphoreBPend((pAda)->SDADAPTER_hBusLock, LW_OPTION_WAIT_INFINITE)
#define __SD_ADAPTER_UNLOCK(pAda)  API_SemaphoreBPost((pAda)->SDADAPTER_hBusLock)
/*********************************************************************************************************
  �ڲ��������
*********************************************************************************************************/
#define __SD_DEBUG_EN              0
#if __SD_DEBUG_EN > 0
#define __SD_DEBUG_MSG             __SD_DEBUG_MSG
#else
#define __SD_DEBUG_MSG(level, msg)
#endif
/*********************************************************************************************************
** ��������: __sdAdapterFind
** ��������: ����һ��sd������
** ��    ��: NpcName  ����������
** ��    ��: NONE
** ��    ��: �ҵ�,����������ָ��,���򷵻�LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_SD_ADAPTER  __sdAdapterFind (CPCHAR  pcName)
{
    PLW_BUS_ADAPTER      pbusadapter;

    pbusadapter = __busAdapterGet(pcName);                              /*  �������߲�������            */

    return  ((PLW_SD_ADAPTER)pbusadapter);
}
/*********************************************************************************************************
** ��������: API_SdLibInit
** ��������: ��ʼ��SD�����
** ��    ��: NONE
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_SdLibInit (VOID)
{
    if (_G_hSdListLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hSdListLock = API_SemaphoreBCreate("sd_listlock",
                                              LW_TRUE, LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                              LW_NULL);
    }

    if (_G_hSdListLock) {
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdAdapterCreate
** ��������: ����һ��sd������
** ��    ��: pcName     ����������
**           psdfunc    ���������߲���
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_SdAdapterCreate (CPCHAR pcName, PLW_SD_FUNCS psdfunc)
{
    PLW_SD_ADAPTER  psdadapter;

    if (!pcName || !psdfunc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pcName && _Object_Name_Invalid(pcName)) {                       /*  ���������Ч��              */
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (PX_ERROR);
    }

    /*
     *  �������ƿ�
     */
    psdadapter = (PLW_SD_ADAPTER)__SHEAP_ALLOC(sizeof(LW_SD_ADAPTER));
    if (psdadapter == LW_NULL) {
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(psdadapter, sizeof(LW_SD_ADAPTER));

    psdadapter->SDADAPTER_psdfunc         = psdfunc;
    psdadapter->SDADAPTER_plineDevHeader  = LW_NULL;                     /*  Ŀǰ�������� sd�豸        */

    /*
     *  ������
     */
    psdadapter->SDADAPTER_hBusLock = API_SemaphoreBCreate("sd_buslock",
                                                          LW_TRUE, 
                                                          LW_OPTION_WAIT_FIFO | 
                                                          LW_OPTION_OBJECT_GLOBAL,
                                                          LW_NULL);
    if (psdadapter->SDADAPTER_hBusLock == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(psdadapter);
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "create semaphore failed.\r\n");
        return  (PX_ERROR);
    }

    /*
     *  �������߲�
     */
    if (__busAdapterCreate(&psdadapter->SDADAPTER_busadapter, pcName) != ERROR_NONE) {
        API_SemaphoreBDelete(&psdadapter->SDADAPTER_hBusLock);
        __SHEAP_FREE(psdadapter);
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "add adapter to system bus failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdAdapterDelete
** ��������: ɾ��һ��sd������
** ��    ��: pcName     ����������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT API_SdAdapterDelete (CPCHAR pcName)
{
    PLW_SD_ADAPTER  psdadapter = __sdAdapterFind(pcName);

    if (psdadapter == LW_NULL) {
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL,"adapter is not exist.\r\n");
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�                      */
        return  (PX_ERROR);
    }

    if (API_SemaphoreBPend(psdadapter->SDADAPTER_hBusLock,
                           LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (psdadapter->SDADAPTER_plineDevHeader) {                         /*  ����Ƿ����豸���ӵ�����    */
        API_SemaphoreBPost(psdadapter->SDADAPTER_hBusLock);
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "some devices are still on the bus.\r\n");
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }

    if (__busAdapterDelete(pcName) != ERROR_NONE) {                     /*  �����߲��Ƴ�                */
        API_SemaphoreBPost(psdadapter->SDADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }

    API_SemaphoreBDelete(&psdadapter->SDADAPTER_hBusLock);              /*  ɾ���������ź���            */
    __SHEAP_FREE(psdadapter);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdAdapterGet
** ��������: ���һ��sd������
** ��    ��: pcName     ����������
** ��    ��: NONE
** ��    ��: �ҵ�,����������ָ��,���򷵻�LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
PLW_SD_ADAPTER   API_SdAdapterGet (CPCHAR pcName)
{
    PLW_SD_ADAPTER  psdadapter = __sdAdapterFind(pcName);

    if (psdadapter == LW_NULL) {
       __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "adapter is not exist.\r\n");
       _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                          /*  δ�ҵ�                      */
       return  (LW_NULL);
    }

    return  (psdadapter);
}
/*********************************************************************************************************
** ��������: API_SdDeviceCreate
** ��������: ����һ��sd�豸
** ��    ��: pcAdapterName     ����������
**           pcDeviceName      �豸����
** ��    ��: NONE
** ��    ��: �ҵ�,����������ָ��,���򷵻�LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
PLW_SD_DEVICE  API_SdDeviceCreate (CPCHAR pcAdapterName, CPCHAR pcDeviceName)
{
    PLW_SD_ADAPTER  psdadapter = __sdAdapterFind(pcAdapterName);
    PLW_SD_DEVICE   psddevice;

    if (psdadapter == LW_NULL) {
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no adapter.\r\n");
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  (LW_NULL);
    }
    
    if (pcDeviceName && _Object_Name_Invalid(pcDeviceName)) {           /*  ���������Ч��              */
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_NULL);
    }

    psddevice = (PLW_SD_DEVICE)__SHEAP_ALLOC(sizeof(LW_SD_DEVICE));
    if ( psddevice == LW_NULL) {
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(psddevice, sizeof(LW_SD_DEVICE));
    
    psddevice->SDDEV_psdAdapter             = psdadapter;
    psddevice->SDDEV_atomicUsageCnt.counter = 0;                        /*  Ŀǰ��û�б�ʹ�ù�          */
    lib_strcpy(psddevice->SDDEV_pDevName, pcDeviceName);

    __SD_LIST_LOCK();
    _List_Line_Add_Ahead(&psddevice->SDDEV_lineManage,
                         &psdadapter->SDADAPTER_plineDevHeader);        /*  �����������豸����          */
    __SD_LIST_UNLOCK();

    LW_BUS_INC_DEV_COUNT(&psdadapter->SDADAPTER_busadapter);

    return  (psddevice);
}
/*********************************************************************************************************
** ��������: API_SdDeviceDelete
** ��������: ɾ��ָ���� sd �豸
** ��    ��: psddevice        ָ���� sd �豸���ƿ�
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_SdDeviceDelete (PLW_SD_DEVICE   psddevice)
{
    PLW_SD_ADAPTER        psdadapter;

    if (psddevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdadapter = psddevice->SDDEV_psdAdapter;
    if (psdadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }

    if (API_AtomicGet(&psddevice->SDDEV_atomicUsageCnt)) {
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }

    __SD_LIST_LOCK();
    _List_Line_Del(&psddevice->SDDEV_lineManage,
                   &psdadapter->SDADAPTER_plineDevHeader);
    __SD_LIST_UNLOCK();

    LW_BUS_DEC_DEV_COUNT(&psdadapter->SDADAPTER_busadapter);            /*  �����豸--                  */

    __SHEAP_FREE(psddevice);                                            /*  �ͷ��ڴ�                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdDeviceGet
** ��������: ����ָ���� sd �豸
** ��    ��: pcAdapterName        ��������
**           pcDeviceName         �豸��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
PLW_SD_DEVICE  API_SdDeviceGet (CPCHAR pcAdapterName, CPCHAR pcDeviceName)
{
    PLW_LIST_LINE          plineTemp  = LW_NULL;
    PLW_SD_ADAPTER         psdadapter = LW_NULL;
    PLW_SD_DEVICE          psddevice  = LW_NULL;

    if (pcAdapterName == LW_NULL || pcDeviceName == LW_NULL) {
        return  (LW_NULL);
    }

    psdadapter = __sdAdapterFind(pcAdapterName);
    if (!psdadapter) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (LW_NULL);
    }

    __SD_LIST_LOCK();
    for (plineTemp  = psdadapter->SDADAPTER_plineDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ������������                */

        psddevice = _LIST_ENTRY(plineTemp, LW_SD_DEVICE, SDDEV_lineManage);
        if (lib_strcmp(pcDeviceName, psddevice->SDDEV_pDevName) == 0) {
            break;                                                      /*  �Ѿ��ҵ���                  */
        }
    }
    __SD_LIST_UNLOCK();

    if (plineTemp) {
        return  (psddevice);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_SdDeviceUsageInc
** ��������: ָ���� sd �豸ʹ�ü���++
** ��    ��: psddevice        ָ���� sd �豸���ƿ�
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT   API_SdDeviceUsageInc (PLW_SD_DEVICE    psddevice)
{
    if (psddevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (API_AtomicInc(&psddevice->SDDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** ��������: API_SdDeviceUsageDec
** ��������: ָ���� sd �豸ʹ�ü���--
** ��    ��: psddevice        ָ���� sd �豸���ƿ�
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_SdDeviceUsageDec (PLW_SD_DEVICE   psddevice)
{
    if (psddevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (API_AtomicDec(&psddevice->SDDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** ��������: API_SdDeviceUsageGet
** ��������: ���ָ���� sd �豸��ʹ�ü���
** ��    ��: psddevice     ָ���� sd �豸���ƿ�
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_SdDeviceUsageGet (PLW_SD_DEVICE   psddevice)
{
    if (psddevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (API_AtomicGet(&psddevice->SDDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** ��������: API_SdDeviceTransfer
** ��������: ���豸����һ������
** ��    ��: psddevice        ָ���� sd �豸���ƿ�
**           psdmsg           ���������Ϣ����
**           iNum             ���������Ϣ��������Ϣ����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdDeviceTransfer (PLW_SD_DEVICE   psddevice,
                                  PLW_SD_MESSAGE  psdmsg,
                                  INT             iNum)
{
    PLW_SD_ADAPTER  psdadapter ;
    INT             iError;

    if (!psddevice || !psdmsg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdadapter = psddevice->SDDEV_psdAdapter;
    if (psdadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }

    if (SDADP_TXF(psdadapter)) {
        __SD_ADAPTER_LOCK(psdadapter);                                  /*  ����SD����                  */

        iError = SDBUS_TRANSFER(psdadapter,
                                psddevice,
                                psdmsg,
                                iNum);

        __SD_ADAPTER_UNLOCK(psdadapter);                                /*  ����SD����                  */
    } else {
        _ErrorHandle(ENOSYS);
        iError = PX_ERROR;
    }

    if (iError != ERROR_NONE) {
        __SD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
        return  (PX_ERROR);
    }

    return  (iError);
}
/*********************************************************************************************************
** ��������: API_SdDeviceCtl
** ��������: ���豸����һ�ο��Ʋ���
** ��    ��: psddevice      ָ���� sd �豸���ƿ�
**           iCmd           ��������
**           lArg           ���Ʋ���
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdDeviceCtl (PLW_SD_DEVICE  psddevice,
                             INT            iCmd,
                             LONG           lArg)
{
    PLW_SD_ADAPTER  psdadapter ;
    INT             iError;

    if (!psddevice ) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdadapter = psddevice->SDDEV_psdAdapter;
    if (psdadapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }

    if (SDADP_CTRL(psdadapter)) {
        __SD_ADAPTER_LOCK(psdadapter);                                  /*  ����SD����                  */

        iError = SDBUS_IOCTRL(psdadapter,
                              iCmd,
                              lArg);

        __SD_ADAPTER_UNLOCK(psdadapter);                                /*  ����SD����                  */
    } else {
        _ErrorHandle(ENOSYS);
        iError = PX_ERROR;
    }

    return  (iError);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
