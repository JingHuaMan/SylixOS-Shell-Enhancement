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
** ��   ��   ��: diskRaid.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 06 �� 06 ��
**
** ��        ��: ��� RAID �������й���.
*********************************************************************************************************/

#ifndef __DISKRAID_H
#define __DISKRAID_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKRAID_EN > 0)

LW_API ULONG    API_DiskRiad0Create(PLW_BLK_DEV  pblkd[],
                                    UINT         uiNDisks,
                                    size_t       stStripe,
                                    PLW_BLK_DEV *ppblkDiskRaid);

LW_API INT      API_DiskRiad0Delete(PLW_BLK_DEV  pblkDiskRaid);

LW_API ULONG    API_DiskRiad1Create(PLW_BLK_DEV  pblkd[],
                                    UINT         uiNDisks,
                                    PLW_BLK_DEV *ppblkDiskRaid);

LW_API INT      API_DiskRiad1Delete(PLW_BLK_DEV  pblkDiskRaid);

LW_API ULONG    API_DiskRiad1Ghost(PLW_BLK_DEV  pblkDest,
                                   PLW_BLK_DEV  pblkSrc,
                                   ULONG        ulStartSector,
                                   ULONG        ulSectorNum);

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKRAID_EN > 0)    */
#endif                                                                  /*  __DISKRAID_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
