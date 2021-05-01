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
** ��   ��   ��: sdcoreLib.h
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2010 �� 11 �� 23 ��
**
** ��        ��: sd ����������ӿ�ͷ�ļ�

** BUG:
2010.11.27 �����˼��� API.
2010.03.30 ���� __sdCoreDevMmcSetRelativeAddr(), ��֧�� MMC ��.
*********************************************************************************************************/

#ifndef __SDCORE_LIB_H
#define __SDCORE_LIB_H

/*********************************************************************************************************
  SDIO functions
*********************************************************************************************************/

INT API_SdCoreDecodeCID(LW_SDDEV_CID  *psdcidDec, UINT32 *pRawCID, UINT8 ucType);
INT API_SdCoreDecodeCSD(LW_SDDEV_CSD  *psdcsdDec, UINT32 *pRawCSD, UINT8 ucType);
INT API_SdCoreDevReset(PLW_SDCORE_DEVICE psdcoredevice);
INT API_SdCoreDevSendIfCond(PLW_SDCORE_DEVICE psdcoredevice);
INT API_SdCoreDevSendAppOpCond(PLW_SDCORE_DEVICE  psdcoredevice,
                               UINT32             uiOCR,
                               LW_SDDEV_OCR      *puiOutOCR,
                               UINT8             *pucType);
INT API_SdCoreDevSendExtCSD(PLW_SDCORE_DEVICE  psdcoredevice, UINT8 *pucExtCsd);
INT API_SdCoreDevSwitch(PLW_SDCORE_DEVICE      psdcoredevice,
                        UINT8                  ucCmdSet,
                        UINT8                  ucIndex,
                        UINT8                  ucValue);
INT API_SdCoreDevSwitchEx(PLW_SDCORE_DEVICE psdcoredevice,
                          INT               iMode,
                          INT               iGroup,
                          INT               ucValue,
                          UINT8            *pucResp);
INT API_SdCoreDecodeExtCSD(PLW_SDCORE_DEVICE  psdcoredevice,
                           LW_SDDEV_CSD      *psdcsd,
                           LW_SDDEV_EXT_CSD  *psdextcsd);
INT API_SdCoreDevSendRelativeAddr(PLW_SDCORE_DEVICE psdcoredevice, UINT32 *puiRCA);
INT API_SdCoreDevSendAllCID(PLW_SDCORE_DEVICE psdcoredevice, LW_SDDEV_CID *psdcid);
INT API_SdCoreDevSendAllCSD(PLW_SDCORE_DEVICE psdcoredevice, LW_SDDEV_CSD *psdcsd);
INT API_SdCoreDevSendAllSCR(PLW_SDCORE_DEVICE psdcoredevice, LW_SDDEV_SCR *psdscr);
INT API_SdCoreDevSelect(PLW_SDCORE_DEVICE psdcoredevice);
INT API_SdCoreDevDeSelect(PLW_SDCORE_DEVICE psdcoredevice);
INT API_SdCoreDevSetBusWidth(PLW_SDCORE_DEVICE psdcoredevice, INT iBusWidth);
INT API_SdCoreDevSetBlkLen(PLW_SDCORE_DEVICE psdcoredevice, INT iBlkLen);
INT API_SdCoreDevSetBlkLenRaw(PLW_SDCORE_DEVICE psdcoredevice, INT iBlkLen);
INT API_SdCoreDevGetStatus(PLW_SDCORE_DEVICE psdcoredevice, UINT32 *puiStatus);
INT API_SdCoreDevSetPreBlkLen(PLW_SDCORE_DEVICE psdcoredevice, INT iPreBlkLen);
INT API_SdCoreDevIsBlockAddr(PLW_SDCORE_DEVICE psdcoredevice, BOOL *pbResult);

INT API_SdCoreDevSpiClkDely(PLW_SDCORE_DEVICE psdcoredevice, INT iClkConts);
INT API_SdCoreDevSpiCrcEn(PLW_SDCORE_DEVICE psdcoredevice, BOOL bEnable);

INT API_SdCoreDevMmcSetRelativeAddr(PLW_SDCORE_DEVICE psdcoredevice, UINT32 uiRCA);

#endif                                                                  /*  __SDCORE_LIB_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
