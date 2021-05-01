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
** ��   ��   ��: yaffs_sylixos.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 07 ��
**
** ��        ��: yaffs �����ϵͳ���ݺ���.
**
** ˵        ��: ���е� yaffs ��������һ�� yaffs �豸��, ������ fat ��һ���豸һ����.

** BUG
2008.12.19  ���� format ����. ���Ч��.
2009.02.18  ������ɾ��Ŀ¼�Ĵ���.
2009.03.14  ���ļ�ϵͳ�����Ļ�ȡ֧�� 4GB ���ϵľ�. (__yaffsStatfsGet)
2009.04.07  format ������Ӱ�ȫ.
2009.04.22  ��ж�ؾ�ʱ, ���ر������ļ�, ���ǽ��й��ļ���Ϊ�쳣״̬.
2009.06.05  �����ļ����Ᵽ��, ��һ���ļ���д��ʽ��, �����ܱ���. ���ļ�������ʽ��, ���ܱ���������
            �ķ�ʽ��. 
2009.06.10  �����ļ�ʱ��ʱ, Ҫ����ļ��Ŀɶ�д��.
2009.06.18  �����ļ����жϵ�����.
2009.06.30  ���봴���ļ���������ж�.
2009.07.05  ʹ�� fsCommon �ṩ�ĺ��������ļ����ж�.
2009.07.09  ֧�� 64 bit �ļ�ָ��.
2009.07.12  ֧�ִ򿪾��ļ����и�ʽ��.
            __yaffsFormat() ͨ��ɨ���豸��ֱ��ȷ���豸.
2009.08.08  __yaffsTimeset() ���� errno ����.
2009.08.26  readdir ʱ, ��Ҫ�� d_shortname ����, ��ʾ��֧�ֶ��ļ���.
2009.08.29  readdir ʱ, �����г� yaffs �豸�����йҽӵľ�.
2009.09.28  ���� yaffs �ļ�ϵͳ, ʹ�� yaffs_fsync() �滻 yaffs_flush().
2009.10.22  read write ӦΪ ssize_t ����.
2009.11.18  __yaffsStatGet() ȷ����Ŀ¼�����豸��ʱ, �����Ի�� stat.
2009.12.11  �� /proc �м��� yaffs �ļ���ʾ���������Ϣ.
2010.01.11  �����������ļ�����Ҫ��ѹ��.
2010.01.14  ֧�� FIOSETFL ����.
2010.03.10  ���� yaffs �޸���ؽӿ�.
2010.07.06  ���� yaffs ��ʼ���ӿڵ���(�뵱ǰ�汾һ�� API_YaffsDrvInstall)
2010.09.03  __yaffsRemove() ж�� yaffs ���ʱ, ��Ҫ���� yaffs_uninstalldev() ɾ��ָ���ľ�.
                            ж������ yaffs io �豸ʱ, ��Ҫ yaffs_unmount() ���й��ڵľ�.
2010.09.08  ������Ҫ���� __yaffsRemove() ʱ�Ƿ���Ҫ���� yaffs_unmount() ����.
2010.09.09  ���� d_type ����.
2011.03.13  ���� yaffs �ļ�ϵͳ.
2011.03.22  ���� yaffs һЩ�ײ�����.
2011.03.27  ����Դ����ļ�����֧������ж�.
2011.03.29  �� iosDevAddEx() ����ʱ��Ҫ���� iosDevAddEx() �Ĵ����.
2011.07.24  yaffs ���ٿ�������ʹ�ö����ڴ�ѹ���, ������ʹ��ϵͳ�ڴ��.
2011.08.07  ���� yaffs �Է������ӵ�֧��. �� yaffscfg.c ������������ SylixOS ��Ҫ�ķ������Ӻ���.
2011.08.11  ѹ��Ŀ¼�������ļ�ϵͳ���, ����ϵͳ�Ѵ�����.
2012.03.10  open ����ʱ, Ӧ�����ж������ļ����, Ȼ���ٴ�.
2012.03.11  yaffs ֱ��ʹ��ϵͳʱ��.
2012.03.31  yaffs ֧�� fdatasync ����.
2012.08.16  ֧�� pread �� pwrite.  
2012.09.01  ���벻��ǿ��ж�ؾ�ı���.
2012.09.25  yaffs ���Դ��� socket �����ļ�.
2012.11.09  __yaffsFsReadlink() ��Ҫ������Ч�ֽ���.
2013.01.06  yaffs ʹ������ NEW_1 �豸��������. ��������֧���ļ���.
2013.07.10  ���� yaffs ��, ֧��ͨ�� fd ����Ŀ¼, ���ﲻ�ٱ�������Ŀ¼�Ĵ���ʽ.
2013.07.12  ���� yaffs ��, ����ʹ����ǰ�ĸ�ʽ������, ת��ʹ�� yaffs �Դ��� yaffs_format �������и�ʽ��.
2017.04.27  yaffs �ڲ�ʹ�� O_RDWR ��, ��ֹ���ش�ʱ�ڲ�Ȩ���жϴ���.
2020.06.11  ���뱳����д����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_YAFFS_DRV
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_YAFFS_EN > 0)
/*********************************************************************************************************
  �ڲ�ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE _G_hYaffsOpLock;
static INT              _G_iYaffsDrvNum = PX_ERROR;
static BOOL             _G_bIsCreateDev = LW_FALSE;
static PCHAR            _G_pcDevName    = LW_NULL;
/*********************************************************************************************************
  �ڲ��ṹ
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          YAFFS_devhdrHdr;                                /*  yaffs �ļ�ϵͳ�豸ͷ        */
    BOOL                YAFFS_bForceDelete;
    LW_LIST_LINE_HEADER YAFFS_plineFdNodeHeader;                        /*  fd_node ����                */
} YAFFS_FSLIB;
typedef YAFFS_FSLIB    *PYAFFS_FSLIB;

typedef struct {
    PYAFFS_FSLIB        YAFFIL_pyaffs;                                  /*  ָ�� yaffs �豸             */
    INT                 YAFFIL_iFd;                                     /*  yaffs �ļ�������            */
    INT                 YAFFIL_iFileType;                               /*  �ļ�����                    */
    CHAR                YAFFIL_cName[1];                                /*  �ļ���                      */
} YAFFS_FILE;
typedef YAFFS_FILE     *PYAFFS_FILE;
/*********************************************************************************************************
  �ļ�����
*********************************************************************************************************/
#define __YAFFS_FILE_TYPE_NODE          0                               /*  open ���ļ�               */
#define __YAFFS_FILE_TYPE_DIR           1                               /*  open ��Ŀ¼               */
#define __YAFFS_FILE_TYPE_DEV           2                               /*  open ���豸               */
/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
#define __STR_IS_YVOL(pcName)           (lib_rindex(pcName, PX_DIVIDER) == pcName)
/*********************************************************************************************************
  �ڲ�������
*********************************************************************************************************/
#define __YAFFS_OPLOCK()                API_SemaphoreMPend(_G_hYaffsOpLock, LW_OPTION_WAIT_INFINITE)
#define __YAFFS_OPUNLOCK()              API_SemaphoreMPost(_G_hYaffsOpLock)
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
static LONG     __yaffsOpen(PYAFFS_FSLIB    pyaffs,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __yaffsRemove(PYAFFS_FSLIB    pyaffs,
                            PCHAR           pcName);
static INT      __yaffsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __yaffsRead(PLW_FD_ENTRY    pfdentry,
                            PCHAR           pcBuffer, 
                            size_t          stMaxBytes);
static ssize_t  __yaffsPRead(PLW_FD_ENTRY    pfdentry,
                             PCHAR           pcBuffer, 
                             size_t          stMaxBytes,
                             off_t          oftPos);
static ssize_t  __yaffsWrite(PLW_FD_ENTRY  pfdentry,
                             PCHAR         pcBuffer, 
                             size_t        stNBytes);
static ssize_t  __yaffsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer, 
                              size_t        stNBytes,
                              off_t          oftPos);
static INT      __yaffsLStatGet(PYAFFS_FSLIB  pyaffs, 
                                PCHAR         pcName, 
                                struct stat  *pstat);
static INT      __yaffsIoctl(PLW_FD_ENTRY  pfdentry,
                             INT           iRequest,
                             LONG          lArg);
static INT      __yaffsFlush(PLW_FD_ENTRY   pfdentry);
static INT      __yaffsDataSync(PLW_FD_ENTRY  pfdentry);
static INT      __yaffsFsSymlink(PYAFFS_FSLIB  pyaffs, 
                                 PCHAR         pcName, 
                                 CPCHAR        pcLinkDst);
static ssize_t __yaffsFsReadlink(PYAFFS_FSLIB  pyaffs, 
                                 PCHAR         pcName,
                                 PCHAR         pcLinkDst,
                                 size_t        stMaxSize);
/*********************************************************************************************************
  yaffs proc ��Ϣ
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
VOID  __procFsYaffsInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
** ��������: __yaffsOsInit
** ��������: yaffs �ļ�ϵͳ����ϵͳ�ӿڳ�ʼ��.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __yaffsOsInit (VOID)
{
    _G_hYaffsOpLock = API_SemaphoreMCreate("yaffs_oplock", LW_PRIO_DEF_CEILING, 
                                         LW_OPTION_WAIT_PRIORITY | LW_OPTION_INHERIT_PRIORITY | 
                                         LW_OPTION_DELETE_SAFE | LW_OPTION_OBJECT_GLOBAL,
                                         LW_NULL);
    if (!_G_hYaffsOpLock) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "yaffs oplock can not create.\r\n");
        return;
    }
}
/*********************************************************************************************************
** ��������: __tshellYaffsCmd
** ��������: yaffs �ļ�ϵͳ�ײ�����.
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellYaffsCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    INT                  i, force;
    struct yaffs_dev    *pyaffsDev;
    
    if (iArgC < 3) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    __YAFFS_OPLOCK();
    pyaffsDev = (struct yaffs_dev *)yaffs_getdev(ppcArgV[1]);
    if (pyaffsDev == LW_NULL) {                                         /*  ���� nand �豸              */
        __YAFFS_OPUNLOCK();
        fprintf(stderr, "can not find yaffs device!\n");                /*  �޷��������豸              */
        return  (PX_ERROR);
    }
    
    if (lib_strcmp("bad", ppcArgV[2]) == 0) {                           /*  ��ӡ������Ϣ                */
        for (i = pyaffsDev->param.start_block; i <= pyaffsDev->param.end_block; i++) {
            enum yaffs_block_state  state = YAFFS_BLOCK_STATE_UNKNOWN;
            u32                     sequenceNumber;
            yaffs_query_init_block_state(pyaffsDev, i, &state, &sequenceNumber);
            if (state == YAFFS_BLOCK_STATE_DEAD) {
                printf("block 0x%x is bad block.\n", i);
            }
        }
    
    } else if (lib_strcmp("info", ppcArgV[2]) == 0) {                   /*  ��ӡ����Ϣ                  */
        printf("Device : \"%s\"\n"
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
               pyaffsDev->param.name,
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
               pyaffsDev->param.is_yaffs2);
    
    } else if (lib_strcmp("markbad", ppcArgV[2]) == 0) {                /*  ��ָ���Ŀ���Ϊ����          */
        int     iBlock = -1;
        
        if (iArgC < 4) {
            __YAFFS_OPUNLOCK();
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        sscanf(ppcArgV[3], "%x", &iBlock);                              /*  ��ȡ��ı��                */
        
        if (yaffs_mark_bad(pyaffsDev, iBlock) == YAFFS_OK) {
            printf("mark the block 0x%x is a bad ok.\n", iBlock);
        } else {
            __YAFFS_OPUNLOCK();
            printf("mark the block 0x%x is a bad error!\n", iBlock);
            return  (PX_ERROR);
        }
    
    } else if (lib_strcmp("erase", ppcArgV[2]) == 0) {                  /*  ����оƬ (���������Ĺ���)   */
        printf("Erasing...\n");

        for (i = pyaffsDev->param.start_block; i <= pyaffsDev->param.end_block; i++) {
            printf("\r%4d / %4d", i, pyaffsDev->param.end_block);
            fflush(stdout);
            yaffs_erase_block(pyaffsDev, i);
        }
        
        printf("\nyaffs volume erase ok\n");

    } else if (lib_strcmp("format", ppcArgV[2]) == 0) {                 /*  ��ʽ��оƬ                  */
        if (iArgC > 3 && ((ppcArgV[3][0] == 'f') || (ppcArgV[3][0] == 'F'))) {
            force = 1;
        } else {
            force = 0;
        }

        if (yaffs_format_reldev(pyaffsDev, 1, force, 1) == 0) {
            printf("yaffs volume format ok.\n");
        } else {
            printf("yaffs volume format error: %s\n", lib_strerror(errno));
        }
    }
    __YAFFS_OPUNLOCK();
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: API_YaffsDrvInstall
** ��������: ��װ yaffs �ļ�ϵͳ��������
** �䡡��  : 
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_YaffsDrvInstall (VOID)
{
    struct file_operations     fileop;
    
    if (_G_iYaffsDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __yaffsOpen;
    fileop.fo_release  = __yaffsRemove;
    fileop.fo_open     = __yaffsOpen;
    fileop.fo_close    = __yaffsClose;
    fileop.fo_read     = __yaffsRead;
    fileop.fo_read_ex  = __yaffsPRead;
    fileop.fo_write    = __yaffsWrite;
    fileop.fo_write_ex = __yaffsPWrite;
    fileop.fo_lstat    = __yaffsLStatGet;
    fileop.fo_ioctl    = __yaffsIoctl;
    fileop.fo_symlink  = __yaffsFsSymlink;
    fileop.fo_readlink = __yaffsFsReadlink;
    
    _G_iYaffsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);     /*  ���� NEW_1 ����             */
    
    DRIVER_LICENSE(_G_iYaffsDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iYaffsDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iYaffsDrvNum, "yaffs2 driver.");
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "yaffs2 file system installed.\r\n");
    yaffs_start_up();                                                   /*  ��ʼ�� YAFFS                */

#if LW_CFG_PROCFS_EN > 0
    __procFsYaffsInit();                                                /*  ���� proc �еĽڵ�          */
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("yaffscmd", __tshellYaffsCmd);
    API_TShellFormatAdd("yaffscmd", " volname [{bad | info | markbad | erase | format}]");
    API_TShellHelpAdd("yaffscmd", "eg. yaffscmd n0 bad         show volume \"n0\" bad block.\n"
                                  "    yaffscmd n0 info        show volume \"n0\" information.\n"
                                  "    yaffscmd n0 markbad 3a  mark block 0x3a is a bad block.\n"
                                  "    yaffscmd n1 erase       erase volume \"n1\"\n"
                                  "    yaffscmd n1 format      format volume \"n1\"\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

    return  ((_G_iYaffsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: API_YaffsDevCreate
** ��������: ���� Yaffs �豸. ����Ҫ����һ��, ���� yaffs ��ȫ���ҽ�������豸��.
**           �� sylixos FAT ��ͬ, yaffs ÿһ�����ǹҽ���Ψһ�� yaffs ���豸��.
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_YaffsDevCreate (PCHAR   pcName)
{    
    REGISTER PYAFFS_FSLIB    pyaffs;

    if (_G_iYaffsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no yaffs driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (_G_bIsCreateDev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is another yaffs device.\r\n");
        _ErrorHandle(ERROR_IO_FILE_EXIST);
        return  (PX_ERROR);
    }
    
    pyaffs = (PYAFFS_FSLIB)__SHEAP_ALLOC(sizeof(YAFFS_FSLIB));
    if (pyaffs == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pyaffs, sizeof(YAFFS_FSLIB));
    
    pyaffs->YAFFS_bForceDelete      = LW_FALSE;
    pyaffs->YAFFS_plineFdNodeHeader = LW_NULL;
    
    if (iosDevAddEx(&pyaffs->YAFFS_devhdrHdr, pcName, _G_iYaffsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  ��װ�ļ�ϵͳ�豸            */
        __SHEAP_FREE(pyaffs);
        return  (PX_ERROR);
    }

    _G_bIsCreateDev = LW_TRUE;                                          /*  ���� yaffs �豸�ɹ�         */
    _G_pcDevName    = pyaffs->YAFFS_devhdrHdr.DEVHDR_pcName;
    
    API_TSyncAdd(API_YaffsDevSync, LW_NULL);                            /*  ���뱳����д����            */

    _DebugFormat(__LOGMESSAGE_LEVEL, "yaffs \"%s\" has been create.\r\n", pcName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_YaffsDevDelete
** ��������: ɾ��һ�� yaffs �����豸, ����: API_YaffsDevDelete("/yaffs2/n0");
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_YaffsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  ������豸, �����ж���豸  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_YaffsDevSync
** ��������: ��ʾ���� yaffs �����豸
** �䡡��  : pcName   ���豸��, NULL ��ʾȫ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_YaffsDevSync (PCHAR  pcName)
{
    INT     iIndex = 0;
    INT     iNext;

    if (_G_pcDevName) {
        __YAFFS_OPLOCK();
        if (pcName) {
            yaffs_sync(pcName);

        } else {
            do {
                pcName = yaffs_getdevname(iIndex, &iNext);
                if (pcName) {
                    iIndex = iNext;
                    yaffs_sync(pcName);
                }
            } while (pcName);
        }
        __YAFFS_OPUNLOCK();
    }
}
/*********************************************************************************************************
** ��������: API_YaffsDevMountShow
** ��������: ��ʾ���� yaffs �����豸
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_YaffsDevMountShow (VOID)
{
    PCHAR   pcMountInfoHdr = "       VOLUME                    BLK NAME\n"
                             "-------------------- --------------------------------\n";
    PCHAR   pcMtdName;
    CHAR    cVolPath[MAX_FILENAME_LENGTH];
    INT     iIndex = 0;
    INT     iNext;
    
    printf("MTD-Mount point show >>\n");
    printf(pcMountInfoHdr);                                             /*  ��ӡ��ӭ��Ϣ                */
    
    if (_G_pcDevName) {
        do {
            pcMtdName = yaffs_getdevname(iIndex, &iNext);
            if (pcMtdName) {
                iIndex = iNext;
                snprintf(cVolPath, sizeof(cVolPath), "%s%s", _G_pcDevName, pcMtdName);
                printf("%-20s MTD:%s\n", cVolPath, pcMtdName);
            }
        } while (pcMtdName);
    }
}
/*********************************************************************************************************
** ��������: __yaffsCloseFile
** ��������: yaffs �ڲ��ر�һ���ļ�
** �䡡��  : pyaffile         yaffs �ļ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __yaffsCloseFile (PYAFFS_FILE   pyaffile)
{
    if (pyaffile) {
        if ((pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) ||
            (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_DIR)) {
            yaffs_close(pyaffile->YAFFIL_iFd);                          /*  ��׼ yaffs �ļ�             */
        }
    }
}
/*********************************************************************************************************
** ��������: __yaffsOpen
** ��������: yaffs open ����
** �䡡��  : pyaffs           �豸
**           pcName           �ļ���
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  __yaffsOpen (PYAFFS_FSLIB    pyaffs,
                          PCHAR           pcName,
                          INT             iFlags,
                          INT             iMode)
{
             INT            iError;
    REGISTER PYAFFS_FILE    pyaffile;
             PLW_FD_NODE    pfdnode;
             BOOL           bIsNew;
    struct yaffs_stat       yafstat;

    if (pcName == LW_NULL) {                                            /*  ���ļ���                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {                                         /*  ��������                    */
            if (__fsCheckFileName(pcName)) {
                _ErrorHandle(ENOENT);
                return  (PX_ERROR);
            }
            if (S_ISFIFO(iMode) || 
                S_ISBLK(iMode)  ||
                S_ISCHR(iMode)) {
                _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                /*  ��֧��������Щ��ʽ          */
                return  (PX_ERROR);
            }
        }
        
        pyaffile = (PYAFFS_FILE)__SHEAP_ALLOC(sizeof(YAFFS_FILE) + 
                                              lib_strlen(pcName));      /*  �����ļ��ڴ�                */
        if (pyaffile == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_strcpy(pyaffile->YAFFIL_cName, pcName);                     /*  ��¼�ļ���                  */
        
        pyaffile->YAFFIL_pyaffs = pyaffs;
        
        __YAFFS_OPLOCK();
        /*
         *  ���ȴ�����������ļ����
         */
        {
            INT     iFollowLinkType;
            PCHAR   pcTail, pcSymfile, pcPrefix;
            struct yaffs_obj *obj = yaffsfs_FindSymlink(pcName, &pcTail, &pcSymfile);
                                                                        /*  ���Ŀ¼�Ƿ���� symlink    */
            if (obj) {                                                  /*  Ŀ¼���� symlink            */
                pcSymfile--;                                            /*  �� / ��ʼ                   */
                if (pcSymfile == pcName) {
                    pcPrefix = LW_NULL;                                 /*  û��ǰ׺                    */
                } else {
                    pcPrefix = pcName;
                    *pcSymfile = PX_EOS;
                }
                if (pcTail && lib_strlen(pcTail)) {
                    iFollowLinkType = FOLLOW_LINK_TAIL;                 /*  ����Ŀ���ڲ��ļ�            */
                } else {
                    iFollowLinkType = FOLLOW_LINK_FILE;                 /*  �����ļ�����                */
                }
                
                if (yaffsfs_PathBuildLink(pcName, pyaffs->YAFFS_devhdrHdr.DEVHDR_pcName,
                                          pcPrefix, obj, pcTail) == ERROR_NONE) {
                                                                        /*  ��������Ŀ��                */
                    __YAFFS_OPUNLOCK();
                    __SHEAP_FREE(pyaffile);
                    return  (iFollowLinkType);
                
                } else {
                    __YAFFS_OPUNLOCK();
                    __SHEAP_FREE(pyaffile);
                    return  (PX_ERROR);
                }
            }
        }
        
        /*
         *  �Ƿ��������ļ�.
         */
        if ((iFlags & O_CREAT) && S_ISDIR(iMode)) {                     /*  ����Ŀ¼                    */
            iError = yaffs_mkdir(pcName, iMode);
            if ((iError != ERROR_NONE) && (iFlags & O_EXCL)) {
                __YAFFS_OPUNLOCK();
                __SHEAP_FREE(pyaffile);
                return  (PX_ERROR);
            }
            iError = yaffs_open(pcName, O_RDONLY, iMode);               /*  ��Ŀ¼                    */
        
        } else {                                                        /*  �򿪻򴴽��ļ�              */
            iError = yaffs_open(pcName, (iFlags & ~O_BINARY), iMode);   /*  ����ͨ�ļ�                */
        }                                                               /*  yaffs ��Ŀ¼�����д�ѡ��  */

        pyaffile->YAFFIL_iFd = iError;
        pyaffile->YAFFIL_iFileType = __YAFFS_FILE_TYPE_NODE;            /*  ���Ա� close ��             */
        
        if (iError < ERROR_NONE) {                                      /*  ��������                    */
            if (__STR_IS_ROOT(pyaffile->YAFFIL_cName)) {                /*  �Ƿ�Ϊ yaffs �ļ�ϵͳ�豸   */
                yafstat.st_dev  = (int)(long)pyaffile->YAFFIL_pyaffs;   /*  Fixed warning in 64bits     */
                yafstat.st_ino  = (int)0;                               /*  ��������ͨ�ļ��ظ�(��Ŀ¼)  */
                yafstat.st_mode = YAFFS_ROOT_MODE | S_IFDIR;
                yafstat.st_uid  = 0;
                yafstat.st_gid  = 0;
                yafstat.st_size = 0;
                pyaffile->YAFFIL_iFileType = __YAFFS_FILE_TYPE_DEV;
                goto    __file_open_ok;                                 /*  �豸������                */
            }
            __YAFFS_OPUNLOCK();
            __SHEAP_FREE(pyaffile);
            return  (PX_ERROR);                                         /*  ��ʧ��                    */
        
        } else {
            yaffs_fstat(pyaffile->YAFFIL_iFd, &yafstat);                /*  ����ļ�����                */
            if (S_ISDIR(yafstat.st_mode)) {
                pyaffile->YAFFIL_iFileType = __YAFFS_FILE_TYPE_DIR;     /*  Ŀ¼                        */
            } else {
                yaffs_handle_rw_set(pyaffile->YAFFIL_iFd, 1, 1);        /*  ӵ�ж�дȨ��                */
            }
        }
        
__file_open_ok:
        pfdnode = API_IosFdNodeAdd(&pyaffs->YAFFS_plineFdNodeHeader,
                                   (dev_t)yafstat.st_dev,
                                   (ino64_t)yafstat.st_ino,
                                   iFlags,
                                   iMode,
                                   yafstat.st_uid,
                                   yafstat.st_gid,
                                   yafstat.st_size,
                                   (PVOID)pyaffile,
                                   &bIsNew);                            /*  ����ļ��ڵ�                */
        if (pfdnode == LW_NULL) {                                       /*  �޷����� fd_node �ڵ�       */
            __yaffsCloseFile(pyaffile);
            __YAFFS_OPUNLOCK();
            __SHEAP_FREE(pyaffile);
            return  (PX_ERROR);
        }
        
        LW_DEV_INC_USE_COUNT(&pyaffs->YAFFS_devhdrHdr);                 /*  ���¼�����                  */
        
        if (bIsNew == LW_FALSE) {                                       /*  ���ظ���                  */
            __yaffsCloseFile(pyaffile);
            __YAFFS_OPUNLOCK();
            __SHEAP_FREE(pyaffile);
        
        } else {
            __YAFFS_OPUNLOCK();
        }
        
        return  ((LONG)pfdnode);
    }
}
/*********************************************************************************************************
** ��������: __yaffsRemove
** ��������: yaffs remove ����
** �䡡��  : pyaffs           �豸
**           pcName           �ļ���
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsRemove (PYAFFS_FSLIB    pyaffs,
                           PCHAR           pcName)
{
    REGISTER INT    iError;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    } else {
    
        __YAFFS_OPLOCK();
        
        if (__STR_IS_ROOT(pcName)) {                                    /*  ��Ŀ¼���� yaffs �豸�ļ�   */
__re_umount_vol:
            if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pyaffs)) {           /*  ����Ƿ������ڹ������ļ�    */
                if (!pyaffs->YAFFS_bForceDelete) {
                    _ErrorHandle(EBUSY);
                    return  (PX_ERROR);
                }
                
                __YAFFS_OPUNLOCK();
                
                _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
                iosDevFileAbnormal(&pyaffs->YAFFS_devhdrHdr);           /*  ����������ļ���Ϊ�쳣ģʽ  */
            
                __YAFFS_OPLOCK();
                goto    __re_umount_vol;
            }
            
            iosDevDelete((LW_DEV_HDR *)pyaffs);                         /*  IO ϵͳ�Ƴ��豸             */
            _G_bIsCreateDev = LW_FALSE;                                 /*  ж�� YAFFS �豸             */
            
#if LW_CFG_YAFFS_UNMOUNT_VOL > 0
            {
                INT                iIndex   = 0;
                PCHAR              pcDelVol = LW_NULL;
                struct yaffs_dev  *pyaffsdev;
                
                /*
                 *  ж�����й��صľ� (yaffs ��Ҫ update check point)
                 */
                do {
                    pcDelVol = yaffs_getdevname(iIndex, &iIndex);       /*  �����һ������              */
                    if (pcDelVol) {
                        iError = yaffs_unmount(pcDelVol);               /*  ж���豸                    */
                        if (iError >= 0) {
                            pyaffsdev = (struct yaffs_dev *)yaffs_getdev(pcDelVol);
                            if (pyaffsdev) {
                                yaffs_remove_device(pyaffsdev);         /*  ���豸����ɾ���豸          */
                            }
                        }
                    }
                } while (pcDelVol);
            }
#endif                                                                  /*  LW_CFG_YAFFS_UNMOUNT_VOL > 0*/
            
            __YAFFS_OPUNLOCK();
            __SHEAP_FREE(pyaffs);
            
            _DebugHandle(__LOGMESSAGE_LEVEL, "yaffs unmount ok.\r\n");
            
            return  (ERROR_NONE);
        
        } else if (__STR_IS_YVOL(pcName)) {                             /*  yaffs ���ص��豸            */
            struct yaffs_dev  *pyaffsdev;
            
            iError = yaffs_unmount(pcName);                             /*  ж���豸                    */
            if (iError >= 0) {
                pyaffsdev = (struct yaffs_dev *)yaffs_getdev(pcName);
                if (pyaffsdev) {
                    yaffs_remove_device(pyaffsdev);                     /*  ���豸����ɾ���豸          */
                }
            }
        
            __YAFFS_OPUNLOCK();
            if (iError < ERROR_NONE) {
                return  (iError);
            }
            
            _DebugFormat(__LOGMESSAGE_LEVEL, "yaffs volume \"%s\" unmount ok.\r\n", pcName);
            
            return  (ERROR_NONE);
        
        } else {
            struct yaffs_stat   yaffsstat;
            
            iError = yaffs_lstat(pcName, &yaffsstat);
            if (iError >= 0) {
                if (S_ISDIR(yaffsstat.st_mode)) {                       /*  Ŀ¼�ļ�                    */
                    iError = yaffs_rmdir(pcName);
                    
                } else {
                    iError = yaffs_unlink(pcName);                      /*  ɾ���ļ�                    */
                }
                
            } else {                                                    /*  ����������ļ�, tailһ������*/
                PCHAR   pcTail, pcSymfile, pcPrefix;
                struct yaffs_obj *obj = yaffsfs_FindSymlink(pcName, &pcTail, &pcSymfile);
                                                                        /*  ���Ŀ¼�Ƿ���� symlink    */
                if (obj) {                                              /*  Ŀ¼���� symlink            */
                    pcSymfile--;                                        /*  �� / ��ʼ                   */
                    if (pcSymfile == pcName) {
                        pcPrefix = LW_NULL;                             /*  û��ǰ׺                    */
                    } else {
                        pcPrefix = pcName;
                        *pcSymfile = PX_EOS;
                    }
                    if (yaffsfs_PathBuildLink(pcName, pyaffs->YAFFS_devhdrHdr.DEVHDR_pcName,
                                              pcPrefix, obj, pcTail) == ERROR_NONE) {
                                                                        /*  ��������Ŀ��                */
                        __YAFFS_OPUNLOCK();
                        return  (FOLLOW_LINK_TAIL);                     /*  һ�����������ļ�����        */
                    }
                }
            }
            __YAFFS_OPUNLOCK();
            
            return  (iError);
        }
    }
}
/*********************************************************************************************************
** ��������: __yaffsClose
** ��������: yaffs close ����
** �䡡��  : pfdentry         �ļ����ƿ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    PYAFFS_FSLIB  pyaffs   = pyaffile->YAFFIL_pyaffs;
    BOOL          bFree    = LW_FALSE;
    BOOL          bRemove  = LW_FALSE;

    if (pyaffile) {
        __YAFFS_OPLOCK();
        if (API_IosFdNodeDec(&pyaffs->YAFFS_plineFdNodeHeader,
                             pfdnode, &bRemove) == 0) {                 /*  fd_node �Ƿ���ȫ�ͷ�        */
            __yaffsCloseFile(pyaffile);
            bFree = LW_TRUE;
        }
        
        LW_DEV_DEC_USE_COUNT(&pyaffs->YAFFS_devhdrHdr);
        
        if (bRemove) {
            if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
                yaffs_unlink(pyaffile->YAFFIL_cName);
            
            } else if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_DIR) {
                yaffs_rmdir(pyaffile->YAFFIL_cName);
            }
        }
        
        __YAFFS_OPUNLOCK();
        
        if (bFree) {
            __SHEAP_FREE(pyaffile);
        }
        
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __yaffsRead
** ��������: yaffs read ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __yaffsRead (PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer, 
                             size_t         stMaxBytes)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile   = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    ssize_t       sstReadNum = PX_ERROR;
             
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType != __YAFFS_FILE_TYPE_NODE) {
        __YAFFS_OPUNLOCK();
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    sstReadNum = (ssize_t)yaffs_pread(pyaffile->YAFFIL_iFd,
                                      (PVOID)pcBuffer, (unsigned int)stMaxBytes, 
                                      pfdentry->FDENTRY_oftPtr);
    if (sstReadNum > 0) {
        pfdentry->FDENTRY_oftPtr += (off_t)sstReadNum;                  /*  �����ļ�ָ��                */
    }
    __YAFFS_OPUNLOCK();
    
    return  (sstReadNum);
}
/*********************************************************************************************************
** ��������: __yaffsPRead
** ��������: yaffs pread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __yaffsPRead (PLW_FD_ENTRY   pfdentry,
                              PCHAR          pcBuffer, 
                              size_t         stMaxBytes,
                              off_t          oftPos)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile   = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    ssize_t       sstReadNum = PX_ERROR;
             
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType != __YAFFS_FILE_TYPE_NODE) {
        __YAFFS_OPUNLOCK();
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    sstReadNum = (ssize_t)yaffs_pread(pyaffile->YAFFIL_iFd,
                                      (PVOID)pcBuffer, (unsigned int)stMaxBytes, 
                                      oftPos);
    __YAFFS_OPUNLOCK();
    
    return  (sstReadNum);
}
/*********************************************************************************************************
** ��������: __yaffsWrite
** ��������: yaffs write ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __yaffsWrite (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer, 
                              size_t        stNBytes)
{
    PLW_FD_NODE   pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile    = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    ssize_t       sstWriteNum = PX_ERROR;
    
    
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType != __YAFFS_FILE_TYPE_NODE) {
        __YAFFS_OPUNLOCK();
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  ׷��ģʽ                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  �ƶ���дָ�뵽ĩβ          */
    }
    
    sstWriteNum = (ssize_t)yaffs_pwrite(pyaffile->YAFFIL_iFd,
                                        (CPVOID)pcBuffer, (unsigned int)stNBytes,
                                        pfdentry->FDENTRY_oftPtr);
    if (sstWriteNum > 0) {
        struct yaffs_stat   yafstat;
        pfdentry->FDENTRY_oftPtr += (off_t)sstWriteNum;                 /*  �����ļ�ָ��                */
        yaffs_fstat(pyaffile->YAFFIL_iFd, &yafstat);
        pfdnode->FDNODE_oftSize = yafstat.st_size;                      /*  �����ļ���С                */
    }
    __YAFFS_OPUNLOCK();
    
    if (sstWriteNum >= 0) {
        if (pfdentry->FDENTRY_iFlag & O_SYNC) {                         /*  ��Ҫ����ͬ��                */
            __yaffsFlush(pfdentry);
        
        } else if (pfdentry->FDENTRY_iFlag & O_DSYNC) {
            __yaffsDataSync(pfdentry);
        }
    }
    
    return  (sstWriteNum);
}
/*********************************************************************************************************
** ��������: __yaffsPWrite
** ��������: yaffs pwrite ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __yaffsPWrite (PLW_FD_ENTRY  pfdentry,
                               PCHAR         pcBuffer, 
                               size_t        stNBytes,
                               off_t         oftPos)
{
    PLW_FD_NODE   pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile    = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    ssize_t       sstWriteNum = PX_ERROR;
    
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType != __YAFFS_FILE_TYPE_NODE) {
        __YAFFS_OPUNLOCK();
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    sstWriteNum = (ssize_t)yaffs_pwrite(pyaffile->YAFFIL_iFd,
                                        (CPVOID)pcBuffer, (unsigned int)stNBytes,
                                        oftPos);
    if (sstWriteNum > 0) {
        struct yaffs_stat   yafstat;
        yaffs_fstat(pyaffile->YAFFIL_iFd, &yafstat);
        pfdnode->FDNODE_oftSize = yafstat.st_size;                      /*  �����ļ���С                */
    }
    __YAFFS_OPUNLOCK();
    
    if (sstWriteNum >= 0) {
        if (pfdentry->FDENTRY_iFlag & O_SYNC) {                         /*  ��Ҫ����ͬ��                */
            __yaffsFlush(pfdentry);
        
        } else if (pfdentry->FDENTRY_iFlag & O_DSYNC) {
            __yaffsDataSync(pfdentry);
        }
    }
    
    return  (sstWriteNum);
}
/*********************************************************************************************************
** ��������: __yaffsSeek
** ��������: yaffs seek ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           oftOffset        ƫ����
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsSeek (PLW_FD_ENTRY  pfdentry,
                         off_t         oftOffset)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    INT           iError   = ERROR_NONE;
    ULONG         ulError  = ERROR_NONE;

    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
        off_t   oftRet = yaffs_lseek(pyaffile->YAFFIL_iFd, oftOffset, SEEK_SET);
        if (oftRet != oftOffset) {
            iError = PX_ERROR;
        } else {
            pfdentry->FDENTRY_oftPtr = oftOffset;
        }
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __YAFFS_OPUNLOCK();
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsWhere
** ��������: yaffs ����ļ���ǰ��дָ��λ�� (ʹ�ò�����Ϊ����ֵ, �� FIOWHERE ��Ҫ�����в�ͬ)
** �䡡��  : pfdentry            �ļ����ƿ�
**           poftPos             ��дָ��λ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __yaffsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    __YAFFS_OPLOCK();
    if (poftPos) {
        *poftPos = (off_t)pfdentry->FDENTRY_oftPtr;
    }
    __YAFFS_OPUNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __yaffsNRead
** ��������: yaffs ����ļ�ʣ���ֽ���
** �䡡��  : pfdentry            �ļ����ƿ�
**           piPos               ����ʣ��������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __yaffsNRead (PLW_FD_ENTRY  pfdentry, INT  *piPos)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    INT           iError   = ERROR_NONE;
    ULONG         ulError  = ERROR_NONE;
    
    if (piPos == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
        *piPos = (INT)(pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr);
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __YAFFS_OPUNLOCK();
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsNRead64
** ��������: yaffs ����ļ�ʣ���ֽ���
** �䡡��  : pfdentry            �ļ����ƿ�
**           poftPos             ����ʣ��������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __yaffsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    INT           iError   = ERROR_NONE;
    ULONG         ulError  = ERROR_NONE;
    
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
        *poftPos = (pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr);
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __YAFFS_OPUNLOCK();
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsFlush
** ��������: yaffs flush ����
** �䡡��  : pfdentry            �ļ����ƿ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsFlush (PLW_FD_ENTRY  pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    INT           iRet;

    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
        iRet = yaffs_fsync(pyaffile->YAFFIL_iFd);
    
    } else {
        yaffs_sync(pyaffile->YAFFIL_cName);
        iRet = ERROR_NONE;
    }
    __YAFFS_OPUNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __yaffsDataSync
** ��������: yaffs data sync ����
** �䡡��  : pfdentry            �ļ����ƿ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsDataSync (PLW_FD_ENTRY  pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE   pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    INT           iRet;

    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
        iRet = yaffs_fdatasync(pyaffile->YAFFIL_iFd);
    
    } else {
        yaffs_sync(pyaffile->YAFFIL_cName);
        iRet = ERROR_NONE;
    }
    __YAFFS_OPUNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __yaffsFormat
** ��������: yaffs mkfs ����
** �䡡��  : pfdentry            �ļ����ƿ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsFormat (PLW_FD_ENTRY  pfdentry)
{
    INT                  iError;
    PLW_FD_NODE          pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE          pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    struct yaffs_dev    *pyaffsDev;
    
    __YAFFS_OPLOCK();
    pyaffsDev = (struct yaffs_dev *)yaffs_getdev(pyaffile->YAFFIL_cName);
    if (pyaffsDev == LW_NULL) {                                         /*  ���� nand �豸              */
        __YAFFS_OPUNLOCK();
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);                       /*  �޷��������豸              */
        return  (PX_ERROR);
    }
    
    __yaffsCloseFile(pyaffile);                                         /*  ��ǰ�ر��ļ�                */
    
    pyaffile->YAFFIL_iFileType = __YAFFS_FILE_TYPE_DEV;                 /*  �ļ��Ѿ���ǰ�ر�, ��Ϊ�豸  */
    
    yaffs_sync(pyaffile->YAFFIL_cName);                                 /*  ͬ�� yaffs ��               */
    
    iError = yaffs_format(pyaffile->YAFFIL_cName, 1, 0, 1);             /*  format �����¹���           */
    __YAFFS_OPUNLOCK();

    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsRename
** ��������: yaffs rename ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcNewName        ���ļ���
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsRename (PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
    REGISTER INT            iError  = PX_ERROR;
    
             CHAR           cNewPath[PATH_MAX + 1];
    REGISTER PCHAR          pcNewPath = &cNewPath[0];
             PLW_FD_NODE    pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
             PYAFFS_FILE    pyaffile  = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
             PYAFFS_FSLIB   pyaffsNew;

    if (__STR_IS_ROOT(pyaffile->YAFFIL_cName)) {                        /*  ����Ƿ�Ϊ�豸�ļ�          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  ��֧���豸������            */
        return  (PX_ERROR);
    }
    if (pcNewName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    if (__STR_IS_ROOT(pcNewName)) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    __YAFFS_OPLOCK();
    if ((pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) ||
        (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_DIR)) {        /*  open ��������ͨ�ļ���Ŀ¼   */
        if (ioFullFileNameGet(pcNewName, 
                              (LW_DEV_HDR **)&pyaffsNew, 
                              cNewPath) != ERROR_NONE) {                /*  �����Ŀ¼·��              */
            __YAFFS_OPUNLOCK();
            return  (PX_ERROR);
        }
        if (pyaffsNew != pyaffile->YAFFIL_pyaffs) {                     /*  ����Ϊͬһ�� yaffs �豸�ڵ� */
            __YAFFS_OPUNLOCK();
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
        
        if (cNewPath[0] == PX_DIVIDER) {
            pcNewPath++;
        }
        
        iError = yaffs_rename(pyaffile->YAFFIL_cName, pcNewPath);       /*  ������                      */
    } else {
        _ErrorHandle(ENOSYS);
    }
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsStatGet
** ��������: yaffs stat ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pstat            �ļ�״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsStatGet (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    INT                 iError;
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE         pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    struct yaffs_stat   yaffsstat;

    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __YAFFS_OPLOCK();
    iError = yaffs_stat(pyaffile->YAFFIL_cName, &yaffsstat);
    if (iError == ERROR_NONE) {
        pstat->st_dev     = (dev_t)yaffsstat.st_dev;                    /* device                       */
        pstat->st_ino     = yaffsstat.st_ino;                           /* inode                        */
        pstat->st_mode    = yaffsstat.st_mode;                          /* protection                   */
        pstat->st_nlink   = yaffsstat.st_nlink;                         /* number of hard links         */
        pstat->st_uid     = (uid_t)yaffsstat.st_uid;                    /* user ID of owner             */
        pstat->st_gid     = (gid_t)yaffsstat.st_gid;                    /* group ID of owner            */
        pstat->st_rdev    = (dev_t)yaffsstat.st_rdev;                   /* device type (if inode device)*/
        pstat->st_size    = yaffsstat.st_size;                          /* total size, in bytes         */
        pstat->st_blksize = yaffsstat.st_blksize;                       /* blocksize for filesystem I/O */
        pstat->st_blocks  = yaffsstat.st_blocks;                        /* number of blocks allocated   */
        
        pstat->st_atime = yaffsstat.yst_atime;                          /* time of last access          */
        pstat->st_mtime = yaffsstat.yst_mtime;                          /* time of last modification    */
        pstat->st_ctime = yaffsstat.yst_ctime;                          /* time of last create          */
        
        /*
         *  ����� yaffs ���豸, ��ôһ���� DIR, ����������, st_mode ����û�� dir ��־.
         *  ��������� yaffs ���豸, ����һ��Ҫǿ��Ϊ dir ����, �Ա� readdir() ˳����ʾ����Թ�
         *  ��ʽ��ʱʹ��.
         */
        if (__STR_IS_ROOT(pyaffile->YAFFIL_cName)) {                    /* yaffs root                   */
            pstat->st_mode |= S_IFDIR;
        }
    
    } else {
        if (__STR_IS_ROOT(pyaffile->YAFFIL_cName)) {                    /* yaffs root                   */
            pstat->st_dev     = (dev_t)pyaffile->YAFFIL_pyaffs;
            pstat->st_ino     = (ino_t)0;                               /* �������ļ��ظ�(��Ŀ¼)       */
            pstat->st_mode    = YAFFS_ROOT_MODE | S_IFDIR;              /* root dir                     */
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            
            pstat->st_atime = API_RootFsTime(LW_NULL);                  /*  use root fs time            */
            pstat->st_mtime = API_RootFsTime(LW_NULL);
            pstat->st_ctime = API_RootFsTime(LW_NULL);
            
            iError = ERROR_NONE;                                        /*  ok                          */
        }
    }
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsLStatGet
** ��������: yaffs stat ����
** �䡡��  : pyaffs           �豸
**           pcName           �ļ���
**           pstat            �ļ�״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsLStatGet (PYAFFS_FSLIB  pyaffs, PCHAR  pcName, struct stat *pstat)
{
    INT                 iError;
    struct yaffs_stat   yaffsstat;

    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __YAFFS_OPLOCK();
    iError = yaffs_lstat(pcName, &yaffsstat);
    if (iError == ERROR_NONE) {
        pstat->st_dev     = (dev_t)yaffsstat.st_dev;                    /* device                       */
        pstat->st_ino     = yaffsstat.st_ino;                           /* inode                        */
        pstat->st_mode    = yaffsstat.st_mode;                          /* protection                   */
        pstat->st_nlink   = yaffsstat.st_nlink;                         /* number of hard links         */
        pstat->st_uid     = (uid_t)yaffsstat.st_uid;                    /* user ID of owner             */
        pstat->st_gid     = (gid_t)yaffsstat.st_gid;                    /* group ID of owner            */
        pstat->st_rdev    = (dev_t)yaffsstat.st_rdev;                   /* device type (if inode device)*/
        pstat->st_size    = yaffsstat.st_size;                          /* total size, in bytes         */
        pstat->st_blksize = yaffsstat.st_blksize;                       /* blocksize for filesystem I/O */
        pstat->st_blocks  = yaffsstat.st_blocks;                        /* number of blocks allocated   */
        
        pstat->st_atime = yaffsstat.yst_atime;                          /* time of last access          */
        pstat->st_mtime = yaffsstat.yst_mtime;                          /* time of last modification    */
        pstat->st_ctime = yaffsstat.yst_ctime;                          /* time of last create          */
        
        /*
         *  ����� yaffs ���豸, ��ôһ���� DIR, ����������, st_mode ����û�� dir ��־.
         *  ��������� yaffs ���豸, ����һ��Ҫǿ��Ϊ dir ����, �Ա� readdir() ˳����ʾ����Թ�
         *  ��ʽ��ʱʹ��.
         */
        if (__STR_IS_ROOT(pcName)) {                                    /* yaffs root                   */
            pstat->st_mode |= S_IFDIR;
        }
    
    } else {
        if (__STR_IS_ROOT(pcName)) {                                    /* yaffs root                   */
            pstat->st_dev     = (dev_t)pyaffs;
            pstat->st_ino     = (ino_t)0;                               /* �������ļ��ظ�(��Ŀ¼)       */
            pstat->st_mode    = YAFFS_ROOT_MODE | S_IFDIR;              /* root dir                     */
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            
            pstat->st_atime = API_RootFsTime(LW_NULL);                  /*  use root fs time            */
            pstat->st_mtime = API_RootFsTime(LW_NULL);
            pstat->st_ctime = API_RootFsTime(LW_NULL);
            
            iError = ERROR_NONE;                                        /*  ok                          */
        }
    }
    __YAFFS_OPUNLOCK();
    
    /*
     *  ��� iError < 0 ϵͳ���Զ�ʹ�� stat ��ȡ
     */
    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsStatfsGet
** ��������: yaffs statfs ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pstatfs          �ļ�ϵͳ״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsStatfsGet (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    INT                 iError;
    struct yaffs_stat   yaffsstat;
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE         pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    
    loff_t              oftFreeBytes;
    loff_t              oftTotalBytes;
    
    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __YAFFS_OPLOCK();
    iError = yaffs_lstat(pyaffile->YAFFIL_cName, &yaffsstat);
    if ((iError == ERROR_NONE) && (yaffsstat.st_blksize > 0)) {
        struct yaffs_dev  *pyaffsdev;
        
        oftFreeBytes  = yaffs_freespace(pyaffile->YAFFIL_cName);
        oftTotalBytes = yaffs_totalspace(pyaffile->YAFFIL_cName);
        
        pstatfs->f_type  = YAFFS_MAGIC;                                 /* type of info, YAFFS_MAGIC   */
        pstatfs->f_bsize = yaffsstat.st_blksize;                        /* fundamental file system block*/
                                                                        /* size                         */
        pstatfs->f_blocks = (long)(oftTotalBytes / yaffsstat.st_blksize);
                                                                        /* total blocks in file system  */
        pstatfs->f_bfree  = (long)(oftFreeBytes / yaffsstat.st_blksize);/* free block in fs             */
        pstatfs->f_bavail = (long)(oftFreeBytes / yaffsstat.st_blksize);/* free blocks avail to         */
                                                                        /* non-superuser                */
        pstatfs->f_files = 0;                                           /* total file nodes in file     */
                                                                        /* system                       */
        pstatfs->f_ffree = 0;                                           /* free file nodes in fs        */
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        pstatfs->f_fsid.val[0] = (int32_t)((addr_t)pyaffile->YAFFIL_pyaffs >> 32);
        pstatfs->f_fsid.val[1] = (int32_t)((addr_t)pyaffile->YAFFIL_pyaffs & 0xffffffff);
#else
        pstatfs->f_fsid.val[0] = (int32_t)pyaffile->YAFFIL_pyaffs;      /* file system sid              */
        pstatfs->f_fsid.val[1] = 0;
#endif
        
        pyaffsdev = (struct yaffs_dev *)yaffs_getdev(pyaffile->YAFFIL_cName);
        if (pyaffsdev && pyaffsdev->read_only) {
            pstatfs->f_flag = ST_RDONLY;
        } else {
            pstatfs->f_flag = 0;
        }
        
        pstatfs->f_namelen = YAFFS_MAX_NAME_LENGTH;                     /* max name len                 */
    
    } else {
        iError = PX_ERROR;
    }
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsChmod
** ��������: yaffs chmod ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           iMode               �µ� mode
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsChmod (PLW_FD_ENTRY  pfdentry, INT  iMode)
{
    INT             iError   = PX_ERROR;
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE     pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    
    iMode |= S_IRUSR;
    
    __YAFFS_OPLOCK();
    if ((pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) ||
        (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_DIR)) {        /*  open ��������ͨ�ļ���Ŀ¼   */
        iError = yaffs_chmod(pyaffile->YAFFIL_cName, (mode_t)iMode);
    }
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsReadDir
** ��������: yaffs ���ָ��Ŀ¼��Ϣ
** �䡡��  : pfdentry            �ļ����ƿ�
**           dir                 Ŀ¼�ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    REGISTER INT             iError  = PX_ERROR;
    struct yaffs_dirent     *yafdirent;
    PLW_FD_NODE              pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE              pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    
    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__STR_IS_ROOT(pyaffile->YAFFIL_cName)) {
        /*
         *  ��� yaffs ��Ŀ¼, �� fat ��ͬ, ���е� yaffs ���ǹ��ص� yaffs ���豸��
         *  ������Ҫ�г���ǰ���� yaffs ��.
         */
        INT     iIndex = (INT)dir->dir_pos;
        INT     iNext;
        PCHAR   pcVolName = yaffs_getdevname(iIndex, &iNext);
        
        if (pcVolName) {                                                /*  �Ƿ��ѯ���豸              */
            dir->dir_pos = (LONG)iNext;
            lib_strcpy(dir->dir_dirent.d_name, &pcVolName[1]);          /*  ���� '/' �ַ�����           */
            dir->dir_dirent.d_shortname[0] = PX_EOS;                    /*  ��֧�ֶ��ļ���              */
            dir->dir_dirent.d_type = DT_DIR;
            iError = ERROR_NONE;
        }
        
        return  (iError);
    }
    
    if (pyaffile->YAFFIL_iFileType != __YAFFS_FILE_TYPE_DIR) {
        _ErrorHandle(ENOTDIR);                                          /*  ��֧��                      */
        return  (PX_ERROR);
    }
    
    __YAFFS_OPLOCK();
    if (dir->dir_pos == 0) {
        yaffs_rewinddir_fd(pyaffile->YAFFIL_iFd);
    }
    
    yafdirent = yaffs_readdir_fd(pyaffile->YAFFIL_iFd);
    if (yafdirent) {
        lib_strcpy(dir->dir_dirent.d_name, yafdirent->d_name);
        dir->dir_dirent.d_shortname[0] = PX_EOS;                        /*  ��֧�ֶ��ļ���              */
        dir->dir_dirent.d_type = yafdirent->d_type;                     /*  ����                        */
        dir->dir_pos++;
        iError = ERROR_NONE;
    }
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsTimeset
** ��������: yaffs �����ļ�ʱ��
** �䡡��  : pfdentry            �ļ����ƿ�
**           utim                utimbuf �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : Ŀǰ�˺����������κδ���.
*********************************************************************************************************/
static INT  __yaffsTimeset (PLW_FD_ENTRY  pfdentry, struct utimbuf  *utim)
{
    struct yaffs_utimbuf    yafutim;
    REGISTER INT            iError;
             PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
             PYAFFS_FILE    pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    
    if (!utim) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_DEV) {
        iError = PX_ERROR;
        _ErrorHandle(ENOSYS);                                           /*  ��֧����ز���              */
    } else {
        yafutim.actime  = (unsigned long)utim->actime;
        yafutim.modtime = (unsigned long)utim->modtime;
        iError = yaffs_futime(pyaffile->YAFFIL_iFd, &yafutim);
    }
    __YAFFS_OPUNLOCK();

    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsFsTruncate
** ��������: yaffs �����ļ���С
** �䡡��  : pfdentry            �ļ����ƿ�
**           oftSize             �ļ���С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsTruncate (PLW_FD_ENTRY  pfdentry, off_t  oftSize)
{
    REGISTER INT            iError   = PX_ERROR;
             PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
             PYAFFS_FILE    pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    
    __YAFFS_OPLOCK();
    if (pyaffile->YAFFIL_iFileType == __YAFFS_FILE_TYPE_NODE) {
        iError = yaffs_ftruncate(pyaffile->YAFFIL_iFd, oftSize);
        if (iError == ERROR_NONE) {
            struct yaffs_stat   yafstat;
            yaffs_fstat(pyaffile->YAFFIL_iFd, &yafstat);
            pfdnode->FDNODE_oftSize = yafstat.st_size;                  /*  �����ļ���С                */
        }
    }
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsFsSymlink
** ��������: yaffs �������������ļ�
** �䡡��  : pyaffs              �豸
**           pcName              �����������ļ�
**           pcLinkDst           ����Ŀ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __yaffsFsSymlink (PYAFFS_FSLIB  pyaffs, PCHAR  pcName, CPCHAR  pcLinkDst)
{
    REGISTER INT   iError;

    __YAFFS_OPLOCK();
    iError = yaffs_symlink(pcLinkDst, pcName);
    __YAFFS_OPUNLOCK();
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __yaffsFsReadlink
** ��������: yaffs ��ȡ���������ļ�����
** �䡡��  : pyaffs              �豸
**           pcName              ����ԭʼ�ļ���
**           pcLinkDst           ����Ŀ���ļ���
**           stMaxSize           �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t __yaffsFsReadlink (PYAFFS_FSLIB  pyaffs, 
                                  PCHAR         pcName,
                                  PCHAR         pcLinkDst,
                                  size_t        stMaxSize)
{
    REGISTER INT   iError;

    __YAFFS_OPLOCK();
    iError = yaffs_readlink(pcName, pcLinkDst, stMaxSize);
    __YAFFS_OPUNLOCK();
    
    if (iError == 0) {
        return  ((ssize_t)lib_strnlen(pcLinkDst, stMaxSize));
    } else {
        return  ((ssize_t)iError);
    }
}
/*********************************************************************************************************
** ��������: __yaffsIoctl
** ��������: yaffs ioctl ����
** �䡡��  : pfdentry           �ļ����ƿ�
**           request,           ����
**           arg                �������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __yaffsIoctl (PLW_FD_ENTRY  pfdentry,
                          INT           iRequest,
                          LONG          lArg)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PYAFFS_FILE    pyaffile = (PYAFFS_FILE)pfdnode->FDNODE_pvFile;
    off_t          oftTemp;
    INT            iError;

    switch (iRequest) {
    
    case FIOCONTIG:
    case FIOTRUNC:
    case FIOLABELSET:
    case FIOATTRIBSET:
        if ((pfdentry->FDENTRY_iFlag & O_ACCMODE) == O_RDONLY) {
            _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
            return  (PX_ERROR);
        }
	}
    
    switch (iRequest) {
    
    case FIODISKFORMAT:                                                 /*  ���̸�ʽ��                  */
        return  (__yaffsFormat(pfdentry));
    
    case FIODISKINIT:                                                   /*  ���̳�ʼ��                  */
        return  (ERROR_NONE);
    
    case FIOSEEK:                                                       /*  �ļ��ض�λ                  */
        oftTemp = *(off_t *)lArg;
        return  (__yaffsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  ����ļ���ǰ��дָ��        */
        iError = __yaffsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
        
    case FIONREAD:                                                      /*  ����ļ�ʣ���ֽ���          */
        return  (__yaffsNRead(pfdentry, (INT *)lArg));
        
    case FIONREAD64:                                                    /*  ����ļ�ʣ���ֽ���          */
        iError = __yaffsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIORENAME:                                                     /*  �ļ�������                  */
        return  (__yaffsRename(pfdentry, (PCHAR)lArg));
    
    case FIOLABELGET:                                                   /*  ��ȡ���                    */
    case FIOLABELSET:                                                   /*  ���þ��                    */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    
    case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
        return  (__yaffsStatGet(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  ����ļ�ϵͳ״̬            */
        return  (__yaffsStatfsGet(pfdentry, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  ��ȡһ��Ŀ¼��Ϣ            */
        return  (__yaffsReadDir(pfdentry, (DIR *)lArg));
    
    case FIOTIMESET:                                                    /*  �����ļ�ʱ��                */
        return  (__yaffsTimeset(pfdentry, (struct utimbuf *)lArg));
        
    /*
     *  FIOTRUNC is 64 bit operate.
     */
    case FIOTRUNC:                                                      /*  �ı��ļ���С                */
        oftTemp = *(off_t *)lArg;
        return  (__yaffsTruncate(pfdentry, oftTemp));
    
    case FIOSYNC:                                                       /*  ���ļ������д              */
    case FIOFLUSH:
        return  (__yaffsFlush(pfdentry));
    
    case FIODATASYNC:                                                   /*  ��д���ݲ���                */
        return  (__yaffsDataSync(pfdentry));
    
    case FIOCHMOD:
        return  (__yaffsChmod(pfdentry, (INT)lArg));                    /*  �ı��ļ�����Ȩ��            */
    
    case FIOSETFL:                                                      /*  �����µ� flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);
    
    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "YAFFS FileSystem";
        return  (ERROR_NONE);
    
    case FIOGETFORCEDEL:                                                /*  ǿ��ж���豸�Ƿ�����      */
        *(BOOL *)lArg = pyaffile->YAFFIL_pyaffs->YAFFS_bForceDelete;
        return  (ERROR_NONE);
        
#if LW_CFG_FS_SELECT_EN > 0
    case FIOSELECT:
        if (((PLW_SEL_WAKEUPNODE)lArg)->SELWUN_seltypType != SELEXCEPT) {
            SEL_WAKE_UP((PLW_SEL_WAKEUPNODE)lArg);                      /*  ���ѽڵ�                    */
        }
        return  (ERROR_NONE);
         
    case FIOUNSELECT:
        if (((PLW_SEL_WAKEUPNODE)lArg)->SELWUN_seltypType != SELEXCEPT) {
            LW_SELWUN_SET_READY((PLW_SEL_WAKEUPNODE)lArg);
        }
        return  (ERROR_NONE);
#endif                                                                  /*  LW_CFG_FS_SELECT_EN > 0     */
        
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
