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
** ��   ��   ��: diskCachePipeline.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 07 �� 25 ��
**
** ��        ��: ���̸��ٻ��岢��д����.
*********************************************************************************************************/

#ifndef __DISKCACHEPIPELINE_H
#define __DISKCACHEPIPELINE_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)

/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
INT   __diskCacheWpCreate(PLW_DISKCACHE_CB  pdiskc,
                          PLW_DISKCACHE_WP  pwp, 
                          BOOL              bCacheCoherence,
                          BOOL              bParallel,
                          INT               iPipeline, 
                          INT               iMsgCount,
                          INT               iMaxRBurstSector,
                          INT               iMaxWBurstSector,
                          ULONG             ulBytesPerSector);
INT   __diskCacheWpDelete(PLW_DISKCACHE_WP  pwp);
PVOID __diskCacheWpGetBuffer(PLW_DISKCACHE_WP  pwp, BOOL bRead);
VOID  __diskCacheWpPutBuffer(PLW_DISKCACHE_WP  pwp, PVOID  pvBuffer);
BOOL  __diskCacheWpSteal(PLW_DISKCACHE_CB  pdiskc,
                         PVOID             pvBuffer,
                         ULONG             ulSectorNo);
INT   __diskCacheWpRead(PLW_DISKCACHE_CB  pdiskc,
                        PLW_BLK_DEV       pblkd,
                        PVOID             pvBuffer,
                        ULONG             ulStartSector,
                        ULONG             ulNSector);
INT   __diskCacheWpWrite(PLW_DISKCACHE_CB  pdiskc,
                         PLW_BLK_DEV       pblkd,
                         PVOID             pvBuffer,
                         ULONG             ulStartSector,
                         ULONG             ulNSector);
VOID  __diskCacheWpSync(PLW_DISKCACHE_WP  pwp, ULONG  ulGetCnt);

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_DISKCACHE_EN > 0     */
#endif                                                                  /*  __DISKCACHEPIPELINE_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
