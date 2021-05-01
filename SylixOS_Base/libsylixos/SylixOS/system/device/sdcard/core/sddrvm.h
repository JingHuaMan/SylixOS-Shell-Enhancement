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
** ��   ��   ��: sddrvm.h
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2014 �� 10 �� 24 ��
**
** ��        ��: sd drv manager layer

** BUG:
2015.09.18  ���ӿ�������չѡ������, ����Ӧ����ʵ��Ӧ�õĳ���
2016.04.01  ���������豸���봴�����¼�. ��Ҫ����ϵͳ����(eMMC)�豸�ĳ�ʼ��.
2017.02.27  ���ӿ��������ñ�־����չѡ��,��������ܺͼ�����.
*********************************************************************************************************/

#ifndef __SDDRVM_H
#define __SDDRVM_H

#include "sdcore.h"

/*********************************************************************************************************
  SDM �¼����� for API_SdmEventNotify()

  SDM_EVENT_DEV_INSERT : ֪ͨ SDM ���豸����
      �������ָ����һ�� SDM Host ����, ��ֻ�Ը� Host �����豸̽�����, �������ͨ������
      ����������������Ȳ�δ���.
      �������Ϊ LW_NULL, ������еĻ��������κ��豸������ڹ¶��豸�� SDM Host ����
      �豸̽��, �������������Ӧ�ò��������о����豸���Ȳ�δ���ʱ��, ���Ҵ������, ����������ͨ������
      �����Ȳ���¼�.

  SDM_EVENT_DEV_REMOVE : ֪ͨ SDM ���Ƴ��豸
      ����ָ����һ�� SDM Host ����, ��ֻ�Ը� Host �����豸�Ƴ�����, �������ͨ������
      ����������������Ȳ�δ���. Ӧ������Ҳ����ͨ�� SD �����豸������ҵ���Ӧ�� SDM Host ����, �����ֶ�
      �Ƴ��豸����.

  SDM_EVENT_SDIO_INTERRUPT : ֪ͨ SDM �������һ�� SDIO �ж�. SDM �㽫��������ע��� SDIO �жϷ���.

  SDM_EVENT_BOOT_DEV_INSERT : ֪ͨ SDM ��һ�� BOOT �����豸����. ��ͬ�� SDM_EVENT_DEV_INSERT �����豸̽��
      �����Ȳ���߳��ﴦ��, ��������Ϣ��ֱ���ڵ�ǰ�߳�ִ���豸̽�����. רΪ BOOT �豸���ٳ�ʼ�����,
      ��һ����Ҫ���ظ��ļ�ϵͳ�� SD/MMC ��, ���������������ϵͳ�����ٶ�.
*********************************************************************************************************/

#define SDM_EVENT_DEV_INSERT            0
#define SDM_EVENT_DEV_REMOVE            1
#define SDM_EVENT_SDIO_INTERRUPT        2
#define SDM_EVENT_BOOT_DEV_INSERT       3

/*********************************************************************************************************
  ǰ������
*********************************************************************************************************/
struct sd_drv;
struct sd_host;

typedef struct sd_drv     SD_DRV;
typedef struct sd_host    SD_HOST;

/*********************************************************************************************************
  sd ����(����sd memory �� sdio base)
*********************************************************************************************************/

struct sd_drv {
    LW_LIST_LINE  SDDRV_lineManage;                               /*  ����������                        */

    CPCHAR        SDDRV_cpcName;
#define SDDRV_SDMEM_NAME      "sd memory"
#define SDDRV_SDIOB_NAME      "sdio base"

    INT         (*SDDRV_pfuncDevCreate)(SD_DRV *psddrv, PLW_SDCORE_DEVICE psdcoredev, VOID **ppvDevPriv);
    INT         (*SDDRV_pfuncDevDelete)(SD_DRV *psddrv, VOID *pvDevPriv);

    atomic_t      SDDRV_atomicDevCnt;

    VOID         *SDDRV_pvSpec;
};

/*********************************************************************************************************
  sd host ��Ϣ�ṹ
*********************************************************************************************************/

#ifdef  __cplusplus
typedef INT     (*SD_CALLBACK)(...);
#else
typedef INT     (*SD_CALLBACK)();
#endif

struct sd_host {
    CPCHAR        SDHOST_cpcName;

    INT           SDHOST_iType;
#define SDHOST_TYPE_SD                  0
#define SDHOST_TYPE_SPI                 1

    INT           SDHOST_iCapbility;                                /*  ����֧�ֵ�����                  */
#define SDHOST_CAP_HIGHSPEED            (1 << 0)                    /*  ֧�ָ��ٴ���                    */
#define SDHOST_CAP_DATA_4BIT            (1 << 1)                    /*  ֧��4λ���ݴ���                 */
#define SDHOST_CAP_DATA_8BIT            (1 << 2)                    /*  ֧��8λ���ݴ���                 */
#define SDHOST_CAP_DATA_4BIT_DDR        (1 << 3)                    /*  ֧��4λddr���ݴ���              */
#define SDHOST_CAP_DATA_8BIT_DDR        (1 << 4)                    /*  ֧��8λddr���ݴ���              */
#define SDHOST_CAP_MMC_FORCE_1BIT       (1 << 5)                    /*  MMC�� ǿ��ʹ�� 1 λ����         */
#define SDHOST_CAP_SDIO_FORCE_1BIT      (1 << 6)                    /*  SDIO �� ǿ��ʹ�� 1 λ����       */
#define SDHOST_CAP_SD_FORCE_1BIT        (1 << 7)                    /*  SD �� ǿ��ʹ�� 1 λ����         */

    VOID          (*SDHOST_pfuncSpicsEn)(SD_HOST *psdhost);
    VOID          (*SDHOST_pfuncSpicsDis)(SD_HOST *psdhost);
    INT           (*SDHOST_pfuncCallbackInstall)
                  (
                  SD_HOST          *psdhost,
                  INT               iCallbackType,                  /*  ��װ�Ļص�����������            */
                  SD_CALLBACK       callback,                       /*  �ص�����ָ��                    */
                  PVOID             pvCallbackArg                   /*  �ص������Ĳ���                  */
                  );

    INT           (*SDHOST_pfuncCallbackUnInstall)
                  (
                  SD_HOST          *psdhost,
                  INT               iCallbackType                   /*  ��װ�Ļص�����������            */
                  );
#define SDHOST_CALLBACK_CHECK_DEV       0                           /*  ��״̬���                      */
#define SDHOST_DEVSTA_UNEXIST           0                           /*  ��״̬:������                   */
#define SDHOST_DEVSTA_EXIST             1                           /*  ��״̬:����                     */

    VOID          (*SDHOST_pfuncSdioIntEn)(SD_HOST *psdhost, BOOL bEnable);
    BOOL          (*SDHOST_pfuncIsCardWp)(SD_HOST *psdhost);

    VOID          (*SDHOST_pfuncDevAttach)(SD_HOST *psdhost, CPCHAR cpcDevName);
    VOID          (*SDHOST_pfuncDevDetach)(SD_HOST *psdhost);
};


/*********************************************************************************************************
  sd host ��չѡ��,��Щѡ�������ָ����һ��Ӳ��������ͨ����Ч
*********************************************************************************************************/

#define SDHOST_EXTOPT_RESERVE_SECTOR_SET        0                   /*  ���ñ�����������                */
#define SDHOST_EXTOPT_RESERVE_SECTOR_GET        1                   /*  ��ñ�����������                */

#define SDHOST_EXTOPT_MAXBURST_SECTOR_SET       2                   /*  �������⧷���������            */
#define SDHOST_EXTOPT_MAXBURST_SECTOR_GET       3                   /*  ������⧷���������            */

#define SDHOST_EXTOPT_CACHE_SIZE_SET            4                   /*  ���ô��� cache ��С             */
#define SDHOST_EXTOPT_CACHE_SIZE_GET            5                   /*  ��ô��� cache ��С             */

#define SDHOST_EXTOPT_CACHE_PL_SET              6                   /*  ���ô��̹�������                */
#define SDHOST_EXTOPT_CACHE_PL_GET              7                   /*  ��ô��̹�������                */

#define SDHOST_EXTOPT_CACHE_COHERENCE_SET       8                   /*  ���ô��� cache һ��������       */
#define SDHOST_EXTOPT_CACHE_COHERENCE_GET       9                   /*  ��ô��� cache һ��������       */

#define SDHOST_EXTOPT_CONFIG_FLAG_SET           10                  /*  �������ñ�־                    */
#define SDHOST_EXTOPT_CONFIG_FLAG_GET           11                  /*  ������ñ�־                    */

#define SDHOST_EXTOPT_CONFIG_RESELECT_SDMEM     (1 << 0)            /*  SDMEM ÿһ�δ��䶼����ѡ��    */
#define SDHOST_EXTOPT_CONFIG_RESELECT_SDIO      (1 << 1)            /*  SDIO ÿһ�δ��䶼����ѡ��     */
#define SDHOST_EXTOPT_CONFIG_SKIP_SDMEM         (1 << 2)            /*  ���� SDMEM ����̽��             */
#define SDHOST_EXTOPT_CONFIG_SKIP_SDIO          (1 << 3)            /*  ���� SDIO ����̽��              */

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API INT   API_SdmLibInit(VOID);

LW_API PVOID API_SdmHostRegister(SD_HOST *psdhost);
LW_API INT   API_SdmHostUnRegister(PVOID  pvSdmHost);

LW_API INT   API_SdmHostCapGet(PLW_SDCORE_DEVICE psdcoredev, INT *piCapbility);
LW_API VOID  API_SdmHostInterEn(PLW_SDCORE_DEVICE psdcoredev, BOOL bEnable);
LW_API BOOL  API_SdmHostIsCardWp(PLW_SDCORE_DEVICE psdcoredev);

LW_API PVOID API_SdmHostGet(PLW_SDCORE_DEVICE psdcoredev);

LW_API INT   API_SdmSdDrvRegister(SD_DRV *psddrv);
LW_API INT   API_SdmSdDrvUnRegister(SD_DRV *psddrv);

LW_API INT   API_SdmEventNotify(PVOID pvSdmHost, INT iEvtType);

/*********************************************************************************************************
  ��չѡ������ API
  API_SdmHostExtOptSet �����������ÿ���������չѡ��
  API_SdmHostExtOptGet ����Э����ȡ����������չѡ��
*********************************************************************************************************/

LW_API INT   API_SdmHostExtOptSet(PVOID pvSdmHost, INT  iOption, LONG  lArg);
LW_API INT   API_SdmHostExtOptGet(PLW_SDCORE_DEVICE psdcoredev, INT  iOption, LONG  lArg);

#endif                                                              /*  __SDDRVM_H                      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
