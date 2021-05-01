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
** ��   ��   ��: romFs.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 06 �� 27 ��
**
** ��        ��: rom �ļ�ϵͳ sylixos �ӿ�. (һЩ�߿ɿ��Եͳɱ��Ĳ�Ʒ����ʹ�� rom fs ����)

** BUG:
2012.07.09  ����ע��.
2012.07.17  ���� romfs �ļ�ϵͳʧ��ʱ, ��Ҫɾ����.
2012.08.16  ֧�� pread ����.
2012.09.01  ���벻��ǿ��ж�ؾ�ı���.
2012.12.14  __FAT_FILE_LOCK() ��Ҫ�жϷ���ֵ, �������Ӱ�ȫ.
2013.01.06  romfs ʹ������ NEW_1 �豸��������. ��������֧���ļ���.
2014.05.24  �ļ�ϵͳ���� uid gid.
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
#if LW_CFG_MAX_VOLUMES > 0 && LW_CFG_ROMFS_EN > 0
#include "romFsLib.h"
/*********************************************************************************************************
  �ڲ�ȫ�ֱ���
*********************************************************************************************************/
static INT                              _G_iRomfsDrvNum = PX_ERROR;
/*********************************************************************************************************
  �ļ�����
*********************************************************************************************************/
#define __ROMFS_FILE_TYPE_NODE          0                               /*  open ���ļ�               */
#define __ROMFS_FILE_TYPE_DIR           1                               /*  open ��Ŀ¼               */
#define __ROMFS_FILE_TYPE_DEV           2                               /*  open ���豸               */
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __ROMFS_FILE_LOCK(promfile)     API_SemaphoreMPend(promfile->ROMFIL_promfs->ROMFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __ROMFS_FILE_UNLOCK(promfile)   API_SemaphoreMPost(promfile->ROMFIL_promfs->ROMFS_hVolLock)

#define __ROMFS_VOL_LOCK(promfs)        API_SemaphoreMPend(promfs->ROMFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __ROMFS_VOL_UNLOCK(promfs)      API_SemaphoreMPost(promfs->ROMFS_hVolLock)
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
static LONG     __romFsOpen(PROM_VOLUME     promfs,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __romFsRemove(PROM_VOLUME   promfs,
                              PCHAR         pcName);
static INT      __romFsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __romFsRead(PLW_FD_ENTRY    pfdentry,
                            PCHAR           pcBuffer,
                            size_t          stMaxBytes);
static ssize_t  __romFsPRead(PLW_FD_ENTRY    pfdentry,
                             PCHAR           pcBuffer,
                             size_t          stMaxBytes,
                             off_t           oftPos);
static ssize_t  __romFsWrite(PLW_FD_ENTRY  pfdentry,
                             PCHAR         pcBuffer,
                             size_t        stNBytes);
static ssize_t  __romFsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes,
                              off_t         oftPos);
static INT      __romFsSeek(PLW_FD_ENTRY  pfdentry,
                            off_t         oftOffset);
static INT      __romFsWhere(PLW_FD_ENTRY pfdentry,
                             off_t       *poftPos);
static INT      __romFsStat(PLW_FD_ENTRY  pfdentry, 
                            struct stat  *pstat);
static INT      __romFsLStat(PROM_VOLUME   promfs,
                             PCHAR         pcName,
                             struct stat  *pstat);
static INT      __romFsIoctl(PLW_FD_ENTRY  pfdentry,
                             INT           iRequest,
                             LONG          lArg);
static ssize_t  __romFsReadlink(PROM_VOLUME   promfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize);
/*********************************************************************************************************
** ��������: API_RomFsDrvInstall
** ��������: ��װ romfs �ļ�ϵͳ��������
** �䡡��  :
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_RomFsDrvInstall (VOID)
{
    struct file_operations     fileop;
    
    if (_G_iRomfsDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __romFsOpen;
    fileop.fo_release  = __romFsRemove;
    fileop.fo_open     = __romFsOpen;
    fileop.fo_close    = __romFsClose;
    fileop.fo_read     = __romFsRead;
    fileop.fo_read_ex  = __romFsPRead;
    fileop.fo_write    = __romFsWrite;
    fileop.fo_write_ex = __romFsPWrite;
    fileop.fo_lstat    = __romFsLStat;
    fileop.fo_ioctl    = __romFsIoctl;
    fileop.fo_readlink = __romFsReadlink;
    
    _G_iRomfsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);     /*  ʹ�� NEW_1 ���豸��������   */

    DRIVER_LICENSE(_G_iRomfsDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iRomfsDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iRomfsDrvNum, "romfs driver.");

    _DebugHandle(__LOGMESSAGE_LEVEL, "rom file system installed.\r\n");
    
    __fsRegister("romfs", API_RomFsDevCreate, LW_NULL, LW_NULL);        /*  ע���ļ�ϵͳ                */

    return  ((_G_iRomfsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: API_RomFsDevCreate
** ��������: ���� romfs �ļ�ϵͳ�豸.
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
**           pblkd             romfs �����豸.
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_RomFsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    PROM_VOLUME     promfs;

    if (_G_iRomfsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "romfs Driver invalidate.\r\n");
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
    
    promfs = (PROM_VOLUME)__SHEAP_ALLOC(sizeof(ROM_VOLUME));
    if (promfs == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(promfs, sizeof(ROM_VOLUME));                              /*  ��վ���ƿ�                */
    
    promfs->ROMFS_bValid = LW_TRUE;
    
    promfs->ROMFS_hVolLock = API_SemaphoreMCreate("romvol_lock", LW_PRIO_DEF_CEILING,
                                             LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                             LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                             LW_NULL);
    if (!promfs->ROMFS_hVolLock) {                                      /*  �޷���������                */
        __SHEAP_FREE(promfs);
        return  (PX_ERROR);
    }
    promfs->ROMFS_pblkd = pblkd;                                        /*  ��¼�����豸                */
    
    promfs->ROMFS_ulSectorSize = pblkd->BLKD_ulBytesPerSector;
    if (promfs->ROMFS_ulSectorSize == 0) {
        if (pblkd->BLKD_pfuncBlkIoctl(pblkd, 
                                      LW_BLKD_GET_SECSIZE, 
                                      &promfs->ROMFS_ulSectorSize) < ERROR_NONE) {
            API_SemaphoreMDelete(&promfs->ROMFS_hVolLock);
            __SHEAP_FREE(promfs);
            return  (PX_ERROR);
        }
    }
    
    promfs->ROMFS_ulSectorNum = pblkd->BLKD_ulNSector;
    if (promfs->ROMFS_ulSectorNum == 0) {
        if (pblkd->BLKD_pfuncBlkIoctl(pblkd, 
                                      LW_BLKD_GET_SECNUM, 
                                      &promfs->ROMFS_ulSectorNum) < ERROR_NONE) {
            API_SemaphoreMDelete(&promfs->ROMFS_hVolLock);
            __SHEAP_FREE(promfs);
            return  (PX_ERROR);
        }
    }
    
    promfs->ROMFS_pcSector = (PCHAR)__SHEAP_ALLOC((size_t)promfs->ROMFS_ulSectorSize);
    if (promfs->ROMFS_pcSector == LW_NULL) {
        API_SemaphoreMDelete(&promfs->ROMFS_hVolLock);
        __SHEAP_FREE(promfs);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    promfs->ROMFS_ulCurSector = (ULONG)-1;                              /*  ��ǰ������Ч                */
    
    if (__rfs_mount(promfs)) {
        API_SemaphoreMDelete(&promfs->ROMFS_hVolLock);
        __SHEAP_FREE(promfs->ROMFS_pcSector);
        __SHEAP_FREE(promfs);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "format unknown.\r\n");
        _ErrorHandle(ERROR_IO_VOLUME_ERROR);
        return  (PX_ERROR);
    }
    
    promfs->ROMFS_uid = getuid();
    promfs->ROMFS_gid = getgid();
    
    lib_time(&promfs->ROMFS_time);                                      /*  ��õ�ǰʱ��                */
    
    if (iosDevAddEx(&promfs->ROMFS_devhdrHdr, pcName, _G_iRomfsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  ��װ�ļ�ϵͳ�豸            */
        API_SemaphoreMDelete(&promfs->ROMFS_hVolLock);
        __SHEAP_FREE(promfs->ROMFS_pcSector);
        __SHEAP_FREE(promfs);
        return  (PX_ERROR);
    }
    
    __fsDiskLinkCounterAdd(pblkd);                                      /*  ���ӿ��豸����              */

    _DebugFormat(__LOGMESSAGE_LEVEL, "target \"%s\" mount ok.\r\n", pcName);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_RomFsDevDelete
** ��������: ɾ��һ�� romfs �ļ�ϵͳ�豸, ����: API_RomFsDevDelete("/mnt/rom0");
** �䡡��  : pcName            �ļ�ϵͳ�豸��(�����豸�ҽӵĽڵ��ַ)
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_RomFsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  ������豸, �����ж���豸  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __romFsOpen
** ��������: �򿪻��ߴ����ļ�
** �䡡��  : promfs           romfs �ļ�ϵͳ
**           pcName           �ļ���
**           iFlags           ��ʽ
**           iMode            mode_t
** �䡡��  : < 0 ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG __romFsOpen (PROM_VOLUME     promfs,
                         PCHAR           pcName,
                         INT             iFlags,
                         INT             iMode)
{
    INT             iError;
    PROM_FILE       promfile;
    PLW_FD_NODE     pfdnode;
    size_t          stSize;
    BOOL            bIsNew;
    struct stat    *pstat;
    
    INT             iFollowLinkType;
    PCHAR           pcTail, pcSymfile, pcPrefix;
    
    if (pcName == LW_NULL) {                                            /*  ���ļ���                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        stSize   = sizeof(ROM_FILE) + lib_strlen(pcName);
        promfile = (PROM_FILE)__SHEAP_ALLOC(stSize);
        if (promfile == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_bzero(promfile, stSize);
        lib_strcpy(promfile->ROMFIL_cName, pcName);                     /*  ��¼�ļ���                  */
        
        promfile->ROMFIL_promfs = promfs;
        
        if (__ROMFS_VOL_LOCK(promfs) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        if (__STR_IS_ROOT(promfile->ROMFIL_cName)) {
            promfile->ROMFIL_iFileType = __ROMFS_FILE_TYPE_DEV;
            goto    __file_open_ok;                                     /*  �豸������                */
        }
        
        if (promfs->ROMFS_bValid == LW_FALSE) {
            __ROMFS_VOL_UNLOCK(promfs);
            __SHEAP_FREE(promfile);
            _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
            return  (PX_ERROR);
        }
        
        iError = __rfs_open(promfs, pcName, &pcTail, &pcSymfile, &promfile->ROMFIL_romfsdnt);
        if (iError) {
            __ROMFS_VOL_UNLOCK(promfs);
            __SHEAP_FREE(promfile);
            if (iFlags & O_CREAT) {
                _ErrorHandle(EROFS);                                    /*  ֻ���ļ�ϵͳ                */
            }
            return  (PX_ERROR);
        }
        
        /*
         *  ���ȴ�����������ļ����
         */
        if (S_ISLNK(promfile->ROMFIL_romfsdnt.ROMFSDNT_stat.st_mode)) {
            pcSymfile--;                                                /*  �� / ��ʼ                   */
            if (pcSymfile == pcName) {
                pcPrefix = LW_NULL;                                     /*  û��ǰ׺                    */
            } else {
                pcPrefix = pcName;
                *pcSymfile = PX_EOS;
            }
            if (pcTail && lib_strlen(pcTail)) {
                iFollowLinkType = FOLLOW_LINK_TAIL;                     /*  ����Ŀ���ڲ��ļ�            */
            } else {
                iFollowLinkType = FOLLOW_LINK_FILE;                     /*  �����ļ�����                */
            }
        
            if (__rfs_path_build_link(promfs, &promfile->ROMFIL_romfsdnt, pcName, PATH_MAX + 1,
                                      pcPrefix, pcTail) == ERROR_NONE) {/*  ��������Ŀ��                */
                __ROMFS_VOL_UNLOCK(promfs);
                __SHEAP_FREE(promfile);
                return  (iFollowLinkType);
            
            } else {                                                    /*  ��������ʧ��                */
                __ROMFS_VOL_UNLOCK(promfs);
                __SHEAP_FREE(promfile);
                return  (PX_ERROR);
            }
        
        } else if (S_ISDIR(promfile->ROMFIL_romfsdnt.ROMFSDNT_stat.st_mode)) {
            promfile->ROMFIL_iFileType = __ROMFS_FILE_TYPE_DIR;
        
        } else {
            if (iFlags & O_DIRECTORY) {
                __ROMFS_VOL_UNLOCK(promfs);
                __SHEAP_FREE(promfile);
                _ErrorHandle(ENOTDIR);
                return  (PX_ERROR);
            }
            promfile->ROMFIL_iFileType = __ROMFS_FILE_TYPE_NODE;
        }
        
__file_open_ok:
        pstat   = &promfile->ROMFIL_romfsdnt.ROMFSDNT_stat;
        pfdnode = API_IosFdNodeAdd(&promfs->ROMFS_plineFdNodeHeader,
                                   pstat->st_dev,
                                   (ino64_t)pstat->st_ino,
                                   iFlags,
                                   iMode,
                                   pstat->st_uid,
                                   pstat->st_gid,
                                   pstat->st_size,
                                   (PVOID)promfile,
                                   &bIsNew);                            /*  ����ļ��ڵ�                */
        if (pfdnode == LW_NULL) {                                       /*  �޷����� fd_node �ڵ�       */
            __ROMFS_VOL_UNLOCK(promfs);
            __SHEAP_FREE(promfile);
            return  (PX_ERROR);
        }
        
        LW_DEV_INC_USE_COUNT(&promfs->ROMFS_devhdrHdr);                 /*  ���¼�����                  */
        
        __ROMFS_VOL_UNLOCK(promfs);
        
        if (bIsNew == LW_FALSE) {                                       /*  ���ظ���                  */
            __SHEAP_FREE(promfile);
        }
        
        return  ((LONG)pfdnode);                                        /*  �����ļ��ڵ�                */
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __romFsRemove
** ��������: romfs remove ����
** �䡡��  : promfs           ���豸
**           pcName           �ļ���
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __romFsRemove (PROM_VOLUME   promfs,
                           PCHAR         pcName)
{
    PLW_BLK_DEV    pblkd;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    if (__STR_IS_ROOT(pcName)) {
        if (__ROMFS_VOL_LOCK(promfs) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        
        if (promfs->ROMFS_bValid == LW_FALSE) {
            __ROMFS_VOL_UNLOCK(promfs);
            return  (ERROR_NONE);                                       /*  ���ڱ���������ж��          */
        }

__re_umount_vol:
        if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)promfs)) {               /*  ����Ƿ������ڹ������ļ�    */
            if (!promfs->ROMFS_bForceDelete) {
                __ROMFS_VOL_UNLOCK(promfs);
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            }
            
            promfs->ROMFS_bValid = LW_FALSE;
            
            __ROMFS_VOL_UNLOCK(promfs);
            
            _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
            iosDevFileAbnormal(&promfs->ROMFS_devhdrHdr);               /*  ����������ļ���Ϊ�쳣ģʽ  */
            
            __ROMFS_VOL_LOCK(promfs);
            goto    __re_umount_vol;
        
        } else {
            promfs->ROMFS_bValid = LW_FALSE;
        }
        
        pblkd = promfs->ROMFS_pblkd;
        if (pblkd) {
            __fsDiskLinkCounterDec(pblkd);                              /*  �������Ӵ���                */
        }

        iosDevDelete((LW_DEV_HDR *)promfs);                             /*  IO ϵͳ�Ƴ��豸             */
        API_SemaphoreMDelete(&promfs->ROMFS_hVolLock);
        
        __SHEAP_FREE(promfs->ROMFS_pcSector);
        __SHEAP_FREE(promfs);

        _DebugHandle(__LOGMESSAGE_LEVEL, "romfs unmount ok.\r\n");
        
        return  (ERROR_NONE);
    
    } else {
        PCHAR           pcTail = LW_NULL, pcSymfile, pcPrefix;
        ROMFS_DIRENT    romfsdnt;
    
        if (__ROMFS_VOL_LOCK(promfs) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        if (__rfs_open(promfs, pcName, &pcTail, &pcSymfile, &romfsdnt) == ERROR_NONE) {
            if (S_ISLNK(romfsdnt.ROMFSDNT_stat.st_mode)) {
                if (pcTail && lib_strlen(pcTail)) {                     /*  �����ļ��к�׺              */
                    pcSymfile--;                                        /*  �� / ��ʼ                   */
                    if (pcSymfile == pcName) {
                        pcPrefix = LW_NULL;                             /*  û��ǰ׺                    */
                    } else {
                        pcPrefix = pcName;
                        *pcSymfile = PX_EOS;
                    }
                    if (__rfs_path_build_link(promfs, &romfsdnt, pcName, PATH_MAX + 1,
                                              pcPrefix, pcTail) == ERROR_NONE) {
                        __ROMFS_VOL_UNLOCK(promfs);
                        return  (FOLLOW_LINK_TAIL);
                    }
                }
            }
        }
        __ROMFS_VOL_UNLOCK(promfs);
        
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __romFsClose
** ��������: romfs close ����
** �䡡��  : pfdentry         �ļ����ƿ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __romFsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;
    PROM_VOLUME   promfs   = promfile->ROMFIL_promfs;
    BOOL          bFree    = LW_FALSE;

    if (promfile) {
        if (__ROMFS_VOL_LOCK(promfs) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        if (API_IosFdNodeDec(&promfs->ROMFS_plineFdNodeHeader,
                             pfdnode, LW_NULL) == 0) {
            bFree = LW_TRUE;
        }
        
        LW_DEV_DEC_USE_COUNT(&promfs->ROMFS_devhdrHdr);
        
        __ROMFS_VOL_UNLOCK(promfs);
        
        if (bFree) {
            __SHEAP_FREE(promfile);
        }
        
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __romFsRead
** ��������: romfs read ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __romFsRead (PLW_FD_ENTRY pfdentry,
                             PCHAR        pcBuffer,
                             size_t       stMaxBytes)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;
    ssize_t       sstRet;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (promfile->ROMFIL_iFileType != __ROMFS_FILE_TYPE_NODE) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__ROMFS_FILE_LOCK(promfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes) {
        sstRet = __rfs_pread(promfile->ROMFIL_promfs, &promfile->ROMFIL_romfsdnt, 
                             pcBuffer, stMaxBytes, (UINT32)pfdentry->FDENTRY_oftPtr);
        if (sstRet > 0) {
            pfdentry->FDENTRY_oftPtr += (off_t)sstRet;                  /*  �����ļ�ָ��                */
        }
        
    } else {
        sstRet = 0;
    }
                         
    __ROMFS_FILE_UNLOCK(promfile);
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: __romFsPRead
** ��������: romfs pread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __romFsPRead (PLW_FD_ENTRY pfdentry,
                              PCHAR        pcBuffer,
                              size_t       stMaxBytes,
                              off_t        oftPos)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;
    ssize_t       sstRet;
    
    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (promfile->ROMFIL_iFileType != __ROMFS_FILE_TYPE_NODE) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__ROMFS_FILE_LOCK(promfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes) {
        sstRet = __rfs_pread(promfile->ROMFIL_promfs, &promfile->ROMFIL_romfsdnt, 
                             pcBuffer, stMaxBytes, (UINT32)oftPos);
    } else {
        sstRet = 0;
    }
    
    __ROMFS_FILE_UNLOCK(promfile);
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: __romFsWrite
** ��������: romfs write ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __romFsWrite (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes)
{
    _ErrorHandle(EROFS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __romFsPWrite
** ��������: romfs pwrite ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __romFsPWrite (PLW_FD_ENTRY  pfdentry,
                               PCHAR         pcBuffer,
                               size_t        stNBytes,
                               off_t         oftPos)
{
    _ErrorHandle(EROFS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __romFsNRead
** ��������: romFs nread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           piNRead          ʣ��������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __romFsNRead (PLW_FD_ENTRY  pfdentry, INT  *piNRead)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;

    if (piNRead == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (promfile->ROMFIL_iFileType != __ROMFS_FILE_TYPE_NODE) {
        return  (PX_ERROR);
    }
    
    if (__ROMFS_FILE_LOCK(promfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    *piNRead = (INT)(promfile->ROMFIL_romfsdnt.ROMFSDNT_stat.st_size - 
                     (UINT32)pfdentry->FDENTRY_oftPtr);
    __ROMFS_FILE_UNLOCK(promfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __romFsNRead64
** ��������: romFs nread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           poftNRead        ʣ��������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __romFsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftNRead)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;

    if (promfile->ROMFIL_iFileType != __ROMFS_FILE_TYPE_NODE) {
        return  (PX_ERROR);
    }
    
    if (__ROMFS_FILE_LOCK(promfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    *poftNRead = (off_t)(promfile->ROMFIL_romfsdnt.ROMFSDNT_stat.st_size - 
                         (UINT32)pfdentry->FDENTRY_oftPtr);
    __ROMFS_FILE_UNLOCK(promfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __romFsSeek
** ��������: romFs seek ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           oftOffset        ƫ����
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __romFsSeek (PLW_FD_ENTRY  pfdentry,
                         off_t         oftOffset)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;

    if (promfile->ROMFIL_iFileType != __ROMFS_FILE_TYPE_NODE) {
        return  (PX_ERROR);
    }
    
    if (oftOffset > promfile->ROMFIL_romfsdnt.ROMFSDNT_stat.st_size) {
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    }
    
    if (__ROMFS_FILE_LOCK(promfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (promfile->ROMFIL_iFileType != __ROMFS_FILE_TYPE_NODE) {
        __ROMFS_FILE_UNLOCK(promfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    pfdentry->FDENTRY_oftPtr = oftOffset;
    
    __ROMFS_FILE_UNLOCK(promfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __romFsWhere
** ��������: romFs ����ļ���ǰ��дָ��λ�� (ʹ�ò�����Ϊ����ֵ, �� FIOWHERE ��Ҫ�����в�ͬ)
** �䡡��  : pfdentry            �ļ����ƿ�
**           poftPos             ��дָ��λ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __romFsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    if (poftPos) {
        *poftPos = (off_t)pfdentry->FDENTRY_oftPtr;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __romFsStatGet
** ��������: romFs stat ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pstat            �ļ�״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __romFsStat (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;
    PROM_VOLUME   promfs   = promfile->ROMFIL_promfs;

    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (promfile->ROMFIL_iFileType == __ROMFS_FILE_TYPE_DEV) {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&promfs->ROMFS_devhdrHdr);
        pstat->st_ino     = (ino_t)0;                                   /*  ���������ļ��ظ�(��Ŀ¼)    */
        pstat->st_mode    = (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH | S_IFDIR);
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 0;
        pstat->st_size    = 0;
        pstat->st_blksize = promfs->ROMFS_ulSectorSize;
        pstat->st_blocks  = 0;
        
        pstat->st_atime   = promfs->ROMFS_time;
        pstat->st_mtime   = promfs->ROMFS_time;
        pstat->st_ctime   = promfs->ROMFS_time;
        
        return  (ERROR_NONE);
        
    } else {
        *pstat = promfile->ROMFIL_romfsdnt.ROMFSDNT_stat;
        
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __romFsLStat
** ��������: romFs stat ����
** �䡡��  : promfs           romfs �ļ�ϵͳ
**           pcName           �ļ���
**           pstat            �ļ�״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __romFsLStat (PROM_VOLUME     promfs, PCHAR  pcName, struct stat *pstat)
{
    ROMFS_DIRENT    romfsdnt;
    PCHAR           pcTail, pcSymfile;
    INT             iError;

    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__ROMFS_VOL_LOCK(promfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    iError = __rfs_open(promfs, pcName, &pcTail, &pcSymfile, &romfsdnt);
    if (iError) {
        __ROMFS_VOL_UNLOCK(promfs);
        
        if (__STR_IS_ROOT(pcName)) {
            pstat->st_dev     = LW_DEV_MAKE_STDEV(&promfs->ROMFS_devhdrHdr);
            pstat->st_ino     = (ino_t)0;                               /*  ���������ļ��ظ�(��Ŀ¼)    */
            pstat->st_mode    = (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH | S_IFDIR);
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 0;
            pstat->st_size    = 0;
            pstat->st_blksize = promfs->ROMFS_ulSectorSize;
            pstat->st_blocks  = 0;
            
            pstat->st_atime   = promfs->ROMFS_time;
            pstat->st_mtime   = promfs->ROMFS_time;
            pstat->st_ctime   = promfs->ROMFS_time;
            
            return  (ERROR_NONE);
        
        } else {
            return  (PX_ERROR);
        }
    }
    __ROMFS_VOL_UNLOCK(promfs);
    
    *pstat = romfsdnt.ROMFSDNT_stat;
        
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __romFsStatfs
** ��������: romFs statfs ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pstatfs          �ļ�ϵͳ״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __romFsStatfs (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE     promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;

    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pstatfs->f_type  = ROMFS_MAGIC;                                     /* type of info, zero for now   */
    pstatfs->f_bsize = promfile->ROMFIL_promfs->ROMFS_ulSectorSize;
    
    pstatfs->f_blocks = (promfile->ROMFIL_promfs->ROMFS_uiTotalSize 
                      / promfile->ROMFIL_promfs->ROMFS_ulSectorSize)
                      + 1;
    pstatfs->f_bfree  = 0;
    pstatfs->f_bavail = 1;
    
    pstatfs->f_files   = 0;                                             /* total file nodes in fs       */
    pstatfs->f_ffree   = 0;                                             /* free file nodes in fs        */
    
#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatfs->f_fsid.val[0] = (int32_t)((addr_t)promfile->ROMFIL_promfs >> 32);
    pstatfs->f_fsid.val[1] = (int32_t)((addr_t)promfile->ROMFIL_promfs & 0xffffffff);
#else
    pstatfs->f_fsid.val[0] = (int32_t)promfile->ROMFIL_promfs;
    pstatfs->f_fsid.val[1] = 0;
#endif
    
    pstatfs->f_flag    = ST_RDONLY;
    pstatfs->f_namelen = PATH_MAX;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __romFsReadDir
** ��������: romFs ���ָ��Ŀ¼��Ϣ
** �䡡��  : pfdentry            �ļ����ƿ�
**           dir                 Ŀ¼�ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __romFsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE       promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;
    ROMFS_DIRENT    romfsdnt;
    INT             iError;

    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (promfile->ROMFIL_iFileType == __ROMFS_FILE_TYPE_NODE) {
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    if (dir->dir_pos == 0) {
        if (promfile->ROMFIL_iFileType == __ROMFS_FILE_TYPE_DEV) {
            promfile->ROMFIL_ulCookieDir = promfile->ROMFIL_promfs->ROMFS_uiRootAddr;
        
        } else if (promfile->ROMFIL_iFileType == __ROMFS_FILE_TYPE_DIR) {
            if (promfile->ROMFIL_romfsdnt.ROMFSDNT_uiSpec == 
                promfile->ROMFIL_romfsdnt.ROMFSDNT_uiMe) {
                promfile->ROMFIL_ulCookieDir = 0;                       /*  ��Ŀ¼��û���ļ�            */
            
            } else {
                promfile->ROMFIL_ulCookieDir = promfile->ROMFIL_romfsdnt.ROMFSDNT_uiSpec;
            }
        
        } else {
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
    }

__get_next:
    if (promfile->ROMFIL_ulCookieDir == 0) {                            /*  ��������                    */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (__ROMFS_FILE_LOCK(promfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    if (__rfs_getfile(promfile->ROMFIL_promfs, promfile->ROMFIL_ulCookieDir, &romfsdnt)) {
        __ROMFS_FILE_UNLOCK(promfile);
        return  (PX_ERROR);
    }
    __ROMFS_FILE_UNLOCK(promfile);
    
    if ((lib_strcmp(romfsdnt.ROMFSDNT_cName, ".") == 0) || 
        (lib_strcmp(romfsdnt.ROMFSDNT_cName, "..") == 0)) {             /*  ���� . �� ..                */
        promfile->ROMFIL_ulCookieDir = romfsdnt.ROMFSDNT_uiNext;        /*  ��¼��һ��λ��              */
        goto    __get_next;
    }
    
    lib_strcpy(dir->dir_dirent.d_name, romfsdnt.ROMFSDNT_cName);
    dir->dir_dirent.d_shortname[0] = PX_EOS;                            /*  ��֧�ֶ��ļ���              */
    dir->dir_dirent.d_type = IFTODT(romfsdnt.ROMFSDNT_stat.st_mode);
    dir->dir_pos++;
    iError = ERROR_NONE;
    
    promfile->ROMFIL_ulCookieDir = romfsdnt.ROMFSDNT_uiNext;            /*  ��¼��һ��λ��              */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __romFsReadlink
** ��������: romFs ��ȡ���������ļ�����
** �䡡��  : promfs              romfs �ļ�ϵͳ
**           pcName              ����ԭʼ�ļ���
**           pcLinkDst           ����Ŀ���ļ���
**           stMaxSize           �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t __romFsReadlink (PROM_VOLUME   promfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize)
{
    ROMFS_DIRENT    romfsdnt;
    PCHAR           pcTail, pcSymfile;
    INT             iError;
    ssize_t         sstRet;
    
    if (__ROMFS_VOL_LOCK(promfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    iError = __rfs_open(promfs, pcName, &pcTail, &pcSymfile, &romfsdnt);
    if (iError) {
        __ROMFS_VOL_UNLOCK(promfs);
        return  (PX_ERROR);
    }
    
    if (!pcTail || (*pcTail == PX_EOS)) {                               /*  �����ļ�                    */
        if (!S_ISLNK(romfsdnt.ROMFSDNT_stat.st_mode)) {
            __ROMFS_VOL_UNLOCK(promfs);
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        
        sstRet = __rfs_readlink(promfs, &romfsdnt, pcLinkDst, stMaxSize);
        __ROMFS_VOL_UNLOCK(promfs);
        
        return  (sstRet);
    }
    __ROMFS_VOL_UNLOCK(promfs);
    
    _ErrorHandle(EINVAL);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __romFsIoctl
** ��������: romFs ioctl ����
** �䡡��  : pfdentry           �ļ����ƿ�
**           request,           ����
**           arg                �������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __romFsIoctl (PLW_FD_ENTRY  pfdentry,
                          INT           iRequest,
                          LONG          lArg)
{
    PLW_FD_NODE     pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PROM_FILE       promfile = (PROM_FILE)pfdnode->FDNODE_pvFile;
    off_t           oftTemp;
    INT             iError;
    
    switch (iRequest) {

    case FIOCONTIG:
    case FIOTRUNC:
    case FIOLABELSET:
    case FIOATTRIBSET:
        _ErrorHandle(EROFS);
        return (PX_ERROR);
    }
    
    switch (iRequest) {

    case FIODISKINIT:                                                   /*  ���̳�ʼ��                  */
        return  (ERROR_NONE);
    
    /*
     *  FIOSEEK, FIOWHERE is 64 bit operate.
     */
    case FIOSEEK:                                                       /*  �ļ��ض�λ                  */
        oftTemp = *(off_t *)lArg;
        return  (__romFsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  ����ļ���ǰ��дָ��        */
        iError = __romFsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIONREAD:                                                      /*  ����ļ�ʣ���ֽ���          */
        return  (__romFsNRead(pfdentry, (INT *)lArg));
        
    case FIONREAD64:                                                    /*  ����ļ�ʣ���ֽ���          */
        iError = __romFsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
        
    case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
        return  (__romFsStat(pfdentry, (struct stat *)lArg));

    case FIOFSTATFSGET:                                                 /*  ����ļ�ϵͳ״̬            */
        return  (__romFsStatfs(pfdentry, (struct statfs *)lArg));

    case FIOREADDIR:                                                    /*  ��ȡһ��Ŀ¼��Ϣ            */
        return  (__romFsReadDir(pfdentry, (DIR *)lArg));
        
    case FIOSETFL:                                                      /*  �����µ� flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);

    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "ROM FileSystem";
        return  (ERROR_NONE);
    
    case FIOGETFORCEDEL:                                                /*  ǿ��ж���豸�Ƿ�����      */
        *(BOOL *)lArg = promfile->ROMFIL_promfs->ROMFS_bForceDelete;
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
                                                                        /*  LW_CFG_ROMFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
