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
** ��   ��   ��: diskCache.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 11 �� 26 ��
**
** ��        ��: ���̸��ٻ��������.
*********************************************************************************************************/

#ifndef __DISKCACHE_H
#define __DISKCACHE_H

/*********************************************************************************************************
  ע��:
  
  ���ʹ�� CACHE, �뱣֤�������豸��һһ��Ӧ, (����߼�����������һ�� CACHE)
  
  DISK CACHE ��ʹ��:

  ʹ�� DISK CACHE �����Ŀ�ľ���������ٿ��豸 IO �ķ���Ч��, ����ϵͳ�ڴ���Ϊ���ݵĻ���, 
  ʹ�� DISK CACHE �������������ڴ��������뻺�����ݵĲ�ͬ����, ͬ�����ݿ��Ե��� ioctl(..., FIOFLUSH, ...)
  ʵ��.
  
  pblkDev = xxxBlkDevCreate(...);
  diskCacheCreate(pblkDev, ..., &pCacheBlk);
  ...(������ڶ���̷���, ���: diskPartition.h)
  fatFsDevCreate(pVolName, pCacheBlk);
  
  ...�����豸
  
  umount(pVolName);
  ...(������ڶ���̷���, ���: diskPartition.h)
  diskCacheDelete(pCacheBlk);
  xxxBlkDevDelete(pblkDev);
  
  �Ƽ�ʹ�� oem ���̲�����.
*********************************************************************************************************/

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)

/*********************************************************************************************************
  ioctl ��������
*********************************************************************************************************/

#define LW_BLKD_DISKCACHE_GET_OPT       LW_OSIOR('b', 150, INT)         /*  ��ȡ CACHE ѡ��             */
#define LW_BLKD_DISKCACHE_SET_OPT       LW_OSIOD('b', 151, INT)         /*  ���� CACHE ѡ��             */
#define LW_BLKD_DISKCACHE_INVALID       LW_OSIO( 'b', 152)              /*  ʹ CACHE ��д��ȫ��������   */
#define LW_BLKD_DISKCACHE_RAMFLUSH      LW_OSIOD('b', 153, ULONG)       /*  �����дһЩ������          */
#define LW_BLKD_DISKCACHE_CALLBACKFUNC  LW_OSIOD('b', 154, FUNCPTR)     /*  �ļ�ϵͳ�ص�����            */
#define LW_BLKD_DISKCACHE_CALLBACKARG   LW_OSIOD('b', 155, PVOID)       /*  �ص���������                */

/*********************************************************************************************************
  DISK CACHE ����
  
  ע��: 
    
    1. ���豸֧�ֲ�����д����ʱ, DCATTR_bParallel = LW_TRUE, ����Ϊ LW_FALSE
    2. DCATTR_iPipeline >= 0 && < LW_CFG_DISKCACHE_MAX_PIPELINE.
       DCATTR_iPipeline == 0 ��ʾ��ʹ�ù��߲�������.
    3. DCATTR_iMsgCount ��СΪ DCATTR_iPipeline, ����Ϊ DCATTR_iPipeline 2 ~ 8 ��.
*********************************************************************************************************/

#define LW_DCATTR_BOPT_CACHE_COHERENCE  0x01                            /*  ��������Ҫ CACHE һ���Ա��� */
#define LW_DCATTR_BOPT_PAGE_ALIGN       0x02                            /*  ����������ҳ�����          */

typedef struct {
    PVOID           DCATTR_pvCacheMem;                                  /*  ���������ַ                */
    size_t          DCATTR_stMemSize;                                   /*  ���������С                */
    INT             DCATTR_iBurstOpt;                                   /*  ��������                    */
    INT             DCATTR_iMaxRBurstSector;                            /*  ����⧷��������������      */
    INT             DCATTR_iMaxWBurstSector;                            /*  ����⧷�д�����������      */
    INT             DCATTR_iMsgCount;                                   /*  ������Ϣ���л������        */
    INT             DCATTR_iPipeline;                                   /*  ��������߳�����            */
    BOOL            DCATTR_bParallel;                                   /*  �Ƿ�֧�ֲ��ж�д            */
    ULONG           DCATTR_ulReserved[8];                               /*  ����                        */
} LW_DISKCACHE_ATTR;
typedef LW_DISKCACHE_ATTR  *PLW_DISKCACHE_ATTR;

/*********************************************************************************************************
  ǰ�����
*********************************************************************************************************/

#define DCATTR_bCacheCoherence      DCATTR_iBurstOpt                    /*  ��������Ҫ CACHE һ���Ա��� */

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API ULONG  API_DiskCacheCreate(PLW_BLK_DEV   pblkdDisk, 
                                  PVOID         pvDiskCacheMem, 
                                  size_t        stMemSize, 
                                  INT           iMaxBurstSector,
                                  PLW_BLK_DEV  *ppblkDiskCache);

LW_API ULONG  API_DiskCacheCreateEx(PLW_BLK_DEV   pblkdDisk, 
                                    PVOID         pvDiskCacheMem, 
                                    size_t        stMemSize, 
                                    INT           iMaxRBurstSector,
                                    INT           iMaxWBurstSector,
                                    PLW_BLK_DEV  *ppblkDiskCache);

LW_API ULONG  API_DiskCacheCreateEx2(PLW_BLK_DEV          pblkdDisk, 
                                     PLW_DISKCACHE_ATTR   pdcattrl,
                                     PLW_BLK_DEV         *ppblkDiskCache);

LW_API INT    API_DiskCacheDelete(PLW_BLK_DEV   pblkdDiskCache);

LW_API INT    API_DiskCacheSync(PLW_BLK_DEV   pblkdDiskCache);

#define diskCacheCreate     API_DiskCacheCreate
#define diskCacheCreateEx   API_DiskCacheCreateEx
#define diskCacheCreateEx2  API_DiskCacheCreateEx2
#define diskCacheDelete     API_DiskCacheDelete
#define diskCacheSync       API_DiskCacheSync

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
#endif                                                                  /*  __DISKCACHE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
