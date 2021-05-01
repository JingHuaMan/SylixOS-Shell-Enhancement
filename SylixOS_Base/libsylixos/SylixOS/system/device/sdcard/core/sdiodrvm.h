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
** ��   ��   ��: sdiodrvm.h
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2014 �� 10 �� 27 ��
**
** ��        ��: sd drv manager layer for sdio

** BUG:
*********************************************************************************************************/

#ifndef __SDIODRVM_H
#define __SDIODRVM_H

#include "sdcore.h"

/*********************************************************************************************************
  ǰ������
*********************************************************************************************************/

struct sdio_drv;
struct sdio_dev_id;
struct sdio_cccr;
struct sdio_func_tuple;
struct sdio_func;
struct sdio_init_data;

typedef struct sdio_drv         SDIO_DRV;
typedef struct sdio_dev_id      SDIO_DEV_ID;
typedef struct sdio_cccr        SDIO_CCCR;
typedef struct sdio_func_tuple  SDIO_FUNC_TUPLE;
typedef struct sdio_func        SDIO_FUNC;
typedef struct sdio_init_data   SDIO_INIT_DATA;

/*********************************************************************************************************
  sdio �豸 ��ض���
*********************************************************************************************************/

struct sdio_dev_id {
    UINT8    DEVID_ucClass;                                         /*  Std interface or SDIO_ANY_ID    */
    UINT16   DEVID_usVendor;                                        /*  Vendor or SDIO_ANY_ID           */
    UINT16   DEVID_usDevice;                                        /*  Device ID or SDIO_ANY_ID        */
    VOID    *DEVID_pvDrvPriv;                                       /*  driver private data             */
};
#define SDIO_DEV_ID_ANY     (~0)

struct sdio_cccr {
    UINT32    CCCR_uiSdioVsn;
    UINT32    CCCR_uiSdVsn;
    BOOL      CCCR_bMulBlk;
    BOOL      CCCR_bLowSpeed;                                       /*  ���ٿ�(400khz)                  */
    BOOL      CCCR_bWideBus;                                        /*  �ڵ��ٿ�����£��Ƿ�֧��4λ���� */
    BOOL      CCCR_bHighPwr;
    BOOL      CCCR_bHighSpeed;                                      /*  ���ٿ�(50Mhz)                   */
    BOOL      CCCR_bDisableCd;                                      /*  for sdio drv config             */
};

struct sdio_func_tuple {
    SDIO_FUNC_TUPLE *TUPLE_ptupleNext;
    UINT8            TUPLE_ucCode;
    UINT8            TUPLE_ucSize;
    UINT8            TUPLE_pucData[1];
};

struct sdio_func {
    UINT32           FUNC_uiNum;
    UINT8            FUNC_ucClass;
    UINT16           FUNC_usVendor;
    UINT16           FUNC_usDevice;

    ULONG            FUNC_ulMaxBlkSize;
    ULONG            FUNC_ulCurBlkSize;

    UINT32           FUNC_uiEnableTimeout;                          /*  in milli second                 */

    UINT32           FUNC_uiMaxDtr;                                 /*  ������0ʹ��                     */

    SDIO_FUNC_TUPLE *FUNC_ptupleListHeader;                         /*  ���ܽ�����TUPLE����             */
    const SDIO_CCCR *FUNC_cpsdiocccr;                               /*  ָ��ͬһ��CCCR                  */
};

#define SDIO_FUNC_MAX   8
struct sdio_init_data {
    SDIO_FUNC          INIT_psdiofuncTbl[SDIO_FUNC_MAX];
    INT                INIT_iFuncCnt;                               /*  ������Func0                     */
    PLW_SDCORE_DEVICE  INIT_psdcoredev;
    SDIO_CCCR          INIT_sdiocccr;
    const SDIO_DEV_ID *INIT_pdevidCurr[SDIO_FUNC_MAX];              /*  ��ǰ�豸������ƥ��Ĺ���ID      */
};

/*********************************************************************************************************
  sdio ����
*********************************************************************************************************/

struct sdio_drv {
    LW_LIST_LINE  SDIODRV_lineManage;                               /*  ����������                      */
    CPCHAR        SDIODRV_cpcName;

    INT         (*SDIODRV_pfuncDevCreate)(SDIO_DRV         *psdiodrv,
                                          SDIO_INIT_DATA   *pinitdata,
                                          VOID            **ppvDevPriv);
    INT         (*SDIODRV_pfuncDevDelete)(SDIO_DRV *psdiodrv, VOID *pvDevPriv);
    VOID        (*SDIODRV_pfuncIrqHandle)(SDIO_DRV *psdiodrv, VOID *pvDevPriv);

    SDIO_DEV_ID  *SDIODRV_pdevidTbl;
    INT           SDIODRV_iDevidCnt;
    VOID         *SDIODRV_pvSpec;

    atomic_t      SDIODRV_atomicDevCnt;
};

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API INT   API_SdmSdioLibInit(VOID);
LW_API INT   API_SdmSdioDrvRegister(SDIO_DRV *psdiodrv);
LW_API INT   API_SdmSdioDrvUnRegister(SDIO_DRV *psdiodrv);

#endif                                                              /*  __SDIODRVM_H                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
