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
** ��   ��   ��: yaffs_sylixosproc.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 11 ��
**
** ��        ��: yaffs �� /proc �е���Ϣ.

** BUG:
2010.08.11  ���� read ����.
2011.03.04  proc �ļ� mode Ϊ S_IFREG.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
#include "diskCacheLib.h"
#include "diskCache.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0) && (LW_CFG_PROCFS_EN > 0)
/*********************************************************************************************************
  ������
*********************************************************************************************************/
#define __LW_DISKCACHE_LIST_LOCK()      \
        API_SemaphoreMPend(_G_ulDiskCacheListLock, LW_OPTION_WAIT_INFINITE)
#define __LW_DISKCACHE_LIST_UNLOCK()    \
        API_SemaphoreMPost(_G_ulDiskCacheListLock)
/*********************************************************************************************************
  diskcache proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsDiskCacheRead(PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft);
/*********************************************************************************************************
  diskcache proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoDiskCacheFuncs = {
    __procFsDiskCacheRead, LW_NULL
};
/*********************************************************************************************************
  diskcache proc �ļ�Ŀ¼��
*********************************************************************************************************/
#define __PROCFS_BUFFER_SIZE_DISKCACHE  2048

static LW_PROCFS_NODE           _G_pfsnDiskCache[] = 
{
    LW_PROCFS_INIT_NODE("diskcache",  (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoDiskCacheFuncs, "D", 
                        __PROCFS_BUFFER_SIZE_DISKCACHE),
};
/*********************************************************************************************************
** ��������: __procFsDiskCachePrint
** ��������: ��ӡ diskcache ��Ϣ
** �䡡��  : pcBuffer      ������
**           stMaxBytes    ��������С
** �䡡��  : ʵ�����ɵ���Ϣ��С
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static size_t  __procFsDiskCachePrint (PCHAR  pcBuffer, size_t  stMaxBytes)
{
    size_t              stRealSize;
    PCHAR               pcName;
    PLW_LIST_LINE       plineTemp;
    PLW_DISKCACHE_CB    pdiskcDiskCache;
    
    stRealSize = bnprintf(pcBuffer, stMaxBytes, 0, "DO NOT INCLUDE 'NAND' READ CACHE INFO.\n\n");
    stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize,
                          "       NAME       OPT SECTOR-SIZE TOTAL-SECs VALID-SECs DIRTY-SECs BURST-R BURST-W  HASH\n"
                          "----------------- --- ----------- ---------- ---------- ---------- ------- ------- ------\n");
                           
    __LW_DISKCACHE_LIST_LOCK();
    for (plineTemp  = _G_plineDiskCacheHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pdiskcDiskCache = _LIST_ENTRY(plineTemp, 
                                      LW_DISKCACHE_CB, 
                                      DISKC_lineManage);
                                      
        if ((pdiskcDiskCache->DISKC_blkdCache.BLKD_pcName) &&
            (pdiskcDiskCache->DISKC_blkdCache.BLKD_pcName[0] != PX_EOS)) {
            pcName = pdiskcDiskCache->DISKC_blkdCache.BLKD_pcName;
        
        } else {
            pcName = "<unkown>";
        }
                                      
        stRealSize = bnprintf(pcBuffer, stMaxBytes, stRealSize,
                              "%-17s %3d %11lu %10lu %10lu %10lu %7d %7d %6d\n",
                              pcName,
                              pdiskcDiskCache->DISKC_iCacheOpt,
                              pdiskcDiskCache->DISKC_ulBytesPerSector,
                              pdiskcDiskCache->DISKC_ulNCacheNode,
                              pdiskcDiskCache->DISKC_ulValidCounter,
                              pdiskcDiskCache->DISKC_ulDirtyCounter,
                              pdiskcDiskCache->DISKC_iMaxRBurstSector,
                              pdiskcDiskCache->DISKC_iMaxWBurstSector,
                              pdiskcDiskCache->DISKC_iHashSize);
        
    }
    __LW_DISKCACHE_LIST_UNLOCK();
    
    return  (stRealSize);
}
/*********************************************************************************************************
** ��������: __procFsDiskCacheRead
** ��������: procfs ��һ�� diskcache �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsDiskCacheRead (PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;

    /*
     *  �������е�����, �ļ�����һ���Ѿ�������Ԥ�õ��ڴ��С.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        /*
         *  �����������ļ�������
         */
        stRealSize = __procFsDiskCachePrint(pcFileBuffer, __PROCFS_BUFFER_SIZE_DISKCACHE);
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);              /*  ��д�ļ�ʵ�ʴ�С            */
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  ����ʵ�ʿ������ֽ���        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: __procFsDiskCacheInit
** ��������: procfs ��ʼ�� diskcache proc �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsDiskCacheInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnDiskCache[0], "/");
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
                                                                        /*  (LW_CFG_PROCFS_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
