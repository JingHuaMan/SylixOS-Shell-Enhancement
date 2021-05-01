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
** ��   ��   ��: sdiobaseDrv.c
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2014 �� 10 �� 26 ��
**
** ��        ��: sdio��������Դ�ļ�

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0) && (LW_CFG_SDCARD_SDIO_EN > 0)
#include "../core/sddrvm.h"
#include "../core/sdiodrvm.h"
#include "../core/sdcoreLib.h"
#include "../core/sdiocoreLib.h"
#include "../include/sddebug.h"
#include "sdiobaseDrv.h"
/*********************************************************************************************************
  sdiobase ˽������
*********************************************************************************************************/
struct __sdm_sdio_base {
    SDIO_INIT_DATA  SDMIOBASE_initdata;
    SDIO_DRV       *SDMIOBASE_psdiodrv;
    VOID           *SDMIOBASE_pvDevPriv;
};
typedef struct __sdm_sdio_base  __SDM_SDIO_BASE;
/*********************************************************************************************************
  ǰ������
*********************************************************************************************************/
static INT    __sdiobaseDevCreate(SD_DRV *psddrv, PLW_SDCORE_DEVICE psdcoredev, VOID **ppvDevPriv);
static INT    __sdiobaseDevDelete(SD_DRV *psddrv, VOID *pvDevPriv);

PLW_LIST_LINE __sdmSdioDrvHeader(VOID);
VOID          __sdmSdioDrvAccessRequest(VOID);
VOID          __sdmSdioDrvAccessRelease(VOID);

static INT    __sdiobaseDrvMatch(SDIO_FUNC *psdiofuncTbl, INT iFuncCnt, SDIO_DRV *psdiodrv);
static VOID   __sdiobaseMatchFuncIdSet(SDIO_INIT_DATA *psdioinitdata,
                                       SDIO_FUNC      *psdiofuncTbl,
                                       INT             iFuncCnt,
                                       SDIO_DRV       *psdiodrv);
static INT    __sdiobaseDrvMatchOne(SDIO_FUNC *psdiofunc, SDIO_DEV_ID *psdiodevid);

static INT    __sdiobasePreInit(SDIO_INIT_DATA *pinitdata, PLW_SDCORE_DEVICE psdcoredev);
static INT    __sdiobaseCommonInit(SDIO_INIT_DATA *pinitdata, UINT32 uiRealOcr);
static INT    __sdiobaseFuncInit(PLW_SDCORE_DEVICE psdcoredev, SDIO_FUNC *psdiofunc);
/*********************************************************************************************************
  sdio base ��������
*********************************************************************************************************/
static SD_DRV _G_sddrvSdioBase;
/*********************************************************************************************************
** ��������: API_SdioBaseDrvInstall
** ��������: ��װSDIO Base����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT API_SdioBaseDrvInstall (VOID)
{
    _G_sddrvSdioBase.SDDRV_cpcName        = SDDRV_SDIOB_NAME;
    _G_sddrvSdioBase.SDDRV_pfuncDevCreate = __sdiobaseDevCreate;
    _G_sddrvSdioBase.SDDRV_pfuncDevDelete = __sdiobaseDevDelete;

    API_SdmSdDrvRegister(&_G_sddrvSdioBase);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdiobaseDevCreate
** ��������: SDIO Base�豸���� ����
** ��    ��: psddrv       sd ����
**           psdcoredev   sd ���Ĵ������
**           ppvDevPriv   ���ڱ����豸�����ɹ�����豸˽������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT   __sdiobaseDevCreate (SD_DRV *psddrv, PLW_SDCORE_DEVICE psdcoredev, VOID **ppvDevPriv)
{
    __SDM_SDIO_BASE   *psdiobase;
    SDIO_INIT_DATA    *psdioinitdata;
    SDIO_DRV          *psdiodrv;
    PLW_LIST_LINE      plineTmp;
    INT                iRet;

    psdiobase= (__SDM_SDIO_BASE *)__SHEAP_ALLOC(sizeof(__SDM_SDIO_BASE));
    if (!psdiobase) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        return  (PX_ERROR);
    }

    psdioinitdata = &psdiobase->SDMIOBASE_initdata;
    *ppvDevPriv   = (VOID *)psdiobase;

    iRet = __sdiobasePreInit(psdioinitdata, psdcoredev);
    if (iRet != ERROR_NONE) {
        /*
         * ���Ԥ��ʼ��ʧ��, ˵������һ�� SDIO �豸, �������
         * ����� SDIO �豸����ƥ�乤��
         */
        goto    __err;
    }

    __sdmSdioDrvAccessRequest();
    for (plineTmp  = __sdmSdioDrvHeader();
         plineTmp != LW_NULL;
         plineTmp  = _list_line_get_next(plineTmp)) {

        psdiodrv = _LIST_ENTRY(plineTmp, SDIO_DRV, SDIODRV_lineManage);
        iRet = __sdiobaseDrvMatch(&psdioinitdata->INIT_psdiofuncTbl[0],
                                  psdioinitdata->INIT_iFuncCnt + 1,
                                  psdiodrv);
        if (iRet != ERROR_NONE) {
            /*
             * ��ǰ����ƥ��ʧ��,ƥ����һ��
             */
            continue;
        }

        __sdiobaseMatchFuncIdSet(psdioinitdata,
                                 &psdioinitdata->INIT_psdiofuncTbl[0],
                                 psdioinitdata->INIT_iFuncCnt + 1,
                                 psdiodrv);

        /*
         * ��Ϊ�ڽ������ľ��������豸���������п��ܻ�����ж��¼�
         * ������ǰ��ɴ����ж�ʱ��Ҫ������
         */
        psdiobase->SDMIOBASE_psdiodrv = psdiodrv;

        iRet = psdiodrv->SDIODRV_pfuncDevCreate(psdiodrv,
                                                psdioinitdata,
                                                &psdiobase->SDMIOBASE_pvDevPriv);
        if (iRet == ERROR_NONE) {
            API_AtomicInc(&psdiodrv->SDIODRV_atomicDevCnt);
            __sdmSdioDrvAccessRelease();
            return  (ERROR_NONE);

        } else {
            /*
             * ���ﲻ���ش���,��Ϊ���ܴ�����һ�� ID ƥ�������
             * ������ȫȡ����Ӧ�������ı�д��
             */
        }
    }
    __sdmSdioDrvAccessRelease();

__err:
    __SHEAP_FREE(psdiobase);

    *ppvDevPriv = LW_NULL;

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdiobaseDevDelete
** ��������: SDIO Base�豸ɾ�� ����
** ��    ��: psddrv       sd ����
**           pvDevPriv    �豸˽������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdiobaseDevDelete (SD_DRV *psddrv,  VOID *pvDevPriv)
{
    __SDM_SDIO_BASE   *psdiobase = (__SDM_SDIO_BASE *)pvDevPriv;
    SDIO_DRV          *psdioDrv;
    SDIO_FUNC         *psdiofunc;
    INT                i;

    if (!psdiobase) {
        return  (PX_ERROR);
    }

    psdioDrv = psdiobase->SDMIOBASE_psdiodrv;
    psdioDrv->SDIODRV_pfuncDevDelete(psdioDrv, psdiobase->SDMIOBASE_pvDevPriv);

    psdiofunc = &psdiobase->SDMIOBASE_initdata.INIT_psdiofuncTbl[0];
    for (i = 0; i < (psdiobase->SDMIOBASE_initdata.INIT_iFuncCnt + 1); i++) {
        API_SdioCoreDevFuncClean(psdiofunc);
        psdiofunc++;
    }

    __SHEAP_FREE(psdiobase);

    API_AtomicDec(&psdioDrv->SDIODRV_atomicDevCnt);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdiobaseDevIrqHandle
** ��������: SDIO Base�жϴ���(�ú�������SDMģ��ɼ�)
** ��    ��: psddrv       sd ����
**           pvDevPriv    �豸˽������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __sdiobaseDevIrqHandle (SD_DRV *psddrv,  VOID *pvDevPriv)
{
    __SDM_SDIO_BASE   *psdiobase = (__SDM_SDIO_BASE *)pvDevPriv;
    SDIO_DRV          *psdiodrv;

    if (!psdiobase) {
        return  (PX_ERROR);
    }

    psdiodrv = psdiobase->SDMIOBASE_psdiodrv;
    psdiodrv->SDIODRV_pfuncIrqHandle(psdiodrv, psdiobase->SDMIOBASE_pvDevPriv);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdiobasePreInit
** ��������: SDIO �豸Ԥ��ʼ��(���SDIO�豸��ͬ����Ϣ)
** ��    ��: pinitdata        �����ʼ������Ϣ
**           psdcoredev       �����豸�������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdiobasePreInit (SDIO_INIT_DATA *pinitdata, PLW_SDCORE_DEVICE psdcoredev)
{
    UINT32      uiOcr     = 0;
    UINT32      uiHostOcr = 0;
    SDIO_FUNC  *psdiofunc;
    INT         iRet;
    INT         i;

    if (!pinitdata || !psdcoredev) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        return  (PX_ERROR);
    }

    /*
     * first init the data
     */
    lib_bzero(pinitdata, sizeof(SDIO_INIT_DATA));

    psdiofunc = &pinitdata->INIT_psdiofuncTbl[0];
    for (i = 0; i < 8; i++) {
        psdiofunc->FUNC_uiNum            = i;
        psdiofunc->FUNC_cpsdiocccr       = &pinitdata->INIT_sdiocccr;
        psdiofunc->FUNC_ptupleListHeader = LW_NULL;

        psdiofunc++;
    }

    pinitdata->INIT_psdcoredev = psdcoredev;

    /*
     * do common init
     */
    API_SdCoreDevCtl(psdcoredev, SDBUS_CTRL_POWEROFF, 0);
    bspDelayUs(100000);
    API_SdCoreDevCtl(psdcoredev, SDBUS_CTRL_POWERON, 0);
    API_SdCoreDevCtl(psdcoredev, SDBUS_CTRL_SETCLK, SDARG_SETCLK_LOW);
    API_SdCoreDevCtl(psdcoredev, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_1);

    API_SdioCoreDevReset(psdcoredev);
    API_SdCoreDevReset(psdcoredev);

    iRet = API_SdioCoreDevSendIoOpCond(psdcoredev, 0, &uiOcr);      /*  ��ȡ�豸֧�ֵ�OCR               */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    uiOcr &= ~0x7f;                                                 /*  �������Э���ﱣ���ĵ�ѹ��Χ    */

    pinitdata->INIT_iFuncCnt = (uiOcr & 0x70000000) >> 28;

    API_SdCoreDevCtl(psdcoredev,
                     SDBUS_CTRL_GETOCR,
                     (LONG)&uiHostOcr);

    uiOcr = uiOcr & uiHostOcr;
    if (uiOcr == 0) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "dev ocr not match with host ocr.\r\n");
        return  (PX_ERROR);
    }

    iRet = __sdiobaseCommonInit(pinitdata, uiOcr);
    if (iRet != ERROR_NONE) {
        goto    __clean;
    }

    /*
     * TODO: disable card cd
     */

    /*
     * then init func 1~7
     */
    psdiofunc = &pinitdata->INIT_psdiofuncTbl[1];
    for (i = 0; i < pinitdata->INIT_iFuncCnt; i++) {
        iRet = __sdiobaseFuncInit(psdcoredev, psdiofunc);
        if (iRet != ERROR_NONE) {
            goto    __clean;
        }
        psdiofunc++;
    }

    return  (ERROR_NONE);

__clean:
    psdiofunc = &pinitdata->INIT_psdiofuncTbl[0];
    for (i = 0; i < (pinitdata->INIT_iFuncCnt + 1); i++) {
        API_SdioCoreDevFuncClean(psdiofunc);
        psdiofunc++;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdiobaseDoInit
** ��������: sdio һ���ʼ��
** ��    ��: pinitdata        �����ʼ������
**           uiRealOcr        ʵ��ʹ�õĵ�ѹ֧����Ϣ
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT   __sdiobaseCommonInit (SDIO_INIT_DATA *pinitdata, UINT32 uiRealOcr)
{
    PLW_SDCORE_DEVICE psdcoredev = pinitdata->INIT_psdcoredev;
    INT               iRet;
    UINT32            uiRca;

    iRet = API_SdioCoreDevSendIoOpCond(psdcoredev, uiRealOcr, LW_NULL);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

#if LW_CFG_SDCARD_CRC_EN > 0
    if (COREDEV_IS_SPI(psdcoredev)) {
        API_SdCoreDevSpiCrcEn(psdcoredev, LW_TRUE);
    }
#endif

    API_SdCoreDevTypeSet(psdcoredev, SDDEV_TYPE_SDIO);

    if (COREDEV_IS_SD(psdcoredev)) {
        iRet = API_SdCoreDevSendRelativeAddr(psdcoredev, &uiRca);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        API_SdCoreDevRcaSet(psdcoredev, uiRca);

        iRet = API_SdCoreDevSelect(psdcoredev);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    iRet = API_SdioCoreDevReadCCCR(psdcoredev, &pinitdata->INIT_sdiocccr);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * func0 : the common CIS
     */
    iRet = API_SdioCoreDevReadCis(psdcoredev, &pinitdata->INIT_psdiofuncTbl[0]);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * try to switch to high speed
     */
    API_SdioCoreDevHighSpeedEn(psdcoredev, &pinitdata->INIT_sdiocccr);

    if (COREDEV_IS_HIGHSPEED(psdcoredev)) {
        iRet = API_SdCoreDevCtl(psdcoredev,
                                SDBUS_CTRL_SETCLK,
                                SDARG_SETCLK_MAX);
        if (iRet != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "host clock set to max failed.\r\n");
        }
    } else {
        iRet = API_SdCoreDevCtl(psdcoredev,
                                SDBUS_CTRL_SETCLK,
                                pinitdata->INIT_psdiofuncTbl[0].FUNC_uiMaxDtr);
        if (iRet != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "host clock set to normal failed.\r\n");
        }
    }

    iRet = API_SdioCoreDevWideBusEn(psdcoredev, &pinitdata->INIT_sdiocccr);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdiobaseFuncInit
** ��������: ��ʼ��һ�� SDIO �豸��ָ������
** ��    ��: psdcoredev       �����豸�������
**           psdiofunc        ��Ҫ��ʼ���Ĺ���
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdiobaseFuncInit (PLW_SDCORE_DEVICE psdcoredev, SDIO_FUNC *psdiofunc)
{
    INT iRet;

    iRet = API_SdioCoreDevReadFbr(psdcoredev, psdiofunc);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_SdioCoreDevReadCis(psdcoredev, psdiofunc);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdiobaseDrvMatchDev
** ��������: sdio �豸����ƥ��.(ֻҪĳһ��������ĳһ�� ID ƥ���Ͼͳɹ�)
** ��    ��: psdiofuncTbl     �豸�Ĺ���������
**           iFuncCnt         �豸�Ĺ��ܸ���
**           psdiodrv         �豸��Ӧ������
** ��    ��: ERROR CODE(ƥ��ɹ����� ERROR_NONE, ���� PX_ERROR)
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdiobaseDrvMatch (SDIO_FUNC *psdiofuncTbl, INT iFuncCnt, SDIO_DRV *psdiodrv)
{
    SDIO_FUNC    *psdiofunc;
    SDIO_DEV_ID  *psdiodevid;
    INT           iIdCnt;
    INT           iRet;

    INT           iFunc;
    INT           iId;

    psdiofunc = psdiofuncTbl;
    iIdCnt    = psdiodrv->SDIODRV_iDevidCnt;
    for (iFunc = 0; iFunc < iFuncCnt; iFunc++) {

        psdiodevid = psdiodrv->SDIODRV_pdevidTbl;
        for (iId = 0; iId < iIdCnt; iId++) {
            iRet = __sdiobaseDrvMatchOne(psdiofunc, psdiodevid);
            if (iRet == ERROR_NONE) {
                return  (ERROR_NONE);
            }
            psdiodevid++;
        }

        psdiofunc++;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdiobaseMatchFuncIdSet
** ��������: ����ƥ��Ĺ��� ID
** ��    ��: psdioinitdata    SDIO ��ʼ������
**           psdiofuncTbl     �豸�Ĺ���������
**           iFuncCnt         �豸�Ĺ��ܸ���
**           psdiodrv         �豸��Ӧ������
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdiobaseMatchFuncIdSet (SDIO_INIT_DATA *psdioinitdata,
                                      SDIO_FUNC      *psdiofuncTbl,
                                      INT             iFuncCnt,
                                      SDIO_DRV       *psdiodrv)
{
    SDIO_FUNC    *psdiofunc;
    SDIO_DEV_ID  *psdiodevid;
    INT           iIdCnt;
    INT           iRet;

    INT           iFunc;
    INT           iId;

    psdiofunc = psdiofuncTbl;
    iIdCnt    = psdiodrv->SDIODRV_iDevidCnt;
    for (iFunc = 0; iFunc < iFuncCnt; iFunc++) {

        psdiodevid = psdiodrv->SDIODRV_pdevidTbl;
        for (iId = 0; iId < iIdCnt; iId++) {
            iRet = __sdiobaseDrvMatchOne(psdiofunc, psdiodevid);
            if (iRet == ERROR_NONE) {
                psdioinitdata->INIT_pdevidCurr[iFunc] = psdiodevid;
                break;
            }
            psdiodevid++;
        }

        psdiofunc++;
    }
}
/*********************************************************************************************************
** ��������: __sdiobaseDrvMatchOne
** ��������: һ��������һ��ID��ƥ��
** ��    ��: psdiofunc        sdio ������������
**           psdiodevid       �豸 ID
** ��    ��: ERROR CODE(ƥ��ɹ����� ERROR_NONE, ���� PX_ERROR)
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdiobaseDrvMatchOne (SDIO_FUNC *psdiofunc, SDIO_DEV_ID *psdiodevid)
{
    if ((psdiodevid->DEVID_ucClass != (UINT8)SDIO_DEV_ID_ANY) &&
        (psdiodevid->DEVID_ucClass != psdiofunc->FUNC_ucClass)) {
        return  (PX_ERROR);
    }

    if ((psdiodevid->DEVID_usVendor != (UINT8)SDIO_DEV_ID_ANY) &&
        (psdiodevid->DEVID_usVendor != psdiofunc->FUNC_usVendor)) {
        return  (PX_ERROR);
    }
    if ((psdiodevid->DEVID_usDevice != (UINT8)SDIO_DEV_ID_ANY) &&
        (psdiodevid->DEVID_usDevice != psdiofunc->FUNC_usDevice)) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_SDIO_EN > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
