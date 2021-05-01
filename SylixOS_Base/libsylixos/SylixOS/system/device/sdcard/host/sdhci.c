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
** ��   ��   ��: sdhci.h
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2011 �� 01 �� 17 ��
**
** ��        ��: SD ��׼������������Դ�ļ�(SD Host Controller Simplified Specification Version 2.00).

** BUG:
2011.03.02  ���� ���ش���ģʽ�鿴\���ú���.����̬�ı䴫��ģʽ(�����ϲ������豸�������).
2011.04.07  ���� SDMA ���书��.
2011.04.07  ���ǵ� SD �������ڲ�ͬƽ̨����Ĵ��������� IO �ռ�,Ҳ�������ڴ�ռ�,���Զ�д�Ĵ���
            ��6����������Ϊ�ⲿ����,�ɾ���ƽ̨������ʵ��.
2014.11.13  �޸� һ�㴫��ģʽΪ�жϷ�ʽ, ͬʱ֧�� SDIO �жϴ���
2014.11.15  ���� SDHCI ע�ᵽ SDM ����ع��ܺ���
2014.11.24  ���� SDIO �жϷ����̵߳�һ�� BUG, �ڵȵ��ж��ź������ٻ�ȡ SDHCI ����������.
2014.11.29  ���� SDHCI �жϷ�����, ���ӷ�ֹ SDIO �ж��쳣��ʧ�Ĵ���.
2015.03.10  ���� ACMD12 ���ܵ��ж���������.
            ���� QUIRK ��ش���.
2015.03.11  ���� ��д��������֧��.
2015.03.14  ���� SDHCI ���ܷ��� SDIO Ӳ���жϵĴ���.
2015.11.20  ���� �Բ�֧�� ACMD12 �Ŀ���������.
            ���� �� SDHCI v3.0 �ļ�����֧��.
2016.12.16  ���� �ڴ�������п��ܲ�������������.
2017.03.15  ���� ʹ�ò�ѯ SDIO �ж������, Ӧ���߳̽�ֹ�жϿ����������������.
2017.07.10  ���� ��æӦ��������Ӧ������������жϺ���������жϵ��Ⱥ�˳��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_IO
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "sdhci.h"
#include "../include/sddebug.h"
#include "../core/sddrvm.h"
#include "../core/sdstd.h"
#include "../core/sdiostd.h"
/*********************************************************************************************************
  �ڲ���
*********************************************************************************************************/
#define __SDHCI_HOST_DEVNUM_MAX   1                                     /*  һ��������֧�ֵĲ�����      */
#define __SDHCI_MAX_BASE_CLK      102400000                             /*  ������������ʱ��102.4Mhz  */

#define __SDHCI_CMD_TOUT_SEC      4                                     /*  ���ʱ����ʱ��(����ֵ)    */
#define __SDHCI_CMD_RETRY         20000                                 /*  ���ʱ����                */

#define __SDHCI_DMA_BOUND_LEN     (1 << (12 + __SDHCI_DMA_BOUND_NBITS))
#define __SDHCI_DMA_BOUND_NBITS   7                                     /*  ϵͳ�����ڴ�߽�λָʾ      */
                                                                        /*  ��ǰ�趨ֵΪ 7 (���):      */
                                                                        /*  1 << (12 + 7) = 512k        */
                                                                        /*  �����ݰ���ʱ����512K�߽�,�� */
                                                                        /*  Ҫ����DMA��ַ               */
#define __SDHCI_HOST_NAME(phs)    \
        ((phs)->SDHCIHS_psdadapter->SDADAPTER_busadapter.BUSADAPTER_cName)

#define __SDHCI_OCR_330           (SD_VDD_32_33 | SD_VDD_33_34)
#define __SDHCI_OCR_300           (SD_VDD_30_31 | SD_VDD_31_32)
#define __SDHCI_OCR_180           (SD_VDD_165_195)

#define __SDHCI_INT_EN_MASK       (SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |\
                                   SDHCI_INT_DATA_CRC  | SDHCI_INT_DATA_TIMEOUT |\
                                   SDHCI_INT_INDEX     | SDHCI_INT_END_BIT      |\
                                   SDHCI_INT_CRC       | SDHCI_INT_TIMEOUT      |\
                                   SDHCI_INT_DATA_END  | SDHCI_INT_RESPONSE     |\
                                   SDHCI_INT_ACMD12ERR)
/*********************************************************************************************************
  ���������߳����
*********************************************************************************************************/
#define __SDHCI_SDIOINTSVR_PRIO   197
#define __SDHCI_SDIOINTSVR_STKSZ  (8 * 1024)
/*********************************************************************************************************
  ���������߲����궨��
*********************************************************************************************************/
#define __SDHCI_TRANS_LOCK(pt)    LW_SPIN_LOCK(&pt->SDHCITS_slLock)
#define __SDHCI_TRANS_UNLOCK(pt)  LW_SPIN_UNLOCK(&pt->SDHCITS_slLock)
/*********************************************************************************************************
  �����ڿ������ܷ��� SDIO Ӳ���жϵ����
*********************************************************************************************************/
#define __SDHCI_SDIO_WAIT(pt)     API_SemaphoreCPend(pt->SDHCITS_hSdioIntSem, LW_OPTION_WAIT_INFINITE)
#define __SDHCI_SDIO_NOTIFY(pt)   API_SemaphoreCPost(pt->SDHCITS_hSdioIntSem)
/*********************************************************************************************************
  �����ڿ��������ܷ��� SDIO Ӳ���ж�, ʹ�ò�ѯ��ʽ���� SDIO �жϵ����
*********************************************************************************************************/
#define __SDHCI_SDIO_DISABLE(pt)  API_SemaphoreBPend(pt->SDHCITS_hSdioIntSem, LW_OPTION_WAIT_INFINITE)
#define __SDHCI_SDIO_ENABLE(pt)   API_SemaphoreBPost(pt->SDHCITS_hSdioIntSem)
#define __SDHCI_SDIO_REQUEST(pt)  __SDHCI_SDIO_DISABLE(pt)
#define __SDHCI_SDIO_RELEASE(pt)  __SDHCI_SDIO_ENABLE(pt)

#define __SDHCI_SDIO_INT_MAX      4
#define __SDHCI_SDIO_DLY_MAX      3
/*********************************************************************************************************
  ǰ������
*********************************************************************************************************/
struct __sdhci_trans;
struct __sdhci_host;
struct __sdhci_sdm_host;

typedef struct __sdhci_trans      __SDHCI_TRANS;
typedef struct __sdhci_host       __SDHCI_HOST;
typedef struct __sdhci_sdm_host   __SDHCI_SDM_HOST;
typedef struct __sdhci_trans     *__PSDHCI_TRANS;
typedef struct __sdhci_host      *__PSDHCI_HOST;
typedef struct __sdhci_sdm_host  *__PSDHCI_SDM_HOST;
/*********************************************************************************************************
  ��׼���������������ṹ
*********************************************************************************************************/
typedef struct __sdhci_capab {
    UINT32      SDHCICAP_uiBaseClkFreq;                                 /*  ��ʱ��Ƶ��                  */
    UINT32      SDHCICAP_uiMaxBlkSize;                                  /*  ֧�ֵ����鳤��            */
    UINT32      SDHCICAP_uiVoltage;                                     /*  ��ѹ֧�����                */

    BOOL        SDHCICAP_bCanSdma;                                      /*  �Ƿ�֧�� SDMA               */
    BOOL        SDHCICAP_bCanAdma;                                      /*  �Ƿ�֧�� ADMA               */
    BOOL        SDHCICAP_bCanHighSpeed;                                 /*  �Ƿ�֧�ָ��ٴ���            */
    BOOL        SDHCICAP_bCanSusRes;                                    /*  �Ƿ�֧�ֹ���\�ָ�����       */
}__SDHCI_CAPAB, *__PSDHCI_CAPAB;
/*********************************************************************************************************
  SD ��׼�������淶��, ����С���Ϊ 2048(SDIO �豸����Ҳ�� 2048), �������Ϊ 65535.
  ͬʱ, �� DMA ������, ��������� 512KB.
  1. һ�㴫��ģʽ:
     ��ģʽֻ�õ� BlkSize �� BlkCnt �Ĵ���. �û������ BlkSize һ���ǲ����� 2048 ��, ���� BlkCnt ����
     ����65535. ��ʱ����Ҫ�����ݽ��в��.
     �����û����� BlkSize Ϊ32, BlkCnt Ϊ 80000, ����Ҫ���Ϊ���δ���.

  2. DMA ����:
     ��ʱ����Ҫ���� 1 �����, ��Ӧ�ý��û����ݲ��Ϊ N �� 512KB ���� DMA ����.
     �����û����� BlkSize Ϊ2048, BlkCnt Ϊ256. ����Ҫ���Ϊ 2 �� 512 KB �� DMA ����.

  3. ��ʵ��ʹ����, �������μ����ټ�(Ϊ�˴�����ȶ��ͼ��ٳ���). ���, ��ǰ�汾��֧��������������⴦��.

  ��׼����������������ƿ�
  ���ｫ�û��ύ�ĵ�������(��������������ݶ�д����ȡӦ���һ�������̲���)����һ������.
*********************************************************************************************************/
struct __sdhci_trans {
    __PSDHCI_HOST         SDHCITS_psdhcihost;
#if LW_CFG_VMM_EN > 0
    UINT8                *SDHCITS_pucDmaBuffer;
#endif
    LW_OBJECT_HANDLE      SDHCITS_hFinishSync;                          /*  �����������ͬ���ź�        */

    BOOL                  SDHCITS_bCmdFinish;
    BOOL                  SDHCITS_bDatFinish;
    INT                   SDHCITS_iCmdError;
    INT                   SDHCITS_iDatError;                            /*  ����״̬����                */

    UINT8                *SDHCITS_pucDatBuffCurr;
    UINT32                SDHCITS_uiBlkSize;
    UINT32                SDHCITS_uiBlkCntRemain;
    BOOL                  SDHCITS_bIsRead;                              /*  ������״̬����            */

    INT                   SDHCITS_iTransType;
#define __SDHIC_TRANS_NORMAL    0
#define __SDHIC_TRANS_SDMA      1
#define __SDHIC_TRANS_ADMA      2

    LW_OBJECT_HANDLE      SDHCITS_hSdioIntThread;                       /*  ���� SDIO �жϴ����߳�      */
    LW_OBJECT_HANDLE      SDHCITS_hSdioIntSem;                          /*  SDIO �ж�ͬ���ź�           */

    UINT32                SDHCITS_uiIntSta;

    INT                   SDHCITS_iStage;
#define __SDHCI_TRANS_STAGE_START  0
#define __SDHCI_TRANS_STAGE_STOP   1
    LW_SD_COMMAND        *SDHCITS_psdcmd;
    LW_SD_COMMAND        *SDHCITS_psdcmdStop;
    LW_SD_DATA           *SDHCITS_psddat;                               /*  ���û����������            */

    LW_SPINLOCK_DEFINE   (SDHCITS_slLock);
};
/*********************************************************************************************************
  ��׼������ HOST�ṹ
*********************************************************************************************************/
struct __sdhci_host {
    LW_SD_FUNCS             SDHCIHS_sdfunc;                             /*  ��Ӧ����������              */
    PLW_SD_ADAPTER          SDHCIHS_psdadapter;                         /*  ��Ӧ������������            */
    LW_SDHCI_HOST_ATTR      SDHCIHS_sdhcihostattr;                      /*  ��������                    */
    __SDHCI_CAPAB           SDHCIHS_sdhcicap;                           /*  ���ع���                    */
    UINT32                  SDHCIHS_uiVersion;                          /*  Ӳ���汾                    */
    atomic_t                SDHCIHS_atomicDevCnt;                       /*  �豸����                    */

    UINT32                  SDHCIHS_ucTransferMod;                      /*  ����ʹ�õĴ���ģʽ          */
    INT                   (*SDHCIHS_pfuncMasterXfer)                    /*  ���ص�ǰʹ�õĴ��亯��      */
                          (
                          __PSDHCI_HOST        psdhcihost,
                          PLW_SD_DEVICE        psddev,
                          PLW_SD_MESSAGE       psdmsg,
                          INT                  iNum
                          );

    __PSDHCI_TRANS          SDHCIHS_psdhcitrans;                        /*  ����������ƿ����          */
    __PSDHCI_SDM_HOST       SDHCIHS_psdhcisdmhost;                      /*  SDM �� HOST ��Ϣ            */

    UINT32                  SDHCIHS_uiClkCurr;
    BOOL                    SDHCIHS_bClkEnable;
    BOOL                    SDHCIHS_bSdioIntEnable;
    UINT8                   SDHCIHS_ucDevType;                          /*  ��ǰ�豸������              */
};
/*********************************************************************************************************
  ��׼�������豸�ڲ��ṹ
*********************************************************************************************************/
typedef struct __sdhci_dev {
    PLW_SD_DEVICE       SDHCIDEV_psddevice;                             /*  ��Ӧ�� SD �豸              */
    __PSDHCI_HOST       SDHCIDEV_psdhcihost;                            /*  ��Ӧ�� HOST                 */
    CHAR                SDHCIDEV_pcDevName[LW_CFG_OBJECT_NAME_SIZE];    /*  �豸����(���Ӧ SD �豸��ͬ)*/
} __SDHCI_DEVICE, *__PSDHCI_DEVICE;
/*********************************************************************************************************
  ��׼������ע�ᵽ SDM ģ������ݽṹ
*********************************************************************************************************/
struct __sdhci_sdm_host {
    SD_HOST         SDHCISDMH_sdhost;
    __PSDHCI_HOST   SDHCISDMH_psdhcihost;
    SD_CALLBACK     SDHCISDMH_callbackChkDev;
    PVOID           SDHCISDMH_pvCallBackArg;
    PVOID           SDHCISDMH_pvDevAttached;
    PVOID           SDHCISDMH_pvSdmHost;
};
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static SDHCI_DRV_FUNCS _G_sdhcidrvfuncTbl[2];
/*********************************************************************************************************
  ˽�к�������
*********************************************************************************************************/
static VOID __sdhciHostCapDecode(PLW_SDHCI_HOST_ATTR psdhcihostattr, __PSDHCI_CAPAB psdhcicap);
/*********************************************************************************************************
  CALLBACK FOR SD BUS LAYER
*********************************************************************************************************/
static INT __sdhciTransfer(PLW_SD_ADAPTER      psdadapter,
                           PLW_SD_DEVICE       psddev,
                           PLW_SD_MESSAGE      psdmsg,
                           INT                 iNum);
static INT __sdhciIoCtl(PLW_SD_ADAPTER  psdadapter,
                        INT             iCmd,
                        LONG            lArg);
/*********************************************************************************************************
  FOR I\O CONTROL PRIVATE
*********************************************************************************************************/
static INT __sdhciClockSet(__PSDHCI_HOST     psdhcihost, UINT32 uiSetClk);
static INT __sdhciClockStop(__PSDHCI_HOST    psdhcihost);
static INT __sdhciBusWidthSet(__PSDHCI_HOST  psdhcihost, UINT32 uiBusWidth);
static INT __sdhciPowerOn(__PSDHCI_HOST      psdhcihost);
static INT __sdhciPowerOff(__PSDHCI_HOST     psdhcihost);
static INT __sdhciPowerSetVol(__PSDHCI_HOST  psdhcihost,
                              UINT8          ucVol,
                              BOOL           bIsOn);
static INT __sdhciHighSpeedEn(__PSDHCI_HOST  psdhcihost,  BOOL bEnable);
/*********************************************************************************************************
  FOR SDM LAYER
*********************************************************************************************************/
static __SDHCI_SDM_HOST *__sdhciSdmHostNew(__PSDHCI_HOST      psdhcihost);
static INT               __sdhciSdmHostDel(__SDHCI_SDM_HOST  *psdhcisdmhost);

static INT               __sdhciSdmCallBackInstall(SD_HOST          *psdhost,
                                                   INT               iCallbackType,
                                                   SD_CALLBACK       callback,
                                                   PVOID             pvCallbackArg);
static INT               __sdhciSdmCallBackUnInstall(SD_HOST          *psdhost,
                                                     INT               iCallbackType);
static VOID              __sdhciSdmSdioIntEn(SD_HOST *psdhost, BOOL bEnable);
static BOOL              __sdhciSdmIsCardWp(SD_HOST *psdhost);

static VOID              __sdhciSdmDevAttach(SD_HOST *psdhost, CPCHAR cpcDevName);
static VOID              __sdhciSdmDevDetach(SD_HOST *psdhost);
/*********************************************************************************************************
  FOR TRANSFER PRIVATE
*********************************************************************************************************/
static INT __sdhciTransferNorm(__PSDHCI_HOST       psdhcihost,
                               PLW_SD_DEVICE       psddev,
                               PLW_SD_MESSAGE      psdmsg,
                               INT                 iNum);
static INT __sdhciTransferSdma(__PSDHCI_HOST       psdhcihost,
                               PLW_SD_DEVICE       psddev,
                               PLW_SD_MESSAGE      psdmsg,
                               INT                 iNum);
static INT __sdhciTransferAdma(__PSDHCI_HOST       psdhcihost,
                               PLW_SD_DEVICE       psddev,
                               PLW_SD_MESSAGE      psdmsg,
                               INT                 iNum);
static INT  __sdhciCmdSend(__PSDHCI_HOST   psdhcihost,
                           PLW_SD_COMMAND  psdcmd,
                           PLW_SD_DATA     psddat);

static VOID __sdhciTransferModSet(__PSDHCI_HOST   psdhcihost);
static VOID __sdhciDataPrepareNorm(__PSDHCI_HOST  psdhcihost);
static VOID __sdhciDataPrepareSdma(__PSDHCI_HOST  psdhcihost);
static VOID __sdhciDataPrepareAdma(__PSDHCI_HOST  psdhcihost);

static VOID __sdhciTransferIntSet(__PSDHCI_HOST  psdhcihost);
static VOID __sdhciIntDisAndEn(__PSDHCI_HOST     psdhcihost,
                               UINT32            uiDisMask,
                               UINT32            uiEnMask);
static VOID __sdhciSdioIntEn(__PSDHCI_HOST     psdhcihost, BOOL bEnable);
/*********************************************************************************************************
  FOR TRANSACTION
*********************************************************************************************************/
static __SDHCI_TRANS *__sdhciTransNew(__PSDHCI_HOST  psdhcihost);
static VOID           __sdhciTransDel(__SDHCI_TRANS *psdhcitrans);
static INT            __sdhciTransTaskInit(__SDHCI_TRANS *psdhcitrans);
static INT            __sdhciTransTaskDeInit(__SDHCI_TRANS *psdhcitrans);

static INT            __sdhciTransPrepare(__SDHCI_TRANS *psdhcitrans,
                                          LW_SD_MESSAGE *psdmsg,
                                          INT            iTransType);
static INT            __sdhciTransStart(__SDHCI_TRANS *psdhcitrans);
static INT            __sdhciTransFinishWait(__SDHCI_TRANS *psdhcitrans);
static VOID           __sdhciTransFinish(__SDHCI_TRANS *psdhcitrans);
static INT            __sdhciTransClean(__SDHCI_TRANS *psdhcitrans);

static irqreturn_t    __sdhciTransIrq(VOID *pvArg, ULONG ulVector);
static PVOID          __sdhciSdioIntSvr(VOID *pvArg);

static INT            __sdhciTransCmdHandle(__SDHCI_TRANS *psdhcitrans, UINT32 uiIntSta);
static INT            __sdhciTransDatHandle(__SDHCI_TRANS *psdhcitrans, UINT32 uiIntSta);

static INT            __sdhciTransCmdFinish(__SDHCI_TRANS *psdhcitrans);
static INT            __sdhciTransDatFinish(__SDHCI_TRANS *psdhcitrans);

static INT            __sdhciDataReadNorm(__PSDHCI_TRANS    psdhcitrans);
static INT            __sdhciDataWriteNorm(__PSDHCI_TRANS    psdhcitrans);
/*********************************************************************************************************
  FOR IO ACCESS DRV
*********************************************************************************************************/
static UINT32 __sdhciIoReadL(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr);
static UINT16 __sdhciIoReadW(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr);
static UINT8  __sdhciIoReadB(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr);

static VOID   __sdhciIoWriteL(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT32 uiLword);
static VOID   __sdhciIoWriteW(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT16 usWord);
static VOID   __sdhciIoWriteB(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT8 ucByte);

static UINT32 __sdhciMemReadL(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr);
static UINT16 __sdhciMemReadW(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr);
static UINT8  __sdhciMemReadB(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr);
static VOID   __sdhciMemWriteL(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT32 uiLword);
static VOID   __sdhciMemWriteW(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT16 usWord);
static VOID   __sdhciMemWriteB(PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT8 ucByte);

static INT    __sdhciRegAccessDrvInit(PLW_SDHCI_HOST_ATTR  psdhcihostattr);
/*********************************************************************************************************
  FOR DEBUG
*********************************************************************************************************/
#ifdef __SYLIXOS_DEBUG
static VOID __sdhciPreStaShow(PLW_SDHCI_HOST_ATTR psdhcihostattr);
static VOID __sdhciIntStaShow(PLW_SDHCI_HOST_ATTR psdhcihostattr);
#else
#define __sdhciPreStaShow(x)
#define __sdhciIntStaShow(x)
#endif                                                                  /*  __SYLIXOS_DEBUG             */
/*********************************************************************************************************
  INLINE FUNCTIONS
*********************************************************************************************************/
static LW_INLINE INT __sdhciCmdRespType (PLW_SD_COMMAND psdcmd)
{
    UINT32  uiRespFlag = SD_RESP_TYPE(psdcmd);
    INT     iType      = 0;

    if (!(uiRespFlag & SD_RSP_PRESENT)) {
        iType = SDHCI_CMD_RESP_TYPE_NONE;
    } else if (uiRespFlag & SD_RSP_136) {
        iType = SDHCI_CMD_RESP_TYPE_LONG;
    } else if (uiRespFlag & SD_RSP_BUSY) {
        iType = SDHCI_CMD_RESP_TYPE_SHORT_BUSY;
    } else {
        iType = SDHCI_CMD_RESP_TYPE_SHORT;
    }

    return  (iType);
}

static LW_INLINE VOID __sdhciSdmaAddrUpdate (__PSDHCI_HOST psdhcihost, LONG lSysAddr)
{
    SDHCI_WRITEL(&psdhcihost->SDHCIHS_sdhcihostattr,
                 SDHCI_SYS_SDMA,
                 (UINT32)lSysAddr);
}

static LW_INLINE VOID __sdhciHostReset (__PSDHCI_HOST psdhcihost, UINT8 ucBitMask)
{
    INT     iTimeout = 1000;

    SDHCI_WRITEB(&psdhcihost->SDHCIHS_sdhcihostattr,
                 SDHCI_SOFTWARE_RESET,
                 ucBitMask);
    while ((SDHCI_READB(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_SOFTWARE_RESET) & ucBitMask) && iTimeout) {
        iTimeout--;
    }

    if (iTimeout <= 0) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "host rest timeout.\r\n");
    }
}

static LW_INLINE VOID __sdhciDmaSelect (__PSDHCI_HOST psdhcihost, UINT8 ucDmaType)
{
    UINT8 ucHostCtrl;

    ucHostCtrl = SDHCI_READB(&psdhcihost->SDHCIHS_sdhcihostattr,
                             SDHCI_HOST_CONTROL);
    ucHostCtrl = (ucHostCtrl & (~SDHCI_HCTRL_DMA_MASK)) | ucDmaType;
    SDHCI_WRITEB(&psdhcihost->SDHCIHS_sdhcihostattr,
                 SDHCI_HOST_CONTROL,
                 ucHostCtrl);
}

static LW_INLINE VOID __sdhciIntClear (__PSDHCI_HOST psdhcihost)
{
    SDHCI_WRITEL(&psdhcihost->SDHCIHS_sdhcihostattr, SDHCI_INT_STATUS, SDHCI_INT_ALL_MASK);
}

static LW_INLINE VOID __sdhciTimeoutSet (__PSDHCI_HOST psdhcihost)
{
    LW_SDHCI_HOST_ATTR *psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;

    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncTimeoutSet) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncTimeoutSet(psdhcihostattr);
    } else {
        SDHCI_WRITEB(psdhcihostattr, SDHCI_TIMEOUT_CONTROL, 0xe);
    }
}
/*********************************************************************************************************
** ��������: API_SdhciHostCreate
** ��������: ����SD��׼��������
** ��    ��: psdAdapter     ����Ӧ�� SD ��������������
**           psdhcihostattr ����������
** ��    ��: �ɹ�: �������ض���ָ��. ʧ��: ���� NULL.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API PVOID  API_SdhciHostCreate (CPCHAR               pcAdapterName,
                                   PLW_SDHCI_HOST_ATTR  psdhcihostattr)
{
    PLW_SD_ADAPTER    psdadapter    = LW_NULL;
    __PSDHCI_HOST     psdhcihost    = LW_NULL;
    __PSDHCI_TRANS    psdhcitrans   = LW_NULL;
    __PSDHCI_SDM_HOST psdhcisdmhost = LW_NULL;
    INT               iError;

    if (!pcAdapterName || !psdhcihostattr) {
        _ErrorHandle(EINVAL);
        return  ((PVOID)LW_NULL);
    }

    /*
     * SDHCI ��׼��������ʱ�ӷ�Ƶϵ�����������, �������ʱ�Ӳ��ܳ������ֵ
     * �������ʹ�������Զ����ʱ�����÷���, ������ƺ���
     */
    if (psdhcihostattr->SDHCIHOST_uiMaxClock > __SDHCI_MAX_BASE_CLK) {
        if (!psdhcihostattr->SDHCIHOST_pquirkop ||
            !psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncClockSet) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "max clock must be less than 102.4Mhz.\r\n");
            _ErrorHandle(EINVAL);
            return  ((PVOID)LW_NULL);
        }
    }

    iError = __sdhciRegAccessDrvInit(psdhcihostattr);                   /*  �Ĵ�������������ʼ��        */
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "register access drv init failed.\r\n");
        return  ((PVOID)LW_NULL);
    }

    psdhcihost = (__PSDHCI_HOST)__SHEAP_ALLOC(sizeof(__SDHCI_HOST));   /*  ����HOST                     */
    if (!psdhcihost) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  ((PVOID)LW_NULL);
    }
    lib_bzero(psdhcihost, sizeof(__SDHCI_HOST));

    psdhcihost->SDHCIHS_uiVersion = SDHCI_READW(psdhcihostattr, SDHCI_HOST_VERSION);
    psdhcihost->SDHCIHS_uiVersion = (psdhcihost->SDHCIHS_uiVersion &
                                     SDHCI_HVER_SPEC_VER_MASK)     >>
                                     SDHCI_HVER_SPEC_VER_SHIFT;

    __sdhciHostCapDecode(psdhcihostattr, &psdhcihost->SDHCIHS_sdhcicap);/*  ���ع��ܽ���                */

    psdhcihost->SDHCIHS_sdfunc.SDFUNC_pfuncMasterXfer = __sdhciTransfer;
    psdhcihost->SDHCIHS_sdfunc.SDFUNC_pfuncMasterCtl  = __sdhciIoCtl;   /*  ��ʼ����������              */

    iError = API_SdAdapterCreate(pcAdapterName, &psdhcihost->SDHCIHS_sdfunc);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "create sd adapter error.\r\n");
        goto    __err0;
    }

    psdadapter = API_SdAdapterGet(pcAdapterName);                       /*  ���������                  */

    psdhcihost->SDHCIHS_psdadapter           = psdadapter;
    psdhcihost->SDHCIHS_atomicDevCnt.counter = 0;                       /*  ��ʼ�豸��Ϊ0               */
    psdhcihost->SDHCIHS_pfuncMasterXfer      = __sdhciTransferNorm;
    psdhcihost->SDHCIHS_ucTransferMod        = SDHCIHOST_TMOD_SET_NORMAL;
    lib_memcpy(&psdhcihost->SDHCIHS_sdhcihostattr,
               psdhcihostattr, sizeof(LW_SDHCI_HOST_ATTR));
                                                                        /*  ����������                  */
    __sdhciIntDisAndEn(psdhcihost, SDHCI_INT_ALL_MASK, 0);              /*  ��ֹ�����ж�                */

    psdhcitrans = __sdhciTransNew(psdhcihost);
    if (!psdhcitrans) {
        goto    __err1;
    }
    psdhcihost->SDHCIHS_psdhcitrans = psdhcitrans;                      /*  ������������                */

    psdhcisdmhost = __sdhciSdmHostNew(psdhcihost);
    if (!psdhcisdmhost) {
        goto    __err2;
    }
    psdhcihost->SDHCIHS_psdhcisdmhost = psdhcisdmhost;                  /*  ���� SDM �� HOST ����       */

    __sdhciHostReset(psdhcihost, SDHCI_SFRST_CMD | SDHCI_SFRST_DATA);

    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DONOT_SET_VOLTAGE)) {
        UINT32 uiVoltage = psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_uiVoltage;
        if ((uiVoltage & __SDHCI_OCR_330) == __SDHCI_OCR_330) {
            __sdhciPowerSetVol(psdhcihost, SDHCI_POWCTL_330, LW_TRUE);
        } else if ((uiVoltage & __SDHCI_OCR_300) == __SDHCI_OCR_300) {
            __sdhciPowerSetVol(psdhcihost, SDHCI_POWCTL_300, LW_TRUE);
        } else if ((uiVoltage & __SDHCI_OCR_180) == __SDHCI_OCR_180) {
            __sdhciPowerSetVol(psdhcihost, SDHCI_POWCTL_180, LW_TRUE);
        } else {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "warning: host ocr info lost.\r\n");
        }
    }

    __sdhciTimeoutSet(psdhcihost);

    __sdhciIntDisAndEn(psdhcihost, SDHCI_INT_ALL_MASK | SDHCI_INT_CARD_INT, __SDHCI_INT_EN_MASK);

    return  ((PVOID)psdhcihost);

__err2:
    __sdhciTransDel(psdhcitrans);

__err1:
    API_SdAdapterDelete(pcAdapterName);

__err0:
    __SHEAP_FREE(psdhcihost);

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_SdhciSdmHostGet
** ��������: ��� SDM Host ����
** ��    ��: pvHost    SDHCI ������ָ��
** ��    ��: SDHCI ��������Ӧ�� SDM ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API PVOID   API_SdhciSdmHostGet (PVOID pvHost)
{
    __PSDHCI_HOST   psdhcihost;
    PVOID           pvSdmHost;

    if (!pvHost) {
        return  (LW_NULL);
    }

    psdhcihost = (__PSDHCI_HOST)pvHost;
    pvSdmHost  = psdhcihost->SDHCIHS_psdhcisdmhost->SDHCISDMH_pvSdmHost;

    return  (pvSdmHost);
}
/*********************************************************************************************************
** ��������: API_SdhciHostDelete
** ��������: ɾ�� SD ��׼��������
** ��    ��: pvHost    ������ָ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdhciHostDelete (PVOID  pvHost)
{
    __PSDHCI_HOST    psdhcihost = LW_NULL;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost = (__PSDHCI_HOST)pvHost;
    if (API_AtomicGet(&psdhcihost->SDHCIHS_atomicDevCnt)) {
        _ErrorHandle(EBUSY);
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device always attached.\r\n");
        return  (PX_ERROR);
    }

   __sdhciSdmHostDel(psdhcihost->SDHCIHS_psdhcisdmhost);
   __sdhciTransDel(psdhcihost->SDHCIHS_psdhcitrans);

    API_SdAdapterDelete(__SDHCI_HOST_NAME(psdhcihost));
    __SHEAP_FREE(psdhcihost);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdhciHostTransferModGet
** ��������: �������֧�ֵĴ���ģʽ
** ��    ��: pvHost    ������ָ��
** ��    ��: �ɹ�,���ش���ģʽ֧�����; ʧ��, ���� 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdhciHostTransferModGet (PVOID    pvHost)
{
    __PSDHCI_HOST   psdhcihost = LW_NULL;
    INT             iTmodFlg   = SDHCIHOST_TMOD_CAN_NORMAL;             /*  ����֧��һ�㴫��            */

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  (0);
    }

    psdhcihost = (__PSDHCI_HOST)pvHost;

    if (psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_bCanSdma) {
        iTmodFlg |= SDHCIHOST_TMOD_CAN_SDMA;
    }

    if (psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_bCanAdma) {
        iTmodFlg |= SDHCIHOST_TMOD_CAN_ADMA;
    }

    return  (iTmodFlg);
}
/*********************************************************************************************************
** ��������: API_SdhciHostTransferModSet
** ��������: �������صĴ���ģʽ
** ��    ��: pvHost    ������ָ��
**           iTmod     Ҫ���õĴ���ģʽ
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdhciHostTransferModSet (PVOID    pvHost, INT   iTmod)
{
    __PSDHCI_HOST    psdhcihost = LW_NULL;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost   = (__PSDHCI_HOST)pvHost;

    /*
     * ���뱣֤������û���豸, ���ܸ������ش���ģʽ
     */
    if (API_AtomicGet(&psdhcihost->SDHCIHS_atomicDevCnt)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device exist.\r\n");
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }

    switch (iTmod) {
    
    case SDHCIHOST_TMOD_SET_NORMAL:
        psdhcihost->SDHCIHS_ucTransferMod   = SDHCIHOST_TMOD_SET_NORMAL;
        psdhcihost->SDHCIHS_pfuncMasterXfer = __sdhciTransferNorm;
        break;

    case SDHCIHOST_TMOD_SET_SDMA:
        if (psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_bCanSdma) {
            psdhcihost->SDHCIHS_ucTransferMod   = SDHCIHOST_TMOD_SET_SDMA;
            psdhcihost->SDHCIHS_pfuncMasterXfer = __sdhciTransferSdma;
        } else {
            return  (PX_ERROR);
        }
        break;

    case SDHCIHOST_TMOD_SET_ADMA:
        if (psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_bCanAdma) {
            psdhcihost->SDHCIHS_ucTransferMod   = SDHCIHOST_TMOD_SET_ADMA;
            psdhcihost->SDHCIHS_pfuncMasterXfer = __sdhciTransferAdma;
        } else {
            return  (PX_ERROR);
        }
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciDeviceAdd
** ��������: ��SD��׼�������������һ�� SD �豸
** ��    ��: pvHost      ������ָ��
**           pcDevice    Ҫ��ӵ� SD �豸����
** ��    ��: �ɹ�,�����豸ָ��.ʧ��,���� NULL.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  __sdhciDeviceAdd (PVOID     pvHost,
                                CPCHAR    pcDeviceName)
{
    __PSDHCI_HOST   psdhcihost   = LW_NULL;
    __PSDHCI_DEVICE psdhcidevice = LW_NULL;
    PLW_SD_DEVICE   psddevice    = LW_NULL;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  ((PVOID)0);
    }

    psdhcihost = (__PSDHCI_HOST)pvHost;

    if (API_AtomicGet(&psdhcihost->SDHCIHS_atomicDevCnt) >= __SDHCI_HOST_DEVNUM_MAX) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "can not add more devices.\r\n");
        return  ((PVOID)0);
    }

    psddevice = API_SdDeviceGet(__SDHCI_HOST_NAME(psdhcihost), pcDeviceName);
    if (!psddevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "find sd device failed.\r\n");
       return  ((PVOID)0);
    }

    psdhcidevice = (__PSDHCI_DEVICE)__SHEAP_ALLOC(sizeof(__SDHCI_DEVICE));
    if (!psdhcidevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  ((PVOID)0);
    }

    lib_bzero(psdhcidevice, sizeof(__SDHCI_DEVICE));

    psdhcidevice->SDHCIDEV_psddevice  = psddevice;
    psdhcidevice->SDHCIDEV_psdhcihost = psdhcihost;

    psddevice->SDDEV_pvUsr            = psdhcidevice;                   /*  ���û�����                */
    psdhcihost->SDHCIHS_ucDevType     = psddevice->SDDEV_ucType;        /*  ��¼�豸����                */

    API_AtomicInc(&psdhcihost->SDHCIHS_atomicDevCnt);                   /*  �豸++                      */

    return  ((PVOID)psdhcidevice);
}
/*********************************************************************************************************
** ��������: __sdhciDeviceRemove
** ��������: �� SD ��׼�����������Ƴ�һ�� SD �豸
** ��    ��: pvDevice  Ҫ�Ƴ��� SD �豸ָ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciDeviceRemove (PVOID pvDevice)
{
    __PSDHCI_HOST   psdhcihost   = LW_NULL;
    __PSDHCI_DEVICE psdhcidevice = LW_NULL;

    if (!pvDevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcidevice  = (__PSDHCI_DEVICE)pvDevice;
    psdhcihost    = psdhcidevice->SDHCIDEV_psdhcihost;

    psdhcidevice->SDHCIDEV_psddevice->SDDEV_pvUsr = LW_NULL;            /*  �û�������Ч                */
    __SHEAP_FREE(psdhcidevice);
    API_AtomicDec(&psdhcihost->SDHCIHS_atomicDevCnt);                   /*  �豸--                      */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdhciDeviceCheckNotify
** ��������: ����֪ͨ SDHCI HOST ����һ���豸���
** ��    ��: pvHost   ����������
**           iDevSta  �豸״̬
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API VOID  API_SdhciDeviceCheckNotify (PVOID pvHost, INT iDevSta)
{
    __PSDHCI_HOST     psdhcihost    = LW_NULL;
    __PSDHCI_SDM_HOST psdhcisdmhost = LW_NULL;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return;
    }

    psdhcihost    = (__PSDHCI_HOST)pvHost;
    psdhcisdmhost = psdhcihost->SDHCIHS_psdhcisdmhost;

    if (psdhcisdmhost->SDHCISDMH_callbackChkDev) {
        psdhcisdmhost->SDHCISDMH_callbackChkDev(psdhcisdmhost->SDHCISDMH_pvCallBackArg, iDevSta);
    }
}
/*********************************************************************************************************
** ��������: API_SdhciDeviceUsageInc
** ��������: �������ϵ��豸��ʹ�ü�������һ
** ��    ��: pvHost   ����������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdhciDeviceUsageInc (PVOID  pvHost)
{
    __PSDHCI_HOST   psdhcihost   = LW_NULL;
    __PSDHCI_DEVICE psdhcidevice = LW_NULL;
    INT             iError;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost   = (__PSDHCI_HOST)pvHost;
    psdhcidevice = (__PSDHCI_DEVICE)psdhcihost->SDHCIHS_psdhcisdmhost->SDHCISDMH_pvDevAttached;
    if (!psdhcidevice) {
        return  (PX_ERROR);
    }

    iError = API_SdDeviceUsageInc(psdhcidevice->SDHCIDEV_psddevice);

    return  (iError);
}
/*********************************************************************************************************
** ��������: API_SdhciDeviceUsageDec
** ��������: �������ϵ��豸��ʹ�ü�����һ
** ��    ��: pvHost   ����������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdhciDeviceUsageDec (PVOID  pvHost)
{
    __PSDHCI_HOST   psdhcihost   = LW_NULL;
    __PSDHCI_DEVICE psdhcidevice = LW_NULL;
    INT             iError;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost   = (__PSDHCI_HOST)pvHost;
    psdhcidevice = (__PSDHCI_DEVICE)psdhcihost->SDHCIHS_psdhcisdmhost->SDHCISDMH_pvDevAttached;
    if (!psdhcidevice) {
        return  (PX_ERROR);
    }

    iError = API_SdDeviceUsageDec(psdhcidevice->SDHCIDEV_psddevice);

    return  (iError);
}
/*********************************************************************************************************
** ��������: API_SdhciDeviceUsageGet
** ��������: ��ÿ������ϵ��豸��ʹ�ü���
** ��    ��: pvHost   ����������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdhciDeviceUsageGet (PVOID  pvHost)
{
    __PSDHCI_HOST   psdhcihost   = LW_NULL;
    __PSDHCI_DEVICE psdhcidevice = LW_NULL;
    INT             iError;

    if (!pvHost) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost   = (__PSDHCI_HOST)pvHost;
    psdhcidevice = (__PSDHCI_DEVICE)psdhcihost->SDHCIHS_psdhcisdmhost->SDHCISDMH_pvDevAttached;
    if (!psdhcidevice) {
        return  (PX_ERROR);
    }

    iError = API_SdDeviceUsageGet(psdhcidevice->SDHCIDEV_psddevice);

    return  (iError);
}
/*********************************************************************************************************
** ��������: __sdhciTransferNorm
** ��������: һ�㴫��
** ��    ��: psdhcihost  SD ��������
**           psddev      SD �豸
**           psdmsg      SD ���������Ϣ��
**           iNum        ��Ϣ����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciTransferNorm (__PSDHCI_HOST       psdhcihost,
                                PLW_SD_DEVICE       psddev,
                                PLW_SD_MESSAGE      psdmsg,
                                INT                 iNum)
{
    __PSDHCI_TRANS  psdhcitrans;
    INT             iError;
    INT             i = 0;

    psdhcitrans = psdhcihost->SDHCIHS_psdhcitrans;

    while ((i < iNum) && (psdmsg != LW_NULL)) {
        __SDHCI_TRANS_LOCK(psdhcitrans);                                /*  ��������                    */
        iError = __sdhciTransPrepare(psdhcitrans, psdmsg, __SDHIC_TRANS_NORMAL);
        if (iError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }

        __SDHCI_TRANS_UNLOCK(psdhcitrans);

        iError = __sdhciTransStart(psdhcitrans);
        if (iError != ERROR_NONE) {
            return  (PX_ERROR);
        }

        iError = __sdhciTransFinishWait(psdhcitrans);                   /*  �ȴ����δ������            */
        if (iError != ERROR_NONE) {
            return  (PX_ERROR);
        }

        __SDHCI_TRANS_LOCK(psdhcitrans);                                /*  ��������                    */
        __sdhciTransClean(psdhcitrans);                                 /*  �����δ���                */
        if (psdhcitrans->SDHCITS_iCmdError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }

        if (psdhcitrans->SDHCITS_iDatError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }
        __SDHCI_TRANS_UNLOCK(psdhcitrans);                              /*  ��������                    */

        i++;
        psdmsg++;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciTransferSdma
** ��������: SDMA ����
** ��    ��: psdhcihost  SD ��������
**           psddev      SD �豸
**           psdmsg      SD ���������Ϣ��
**           iNum        ��Ϣ����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciTransferSdma (__PSDHCI_HOST       psdhcihost,
                                PLW_SD_DEVICE       psddev,
                                PLW_SD_MESSAGE      psdmsg,
                                INT                 iNum)
{
    __PSDHCI_TRANS  psdhcitrans;
    INT             iError;
    INT             i = 0;

    psdhcitrans = psdhcihost->SDHCIHS_psdhcitrans;

    while ((i < iNum) && (psdmsg != LW_NULL)) {
        __SDHCI_TRANS_LOCK(psdhcitrans);                                /*  ��������                    */
        iError = __sdhciTransPrepare(psdhcitrans, psdmsg, __SDHIC_TRANS_SDMA);
        if (iError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }
        iError = __sdhciTransStart(psdhcitrans);
        if (iError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }
        __SDHCI_TRANS_UNLOCK(psdhcitrans);                              /*  ��������                    */

        iError = __sdhciTransFinishWait(psdhcitrans);                   /*  �ȴ����δ������            */
        if (iError != ERROR_NONE) {
            return  (PX_ERROR);
        }

        __SDHCI_TRANS_LOCK(psdhcitrans);                                /*  ��������                    */
        __sdhciTransClean(psdhcitrans);                                 /*  �����δ���                */
        if (psdhcitrans->SDHCITS_iCmdError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }

        if (psdhcitrans->SDHCITS_iDatError != ERROR_NONE) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            return  (PX_ERROR);
        }
        __SDHCI_TRANS_UNLOCK(psdhcitrans);                              /*  ��������                    */

        i++;
        psdmsg++;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciTransferAdma
** ��������: ADMA����
** ��    ��: psdadapter  SD ��������
**           psddev      SD �豸
**           psdmsg      SD ���������Ϣ��
**           iNum        ��Ϣ����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciTransferAdma (__PSDHCI_HOST       psdhcihost,
                                PLW_SD_DEVICE       psddev,
                                PLW_SD_MESSAGE      psdmsg,
                                INT                 iNum)
{
    /*
     *  TODO: adma mode support.
     */
    __sdhciDataPrepareAdma(LW_NULL);
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdhciTransfer
** ��������: ���ߴ��亯��
** ��    ��: psdadapter  SD ����������
**           iCmd        ��������
**           lArg        ����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciTransfer (PLW_SD_ADAPTER      psdadapter,
                            PLW_SD_DEVICE       psddev,
                            PLW_SD_MESSAGE      psdmsg,
                            INT                 iNum)
{
    __PSDHCI_HOST  psdhcihost = LW_NULL;
    INT            iError     = ERROR_NONE;

    if (!psdadapter) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost = (__PSDHCI_HOST)psdadapter->SDADAPTER_psdfunc;
    if (!psdhcihost) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no sdhci host.\r\n");
        return  (PX_ERROR);
    }

    /*
     * ִ�������϶�Ӧ�����ߴ��亯��
     */
    iError = psdhcihost->SDHCIHS_pfuncMasterXfer(psdhcihost, psddev, psdmsg, iNum);

    return  (iError);
}
/*********************************************************************************************************
** ��������: __sdhciIoCtl
** ��������: SD ������IO���ƺ���
** ��    ��: psdadapter  SD ����������
**           iCmd        ��������
**           lArg        ����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciIoCtl (PLW_SD_ADAPTER  psdadapter,
                         INT             iCmd,
                         LONG            lArg)
{
    __PSDHCI_HOST  psdhcihost = LW_NULL;
    INT            iError     = ERROR_NONE;

    if (!psdadapter) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdhcihost = (__PSDHCI_HOST)psdadapter->SDADAPTER_psdfunc;
    if (!psdhcihost) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no sdhci host.\r\n");
        return  (PX_ERROR);
    }

    switch (iCmd) {
    
    case SDBUS_CTRL_POWEROFF:
        iError = __sdhciPowerOff(psdhcihost);
        break;

    case SDBUS_CTRL_POWERUP:
    case SDBUS_CTRL_POWERON:
        iError = __sdhciPowerOn(psdhcihost);
        break;

    case SDBUS_CTRL_SETBUSWIDTH:
        iError = __sdhciBusWidthSet(psdhcihost, (UINT32)lArg);
        break;

    case SDBUS_CTRL_SETCLK:
        if (lArg == SDARG_SETCLK_MAX) {
            iError = __sdhciHighSpeedEn(psdhcihost, LW_TRUE);
            if (iError != ERROR_NONE) {
                break;
            }
        } else {
            __sdhciHighSpeedEn(psdhcihost, LW_FALSE);
        }
        iError = __sdhciClockSet(psdhcihost, (UINT32)lArg);
        break;

    case SDBUS_CTRL_STOPCLK:
        iError = __sdhciClockStop(psdhcihost);
        break;

    case SDBUS_CTRL_STARTCLK:
        iError = __sdhciClockSet(psdhcihost, psdhcihost->SDHCIHS_uiClkCurr);
        break;

    case SDBUS_CTRL_DELAYCLK:
        break;

    case SDBUS_CTRL_GETOCR:
        *(UINT32 *)lArg = psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_uiVoltage;
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
** ��������: __sdhciHostCapDecode
** ��������: �������صĹ��ܼĴ���
** ��    ��: psdhcihostattr  ��������
             psdhcicap       ���ܽṹָ��
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciHostCapDecode (PLW_SDHCI_HOST_ATTR psdhcihostattr, __PSDHCI_CAPAB psdhcicap)
{
    UINT32  uiCapReg = 0;
    UINT32  uiTmp    = 0;

    if (!psdhcicap) {
        _ErrorHandle(EINVAL);
        return;
    }

    uiCapReg = SDHCI_READL(psdhcihostattr, SDHCI_CAPABILITIES);

    psdhcicap->SDHCICAP_uiBaseClkFreq  = (uiCapReg & SDHCI_CAP_BASECLK_MASK) >> SDHCI_CAP_BASECLK_SHIFT;
    psdhcicap->SDHCICAP_uiMaxBlkSize   = (uiCapReg & SDHCI_CAP_MAXBLK_MASK) >> SDHCI_CAP_MAXBLK_SHIFT;
    psdhcicap->SDHCICAP_bCanSdma       = (uiCapReg & SDHCI_CAP_CAN_DO_SDMA) ? LW_TRUE : LW_FALSE;
    psdhcicap->SDHCICAP_bCanAdma       = (uiCapReg & SDHCI_CAP_CAN_DO_ADMA) ? LW_TRUE : LW_FALSE;
    psdhcicap->SDHCICAP_bCanHighSpeed  = (uiCapReg & SDHCI_CAP_CAN_DO_HISPD) ? LW_TRUE : LW_FALSE;
    psdhcicap->SDHCICAP_bCanSusRes     = (uiCapReg & SDHCI_CAP_CAN_DO_SUSRES) ? LW_TRUE : LW_FALSE;

    psdhcicap->SDHCICAP_uiBaseClkFreq *= 1000000;                       /*  תΪʵ��Ƶ��                */
    psdhcicap->SDHCICAP_uiMaxBlkSize   = 512 << psdhcicap->SDHCICAP_uiMaxBlkSize;
                                                                        /*  ת��Ϊʵ�������С        */
    /*
     * ����ڴ˼Ĵ������ʱ��Ϊ0, ��˵��������û���Լ��ڲ�ʱ��,
     * ������Դ������ʱ��Դ. ��˲��������ʱ��Դ��Ϊ��ʱ��.
     */
    if (psdhcicap->SDHCICAP_uiBaseClkFreq == 0) {
        uiTmp = psdhcihostattr->SDHCIHOST_uiMaxClock;
        if (uiTmp == 0) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "sdhci without clock source .\r\n");
            return;
        }
        psdhcicap->SDHCICAP_uiBaseClkFreq = uiTmp;
    }

    uiTmp = 0;
    if ((uiCapReg & SDHCI_CAP_CAN_VDD_330) != 0) {
        uiTmp |= __SDHCI_OCR_330;
    }
    if ((uiCapReg & SDHCI_CAP_CAN_VDD_300) != 0) {
        uiTmp |= __SDHCI_OCR_300;
    }
    if ((uiCapReg & SDHCI_CAP_CAN_VDD_180) != 0) {
        uiTmp |= __SDHCI_OCR_180;
    }

    if (uiTmp == 0) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "sdhci without voltage information.\r\n");
        return;
    }
    psdhcicap->SDHCICAP_uiVoltage = uiTmp;

#ifdef  __SYLIXOS_DEBUG
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */
    printk("\nhost capablity >>\n");
    printk("uiCapReg       : %08x\n", uiCapReg);
    printk("base clock     : %u\n", psdhcicap->SDHCICAP_uiBaseClkFreq);
    printk("max block size : %u\n", psdhcicap->SDHCICAP_uiMaxBlkSize);
    printk("can sdma       : %s\n", psdhcicap->SDHCICAP_bCanSdma ? "yes" : "no");
    printk("can adma       : %s\n", psdhcicap->SDHCICAP_bCanAdma ? "yes" : "no");
    printk("can high speed : %s\n", psdhcicap->SDHCICAP_bCanHighSpeed ? "yes" : "no");
    printk("voltage support: %08x\n", psdhcicap->SDHCICAP_uiVoltage);
#endif                                                                  /*  __SYLIXOS_DEBUG             */
}
/*********************************************************************************************************
** ��������: __sdhciClockSet
** ��������: ʱ��Ƶ�����ò�ʹ��
** ��    ��: psdhcihost      SDHCI HOST �ṹָ��
**           uiSetClk        Ҫ���õ�ʱ��Ƶ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciClockSet (__PSDHCI_HOST  psdhcihost, UINT32 uiSetClk)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT32              uiBaseClk      = psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_uiBaseClkFreq;

    UINT16              usDivClk       = 0;
    UINT                uiTimeout      = 30;

    if ((uiSetClk == psdhcihost->SDHCIHS_uiClkCurr) &&
        (psdhcihost->SDHCIHS_bClkEnable)) {
        return  (ERROR_NONE);
    }

    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncClockSet) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncClockSet(psdhcihostattr, uiSetClk);
        goto __ret;
    }

    if (uiSetClk > uiBaseClk) {
        uiSetClk = uiBaseClk;
    }

    SDHCI_WRITEW(psdhcihostattr, SDHCI_CLOCK_CONTROL, 0);               /*  ��ֹʱ��ģ�������ڲ�����    */

    /*
     * �����Ƶ����
     */
    if (uiSetClk < uiBaseClk) {
        usDivClk = (UINT16)((uiBaseClk + uiSetClk - 1) / uiSetClk);
        if (usDivClk <= 2) {
            usDivClk = 1 << 0;
        } else if (usDivClk <= 4) {
            usDivClk = 1 << 1;
        } else if (usDivClk <= 8) {
            usDivClk = 1 << 2;
        } else if (usDivClk <= 16) {
            usDivClk = 1 << 3;
        } else if (usDivClk <= 32) {
            usDivClk = 1 << 4;
        } else if (usDivClk <= 64) {
            usDivClk = 1 << 5;
        } else if (usDivClk <= 128) {
            usDivClk = 1 << 6;
        } else {
            usDivClk = 1 << 7;
        }
    }

#ifdef __SYLIXOS_DEBUG
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */
        printk("sdhci current clock: %uHz\n", uiBaseClk / (usDivClk << 1));
#endif                                                                  /*  __SYLIXOS_DEBUG             */

    usDivClk <<= SDHCI_CLKCTL_DIVIDER_SHIFT;
    usDivClk  |= SDHCI_CLKCTL_INTER_EN;                                 /*  �ڲ�ʱ��ʹ��                */
    SDHCI_WRITEW(psdhcihostattr, SDHCI_CLOCK_CONTROL, usDivClk);

    /*
     * �ȴ�ʱ���ȶ�
     */
    while (1) {
        usDivClk = SDHCI_READW(psdhcihostattr, SDHCI_CLOCK_CONTROL);
        if (usDivClk & SDHCI_CLKCTL_INTER_STABLE) {
            break;
        }

        uiTimeout--;
        if (uiTimeout == 0) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait internal clock to be stable timeout.\r\n");
            return  (PX_ERROR);
        }
        SDHCI_DELAYMS(1);
    }

    usDivClk |= SDHCI_CLKCTL_CLOCK_EN;                                  /*  SDCLK �豸ʱ��ʹ��          */
    SDHCI_WRITEW(psdhcihostattr, SDHCI_CLOCK_CONTROL, usDivClk);

__ret:
    psdhcihost->SDHCIHS_uiClkCurr  = uiSetClk;
    psdhcihost->SDHCIHS_bClkEnable = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciClockStop
** ��������: ֹͣʱ�����
** ��    ��: psdhcihost      SDHCI HOST �ṹָ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciClockStop (__PSDHCI_HOST    psdhcihost)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT32              uiData;

    if (!psdhcihost->SDHCIHS_bClkEnable) {
        return  (ERROR_NONE);
    }

    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncClockStop) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncClockStop(psdhcihostattr);
        goto __ret;
    }

    uiData = SDHCI_READW(psdhcihostattr, SDHCI_CLOCK_CONTROL) & (~SDHCI_CLKCTL_CLOCK_EN);
    SDHCI_WRITEW(psdhcihostattr, SDHCI_CLOCK_CONTROL, uiData);

__ret:
    psdhcihost->SDHCIHS_bClkEnable = LW_FALSE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciBusWidthSet
** ��������: ��������λ������
** ��    ��: psdhcihost      SDHCI HOST �ṹָ��
**           uiBusWidth      Ҫ���õ�����λ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciBusWidthSet (__PSDHCI_HOST  psdhcihost, UINT32 uiBusWidth)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    BOOL                bSdioIntEn     = psdhcihost->SDHCIHS_bSdioIntEnable;
    UINT8               ucCtl;

    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_CANNOT_SDIO_INT) &&
        !SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_SDIO_INT_OOB)) {
        __sdhciSdioIntEn(psdhcihost, LW_FALSE);                         /*  ��ֹ���ж�                  */
    }

    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncBusWidthSet) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncBusWidthSet(psdhcihostattr, uiBusWidth);
        goto __ret;
    }

    ucCtl  = SDHCI_READB(psdhcihostattr, SDHCI_HOST_CONTROL);

    switch (uiBusWidth) {
    case SDARG_SETBUSWIDTH_1:
        ucCtl &= ~SDHCI_HCTRL_8BITBUS;
        ucCtl &= ~SDHCI_HCTRL_4BITBUS;
        break;

    case SDARG_SETBUSWIDTH_4:
        ucCtl &= ~SDHCI_HCTRL_8BITBUS;
        ucCtl |= SDHCI_HCTRL_4BITBUS;
        break;

    case SDARG_SETBUSWIDTH_8:
        if (psdhcihost->SDHCIHS_uiVersion > SDHCI_HVER_SPEC_200) {
            ucCtl &= ~SDHCI_HCTRL_4BITBUS;
            ucCtl |= SDHCI_HCTRL_8BITBUS;
        } else {
            SDCARD_DEBUG_MSG(__LOGMESSAGE_LEVEL, "sdhci host can't support 8-bit width.\r\n");
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        break;

    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter of bus width error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    SDHCI_WRITEB(psdhcihostattr, SDHCI_HOST_CONTROL, ucCtl);

__ret:
    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_CANNOT_SDIO_INT) &&
        !SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_SDIO_INT_OOB)) {
        __sdhciSdioIntEn(psdhcihost, bSdioIntEn);                         /*  �ָ�֮ǰ�Ŀ��ж�����      */
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciPowerOn
** ��������: �򿪵�Դ
** ��    ��: psdhcihost     SDHCI HOST �ṹָ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciPowerOn (__PSDHCI_HOST  psdhcihost)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT8               ucPow;

    /*
     * ƽ̨���Ӳ����λ
     */
    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncHwReset) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncHwReset(psdhcihostattr);
    }

    /*
     * ��������׼��λ����
     */
    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DONOT_SET_POWER)) {
        ucPow  = SDHCI_READB(psdhcihostattr, SDHCI_POWER_CONTROL);
        ucPow |= SDHCI_POWCTL_ON;
        SDHCI_WRITEB(psdhcihostattr, SDHCI_POWER_CONTROL, ucPow);

        if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DO_RESET_AFTER_SET_POWER_ON)) {
            __sdhciHostReset(psdhcihost, SDHCI_SFRST_CMD);
            __sdhciHostReset(psdhcihost, SDHCI_SFRST_DATA);
        }
    }

    /*
     * �����Դ����������������޹�
     */
    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncExtraPowerSet) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncExtraPowerSet(psdhcihostattr, LW_TRUE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciPowerff
** ��������: �رյ�Դ
** ��    ��: psdhcihost       SDHCI HOST �ṹָ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciPowerOff (__PSDHCI_HOST  psdhcihost)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT8               ucPow;

    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DONOT_SET_POWER)) {
        ucPow  = SDHCI_READB(psdhcihostattr, SDHCI_POWER_CONTROL);
        ucPow &= ~SDHCI_POWCTL_ON;
        SDHCI_WRITEB(psdhcihostattr, SDHCI_POWER_CONTROL, ucPow);
    }

    /*
     * �����Դ����������������޹�
     */
    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncExtraPowerSet) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncExtraPowerSet(psdhcihostattr, LW_FALSE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciPowerSetVol
** ��������: ��Դ���õ�ѹ
** ��    ��: psdhcihost      SDHCI HOST �ṹָ��
**           ucVol           ��ѹ
**           bIsOn           ����֮���Դ�Ƿ���
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciPowerSetVol (__PSDHCI_HOST  psdhcihost,
                               UINT8          ucVol,
                               BOOL           bIsOn)
{
    if (bIsOn) {
        ucVol |= SDHCI_POWCTL_ON;
    } else {
        ucVol &= ~SDHCI_POWCTL_ON;
    }

    __sdhciPowerOff(psdhcihost);

    SDHCI_WRITEB(&psdhcihost->SDHCIHS_sdhcihostattr, SDHCI_POWER_CONTROL, ucVol);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciHighSpeedEn
** ��������: ���ظ�������
** ��    ��: psdhcihost      SDHCI HOST �ṹָ��
**           bEnable         �Ƿ�ʹ�ܸ���
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciHighSpeedEn (__PSDHCI_HOST  psdhcihost,  BOOL bEnable)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT8               ucDat;

    ucDat  = SDHCI_READB(psdhcihostattr, SDHCI_HOST_CONTROL);
    if (bEnable) {
        if (!psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_bCanHighSpeed) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "host don't suport highspeed mode.\r\n");
            return  (PX_ERROR);
        }
        ucDat |= SDHCI_HCTRL_HISPD;
    } else {
        ucDat &= ~SDHCI_HCTRL_HISPD;
    }

    SDHCI_WRITEB(psdhcihostattr, SDHCI_HOST_CONTROL, ucDat);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciSdmHostNew
** ��������: ��������ʼ��һ�� SDM �涨�Ŀ���������
** ��    ��: psdhcihost    SDHCI �������ڲ��ṹ����
** ��    ��: ����ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static __SDHCI_SDM_HOST *__sdhciSdmHostNew (__PSDHCI_HOST   psdhcihost)
{
    __SDHCI_SDM_HOST *psdhcisdmhost;
    SD_HOST          *psdhost;
    INT               iCapablity;
    PVOID             pvSdmHost;

    if (!psdhcihost) {
        return  (LW_NULL);
    }

    psdhcisdmhost = (__SDHCI_SDM_HOST *)__SHEAP_ALLOC(sizeof(__SDHCI_SDM_HOST));
    if (!psdhcisdmhost) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        return  (LW_NULL);
    }
    lib_bzero(psdhcisdmhost, sizeof(__SDHCI_SDM_HOST));

    iCapablity = SDHOST_CAP_DATA_4BIT;

    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_QUIRK_FLG_CAN_DATA_8BIT)) {
        iCapablity |= SDHOST_CAP_DATA_8BIT;
    }
    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_QUIRK_FLG_CAN_DATA_4BIT_DDR)) {
        iCapablity |= SDHOST_CAP_DATA_4BIT_DDR;
    }
    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_QUIRK_FLG_CAN_DATA_8BIT_DDR)) {
        iCapablity |= SDHOST_CAP_DATA_8BIT_DDR;
    }
    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_QUIRK_FLG_MMC_FORCE_1BIT)) {
        iCapablity |= SDHOST_CAP_MMC_FORCE_1BIT;
    }
    if (psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_bCanHighSpeed) {
        iCapablity |= SDHOST_CAP_HIGHSPEED;
    }
    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_QUIRK_FLG_CANNOT_HIGHSPEED)) {
        iCapablity &= ~SDHOST_CAP_HIGHSPEED;
    }
    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
                        SDHCI_QUIRK_FLG_SDIO_FORCE_1BIT)) {
        iCapablity |= SDHOST_CAP_SDIO_FORCE_1BIT;
    }
    if (SDHCI_QUIRK_FLG(&psdhcihost->SDHCIHS_sdhcihostattr,
    					SDHCI_QUIRK_FLG_SD_FORCE_1BIT)) {
        iCapablity |= SDHOST_CAP_SD_FORCE_1BIT;
    }

    psdhost = &psdhcisdmhost->SDHCISDMH_sdhost;

    psdhost->SDHOST_cpcName                = __SDHCI_HOST_NAME(psdhcihost);
    psdhost->SDHOST_iType                  = SDHOST_TYPE_SD;
    psdhost->SDHOST_iCapbility             = iCapablity;
    psdhost->SDHOST_pfuncCallbackInstall   = __sdhciSdmCallBackInstall;
    psdhost->SDHOST_pfuncCallbackUnInstall = __sdhciSdmCallBackUnInstall;
    psdhost->SDHOST_pfuncSdioIntEn         = __sdhciSdmSdioIntEn;
    psdhost->SDHOST_pfuncIsCardWp          = __sdhciSdmIsCardWp;
    psdhost->SDHOST_pfuncDevAttach         = __sdhciSdmDevAttach;
    psdhost->SDHOST_pfuncDevDetach         = __sdhciSdmDevDetach;

    psdhcisdmhost->SDHCISDMH_psdhcihost    = psdhcihost;
    psdhcihost->SDHCIHS_psdhcisdmhost      = psdhcisdmhost;

    pvSdmHost = API_SdmHostRegister(psdhost);
    if (!pvSdmHost) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "can't register into sdm modules.\r\n");
        __SHEAP_FREE(psdhcisdmhost);
        return  (LW_NULL);

    }
    psdhcisdmhost->SDHCISDMH_pvSdmHost = pvSdmHost;

    return  (psdhcisdmhost);
}
/*********************************************************************************************************
** ��������: __sdhciSdmHostDel
** ��������: ���� SDM �涨�Ŀ���������
** ��    ��: psdhcisdmhost    SDM ��涨�� HOST ��Ϣ�ṹ
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciSdmHostDel ( __SDHCI_SDM_HOST  *psdhcisdmhost)
{
    PVOID  pvSdmHost;

    pvSdmHost = psdhcisdmhost->SDHCISDMH_pvSdmHost;
    if (API_SdmHostUnRegister(pvSdmHost) != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "can't unregister from sdm modules.\r\n");
        return  (PX_ERROR);
    }

    __SHEAP_FREE(psdhcisdmhost);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciSdmCallBackInstall
** ��������: ��װ�ص�����
** ��    ��: psdhost          SDM ��涨�� HOST ��Ϣ�ṹ
**           iCallbackType    �ص���������
**           callback         �ص�����ָ��
**           pvCallbackArg    �ص���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciSdmCallBackInstall (SD_HOST       *psdhost,
                                       INT            iCallbackType,
                                       SD_CALLBACK    callback,
                                       PVOID          pvCallbackArg)
{
    __SDHCI_SDM_HOST *psdhcisdmhost = (__SDHCI_SDM_HOST *)psdhost;
    if (!psdhcisdmhost) {
        return  (PX_ERROR);
    }

    if (iCallbackType == SDHOST_CALLBACK_CHECK_DEV) {
        psdhcisdmhost->SDHCISDMH_callbackChkDev = callback;
        psdhcisdmhost->SDHCISDMH_pvCallBackArg  = pvCallbackArg;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciSdmCallBackUnInstall
** ��������: ע����װ�Ļص�����
** ��    ��: psdhost          SDM ��涨�� HOST ��Ϣ�ṹ
**           iCallbackType    �ص���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciSdmCallBackUnInstall (SD_HOST    *psdhost,
                                         INT         iCallbackType)
{
    __SDHCI_SDM_HOST *psdhcisdmhost = (__SDHCI_SDM_HOST *)psdhost;
    if (!psdhcisdmhost) {
        return  (PX_ERROR);
    }

    if (iCallbackType == SDHOST_CALLBACK_CHECK_DEV) {
        psdhcisdmhost->SDHCISDMH_callbackChkDev = LW_NULL;
        psdhcisdmhost->SDHCISDMH_pvCallBackArg  = LW_NULL;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciSdmIsCardWp
** ��������: �жϸ� HOST �϶�Ӧ�Ŀ��Ƿ�д����
** ��    ��: psdhost          SDM ��涨�� HOST ��Ϣ�ṹ
** ��    ��: LW_TRUE: ��д����    LW_FALSE: ��δд����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL __sdhciSdmIsCardWp (SD_HOST *psdhost)
{
    __SDHCI_SDM_HOST   *psdhcisdmhost  = (__SDHCI_SDM_HOST *)psdhost;
    LW_SDHCI_HOST_ATTR *psdhcihostattr = LW_NULL;
    BOOL                bIsWp          = LW_FALSE;

    if (!psdhcisdmhost) {
        return  (LW_FALSE);
    }

    psdhcihostattr = &psdhcisdmhost->SDHCISDMH_psdhcihost->SDHCIHS_sdhcihostattr;
    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncIsCardWp) {
        bIsWp = psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncIsCardWp(psdhcihostattr);
    }

    return  (bIsWp);
}
/*********************************************************************************************************
** ��������: __sdhciSdmSdioIntEn
** ��������: ʹ�� SDIO �ж�
** ��    ��: psdhost          SDM ��涨�� HOST ��Ϣ�ṹ
**           bEnable          �Ƿ�ʹ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciSdmSdioIntEn (SD_HOST *psdhost,  BOOL bEnable)
{
    __SDHCI_SDM_HOST    *psdhcisdmhost  = (__SDHCI_SDM_HOST *)psdhost;
    __SDHCI_HOST        *psdhcihost     = psdhcisdmhost->SDHCISDMH_psdhcihost;
    LW_SDHCI_HOST_ATTR  *psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;

    if (!psdhcisdmhost) {
        return;
    }

    psdhcihost     = psdhcisdmhost->SDHCISDMH_psdhcihost;
    psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;

    if(SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_SDIO_INT_OOB)) {
        if (psdhcihostattr->SDHCIHOST_pquirkop &&
            psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncOOBInterSet) {
            psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncOOBInterSet(psdhcihostattr, bEnable);
        }
        return;
    }

    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_CANNOT_SDIO_INT)) {
        if (psdhcihost->SDHCIHS_bSdioIntEnable == bEnable) {
            return;
        }
        if (bEnable) {
            __SDHCI_SDIO_ENABLE(psdhcihost->SDHCIHS_psdhcitrans);
        } else {
            __SDHCI_SDIO_DISABLE(psdhcihost->SDHCIHS_psdhcitrans);
        }
        psdhcihost->SDHCIHS_bSdioIntEnable = bEnable;

    } else {
        __sdhciSdioIntEn(psdhcihost, bEnable);
    }
}
/*********************************************************************************************************
** ��������: __sdhciSdmDevAttach
** ��������: ����豸
** ��    ��: psdhost          SDM ��涨�� HOST ��Ϣ�ṹ
**           cpcDevName       �豸������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdhciSdmDevAttach (SD_HOST *psdhost, CPCHAR cpcDevName)
{
    __SDHCI_SDM_HOST *psdhcisdmhost = (__SDHCI_SDM_HOST *)psdhost;
    PVOID             pvDevAttached;

    if (!psdhcisdmhost) {
        return;
    }

    pvDevAttached = __sdhciDeviceAdd(psdhcisdmhost->SDHCISDMH_psdhcihost, cpcDevName);
    if (!pvDevAttached) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device attached failed.\r\n");
        return;
    }

    psdhcisdmhost->SDHCISDMH_pvDevAttached = pvDevAttached;
}
/*********************************************************************************************************
** ��������: __sdhciSdmDevDetach
** ��������: ɾ���豸
** ��    ��: psdhost          SDM ��涨�� HOST ��Ϣ�ṹ
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdhciSdmDevDetach (SD_HOST *psdhost)
{
    __SDHCI_SDM_HOST *psdhcisdmhost = (__SDHCI_SDM_HOST *)psdhost;

    if (!psdhcisdmhost) {
        return;
    }

    __sdhciDeviceRemove(psdhcisdmhost->SDHCISDMH_pvDevAttached);
}
/*********************************************************************************************************
** ��������: __sdhciCmdSend
** ��������: �����
** ��    ��: psdhcihost            SDHCI HOST �ṹָ��
**           psdcmd                ����ṹָ��
**           psddat                ���ݴ�����ƽṹ
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciCmdSend (__PSDHCI_HOST   psdhcihost,
                            PLW_SD_COMMAND  psdcmd,
                            PLW_SD_DATA     psddat)
{
    PLW_SDHCI_HOST_ATTR   psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    __PSDHCI_TRANS        psdhcitrans    = psdhcihost->SDHCIHS_psdhcitrans;
    UINT32                uiMask;
    UINT                  uiTimeout;
    INT                   iCmdFlg;

    struct timespec       tvOld;
    struct timespec       tvNow;

    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DONOT_CHECK_BUSY_BEFORE_CMD_SEND)) {
        uiMask = SDHCI_PSTA_CMD_INHIBIT;
        if (psddat || SD_CMD_TEST_RSP(psdcmd, SD_RSP_BUSY)) {
            uiMask |= SDHCI_PSTA_DATA_INHIBIT;
        }

        if (psdcmd == psdhcitrans->SDHCITS_psdcmdStop) {
            uiMask &= ~SDHCI_PSTA_DATA_INHIBIT;
        }

        /*
         * �ȴ�����(����)�߿���
         */
        uiTimeout = 0;
        lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
        
        while (SDHCI_READL(psdhcihostattr, SDHCI_PRESENT_STATE) & uiMask) {
            uiTimeout++;
            if (uiTimeout > __SDHCI_CMD_RETRY) {
                goto    __timeout;
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __SDHCI_CMD_TOUT_SEC) {
                goto    __timeout;
            }
        }
    }

    if ((psdcmd->SDCMD_uiFlag & SD_RSP_BUSY) || psddat) {
        __sdhciTimeoutSet(psdhcihost);
    }

    /*
     * д�����Ĵ���
     */
    SDHCI_WRITEL(psdhcihostattr, SDHCI_ARGUMENT, psdcmd->SDCMD_uiArg);

    /*
     * ���ô���ģʽ
     */
    if (psddat) {
        __sdhciTransferModSet(psdhcihost);
    } else {
        uiMask = SDHCI_READW(psdhcihostattr, SDHCI_TRANSFER_MODE);
        uiMask &=  ~(SDHCI_TRNS_ACMD12 | SDHCI_TRNS_ACMD23);
        SDHCI_WRITEW(psdhcihostattr, SDHCI_TRANSFER_MODE, uiMask);
    }

    if ((psdcmd->SDCMD_uiFlag & SD_RSP_136) && (psdcmd->SDCMD_uiFlag & SD_RSP_BUSY)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "response type unavailable.\r\n");
        psdhcitrans->SDHCITS_iCmdError = PX_ERROR;
        return  (PX_ERROR);
    }

    /*
     * �����
     */
    iCmdFlg = __sdhciCmdRespType(psdcmd);                               /*  Ӧ������                    */
    if (SD_CMD_TEST_RSP(psdcmd, SD_RSP_CRC)) {
        iCmdFlg |= SDHCI_CMD_CRC_CHK;
    }
    if (SD_CMD_TEST_RSP(psdcmd, SD_RSP_OPCODE)) {
        iCmdFlg |= SDHCI_CMD_INDEX_CHK;
    }
    if (psddat) {
        iCmdFlg |= SDHCI_CMD_DATA;                                      /*  ��������                    */
    }

    /*
     * ����� CMD12 ������ CMD52 �е�дI/O Abort ����
     */
    if (psdcmd->SDCMD_uiOpcode == SD_STOP_TRANSMISSION) {
        iCmdFlg |= SDHCI_CMD_TYPE_ABORT;
    } else if (psdcmd->SDCMD_uiOpcode == SDIO_RW_DIRECT) {
        UINT32 uiArg = psdcmd->SDCMD_uiArg;
        if ((((uiArg >>  9) & 0x1ffff) == SDIO_CCCR_ABORT) &&           /*  CCCR abort reg addr         */
            (((uiArg >> 28) & 0x7)     == 0)               &&           /*  Function 0                  */
            (((uiArg >> 31) & 0x1)     != 0)) {                         /*  Write the Reg               */
            iCmdFlg |= SDHCI_CMD_TYPE_ABORT;
        }
    }

    SDHCI_WRITEW(psdhcihostattr, SDHCI_COMMAND, SDHCI_MAKE_CMD(psdcmd->SDCMD_uiOpcode, iCmdFlg));

    KN_IO_MB();
    return  (ERROR_NONE);

__timeout:
    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL,"timeout error.\r\n");

    psdhcitrans->SDHCITS_iCmdError = PX_ERROR;
    psdhcitrans->SDHCITS_iDatError = PX_ERROR;
    __sdhciTransFinish(psdhcitrans);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdhciTransferModSet
** ��������: ���ô���ģʽ
** ��    ��: psdhcihost      SDHCI HOST �ṹָ��
**           psddat          ���ݴ�����ƻ���
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciTransferModSet (__PSDHCI_HOST  psdhcihost)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    __PSDHCI_TRANS      psdhcitrans    = psdhcihost->SDHCIHS_psdhcitrans;
    UINT16              usTmod;

    if (!psdhcitrans->SDHCITS_pucDatBuffCurr) {
        return;
    }

    usTmod = SDHCI_TRNS_BLK_CNT_EN;                                     /*  ʹ�ܿ����                  */

    if (psdhcitrans->SDHCITS_uiBlkCntRemain > 1) {
        usTmod |= SDHCI_TRNS_MULTI;
        if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DONOT_USE_ACMD12) &&
            (psdhcitrans->SDHCITS_psdcmd->SDCMD_uiOpcode != SDIO_RW_EXTENDED)) {
            usTmod |= SDHCI_TRNS_ACMD12;                                /*  ʹ��ACMD12����              */
        }
    }

    if (psdhcitrans->SDHCITS_bIsRead) {
        usTmod |= SDHCI_TRNS_READ;
    }

    if (psdhcihost->SDHCIHS_ucTransferMod != SDHCIHOST_TMOD_SET_NORMAL) {
        usTmod |= SDHCI_TRNS_DMA;
    }

    SDHCI_WRITEW(psdhcihostattr, SDHCI_TRANSFER_MODE, usTmod);
}
/*********************************************************************************************************
** ��������: __sdhciDataPrepareNorm
** ��������: һ�����ݴ���׼��
** ��    ��: psdhcihost      SDHCI HOST �ṹָ��
**           psddat          ���ݴ�����ƻ���
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciDataPrepareNorm (__PSDHCI_HOST  psdhcihost)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    __PSDHCI_TRANS      psdhcitrans    = psdhcihost->SDHCIHS_psdhcitrans;

    if (!psdhcitrans->SDHCITS_pucDatBuffCurr) {
        return;
    }

    __sdhciTransferIntSet(psdhcihost);                                  /*  �����ж�ʹ��                */

    SDHCI_WRITEW(psdhcihostattr,
                 SDHCI_BLOCK_SIZE,
                 SDHCI_MAKE_BLKSZ(__SDHCI_DMA_BOUND_NBITS,
                                  psdhcitrans->SDHCITS_uiBlkSize));     /*  ���С�Ĵ���                */

    SDHCI_WRITEW(psdhcihostattr,
                 SDHCI_BLOCK_COUNT,
                 psdhcitrans->SDHCITS_uiBlkCntRemain);                  /*  ������Ĵ���                */
}
/*********************************************************************************************************
** ��������: __sdhciDataPrepareSdma
** ��������: DMA���ݴ���׼��
** ��    ��: psdhcihost      SDHCI HOST �ṹָ��
**           psdmsg          ���������Ϣ�ṹָ��
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciDataPrepareSdma (__PSDHCI_HOST  psdhcihost)
{
    __PSDHCI_TRANS psdhcitrans = psdhcihost->SDHCIHS_psdhcitrans;
    UINT8         *pucBuf      = psdhcitrans->SDHCITS_pucDatBuffCurr;

    if (!pucBuf) {
        return;
    }

#if LW_CFG_VMM_EN > 0
    pucBuf = psdhcitrans->SDHCITS_pucDmaBuffer;
    if (!psdhcitrans->SDHCITS_bIsRead) {
        lib_memcpy(psdhcitrans->SDHCITS_pucDmaBuffer,
                   psdhcitrans->SDHCITS_pucDatBuffCurr,
                   psdhcitrans->SDHCITS_uiBlkCntRemain * psdhcitrans->SDHCITS_uiBlkSize);
    }
#else
    pucBuf = psdhcitrans->SDHCITS_pucDatBuffCurr;
#endif

    __sdhciSdmaAddrUpdate(psdhcihost, (LONG)pucBuf);                    /*  д�� DMA ϵͳ��ַ�Ĵ���     */
                                                                        /*  ����ͺ�Ż����� DMA ���� */
    __sdhciDataPrepareNorm(psdhcihost);
    __sdhciDmaSelect(psdhcihost, SDHCI_HCTRL_SDMA);
}
/*********************************************************************************************************
** ��������: __sdhciDataPrepareAdma
** ��������: ��Ч DMA ���ݴ���׼��
** ��    ��: psdhcihost      SDHCI HOST �ṹָ��
**           psddat          ���ݴ�����ƻ���
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciDataPrepareAdma (__PSDHCI_HOST  psdhcihost)
{
}
/*********************************************************************************************************
** ��������: __sdhciDataReadNorm
** ��������: һ�����ݴ���(������)
** ��    ��: psdhcitrans     ������ƿ�
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciDataReadNorm (__PSDHCI_TRANS    psdhcitrans)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr;
    UINT32              uiBlkSize;
    UINT32              uiChunk;
    UINT32              uiData;
    UINT8              *pucBuffer;

    psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;
    uiBlkSize      = psdhcitrans->SDHCITS_uiBlkSize;
    pucBuffer      = psdhcitrans->SDHCITS_pucDatBuffCurr;
    uiChunk        = 0;
    uiData         = 0;

    if (((ULONG)pucBuffer) & 0x3) {
        while (uiBlkSize) {
            if (uiChunk == 0) {
                uiData = SDHCI_READL(psdhcihostattr, SDHCI_BUFFER);
                uiChunk = 4;
            }

            *pucBuffer = uiData & 0xFF;
            pucBuffer++;
            uiData >>= 8;
            uiChunk--;
            uiBlkSize--;
        }

    } else {
        UINT32 *puiBuffer = (UINT32 *)pucBuffer;

        while (uiBlkSize >= 4) {
            *puiBuffer++ = SDHCI_READL(psdhcihostattr, SDHCI_BUFFER);
            uiBlkSize   -= 4;
        }

        pucBuffer = (UINT8 *)puiBuffer;

        if (uiBlkSize > 0) {
            uiData = SDHCI_READL(psdhcihostattr, SDHCI_BUFFER);
            while (uiBlkSize-- > 0) {
                *pucBuffer++ = uiData & 0xFF;
                uiData >>= 8;
            }
        }
    }

    psdhcitrans->SDHCITS_uiBlkCntRemain -= 1;
    psdhcitrans->SDHCITS_pucDatBuffCurr  = pucBuffer;                   /*  ��¼��һ�����ݴ���λ��      */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciDataWriteNorm
** ��������: һ�����ݴ���(д����)
** ��    ��: psdhcitrans     ������ƿ�
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciDataWriteNorm (__PSDHCI_TRANS    psdhcitrans)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr;
    UINT32              uiBlkSize;
    UINT32              uiChunk;
    UINT32              uiData;
    UINT8              *pucBuffer;

    psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;
    uiBlkSize      = psdhcitrans->SDHCITS_uiBlkSize;
    pucBuffer      = psdhcitrans->SDHCITS_pucDatBuffCurr;
    uiChunk        = 0;
    uiData         = 0;

    /*
     * ���¿��Դ�����С���� 4 ���������
     */
    if (((ULONG)pucBuffer) & 0x3) {
        while (uiBlkSize) {
            uiData |= (UINT32)(*pucBuffer) << (uiChunk << 3);

            pucBuffer++;
            uiChunk++;
            uiBlkSize--;
            if ((uiChunk == 4) || (uiBlkSize == 0)) {
                SDHCI_WRITEL(psdhcihostattr, SDHCI_BUFFER, uiData);
                uiChunk = 0;
                uiData  = 0;
            }
        }

    } else {
        UINT32 *puiBuffer = (UINT32 *)pucBuffer;

        while (uiBlkSize >= 4) {
            SDHCI_WRITEL(psdhcihostattr, SDHCI_BUFFER, *puiBuffer++);
            uiBlkSize -= 4;
        }

        pucBuffer = (UINT8 *)puiBuffer;

        if (uiBlkSize > 0) {
            while (uiBlkSize-- > 0) {
                uiData <<= 8;
                uiData |= *pucBuffer++;
            }
            SDHCI_WRITEL(psdhcihostattr, SDHCI_BUFFER, uiData);
        }
    }

    psdhcitrans->SDHCITS_uiBlkCntRemain -= 1;
    psdhcitrans->SDHCITS_pucDatBuffCurr  = pucBuffer;                   /*  ��¼��һ�����ݴ���λ��      */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciTransferIntSet
** ��������: ���ݴ����ж�����
** ��    ��: psdhcihost       SDHCI HOST �ṹָ��
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciTransferIntSet (__PSDHCI_HOST  psdhcihost)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT32              uiIntIoMsk;                                     /*  ʹ��һ�㴫��ʱ���ж�����    */
    UINT32              uiIntDmaMsk;                                    /*  ʹ�� DMA ����ʱ���ж�����   */
    UINT32              uiEnMask;                                       /*  ����ʹ������                */

    uiIntIoMsk  = SDHCI_INT_SPACE_AVAIL | SDHCI_INT_DATA_AVAIL;
    uiIntDmaMsk = SDHCI_INT_DMA_END     | SDHCI_INT_ADMA_ERROR;
    uiEnMask    = SDHCI_READL(psdhcihostattr, SDHCI_INTSTA_ENABLE);     /*  ��ȡ32λ(���������ж�)      */

    if (psdhcihost->SDHCIHS_ucTransferMod != SDHCIHOST_TMOD_SET_NORMAL) {
        uiEnMask &= ~uiIntIoMsk;
        uiEnMask |=  uiIntDmaMsk;
    } else {
        uiEnMask &= ~uiIntDmaMsk;
        uiEnMask |=  uiIntIoMsk;
    }

    uiEnMask |= SDHCI_INT_RESPONSE | SDHCI_INT_DATA_END;

    /*
     * ��Ϊ�������Ĵ�����λ��λ�ö���ȫ��ͬ,
     * ���Կ���ͬʱ��һ������.
     */
    SDHCI_WRITEL(psdhcihostattr, SDHCI_INTSTA_ENABLE, uiEnMask);
    SDHCI_WRITEL(psdhcihostattr, SDHCI_SIGNAL_ENABLE, uiEnMask);
}
/*********************************************************************************************************
** ��������: __sdhciIntDisAndEn
** ��������: �ж�����(ʹ��\����).�����ж�״̬(����\һ��״̬)���ж��ź�(����\һ���ź�)�Ĵ���.
** ��    ��: psdhcihost      SDHCI HOST �ṹָ��
**           uiDisMask       ��������
**           uiEnMask        ʹ������
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciIntDisAndEn (__PSDHCI_HOST  psdhcihost,
                                UINT32         uiDisMask,
                                UINT32         uiEnMask)
{
    PLW_SDHCI_HOST_ATTR psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT32              uiMask;

    uiMask  =  SDHCI_READL(psdhcihostattr, SDHCI_INTSTA_ENABLE);
    uiMask &= ~uiDisMask;
    uiMask |=  uiEnMask;

    /*
     * ��Ϊ�������Ĵ�����λ��λ�ö���ȫ��ͬ,
     * ���Կ���ͬʱ��һ������.
     */
    SDHCI_WRITEL(psdhcihostattr, SDHCI_INTSTA_ENABLE, uiMask);
    SDHCI_WRITEL(psdhcihostattr, SDHCI_SIGNAL_ENABLE, uiMask);
}
/*********************************************************************************************************
** ��������: __sdhciSdioIntEn
** ��������: ʹ�� SDIO �ж�
** ��    ��: psdhcihost      SDHCI HOST �ṹָ��
**           bEnable         �Ƿ�ʹ��
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciSdioIntEn (__PSDHCI_HOST psdhcihost, BOOL bEnable)
{
    PLW_SDHCI_HOST_ATTR  psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    UINT32               uiMask;

    if (psdhcihost->SDHCIHS_bSdioIntEnable == bEnable) {
        return;
    }

    uiMask = SDHCI_READL(psdhcihostattr, SDHCI_INTSTA_ENABLE);
    if (bEnable) {
        uiMask |= SDHCI_INT_CARD_INT;
    } else {
        uiMask &= ~SDHCI_INT_CARD_INT;
    }

    SDHCI_WRITEL(psdhcihostattr, SDHCI_INTSTA_ENABLE, uiMask);
    SDHCI_WRITEL(psdhcihostattr, SDHCI_SIGNAL_ENABLE, uiMask);
    psdhcihost->SDHCIHS_bSdioIntEnable = bEnable;
}
/*********************************************************************************************************
** ��������: __sdhciTransNew
** ��������: �·��䲢��ʼ��һ�������������
** ��    ��: psdhcihost   ������������
** ��    ��: �����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static __SDHCI_TRANS *__sdhciTransNew (__PSDHCI_HOST   psdhcihost)
{
    __SDHCI_TRANS    *psdhcitrans;
    LW_OBJECT_HANDLE  hSync;
    INT               iError;

#if LW_CFG_VMM_EN > 0
    VOID             *pvDmaBuf;
#endif

    if (!psdhcihost) {
        return  (LW_NULL);
    }

    psdhcitrans = (__SDHCI_TRANS *)__SHEAP_ALLOC(sizeof(__SDHCI_TRANS));
    if (!psdhcitrans) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        return  (LW_NULL);
    }
    lib_bzero(psdhcitrans, sizeof(__SDHCI_TRANS));

#if LW_CFG_VMM_EN > 0
    pvDmaBuf = API_VmmDmaAllocAlign(__SDHCI_DMA_BOUND_LEN, 4);
    if (!pvDmaBuf) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        goto    __err0;
    }
    psdhcitrans->SDHCITS_pucDmaBuffer = (UINT8 *)pvDmaBuf;
#endif

    hSync = API_SemaphoreBCreate("sdhcits_sync", LW_FALSE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (hSync == LW_OBJECT_HANDLE_INVALID) {
        goto    __err1;
    }

    LW_SPIN_INIT(&psdhcitrans->SDHCITS_slLock);

#if LW_CFG_VMM_EN > 0
    psdhcitrans->SDHCITS_pucDmaBuffer = pvDmaBuf;
#endif
    psdhcitrans->SDHCITS_hFinishSync  = hSync;
    psdhcitrans->SDHCITS_psdhcihost   = psdhcihost;
    psdhcihost->SDHCIHS_psdhcitrans   = psdhcitrans;

    iError = __sdhciTransTaskInit(psdhcitrans);
    if (iError != ERROR_NONE) {
        goto    __err2;
    }

    API_InterVectorConnect(psdhcihost->SDHCIHS_sdhcihostattr.SDHCIHOST_ulIntVector,
                           __sdhciTransIrq,
                           (VOID *)psdhcitrans,
                           "sdhci_isr");
    API_InterVectorEnable(psdhcihost->SDHCIHS_sdhcihostattr.SDHCIHOST_ulIntVector);

    return  (psdhcitrans);

__err2:
    API_SemaphoreBDelete(&hSync);

__err1:
#if LW_CFG_VMM_EN > 0
   API_VmmDmaFree(pvDmaBuf);

__err0:
#endif
    __SHEAP_FREE(psdhcitrans);

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __sdhciTransTaskInit
** ��������: ��ʼ�����������߳����
** ��    ��: psdhcitrans  ����������ƿ�
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciTransTaskInit (__SDHCI_TRANS *psdhcitrans)
{
    LW_OBJECT_HANDLE     hSdioIntSem;
    LW_OBJECT_HANDLE     hSdioIntThread;
    LW_CLASS_THREADATTR  threadattr;
    LW_SDHCI_HOST_ATTR  *psdhcihostattr;

    psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;
    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_SDIO_INT_OOB)) {
        return  (ERROR_NONE);
    }

    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_CANNOT_SDIO_INT)) {
        hSdioIntSem = API_SemaphoreBCreate("sdhcisdio_sem",
                                           LW_FALSE,
                                           LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);
    } else {
        hSdioIntSem = API_SemaphoreCCreate("sdhcisdio_sem",
                                           0,
                                           4096,
                                           LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);
    }

    if (hSdioIntSem == LW_OBJECT_HANDLE_INVALID) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "create semaphore failed.\r\n");
        return  (PX_ERROR);
    }
    psdhcitrans->SDHCITS_hSdioIntSem = hSdioIntSem;

    threadattr = API_ThreadAttrGetDefault();
    threadattr.THREADATTR_pvArg            = (PVOID)psdhcitrans;
    threadattr.THREADATTR_ucPriority       = __SDHCI_SDIOINTSVR_PRIO;
    threadattr.THREADATTR_stStackByteSize  = __SDHCI_SDIOINTSVR_STKSZ;
    threadattr.THREADATTR_ulOption        |= LW_OPTION_OBJECT_GLOBAL;
    hSdioIntThread = API_ThreadCreate("t_sdhcisdio",
                                      __sdhciSdioIntSvr,
                                      &threadattr,
                                      LW_NULL);
    if (hSdioIntThread == LW_OBJECT_HANDLE_INVALID) {
        goto    __err;
    }
    psdhcitrans->SDHCITS_hSdioIntThread = hSdioIntThread;

    return  (ERROR_NONE);

__err:
    API_SemaphoreDelete(&hSdioIntSem);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdhciTransTaskDeInit
** ��������: �����������߳����
** ��    ��: psdhcitrans  ����������ƿ�
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciTransTaskDeInit (__SDHCI_TRANS *psdhcitrans)
{
    LW_SDHCI_HOST_ATTR  *psdhcihostattr;

    psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;

    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_SDIO_INT_OOB)) {
        return  (ERROR_NONE);
    }

    API_ThreadDelete(&psdhcitrans->SDHCITS_hSdioIntThread, LW_NULL);
    API_SemaphoreDelete(&psdhcitrans->SDHCITS_hSdioIntSem);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciTransDel
** ��������: �����ͷŴ����������ռ�
** ��    ��: psdhcitrans
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciTransDel (__SDHCI_TRANS *psdhcitrans)
{
    ULONG  ulVector;

    if (!psdhcitrans) {
        return;
    }

    ulVector = psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr.SDHCIHOST_ulIntVector;

    API_InterVectorDisableEx(ulVector, 1);
    API_InterVectorDisconnect(ulVector, __sdhciTransIrq, (VOID *)psdhcitrans);

    __sdhciTransTaskDeInit(psdhcitrans);

#if LW_CFG_VMM_EN > 0
    API_VmmDmaFree(psdhcitrans->SDHCITS_pucDmaBuffer);
#endif

    API_SemaphoreBDelete(&psdhcitrans->SDHCITS_hFinishSync);
    __SHEAP_FREE(psdhcitrans);
}
/*********************************************************************************************************
** ��������: __sdhciTransPrepare
** ��������: ׼����������
** ��    ��: psdhcitrans      �����������
**           psdmsg           �û�����Ĵ�����Ϣ
**           iTransType       ��������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciTransPrepare (__SDHCI_TRANS *psdhcitrans,
                                 LW_SD_MESSAGE *psdmsg,
                                 INT            iTransType)
{
    LW_SD_DATA  *psddata;
    UINT32       uiMaxBlkSize;

    if (!psdhcitrans || !psdmsg || !psdmsg->SDMSG_psdcmdCmd) {
        return  (PX_ERROR);
    }

    uiMaxBlkSize = psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcicap.SDHCICAP_uiMaxBlkSize;
    psddata      = psdmsg->SDMSG_psddata;
    if (psddata) {
        if ((psddata->SDDAT_uiBlkSize < 1) || (psddata->SDDAT_uiBlkSize > uiMaxBlkSize)) {
            SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL,
                              "blk size(%u) is out of range(1~%u)\r\n",
                              psddata->SDDAT_uiBlkSize, uiMaxBlkSize);
            return  (PX_ERROR);
        }
        if ((psddata->SDDAT_uiBlkNum < 1) || (psddata->SDDAT_uiBlkNum > 65535)) {
            SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL,
                              "blk num(%u) is out of range(1~65535)\r\n",
                              psddata->SDDAT_uiBlkNum);
            return  (PX_ERROR);
        }
        if ((psddata->SDDAT_uiBlkSize * psddata->SDDAT_uiBlkNum) > (512 * 1024)) {
            SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL,
                              "transfer data length(%u) is out of range(512KB)\r\n",
                              psddata->SDDAT_uiBlkSize * psddata->SDDAT_uiBlkNum);
            return  (PX_ERROR);
        }

        if (SD_DAT_IS_READ(psddata)) {
            psdhcitrans->SDHCITS_pucDatBuffCurr = psdmsg->SDMSG_pucRdBuffer;
            psdhcitrans->SDHCITS_bIsRead        = LW_TRUE;
        } else {
            psdhcitrans->SDHCITS_pucDatBuffCurr = psdmsg->SDMSG_pucWrtBuffer;
            psdhcitrans->SDHCITS_bIsRead        = LW_FALSE;
        }

        psdhcitrans->SDHCITS_uiBlkSize      = psddata->SDDAT_uiBlkSize;
        psdhcitrans->SDHCITS_uiBlkCntRemain = psddata->SDDAT_uiBlkNum;

    } else {
        psdhcitrans->SDHCITS_pucDatBuffCurr = LW_NULL;
        psdhcitrans->SDHCITS_uiBlkSize      = 0;
        psdhcitrans->SDHCITS_uiBlkCntRemain = 0;
    }

    psdhcitrans->SDHCITS_iTransType  = iTransType;
    psdhcitrans->SDHCITS_bCmdFinish  = LW_FALSE;
    psdhcitrans->SDHCITS_bDatFinish  = LW_FALSE;
    psdhcitrans->SDHCITS_iCmdError   = ERROR_NONE;
    psdhcitrans->SDHCITS_iDatError   = ERROR_NONE;

    psdhcitrans->SDHCITS_psdcmd      = psdmsg->SDMSG_psdcmdCmd;
    psdhcitrans->SDHCITS_psdcmdStop  = psdmsg->SDMSG_psdcmdStop;
    psdhcitrans->SDHCITS_psddat      = psdmsg->SDMSG_psddata;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciTransStart
** ��������: ����һ�δ�������
** ��    ��: psdhcitrans      �����������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciTransStart (__SDHCI_TRANS *psdhcitrans)
{
    INT     iRet;

    if (!psdhcitrans) {
        return  (PX_ERROR);
    }

    if (!SDHCI_QUIRK_FLG(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                         SDHCI_QUIRK_FLG_DONOT_RESET_ON_EVERY_TRANSACTION)) {
        __sdhciHostReset(psdhcitrans->SDHCITS_psdhcihost, SDHCI_SFRST_DATA | SDHCI_SFRST_CMD);
    }


    if (SDHCI_QUIRK_FLG(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                         SDHCI_QUIRK_FLG_REENABLE_INTS_ON_EVERY_TRANSACTION)) {
        __sdhciIntClear(psdhcitrans->SDHCITS_psdhcihost);
        __sdhciIntDisAndEn(psdhcitrans->SDHCITS_psdhcihost, SDHCI_INT_ALL_MASK, __SDHCI_INT_EN_MASK);
    }

    if (psdhcitrans->SDHCITS_pucDatBuffCurr) {
        if (psdhcitrans->SDHCITS_iTransType == __SDHIC_TRANS_NORMAL) {
            __sdhciDataPrepareNorm(psdhcitrans->SDHCITS_psdhcihost);
        } else if (psdhcitrans->SDHCITS_iTransType == __SDHIC_TRANS_SDMA) {
            __sdhciDataPrepareSdma(psdhcitrans->SDHCITS_psdhcihost);
        } else {
            __sdhciDataPrepareAdma(psdhcitrans->SDHCITS_psdhcihost);
        }
    }

    psdhcitrans->SDHCITS_iStage = __SDHCI_TRANS_STAGE_START;
    iRet = __sdhciCmdSend(psdhcitrans->SDHCITS_psdhcihost,
                          psdhcitrans->SDHCITS_psdcmd,
                          psdhcitrans->SDHCITS_psddat);

    if (iRet != ERROR_NONE) {
        API_SemaphoreBClear(psdhcitrans->SDHCITS_hFinishSync);
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: __sdhciTransFinishWait
** ��������: �ȴ����δ����������
** ��    ��: psdhcitrans      �����������
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciTransFinishWait (__SDHCI_TRANS *psdhcitrans)
{
    INT iRet;
    iRet = API_SemaphoreBPend(psdhcitrans->SDHCITS_hFinishSync, LW_OPTION_WAIT_INFINITE);
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __sdhciTransFinish
** ��������: ��ɴ�������
** ��    ��: psdhcitrans      �����������
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdhciTransFinish (__SDHCI_TRANS *psdhcitrans)
{
    API_SemaphoreBPost(psdhcitrans->SDHCITS_hFinishSync);
}
/*********************************************************************************************************
** ��������: __sdhciTransClean
** ��������: �����δ�������
** ��    ��: psdhcitrans      �����������
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciTransClean (__SDHCI_TRANS *psdhcitrans)
{
    if (!psdhcitrans) {
        return  (PX_ERROR);
    }

    psdhcitrans->SDHCITS_psdcmd     = LW_NULL;
    psdhcitrans->SDHCITS_psdcmdStop = LW_NULL;
    psdhcitrans->SDHCITS_psddat     = LW_NULL;

    if (SDHCI_QUIRK_FLG(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                         SDHCI_QUIRK_FLG_REENABLE_INTS_ON_EVERY_TRANSACTION)) {
        __sdhciIntDisAndEn(psdhcitrans->SDHCITS_psdhcihost, SDHCI_INT_ALL_MASK, 0);
        __sdhciIntClear(psdhcitrans->SDHCITS_psdhcihost);
    }

    API_SemaphoreBClear(psdhcitrans->SDHCITS_hFinishSync);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciTransIrq
** ��������: SDHCI �жϷ������
** ��    ��: pvArg      ������ƿ�
**           ulVector   �ж�����
** ��    ��: �жϴ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t   __sdhciTransIrq (VOID *pvArg, ULONG ulVector)
{
    __SDHCI_TRANS       *psdhcitrans    = (__SDHCI_TRANS *)pvArg;
    __SDHCI_HOST        *psdhcihost     = psdhcitrans->SDHCITS_psdhcihost;
    LW_SDHCI_HOST_ATTR  *psdhcihostattr = &psdhcihost->SDHCIHS_sdhcihostattr;
    BOOL                 bSdioInt       = LW_FALSE;

    UINT32               uiIntSta;
    irqreturn_t          irqret;

    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncIsrEnterHook) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncIsrEnterHook(psdhcihostattr);
    }

    uiIntSta = SDHCI_READL(psdhcihostattr, SDHCI_INT_STATUS);

__redo:
    if (!uiIntSta || uiIntSta == 0xffffffff) {
        SDHCI_WRITEL(psdhcihostattr, SDHCI_INT_STATUS, uiIntSta);
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unknown int signals\r\n");
        irqret = LW_IRQ_NONE;
        goto    __end;                                                  /*  ��Ч��δ֪���ж�            */
    }

    SDHCI_WRITEL(psdhcihostattr, SDHCI_INT_STATUS, uiIntSta);

    __SDHCI_TRANS_LOCK(psdhcitrans);

    psdhcitrans->SDHCITS_uiIntSta = uiIntSta;

    if (uiIntSta & SDHCI_INT_CMD_MASK) {
        SDHCI_WRITEL(psdhcihostattr, SDHCI_INT_STATUS, uiIntSta & SDHCI_INT_CMD_MASK);
        __sdhciTransCmdHandle(psdhcitrans, uiIntSta & SDHCI_INT_CMD_MASK);
    }

    if (uiIntSta & SDHCI_INT_DATA_MASK) {
        SDHCI_WRITEL(psdhcihostattr, SDHCI_INT_STATUS, uiIntSta & SDHCI_INT_DATA_MASK);
        __sdhciTransDatHandle(psdhcitrans, uiIntSta & SDHCI_INT_DATA_MASK);
    }

    /*
     * �����Է���, ĳЩ�����, �ڱ������ݴ�����ɺ�, HOST �����ܲ��� SDIO �ж�.
     * ����ܺ;���� SDIO Ӧ��������ʵ���й�. ������Ϊ��ȷ���������ԭ�����
     * ������쳣�ж�, �������⴦��.
     */
    if ((psdhcitrans->SDHCITS_pucDatBuffCurr)  &&
        (!psdhcitrans->SDHCITS_uiBlkCntRemain) &&
        (uiIntSta & SDHCI_INT_DATA_END)        &&
        psdhcihost->SDHCIHS_bSdioIntEnable) {
        bSdioInt = LW_TRUE;
    }

    uiIntSta &= ~(SDHCI_INT_CMD_MASK | SDHCI_INT_DATA_MASK);
    uiIntSta &= ~(SDHCI_INT_ERROR);

    if (uiIntSta & SDHCI_INT_BUS_POWER) {
        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL,
                          "sdhci(%s): card consumed too much power!\r\n",
                          __SDHCI_HOST_NAME(psdhcitrans->SDHCITS_psdhcihost));
        SDHCI_WRITEL(psdhcihostattr, SDHCI_INT_STATUS, SDHCI_INT_BUS_POWER);
        uiIntSta &= ~SDHCI_INT_BUS_POWER;
    }

    if (uiIntSta & SDHCI_INT_CARD_INT) {
        bSdioInt = LW_TRUE;
        uiIntSta &= ~SDHCI_INT_CARD_INT;
    }

    if (uiIntSta) {
        SDHCI_WRITEL(psdhcihostattr, SDHCI_INT_STATUS, uiIntSta);
    }

    irqret = LW_IRQ_HANDLED;

    KN_IO_MB();

    if (!SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_CANNOT_SDIO_INT) &&
        !SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_SDIO_INT_OOB)    &&
        bSdioInt) {
        __sdhciSdioIntEn(psdhcitrans->SDHCITS_psdhcihost, LW_FALSE);
        __SDHCI_SDIO_NOTIFY(psdhcitrans);
    }

    /*
     * �еĿ������ڴ����굱ǰ���жϺ�, ���ܻ��ٴβ����µ��ж�״̬,
     * ��״̬��һ������֮����Ӳ���жϵķ�ʽ֪ͨ CPU,
     * ���������Ҫ�ٴβ�ѯֱ���������.
     */
    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_RECHECK_INTS_AFTER_ISR)) {
        uiIntSta = SDHCI_READL(psdhcihostattr, SDHCI_INT_STATUS);
        if (uiIntSta) {
            __SDHCI_TRANS_UNLOCK(psdhcitrans);
            bSdioInt = LW_FALSE;
            goto __redo;
        }
    }

    __SDHCI_TRANS_UNLOCK(psdhcitrans);

__end:
    if (psdhcihostattr->SDHCIHOST_pquirkop &&
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncIsrExitHook) {
        psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncIsrExitHook(psdhcihostattr);
    }

    return  (irqret);
}
/*********************************************************************************************************
** ��������: __sdhciSdioIntSvr
** ��������: SDHCI SDIO �жϷ����߳�
** ��    ��: pvArg      ������ƿ�
** ��    ��: �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  __sdhciSdioIntSvr (VOID *pvArg)
{
    __SDHCI_TRANS       *psdhcitrans     = (__SDHCI_TRANS *)pvArg;
    __SDHCI_HOST        *psdhcihost      = psdhcitrans->SDHCITS_psdhcihost;
    LW_SDHCI_HOST_ATTR  *psdhcihostattr  = &psdhcihost->SDHCIHS_sdhcihostattr;
    INT                  iError          = ERROR_NONE;

    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_CANNOT_SDIO_INT)) {
        INT     iTickDly  = __SDHCI_SDIO_DLY_MAX;
        INT     iDoIntCnt = 1;
        INT     i;

        while (1) {
            __SDHCI_SDIO_REQUEST(psdhcitrans);
            __SDHCI_SDIO_RELEASE(psdhcitrans);                          /*  �����ͷ��������������ֹ�ж�*/

            for (i = 0; i < iDoIntCnt; i++) {
                iError = API_SdmEventNotify(psdhcihost->SDHCIHS_psdhcisdmhost->SDHCISDMH_pvSdmHost,
                                            SDM_EVENT_SDIO_INTERRUPT);
                if (iError != ERROR_NONE) {
                    /*
                     * �������ȷʵû�� SDIO �ж�, ���ﲻ������ֹ��ѯ
                     * ��Ϊ�п�������һ�λ����, ����������Ӱ�촫���ٶ�
                     */
                }
            }

            /*
             * ��̬������ѯ���
             */
            if (iError != ERROR_NONE) {
                /*
                 * �������Ͳ�ѯƵ��
                 */
                if (iTickDly < __SDHCI_SDIO_DLY_MAX) {
                    iTickDly++;
                }
                if (iDoIntCnt > 1) {
                    if (iTickDly >= __SDHCI_SDIO_DLY_MAX) {
                        iDoIntCnt--;
                    }
                }

            } else {
                /*
                 * ����������ѯƵ�ʵ����
                 */
                iTickDly  = 1;
                iDoIntCnt = __SDHCI_SDIO_INT_MAX;
            }

            API_TimeSleep(iTickDly);
        }
    } else {
        while (1) {
            __SDHCI_SDIO_WAIT(psdhcitrans);
            API_SdmEventNotify(psdhcihost->SDHCIHS_psdhcisdmhost->SDHCISDMH_pvSdmHost,
                               SDM_EVENT_SDIO_INTERRUPT);
            __sdhciSdioIntEn(psdhcihost, LW_TRUE);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __sdhciTransCmdHandle
** ��������: ����������ص�����
** ��    ��: psdhcitrans      �����������
**           uiIntSta         ������ص��ж�״̬
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciTransCmdHandle (__SDHCI_TRANS *psdhcitrans, UINT32 uiIntSta)
{
    if (!psdhcitrans->SDHCITS_psdcmd) {
        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "unexpected cmd interrupt: %08x.\r\n", uiIntSta);

        do {
            SDHCI_WRITEL(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                         SDHCI_INT_STATUS, uiIntSta);
            uiIntSta = SDHCI_READL(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                                   SDHCI_INT_STATUS);
        } while (uiIntSta);

        return  (PX_ERROR);
    }

    if (uiIntSta &
        (SDHCI_INT_TIMEOUT |
         SDHCI_INT_CRC     |
         SDHCI_INT_END_BIT |
         SDHCI_INT_INDEX   |
         SDHCI_INT_ACMD12ERR)) {

        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "cmd error interrupt: %08x.\r\n", uiIntSta);

        if (SDHCI_QUIRK_FLG(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                            SDHCI_QUIRK_FLG_DO_RESET_ON_TRANSACTION_ERROR)) {
            if (uiIntSta & (SDHCI_INT_TIMEOUT | SDHCI_INT_CRC)) {
                if (!psdhcitrans->SDHCITS_bCmdFinish) {
                    __sdhciHostReset(psdhcitrans->SDHCITS_psdhcihost, SDHCI_SFRST_CMD);
                }

                if ((!psdhcitrans->SDHCITS_bDatFinish) ||
                    (SD_CMD_TEST_RSP(psdhcitrans->SDHCITS_psdcmd, SD_RSP_BUSY))) {
                    __sdhciHostReset(psdhcitrans->SDHCITS_psdhcihost, SDHCI_SFRST_DATA);
                }
            }
        }

        psdhcitrans->SDHCITS_iCmdError = PX_ERROR;
        __sdhciTransFinish(psdhcitrans);                                /*  �������δ���                */
        return  (PX_ERROR);
    }

    if (uiIntSta & SDHCI_INT_RESPONSE) {
        __sdhciTransCmdFinish(psdhcitrans);                             /*  ������ɴ���                */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciTransDatHandle
** ��������: ����������ص�����
** ��    ��: psdhcitrans      �����������
**           uiIntSta         ������ص��ж�״̬
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdhciTransDatHandle (__SDHCI_TRANS *psdhcitrans, UINT32 uiIntSta)
{
    LW_SDHCI_HOST_ATTR  *psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;
    SDHCI_QUIRK_OP      *psdhciquirkop  = psdhcihostattr->SDHCIHOST_pquirkop;

    /*
     * ��û�����ݴ���ʱ, ������������ æ�ȴ��ź�, ��Ҳ�������������ж�
     * ��� SDHCI �淶 2.2.17(page64)
     */
    if (!psdhcitrans->SDHCITS_pucDatBuffCurr) {
        if (SD_CMD_TEST_RSP(psdhcitrans->SDHCITS_psdcmd, SD_RSP_BUSY)) {
            if (uiIntSta & SDHCI_INT_DATA_TIMEOUT) {
                psdhcitrans->SDHCITS_iDatError = PX_ERROR;
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "timeout on busy irq.\r\n");
                __sdhciTransFinish(psdhcitrans);
                return  (PX_ERROR);
            }

            if (uiIntSta & SDHCI_INT_DATA_END) {
                psdhcitrans->SDHCITS_iDatError  = ERROR_NONE;
                psdhcitrans->SDHCITS_bDatFinish = LW_TRUE;

                /*
                 * �������δ���,��������жϲ������������
                 */
                if (psdhcitrans->SDHCITS_bCmdFinish) {
                    __sdhciTransFinish(psdhcitrans);
                }

                return  (ERROR_NONE);
            }
        }
    }

    if (uiIntSta                &
        (SDHCI_INT_DATA_TIMEOUT |
         SDHCI_INT_DATA_CRC     |
         SDHCI_INT_DATA_END_BIT |
         SDHCI_INT_ADMA_ERROR)) {
        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "data error interrupt: %08x.\r\n", uiIntSta);

        if (SDHCI_QUIRK_FLG(&psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr,
                            SDHCI_QUIRK_FLG_DO_RESET_ON_TRANSACTION_ERROR)) {
            if (uiIntSta & (SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_DATA_CRC)) {
                if ((!psdhcitrans->SDHCITS_bDatFinish) ||
                    (SD_CMD_TEST_RSP(psdhcitrans->SDHCITS_psdcmd, SD_RSP_BUSY))) {
                    __sdhciHostReset(psdhcitrans->SDHCITS_psdhcihost, SDHCI_SFRST_DATA);
                }
            }
        }

        psdhcitrans->SDHCITS_iDatError = PX_ERROR;
        psdhcitrans->SDHCITS_iCmdError = PX_ERROR;
        __sdhciTransDatFinish(psdhcitrans);
        return  (PX_ERROR);
    }

    /*
     * ���ݿ� ��/д �ж�
     */
    if ((uiIntSta & (SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL)) &&
        (psdhcitrans->SDHCITS_uiBlkCntRemain > 0)) {
        if (psdhcitrans->SDHCITS_bIsRead) {
            while (SDHCI_READL(psdhcihostattr, SDHCI_PRESENT_STATE) & SDHCI_PSTA_DATA_AVAILABLE) {
            	if (psdhciquirkop && psdhciquirkop->SDHCIQOP_pfuncPioXferHook) {
            		psdhciquirkop->SDHCIQOP_pfuncPioXferHook(psdhcihostattr, LW_TRUE);
            	}
                __sdhciDataReadNorm(psdhcitrans);
                if (psdhcitrans->SDHCITS_uiBlkCntRemain == 0) {
                    /*
                     * ������, �����Ѿ���������. �������ﲻֱ�ӷ���
                     * ��Ϊһ�������һ�� ������� �ж�, ��֮����
                     */
                    break;
                }
            }
        } else {
            while (SDHCI_READL(psdhcihostattr, SDHCI_PRESENT_STATE) & SDHCI_PSTA_SPACE_AVAILABLE) {
            	if (psdhciquirkop && psdhciquirkop->SDHCIQOP_pfuncPioXferHook) {
            		psdhciquirkop->SDHCIQOP_pfuncPioXferHook(psdhcihostattr, LW_FALSE);
            	}
                __sdhciDataWriteNorm(psdhcitrans);
                if (psdhcitrans->SDHCITS_uiBlkCntRemain == 0) {
                    /*
                     * ������, �����Ѿ���������. �������ﲻֱ�ӷ���
                     * ��Ϊһ�������һ�� ������� �ж�, ��֮����
                     */
                    break;
                }
            }
        }
    }

    /*
     * ��ǰδ���� DMA �߽��жϵ����. ����
     * ���ȷʵ������, �������� DMA ���伴��
     */
    if (uiIntSta & SDHCI_INT_DMA_END) {
        UINT32  uiDmaAddr;
        uiDmaAddr = SDHCI_READL(psdhcihostattr, SDHCI_SYS_SDMA);
        SDHCI_WRITEL(psdhcihostattr, SDHCI_SYS_SDMA, uiDmaAddr);
    }

    if (uiIntSta & SDHCI_INT_DATA_END) {
        psdhcitrans->SDHCITS_bDatFinish = LW_TRUE;
        if (!psdhcitrans->SDHCITS_bCmdFinish) {
            /*
             * �����жϺ������жϵ�˳��ȷ��
             * ��������жϻ�δ����, �������������ݴ������
             * �ȴ��Ժ�������жϽ��к�������
             */
            psdhcitrans->SDHCITS_bDatFinish = LW_TRUE;
        } else {
            __sdhciTransDatFinish(psdhcitrans);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciTransCmdFinish
** ��������: ������ȷ��ɺ�Ĵ���
** ��    ��: psdhcitrans      �����������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciTransCmdFinish (__SDHCI_TRANS *psdhcitrans)
{
    LW_SDHCI_HOST_ATTR  *psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;
    LW_SD_COMMAND       *psdcmd         = psdhcitrans->SDHCITS_psdcmd;
    UINT32               uiRespFlag;

    if (psdhcitrans->SDHCITS_iStage == __SDHCI_TRANS_STAGE_START) {
        psdcmd = psdhcitrans->SDHCITS_psdcmd;
    } else {
        psdcmd = psdhcitrans->SDHCITS_psdcmdStop;
    }

    uiRespFlag = SD_RESP_TYPE(psdcmd);
    if (uiRespFlag & SD_RSP_PRESENT) {
        UINT32  *puiResp = psdcmd->SDCMD_uiResp;

        if (psdhcihostattr->SDHCIHOST_pquirkop &&
            psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncResponseGet) {
            psdhcihostattr->SDHCIHOST_pquirkop->SDHCIQOP_pfuncResponseGet(psdhcihostattr,
                                                                          uiRespFlag,
                                                                          puiResp);
            goto __ret;
        }

        /*
         * ��Ӧ��,Ӧ��Ĵ�����ȥ���� CRC ��һ���ֽ�,�������λ����.
         * ��Ӧ���ֽ������ϲ�涨���෴,����Ӧ��ת��.
         */
        if (uiRespFlag & SD_RSP_136) {
            UINT32   uiRsp0;
            UINT32   uiRsp1;
            UINT32   uiRsp2;
            UINT32   uiRsp3;

            uiRsp3 = SDHCI_READL(psdhcihostattr, SDHCI_RESPONSE3);
            uiRsp2 = SDHCI_READL(psdhcihostattr, SDHCI_RESPONSE2);
            uiRsp1 = SDHCI_READL(psdhcihostattr, SDHCI_RESPONSE1);
            uiRsp0 = SDHCI_READL(psdhcihostattr, SDHCI_RESPONSE0);

            puiResp[0] = (uiRsp3 << 8) | ( uiRsp2 >> 24);
            puiResp[1] = (uiRsp2 << 8) | ( uiRsp1 >> 24);
            puiResp[2] = (uiRsp1 << 8) | ( uiRsp0 >> 24);
            puiResp[3] = (uiRsp0 << 8);
        } else {
            puiResp[0] = SDHCI_READL(psdhcihostattr, SDHCI_RESPONSE0);
        }
    }

__ret:
    psdhcitrans->SDHCITS_iCmdError  = ERROR_NONE;
    psdhcitrans->SDHCITS_bCmdFinish = LW_TRUE;

    if (psdhcitrans->SDHCITS_iStage == __SDHCI_TRANS_STAGE_STOP) {
        __sdhciTransFinish(psdhcitrans);                                /*  �����������                */
        return  (ERROR_NONE);
    }

    if (psdhcitrans->SDHCITS_pucDatBuffCurr &&
        psdhcitrans->SDHCITS_bDatFinish) {
        __sdhciTransDatFinish(psdhcitrans);                             /*  �����������                */
    
	} else if (!psdhcitrans->SDHCITS_pucDatBuffCurr) {
        /*
         * ��æ�ȴ�������������������ж�
         * �����������Ҫ�ȴ��жϲ����ٽ�������
         */
        if(SD_CMD_TEST_RSP(psdhcitrans->SDHCITS_psdcmd, SD_RSP_BUSY) &&
           !psdhcitrans->SDHCITS_bDatFinish                          &&
           SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_HAS_DATEND_IRQ_WHEN_NOT_BUSY)) {
            return  (ERROR_NONE);
        }

        __sdhciTransFinish(psdhcitrans);                            /*  �����������                */

    } else {
        /*
         * �����ʾ���ݻ�û�������
         * ���ں���������ж��ﴦ��
         */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciTransDatFinish
** ��������: ������ȷ��ɺ�Ĵ���
** ��    ��: psdhcitrans      �����������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciTransDatFinish (__SDHCI_TRANS *psdhcitrans)
{
    LW_SDHCI_HOST_ATTR  *psdhcihostattr = &psdhcitrans->SDHCITS_psdhcihost->SDHCIHS_sdhcihostattr;

#if LW_CFG_VMM_EN > 0
    if (psdhcitrans->SDHCITS_iDatError == ERROR_NONE) {
        if (psdhcitrans->SDHCITS_bIsRead &&
            (psdhcitrans->SDHCITS_iTransType != __SDHIC_TRANS_NORMAL)) {

            lib_memcpy(psdhcitrans->SDHCITS_pucDatBuffCurr,
                       psdhcitrans->SDHCITS_pucDmaBuffer,
                       psdhcitrans->SDHCITS_uiBlkSize * psdhcitrans->SDHCITS_uiBlkCntRemain);
        }
    }
#endif

    if (SDHCI_QUIRK_FLG(psdhcihostattr, SDHCI_QUIRK_FLG_DONOT_USE_ACMD12)) {
        if (psdhcitrans->SDHCITS_psdcmdStop) {
            psdhcitrans->SDHCITS_iStage = __SDHCI_TRANS_STAGE_STOP;
            __sdhciCmdSend(psdhcitrans->SDHCITS_psdhcihost, psdhcitrans->SDHCITS_psdcmdStop, LW_NULL);
            return  (ERROR_NONE);
        }
    }

    __sdhciTransFinish(psdhcitrans);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciRegAccessDrvInit
** ��������: �Ĵ����ռ����������ʼ��
** ��    ��: psdhcihostattr ������������������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdhciRegAccessDrvInit (PLW_SDHCI_HOST_ATTR  psdhcihostattr)
{
    static BOOL bInit = LW_FALSE;
    INT         iType;

    if (!psdhcihostattr) {
        return  (PX_ERROR);
    }

    iType = psdhcihostattr->SDHCIHOST_iRegAccessType;
    if ((iType != SDHCI_REGACCESS_TYPE_IO)  &&
        (iType != SDHCI_REGACCESS_TYPE_MEM) &&
        !(psdhcihostattr->SDHCIHOST_pdrvfuncs)) {
        return  (PX_ERROR);
    }

    if (!bInit) {
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_IO].sdhciReadB   = __sdhciIoReadB;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_IO].sdhciReadW   = __sdhciIoReadW;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_IO].sdhciReadL   = __sdhciIoReadL;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_IO].sdhciWriteB  = __sdhciIoWriteB;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_IO].sdhciWriteW  = __sdhciIoWriteW;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_IO].sdhciWriteL  = __sdhciIoWriteL;

        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_MEM].sdhciReadB  = __sdhciMemReadB;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_MEM].sdhciReadW  = __sdhciMemReadW;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_MEM].sdhciReadL  = __sdhciMemReadL;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_MEM].sdhciWriteB = __sdhciMemWriteB;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_MEM].sdhciWriteW = __sdhciMemWriteW;
        _G_sdhcidrvfuncTbl[SDHCI_REGACCESS_TYPE_MEM].sdhciWriteL = __sdhciMemWriteL;

        bInit = LW_TRUE;
    }

    /*
     * ����ṩ���Լ��ļĴ�������������ʹ��Ĭ�ϵķ���
     */
    if (!psdhcihostattr->SDHCIHOST_pdrvfuncs) {
        psdhcihostattr->SDHCIHOST_pdrvfuncs = &_G_sdhcidrvfuncTbl[iType];
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdhciIoReadL
** ��������: IO �ռ��ȡ32λ���ȵ�����
** ��    ��: uiAddr   IO �ռ��ַ
** ��    ��: ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32 __sdhciIoReadL (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr)
{
    return  in32(psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** ��������: __sdhciIoReadW
** ��������: IO �ռ��ȡ16λ���ȵ�����
** ��    ��: uiAddr   IO �ռ��ַ
** ��    ��: ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT16 __sdhciIoReadW (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr)
{
    return  in16(psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** ��������: __sdhciIoReadB
** ��������: IO �ռ��ȡ8λ���ȵ�����
** ��    ��: uiAddr   IO �ռ��ַ
** ��    ��: ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT8 __sdhciIoReadB (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr)
{
    return  in8(psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** ��������: __sdhciIoWriteL
** ��������: IO �ռ�д��32λ���ȵ�����
** ��    ��: uiAddr     IO �ռ��ַ
**           uiLword    д�������
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciIoWriteL (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT32 uiLword)
{
    out32(uiLword, psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** ��������: __sdhciIoWriteW
** ��������: IO �ռ�д��16λ���ȵ�����
** ��    ��: uiAddr     IO �ռ��ַ
**           usWord     д�������
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciIoWriteW (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT16 usWord)
{
    out16(usWord, psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** ��������:
** ��������: IO �ռ�д��8λ���ȵ�����
** ��    ��: uiAddr     IO �ռ��ַ
**           ucByte     д�������
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciIoWriteB (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT8 ucByte)
{
    out8(ucByte, psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** ��������: __sdhciMemReadL
** ��������: �ڴ�ռ��ȡ32λ���ȵ�����
** ��    ��: ulAddr   �ڴ�ռ��ַ
** ��    ��: ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32 __sdhciMemReadL (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr)
{
    return  read32(psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** ��������: __sdhciMemReadW
** ��������: �ڴ�ռ��ȡ16λ���ȵ�����
** ��    ��: ulAddr   �ڴ�ռ��ַ
** ��    ��: ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT16 __sdhciMemReadW (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr)
{
    return  read16(psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** ��������: __sdhciMemReadB
** ��������: �ڴ�ռ��ȡ8λ���ȵ�����
** ��    ��: ulAddr   �ڴ�ռ��ַ
** ��    ��: ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT8 __sdhciMemReadB (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr)
{
    return  read8(psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** ��������: __sdhciMemWriteL
** ��������: �ڴ�ռ�д��32λ���ȵ�����
** ��    ��: ulAddr     �ڴ�ռ��ַ
**           uiLword    д�������
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciMemWriteL (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT32 uiLword)
{
    write32(uiLword, psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** ��������: __sdhciMemWriteW
** ��������: �ڴ�ռ�д��16λ���ȵ�����
** ��    ��: ulAddr     �ڴ�ռ��ַ
**           usWord     д�������
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciMemWriteW (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT16 usWord)
{
    write16(usWord, psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** ��������: __sdhciMemWriteB
** ��������: �ڴ�ռ�д��8λ���ȵ�����
** ��    ��: ulAddr     �ڴ�ռ��ַ
**           ucByte     д�������
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciMemWriteB (PLW_SDHCI_HOST_ATTR   psdhcihostattr, ULONG ulAddr, UINT8 ucByte)
{
    write8(ucByte, psdhcihostattr->SDHCIHOST_ulBasePoint + ulAddr);
}
/*********************************************************************************************************
** ��������: __sdhciPreStaShow
** ��������: ��ʾ��ǰ����״̬
** ��    ��: psdhcihostattr
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#ifdef  __SYLIXOS_DEBUG
static VOID __sdhciPreStaShow (PLW_SDHCI_HOST_ATTR psdhcihostattr)
{
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */

    UINT32  uiSta;

    uiSta = SDHCI_READL(psdhcihostattr, SDHCI_PRESENT_STATE);
    printk("\nhost present status >>\n");
    printk("cmd line(cmd) : %s\n", uiSta & SDHCI_PSTA_CMD_INHIBIT ? "busy" : "free");
    printk("cmd line(dat) : %s\n", uiSta & SDHCI_PSTA_DATA_INHIBIT ? "busy" : "free");
    printk("dat line      : %s\n", uiSta & SDHCI_PSTA_DATA_ACTIVE ? "busy" : "free");
    printk("write active  : %s\n", uiSta & SDHCI_PSTA_DOING_WRITE ? "active" : "inactive");
    printk("read active   : %s\n", uiSta & SDHCI_PSTA_DOING_READ ? "active" : "inactive");
    printk("write buffer  : %s\n", uiSta & SDHCI_PSTA_SPACE_AVAILABLE ? "ready" : "not ready");
    printk("read  buffer  : %s\n", uiSta & SDHCI_PSTA_DATA_AVAILABLE ? "ready" : "not ready");
    printk("card insert   : %s\n", uiSta & SDHCI_PSTA_CARD_PRESENT ? "insert" : "not insert");
}
/*********************************************************************************************************
** ��������: __sdhciIntStaShow
** ��������: ��ʾ�ж�״̬
** ��    ��: psdhcihostattr
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdhciIntStaShow (PLW_SDHCI_HOST_ATTR psdhcihostattr)
{
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */

    UINT32  uiSta;

    uiSta = SDHCI_READL(psdhcihostattr, SDHCI_INT_STATUS);
    printk("\nhost int status >>\n");
    printk("cmd finish  : %s\n", uiSta & SDHCI_INT_RESPONSE ? "yes" : "no");
    printk("dat finish  : %s\n", uiSta & SDHCI_INT_DATA_END ? "yes" : "no");
    printk("dma finish  : %s\n", uiSta & SDHCI_INT_DMA_END ? "yes" : "no");
    printk("space avail : %s\n", uiSta & SDHCI_INT_SPACE_AVAIL ? "yes" : "no");
    printk("data avail  : %s\n", uiSta & SDHCI_INT_DATA_AVAIL ? "yes" : "no");
    printk("card insert : %s\n", uiSta & SDHCI_INT_CARD_INSERT ? "insert" : "not insert");
    printk("card remove : %s\n", uiSta & SDHCI_INT_CARD_REMOVE ? "remove" : "not remove");
}

#endif                                                                  /*  __SYLIXOS_DEBUG             */
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
