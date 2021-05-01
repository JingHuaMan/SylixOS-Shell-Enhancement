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
** ��   ��   ��: diskRaidLib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 06 �� 08 ��
**
** ��        ��: ��� RAID �������й����ڲ���.
**
** ע        ��: ����������������̲���������ȫһ��, ������̴�С, ������С�Ȳ���������ͬ.
*********************************************************************************************************/

#ifndef __DISKRAIDLIB_H
#define __DISKRAIDLIB_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKRAID_EN > 0)

INT  __diskRaidBytesPerSector(PLW_BLK_DEV  pblkd, ULONG  *pulBytesPerSector);
INT  __diskRaidBytesPerBlock(PLW_BLK_DEV  pblkd, ULONG  *pulBytesPerBlock);
INT  __diskRaidTotalSector(PLW_BLK_DEV  pblkd, ULONG  *pulTotalSector);
INT  __diskRaidCheck(PLW_BLK_DEV   pblkd[],
                     UINT          uiNDisks,
                     ULONG        *pulBytesPerSector,
                     ULONG        *pulBytesPerBlock,
                     ULONG        *pulTotalSector);

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKRAID_EN > 0)    */
#endif                                                                  /*  __DISKRAIDLIB_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
