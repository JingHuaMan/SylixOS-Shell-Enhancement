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
** ��   ��   ��: ramFs.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 05 �� 24 ��
**
** ��        ��: �ڴ��ļ�ϵͳ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 && LW_CFG_RAMFS_EN > 0
#include "ramFsLib.h"
/*********************************************************************************************************
  �ڲ�ȫ�ֱ���
*********************************************************************************************************/
static INT                              _G_iRamfsDrvNum = PX_ERROR;
/*********************************************************************************************************
  �ļ�����
*********************************************************************************************************/
#define __RAMFS_FILE_TYPE_NODE          0                               /*  open ���ļ�               */
#define __RAMFS_FILE_TYPE_DIR           1                               /*  open ��Ŀ¼               */
#define __RAMFS_FILE_TYPE_DEV           2                               /*  open ���豸               */
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __RAMFS_FILE_LOCK(pramn)        API_SemaphoreMPend(pramn->RAMN_pramfs->RAMFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __RAMFS_FILE_UNLOCK(pramn)      API_SemaphoreMPost(pramn->RAMN_pramfs->RAMFS_hVolLock)

#define __RAMFS_VOL_LOCK(pramfs)        API_SemaphoreMPend(pramfs->RAMFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __RAMFS_VOL_UNLOCK(pramfs)      API_SemaphoreMPost(pramfs->RAMFS_hVolLock)
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
static LONG     __ramFsOpen(PRAM_VOLUME     pramfs,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __ramFsRemove(PRAM_VOLUME   pramfs,
                              PCHAR         pcName);
static INT      __ramFsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __ramFsRead(PLW_FD_ENTRY    pfdentry,
                            PCHAR           pcBuffer,
                            size_t          stMaxBytes);
static ssize_t  __ramFsPRead(PLW_FD_ENTRY    pfdentry,
                             PCHAR           pcBuffer,
                             size_t          stMaxBytes,
                             off_t           oftPos);
static ssize_t  __ramFsWrite(PLW_FD_ENTRY  pfdentry,
                             PCHAR         pcBuffer,
                             size_t        stNBytes);
static ssize_t  __ramFsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes,
                              off_t         oftPos);
static INT      __ramFsSeek(PLW_FD_ENTRY  pfdentry,
                            off_t         oftOffset);
static INT      __ramFsWhere(PLW_FD_ENTRY pfdentry,
                             off_t       *poftPos);
static INT      __ramFsStat(PLW_FD_ENTRY  pfdentry, 
                            struct stat  *pstat);
static INT      __ramFsLStat(PRAM_VOLUME   pramfs,
                             PCHAR         pcName,
                             struct stat  *pstat);
static INT      __ramFsIoctl(PLW_FD_ENTRY  pfdentry,
                             INT           iRequest,
                             LONG          lArg);
static INT      __ramFsSymlink(PRAM_VOLUME   pramfs,
                               PCHAR         pcName,
                               CPCHAR        pcLinkDst);
static ssize_t  __ramFsReadlink(PRAM_VOLUME   pramfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize);
/*********************************************************************************************************
** ��������: API_RamFsDrvInstall
** ��������: ��װ ramfs �ļ�ϵͳ��������
** �䡡��  :
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_RamFsDrvInstall (VOID)
{
    struct file_operations     fileop;
    
    if (_G_iRamfsDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __ramFsOpen;
    fileop.fo_release  = __ramFsRemove;
    fileop.fo_open     = __ramFsOpen;
    fileop.fo_close    = __ramFsClose;
    fileop.fo_read     = __ramFsRead;
    fileop.fo_read_ex  = __ramFsPRead;
    fileop.fo_write    = __ramFsWrite;
    fileop.fo_write_ex = __ramFsPWrite;
    fileop.fo_lstat    = __ramFsLStat;
    fileop.fo_ioctl    = __ramFsIoctl;
    fileop.fo_symlink  = __ramFsSymlink;
    fileop.fo_readlink = __ramFsReadlink;
    
    _G_iRamfsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);     /*  ʹ�� NEW_1 ���豸��������   */

    DRIVER_LICENSE(_G_iRamfsDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iRamfsDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iRamfsDrvNum, "ramfs driver.");

    _DebugHandle(__LOGMESSAGE_LEVEL, "ram file system installed.\r\n");
                                     
    __fsRegister("ramfs", API_RamFsDevCreate, LW_NULL, LW_NULL);        /*  ע���ļ�ϵͳ                */

    return  ((_G_iRamfsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: API_RamFsDevCreate
** ��������: ���� ramfs �ļ�ϵͳ�豸.
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
**           pblkd             ʹ�� pblkd->BLKD_pcName ��Ϊ ����С ��ʾ.
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_RamFsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    PRAM_VOLUME     pramfs;
    size_t          stMax;

    if (_G_iRamfsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ramfs Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    if ((pblkd == LW_NULL) || (pblkd->BLKD_pcName == LW_NULL)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "block device invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    if ((pcName == LW_NULL) || __STR_IS_ROOT(pcName)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "mount name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    if (sscanf(pblkd->BLKD_pcName, "%zu", &stMax) != 1) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "max size invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pramfs = (PRAM_VOLUME)__SHEAP_ALLOC(sizeof(RAM_VOLUME));
    if (pramfs == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pramfs, sizeof(RAM_VOLUME));                              /*  ��վ���ƿ�                */
    
    pramfs->RAMFS_bValid = LW_TRUE;
    
    pramfs->RAMFS_hVolLock = API_SemaphoreMCreate("ramvol_lock", LW_PRIO_DEF_CEILING,
                                             LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                             LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                             LW_NULL);
    if (!pramfs->RAMFS_hVolLock) {                                      /*  �޷���������                */
        __SHEAP_FREE(pramfs);
        return  (PX_ERROR);
    }
    
    pramfs->RAMFS_mode     = S_IFDIR | DEFAULT_DIR_PERM;
    pramfs->RAMFS_uid      = getuid();
    pramfs->RAMFS_gid      = getgid();
    pramfs->RAMFS_time     = lib_time(LW_NULL);
    pramfs->RAMFS_ulCurBlk = 0ul;
    
    if (stMax == 0) {
#if LW_CFG_CPU_WORD_LENGHT == 32
        pramfs->RAMFS_ulMaxBlk = (__ARCH_ULONG_MAX / __RAM_BSIZE);
#else
        pramfs->RAMFS_ulMaxBlk = ((ULONG)(128ul * LW_CFG_GB_SIZE) / __RAM_BSIZE);
#endif
    } else {
        pramfs->RAMFS_ulMaxBlk = (ULONG)(stMax / __RAM_BSIZE);
    }

    __ram_mount(pramfs);
    
    if (iosDevAddEx(&pramfs->RAMFS_devhdrHdr, pcName, _G_iRamfsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  ��װ�ļ�ϵͳ�豸            */
        API_SemaphoreMDelete(&pramfs->RAMFS_hVolLock);
        __SHEAP_FREE(pramfs);
        return  (PX_ERROR);
    }
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "target \"%s\" mount ok.\r\n", pcName);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_RamFsDevDelete
** ��������: ɾ��һ�� ramfs �ļ�ϵͳ�豸, ����: API_RamFsDevDelete("/mnt/ram0");
** �䡡��  : pcName            �ļ�ϵͳ�豸��(�����豸�ҽӵĽڵ��ַ)
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_RamFsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  ������豸, �����ж���豸  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __ramFsOpen
** ��������: �򿪻��ߴ����ļ�
** �䡡��  : pramfs           ramfs �ļ�ϵͳ
**           pcName           �ļ���
**           iFlags           ��ʽ
**           iMode            mode_t
** �䡡��  : < 0 ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG __ramFsOpen (PRAM_VOLUME     pramfs,
                         PCHAR           pcName,
                         INT             iFlags,
                         INT             iMode)
{
    PLW_FD_NODE pfdnode;
    PRAM_NODE   pramn;
    PRAM_NODE   pramnFather;
    BOOL        bRoot;
    BOOL        bLast;
    PCHAR       pcTail;
    BOOL        bIsNew;
    BOOL        bCreate = LW_FALSE;
    struct stat statGet;
    
    if (pcName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    if (iFlags & O_CREAT) {                                             /*  ��������                    */
        if (__fsCheckFileName(pcName)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        if (S_ISFIFO(iMode) || 
            S_ISBLK(iMode)  ||
            S_ISCHR(iMode)) {
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  ��֧��������Щ��ʽ          */
            return  (PX_ERROR);
        }
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    pramn = __ram_open(pramfs, pcName, &pramnFather, &bRoot, &bLast, &pcTail);
    if (pramn) {
        if (!S_ISLNK(pramn->RAMN_mode)) {
            if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {              /*  ���������ļ�                */
                __RAMFS_VOL_UNLOCK(pramfs);
                _ErrorHandle(EEXIST);                                   /*  �Ѿ������ļ�                */
                return  (PX_ERROR);
            
            } else if ((iFlags & O_DIRECTORY) && !S_ISDIR(pramn->RAMN_mode)) {
                __RAMFS_VOL_UNLOCK(pramfs);
                _ErrorHandle(ENOTDIR);
                return  (PX_ERROR);
            
            } else {
                goto    __file_open_ok;
            }
        }
    
    } else if ((iFlags & O_CREAT) && bLast) {                           /*  �����ڵ�                    */
        pramn = __ram_maken(pramfs, pcName, pramnFather, iMode, LW_NULL);
        if (pramn) {
            bCreate = LW_TRUE;
            goto    __file_open_ok;
        
        } else {
            return  (PX_ERROR);
        }
    }
    
    if (pramn) {                                                        /*  �������Ӵ���                */
        INT     iError;
        INT     iFollowLinkType;
        PCHAR   pcSymfile = pcTail - lib_strlen(pramn->RAMN_pcName) - 1;
        PCHAR   pcPrefix;
        
        if (*pcSymfile != PX_DIVIDER) {
            pcSymfile--;
        }
        if (pcSymfile == pcName) {
            pcPrefix = LW_NULL;                                         /*  û��ǰ׺                    */
        } else {
            pcPrefix = pcName;
            *pcSymfile = PX_EOS;
        }
        if (pcTail && lib_strlen(pcTail)) {
            iFollowLinkType = FOLLOW_LINK_TAIL;                         /*  ����Ŀ���ڲ��ļ�            */
        } else {
            iFollowLinkType = FOLLOW_LINK_FILE;                         /*  �����ļ�����                */
        }
        
        iError = _PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                LW_NULL, pcPrefix, pramn->RAMN_pcLink, pcTail);
        if (iError) {
            __RAMFS_VOL_UNLOCK(pramfs);
            return  (PX_ERROR);                                         /*  �޷�����������Ŀ��Ŀ¼      */
        } else {
            __RAMFS_VOL_UNLOCK(pramfs);
            return  (iFollowLinkType);
        }
    
    } else if (bRoot == LW_FALSE) {                                     /*  ���Ǵ򿪸�Ŀ¼              */
        __RAMFS_VOL_UNLOCK(pramfs);
        _ErrorHandle(ENOENT);                                           /*  û���ҵ��ļ�                */
        return  (PX_ERROR);
    }
    
__file_open_ok:
    __ram_stat(pramn, pramfs, &statGet);
    pfdnode = API_IosFdNodeAdd(&pramfs->RAMFS_plineFdNodeHeader,
                               statGet.st_dev,
                               (ino64_t)statGet.st_ino,
                               iFlags,
                               iMode,
                               statGet.st_uid,
                               statGet.st_gid,
                               statGet.st_size,
                               (PVOID)pramn,
                               &bIsNew);                                /*  ����ļ��ڵ�                */
    if (pfdnode == LW_NULL) {                                           /*  �޷����� fd_node �ڵ�       */
        __RAMFS_VOL_UNLOCK(pramfs);
        if (bCreate) {
            __ram_unlink(pramn);                                        /*  ɾ���½��Ľڵ�              */
        }
        return  (PX_ERROR);
    }
    
    pfdnode->FDNODE_pvFsExtern = (PVOID)pramfs;                         /*  ��¼�ļ�ϵͳ��Ϣ            */
    
    if ((iFlags & O_TRUNC) && ((iFlags & O_ACCMODE) != O_RDONLY)) {     /*  ��Ҫ�ض�                    */
        if (pramn) {
            __ram_truncate(pramn, 0);
            pfdnode->FDNODE_oftSize = 0;
        }
    }
    
    LW_DEV_INC_USE_COUNT(&pramfs->RAMFS_devhdrHdr);                     /*  ���¼�����                  */
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  ((LONG)pfdnode);                                            /*  �����ļ��ڵ�                */
}
/*********************************************************************************************************
** ��������: __ramFsRemove
** ��������: ramfs remove ����
** �䡡��  : pramfs           ���豸
**           pcName           �ļ���
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsRemove (PRAM_VOLUME   pramfs,
                           PCHAR         pcName)
{
    PRAM_NODE  pramn;
    BOOL       bRoot;
    PCHAR      pcTail;
    INT        iError;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
        
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    pramn = __ram_open(pramfs, pcName, LW_NULL, &bRoot, LW_NULL, &pcTail);
    if (pramn) {
        if (S_ISLNK(pramn->RAMN_mode)) {                                /*  ��������                    */
            size_t  stLenTail = 0;
            if (pcTail) {
                stLenTail = lib_strlen(pcTail);                         /*  ȷ�� tail ����              */
            }
            if (stLenTail) {                                            /*  ָ�������ļ�                */
                PCHAR   pcSymfile = pcTail - lib_strlen(pramn->RAMN_pcName) - 1;
                PCHAR   pcPrefix;
                
                if (*pcSymfile != PX_DIVIDER) {
                    pcSymfile--;
                }
                if (pcSymfile == pcName) {
                    pcPrefix = LW_NULL;                                 /*  û��ǰ׺                    */
                } else {
                    pcPrefix = pcName;
                    *pcSymfile = PX_EOS;
                }
                
                if (_PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                   LW_NULL, pcPrefix, pramn->RAMN_pcLink, pcTail) < ERROR_NONE) {
                    __RAMFS_VOL_UNLOCK(pramfs);
                    return  (PX_ERROR);                                 /*  �޷�����������Ŀ��Ŀ¼      */
                } else {
                    __RAMFS_VOL_UNLOCK(pramfs);
                    return  (FOLLOW_LINK_TAIL);
                }
            }
        }
        
        iError = __ram_unlink(pramn);
        __RAMFS_VOL_UNLOCK(pramfs);
        return  (iError);
            
    } else if (bRoot) {                                                 /*  ɾ�� ramfs �ļ�ϵͳ         */
        if (pramfs->RAMFS_bValid == LW_FALSE) {
            __RAMFS_VOL_UNLOCK(pramfs);
            return  (ERROR_NONE);                                       /*  ���ڱ���������ж��          */
        }
        
__re_umount_vol:
        if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pramfs)) {
            if (!pramfs->RAMFS_bForceDelete) {
                __RAMFS_VOL_UNLOCK(pramfs);
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            }
            
            pramfs->RAMFS_bValid = LW_FALSE;
            
            __RAMFS_VOL_UNLOCK(pramfs);
            
            _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
            iosDevFileAbnormal(&pramfs->RAMFS_devhdrHdr);               /*  ����������ļ���Ϊ�쳣ģʽ  */
            
            __RAMFS_VOL_LOCK(pramfs);
            goto    __re_umount_vol;
        
        } else {
            pramfs->RAMFS_bValid = LW_FALSE;
        }
        
        iosDevDelete((LW_DEV_HDR *)pramfs);                             /*  IO ϵͳ�Ƴ��豸             */
        API_SemaphoreMDelete(&pramfs->RAMFS_hVolLock);
        
        __ram_unmount(pramfs);                                          /*  �ͷ������ļ�����            */
        __SHEAP_FREE(pramfs);
        
        _DebugHandle(__LOGMESSAGE_LEVEL, "romfs unmount ok.\r\n");
        
        return  (ERROR_NONE);
        
    } else {
        __RAMFS_VOL_UNLOCK(pramfs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __ramFsClose
** ��������: ramfs close ����
** �䡡��  : pfdentry         �ļ����ƿ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    BOOL          bRemove = LW_FALSE;
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (API_IosFdNodeDec(&pramfs->RAMFS_plineFdNodeHeader, 
                         pfdnode, &bRemove) == 0) {
        if (pramn) {
            __ram_close(pramn, pfdentry->FDENTRY_iFlag);
        }
    }
    
    LW_DEV_DEC_USE_COUNT(&pramfs->RAMFS_devhdrHdr);
    
    if (bRemove && pramn) {
        __ram_unlink(pramn);
    }
        
    __RAMFS_VOL_UNLOCK(pramfs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsRead
** ��������: ramfs read ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __ramFsRead (PLW_FD_ENTRY pfdentry,
                             PCHAR        pcBuffer,
                             size_t       stMaxBytes)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn      = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstReadNum = PX_ERROR;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes) {
        sstReadNum = __ram_read(pramn, pcBuffer, stMaxBytes, (size_t)pfdentry->FDENTRY_oftPtr);
        if (sstReadNum > 0) {
            pfdentry->FDENTRY_oftPtr += (off_t)sstReadNum;              /*  �����ļ�ָ��                */
        }
    
    } else {
        sstReadNum = 0;
    }
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (sstReadNum);
}
/*********************************************************************************************************
** ��������: __ramFsPRead
** ��������: ramfs pread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __ramFsPRead (PLW_FD_ENTRY pfdentry,
                              PCHAR        pcBuffer,
                              size_t       stMaxBytes,
                              off_t        oftPos)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn      = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstReadNum = PX_ERROR;
    
    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes) {
        sstReadNum = __ram_read(pramn, pcBuffer, stMaxBytes, (size_t)oftPos);
    
    } else {
        sstReadNum = 0;
    }
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (sstReadNum);
}
/*********************************************************************************************************
** ��������: __ramFsWrite
** ��������: ramfs write ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __ramFsWrite (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes)
{
    PLW_FD_NODE   pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn       = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstWriteNum = PX_ERROR;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  ׷��ģʽ                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  �ƶ���дָ�뵽ĩβ          */
    }
    
    if (stNBytes) {
        sstWriteNum = __ram_write(pramn, pcBuffer, stNBytes, (size_t)pfdentry->FDENTRY_oftPtr);
        if (sstWriteNum > 0) {
            pfdentry->FDENTRY_oftPtr += (off_t)sstWriteNum;             /*  �����ļ�ָ��                */
            pfdnode->FDNODE_oftSize   = (off_t)pramn->RAMN_stSize;
        }
        
    } else {
        sstWriteNum = 0;
    }
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (sstWriteNum);
}
/*********************************************************************************************************
** ��������: __ramFsPWrite
** ��������: ramfs pwrite ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __ramFsPWrite (PLW_FD_ENTRY  pfdentry,
                               PCHAR         pcBuffer,
                               size_t        stNBytes,
                               off_t         oftPos)
{
    PLW_FD_NODE   pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn       = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstWriteNum = PX_ERROR;
    
    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stNBytes) {
        sstWriteNum = __ram_write(pramn, pcBuffer, stNBytes, (size_t)oftPos);
        if (sstWriteNum > 0) {
            pfdnode->FDNODE_oftSize = (off_t)pramn->RAMN_stSize;
        }
        
    } else {
        sstWriteNum = 0;
    }
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (sstWriteNum);
}
/*********************************************************************************************************
** ��������: __ramFsNRead
** ��������: ramFs nread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           piNRead          ʣ��������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsNRead (PLW_FD_ENTRY  pfdentry, INT  *piNRead)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    
    if (piNRead == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    *piNRead = (INT)(pramn->RAMN_stSize - (size_t)pfdentry->FDENTRY_oftPtr);
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsNRead64
** ��������: ramFs nread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           poftNRead        ʣ��������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftNRead)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    *poftNRead = (off_t)(pramn->RAMN_stSize - (size_t)pfdentry->FDENTRY_oftPtr);
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsRename
** ��������: ramFs rename ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcNewName        �µ�����
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsRename (PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    PRAM_VOLUME   pramfsNew;
    CHAR          cNewPath[PATH_MAX + 1];
    INT           iError;
    
    if (pramn == LW_NULL) {                                             /*  ����Ƿ�Ϊ�豸�ļ�          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  ��֧���豸������            */
        return (PX_ERROR);
    }
    
    if (pcNewName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return (PX_ERROR);
    }
    
    if (__STR_IS_ROOT(pcNewName)) {
        _ErrorHandle(ENOENT);
        return (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (ioFullFileNameGet(pcNewName, 
                          (LW_DEV_HDR **)&pramfsNew, 
                          cNewPath) != ERROR_NONE) {                    /*  �����Ŀ¼·��              */
        __RAMFS_FILE_UNLOCK(pramn);
        return  (PX_ERROR);
    }
    
    if (pramfsNew != pramfs) {                                          /*  ����Ϊͬһ�豸�ڵ�          */
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    iError = __ram_move(pramn, cNewPath);
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __ramFsSeek
** ��������: ramFs seek ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           oftOffset        ƫ����
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsSeek (PLW_FD_ENTRY  pfdentry,
                         off_t         oftOffset)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (oftOffset > (size_t)~0) {
        _ErrorHandle(EOVERFLOW);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_FILE_LOCK(pramn) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_FILE_UNLOCK(pramn);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    pfdentry->FDENTRY_oftPtr = oftOffset;
    if (pramn->RAMN_stVSize < (size_t)oftOffset) {
        pramn->RAMN_stVSize = (size_t)oftOffset;
    }
    
    __RAMFS_FILE_UNLOCK(pramn);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsWhere
** ��������: ramFs ����ļ���ǰ��дָ��λ�� (ʹ�ò�����Ϊ����ֵ, �� FIOWHERE ��Ҫ�����в�ͬ)
** �䡡��  : pfdentry            �ļ����ƿ�
**           poftPos             ��дָ��λ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    if (poftPos) {
        *poftPos = (off_t)pfdentry->FDENTRY_oftPtr;
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __ramFsStatGet
** ��������: ramFs stat ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pstat            �ļ�״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsStat (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    __ram_stat(pramn, pramfs, pstat);
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsLStat
** ��������: ramFs stat ����
** �䡡��  : pramfs           romfs �ļ�ϵͳ
**           pcName           �ļ���
**           pstat            �ļ�״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsLStat (PRAM_VOLUME  pramfs, PCHAR  pcName, struct stat *pstat)
{
    PRAM_NODE     pramn;
    BOOL          bRoot;
    
    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    pramn = __ram_open(pramfs, pcName, LW_NULL, &bRoot, LW_NULL, LW_NULL);
    if (pramn) {
        __ram_stat(pramn, pramfs, pstat);
    
    } else if (bRoot) {
        __ram_stat(LW_NULL, pramfs, pstat);
    
    } else {
        __RAMFS_VOL_UNLOCK(pramfs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsStatfs
** ��������: ramFs statfs ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pstatfs          �ļ�ϵͳ״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsStatfs (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    __ram_statfs(pramfs, pstatfs);
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsReadDir
** ��������: ramFs ���ָ��Ŀ¼��Ϣ
** �䡡��  : pfdentry            �ļ����ƿ�
**           dir                 Ŀ¼�ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    INT                i;
    LONG               iStart;
    INT                iError = ERROR_NONE;
    PLW_LIST_LINE      plineTemp;
    PLW_LIST_LINE      plineHeader;
    PRAM_NODE          pramnTemp;
    
    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    if (pramn == LW_NULL) {
        plineHeader = pramfs->RAMFS_plineSon;
    } else {
        if (!S_ISDIR(pramn->RAMN_mode)) {
            __RAMFS_VOL_UNLOCK(pramfs);
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
        plineHeader = pramn->RAMN_plineSon;
    }
    
    iStart = dir->dir_pos;

    for ((plineTemp  = plineHeader), (i = 0); 
         (plineTemp != LW_NULL) && (i < iStart); 
         (plineTemp  = _list_line_get_next(plineTemp)), (i++));         /*  ����                        */
    
    if (plineTemp == LW_NULL) {
        _ErrorHandle(ENOENT);
        iError = PX_ERROR;                                              /*  û�ж���Ľڵ�              */
    
    } else {
        pramnTemp = _LIST_ENTRY(plineTemp, RAM_NODE, RAMN_lineBrother);
        dir->dir_pos++;
        
        lib_strlcpy(dir->dir_dirent.d_name, 
                    pramnTemp->RAMN_pcName, 
                    sizeof(dir->dir_dirent.d_name));
                    
        dir->dir_dirent.d_type = IFTODT(pramnTemp->RAMN_mode);
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __ramFsTimeset
** ��������: ramfs �����ļ�ʱ��
** �䡡��  : pfdentry            �ļ����ƿ�
**           utim                utimbuf �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramFsTimeset (PLW_FD_ENTRY  pfdentry, struct utimbuf  *utim)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!utim) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pramn) {
        pramn->RAMN_timeAccess = utim->actime;
        pramn->RAMN_timeChange = utim->modtime;
    
    } else {
        pramfs->RAMFS_time = utim->modtime;
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsTruncate
** ��������: ramfs �����ļ���С
** �䡡��  : pfdentry            �ļ����ƿ�
**           oftSize             �ļ���С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramFsTruncate (PLW_FD_ENTRY  pfdentry, off_t  oftSize)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    size_t        stTru;
    
    if (pramn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (oftSize < 0) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (oftSize > (size_t)~0) {
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pramn->RAMN_mode)) {
        __RAMFS_VOL_UNLOCK(pramfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    stTru = (size_t)oftSize;
    
    if (stTru > pramn->RAMN_stSize) {
        __ram_increase(pramn, stTru);
        
    } else if (stTru < pramn->RAMN_stSize) {
        __ram_truncate(pramn, stTru);
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsChmod
** ��������: ramfs chmod ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           iMode               �µ� mode
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramFsChmod (PLW_FD_ENTRY  pfdentry, INT  iMode)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    iMode |= S_IRUSR;
    iMode &= ~S_IFMT;
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pramn) {
        pramn->RAMN_mode &= S_IFMT;
        pramn->RAMN_mode |= iMode;
    } else {
        pramfs->RAMFS_mode &= S_IFMT;
        pramfs->RAMFS_mode |= iMode;
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsChown
** ��������: ramfs chown ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           pusr                �µ������û�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramFsChown (PLW_FD_ENTRY  pfdentry, LW_IO_USR  *pusr)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_NODE     pramn   = (PRAM_NODE)pfdnode->FDNODE_pvFile;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!pusr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pramn) {
        pramn->RAMN_uid = pusr->IOU_uid;
        pramn->RAMN_gid = pusr->IOU_gid;
    } else {
        pramfs->RAMFS_uid = pusr->IOU_uid;
        pramfs->RAMFS_gid = pusr->IOU_gid;
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsSymlink
** ��������: ramFs �������������ļ�
** �䡡��  : pramfs              romfs �ļ�ϵͳ
**           pcName              ����ԭʼ�ļ���
**           pcLinkDst           ����Ŀ���ļ���
**           stMaxSize           �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsSymlink (PRAM_VOLUME   pramfs,
                            PCHAR         pcName,
                            CPCHAR        pcLinkDst)
{
    PRAM_NODE     pramn;
    PRAM_NODE     pramnFather;
    BOOL          bRoot;
    
    if (!pcName || !pcLinkDst) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__fsCheckFileName(pcName)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    pramn = __ram_open(pramfs, pcName, &pramnFather, &bRoot, LW_NULL, LW_NULL);
    if (pramn || bRoot) {
        __RAMFS_VOL_UNLOCK(pramfs);
        _ErrorHandle(EEXIST);
        return  (PX_ERROR);
    }
    
    pramn = __ram_maken(pramfs, pcName, pramnFather, S_IFLNK | DEFAULT_SYMLINK_PERM, pcLinkDst);
    if (pramn == LW_NULL) {
        __RAMFS_VOL_UNLOCK(pramfs);
        return  (PX_ERROR);
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsReadlink
** ��������: ramFs ��ȡ���������ļ�����
** �䡡��  : pramfs              romfs �ļ�ϵͳ
**           pcName              ����ԭʼ�ļ���
**           pcLinkDst           ����Ŀ���ļ���
**           stMaxSize           �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t __ramFsReadlink (PRAM_VOLUME   pramfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize)
{
    PRAM_NODE   pramn;
    size_t      stLen;
    
    if (!pcName || !pcLinkDst || !stMaxSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__RAMFS_VOL_LOCK(pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    pramn = __ram_open(pramfs, pcName, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    if ((pramn == LW_NULL) || !S_ISLNK(pramn->RAMN_mode)) {
        __RAMFS_VOL_UNLOCK(pramfs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    stLen = lib_strlen(pramn->RAMN_pcLink);
    
    lib_strncpy(pcLinkDst, pramn->RAMN_pcLink, stMaxSize);
    
    if (stLen > stMaxSize) {
        stLen = stMaxSize;                                              /*  ������Ч�ֽ���              */
    }
    
    __RAMFS_VOL_UNLOCK(pramfs);
    
    return  ((ssize_t)stLen);
}
/*********************************************************************************************************
** ��������: __ramFsIoctl
** ��������: ramFs ioctl ����
** �䡡��  : pfdentry           �ļ����ƿ�
**           request,           ����
**           arg                �������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ramFsIoctl (PLW_FD_ENTRY  pfdentry,
                          INT           iRequest,
                          LONG          lArg)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PRAM_VOLUME   pramfs  = (PRAM_VOLUME)pfdnode->FDNODE_pvFsExtern;
    off_t         oftTemp;
    INT           iError;
    
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
	
	case FIODISKINIT:                                                   /*  ���̳�ʼ��                  */
        return  (ERROR_NONE);
        
    case FIOSEEK:                                                       /*  �ļ��ض�λ                  */
        oftTemp = *(off_t *)lArg;
        return  (__ramFsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  ����ļ���ǰ��дָ��        */
        iError = __ramFsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
        
    case FIONREAD:                                                      /*  ����ļ�ʣ���ֽ���          */
        return  (__ramFsNRead(pfdentry, (INT *)lArg));
        
    case FIONREAD64:                                                    /*  ����ļ�ʣ���ֽ���          */
        iError = __ramFsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIORENAME:                                                     /*  �ļ�������                  */
        return  (__ramFsRename(pfdentry, (PCHAR)lArg));
    
    case FIOLABELGET:                                                   /*  ��ȡ���                    */
    case FIOLABELSET:                                                   /*  ���þ��                    */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    
    case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
        return  (__ramFsStat(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  ����ļ�ϵͳ״̬            */
        return  (__ramFsStatfs(pfdentry, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  ��ȡһ��Ŀ¼��Ϣ            */
        return  (__ramFsReadDir(pfdentry, (DIR *)lArg));
    
    case FIOTIMESET:                                                    /*  �����ļ�ʱ��                */
        return  (__ramFsTimeset(pfdentry, (struct utimbuf *)lArg));
        
    case FIOTRUNC:                                                      /*  �ı��ļ���С                */
        oftTemp = *(off_t *)lArg;
        return  (__ramFsTruncate(pfdentry, oftTemp));
    
    case FIOSYNC:                                                       /*  ���ļ������д              */
    case FIOFLUSH:
    case FIODATASYNC:
        return  (ERROR_NONE);
        
    case FIOCHMOD:
        return  (__ramFsChmod(pfdentry, (INT)lArg));                    /*  �ı��ļ�����Ȩ��            */
    
    case FIOSETFL:                                                      /*  �����µ� flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);
    
    case FIOCHOWN:                                                      /*  �޸��ļ�������ϵ            */
        return  (__ramFsChown(pfdentry, (LW_IO_USR *)lArg));
    
    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "RAM FileSystem";
        return  (ERROR_NONE);
    
    case FIOGETFORCEDEL:                                                /*  ǿ��ж���豸�Ƿ�����      */
        *(BOOL *)lArg = pramfs->RAMFS_bForceDelete;
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

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_RAMFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
