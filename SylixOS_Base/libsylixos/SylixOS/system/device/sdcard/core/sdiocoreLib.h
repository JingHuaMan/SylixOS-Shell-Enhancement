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
** ��   ��   ��: sdiocoreLib.h
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2014 �� 10 �� 28 ��
**
** ��        ��: sdio ����������ӿ�ͷ�ļ�

** BUG:
*********************************************************************************************************/

#ifndef __SDIOCORE_LIB_H
#define __SDIOCORE_LIB_H

/*********************************************************************************************************
  SDIO API
*********************************************************************************************************/

INT API_SdioCoreDevReset(PLW_SDCORE_DEVICE   psdcoredev);
INT API_SdioCoreDevSendIoOpCond(PLW_SDCORE_DEVICE   psdcoredev, UINT32 uiOcr, UINT32 *puiOcrOut);

INT API_SdioCoreDevReadFbr(PLW_SDCORE_DEVICE   psdcoredev, SDIO_FUNC *psdiofunc);
INT API_SdioCoreDevReadCCCR(PLW_SDCORE_DEVICE   psdcoredev, SDIO_CCCR *psdiocccr);

INT API_SdioCoreDevReadCis(PLW_SDCORE_DEVICE   psdcoredev, SDIO_FUNC *psdiofunc);
INT API_SdioCoreDevFuncClean(SDIO_FUNC *psdiofunc);

INT API_SdioCoreDevHighSpeedEn(PLW_SDCORE_DEVICE   psdcoredev, SDIO_CCCR *psdiocccr);
INT API_SdioCoreDevWideBusEn(PLW_SDCORE_DEVICE   psdcoredev, SDIO_CCCR *psdiocccr);

INT API_SdioCoreDevReadByte(PLW_SDCORE_DEVICE   psdcoredev,
                            UINT32              uiFn,
                            UINT32              uiAddr,
                            UINT8              *pucByte);
INT API_SdioCoreDevWriteByte(PLW_SDCORE_DEVICE   psdcoredev,
                             UINT32              uiFn,
                             UINT32              uiAddr,
                             UINT8               ucByte);
INT API_SdioCoreDevWriteThenReadByte(PLW_SDCORE_DEVICE   psdcoredev,
                                     UINT32              uiFn,
                                     UINT32              uiAddr,
                                     UINT8               ucWrByte,
                                     UINT8              *pucRdByte);

INT API_SdioCoreDevFuncEn(PLW_SDCORE_DEVICE   psdcoredev,
                          SDIO_FUNC          *psdiofunc);
INT API_SdioCoreDevFuncDis(PLW_SDCORE_DEVICE   psdcoredev,
                           SDIO_FUNC          *psdiofunc);

INT API_SdioCoreDevFuncIntEn(PLW_SDCORE_DEVICE   psdcoredev,
                             SDIO_FUNC          *psdiofunc);
INT API_SdioCoreDevFuncIntDis(PLW_SDCORE_DEVICE   psdcoredev,
                              SDIO_FUNC          *psdiofunc);

INT API_SdioCoreDevFuncBlkSzSet(PLW_SDCORE_DEVICE   psdcoredev,
                                SDIO_FUNC          *psdiofunc,
                                UINT32              uiBlkSz);

INT API_SdioCoreDevRwDirect(PLW_SDCORE_DEVICE   psdcoredev,
                            BOOL                bWrite,
                            UINT32              uiFn,
                            UINT32              uiAddr,
                            UINT8               ucWrData,
                            UINT8              *pucRdBack);

INT API_SdioCoreDevRwExtend(PLW_SDCORE_DEVICE   psdcoredev,
                            BOOL                bWrite,
                            UINT32              uiFn,
                            UINT32              uiAddr,
                            BOOL                bAddrInc,
                            UINT8              *pucBuf,
                            UINT32              uiBlkCnt,
                            UINT32              uiBlkSz);

INT API_SdioCoreDevRwExtendX(PLW_SDCORE_DEVICE   psdcoredev,
                             BOOL                bWrite,
                             UINT32              uiFn,
                             BOOL                bIsBlkMode,
                             UINT32              uiAddr,
                             BOOL                bAddrInc,
                             UINT8              *pucBuf,
                             UINT32              uiBlkCnt,
                             UINT32              uiBlkSz);

#endif                                                              /*  __SDIOCORE_LIB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
