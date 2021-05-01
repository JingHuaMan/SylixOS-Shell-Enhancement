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
** ��   ��   ��: memdev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 06 �� 04 ��
**
** ��        ��: VxWorks �ڴ��豸���ݽӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_MEMDEV_EN > 0)
/*********************************************************************************************************
  �豸�ṹ
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          MEM_devhdr;
    MEM_DRV_DIRENTRY    MEM_memdir;
    INT                 MEM_iAllowOft;                                  /*  �Ƿ���������ƫ����          */
    time_t              MEM_time;
    uid_t               MEM_uid;
    gid_t               MEM_gid;
} LW_MEM_DEV;
typedef LW_MEM_DEV     *PLW_MEM_DEV;

typedef struct {
    PLW_MEM_DEV         MEMF_memdev;
    MEM_DRV_DIRENTRY   *MEMF_pmemdir;
    size_t              MEMF_stOffset;                                  /*  ��ǰ�ļ�ƫ����              */
} LW_MEM_FILE;
typedef LW_MEM_FILE    *PLW_MEM_FILE;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static INT      _G_iMemDrvNum = PX_ERROR;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LONG     _MemOpen(PLW_MEM_DEV   pmemdev, 
                         PCHAR         pcName,   
                         INT           iFlags, 
                         INT           iMode);
static INT      _MemClose(PLW_MEM_FILE  pmemfile);
static ssize_t  _MemRead(PLW_MEM_FILE  pmemfile, 
                         PCHAR         pcBuffer, 
                         size_t        stMaxBytes);
static ssize_t  _MemPRead(PLW_MEM_FILE  pmemfile, 
                          PCHAR         pcBuffer, 
                          size_t        stMaxBytes,
                          off_t         oftOffset);
static ssize_t  _MemWrite(PLW_MEM_FILE  pmemfile, 
                          PCHAR         pcBuffer, 
                          size_t        stNBytes);
static ssize_t  _MemPWrite(PLW_MEM_FILE  pmemfile, 
                           PCHAR         pcBuffer, 
                           size_t        stNBytes,
                           off_t         oftOffset);
static INT      _MemIoctl(PLW_MEM_FILE  pmemfile,
                          INT           iRequest,
                          LONG          lArg);
/*********************************************************************************************************
** ��������: API_MemDrvInstall
** ��������: ��װ�ڴ��豸��������
** �䡡��  : VOID
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_MemDrvInstall (VOID)
{
    struct file_operations     fileop;
    
    if (_G_iMemDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner       = THIS_MODULE;
    fileop.fo_open     = _MemOpen;
    fileop.fo_close    = _MemClose;
    fileop.fo_read     = _MemRead;
    fileop.fo_read_ex  = _MemPRead;
    fileop.fo_write    = _MemWrite;
    fileop.fo_write_ex = _MemPWrite;
    fileop.fo_ioctl    = _MemIoctl;

    _G_iMemDrvNum =  iosDrvInstallEx(&fileop);
    
    DRIVER_LICENSE(_G_iMemDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iMemDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iMemDrvNum, "VxWorks memory device driver.");
    
    return  ((_G_iMemDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_MemDevCreate
** ��������: ����һ���ڴ��豸
** �䡡��  : name      �豸��
**           base      �ڴ����ַ
**           length    �ڴ泤��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_MemDevCreate (char *name, char *base, size_t length)
{
    PLW_MEM_DEV  pmemdev;
    
    if (!name || !length) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmemdev = (PLW_MEM_DEV)__SHEAP_ALLOC(sizeof(LW_MEM_DEV));
    if (pmemdev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pmemdev, sizeof(LW_MEM_DEV));
    
    pmemdev->MEM_memdir.name   = "";
    pmemdev->MEM_memdir.base   = base;
    pmemdev->MEM_memdir.pDir   = LW_NULL;
    pmemdev->MEM_memdir.length = length;
    pmemdev->MEM_iAllowOft     = 1;
    
    pmemdev->MEM_uid = getuid();
    pmemdev->MEM_gid = getgid();
    
    if (iosDevAddEx(&pmemdev->MEM_devhdr, name, _G_iMemDrvNum, DT_REG) != ERROR_NONE) {
        __SHEAP_FREE(pmemdev);
        return  (PX_ERROR);
    }
    
    pmemdev->MEM_time = lib_time(LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MemDevCreateDir
** ��������: ����һ���ڴ��豸Ŀ¼
** �䡡��  : name      �豸��
**           files     �ڴ��ļ��б�
**           numFiles  �ļ�����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_MemDevCreateDir (char *name, MEM_DRV_DIRENTRY *files, int numFiles)
{
    PLW_MEM_DEV  pmemdev;
    
    if (!name || !files || (numFiles < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmemdev = (PLW_MEM_DEV)__SHEAP_ALLOC(sizeof(LW_MEM_DEV));
    if (pmemdev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pmemdev, sizeof(LW_MEM_DEV));
    
    pmemdev->MEM_memdir.name   = "";
    pmemdev->MEM_memdir.base   = LW_NULL;
    pmemdev->MEM_memdir.pDir   = files;
    pmemdev->MEM_memdir.length = (size_t)numFiles;
    pmemdev->MEM_iAllowOft     = 0;
    
    if (iosDevAddEx(&pmemdev->MEM_devhdr, name, _G_iMemDrvNum, DT_DIR) != ERROR_NONE) {
        __SHEAP_FREE(pmemdev);
        return  (PX_ERROR);
    }
    
    pmemdev->MEM_time = lib_time(LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_MemDevDelete
** ��������: ɾ��һ���ڴ��豸
** �䡡��  : name      �豸��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_MemDevDelete (char *name)
{
    PLW_MEM_DEV   pmemdev;
    PCHAR         pcTail = LW_NULL;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    pmemdev = (PLW_MEM_DEV)iosDevFind(name, &pcTail);
    if ((pmemdev == LW_NULL) || (name == pcTail)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device not found.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    if (LW_DEV_GET_USE_COUNT(&pmemdev->MEM_devhdr)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "too many open files.\r\n");
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    iosDevFileAbnormal(&pmemdev->MEM_devhdr);
    iosDevDelete(&pmemdev->MEM_devhdr);
    
    __SHEAP_FREE(pmemdev);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _MemFind
** ��������: ����һ���ڴ��ļ�
** �䡡��  : pmemdev          �ڴ��豸���ƿ�
**           pcName           ����
**           pmemdir          ����Ŀ¼
**           pstFileOft       �ļ�ƫ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static MEM_DRV_DIRENTRY *_MemFind (PLW_MEM_DEV          pmemdev, 
                                   PCHAR                pcName, 
                                   MEM_DRV_DIRENTRY    *pmemdir, 
                                   size_t              *pstFileOft)
{
    MEM_DRV_DIRENTRY  *pmemdirFile = LW_NULL;
    
    *pstFileOft = 0;
    
    if (*pcName == PX_ROOT) {
        pcName++;
    }
    
    if (lib_strcmp(pmemdir->name, pcName) == 0) {
        pmemdirFile = pmemdir;
    
    } else if (lib_strncmp(pmemdir->name, pcName, lib_strlen(pmemdir->name)) == 0) {
        INT  i;
        pcName += lib_strlen(pmemdir->name);
        if (pmemdir->pDir != LW_NULL) {                                 /*  ������Ŀ¼                  */
	        for (i = 0; i < pmemdir->length; i++) {
	            pmemdirFile = _MemFind(pmemdev, pcName, &pmemdir->pDir[i], pstFileOft);
	            if (pmemdirFile) {
	                break;
	            }
	        }
        }
        
    } else if (pmemdev->MEM_iAllowOft) {
        size_t  stOff = 0;
        if (*pcName == PX_ROOT) {
            pcName++;
        }
        if (sscanf(pcName, "%zu", &stOff) == 1) {
            pmemdirFile = pmemdir;
            *pstFileOft = stOff;
        }
    }
    
    return  (pmemdirFile);
}
/*********************************************************************************************************
** ��������: _MemOpen
** ��������: ���ڴ��豸
** �䡡��  : pmemdev          �ڴ��豸���ƿ�
**           pcName           ����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  _MemOpen (PLW_MEM_DEV   pmemdev, 
                       PCHAR         pcName,   
                       INT           iFlags, 
                       INT           iMode)
{
    MEM_DRV_DIRENTRY  *pmemdirFile;
    PLW_MEM_FILE       pmemfile;
    size_t             stOft;
    
    if (iFlags & O_CREAT) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    pmemdirFile = _MemFind(pmemdev, pcName, &pmemdev->MEM_memdir, &stOft);
    if (pmemdirFile == LW_NULL) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    pmemfile = (PLW_MEM_FILE)__SHEAP_ALLOC(sizeof(LW_MEM_FILE));
    if (pmemfile == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    
    pmemfile->MEMF_memdev   = pmemdev;
    pmemfile->MEMF_pmemdir  = pmemdirFile;
    pmemfile->MEMF_stOffset = stOft;
    
    LW_DEV_INC_USE_COUNT(&pmemdev->MEM_devhdr);
    
    return  ((LONG)pmemfile);
}
/*********************************************************************************************************
** ��������: _MemClose
** ��������: �ر��ڴ��豸
** �䡡��  : pmemfile          �ڴ��豸�ļ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _MemClose (PLW_MEM_FILE  pmemfile)
{
    if (pmemfile) {
        LW_DEV_DEC_USE_COUNT(&pmemfile->MEMF_memdev->MEM_devhdr);
        __SHEAP_FREE(pmemfile);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _MemRead
** ��������: ���ڴ��ļ�
** �䡡��  : pmemfile         �ڴ��豸�ļ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _MemRead (PLW_MEM_FILE  pmemfile, 
                          PCHAR         pcBuffer, 
                          size_t        stMaxBytes)
{
    MEM_DRV_DIRENTRY  *pmemdirFile;
    size_t             stCopeBytes;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!stMaxBytes) {
        return  (0);
    }

    pmemdirFile = pmemfile->MEMF_pmemdir;
    if (pmemdirFile->pDir) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pmemfile->MEMF_stOffset >= pmemdirFile->length) {
        return  (0);
    }
    
    stCopeBytes = __MIN(stMaxBytes, pmemdirFile->length - pmemfile->MEMF_stOffset);
    lib_memcpy(pcBuffer, (CPVOID)(pmemdirFile->base + pmemfile->MEMF_stOffset), stCopeBytes);
    
    pmemfile->MEMF_stOffset += stCopeBytes;
    
    return  ((size_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: _MemPRead
** ��������: ��չ���ڴ��ļ�
** �䡡��  : pmemfile         �ڴ��豸�ļ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
**           oftOffset        �ļ�ƫ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _MemPRead (PLW_MEM_FILE  pmemfile, 
                           PCHAR         pcBuffer, 
                           size_t        stMaxBytes,
                           off_t         oftOffset)
{
    MEM_DRV_DIRENTRY  *pmemdirFile;
    size_t             stCopeBytes;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!stMaxBytes) {
        return  (0);
    }

    pmemdirFile = pmemfile->MEMF_pmemdir;
    if (pmemdirFile->pDir) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (oftOffset >= pmemdirFile->length) {
        return  (0);
    }
    
    stCopeBytes = __MIN(stMaxBytes, pmemdirFile->length - (size_t)oftOffset);
    lib_memcpy(pcBuffer, (CPVOID)(pmemdirFile->base + (size_t)oftOffset), stCopeBytes);
    
    return  ((size_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: _MemWrite
** ��������: д�ڴ��ļ�
** �䡡��  : pmemfile         �ڴ��豸�ļ�
**           pcBuffer         ���ͻ�����
**           stNBytes         д����Ŀ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _MemWrite (PLW_MEM_FILE  pmemfile, 
                           PCHAR         pcBuffer, 
                           size_t        stNBytes)
{
    MEM_DRV_DIRENTRY  *pmemdirFile;
    size_t             stCopeBytes;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!stNBytes) {
        return  (0);
    }

    pmemdirFile = pmemfile->MEMF_pmemdir;
    if (pmemdirFile->pDir) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pmemfile->MEMF_stOffset >= pmemdirFile->length) {
        return  (0);
    }
    
    stCopeBytes = __MIN(stNBytes, pmemdirFile->length - pmemfile->MEMF_stOffset);
    lib_memcpy((PVOID)(pmemdirFile->base + pmemfile->MEMF_stOffset), pcBuffer, stCopeBytes);
    
    pmemfile->MEMF_stOffset += stCopeBytes;
    
    KN_SMP_MB();
    
    return  ((size_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: _MemPWrite
** ��������: ��չд�ڴ��ļ�
** �䡡��  : pmemfile         �ڴ��豸�ļ�
**           pcBuffer         ���ͻ�����
**           stNBytes         д����Ŀ
**           oftOffset        �ļ�ƫ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _MemPWrite (PLW_MEM_FILE  pmemfile, 
                            PCHAR         pcBuffer, 
                            size_t        stNBytes,
                            off_t         oftOffset)
{
    MEM_DRV_DIRENTRY  *pmemdirFile;
    size_t             stCopeBytes;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!stNBytes) {
        return  (0);
    }

    pmemdirFile = pmemfile->MEMF_pmemdir;
    if (pmemdirFile->pDir) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (oftOffset >= pmemdirFile->length) {
        return  (0);
    }
    
    stCopeBytes = __MIN(stNBytes, pmemdirFile->length - (size_t)oftOffset);
    lib_memcpy((PVOID)(pmemdirFile->base + (size_t)oftOffset), pcBuffer, stCopeBytes);
    
    KN_SMP_MB();
    
    return  ((size_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: _MemStats
** ��������: ����ļ���Ϣ
** �䡡��  : pmemfile         �ڴ��豸�ļ�
**           pstat            �ļ���Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _MemStats (PLW_MEM_FILE  pmemfile, struct stat *pstat)
{
    BOOL               bIsDir;
    PLW_MEM_DEV        pmemdev;
    MEM_DRV_DIRENTRY  *pmemdirFile;
    
    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmemdev     = pmemfile->MEMF_memdev;
    pmemdirFile = pmemfile->MEMF_pmemdir;
    bIsDir      = (pmemdirFile->pDir) ? LW_TRUE : LW_FALSE;
    
    pstat->st_dev = LW_DEV_MAKE_STDEV(&pmemdev->MEM_devhdr);
    pstat->st_ino = (ino_t)pmemdirFile;
    
    if (bIsDir) {
        pstat->st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    } else {
        pstat->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    }
    
    pstat->st_nlink = 1;
    pstat->st_uid   = pmemdev->MEM_uid;
    pstat->st_gid   = pmemdev->MEM_gid;
    pstat->st_rdev  = 1;
    
    if (bIsDir) {
        pstat->st_size   = 0;
        pstat->st_blocks = 1;
    } else {
        pstat->st_size   = (off_t)pmemdirFile->length;
        pstat->st_blocks = (blkcnt_t)pmemdirFile->length;
    }
    
    pstat->st_atime   = pmemdev->MEM_time;
    pstat->st_mtime   = pmemdev->MEM_time;
    pstat->st_ctime   = pmemdev->MEM_time;
    pstat->st_blksize = 1;
    
    pstat->st_resv1 = LW_NULL;
    pstat->st_resv2 = LW_NULL;
    pstat->st_resv3 = LW_NULL;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _MemStatsfs
** ��������: ����ļ�ϵͳ��Ϣ
** �䡡��  : pmemfile         �ڴ��豸�ļ�
**           pstatfs          �ļ�ϵͳ��Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _MemStatsfs (PLW_MEM_FILE  pmemfile, struct statfs *pstatfs)
{
    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pstatfs->f_type   = DEVFS_SUPER_MAGIC;
    pstatfs->f_bsize  = 0;
    pstatfs->f_blocks = 1;
    pstatfs->f_bfree  = 0;
    pstatfs->f_bavail = 1;
    
    pstatfs->f_files  = 0;
    pstatfs->f_ffree  = 0;
    
#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatfs->f_fsid.val[0] = (int32_t)((addr_t)pmemfile->MEMF_memdev >> 32);
    pstatfs->f_fsid.val[1] = (int32_t)((addr_t)pmemfile->MEMF_memdev & 0xffffffff);
#else
    pstatfs->f_fsid.val[0] = (int32_t)pmemfile->MEMF_memdev;
    pstatfs->f_fsid.val[1] = 0;
#endif
    
    pstatfs->f_flag    = 0;
    pstatfs->f_namelen = PATH_MAX;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _MemReadDir
** ��������: ��ȡĿ¼��Ϣ
** �䡡��  : pmemfile         �ڴ��豸�ļ�
**           dir              Ŀ¼��Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _MemReadDir (PLW_MEM_FILE  pmemfile, DIR  *dir)
{
    BOOL               bIsDir;
    MEM_DRV_DIRENTRY  *pmemdirFile;
    
    pmemdirFile = pmemfile->MEMF_pmemdir;
    bIsDir      = (pmemdirFile->pDir) ? LW_TRUE : LW_FALSE;
    
    if (bIsDir == LW_FALSE) {
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    if (dir->dir_pos >= pmemdirFile->length) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    lib_strlcpy(dir->dir_dirent.d_name, 
                pmemdirFile->pDir[dir->dir_pos].name, 
                sizeof(dir->dir_dirent.d_name));
                
    if (bIsDir) {
        dir->dir_dirent.d_type = DT_DIR;
    } else {
        dir->dir_dirent.d_type = DT_REG;
    }
    
    dir->dir_dirent.d_shortname[0] = PX_EOS;
    
    dir->dir_pos++;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _MemIoctl
** ��������: memdev ioctl ����
** �䡡��  : pmemfile           �ڴ��豸�ļ�
**           request,           ����
**           arg                �������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _MemIoctl (PLW_MEM_FILE  pmemfile,
                       INT           iRequest,
                       LONG          lArg)
{
    BOOL               bIsDir;
    MEM_DRV_DIRENTRY  *pmemdirFile;
    
    pmemdirFile = pmemfile->MEMF_pmemdir;
    bIsDir      = (pmemdirFile->pDir) ? LW_TRUE : LW_FALSE;
    
    switch (iRequest) {
    
    case FIOSEEK:
        if (bIsDir) {
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        } else {
            pmemfile->MEMF_stOffset = (size_t)(*(off_t *)lArg);
        }
        break;
        
    case FIOWHERE:
        if (bIsDir) {
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = (off_t)pmemfile->MEMF_stOffset;
        }
        break;
        
    case FIONREAD:
        if (bIsDir) {
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        } else {
            if (pmemfile->MEMF_stOffset >= pmemdirFile->length) {
                *(INT *)lArg = 0;
            } else {
                *(INT *)lArg = pmemdirFile->length - pmemfile->MEMF_stOffset;
            }
        }
        break;
        
    case FIONREAD64:
        if (bIsDir) {
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        } else {
            if (pmemfile->MEMF_stOffset >= pmemdirFile->length) {
                *(off_t *)lArg = 0;
            } else {
                *(off_t *)lArg = (off_t)pmemdirFile->length - pmemfile->MEMF_stOffset;
            }
        }
        break;
        
    case FIOFSTATGET:
        return  (_MemStats(pmemfile, (struct stat *)lArg));
        
    case FIOFSTATFSGET:
        return  (_MemStatsfs(pmemfile, (struct statfs *)lArg));
        
    case FIOREADDIR:
        return  (_MemReadDir(pmemfile, (DIR *)lArg));
        
    case FIOSYNC:
    case FIOFLUSH:
    case FIODATASYNC:
#if LW_CFG_CACHE_EN > 0
        if (bIsDir == FALSE) {
            API_CacheFlush(DATA_CACHE, pmemdirFile->base, pmemdirFile->length);
        }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        break;
    
    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "Memory FileSystem";
        break;
    
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_MEMDEV_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
