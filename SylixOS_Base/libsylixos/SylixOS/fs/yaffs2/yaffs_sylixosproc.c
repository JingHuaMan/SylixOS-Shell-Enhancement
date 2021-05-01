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
2010.01.07  �޸�һЩ errno.
2010.03.10  ���� yaffs �޸���ؽӿ�.
2010.07.06  ���� yaffs �޸���ؽӿ�.
2010.08.11  ���� read ����.
2011.03.04  proc �ļ� mode Ϊ S_IFREG
            �����������ʣ�೤�ȴ���.
2011.03.13  ���� yaffs �ļ�ϵͳ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_YAFFS_DRV
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_YAFFS_EN > 0) && (LW_CFG_PROCFS_EN > 0)
/*********************************************************************************************************
  yaffs proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsYaffsRead(PLW_PROCFS_NODE  p_pfsn, 
                                  PCHAR            pcBuffer, 
                                  size_t           stMaxBytes,
                                  off_t            oft);
/*********************************************************************************************************
  yaffs proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoYaffsFuncs = {
    __procFsYaffsRead, LW_NULL
};
/*********************************************************************************************************
  yaffs proc �ļ�Ŀ¼��
*********************************************************************************************************/
#define __PROCFS_BUFFER_SIZE_YAFFS      4096

static LW_PROCFS_NODE           _G_pfsnYaffs[] = 
{
    LW_PROCFS_INIT_NODE("yaffs",  (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoYaffsFuncs, "Y", __PROCFS_BUFFER_SIZE_YAFFS),
};
/*********************************************************************************************************
** ��������: __procFsYaffsPrint
** ��������: ��ӡ yaffs ��Ϣ
** �䡡��  : pyaffsDev     yaffs �豸
**           pcVolName     �豸��
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           stOffset      ƫ����
** �䡡��  : �µ�ƫ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static size_t  __procFsYaffsPrint (struct yaffs_dev *pyaffsDev, 
                                   PCHAR             pcVolName,
                                   PCHAR             pcBuffer, 
                                   size_t            stMaxBytes,
                                   size_t            stOffset)
{
    return  (bnprintf(pcBuffer, stMaxBytes, stOffset,
                      "Device : \"%s\"\n"
                      "startBlock......... %d\n"
                      "endBlock........... %d\n"
                      "totalBytesPerChunk. %d\n"
                      "chunkGroupBits..... %d\n"
                      "chunkGroupSize..... %d\n"
                      "nErasedBlocks...... %d\n"
                      "nReservedBlocks.... %d\n"
                      "nCheckptResBlocks.. nil\n"
                      "blocksInCheckpoint. %d\n"
                      "nObjects........... %d\n"
                      "nTnodes............ %d\n"
                      "nFreeChunks........ %d\n"
                      "nPageWrites........ %d\n"
                      "nPageReads......... %d\n"
                      "nBlockErasures..... %d\n"
                      "nErasureFailures... %d\n"
                      "nGCCopies.......... %d\n"
                      "allGCs............. %d\n"
                      "passiveGCs......... %d\n"
                      "nRetriedWrites..... %d\n"
                      "nShortOpCaches..... %d\n"
                      "nRetiredBlocks..... %d\n"
                      "eccFixed........... %d\n"
                      "eccUnfixed......... %d\n"
                      "tagsEccFixed....... %d\n"
                      "tagsEccUnfixed..... %d\n"
                      "cacheHits.......... %d\n"
                      "nDeletedFiles...... %d\n"
                      "nUnlinkedFiles..... %d\n"
                      "nBackgroudDeletions %d\n"
                      "useNANDECC......... %d\n"
                      "isYaffs2........... %d\n\n",
                      
                      pcVolName,
                      pyaffsDev->param.start_block,
                      pyaffsDev->param.end_block,
                      pyaffsDev->param.total_bytes_per_chunk,
                      pyaffsDev->chunk_grp_bits,
                      pyaffsDev->chunk_grp_size,
                      pyaffsDev->n_erased_blocks,
                      pyaffsDev->param.n_reserved_blocks,
                      pyaffsDev->blocks_in_checkpt,
                      pyaffsDev->n_obj,
                      pyaffsDev->n_tnodes,
                      pyaffsDev->n_free_chunks,
                      pyaffsDev->n_page_writes,
                      pyaffsDev->n_page_reads,
                      pyaffsDev->n_erasures,
                      pyaffsDev->n_erase_failures,
                      pyaffsDev->n_gc_copies,
                      pyaffsDev->all_gcs,
                      pyaffsDev->passive_gc_count,
                      pyaffsDev->n_retried_writes,
                      pyaffsDev->param.n_caches,
                      pyaffsDev->n_retired_blocks,
                      pyaffsDev->n_ecc_fixed,
                      pyaffsDev->n_ecc_unfixed,
                      pyaffsDev->n_tags_ecc_fixed,
                      pyaffsDev->n_tags_ecc_unfixed,
                      pyaffsDev->cache_hits,
                      pyaffsDev->n_deleted_files,
                      pyaffsDev->n_unlinked_files,
                      pyaffsDev->n_bg_deletions,
                      pyaffsDev->param.use_nand_ecc,
                      pyaffsDev->param.is_yaffs2));
}
/*********************************************************************************************************
** ��������: __procFsYaffsRead
** ��������: procfs ��һ�� yaffs �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsYaffsRead (PLW_PROCFS_NODE  p_pfsn, 
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
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        /*
         *  �����������ļ�������
         */
        INT                 i = 0;
        PCHAR               pcVolName;
        struct yaffs_dev   *pyaffsDev;                                  /*  yaffs �����豸              */
        
        do {
            pcVolName = yaffs_getdevname(i, &i);
            if (pcVolName == LW_NULL) {
                break;                                                  /*  û�������ľ�                */
            }
            pyaffsDev = (struct yaffs_dev *)yaffs_getdev(pcVolName);
            if (pyaffsDev == LW_NULL) {
                break;                                                  /*  �޷���ѯ���豸�ڵ�          */
            }
            
            /*
             *  (__PROCFS_BUFFER_SIZE_YAFFS - stRealSize) Ϊ�ļ�������ʣ��Ŀռ��С
             */
            stRealSize = __procFsYaffsPrint(pyaffsDev, 
                                            pcVolName,
                                            pcFileBuffer,
                                            __PROCFS_BUFFER_SIZE_YAFFS,
                                            stRealSize);
        } while (i != PX_ERROR);
    
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
** ��������: __procFsYaffsInit
** ��������: procfs ��ʼ�� yaffs proc �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsYaffsInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnYaffs[0], "/");
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
                                                                        /*  (LW_CFG_PROCFS_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
