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
** ��   ��   ��: sdcore.h
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2010 �� 11 �� 23 ��
**
** ��        ��: sd ���ں�Э���ӿ�ͷ�ļ�

** BUG:
2011.01.12  ���Ӷ� SPI ��֧��(SPI �µ����⹤�ߺ���).
2011.02.21  ���� API_SdCoreSpiSendIfCond ����.�ú���ֻ������ SPI ģʽ��.
2011.02.21  �� SPI ���豸�Ĵ����Ķ�ȡ����: API_SdCoreSpiRegisterRead().
2011.03.25  �޸� API_SdCoreDevCreate(), ���ڵײ�������װ�ϲ�Ļص�.
2015.09.15  �޸� SD_OPCOND_DELAY_CONTS ��100��Ϊ5000, ʹ��ʶ��׶ξ��и��õļ�����.
*********************************************************************************************************/

#ifndef __SDCORE_H
#define __SDCORE_H

/*********************************************************************************************************
  һ��궨��
*********************************************************************************************************/

#define SDADAPTER_TYPE_SD         0
#define SDADAPTER_TYPE_SPI        1

#define SD_CMD_GEN_RETRY          4
#define SD_OPCOND_DELAY_CONTS     5000

#define SD_DELAYMS(ms)                                      \
        do {                                                \
            ULONG   ulTimeout = LW_MSECOND_TO_TICK_1(ms);   \
            API_TimeSleep(ulTimeout);                       \
        } while (0)

/*********************************************************************************************************
  SD ���Ĳ��豸
*********************************************************************************************************/

typedef struct lw_sdcore_device {
    PVOID      COREDEV_pvDevHandle;                                     /*  �豸���                    */
    INT        COREDEV_iAdapterType;                                    /*  ���ڵ�����������            */
#define COREDEV_IS_SD(pcdev)            (pcdev->COREDEV_iAdapterType == SDADAPTER_TYPE_SD)
#define COREDEV_IS_SPI(pcdev)           (pcdev->COREDEV_iAdapterType == SDADAPTER_TYPE_SPI)

    INT        COREDEV_iDevSta;
#define COREDEV_STA_HIGHSPEED_EN        (1 << 0)
#define COREDEV_HIGHSPEED_SET(pcdev)    (pcdev->COREDEV_iDevSta |= COREDEV_STA_HIGHSPEED_EN)
#define COREDEV_IS_HIGHSPEED(pcdev)     (pcdev->COREDEV_iDevSta & COREDEV_STA_HIGHSPEED_EN)

#define COREDEV_STA_HIGHCAP_OCR         (1 << 1)                        /*  �豸 OCR ������������Ϣ     */

    spinlock_t COREDEV_slLock;

    INT      (*COREDEV_pfuncCoreDevXfer)(PVOID  pvDevHandle, PLW_SD_MESSAGE psdmsg, INT iNum);
    INT      (*COREDEV_pfuncCoreDevCtl)(PVOID   pvDevHandle, INT iCmd, LONG lArg);
    INT      (*COREDEV_pfuncCoreDevDelet)(PVOID pvDevHandle);
} LW_SDCORE_DEVICE, *PLW_SDCORE_DEVICE;

/*********************************************************************************************************
  SD ͨ���ṹ
*********************************************************************************************************/

struct  __sdcore_drv_funcs;
typedef struct __sdcore_drv_funcs    SDCORE_DRV_FUNCS;

typedef struct __sdcore_chan {
    SDCORE_DRV_FUNCS    *SDCORECHA_pDrvFuncs;
} LW_SDCORE_CHAN, *PLW_SDCORE_CHAN;                                     /*  SD �����ṹ��               */
#define SDCORE_CHAN_INSTALL(cha)                    ((cha)->SDCORECHA_pDrvFuncs->callbackInstall)
#define SDCORE_CHAN_CBINSTALL(cha, type, cb, arg)   SDCORE_CHAN_INSTALL(cha)(cha, type, cb, arg)

#define SDCORE_CHAN_SPICSEN(cha)                    ((cha)->SDCORECHA_pDrvFuncs->callbackSpicsEn)
#define SDCORE_CHAN_SPICSDIS(cha)                   ((cha)->SDCORECHA_pDrvFuncs->callbackSpicsDis)
/*********************************************************************************************************
  SD �ص�����
*********************************************************************************************************/

#ifdef  __cplusplus
typedef INT     (*SDCORE_CALLBACK)(...);
#else
typedef INT     (*SDCORE_CALLBACK)();
#endif

/*********************************************************************************************************
  SD ������װ�ص�����
*********************************************************************************************************/

typedef INT     (*SDCORE_CALLBACK_INSTALL)
                (
                PLW_SDCORE_CHAN  psdcorechan,                           /*  ��װ�ص�ʹ�õĲ��� �������� */
                INT              iCallbackType,                         /*  ��װ�Ļص�����������        */
                SDCORE_CALLBACK  callback,                              /*  �ص�����ָ��                */
                PVOID            pvCallbackArg                          /*  �ص������Ĳ���              */
                );

/*********************************************************************************************************
  SD SPIģʽ�µ�Ƭѡ�ص�
*********************************************************************************************************/
typedef VOID    (*SDCORE_CALLBACK_SPICS_ENABLE)(PLW_SDCORE_CHAN psdcorechan);
typedef VOID    (*SDCORE_CALLBACK_SPICS_DISABLE)(PLW_SDCORE_CHAN psdcorechan);

/*********************************************************************************************************
  SD �����ṹ
*********************************************************************************************************/

struct __sdcore_drv_funcs {
    SDCORE_CALLBACK_INSTALL       callbackInstall;
    SDCORE_CALLBACK_SPICS_ENABLE  callbackSpicsEn;
    SDCORE_CALLBACK_SPICS_DISABLE callbackSpicsDis;
};

/*********************************************************************************************************
  SD ��װ�ص���������
*********************************************************************************************************/

#define SD_CALLBACK_CHECK_DEV     0                                     /*  ��״̬���                  */
#define SD_DEVSTA_UNEXIST         0                                     /*  ��״̬:������               */
#define SD_DEVSTA_EXIST           1                                     /*  ��״̬:����                 */

/*********************************************************************************************************
  ���Ĳ��豸��������
*********************************************************************************************************/

LW_API PLW_SDCORE_DEVICE  API_SdCoreDevCreate(INT                       iAdapterType,
                                              CPCHAR                    pcAdapterName,
                                              CPCHAR                    pcDeviceName,
                                              PLW_SDCORE_CHAN           psdcorechan);
LW_API INT              API_SdCoreDevDelete(PLW_SDCORE_DEVICE    psdcoredevice);

LW_API INT              API_SdCoreDevCtl(PLW_SDCORE_DEVICE    psdcoredevice,
                                         INT                  iCmd,
                                         LONG                 lArg);
LW_API INT              API_SdCoreDevTransfer(PLW_SDCORE_DEVICE  psdcoredevice,
                                              PLW_SD_MESSAGE     psdMsg,
                                              INT                iNum);
LW_API INT              API_SdCoreDevCmd(PLW_SDCORE_DEVICE psdcoredevice,
                                         PLW_SD_COMMAND    psdCmd,
                                         UINT32            uiRetry);
LW_API INT              API_SdCoreDevAppSwitch(PLW_SDCORE_DEVICE psdcoredevice, BOOL bIsBc);
LW_API INT              API_SdCoreDevAppCmd(PLW_SDCORE_DEVICE psdcoredevice,
                                            PLW_SD_COMMAND    psdcmdAppCmd,
                                            BOOL              bIsBc,
                                            UINT32            uiRetry);

LW_API CPCHAR           API_SdCoreDevAdapterName(PLW_SDCORE_DEVICE psdcoredevice);

/*********************************************************************************************************
  ����APIֻ��ȥ �鿴/���� �ں˽ṹ�еĳ�Ա����,��δ���豸����ʵ���ϵ��������
*********************************************************************************************************/

LW_API INT              API_SdCoreDevCsdSet(PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CSD    psdcsd);
LW_API INT              API_SdCoreDevCidSet(PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CID    psdcid);
LW_API INT              API_SdCoreDevScrSet(PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_SCR    psdscr);
LW_API INT              API_SdCoreDevSwCapSet(PLW_SDCORE_DEVICE psdcoredevice,PLW_SDDEV_SW_CAP psdswcap);
LW_API INT              API_SdCoreDevRcaSet(PLW_SDCORE_DEVICE psdcoredevice,  UINT32           uiRCA);
LW_API INT              API_SdCoreDevTypeSet(PLW_SDCORE_DEVICE psdcoredevice, UINT8            ucType);

LW_API INT              API_SdCoreDevCsdView(PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CSD    psdcsd);
LW_API INT              API_SdCoreDevCidView(PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CID    psdcid);
LW_API INT              API_SdCoreDevScrView(PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_SCR    psdscr);
LW_API INT              API_SdCoreDevSwCapView(PLW_SDCORE_DEVICE psdcoredevice,PLW_SDDEV_SW_CAP psdswcap);
LW_API INT              API_SdCoreDevRcaView(PLW_SDCORE_DEVICE psdcoredevice,  UINT32          *puiRCA);
LW_API INT              API_SdCoreDevTypeView(PLW_SDCORE_DEVICE psdcoredevice, UINT8           *pucType);

/*********************************************************************************************************
  ״̬�鿴
*********************************************************************************************************/

LW_API INT              API_SdCoreDevStaView(PLW_SDCORE_DEVICE  psdcoredevice);

/*********************************************************************************************************
  SPI �µ�����Ӧ��
*********************************************************************************************************/

LW_API VOID             API_SdCoreSpiCxdFormat(UINT32 *puiCxdOut, UINT8 *pucRawCxd);
LW_API VOID             API_SdCoreSpiMulWrtStop(PLW_SDCORE_DEVICE psdcoredevice);
LW_API INT              API_SdCoreSpiSendIfCond(PLW_SDCORE_DEVICE psdcoredevice);
LW_API INT              API_SdCoreSpiRegisterRead(PLW_SDCORE_DEVICE  psdcoredevice,
                                                  UINT8             *pucReg,
                                                  UINT               uiLen);

#endif                                                                  /*  __SDCORE_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
