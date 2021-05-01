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
** ��   ��   ��: diskRaidLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 06 �� 08 ��
**
** ��        ��: ��� RAID �������й����ڲ���.
**
** ע        ��: ����������������̲���������ȫһ��, ������̴�С, ������С�Ȳ���������ͬ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKRAID_EN > 0)
/*********************************************************************************************************
** ��������: __diskRaidBytesPerSector
** ��������: ��ô���ÿ�����ֽ���
** �䡡��  : pblkd              RAID ������
**           pulBytesPerSector  ��ȡ�Ĵ���ÿ�����ֽ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __diskRaidBytesPerSector (PLW_BLK_DEV  pblkd, ULONG  *pulBytesPerSector)
{
    if (pblkd->BLKD_ulBytesPerSector) {
        *pulBytesPerSector = pblkd->BLKD_ulBytesPerSector;
    } else {
        if (!pblkd->BLKD_pfuncBlkIoctl) {
            return  (PX_ERROR);
        }
        if (pblkd->BLKD_pfuncBlkIoctl(pblkd, LW_BLKD_GET_SECSIZE, pulBytesPerSector) < 0) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __diskRaidBytesPerBlock
** ��������: ��ô���ÿ���ֽ���
** �䡡��  : pblkd              RAID ������
**           pulBytesPerBlock   ��ȡ�Ĵ���ÿ�����ֽ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __diskRaidBytesPerBlock (PLW_BLK_DEV  pblkd, ULONG  *pulBytesPerBlock)
{
    if (pblkd->BLKD_ulBytesPerBlock) {
        *pulBytesPerBlock = pblkd->BLKD_ulBytesPerBlock;
    } else {
        if (!pblkd->BLKD_pfuncBlkIoctl) {
            return  (PX_ERROR);
        }
        if (pblkd->BLKD_pfuncBlkIoctl(pblkd, LW_BLKD_GET_BLKSIZE, pulBytesPerBlock) < 0) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __diskRaidTotalSector
** ��������: ��ô�����������
** �䡡��  : pblkd              RAID ������
**           pulTotalSector     ��ȡ�Ĵ�����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __diskRaidTotalSector (PLW_BLK_DEV  pblkd, ULONG  *pulTotalSector)
{
    if (pblkd->BLKD_ulNSector) {
        *pulTotalSector = pblkd->BLKD_ulNSector;
    } else {
        if (!pblkd->BLKD_pfuncBlkIoctl) {
            return  (PX_ERROR);
        }
        if (pblkd->BLKD_pfuncBlkIoctl(pblkd, LW_BLKD_GET_SECNUM, pulTotalSector) < 0) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __diskRaidCheck
** ��������: ���һ������Ƿ����� RAID ���е�����
** �䡡��  : pblkd              RAID ������
**           uiNDisks           ��������
**           pulBytesPerSector  ��ȡ�Ĵ���ÿ�����ֽ���
**           pulBytesPerBlock   ��ȡ�Ĵ���ÿ���ֽ���
**           pulTotalSector     ��ȡ�Ĵ�����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __diskRaidCheck (PLW_BLK_DEV   pblkd[],
                      UINT          uiNDisks,
                      ULONG        *pulBytesPerSector,
                      ULONG        *pulBytesPerBlock,
                      ULONG        *pulTotalSector)
{
    INT    i;
    ULONG  ulBytes0, ulTotal0, ulBlkSize;
    ULONG  ulBytes1, ulTotal1;

    if (__diskRaidBytesPerSector(pblkd[0], &ulBytes0)) {
        return  (PX_ERROR);
    }

    if (__diskRaidTotalSector(pblkd[0], &ulTotal0)) {
        return  (PX_ERROR);
    }

    if (__diskRaidBytesPerBlock(pblkd[0], &ulBlkSize)) {
        return  (PX_ERROR);
    }

    for (i = 1; i < uiNDisks; i++) {
        if (__diskRaidBytesPerSector(pblkd[i], &ulBytes1)) {
            return  (PX_ERROR);
        }

        if (__diskRaidTotalSector(pblkd[i], &ulTotal1)) {
            return  (PX_ERROR);
        }

        if ((ulBytes0 != ulBytes1) || (ulTotal0 != ulTotal1)) {
            return  (PX_ERROR);
        }
    }

    if (pulBytesPerSector) {
        *pulBytesPerSector = ulBytes0;
    }

    if (pulBytesPerBlock) {
        *pulBytesPerBlock = ulBlkSize;
    }

    if (pulTotalSector) {
        *pulTotalSector = ulTotal0;
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKRAID_EN > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
