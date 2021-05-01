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
** ��   ��   ��: diskCache.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 11 �� 26 ��
**
** ��        ��: ���̸��ٻ��������.

** BUG
2008.12.03  ����ʱ����Ҫ��� iMaxBurstSector �Ĵ�С.
2009.03.11  �޸� hash ��Ĵ�С. ����.
2009.03.22  DISK CACHE ��ʼ��Ϊ�����������, ���� DISK CACHE �����������豸һһ��Ӧ.
2009.03.23  ����Ա�����д�����֧��.
2009.03.25  ����豸������Сǰ��Ҫ��ʼ������.
2009.11.03  ������ƶ��豸�Ĵ���.
2009.12.11  ���� /proc �ļ���ʼ��.
2014.07.29  ⧷���д����һ��ʹ��ҳ������ڴ����, ���������ɵײ� DMA �� CACHE һ���Բ�������(invalidate)
2016.01.30  ⧷���д������Էֱ�����.
2016.07.13  ��� HASH ����Ч��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
#include "diskCacheLib.h"
#include "diskCachePipeline.h"
#include "diskCache.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
extern LW_OBJECT_HANDLE   _G_ulDiskCacheListLock;
/*********************************************************************************************************
  ������
*********************************************************************************************************/
#define __LW_DISKCACHE_LIST_LOCK()      \
        API_SemaphoreMPend(_G_ulDiskCacheListLock, LW_OPTION_WAIT_INFINITE)
#define __LW_DISKCACHE_LIST_UNLOCK()    \
        API_SemaphoreMPost(_G_ulDiskCacheListLock)
/*********************************************************************************************************
  һ������ CF ��ʹ�� DISK CACHE �ṹʾ��:
  
CF card disk volume:
  +------------+
  | /media/hdd |
  |    TPSFS   |
  +------------+
        \
         \______________________
                                \
disk cache block device:         \
                           +------------+
                           | DISK CACHE |
                           |   BLK_DEV  |
                           +------------+
                                 \
                                  \________________________
                                                           \
physical block device:                                      \
                                                       +------------+
                                                       |  PHYSICAL  |
                                                       |   BLK_DEV  |
                                                       +------------+
*********************************************************************************************************/
/*********************************************************************************************************
  ��ز�����
*********************************************************************************************************/
static const INT    _G_iDiskCacheHashSize[][2] = {
/*********************************************************************************************************
    CACHE SIZE      HASH SIZE
     (sector)        (entry)
*********************************************************************************************************/
{         16,            8,         },
{         32,           16,         },
{         64,           32,         },
{        128,           64,         },
{        256,          128,         },
{        512,          256,         },
{       1024,          512,         },
{       2048,         1024,         },
{       4096,         2048,         },
{          0,         4096,         }
};
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
INT  __diskCacheMemInit(PLW_DISKCACHE_CB   pdiskcDiskCache,
                        VOID              *pvDiskCacheMem,
                        ULONG              ulBytesPerSector,
                        ULONG              ulNSector);
/*********************************************************************************************************
  ������������
*********************************************************************************************************/
INT  __diskCacheRead(PLW_DISKCACHE_CB   pdiskcDiskCache,
                     VOID              *pvBuffer, 
                     ULONG              ulStartSector, 
                     ULONG              ulSectorCount);
INT  __diskCacheWrite(PLW_DISKCACHE_CB   pdiskcDiskCache,
                      VOID              *pvBuffer, 
                      ULONG              ulStartSector, 
                      ULONG              ulSectorCount);
INT  __diskCacheIoctl(PLW_DISKCACHE_CB   pdiskcDiskCache, INT  iCmd, LONG  lArg);
INT  __diskCacheReset(PLW_DISKCACHE_CB   pdiskcDiskCache);
INT  __diskCacheStatusChk(PLW_DISKCACHE_CB   pdiskcDiskCache);
/*********************************************************************************************************
  proc ��ʼ����������
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
VOID  __procFsDiskCacheInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
** ��������: API_DiskCacheCreateEx2
** ��������: ����һ������ CACHE ���豸 (�ڶ����ӿ�).
** �䡡��  : pblkdDisk          ��Ҫ CACHE �Ŀ��豸
**           pdcattrl           ���� CACHE ������Ϣ
**           ppblkDiskCache     ���������� CACHE ���豸.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DiskCacheCreateEx2 (PLW_BLK_DEV          pblkdDisk, 
                               PLW_DISKCACHE_ATTR   pdcattrl,
                               PLW_BLK_DEV         *ppblkDiskCache)
{
             INT                i;
             INT                iError;
             INT                iErrLevel = 0;

    REGISTER PLW_DISKCACHE_CB   pdiskcDiskCache;
             ULONG              ulBytesPerSector;
             ULONG              ulNSector;
    REGISTER INT                iHashSize;
    
    /*
     *  ������д��
     */
    if (_G_ulDiskCacheListLock == LW_OBJECT_HANDLE_INVALID) {
        _G_ulDiskCacheListLock = API_SemaphoreMCreate("dcachelist_lock", 
                                                      LW_PRIO_DEF_CEILING,
                                                      LW_OPTION_WAIT_PRIORITY | 
                                                      LW_OPTION_DELETE_SAFE | 
                                                      LW_OPTION_INHERIT_PRIORITY |
                                                      LW_OPTION_OBJECT_GLOBAL,
                                                      LW_NULL);
        if (_G_ulDiskCacheListLock == LW_OBJECT_HANDLE_INVALID) {
            return  (API_GetLastError());
        }

        API_TSyncAdd(__diskCacheSync, LW_NULL);                         /*  ��ӱ�����д����            */

#if LW_CFG_PROCFS_EN > 0
        __procFsDiskCacheInit();                                        /*  ���� /proc �ļ��ڵ�         */
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
    }
    
    if (!pblkdDisk || !pdcattrl || !ppblkDiskCache) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (!pdcattrl->DCATTR_pvCacheMem       ||
        !pdcattrl->DCATTR_stMemSize        ||
        !pdcattrl->DCATTR_iMaxRBurstSector ||
        !pdcattrl->DCATTR_iMaxWBurstSector) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (pdcattrl->DCATTR_iPipeline < 0) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (pdcattrl->DCATTR_iPipeline > LW_CFG_DISKCACHE_MAX_PIPELINE) {
        pdcattrl->DCATTR_iPipeline = LW_CFG_DISKCACHE_MAX_PIPELINE;
    }
    
    if (pdcattrl->DCATTR_iMsgCount < pdcattrl->DCATTR_iPipeline) {
        pdcattrl->DCATTR_iMsgCount = pdcattrl->DCATTR_iPipeline;
    }
    
    if (pblkdDisk->BLKD_pfuncBlkIoctl &&
        pblkdDisk->BLKD_pfuncBlkReset) {                                /*  ��ʼ������                  */
        pblkdDisk->BLKD_pfuncBlkIoctl(pblkdDisk, LW_BLKD_CTRL_POWER, LW_BLKD_POWER_ON);
        pblkdDisk->BLKD_pfuncBlkReset(pblkdDisk);
        pblkdDisk->BLKD_pfuncBlkIoctl(pblkdDisk, FIODISKINIT, 0);
    }
    
    if (pblkdDisk->BLKD_ulBytesPerSector) {
        ulBytesPerSector = pblkdDisk->BLKD_ulBytesPerSector;            /*  ֱ�ӻ�ȡ������С            */
    
    } else if (pblkdDisk->BLKD_pfuncBlkIoctl) {
        if (pblkdDisk->BLKD_pfuncBlkIoctl(pblkdDisk, 
                                          LW_BLKD_GET_SECSIZE, 
                                          (LONG)&ulBytesPerSector)) {   /*  ͨ�� IOCTL ���������С     */
            return  (ERROR_IO_DEVICE_ERROR);
        }
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pblkdDisk ioctl error.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);                               /*  pblkdDisk ����              */
        return  (ERROR_IO_NO_DRIVER);
    }
    
    if (pdcattrl->DCATTR_stMemSize < ulBytesPerSector) {                /*  �޷�����һ������            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "cache buffer too small.\r\n");
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    ulNSector = pdcattrl->DCATTR_stMemSize / ulBytesPerSector;          /*  ���Ի������������          */
    
    /*
     *  ���ٿ��ƿ��ڴ�
     */
    pdiskcDiskCache = (PLW_DISKCACHE_CB)__SHEAP_ALLOC(sizeof(LW_DISKCACHE_CB));
    if (pdiskcDiskCache == LW_NULL) {
        iErrLevel = 1;
        goto    __error_handle;
    }
    lib_bzero(pdiskcDiskCache, sizeof(LW_DISKCACHE_CB));
    
    pdiskcDiskCache->DISKC_pblkdDisk = pblkdDisk;
    
    /*
     *  �������������ź���
     */
    pdiskcDiskCache->DISKC_hDiskCacheLock = API_SemaphoreMCreate("dcache_lock", 
                                            LW_PRIO_DEF_CEILING,
                                            LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                            LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                            LW_NULL);
    if (!pdiskcDiskCache->DISKC_hDiskCacheLock) {
        __SHEAP_FREE(pdiskcDiskCache);
        return  (API_GetLastError());
    }
    
    /*
     *  ��������д����
     */
    if (__diskCacheWpCreate(pdiskcDiskCache, 
                            &pdiskcDiskCache->DISKC_wpWrite,
                            pdcattrl->DCATTR_bParallel,
                            pdcattrl->DCATTR_iBurstOpt,
                            pdcattrl->DCATTR_iPipeline,
                            pdcattrl->DCATTR_iMsgCount,
                            pdcattrl->DCATTR_iMaxRBurstSector,
                            pdcattrl->DCATTR_iMaxWBurstSector,
                            ulBytesPerSector)) {
        iErrLevel = 2;
        goto    __error_handle;
    }
    
    pdiskcDiskCache->DISKC_ulEndStector     = (ULONG)PX_ERROR;
    pdiskcDiskCache->DISKC_ulBytesPerSector = ulBytesPerSector;
    pdiskcDiskCache->DISKC_iMaxRBurstSector = pdcattrl->DCATTR_iMaxRBurstSector;
    pdiskcDiskCache->DISKC_iMaxWBurstSector = pdcattrl->DCATTR_iMaxWBurstSector;
    
    /*
     *  ȷ�� HASH ��Ĵ�С
     */
    for (i = 0; ; i++) {
        if (_G_iDiskCacheHashSize[i][0] == 0) {
            iHashSize = _G_iDiskCacheHashSize[i][1];                    /*  �����С                  */
            break;
        
        } else {
            if (ulNSector >= _G_iDiskCacheHashSize[i][0]) {
                continue;
            
            } else {
                iHashSize = _G_iDiskCacheHashSize[i][1];                /*  ȷ��                        */
                break;
            }
        }
    }
    
    pdiskcDiskCache->DISKC_pplineHash = 
        (PLW_LIST_LINE *)__SHEAP_ALLOC(sizeof(PVOID) * (size_t)iHashSize);
    if (!pdiskcDiskCache->DISKC_pplineHash) {
        iErrLevel = 3;
        goto    __error_handle;
    }
    pdiskcDiskCache->DISKC_iHashSize = iHashSize;
    
    /*
     *  ���ٻ����ڴ��
     */
    iError = __diskCacheMemInit(pdiskcDiskCache, pdcattrl->DCATTR_pvCacheMem, 
                                ulBytesPerSector, ulNSector);
    if (iError < 0) {
        iErrLevel = 4;
        goto    __error_handle;
    }
    pdiskcDiskCache->DISKC_disknLuck = LW_NULL;
    
    /*
     *  ��ʼ�� BLK_DEV ������Ϣ
     */
    pdiskcDiskCache->DISKC_blkdCache = *pdiskcDiskCache->DISKC_pblkdDisk;
                                                                        /*  ������Ҫͬ������Ϣ          */
    pdiskcDiskCache->DISKC_blkdCache.BLKD_pfuncBlkRd        = __diskCacheRead;
    pdiskcDiskCache->DISKC_blkdCache.BLKD_pfuncBlkWrt       = __diskCacheWrite;
    pdiskcDiskCache->DISKC_blkdCache.BLKD_pfuncBlkIoctl     = __diskCacheIoctl;
    pdiskcDiskCache->DISKC_blkdCache.BLKD_pfuncBlkReset     = __diskCacheReset;
    pdiskcDiskCache->DISKC_blkdCache.BLKD_pfuncBlkStatusChk = __diskCacheStatusChk;
    
    pdiskcDiskCache->DISKC_blkdCache.BLKD_iLogic        = 0;            /*  CACHE �������豸һһ��Ӧ    */
    pdiskcDiskCache->DISKC_blkdCache.BLKD_uiLinkCounter = 0;
    pdiskcDiskCache->DISKC_blkdCache.BLKD_pvLink        = (PVOID)pdiskcDiskCache->DISKC_pblkdDisk;
    
    pdiskcDiskCache->DISKC_blkdCache.BLKD_uiPowerCounter = 0;           /*  must be 0                   */
    pdiskcDiskCache->DISKC_blkdCache.BLKD_uiInitCounter  = 0;
    
    __diskCacheListAdd(pdiskcDiskCache);                                /*  ���뱳���߳�                */
    
    *ppblkDiskCache = (PLW_BLK_DEV)pdiskcDiskCache;                     /*  ������ƿ��ַ              */
    
    return  (ERROR_NONE);
    
__error_handle:
    if (iErrLevel > 3) {
        __SHEAP_FREE(pdiskcDiskCache->DISKC_pplineHash);
    }
    if (iErrLevel > 2) {
        __diskCacheWpDelete(&pdiskcDiskCache->DISKC_wpWrite);
    }
    if (iErrLevel > 1) {
        API_SemaphoreMDelete(&pdiskcDiskCache->DISKC_hDiskCacheLock);
        __SHEAP_FREE(pdiskcDiskCache);
    }
    
    _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
    _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
    return  (ERROR_SYSTEM_LOW_MEMORY);
}
/*********************************************************************************************************
** ��������: API_DiskCacheCreateEx
** ��������: ����һ������ CACHE ���豸.
** �䡡��  : pblkdDisk          ��Ҫ CACHE �Ŀ��豸
**           pvDiskCacheMem     ���� CACHE ���������ڴ���ʼ��ַ
**           stMemSize          ���� CACHE ��������С
**           iMaxRBurstSector   ����⧷��������������
**           iMaxWBurstSector   ����⧷�д�����������
**           ppblkDiskCache     ���������� CACHE ���豸.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DiskCacheCreateEx (PLW_BLK_DEV   pblkdDisk, 
                              PVOID         pvDiskCacheMem, 
                              size_t        stMemSize, 
                              INT           iMaxRBurstSector,
                              INT           iMaxWBurstSector,
                              PLW_BLK_DEV  *ppblkDiskCache)
{
    LW_DISKCACHE_ATTR   dcattrl;
    
    dcattrl.DCATTR_pvCacheMem       = pvDiskCacheMem;
    dcattrl.DCATTR_stMemSize        = stMemSize;
    dcattrl.DCATTR_iBurstOpt        = 0;
    dcattrl.DCATTR_iMaxRBurstSector = iMaxRBurstSector;
    dcattrl.DCATTR_iMaxWBurstSector = iMaxWBurstSector;
    dcattrl.DCATTR_iMsgCount        = 4;
    dcattrl.DCATTR_iPipeline        = 1;
    dcattrl.DCATTR_bParallel        = LW_FALSE;

    return  (API_DiskCacheCreateEx2(pblkdDisk, &dcattrl, ppblkDiskCache));
}
/*********************************************************************************************************
** ��������: API_DiskCacheCreate
** ��������: ����һ������ CACHE ���豸.
** �䡡��  : pblkdDisk          ��Ҫ CACHE �Ŀ��豸
**           pvDiskCacheMem     ���� CACHE ���������ڴ���ʼ��ַ
**           stMemSize          ���� CACHE ��������С
**           iMaxBurstSector    ����⧷���д�����������
**           ppblkDiskCache     ���������� CACHE ���豸.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DiskCacheCreate (PLW_BLK_DEV   pblkdDisk, 
                            PVOID         pvDiskCacheMem, 
                            size_t        stMemSize, 
                            INT           iMaxBurstSector,
                            PLW_BLK_DEV  *ppblkDiskCache)
{
    INT   iMaxRBurstSector;
    
    if (iMaxBurstSector > 2) {
        iMaxRBurstSector = iMaxBurstSector >> 1;                        /*  ��⧷�����Ĭ�ϱ�д��һ��    */
    
    } else {
        iMaxRBurstSector = iMaxBurstSector;
    }
    
    return  (API_DiskCacheCreateEx(pblkdDisk, 
                                   pvDiskCacheMem, 
                                   stMemSize, 
                                   iMaxRBurstSector,
                                   iMaxBurstSector,
                                   ppblkDiskCache));
}
/*********************************************************************************************************
** ��������: API_DiskCacheDelete
** ��������: ɾ��һ������ CACHE ���豸.
** �䡡��  : pblkdDiskCache     ���� CACHE �Ŀ��豸
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ɾ������֮ǰ����Ҫʹ�� remove ����ж�ؾ�, 
             ����:
                    BLK_DEV   *pblkdCf;
                    BLK_DEV   *pblkdCfCache;
                
                    cfDiskCreate(..., &pblkdCf);
                    diskCacheCreate(pblkdCf, ..., &pblkdCfCache)
                    fatFsDevCreate("/cf0", pblkdCfCache);
                    ...
                    unlink("/cf0");
                    diskCacheDelete(pblkdCfCache);
                    cfDiskCreate(pblkdCf);
                    
** ע  ��  : ������ ListDel �� DISKCACHE_LOCK ��ֹ����.

                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_DiskCacheDelete (PLW_BLK_DEV   pblkdDiskCache)
{
    REGISTER PLW_DISKCACHE_CB   pdiskcDiskCache = (PLW_DISKCACHE_CB)pblkdDiskCache;

    if (pblkdDiskCache) {
        __diskCacheListDel(pdiskcDiskCache);                            /*  �˳������߳�                */

        __LW_DISKCACHE_LOCK(pdiskcDiskCache);                           /*  �ȴ�ʹ��Ȩ                  */
        __diskCacheWpSync(&pdiskcDiskCache->DISKC_wpWrite, 0);
        __diskCacheWpDelete(&pdiskcDiskCache->DISKC_wpWrite);
        
        API_SemaphoreMDelete(&pdiskcDiskCache->DISKC_hDiskCacheLock);
        __SHEAP_FREE(pdiskcDiskCache->DISKC_pcCacheNodeMem);
        __SHEAP_FREE(pdiskcDiskCache->DISKC_pplineHash);
        __SHEAP_FREE(pdiskcDiskCache);
        
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_DiskCacheSync
** ��������: ���̸��ٻ����������д
** �䡡��  : pblkdDiskCache     ���� CACHE �Ŀ��豸 (NULL Ϊ��д���и��ٻ��������)
** �䡡��  : NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_API 
INT  API_DiskCacheSync (PLW_BLK_DEV   pblkdDiskCache)
{
    REGISTER PLW_DISKCACHE_CB   pdiskcDiskCache = (PLW_DISKCACHE_CB)pblkdDiskCache;
             PLW_LIST_LINE      plineCache;

    if (pdiskcDiskCache) {
        pdiskcDiskCache->DISKC_blkdCache.BLKD_pfuncBlkIoctl(pdiskcDiskCache, FIOSYNC, 0);
    
    } else if (_G_ulDiskCacheListLock) {
        __LW_DISKCACHE_LIST_LOCK();
        for (plineCache  = _G_plineDiskCacheHeader;
             plineCache != LW_NULL;
             plineCache  = _list_line_get_next(plineCache)) {
            
            pdiskcDiskCache = _LIST_ENTRY(plineCache, 
                                          LW_DISKCACHE_CB, 
                                          DISKC_lineManage);
            pdiskcDiskCache->DISKC_blkdCache.BLKD_pfuncBlkIoctl(pdiskcDiskCache, FIOSYNC, 0);
        }
        __LW_DISKCACHE_LIST_UNLOCK();
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
