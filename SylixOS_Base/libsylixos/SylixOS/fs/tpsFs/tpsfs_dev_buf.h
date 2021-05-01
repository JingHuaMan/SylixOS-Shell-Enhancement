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
** ��   ��   ��: tpsfs_dev_buf.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: ���̻���������

** BUG:
*********************************************************************************************************/

#ifndef __TSPSFS_DEV_BUF_H
#define __TSPSFS_DEV_BUF_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0
                                                                        /* д�����ݵ�����               */
TPS_RESULT tpsFsDevBufWrite(PTPS_SUPER_BLOCK psb, TPS_IBLK blk, UINT uiOff,
                            PUCHAR pucBuff, size_t szLen, BOOL bSync);
                                                                        /* �Ӵ��̶�ȡ����               */
TPS_RESULT tpsFsDevBufRead(PTPS_SUPER_BLOCK psb, TPS_IBLK inum, UINT uiOff,
                           PUCHAR pucBuff, size_t szLen);
                                                                        /* ͬ�����̻�����               */
TPS_RESULT tpsFsDevBufSync(PTPS_SUPER_BLOCK psb, TPS_IBLK blk, UINT uiOff, size_t szLen);
                                                                        /* �Դ���ִ��FIOTRIM����        */
TPS_RESULT tpsFsDevBufTrim(PTPS_SUPER_BLOCK psb, TPS_IBLK blkStart, TPS_IBLK blkCnt);

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TSPSFS_DEV_BUF_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
