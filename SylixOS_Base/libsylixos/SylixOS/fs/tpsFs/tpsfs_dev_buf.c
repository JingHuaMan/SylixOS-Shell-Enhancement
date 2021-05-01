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
** ��   ��   ��: tpsfs_dev_buf.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: ���̻���������

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0
#include "tpsfs_type.h"
#include "tpsfs_error.h"
#include "tpsfs_port.h"
#include "tpsfs_super.h"
#include "tpsfs_trans.h"
#include "tpsfs_btree.h"
#include "tpsfs_inode.h"
#include "tpsfs_dir.h"
#include "tpsfs_dev_buf.h"
/*********************************************************************************************************
** ��������: tpsFsDevBufWrite
** ��������: д�����ݵ�����
** �䡡��  : psb              ������ָ��
**           blk              ���
**           uiOff            ����ƫ��
**           pucBuff          ���ݻ�����
**           szLen            д�볤��
**           bSync            �Ƿ�ͬ��д��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsDevBufWrite (PTPS_SUPER_BLOCK  psb,
                              TPS_IBLK          blk,
                              UINT              uiOff,
                              PUCHAR            pucBuff,
                              size_t            szLen,
                              BOOL              bSync)
{
    size_t        szCompleted = 0;
    UINT          uiReadLen;
    PUCHAR        pucSecBuf   = LW_NULL;
    UINT          uiSecSize   = 0;
    UINT          uiSecOff    = 0;
    UINT64        ui64SecNum  = 0;
    UINT64        ui64SecCnt  = 0;

    if ((psb == LW_NULL)     ||
        (pucBuff == LW_NULL) ||
        (psb->SB_pucSectorBuf == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    uiSecSize = psb->SB_uiSectorSize;
    pucSecBuf = psb->SB_pucSectorBuf;
    
    ui64SecNum = ((blk << psb->SB_uiBlkShift) + uiOff) >> psb->SB_uiSectorShift;
    ui64SecCnt = (((blk << psb->SB_uiBlkShift) + uiOff + szLen) >> psb->SB_uiSectorShift) - ui64SecNum;

    uiSecOff = uiOff & psb->SB_uiSectorMask;
    if (uiSecOff != 0) {                                                /* ��ʼλ�ò�����,�ȶ�ȡ��д��  */
        uiReadLen = uiSecSize - uiSecOff;
        uiReadLen = min(szLen, uiReadLen);
        if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1) != 0) {
            return  (TPS_ERR_BUF_READ);
        }

        lib_memcpy(pucSecBuf + uiSecOff, pucBuff, uiReadLen);

        if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1, bSync) != 0) {
            return  (TPS_ERR_BUF_WRITE);
        }

        szCompleted += uiReadLen;
        ui64SecNum++;
        if (ui64SecCnt > 0) {
            ui64SecCnt--;
        }
    }

    if (ui64SecCnt > 0) {                                               /* ��������ֱ�Ӳ���������       */
        if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, pucBuff + szCompleted,
                                         ui64SecNum, ui64SecCnt, bSync) != 0) {
            return  (TPS_ERR_BUF_WRITE);
        }

        szCompleted += (size_t)(ui64SecCnt * uiSecSize);
        ui64SecNum  += ui64SecCnt;
        ui64SecCnt   = 0;
    }

    if (szCompleted < szLen) {                                          /* ����λ�ò�����,�ȶ�ȡ��д��  */
        uiReadLen = ((uiOff + szLen) & psb->SB_uiSectorMask);
        if (uiReadLen > 0) {
            if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1) != 0) {
                return  (TPS_ERR_BUF_READ);
            }

            lib_memcpy(pucSecBuf, pucBuff + szCompleted, uiReadLen);

            if (psb->SB_dev->DEV_WriteSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1, bSync) != 0) {
                return  (TPS_ERR_BUF_WRITE);
            }

            szCompleted += uiReadLen;
        }
    }


    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsDevBufRead
** ��������: �Ӵ��̶�ȡ����
** �䡡��  : psb              ������ָ��
**           blk              ���
**           uiOff            ����ƫ��
**           pucBuff          ���ݻ�����
**           szLen            д�볤��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsDevBufRead (PTPS_SUPER_BLOCK   psb,
                             TPS_IBLK           blk,
                             UINT               uiOff,
                             PUCHAR             pucBuff,
                             size_t             szLen)
{
    size_t        szCompleted  = 0;
    UINT          uiReadLen;
    PUCHAR        pucSecBuf    = LW_NULL;
    UINT          uiSecSize    = 0;
    UINT          uiSecOff     = 0;
    UINT64        ui64SecNum   = 0;
    UINT64        ui64SecCnt   = 0;

    if ((psb == LW_NULL)     ||
        (pucBuff == LW_NULL) ||
        (psb->SB_pucSectorBuf == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    uiSecSize = psb->SB_uiSectorSize;
    pucSecBuf = psb->SB_pucSectorBuf;
    
    ui64SecNum = ((blk << psb->SB_uiBlkShift) + uiOff) >> psb->SB_uiSectorShift;
    ui64SecCnt = (((blk << psb->SB_uiBlkShift) + uiOff + szLen) >> psb->SB_uiSectorShift) - ui64SecNum;

    uiSecOff = uiOff & psb->SB_uiSectorMask;
    if (uiSecOff != 0) {                                                /* ��ʼλ�ò�����               */
        uiReadLen = uiSecSize - uiSecOff;
        uiReadLen = min(szLen, uiReadLen);
        if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1) != 0) {
            return  (TPS_ERR_BUF_READ);
        }

        lib_memcpy(pucBuff, pucSecBuf + uiSecOff, uiReadLen);
        
        szCompleted += uiReadLen;
        ui64SecNum++;
        if (ui64SecCnt > 0) {
            ui64SecCnt--;
        }
    }

    if (ui64SecCnt > 0) {                                               /* ��������ֱ��д������         */
        if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucBuff + szCompleted,
                                        ui64SecNum, ui64SecCnt) != 0) {
            return  (TPS_ERR_BUF_READ);
        }

        szCompleted += (size_t)(ui64SecCnt * uiSecSize);
        ui64SecNum  += ui64SecCnt;
        ui64SecCnt   = 0;
    }

    if (szCompleted <  szLen) {                                         /* ����λ�ò�����               */
        uiReadLen = ((uiOff + szLen) & psb->SB_uiSectorMask);
        if (uiReadLen > 0) {
            if (psb->SB_dev->DEV_ReadSector(psb->SB_dev, pucSecBuf, ui64SecNum, 1) != 0) {
                return  (TPS_ERR_BUF_READ);
            }

            lib_memcpy(pucBuff + szCompleted, pucSecBuf, uiReadLen);

            szCompleted += uiReadLen;
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsDevBufSync
** ��������: ͬ�����̻�����
** �䡡��  : psb              ������ָ��
**           blk              ���
**           uiOff            ����ƫ��
**           szLen            д�볤��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsDevBufSync (PTPS_SUPER_BLOCK psb, TPS_IBLK blk, UINT uiOff, size_t szLen)
{
    UINT64  ui64SecNum = 0;
    UINT64  ui64SecCnt = 0;

    if (psb == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }
    
    ui64SecNum = ((blk << psb->SB_uiBlkShift) + uiOff) >> psb->SB_uiSectorShift;
    ui64SecCnt = (((blk << psb->SB_uiBlkShift) + uiOff + szLen) >> psb->SB_uiSectorShift) - ui64SecNum;
    
    if ((uiOff + szLen) & psb->SB_uiSectorMask) {
        ui64SecCnt++;
    }

    if (psb->SB_dev->DEV_Sync(psb->SB_dev, ui64SecNum, ui64SecCnt) != 0) {
        return  (TPS_ERR_BUF_SYNC);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsDevBufTrim
** ��������: �Դ���ִ��FIOTRIM����
** �䡡��  : psb              ������ָ��
**           blk              ���
**           blkStart         ��ʼ��
**           blkCnt           ����Ŀ
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsDevBufTrim (PTPS_SUPER_BLOCK psb, TPS_IBLK blkStart, TPS_IBLK blkCnt)
{
    UINT64  ui64SecNum = 0;
    UINT64  ui64SecCnt = 0;

    if (psb == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    ui64SecNum = (blkStart << psb->SB_uiBlkShift) >> psb->SB_uiSectorShift;
    ui64SecCnt = (blkCnt << psb->SB_uiBlkShift) >> psb->SB_uiSectorShift;

    if (psb->SB_dev->DEV_Trim(psb->SB_dev, ui64SecNum, ui64SecCnt) != 0) {
        return  (TPS_ERR_BUF_TRIM);
    }

    return  (TPS_ERR_NONE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
