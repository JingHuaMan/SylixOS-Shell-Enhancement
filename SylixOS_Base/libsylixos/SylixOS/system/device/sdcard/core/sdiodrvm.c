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
** ��   ��   ��: sdiodrvm.c
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2014 �� 10 �� 27 ��
**
** ��        ��: sd drv manager layer for sdio

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0) && (LW_CFG_SDCARD_SDIO_EN > 0)
#include "sddrvm.h"
#include "sdiodrvm.h"
#include "sdutil.h"
#include "../include/sddebug.h"
/*********************************************************************************************************
  SDM ������ע���¼�֪ͨ
*********************************************************************************************************/
#define SDM_EVENT_NEW_DRV       10
/*********************************************************************************************************
  ȫ�ֶ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE         _G_hSdmDrvLock        = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER      _G_plineSdiodrvHeader = LW_NULL;

#define __SDM_DRV_LOCK()        API_SemaphoreBPend(_G_hSdmDrvLock, LW_OPTION_WAIT_INFINITE)
#define __SDM_DRV_UNLOCK()      API_SemaphoreBPost(_G_hSdmDrvLock)
/*********************************************************************************************************
  ǰ������
*********************************************************************************************************/
static VOID  __sdmIoDrvInsert(SDIO_DRV *psdiodrv);
static VOID  __sdmIoDrvDelete(SDIO_DRV *psdiodrv);
/*********************************************************************************************************
** ��������: API_SdmSdioLibInit
** ��������: SDM SDIO���� ������ʼ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmSdioLibInit (VOID)
{
    if (_G_hSdmDrvLock != LW_OBJECT_HANDLE_INVALID) {
        return  (ERROR_NONE);
    }

    _G_hSdmDrvLock = API_SemaphoreBCreate("sdmdrv_lock", LW_TRUE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (_G_hSdmDrvLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmSdioDrvRegister
** ��������: ��SDMע��һ��SDIO�豸Ӧ������
** ��    ��: psdiodrv         SDIO ��������. ע��SDM�ڲ���ֱ�����øö���, ��˸ö�����Ҫ������Ч
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmSdioDrvRegister (SDIO_DRV *psdiodrv)
{
    SDIO_DRV          *psdiodrvTmp;
    PLW_LIST_LINE      plineTmp;

    if (!psdiodrv) {
        return  (PX_ERROR);
    }

    __SDM_DRV_LOCK();
    for (plineTmp  = _G_plineSdiodrvHeader;
         plineTmp != LW_NULL;
         plineTmp  = _list_line_get_next(plineTmp)) {

        psdiodrvTmp = _LIST_ENTRY(plineTmp, SDIO_DRV, SDIODRV_lineManage);
        if (psdiodrvTmp == psdiodrv) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "drv has been already registered.\r\n");
            __SDM_DRV_UNLOCK();
            return  (PX_ERROR);
        }

        /*
         * ��Ϊ SDM �ڲ������������ֲ�����, �����������
         */
        if (lib_strcmp(psdiodrvTmp->SDIODRV_cpcName, psdiodrv->SDIODRV_cpcName) == 0) {
            SDCARD_DEBUG_MSG(__LOGMESSAGE_LEVEL, " warning: exist a same name drv"
                                                 " as current registering.\r\n");
        }
    }
    __SDM_DRV_UNLOCK();

    API_AtomicSet(0, &psdiodrv->SDIODRV_atomicDevCnt);
    __sdmIoDrvInsert(psdiodrv);

    API_SdmEventNotify(LW_NULL, SDM_EVENT_NEW_DRV);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmSdioDrvUnRegister
** ��������: ��SDMע��һ��SDIO�豸Ӧ������
** ��    ��: psdiodrv     SDIO ��������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmSdioDrvUnRegister (SDIO_DRV *psdiodrv)
{
    if (!psdiodrv) {
        return  (PX_ERROR);
    }

    if (API_AtomicGet(&psdiodrv->SDIODRV_atomicDevCnt)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "exist device using this drv.\r\n");
        return  (PX_ERROR);
    }

    __sdmIoDrvDelete(psdiodrv);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdmIoDrvInsert
** ��������: ����һ�� SD ����
** ��    ��: psdiodrv     SDIO ��������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdmIoDrvInsert (SDIO_DRV *psdiodrv)
{
    __SDM_DRV_LOCK();
    _List_Line_Add_Ahead(&psdiodrv->SDIODRV_lineManage, &_G_plineSdiodrvHeader);
    __SDM_DRV_UNLOCK();
}
/*********************************************************************************************************
** ��������: __sdmIoDrvDelete
** ��������: ɾ��һ�� SD ����
** ��    ��: psdiodrv     SDIO ��������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdmIoDrvDelete (SDIO_DRV *psdiodrv)
{
    __SDM_DRV_LOCK();
    _List_Line_Del(&psdiodrv->SDIODRV_lineManage, &_G_plineSdiodrvHeader);
    __SDM_DRV_UNLOCK();
}
/*********************************************************************************************************
** ��������: __sdmSdioDrvHeader
** ��������: ��õ�һ�� SDIO ����(��sdiobase ����ģ��ɼ�)
** ��    ��: NONE
** ��    ��: ��һ�� SDIO ����������ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_LIST_LINE  __sdmSdioDrvHeader (VOID)
{
    return  (_G_plineSdiodrvHeader);
}
/*********************************************************************************************************
** ��������: __sdmSdioDrvAccessRequest
** ��������: SDIO ���������������(��sdiobase ����ģ��ɼ�)
** ��    ��: NONE
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __sdmSdioDrvAccessRequest (VOID)
{
    __SDM_DRV_LOCK();
}
/*********************************************************************************************************
** ��������: __sdmSdioDrvAccessRelease
** ��������: SDIO ������������ͷ�(��sdiobase ����ģ��ɼ�)
** ��    ��: NONE
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __sdmSdioDrvAccessRelease (VOID)
{
    __SDM_DRV_UNLOCK();
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_SDIO_EN > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
