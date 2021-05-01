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
** ��   ��   ��: sdcore.c
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2010 �� 11 �� 23 ��
**
** ��        ��: sd ���ں�Э���ӿ�Դ�ļ�

** BUG:
2010.11.25  ���Ӷ� SPI �豸�ķ�װ.���Ӽ����豸��Ϣ�鿴 API.
2010.12.08  ���Ӷ� SDHC ��֧��.
2011.01.10  ���Ӷ� SPI ��֧��.
2011.02.21  ���� API_SdCoreSpiSendIfCond ����.�ú���ֻ������ SPI ģʽ��.
2011.02.21  �� SPI ���豸�Ĵ����Ķ�ȡ����: API_SdCoreSpiRegisterRead().
2011.02.22  �� SPI �� CRC У��.
2011.03.25  �޸� API_SdCoreDevCreate(), ���ڵײ�������װ�ϲ�Ļص�.
2011.04.02  ȫ���޸� SPI ģʽ�µ���غ���,ʹ���µ� SPI ģ��.
2014.11.05  �豸��״̬��������������.
2016.01.21  ���� SPI ģʽ�µ����߿��Ʋ����Լ�Э����صĴ���.
2017.01.06  ���� SPI ģʽ�·���������ش���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "sdstd.h"
#include "sdcrc.h"
#include "sdcore.h"
#include "../include/sddebug.h"
/*********************************************************************************************************
  SPIͨ������
*********************************************************************************************************/
#define __SD_SPI_BITS_PEROP   8
#define __SD_SPI_CLKMOD_FLG   LW_SPI_M_CPOL_0  | LW_SPI_M_CPHA_0  |  \
                              LW_SPI_M_CPOL_EN | LW_SPI_M_CPHA_EN       /*  SPIͨ��ʹ�õ�ʱ��ģʽ       */

#define __SD_SPI_TMOD_RD      __SD_SPI_CLKMOD_FLG | LW_SPI_M_WRBUF_FIX  /*  ���������Ʊ��              */
#define __SD_SPI_TMOD_WR      __SD_SPI_CLKMOD_FLG | LW_SPI_M_RDBUF_FIX  /*  д�������Ʊ��              */

#define __SD_SPI_RSP_TIMEOUT  0x3fffff                                  /*  SPIӦ��ʱ������         */
#define __SD_SPI_RSP_WAITSEC  4                                         /*  SPIӦ��ʱ�������ʱ��     */
#define __SD_SPI_DAT_WAITSEC  4                                         /*  SPI���ݱ�̳�ʱ�������ʱ�� */

#define __SD_SPI_CLK_LOW      400000                                    /*  һ��ʱ��, 400k              */
#define __SD_SPI_CLK_MAX      25000000                                  /*  ����ʱ��, 25m               */
/*********************************************************************************************************
  SPI ��������\�ͷ�
*********************************************************************************************************/
#define __SD_SPI_CSEN(pcsdev)           (pcsdev)->CSPIDEV_cbCsEn((pcsdev)->CSPIDEV_psdcorecha)
#define __SD_SPI_CSDIS(pcsdev)          (pcsdev)->CSPIDEV_cbCsDis((pcsdev)->CSPIDEV_psdcorecha)
/*********************************************************************************************************
  SD ���Ĳ��豸������
*********************************************************************************************************/
#define __CORE_DEV_TXF(cd, pmsg, num)   cd->COREDEV_pfuncCoreDevXfer(cd->COREDEV_pvDevHandle,\
                                                                     pmsg, num)
#define __CORE_DEV_CTL(cd, icmd, larg)  cd->COREDEV_pfuncCoreDevCtl(cd->COREDEV_pvDevHandle, \
                                                                    icmd, larg)
#define __CORE_DEV_DEL(cd)              cd->COREDEV_pfuncCoreDevDelet(cd->COREDEV_pvDevHandle)
/*********************************************************************************************************
   ���Ĳ��װ��SPI�豸
*********************************************************************************************************/
typedef struct __core_spi_dev {
    PLW_SPI_DEVICE                CSPIDEV_pspiDev;
    UINT8                         CSPIDEV_ucType;
    UINT32                        CSPIDEV_uiState;                      /*  �豸״̬λ��                */

    LW_SDDEV_CID                  CSPIDEV_cid;
    LW_SDDEV_CSD                  CSPIDEV_csd;
    LW_SDDEV_SCR                  CSPIDEV_scr;
    LW_SDDEV_SW_CAP               CSPIDEV_swcap;

    PLW_SDCORE_CHAN               CSPIDEV_psdcorecha;
    SDCORE_CALLBACK_SPICS_ENABLE  CSPIDEV_cbCsEn;
    SDCORE_CALLBACK_SPICS_DISABLE CSPIDEV_cbCsDis;
} __CORE_SPI_DEV, *__PCORE_SPI_DEV;
/*********************************************************************************************************
  ˽�нӿ�����
*********************************************************************************************************/
static INT  __sdCoreSpiDevTransfer(PVOID pvDevHandle, PLW_SD_MESSAGE psdmsg, INT iNum);
static INT  __sdCoreSdDevTransfer(PVOID pvDevHandle, PLW_SD_MESSAGE psdmsg, INT iNum);

static INT  __sdCoreSpiDevCtl(PVOID  pvDevHandle, INT  iCmd, LONG lArg);
static INT  __sdCoreSdDevCtl(PVOID  pvDevHandle, INT  iCmd, LONG lArg);

static INT  __sdCoreSpiDevDelet(PVOID pvDevHandle);
static INT  __sdCoreSdDevDelet(PVOID pvDevHandle);

static INT  __sdCoreSpiCallbackCheckDev(PVOID pvDevHandle, INT iDevSta);
static INT  __sdCoreSdCallbackCheckDev(PVOID pvDevHandle, INT iDevSta);

static INT  __sdCoreCallbackCheckDev(PLW_SDCORE_DEVICE psdcoredevice, INT iDevSta);

static INT  __sdCoreDevSwitchToApp(PLW_SDCORE_DEVICE psdcoredevice, BOOL bIsBc);
/*********************************************************************************************************
  ����˽�нӿ�Ϊsd��spi������Ƶķ�װ
*********************************************************************************************************/
static INT  __sdCoreSpiCmd(__PCORE_SPI_DEV pcspidevice, LW_SD_COMMAND *psdcmd);
static INT  __sdCoreSpiDataRd(__PCORE_SPI_DEV pcspidevice,
                              UINT32          uiBlkNum,
                              UINT32          uiBlkLen,
                              UINT8          *pucRdBuff);
static INT  __sdCoreSpiDataWrt(__PCORE_SPI_DEV pcspidevice,
                               UINT32          uiBlkNum,
                               UINT32          uiBlkLen,
                               UINT8          *pucWrtBuff);

static INT  __sdCoreSpiBlkRd(__PCORE_SPI_DEV pcspidevice,
                             UINT32          uiBlkLen,
                             UINT8          *pucRdBuff);
static INT  __sdCoreSpiBlkWrt(__PCORE_SPI_DEV pcspidevice,
                              UINT32          uiBlkLen,
                              UINT8          *pucWrtBuff,
                              BOOL            bIsMul);

static INT  __sdCoreSpiByteRd(__PCORE_SPI_DEV pcspidevice,
                              UINT32          uiLen,
                              UINT8          *pucRdBuff);
static INT  __sdCoreSpiByteWrt(__PCORE_SPI_DEV pcspidevice,
                               UINT32          uiLen,
                               UINT8          *pucWrtBuff);
static INT  __sdCoreSpiWaitBusy(__PCORE_SPI_DEV pcspidevice);
static VOID __sdCoreSpiMulWrtStop(__PCORE_SPI_DEV pcspidevice);
/*********************************************************************************************************
  SPI ���ߺ���
*********************************************************************************************************/
static VOID  __sdCoreSpiParamConvert(UINT8 *pucParam, UINT32 uiParam);
static VOID  __sdCoreSpiRespConvert(UINT32 *puiResp, const UINT8 *pucResp, INT iRespLen);
static INT   __sdCoreSpiRespLen(UINT32   uiCmdFlg);
/*********************************************************************************************************
** ��������: API_SdCoreDevCreate
** ��������: ����һ������SD�豸
** ��    ��: iAdapterType     �豸�ҽӵ����������� (SDADAPTER_TYPE_SPI �� SDADAPTER_TYPE_SD)
**           pcAdapterName    �ҽӵ�����������
**           pcDeviceName     �豸����
**           psdcorechan      ͨ��
** ��    ��: NONE
** ��    ��: �ɹ�,�����豸�豸ָ��,���򷵻�LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API PLW_SDCORE_DEVICE  API_SdCoreDevCreate (INT                       iAdapterType,
                                               CPCHAR                    pcAdapterName,
                                               CPCHAR                    pcDeviceName,
                                               PLW_SDCORE_CHAN           psdcorechan)
{
    PLW_SDCORE_DEVICE   psdcoredevice   = LW_NULL;
    PLW_SD_DEVICE       psddevice       = LW_NULL;
    PLW_SPI_DEVICE      pspidevice      = LW_NULL;
    __PCORE_SPI_DEV     pcspidevice     = LW_NULL;

    if (!pcAdapterName || !pcDeviceName || !psdcorechan) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    switch (iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = API_SdDeviceCreate(pcAdapterName, pcDeviceName);
        if (!psddevice) {
            goto    __createdev_failed;
        }
        psddevice->SDDEV_uiState |= SD_STATE_EXIST;                     /*  ���豸�Ѿ�����(�˴��ܹؼ�)  */

        psdcoredevice = (PLW_SDCORE_DEVICE)__SHEAP_ALLOC(sizeof(LW_SDCORE_DEVICE));
        if (!psdcoredevice) {
            API_SdDeviceDelete(psddevice);
            goto    __allocdev_failed;
        }
        lib_bzero(psdcoredevice, sizeof(LW_SDCORE_DEVICE));
        
        psdcoredevice->COREDEV_pvDevHandle        = (PVOID)psddevice;
        psdcoredevice->COREDEV_pfuncCoreDevXfer   = __sdCoreSdDevTransfer;
        psdcoredevice->COREDEV_pfuncCoreDevCtl    = __sdCoreSdDevCtl;
        psdcoredevice->COREDEV_pfuncCoreDevDelet  = __sdCoreSdDevDelet;
        psdcoredevice->COREDEV_iAdapterType       = SDADAPTER_TYPE_SD;

        LW_SPIN_INIT(&psdcoredevice->COREDEV_slLock);

        if (SDCORE_CHAN_INSTALL(psdcorechan)) {                         /*  ��װ�ص�                    */
            SDCORE_CHAN_CBINSTALL(psdcorechan,
                                  SD_CALLBACK_CHECK_DEV,
                                  __sdCoreCallbackCheckDev,
                                  psdcoredevice);
        } else {
            __SHEAP_FREE(psdcoredevice);
            API_SdDeviceDelete(psddevice);
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "need callback of install.\r\n");
            return  (LW_NULL);
        }
        return  (psdcoredevice);

    case SDADAPTER_TYPE_SPI:
        if (!SDCORE_CHAN_SPICSEN(psdcorechan) ||
            !SDCORE_CHAN_SPICSDIS(psdcorechan)) {                       /*  spi�±�����Ƭѡ�ص�         */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "need callback of spi card select in spi mode.\r\n");
            _ErrorHandle(EINVAL);
            return  (LW_NULL);
        }

        pspidevice = API_SpiDeviceCreate(pcAdapterName, pcDeviceName);  /*  ����spi�豸                 */
        if (!pspidevice) {
            goto    __createdev_failed;
        }
        pcspidevice = (__PCORE_SPI_DEV)__SHEAP_ALLOC(sizeof(__CORE_SPI_DEV));
        if (!pcspidevice) {                                             /*  ����core spi �豸           */
            API_SpiDeviceDelete(pspidevice);
            goto    __allocdev_failed;
        }
        lib_bzero(pcspidevice, sizeof(__CORE_SPI_DEV));
        
        pcspidevice->CSPIDEV_uiState = SD_DEVSTA_EXIST;                 /*  ���豸�Ѿ�����(�˴��ܹؼ�)  */

        psdcoredevice = (PLW_SDCORE_DEVICE)__SHEAP_ALLOC(sizeof(LW_SDCORE_DEVICE));
        if (!psdcoredevice) {                                           /*  ���������豸                */
            API_SpiDeviceDelete(pspidevice);
            __SHEAP_FREE(pcspidevice);
            goto    __allocdev_failed;
        }

        pcspidevice->CSPIDEV_pspiDev    = pspidevice;                   /*  ��                        */
        pcspidevice->CSPIDEV_psdcorecha = psdcorechan;                  /*  ����ͨ��                    */
        pcspidevice->CSPIDEV_cbCsEn     = SDCORE_CHAN_SPICSEN(psdcorechan);
        pcspidevice->CSPIDEV_cbCsDis    = SDCORE_CHAN_SPICSDIS(psdcorechan);
                                                                        /*  ����Ƭѡ�ص�                */
        lib_bzero(psdcoredevice, sizeof(LW_SDCORE_DEVICE));
        
        psdcoredevice->COREDEV_pvDevHandle        = (PVOID)pcspidevice;
        psdcoredevice->COREDEV_pfuncCoreDevXfer   = __sdCoreSpiDevTransfer;
        psdcoredevice->COREDEV_pfuncCoreDevCtl    = __sdCoreSpiDevCtl;
        psdcoredevice->COREDEV_pfuncCoreDevDelet  = __sdCoreSpiDevDelet;
        psdcoredevice->COREDEV_iAdapterType       = SDADAPTER_TYPE_SPI;

        LW_SPIN_INIT(&psdcoredevice->COREDEV_slLock);

        if (SDCORE_CHAN_INSTALL(psdcorechan)) {                         /*  ��װ�ص�����                */
            SDCORE_CHAN_CBINSTALL(psdcorechan,
                                  SD_CALLBACK_CHECK_DEV,
                                  __sdCoreCallbackCheckDev,
                                  psdcoredevice);
        } else {
            __SHEAP_FREE(psdcoredevice);
            __SHEAP_FREE(pcspidevice);
            API_SpiDeviceDelete(pspidevice);
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "callback install error.\r\n");
            return  (LW_NULL);
        }
        return  (psdcoredevice);

    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "adapter type error.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

__createdev_failed:
    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "create device(sd/spi) failed.\r\n");
    return  (LW_NULL);

__allocdev_failed:
    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
    _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevDelete
** ��������: ɾ��һ������SD�豸
** ��    ��: psdcoredevice �豸�ṹָ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevDelete (PLW_SDCORE_DEVICE    psdcoredevice)
{
    INT     iError;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     * ����ɾ����Ӧ��SPI��SD�豸
     */
    iError = __CORE_DEV_DEL(psdcoredevice);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "delete device(sd/spi) failed.\r\n");
        return  (PX_ERROR);
    }

    /*
     * ���ͷ�core�豸
     */
    __SHEAP_FREE(psdcoredevice);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevCtl
** ��������: �����豸����
** ��    ��: psdcoredevice �豸�ṹָ��
**           iCmd          ��������
**           lArg          �������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevCtl (PLW_SDCORE_DEVICE    psdcoredevice,
                              INT                  iCmd,
                              LONG                 lArg)
{
    INT     iError;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iError = API_SdCoreDevStaView(psdcoredevice);
    if (iError == SD_DEVSTA_UNEXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "dev is not exist.\r\n");
        return  (PX_ERROR);
    }

    iError = __CORE_DEV_CTL(psdcoredevice, iCmd, lArg);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "ctrl of device(sd/spi) failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevTransfer
** ��������: �����豸����
** ��    ��: psdcoredevice �豸�ṹָ��
**           psdmsg        ������Ϣ
**           iNum          ��Ϣ����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevTransfer (PLW_SDCORE_DEVICE  psdcoredevice,
                                   PLW_SD_MESSAGE     psdmsg,
                                   INT                iNum)
{
    INT     iError;

    if (!psdcoredevice || !psdmsg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iError = API_SdCoreDevStaView(psdcoredevice);
    if (iError == SD_DEVSTA_UNEXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "dev is not exist.\r\n");
        return  (PX_ERROR);
    }

    iError = __CORE_DEV_TXF(psdcoredevice, psdmsg, iNum);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request of device(sd/spi) failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevCmd
** ��������: �����豸��������
** ��    ��: psdcoredevice �豸�ṹָ��
**           psdcmd        ����
**           uiRetry       ���Լ���
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevCmd (PLW_SDCORE_DEVICE psdcoredevice,
                              PLW_SD_COMMAND    psdcmd,
                              UINT32            uiRetry)
{
    LW_SD_MESSAGE   sdmsg;
    INT             iError;

    if (!psdcoredevice || !psdcmd) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    lib_bzero(&sdmsg, sizeof(sdmsg));
    lib_bzero(psdcmd->SDCMD_uiResp, sizeof(psdcmd->SDCMD_uiResp));
    
    psdcmd->SDCMD_uiRetry = uiRetry;
    sdmsg.SDMSG_psdcmdCmd = psdcmd;

    iError = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "send cmd%d error.\r\n", psdcmd->SDCMD_uiOpcode);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevAppSwitch
** ��������: �л���APP����
** ��    ��: psdcoredevice  ���Ľṹָ��
**           bIsBc          �Ƿ���Թ㲥����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevAppSwitch (PLW_SDCORE_DEVICE psdcoredevice, BOOL bIsBc)
{
    INT iRet;

    iRet = __sdCoreDevSwitchToApp(psdcoredevice, bIsBc);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevAppCmd
** ��������: �����豸����APP����
** ��    ��: psdcoredevice  �豸�ṹָ��
**           psdcmdAppCmd   Ӧ������
**           bIsBc          �Ƿ��ǹ㲥����
**           uiRetry        ���Լ���
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdCoreDevAppCmd (PLW_SDCORE_DEVICE psdcoredevice,
                                  PLW_SD_COMMAND    psdcmdAppCmd,
                                  BOOL              bIsBc,
                                  UINT32            uiRetry)
{
    INT     iError = PX_ERROR;
    INT     i;

    /*
     * ����Ӧ������,ÿ�η���ǰ��Ҫʹ��CMD55�л���Ӧ������״̬.
     * �л�ʧ��ԭ���кܶ�,��˿��Զ��Լ���.
     */
    for (i = 0; i <= uiRetry; i++) {
       iError = __sdCoreDevSwitchToApp(psdcoredevice, bIsBc);
       if (iError != ERROR_NONE) {
           continue;
       }

       iError = API_SdCoreDevCmd(psdcoredevice, psdcmdAppCmd, 0);
       break;
    }

    if (i > uiRetry) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "timeout.\r\n");
        iError = PX_ERROR;
    } else if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send app cmd error.\r\n");
    }

    return  (iError);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevAdapterName
** ��������: ���ض�Ӧ����������������
** ��    ��: psdcoredevice  �豸�ṹָ��
** ��    ��:
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API CPCHAR  API_SdCoreDevAdapterName (PLW_SDCORE_DEVICE psdcoredevice)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;
    LW_BUS_ADAPTER  *pBusAdapter;

    if (!psdcoredevice) {
        return  (LW_NULL);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice   = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        pBusAdapter = &psddevice->SDDEV_psdAdapter->SDADAPTER_busadapter;
        break;

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        pBusAdapter = &pcspidevice->CSPIDEV_pspiDev->SPIDEV_pspiadapter->SPIADAPTER_pbusadapter;
        break;

    default:
        return  (LW_NULL);
    }

    return  ((CPCHAR)pBusAdapter->BUSADAPTER_cName);
}
/*********************************************************************************************************
** ��������: API_SdCoreDevCsdView
** ��������: �鿴�豸��CSD
** ��    ��: psdcoredevice �豸�ṹָ��
** ��    ��: psdcsd        �豸CSD
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdCoreDevCsdView (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CSD psdcsd)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdcsd, &psddevice->SDDEV_csd, sizeof(LW_SDDEV_CSD));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdcsd, &pcspidevice->CSPIDEV_csd, sizeof(LW_SDDEV_CSD));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevCidView
** ��������: �鿴�豸��CID
** ��    ��: psdcoredevice �豸�ṹָ��
** ��    ��: psdcid        �豸CID
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevCidView (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CID psdcid)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdcid) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdcid, &psddevice->SDDEV_cid, sizeof(LW_SDDEV_CID));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdcid, &pcspidevice->CSPIDEV_cid, sizeof(LW_SDDEV_CID));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevScrView
** ��������: �鿴�豸��SCR
** ��    ��: psdcoredevice �豸�ṹָ��
** ��    ��: psdscr        �豸SCR
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevScrView (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_SCR psdscr)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdscr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdscr, &psddevice->SDDEV_scr, sizeof(LW_SDDEV_SCR));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdscr, &pcspidevice->CSPIDEV_scr, sizeof(LW_SDDEV_SCR));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevSwCapView
** ��������: �鿴�豸�� SWITCH ����
** ��    ��: psdcoredevice �豸�ṹָ��
** ��    ��: psdswcap      SWITCH ����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevSwCapView (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_SW_CAP psdswcap)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdswcap) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdswcap, &psddevice->SDDEV_swcap, sizeof(LW_SDDEV_SW_CAP));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdswcap, &pcspidevice->CSPIDEV_swcap, sizeof(LW_SDDEV_SW_CAP));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevRcaView
** ��������: �鿴�豸�ı��ص�ַ(ֻ������SD�����ϵ��豸)
** ��    ��: psdcoredevice �豸�ṹָ��
** ��    ��: puiRCA       �豸RCA
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevRcaView (PLW_SDCORE_DEVICE psdcoredevice,  UINT32   *puiRCA)
{
    PLW_SD_DEVICE    psddevice;

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        *puiRCA = psddevice->SDDEV_uiRCA;
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        *puiRCA = 0;                                                    /*  ������rca set ��������ͻ    */
        return  (ERROR_NONE);

    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevTypeView
** ��������: �鿴�豸������(MMC\SDSC\SDXC\SDHC\SDIO\COMM)
** ��    ��: psdcoredevice �豸�ṹָ��
** ��    ��: pucType     �豸����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdCoreDevTypeView (PLW_SDCORE_DEVICE psdcoredevice,  UINT8  *pucType)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !pucType) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
         psddevice  = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        *pucType    = psddevice->SDDEV_ucType;
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
         pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        *pucType     = pcspidevice->CSPIDEV_ucType;
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevCsdSet
** ��������: �����豸��CSD
** ��    ��: psdcoredevice �豸�ṹָ��
**           psdcsd        �豸CSD
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevCsdSet (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CSD psdcsd)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdcsd) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(&psddevice->SDDEV_csd, psdcsd, sizeof(LW_SDDEV_CSD));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(&pcspidevice->CSPIDEV_csd, psdcsd, sizeof(LW_SDDEV_CSD));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevCidSet
** ��������: �����豸��CID
** ��    ��: psdcoredevice �豸�ṹָ��
**           psdcid        �豸CID
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevCidSet (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CID psdcid)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdcid) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(&psddevice->SDDEV_cid, psdcid,  sizeof(LW_SDDEV_CID));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy( &pcspidevice->CSPIDEV_cid, psdcid, sizeof(LW_SDDEV_CID));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevScrSet
** ��������: �����豸��SCR
** ��    ��: psdcoredevice �豸�ṹָ��
**           psdscr        �豸SCR
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevScrSet (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_SCR psdscr)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdscr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(&psddevice->SDDEV_scr, psdscr,  sizeof(LW_SDDEV_SCR));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy( &pcspidevice->CSPIDEV_scr, psdscr, sizeof(LW_SDDEV_SCR));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevSwCapSet
** ��������: �����豸�� SWITCH ����
** ��    ��: psdcoredevice �豸�ṹָ��
**           psdswcap      SWITCH ����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevSwCapSet (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_SW_CAP psdswcap)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdswcap) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(&psddevice->SDDEV_swcap, psdswcap,  sizeof(LW_SDDEV_SW_CAP));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(&pcspidevice->CSPIDEV_swcap, psdswcap, sizeof(LW_SDDEV_SW_CAP));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevRcaSet
** ��������: �����豸��RCA(ֻ������SD���ߵ��豸)
** ��    ��: psdcoredevice �豸�ṹָ��
**           uiRCA       �豸RCA
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevRcaSet (PLW_SDCORE_DEVICE psdcoredevice,  UINT32  uiRCA)
{
    PLW_SD_DEVICE    psddevice;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        psddevice->SDDEV_uiRCA = uiRCA;
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "in spi mode, device has no rca .\r\n");
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevTypeSet
** ��������: �����豸������(MMC\SDSC\SDXC\SDHC\SDIO\COMM)
** ��    ��: psdcoredevice �豸�ṹָ��
**           iType         �豸����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdCoreDevTypeSet (PLW_SDCORE_DEVICE psdcoredevice,  UINT8  ucType)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || (ucType > SDDEV_TYPE_MAXVAL)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        psddevice->SDDEV_ucType = ucType;
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        pcspidevice->CSPIDEV_ucType = ucType;
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_SdCoreDevStaView
** ��������: �鿴�豸��״̬
** ��    ��: psdcoredevice �豸�ṹָ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdCoreDevStaView (PLW_SDCORE_DEVICE  psdcoredevice)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;
    INT              iSta;
    INTREG           iregInterLevel;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&psdcoredevice->COREDEV_slLock, &iregInterLevel);
    if (COREDEV_IS_SD(psdcoredevice)) {
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        if (psddevice->SDDEV_uiState & SD_STATE_EXIST) {
            iSta = SD_DEVSTA_EXIST;
        } else {
            iSta = SD_DEVSTA_UNEXIST;
        }

    } else {
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        iSta = pcspidevice->CSPIDEV_uiState;
    }
    LW_SPIN_UNLOCK_QUICK(&psdcoredevice->COREDEV_slLock, iregInterLevel);

    return  (iSta);
}
/*********************************************************************************************************
** ��������: API_SdCoreSpiCxdFormat
** ��������: SPI �µ�CID��CSD���ݸ�ʽ������(16�ֽ�ת 4*4 word)
** ��    ��: puiCxdOut   ���
**           pucRawCxd   ԭʼ�ֽ�
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API VOID API_SdCoreSpiCxdFormat (UINT32 *puiCxdOut, UINT8 *pucRawCxd)
{
    *puiCxdOut   = *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;                    /*  0                           */

     puiCxdOut++;
    *puiCxdOut   = *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;                    /*  1                           */

     puiCxdOut++;
    *puiCxdOut   = *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;                    /*  2                           */

     puiCxdOut++;
    *puiCxdOut   = *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;                    /*  3                           */
}
/*********************************************************************************************************
** ��������: API_SdCoreSpiMulWrtStop
** ��������: SPI �µĶ��д����ֹͣ����
** ��    ��: psdcoredevice sd�ں��豸ָ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API VOID  API_SdCoreSpiMulWrtStop (PLW_SDCORE_DEVICE psdcoredevice)
{
    UINT8           pucWrtTmp[3] = {0xff, SD_SPITOKEN_STOP_MULBLK, 0xff};
    INT             iError;
    __PCORE_SPI_DEV pcspidevice;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return;
    }

    pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
    if (!pcspidevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no core spi device.\r\n");
        return;
    }

    iError = API_SpiDeviceBusRequest(pcspidevice->CSPIDEV_pspiDev); 
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
        return;
    }
    __SD_SPI_CSEN(pcspidevice);                                    

    __sdCoreSpiByteWrt(pcspidevice, 3, pucWrtTmp);

    iError = __sdCoreSpiWaitBusy(pcspidevice);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait data program timeout.\r\n");
    }

    __SD_SPI_CSDIS(pcspidevice);
    API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);
}
/*********************************************************************************************************
** ��������: API_SdCoreSpiSendIfCond
** ��������: SPI ��cmd8����.��Ϊ��������ҪcrcУ��,����Ӧ��Ҳ��ͬ,��˲�ͬ��sdģʽ�µ�cmd8
** ��    ��: psdcoredevice sd�ں��豸ָ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreSpiSendIfCond (PLW_SDCORE_DEVICE psdcoredevice)
{
    UINT8           ucBuf[6] = {
                         8 + 0x40,                                      /*  ����                        */
                         0x00, 0x00, 0x01, 0xaa,                        /*  ����                        */
                         0x87,                                          /*  crc                         */
                    };

    UINT8           ucTmp;
    INT             iError;
    INT             iRetry;

    LW_SPI_MESSAGE  spimsg;
    __PCORE_SPI_DEV pcspidev;

    struct timespec   tvOld;
    struct timespec   tvNow;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    spimsg.SPIMSG_pfuncComplete = LW_NULL;

    pcspidev = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
    if (!pcspidev) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no core spi device.\r\n");
        return  (PX_ERROR);
    }

    iError = API_SpiDeviceBusRequest(pcspidev->CSPIDEV_pspiDev);        /*  ����spi����                 */
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
        return  (PX_ERROR);
    }
    __SD_SPI_CSEN(pcspidev);                                            /*  ʹ��Ƭѡ                    */

    /*
     * д�����������crc
     */
    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = ucBuf;
    spimsg.SPIMSG_pucRdBuffer = &ucTmp;
    spimsg.SPIMSG_uiLen       = 6;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_WR;
    iError = API_SpiDeviceTransfer(pcspidev->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        goto    __spibus_release;
    }

    /*
     * �ȴ���Ӧ
     */
    iRetry                    = 0;
    ucTmp                     = 0xff;
    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = &ucTmp;
    spimsg.SPIMSG_pucRdBuffer = ucBuf;
    spimsg.SPIMSG_uiLen       = 1;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_RD;

    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
    
    while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
        iError = API_SpiDeviceTransfer(pcspidev->CSPIDEV_pspiDev,
                                       &spimsg,
                                       1);
        if (iError == PX_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get response error.\r\n");
            goto    __spibus_release;
        }
        if (!(ucBuf[0] & 0x80)) {
            goto    __resp_accept;
        }
        
        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {    /*  ��ʱ�˳�                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get response timeout.\r\n");
            goto    __spibus_release;
        }
    }

__resp_accept:

    ucTmp                     = 0xff;
    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = &ucTmp;
    spimsg.SPIMSG_pucRdBuffer = ucBuf;
    spimsg.SPIMSG_uiLen       = 4;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_RD;
    iError = API_SpiDeviceTransfer(pcspidev->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get response error.\r\n");
        goto    __spibus_release;
    }

    if (ucBuf[3] != 0xaa) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "check pattern error.\r\n");
        goto    __spibus_release;
    }

    __SD_SPI_CSDIS(pcspidev);                                           /*  ����Ƭѡ                    */
    API_SpiDeviceBusRelease(pcspidev->CSPIDEV_pspiDev);                 /*  �ͷ�spi����                 */
    return  (ERROR_NONE);                                               /*  �ɹ�����                    */

__spibus_release:
    __SD_SPI_CSDIS(pcspidev);                                           /*  ����Ƭѡ                    */
    API_SpiDeviceBusRelease(pcspidev->CSPIDEV_pspiDev);                 /*  �ͷ�spi����                 */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_SdCoreSpiRegisterRead
** ��������: SPI �¶�ȡ�Ĵ�����ֵ.���ȡ���ݲ�ͬ����,�е�sd���豸��һ�����ȷ���һ��������ʼ����(0xfe),��
**           ��ֱ�ӷ�����Ĵ�����ֵ,���е��豸ȴ�ᷢ����ʼ����.�������ݵĶ�ȡ,�豸һ�������ȷ���������ʼ
**           ����.
** ��    ��: psdcoredevice sd�ں��豸ָ��
**           pucReg        ��ȡ�������
**           uiLen         �Ĵ�������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdCoreSpiRegisterRead (PLW_SDCORE_DEVICE  psdcoredevice,
                                       UINT8             *pucReg,
                                       UINT               uiLen)
{
    __PCORE_SPI_DEV   pcspidevice;
    UINT8             ucRdToken;
    INT               iRetry   = 0;
    UINT8             ucWrtClk = 0xff;
    INT               iError;
    UINT8             pucCrc16[2];

    struct timespec   tvOld;
    struct timespec   tvNow;

    if (!psdcoredevice || !pucReg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
    if (!pcspidevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no spi device error.\r\n");
        return  (PX_ERROR);

    }

    iError = API_SpiDeviceBusRequest(pcspidevice->CSPIDEV_pspiDev);     /*  ����spi����                 */
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
        return  (PX_ERROR);
    }
    __SD_SPI_CSEN(pcspidevice);                                         /*  ʹ��Ƭѡ                    */

    __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtClk);                      /*  sync clock                  */

    /*
     * �ȴ�������ʼ����.
     */
    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
    
    while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
        __sdCoreSpiByteRd(pcspidevice, 1, &ucRdToken);
        if (ucRdToken != 0xff) {
            goto    __resp_accept;
        }
        
        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {    /*  ��ʱ�˳�                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait read start token timeout.\r\n");
            goto    __spibus_release;
        }
    }

__resp_accept:
    /*
     * ������ʼ����, ˵�����ֽھ���Ҫ���ļĴ�����ֵ.
     * ���һ�ʣ��iLen - 1���ֽ�Ҫ��ȡ.
     */
    if (ucRdToken != SD_SPITOKEN_START_SIGBLK) {
        uiLen--;
        *pucReg++ = ucRdToken;
    }

    /*
     * �������ݿ�.
     */
    iError = __sdCoreSpiByteRd(pcspidevice, uiLen, pucReg);

    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get data error.\r\n");
        goto    __spibus_release;
    }

    /*
     * ��ȡ16λ CRC
     */
    iError = __sdCoreSpiByteRd(pcspidevice, 2, pucCrc16);
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get data error.\r\n");
        goto    __spibus_release;
    }

#if LW_CFG_SDCARD_CRC_EN > 0
    if (ucRdToken != SD_SPITOKEN_START_SIGBLK) {
        uiLen++;
        pucReg--;                                                       /*  �ָ���ֵ�Լ���crc           */
    }

    if (((pucCrc16[0] << 8) | pucCrc16[1]) != __sdCrc16(pucReg, (UINT16)uiLen)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "crc error.\r\n");
        goto    __spibus_release;
    }
#endif

    __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtClk);
    __SD_SPI_CSDIS(pcspidevice);                                        /*  ����Ƭѡ                    */
    API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);              /*  �ͷ�spi����                 */
    return  (ERROR_NONE);

__spibus_release:
    __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtClk);
    __SD_SPI_CSDIS(pcspidevice);                                        /*  ����Ƭѡ                    */
    API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);              /*  �ͷ�spi����                 */

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdCoreSdDevTransfer
** ��������: ��sD�豸��������
** ��    ��: pvDevHandle �豸���
**           psdmsg      ������Ϣ��
**           iNum        ��Ϣ����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSdDevTransfer (PVOID          pvDevHandle,
                                  PLW_SD_MESSAGE psdmsg,
                                  INT            iNum)
{
    PLW_SD_DEVICE   psddevice = (PLW_SD_DEVICE)pvDevHandle;
    INT             iError;

    iError = API_SdDeviceTransfer(psddevice, psdmsg, iNum);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreSdDevCtl
** ��������: ��sd�豸��������
** ��    ��: pvDevHandle �豸���
**           iCmd        ��������
**           lArg        �������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSdDevCtl (PVOID      pvDevHandle,
                             INT        iCmd,
                             LONG       lArg)
{
    PLW_SD_DEVICE   psddevice = (PLW_SD_DEVICE)pvDevHandle;
    INT             iError;

    if (!pvDevHandle) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iError = API_SdDeviceCtl(psddevice, iCmd, lArg);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "ctrl failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreSdDevDelet
** ��������: ɾ��һ��SD�����豸
** ��    ��: pvDevHandle  �豸���
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSdDevDelet (PVOID  pvDevHandle)
{
    PLW_SD_DEVICE   psddevice;
    INT             iError;

    if (!pvDevHandle) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psddevice = (PLW_SD_DEVICE)pvDevHandle;
    iError = API_SdDeviceDelete(psddevice);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "delet sd device failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreSdCallbackCheckDev
** ��������: sd�豸״̬���(ּ�ڸ����ڲ�״̬��ʶ)
** ��    ��: pvDevHandle  �豸���
**           iDevSta      �豸״̬
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSdCallbackCheckDev (PVOID  pvDevHandle, INT iDevSta)
{
    PLW_SD_DEVICE   psddevice;

    if (!pvDevHandle) {
        return  (PX_ERROR);
    }

    psddevice = (PLW_SD_DEVICE)pvDevHandle;
    if (iDevSta == SD_DEVSTA_UNEXIST) {
        psddevice->SDDEV_uiState &= ~SD_STATE_EXIST;
    } else {
        psddevice->SDDEV_uiState |= SD_STATE_EXIST;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreSdCallbackCheckDev
** ��������: sd�豸״̬���(ּ�ڸ����ڲ�״̬��ʶ)
** ��    ��: pvDevHandle  �豸���
**           iDevSta      �豸״̬
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreCallbackCheckDev (PLW_SDCORE_DEVICE psdcoredevice, INT iDevSta)
{
    INTREG  iregInterLevel;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        LW_SPIN_LOCK_QUICK(&psdcoredevice->COREDEV_slLock, &iregInterLevel);
        __sdCoreSdCallbackCheckDev(psdcoredevice->COREDEV_pvDevHandle, iDevSta);
        LW_SPIN_UNLOCK_QUICK(&psdcoredevice->COREDEV_slLock, iregInterLevel);
        break;

    case SDADAPTER_TYPE_SPI:
        LW_SPIN_LOCK_QUICK(&psdcoredevice->COREDEV_slLock, &iregInterLevel);
        __sdCoreSpiCallbackCheckDev(psdcoredevice->COREDEV_pvDevHandle, iDevSta);
        LW_SPIN_UNLOCK_QUICK(&psdcoredevice->COREDEV_slLock, iregInterLevel);
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiDevTransfer
** ��������: ��spi�豸��������
** ��    ��: pvDevHandle  �豸���
**           psdmsg       ������Ϣ
**           iNum         ��Ϣ����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSpiDevTransfer (PVOID  pvDevHandle, PLW_SD_MESSAGE psdmsg, INT iNum)
{
    __PCORE_SPI_DEV  pcspidevice = (__PCORE_SPI_DEV)pvDevHandle;
    PLW_SD_COMMAND   psdcmd;
    PLW_SD_DATA      psddat;
    INT              iError;
    INT              i = 0;

    if (!pvDevHandle) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    while ((i < iNum) && (psdmsg != LW_NULL)) {
        iError = API_SpiDeviceBusRequest(pcspidevice->CSPIDEV_pspiDev); /*  ����spi����                 */
        if (iError == PX_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
            return  (PX_ERROR);
        }
        __SD_SPI_CSEN(pcspidevice);                                     /*  ʹ��Ƭѡ                    */

        psdcmd = psdmsg->SDMSG_psdcmdCmd;
        psddat = psdmsg->SDMSG_psddata;

        /*
         * ���ȷ�����������
         */
        iError = __sdCoreSpiCmd(pcspidevice, psdcmd);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
            goto    __spibus_release;
        }

        /*
         * ���ݴ���
         */
        if (psddat != LW_NULL) {
            if (SD_DAT_IS_STREAM(psddat)) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "don't support stream operation.\r\n");
                goto    __spibus_release;
            }

            if (SD_DAT_IS_BOTHRW(psddat)) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "don't support both read and write data.\r\n");
                goto    __spibus_release;
            }

            if (SD_DAT_IS_WRITE(psddat)) {
                iError = __sdCoreSpiDataWrt(pcspidevice,
                                            psddat->SDDAT_uiBlkNum,
                                            psddat->SDDAT_uiBlkSize,
                                            psdmsg->SDMSG_pucWrtBuffer);
                if (iError != ERROR_NONE) {
                    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "write block error.\r\n");
                    goto    __spibus_release;
                }

            } else if (SD_DAT_IS_READ(psddat)) {
                iError = __sdCoreSpiDataRd(pcspidevice,
                                           psddat->SDDAT_uiBlkNum,
                                           psddat->SDDAT_uiBlkSize,
                                           psdmsg->SDMSG_pucRdBuffer);
                if (iError != ERROR_NONE) {
                    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "read block error.\r\n");
                    goto    __spibus_release;
                }

            } else {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unknown data flag.\r\n");
                goto    __spibus_release;
            }
        }

        /*
         * �����ֹͣ����, ���͵��豸
         */
        psdcmd = psdmsg->SDMSG_psdcmdStop;
        if (psdcmd) {
            iError = __sdCoreSpiCmd(pcspidevice, psdcmd);
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send stop cmd error.\r\n");
                goto    __spibus_release;
            }
        }

        i++;
        psdmsg++;
        __SD_SPI_CSDIS(pcspidevice);                                    /*  ����Ƭѡ                    */
        API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);          /*  ÿһ����Ϣ�������ͷ�spi���� */
    }

    return  (ERROR_NONE);

__spibus_release:
    __SD_SPI_CSDIS(pcspidevice);                                        /*  ����Ƭѡ                    */
    API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);              /*  �ͷ�spi����                 */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiDevCtl
** ��������: ��spi�豸��������
** ��    ��: pvDevHandle  �豸���
**           iCmd         ��������
**           lArg         �������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSpiDevCtl (PVOID  pvDevHandle, INT  iCmd, LONG lArg)
{
    __PCORE_SPI_DEV  pcspidevice = (__PCORE_SPI_DEV)pvDevHandle;
    UINT8            ucWrtBuf;
    INT              iError = ERROR_NONE;

    if (!pvDevHandle) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (iCmd) {

    case SDBUS_CTRL_POWEROFF:                                           /*  �رյ�Դ                    */
        break;

    case SDBUS_CTRL_POWERUP:
    case SDBUS_CTRL_POWERON:                                            /*  �ϵ�\����Դ                 */
        break;

    case SDBUS_CTRL_SETBUSWIDTH:                                        /*  ����λ��IO����              */
        break;

    case SDBUS_CTRL_SETCLK:                                             /*  ʱ��Ƶ������                */
        iError = API_SpiDeviceBusRequest(pcspidevice->CSPIDEV_pspiDev); 
        if (iError == PX_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
            break;
        }
        __SD_SPI_CSEN(pcspidevice);                                    

        if (lArg >= __SD_SPI_CLK_MAX) {
            lArg = __SD_SPI_CLK_MAX;
        }

        iError = API_SpiDeviceCtl(pcspidevice->CSPIDEV_pspiDev,
                                  LW_SPI_CTL_BAUDRATE,
                                  lArg);


        __SD_SPI_CSDIS(pcspidevice);                                    
        API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);         
        break;

    case SDBUS_CTRL_DELAYCLK:                                           /*  ����ʱ����ʱ����            */
        iError = API_SpiDeviceBusRequest(pcspidevice->CSPIDEV_pspiDev); 
        if (iError == PX_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
            break;
        }
        __SD_SPI_CSEN(pcspidevice);                                   

        while (lArg--) {
            ucWrtBuf = 0xff;
            iError = __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtBuf);     /*  ��ʱn*8��ʱ��               */
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "delay clock error.\r\n");
                break;
            }
        }

        __SD_SPI_CSDIS(pcspidevice);                                    
        API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);        
        break;


    case SDBUS_CTRL_GETOCR:                                             /*  ��ȡ����OCR                 */
        *(UINT32 *)lArg = SD_VDD_33_34;
        iError = ERROR_NONE;
        break;

    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "invalidate command.\r\n");
        iError = PX_ERROR;
        break;
    }

    return  (iError);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiDevDelet
** ��������: ɾ��һ��SPI�豸
** ��    ��: pvDevHandle  �豸���
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSpiDevDelet (PVOID pvDevHandle)
{
    __PCORE_SPI_DEV  pcspidevice;
    INT              iError;

    if (!pvDevHandle) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pcspidevice = (__PCORE_SPI_DEV)pvDevHandle;

    /*
     * ����ɾ��SPI�豸
     */
    iError = API_SpiDeviceDelete(pcspidevice->CSPIDEV_pspiDev);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "delet spi device failed.\r\n");
        return  (PX_ERROR);
    }

    /*
     * ��ɾ����װ�ĺ���SPI�豸
     */
    __SHEAP_FREE(pcspidevice);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiCallbackCheckDev
** ��������: spi�豸״̬���(ּ�ڸ����ڲ�״̬��ʶ)
** ��    ��: pvDevHandle  �豸���
**           iDevSta      �豸״̬
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSpiCallbackCheckDev (PVOID  pvDevHandle, INT iDevSta)
{
    __PCORE_SPI_DEV  pcspidevice;

    if (!pvDevHandle) {
        return  (PX_ERROR);
    }

    pcspidevice = (__PCORE_SPI_DEV)pvDevHandle;
    pcspidevice->CSPIDEV_uiState = iDevSta;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreDevSwitchToApp
** ��������: �л���APP����
** ��    ��: psdcoredevice  ���Ľṹָ��
**           bIsBc        �Ƿ���Թ㲥����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdCoreDevSwitchToApp (PLW_SDCORE_DEVICE psdcoredevice, BOOL bIsBc)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError;
    UINT32         uiRca;

    if (!bIsBc) {
        iError = API_SdCoreDevRcaView(psdcoredevice, &uiRca);
        if (iError != ERROR_NONE) {
            return  (PX_ERROR);
        }
        uiRca = uiRca << 16;
    } else {
        uiRca = 0;
    }

    sdcmd.SDCMD_uiArg    = uiRca;
    sdcmd.SDCMD_uiOpcode = SD_APP_CMD;
    sdcmd.SDCMD_uiFlag   = bIsBc ?
                           (SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_BCR) :
                           (SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_AC);

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
    if (iError != ERROR_NONE) {
    }

    return  (iError);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiCmd
** ��������: spi�豸��������
** ��    ��: pcspidevice  spi�����豸�ṹָ��
**           psdcmd       Ҫ���͵�����ָ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSpiCmd (__PCORE_SPI_DEV pcspidevice, LW_SD_COMMAND *psdcmd)
{
    LW_SPI_MESSAGE    spimsg;
    UINT8             ucWrtBuf[4];                                      /*  д����.������ʱ�������     */
    UINT8             ucRdBuf[6];                                       /*  ������.������ʱ���Ӧ��     */
    INT               iError;
    INT               iRespLen;
    INT               iRetry = 0;

    struct timespec   tvOld;
    struct timespec   tvNow;

#if LW_CFG_SDCARD_CRC_EN > 0
    UINT8             ucCmdBackUp;                                      /*  CRC����ʱ,��Ҫ������������  */
#endif

    spimsg.SPIMSG_pfuncComplete = LW_NULL;

    ucWrtBuf[0] = 0xff;
    __sdCoreSpiByteWrt(pcspidevice, 1, ucWrtBuf);                       /*  8��ͬ��ʱ��ȷ����ready      */

    /*
     * ����,����������.
     * ��SD����ṹת��ΪSPI���������Ϣ�ṹ.
     */
    ucWrtBuf[0] = (UINT8)((SD_CMD_OPC(psdcmd) & 0x3f) | 0x40);

#if LW_CFG_SDCARD_CRC_EN > 0
    ucCmdBackUp = ucWrtBuf[0];
#endif

    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = ucWrtBuf;
    spimsg.SPIMSG_pucRdBuffer = ucRdBuf;
    spimsg.SPIMSG_uiLen       = 1;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_WR;
    iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);

    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        goto    __error_out;
    }

    /*
     * ���,���Ͳ���
     */
    __sdCoreSpiParamConvert(ucWrtBuf, psdcmd->SDCMD_uiArg);

    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = ucWrtBuf;
    spimsg.SPIMSG_pucRdBuffer = ucRdBuf;
    spimsg.SPIMSG_uiLen       = 4;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_WR;
    iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);

    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send argument error.\r\n");
        goto    __error_out;
    }

    /*
     * �������һ���ֽ�crc
     */
#if LW_CFG_SDCARD_CRC_EN > 0
    ucWrtBuf[0] = __sdCrcCmdCrc7(ucCmdBackUp, ucWrtBuf);
#else
    ucWrtBuf[0] = 0x95;
#endif

    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = ucWrtBuf;
    spimsg.SPIMSG_pucRdBuffer = ucRdBuf;
    spimsg.SPIMSG_uiLen       = 1;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_WR;
    iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);

    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send last byte error.\r\n");
        goto    __error_out;
    }

    /*
     * �ȴ�Ӧ��.spiģʽ������Ӧ��.
     */
    iRetry                    = 0;
    ucWrtBuf[0]               = 0xff;
    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = ucWrtBuf;
    spimsg.SPIMSG_pucRdBuffer = ucRdBuf;
    spimsg.SPIMSG_uiLen       = 1;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_RD;
    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
    
    while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
        iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                       &spimsg,
                                       1);
        if (iError == PX_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get response error.\r\n");
            goto    __error_out;
        }

        if (!(ucRdBuf[0] & 0x80)) {                                     /*  Ӧ����ʼλ��0��ʼ           */
            goto    __resp_accept;
        }

        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {    /*  ��ʱ�˳�                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "response time out.\r\n");
            goto    __error_out;
        }
    }

    /*
     * ���ղ�����Ӧ��
     */
__resp_accept:
    iRespLen = __sdCoreSpiRespLen(psdcmd->SDCMD_uiFlag);
    if (iRespLen <= 0) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "response length error.\r\n");
        goto    __error_out;
    }
    
    if (iRespLen > 1) {
        ucWrtBuf[0]               = 0xff;
        spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
        spimsg.SPIMSG_pucWrBuffer = ucWrtBuf;
        spimsg.SPIMSG_pucRdBuffer = ucRdBuf + 1;                        /*  ֮ǰ�Ѿ�������һ���ֽ�Ӧ��  */
        spimsg.SPIMSG_uiLen       = iRespLen - 1;
        spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_RD;
        iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                       &spimsg,
                                       1);
        if (iError == PX_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get response error.\r\n");
            goto    __error_out;
        }
    }

    __sdCoreSpiRespConvert(psdcmd->SDCMD_uiResp, ucRdBuf, iRespLen);    /*  ת��ΪSD��Ϣ�ṹ��Ӧ��      */

    /*
     * ����R1B���͵�Ӧ��,��ʾӦ����豸�ᴦ��æ״̬,
     * ��ʱ�豸�������������,��Ⲣ�ȴ�.
     */
    if ((SD_RSP_SPI_MASK & psdcmd->SDCMD_uiFlag) == SD_RSP_SPI_R1B) {
        iRetry                    = 0;
        ucWrtBuf[0]               = 0xff;
        spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
        spimsg.SPIMSG_pucWrBuffer = ucWrtBuf;
        spimsg.SPIMSG_pucRdBuffer = ucRdBuf;
        spimsg.SPIMSG_uiLen       = 1;
        spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_RD;
        lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
        
        while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
            iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                           &spimsg,
                                           1);
            if (iError == PX_ERROR) {
                goto    __error_out;
            }

            if (ucRdBuf[0] != 0) {                                      /*  æ�ź�Ϊ0                   */
                break;
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {
                goto    __error_out;
            }
        }
    }

    __SD_SPI_CSDIS(pcspidevice);
    ucWrtBuf[0] = 0xff;
    __sdCoreSpiByteWrt(pcspidevice, 1, ucWrtBuf);
    __SD_SPI_CSEN(pcspidevice);

    return  (ERROR_NONE);

__error_out:
    __SD_SPI_CSDIS(pcspidevice);
    ucWrtBuf[0] = 0xff;
    __sdCoreSpiByteWrt(pcspidevice, 1, ucWrtBuf);
    __SD_SPI_CSEN(pcspidevice);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiDataRd
** ��������: spi�豸�����ݿ�
** ��    ��: pcspidevice  spi�����豸�ṹָ��
**           uiBlkNum     ������
**           uiBlkLen     �鳤��
**           pucRdBuff    ������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSpiDataRd (__PCORE_SPI_DEV pcspidevice,
                              UINT32          uiBlkNum,
                              UINT32          uiBlkLen,
                              UINT8          *pucRdBuff)
{
    INT iError = ERROR_NONE;
    INT i      = 0;

    while (i++ < uiBlkNum) {
        iError = __sdCoreSpiBlkRd(pcspidevice, uiBlkLen, pucRdBuff);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "read a block error.\r\n");
            return  (PX_ERROR);
        }
        pucRdBuff += uiBlkLen;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiDataWrt
** ��������: spi�豸д��
** ��    ��: pcspidevice  spi�����豸�ṹָ��
**           uiBlkNum     ������
**           uiBlkLen     �鳤��
**           pucWrtBuff    д����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSpiDataWrt (__PCORE_SPI_DEV pcspidevice,
                               UINT32          uiBlkNum,
                               UINT32          uiBlkLen,
                               UINT8          *pucWrtBuff)
{
    INT     iError = ERROR_NONE;
    INT     i      = 0;
    BOOL    bIsMul = uiBlkNum > 1 ? LW_TRUE : LW_FALSE;

    while (i++ < uiBlkNum) {
        iError = __sdCoreSpiBlkWrt(pcspidevice, uiBlkLen, pucWrtBuff, bIsMul);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "write a block error.\r\n");

            /*
             * ����Ƕ��,����ֹSPI��鴫��״̬
             */
            if (bIsMul) {
                __sdCoreSpiMulWrtStop(pcspidevice);
            }

            return  (PX_ERROR);
        }
        pucWrtBuff += uiBlkLen;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiBlkRd
** ��������: ��spi���豸��ȡһ��������
** ��    ��: pcspidevice  spi�����豸�ṹָ��
**           uiBlkLen     �鳤��
**           pucRdBuff    ������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSpiBlkRd (__PCORE_SPI_DEV pcspidevice,
                             UINT32          uiBlkLen,
                             UINT8          *pucRdBuff)
{
    UINT8             ucRdToken;
    struct timespec   tvOld;
    struct timespec   tvNow;
    INT               iRetry = 0;
    INT               iError;
    UINT8             pucCrc16[2];

    /*
     * �ȴ�������ʼ����.
     */
    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
    
    while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
        __sdCoreSpiByteRd(pcspidevice, 1, &ucRdToken);
        if (ucRdToken == SD_SPITOKEN_START_SIGBLK) {
            goto    __resp_accept;
        }

        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {    /*  ��ʱ�˳�                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait read token timeout.\r\n");
            goto    __error_out;
        }
    }

__resp_accept:

    /*
     * �������ݿ�.
     */
    iError = __sdCoreSpiByteRd(pcspidevice, uiBlkLen, pucRdBuff);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get data error.\r\n");
        goto    __error_out;
    }

    /*
     * ��ȡ16λ CRC
     */
    iError = __sdCoreSpiByteRd(pcspidevice, 2, pucCrc16);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get crc16 error.\r\n");
        goto    __error_out;
    }

#if LW_CFG_SDCARD_CRC_EN > 0
    if (((pucCrc16[0] << 8) | pucCrc16[1]) != __sdCrc16(pucRdBuff, (UINT16)uiBlkLen)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "crc error.\r\n");
        goto    __error_out;
    }

#endif

    __SD_SPI_CSDIS(pcspidevice);
    pucCrc16[0] = 0xff;
    __sdCoreSpiByteWrt(pcspidevice, 1, pucCrc16);
    __SD_SPI_CSEN(pcspidevice);
    return  (ERROR_NONE);

__error_out:
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiBlkWrt
** ��������: ��spi���豸д��һ��������
** ��    ��: pcspidevice  spi�����豸�ṹָ��
**           uiBlkLen     �鳤��
**           pucRdBuff    д�뻺��
**           bIsMul       �Ƿ��Ƕ��д(����д����д�Ĳ�����ͬ)
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSpiBlkWrt (__PCORE_SPI_DEV pcspidevice,
                              UINT32          uiBlkLen,
                              UINT8          *pucWrtBuff,
                              BOOL            bIsMul)
{
    UINT8             ucWrtToken;
    UINT8             ucWrtClk = 0xff;
    INT               iError;
    INT               iRetry;

    struct timespec   tvOld;
    struct timespec   tvNow;

#if LW_CFG_SDCARD_CRC_EN > 0
    UINT16            usCrc16;
    UINT8             ucCrc16[2];
#endif

    /*
     * ��ʼ����֮ǰ����ͬ��ʱ��
     */
    __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtClk);

    if (bIsMul) {
        ucWrtToken = SD_SPITOKEN_START_MULBLK;
    } else {
        ucWrtToken = SD_SPITOKEN_START_SIGBLK;
    }

    iError = __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtToken);           /*  ����д�鿪ʼ����            */
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send write token error.\r\n");
        return  (PX_ERROR);
    }

    iError = __sdCoreSpiByteWrt(pcspidevice, uiBlkLen, pucWrtBuff);     /*  ��������                    */
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "write data error.\r\n");
        return  (PX_ERROR);
    }

#if LW_CFG_SDCARD_CRC_EN > 0
    usCrc16 = __sdCrc16(pucWrtBuff, (UINT16)uiBlkLen);
    ucCrc16[0] = (usCrc16 >> 8) & 0xff;
    ucCrc16[1] =  usCrc16 & 0xff;
    __sdCoreSpiByteWrt(pcspidevice, 2, ucCrc16);                        /*  ����crc16                   */
#else
    __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtToken);
    __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtToken);                    /*  ���������16λ����          */
#endif

    /*
     * SPIģʽ��,�豸��ÿһ������д��Ŀ鶼�����һ����Ӧ����.
     * �鿴�豸��д�����ݵ���Ӧ���.
     */
    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
    
    iRetry = 0;
    while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
        __sdCoreSpiByteRd(pcspidevice, 1, &ucWrtToken);
        if ((!(ucWrtToken & 0x10)) && ((ucWrtToken & 0x01))) {          /*  Ӧ������                    */

            if ((ucWrtToken & SD_SPITOKEN_DATRSP_MASK) != SD_SPITOKEN_DATRSP_DATACCEPT) {
                return  (PX_ERROR);

            } else {
                break;
            }
        }

        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {    /*  ��ʱ�˳�                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait write token timeout.\r\n");
            return  (PX_ERROR);
        }
    }

    if (iRetry >= __SD_SPI_RSP_TIMEOUT) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait write token timeout(retry timeout).\r\n");
        return  (PX_ERROR);
    }


    /*
     * �ȴ����ݱ�̽���
     */
    iError = __sdCoreSpiWaitBusy(pcspidevice);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait data program timeout.\r\n");
    }

    return  (iError);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiByteRd
** ��������: spi�豸���ֽ�
** ��    ��: pcspidevice  spi�����豸�ṹָ��
**           uiLen        ���ֽڳ���
**           pucRdBuff     ������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSpiByteRd (__PCORE_SPI_DEV pcspidevice, UINT32 uiLen, UINT8 *pucRdBuff)
{
    INT             iError;
    LW_SPI_MESSAGE  spimsg;
    UINT8           ucWrtTmp  = 0xff;

    spimsg.SPIMSG_pfuncComplete = LW_NULL;
    spimsg.SPIMSG_usBitsPerOp   = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer   = &ucWrtTmp;
    spimsg.SPIMSG_pucRdBuffer   = pucRdBuff;
    spimsg.SPIMSG_uiLen         = uiLen;
    spimsg.SPIMSG_usFlag        = __SD_SPI_TMOD_RD;
    iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "spi transfer error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiByteWrt
** ��������: spi�豸д�ֽ�
** ��    ��: pcspidevice  spi�����豸�ṹָ��
**           uiLen        д�ֽڳ���
**           pucWrtBuff    д����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSpiByteWrt (__PCORE_SPI_DEV pcspidevice, UINT32 uiLen, UINT8 *pucWrtBuff)
{
    INT             iError;
    LW_SPI_MESSAGE  spimsg;
    UINT8           ucRdTmp;

    spimsg.SPIMSG_pfuncComplete = LW_NULL;
    spimsg.SPIMSG_usBitsPerOp   = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer   = pucWrtBuff;
    spimsg.SPIMSG_pucRdBuffer   = &ucRdTmp;
    spimsg.SPIMSG_uiLen         = uiLen;
    spimsg.SPIMSG_usFlag        = __SD_SPI_TMOD_WR;
    iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "spi transfer error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiWaitBusy
** ��������: spiæ�ȴ�
** ��    ��: pcspidevice  spi�����豸�ṹָ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdCoreSpiWaitBusy (__PCORE_SPI_DEV pcspidevice)
{
    struct timespec   tvOld;
    struct timespec   tvNow;

    UINT8             ucDatBsy;
    INT               iError;
    INT               iRetry = 0;

    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
    
    while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
        iError = __sdCoreSpiByteRd(pcspidevice, 1, &ucDatBsy);
        if (iError != ERROR_NONE) {
            return  (PX_ERROR);
        }

        if (ucDatBsy == 0xff) {
            return  (ERROR_NONE);                                       /*  ���ݱ�̽���                */
        }

        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {    /*  ��ʱ�˳�                    */
            break;
        }
    }

    /*
     * TIME OUT
     */
    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "programing data timeout.\r\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiMulWrtStop
** ��������: spi���дֹͣ
** ��    ��: pcspidevice  spi�����豸�ṹָ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdCoreSpiMulWrtStop (__PCORE_SPI_DEV pcspidevice)
{
    UINT8   pucWrtTmp[3] = {0xff, SD_SPITOKEN_STOP_MULBLK, 0xff};
    INT     iError;

    __sdCoreSpiByteWrt(pcspidevice, 3, pucWrtTmp);
    iError = __sdCoreSpiWaitBusy(pcspidevice);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "stop to wait data program timeout.\r\n");
    }
}
/*********************************************************************************************************
** ��������: __sdCoreSpiParamConvert
** ��������: SPI ����ת��
** ��    ��: pucParam     ����ת����Ĳ���
**           uiParam      �ϲ㴫���ԭʼ����
** ��    ��: NONE
** ��    ��: �ɹ�,�����豸�豸ָ��,���򷵻�LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdCoreSpiParamConvert (UINT8 *pucParam, UINT32 uiParam)
{
    pucParam[0] = (UINT8)(uiParam >> 24);
    pucParam[1] = (UINT8)(uiParam >> 16);
    pucParam[2] = (UINT8)(uiParam >> 8);
    pucParam[3] = (UINT8)(uiParam);
}
/*********************************************************************************************************
** ��������: __sdCoreSpiRespConvert
** ��������: SPI Ӧ��ת��
** ��    ��: puiResp     ����ת�����Ӧ��
**           pucResp     ԭʼӦ�����ݻ���
**           iRespLen    Ӧ��ĳ���
** ��    ��: NONE
** ��    ��: �ɹ�,�����豸�豸ָ��,���򷵻�LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdCoreSpiRespConvert (UINT32 *puiResp, const UINT8 *pucResp, INT iRespLen)
{
    switch (iRespLen) {

    case 1:
        *puiResp = *pucResp;
        break;

    case 2:
        *puiResp = *pucResp++;
        *puiResp = (*puiResp << 8) | *pucResp;
        break;

    case 5:
        *puiResp++ = *pucResp++;                                        /*  resp[0]                     */
        *puiResp   = *pucResp++;                                        /*  resp[1]                     */
        *puiResp   = (*puiResp << 8) | *pucResp++;
        *puiResp   = (*puiResp << 8) | *pucResp++;
        *puiResp   = (*puiResp << 8) | *pucResp;
        break;

    default:
        return;
    }
}
/*********************************************************************************************************
** ��������: __sdCoreSpiRespLen
** ��������: �����������ж� SPI Ӧ��ĳ���
** ��    ��: uiCmdFlg     �����־
** ��    ��: NONE
** ��    ��: �ɹ�,�����豸�豸ָ��,���򷵻�LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdCoreSpiRespLen (UINT32   uiCmdFlg)
{
    INT    iRspLen = 0;

    switch (SD_RSP_SPI_MASK & uiCmdFlg) {

    case SD_RSP_SPI_R1:
    case SD_RSP_SPI_R1B:
        iRspLen = 1;
        break;

    case SD_RSP_SPI_R2:                                                 /*  same as SD_RSP_SPI_R5:      */
        iRspLen = 2;
        break;

    case SD_RSP_SPI_R3:                                                 /*  same as SD_RSP_SPI_R4:      */
                                                                        /*  same as SD_RSP_SPI_R7:      */
        iRspLen = 5;
        break;

    default:
        iRspLen = -1;
        break;
    }

    return  (iRspLen);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
