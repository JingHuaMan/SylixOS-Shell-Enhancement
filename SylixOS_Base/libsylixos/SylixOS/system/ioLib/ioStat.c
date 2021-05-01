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
** ��   ��   ��: ioStat.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 19 ��
**
** ��        ��: ����ļ������״̬�� POSIX ����.

** BUG
2008.11.11  �����ǰ���¾Ӻ��һ����������ϵͳ, access() �����ڲ�������ʱ, û�йر��ļ�.
2008.12.04  chmod(), ��Ҫ��ֻ����ʽ���ļ�.
2009.02.13  �޸Ĵ�������Ӧ�µ� I/O ϵͳ�ṹ��֯.
2009.05.19  chmod() �� O_WRONLY ��ʽ��.
2009.06.18  access Ӧ��ʹ�� USR Ȩ���ж�.
2009.06.24  sync() ��Ӧ�����쳣�ļ�.
2009.07.09  ftruncate() ֧�� 64 bit �ļ�.
2009.07.22  sync() ʹ�� FIOSYNC ������ FIOFLUSH. (FIOFLUSH ����� tty pipe ���豸��������).
2009.08.26  fchmod() mode ����Ҫ���� S_IRUSR Ȩ��.
2009.12.14  fstat() ���豸��֧��ʱ, ����һ��ȱʡ�� stat.
2011.08.07  ���� lstat() ����.
2012.04.11  ���� 64bit �ļ�����.
2012.10.17  Ϊ�˼�����, ���� chown() ϵ�к���, ��ǰ����֧��.
2012.12.21  ����ÿ������ʵ�����Լ��������ļ�������, �������� sync ���ñ��� fd_entry �ķ�ʽ.
            ע��: sync() ����ʱ IO ϵͳΪ����״̬.
2013.01.03  ���ж����� close �� ioctl ��������ͨ�� _IosFileClose �� _IosFileIoctl �ӿ�.
2013.01.08  fstat64 ������ʹ�� FIOFSTATGET64 ��������ɹ���ֱ�ӷ���.
2013.01.21  fchmod() ����ӵ�п�дȨ��.
            ʵ�� chown().
2013.09.16  fstat() �豸���ʧ��, �� st_dev ʹ�� pfdentry �豸ָ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "limits.h"
/*********************************************************************************************************
  �ļ�ϵͳ���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0
#include "../SylixOS/fs/include/fs_fs.h"
#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
/*********************************************************************************************************
** ��������: fstat
** ��������: ����ļ��������Ϣ.
** �䡡��  : iFd           �ļ�������
**           pstat         ��õ�״̬������
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  fstat (INT  iFd, struct stat *pstat)
{
LW_API time_t  API_RootFsTime(time_t  *time);

    INT     iErrCode;

    if (iFd < 0) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    if (!pstat) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pstat->st_resv1 = LW_NULL;
    pstat->st_resv2 = LW_NULL;
    pstat->st_resv3 = LW_NULL;
    
    iErrCode = API_IosFstat(iFd, pstat);                                /*  ����ʹ����������          */
    if (iErrCode >= ERROR_NONE) {
        return  (iErrCode);
    }
    
    iErrCode = ioctl(iFd, FIOFSTATGET, (LONG)pstat);
    if ((iErrCode != ERROR_NONE) && 
        ((iErrCode == ENOSYS) || (errno == ENOSYS))) {
        PLW_FD_ENTRY  pfdentry = _IosFileGet(iFd, LW_FALSE);
        
        if (pfdentry) {
            pstat->st_dev = LW_DEV_MAKE_STDEV(pfdentry->FDENTRY_pdevhdrHdr);
        } else {
            return  (PX_ERROR);
        }
        
        pstat->st_ino     = 0;
        pstat->st_mode    = 0666 | S_IFCHR;                             /*  Ĭ������                    */
        pstat->st_nlink   = 0;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = API_RootFsTime(LW_NULL);                    /*  Ĭ��ʹ�� root fs ��׼ʱ��   */
        pstat->st_mtime   = API_RootFsTime(LW_NULL);
        pstat->st_ctime   = API_RootFsTime(LW_NULL);
        
        iErrCode = ERROR_NONE;
    }
    
    return  (iErrCode);
}
/*********************************************************************************************************
** ��������: fstat64
** ��������: ����ļ��������Ϣ.
** �䡡��  : iFd           �ļ�������
**           pstat64       ��õ�״̬������
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  fstat64 (INT  iFd, struct stat64 *pstat64)
{
    struct stat statFile;
    INT         iErrCode;
    
    if (!pstat64) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pstat64->st_resv1 = LW_NULL;
    pstat64->st_resv2 = LW_NULL;
    pstat64->st_resv3 = LW_NULL;
    
    iErrCode = ioctl(iFd, FIOFSTATGET64, (LONG)pstat64);
    if (iErrCode == ERROR_NONE) {
        return  (iErrCode);
    }
    
    iErrCode = fstat(iFd, &statFile);
    if (iErrCode == ERROR_NONE) {
        pstat64->st_dev     = statFile.st_dev;
        pstat64->st_ino     = (ino64_t)statFile.st_ino;
        pstat64->st_mode    = statFile.st_mode;
        pstat64->st_nlink   = statFile.st_nlink;
        pstat64->st_uid     = statFile.st_uid;
        pstat64->st_gid     = statFile.st_gid;
        pstat64->st_rdev    = statFile.st_rdev;
        pstat64->st_size    = (off64_t)statFile.st_size;
        pstat64->st_atime   = statFile.st_atime;
        pstat64->st_mtime   = statFile.st_mtime;
        pstat64->st_ctime   = statFile.st_ctime;
        pstat64->st_blksize = statFile.st_blksize;
        pstat64->st_blocks  = (blkcnt64_t)statFile.st_blocks;
    }
        
    return  (iErrCode);
}
/*********************************************************************************************************
** ��������: stat
** ��������: ͨ���ļ�������ļ��������Ϣ.
** �䡡��  : pcName        �ļ���
**           pstat         ��õ�״̬������
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  stat (CPCHAR  pcName, struct stat *pstat)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_RDONLY | O_PEEKONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = fstat(iFd, pstat);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: stat64
** ��������: ͨ���ļ�������ļ��������Ϣ.
** �䡡��  : pcName        �ļ���
**           pstat64       ��õ�״̬������
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  stat64 (CPCHAR  pcName, struct stat64 *pstat64)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_RDONLY | O_PEEKONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = fstat64(iFd, pstat64);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: lstat
** ��������: ͨ���ļ�������ļ��������Ϣ. (����������ļ�, �򷵻������ļ������Ϣ)
** �䡡��  : pcName        �ļ���
**           pstat         ��õ�״̬������
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  lstat (CPCHAR  pcName, struct stat *pstat)
{
    REGISTER LONG  lValue;
    PLW_FD_ENTRY   pfdentry;
    PLW_DEV_HDR    pdevhdrHdr;
    CHAR           cFullFileName[MAX_FILENAME_LENGTH];
    PCHAR          pcLastTimeName;
    INT            iLinkCount = 0;
    
    INT            iError;
    
    if (pcName == LW_NULL) {                                            /*  ����ļ���                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    if (pcName[0] == PX_EOS) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pstat->st_resv1 = LW_NULL;
    pstat->st_resv2 = LW_NULL;
    pstat->st_resv3 = LW_NULL;
    
    if (lib_strcmp(pcName, ".") == 0) {                                 /*  �˵���ǰĿ¼                */
        pcName++;
    }
    
    if (ioFullFileNameGet(pcName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    pcLastTimeName = (PCHAR)__SHEAP_ALLOC(MAX_FILENAME_LENGTH);
    if (pcLastTimeName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_strlcpy(pcLastTimeName, cFullFileName, MAX_FILENAME_LENGTH);
    
    pfdentry = _IosFileNew(pdevhdrHdr, cFullFileName);                  /*  ����һ����ʱ�� fd_entry     */
    if (pfdentry == LW_NULL) {
        __SHEAP_FREE(pcLastTimeName);
        return  (PX_ERROR);
    }
    
    for (;;) {
        lValue = iosOpen(pdevhdrHdr, cFullFileName, O_RDONLY | O_PEEKONLY, 0);
        if (lValue != FOLLOW_LINK_TAIL) {                               /*  FOLLOW_LINK_FILE ֱ���˳�   */
            break;
        
        } else {
            if (iLinkCount++ > _S_iIoMaxLinkLevels) {                   /*  �����ļ�����̫��            */
                _IosFileDelete(pfdentry);
                __SHEAP_FREE(pcLastTimeName);
                _ErrorHandle(ELOOP);
                return  (PX_ERROR);
            }
        }
    
        /*
         *  ��������������� FOLLOW_LINK_????, cFullFileName�ڲ�һ����Ŀ��ľ��Ե�ַ, ����/��ʼ���ļ���.
         */
        if (ioFullFileNameGet(cFullFileName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
            _IosFileDelete(pfdentry);
            __SHEAP_FREE(pcLastTimeName);
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
        lib_strlcpy(pcLastTimeName, cFullFileName, MAX_FILENAME_LENGTH);
    }
    
    if ((lValue != PX_ERROR) && (lValue != FOLLOW_LINK_FILE)) {
        _IosFileSet(pfdentry, pdevhdrHdr, lValue, O_RDONLY | O_PEEKONLY, FDSTAT_CLOSING);
        _IosFileClose(pfdentry);                                        /*  �ر�                        */
    }
    
    _IosFileDelete(pfdentry);                                           /*  ɾ����ʱ�� fd_entry         */
    
    iError = API_IosLstat(pdevhdrHdr, pcLastTimeName, pstat);
    
    __SHEAP_FREE(pcLastTimeName);
    
    if (iError < ERROR_NONE) {
        return  (stat(pcName, pstat));
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: lstat64
** ��������: ͨ���ļ�������ļ��������Ϣ. (����������ļ�, �򷵻������ļ������Ϣ)
** �䡡��  : pcName        �ļ���
**           pstat64       ��õ�״̬������
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  lstat64 (CPCHAR  pcName, struct stat64 *pstat64)
{
    struct stat statFile;
    INT         iErrCode;
    
    if (!pstat64) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pstat64->st_resv1 = LW_NULL;
    pstat64->st_resv2 = LW_NULL;
    pstat64->st_resv3 = LW_NULL;

    iErrCode = lstat(pcName, &statFile);
    if (iErrCode == ERROR_NONE) {
        pstat64->st_dev     = statFile.st_dev;
        pstat64->st_ino     = (ino64_t)statFile.st_ino;
        pstat64->st_mode    = statFile.st_mode;
        pstat64->st_nlink   = statFile.st_nlink;
        pstat64->st_uid     = statFile.st_uid;
        pstat64->st_gid     = statFile.st_gid;
        pstat64->st_rdev    = statFile.st_rdev;
        pstat64->st_size    = (off64_t)statFile.st_size;
        pstat64->st_atime   = statFile.st_atime;
        pstat64->st_mtime   = statFile.st_mtime;
        pstat64->st_ctime   = statFile.st_ctime;
        pstat64->st_blksize = statFile.st_blksize;
        pstat64->st_blocks  = (blkcnt64_t)statFile.st_blocks;
    }
        
    return  (iErrCode);
}
/*********************************************************************************************************
** ��������: fstatfs
** ��������: ����ļ�ϵͳ�������Ϣ.
** �䡡��  : iFd           �ļ�������
**           pstatfs       �ļ�ϵͳ״̬������
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  fstatfs (INT  iFd, struct statfs *pstatfs)
{
    if (!pstatfs) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ioctl(iFd, FIOFSTATFSGET, (LONG)pstatfs));
}
/*********************************************************************************************************
** ��������: statfs
** ��������: ͨ���ļ�������ļ�ϵͳ�������Ϣ.
** �䡡��  : pcName        �ļ���
**           pstatfs       �ļ�ϵͳ״̬������
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  statfs (CPCHAR  pcName, struct statfs *pstatfs)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_RDONLY | O_PEEKONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = fstatfs(iFd, pstatfs);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: ftruncate
** ��������: ��������չ�ļ����ȡ����֮ǰ���ļ����ȱ�lengthָ���ĳ��ȴ󣬶�������ݻᶪʧ��
**           ���֮ǰ���ļ����ȱ�ָ���ĳ���С�������á�/0��������󲿷֡�
** �䡡��  : iFd           �ļ�������
**           oftLength     �ļ�����
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  ftruncate (INT  iFd, off_t  oftLength)
{
    return  (ioctl(iFd, FIOTRUNC, (LONG)&oftLength));
}
/*********************************************************************************************************
** ��������: ftruncate64
** ��������: ��������չ�ļ����ȡ����֮ǰ���ļ����ȱ�lengthָ���ĳ��ȴ󣬶�������ݻᶪʧ��
**           ���֮ǰ���ļ����ȱ�ָ���ĳ���С�������á�/0��������󲿷֡�
** �䡡��  : iFd           �ļ�������
**           oftLength     �ļ�����
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  ftruncate64 (INT  iFd, off64_t  oftLength)
{
    return  (ioctl(iFd, FIOTRUNC, (LONG)&oftLength));
}
/*********************************************************************************************************
** ��������: truncate
** ��������: ��������չ�ļ����ȡ����֮ǰ���ļ����ȱ�lengthָ���ĳ��ȴ󣬶�������ݻᶪʧ��
**           ���֮ǰ���ļ����ȱ�ָ���ĳ���С�������á�/0��������󲿷֡�
** �䡡��  : pcName        �ļ���
**           oftLength     �ļ�����
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  truncate (CPCHAR  pcName, off_t  oftLength)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_RDWR, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = ftruncate(iFd, oftLength);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: truncate64
** ��������: ��������չ�ļ����ȡ����֮ǰ���ļ����ȱ�lengthָ���ĳ��ȴ󣬶�������ݻᶪʧ��
**           ���֮ǰ���ļ����ȱ�ָ���ĳ���С�������á�/0��������󲿷֡�
** �䡡��  : pcName        �ļ���
**           oftLength     �ļ�����
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  truncate64 (CPCHAR  pcName, off64_t  oftLength)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_RDWR, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = ftruncate64(iFd, oftLength);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: fchmod
** ��������: ���Խ� filename ��ָ���ļ���ģʽ�ĳ� mode ��������
** �䡡��  : iFd           �ļ�������
**           iMode         ģʽ
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  fchmod (INT  iFd, INT  iMode)
{
    struct stat statBuf;

    iMode |= S_IRUSR;                                                   /*  ���뱣֤�������ܶ�          */
    iMode &= ~S_IFMT;                                                   /*  ȥ���ļ�����                */
    
    if (fstat(iFd, &statBuf) < 0) {
        return  (PX_ERROR);
    }
    
    if (_IosCheckPermissions(O_WRONLY, LW_FALSE, statBuf.st_mode, 
                             statBuf.st_uid, statBuf.st_gid) < ERROR_NONE) {
        _ErrorHandle(EACCES);
        return  (PX_ERROR);
    }

    return  (ioctl(iFd, FIOCHMOD, iMode));
}
/*********************************************************************************************************
** ��������: chmod
** ��������: ���Խ� filename ��ָ���ļ���ģʽ�ĳ� mode ��������
** �䡡��  : pcName        �ļ���
**           iMode         ģʽ
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  chmod (CPCHAR  pcName, INT  iMode)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_RDONLY | O_PEEKONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = fchmod(iFd, iMode);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: fchown
** ��������: �����ļ����û�ID�ͣ���ID
** �䡡��  : iFd           �ļ�������
**           uid           ������ ID
**           gid           �������� ID
** �䡡��  : ERROR_NONE    û�д���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  fchown (INT  iFd, uid_t uid, gid_t gid)
{
    struct stat statBuf;
    LW_IO_USR   usr;
    
    usr.IOU_uid = uid;
    usr.IOU_gid = gid;
    
    if (fstat(iFd, &statBuf) < 0) {
        return  (PX_ERROR);
    }
    
    if (_IosCheckPermissions(O_WRONLY, LW_FALSE, statBuf.st_mode, 
                             statBuf.st_uid, statBuf.st_gid) < ERROR_NONE) {
        _ErrorHandle(EACCES);
        return  (PX_ERROR);
    }

    return  (ioctl(iFd, FIOCHOWN, (LONG)&usr));
}
/*********************************************************************************************************
** ��������: chown
** ��������: �����ļ����û�ID�ͣ���ID
** �䡡��  : pcName        �ļ���
**           uid           ������ ID
**           gid           �������� ID
** �䡡��  : ERROR_NONE    û�д���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  chown (CPCHAR  pcName, uid_t uid, gid_t gid)
{
    REGISTER INT    iFd;
    REGISTER INT    iError;
    
    iFd = open(pcName, O_RDONLY | O_PEEKONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = fchown(iFd, uid, gid);
    
    if (iError < 0) {
        iError = errno;
        close(iFd);
        errno  = iError;
        return  (PX_ERROR);
    
    } else {
        close(iFd);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: lchown
** ��������: �����ļ����û�ID�ͣ���ID
** �䡡��  : pcName        �ļ���
**           uid           ������ ID
**           gid           �������� ID
** �䡡��  : ERROR_NONE    û�д���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  lchown (CPCHAR  pcName, uid_t uid, gid_t gid)
{
    errno = ENOSYS;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: fsync
** ��������: ���ļ�����д�����
** �䡡��  : iFd           �ļ�������
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  fsync (INT  iFd)
{
    INT     iRet;

    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */

    iRet = ioctl(iFd, FIOSYNC);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: fdatasync
** ��������: ���ļ�����д�����(���ݲ���)
** �䡡��  : iFd           �ļ�������
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  fdatasync (INT  iFd)
{
    INT     iRet;
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    iRet = ioctl(iFd, FIODATASYNC);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: sync
** ��������: ϵͳ IO ����д����� (��Σ��, �豸�����в��ܲ�żż)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  sync (VOID)
{
    REGISTER PLW_LIST_LINE  plineFdEntry;
    REGISTER PLW_FD_ENTRY   pfdentry;
    REGISTER PLW_DEV_HDR    pdevhdr;
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    
    _IosFileListLock();                                                 /*  ��ʼ����                    */
    
    plineFdEntry = _S_plineFileEntryHeader;
    while (plineFdEntry) {
        pfdentry = _LIST_ENTRY(plineFdEntry, LW_FD_ENTRY, FDENTRY_lineManage);
        pdevhdr = pfdentry->FDENTRY_pdevhdrHdr;
        plineFdEntry = _list_line_get_next(plineFdEntry);

        if (!pdevhdr) {                                                 /*  δ�����豸ͷ������          */
            continue;
        }

        if ((pdevhdr->DEVHDR_ucType == DT_CHR)  ||
            (pdevhdr->DEVHDR_ucType == DT_FIFO) ||
            (pdevhdr->DEVHDR_ucType == DT_SOCK)) {                      /*  CHR, FIFO, SOCK ������      */
            continue;
        }
        
        if (pfdentry->FDENTRY_state != FDSTAT_OK) {
            continue;
        }
        
        pfdentry->FDENTRY_state = FDSTAT_SYNC;                          /*  ��������� close ����       */
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        
        _IosFileSync(pfdentry);                                         /*  ��������ͬ������            */
        
        _IosLock();                                                     /*  ���� IO �ٽ���              */
        if (pfdentry->FDENTRY_state == FDSTAT_REQCLOSE) {
            pfdentry->FDENTRY_state =  FDSTAT_CLOSING;
            _IosUnlock();                                               /*  �˳� IO �ٽ���              */
            
            _IosFileClose(pfdentry);                                    /*  ���������ر��ļ�            */
            
            _IosLock();                                                 /*  ���� IO �ٽ���              */
        
        } else if (pfdentry->FDENTRY_state == FDSTAT_SYNC) {
            pfdentry->FDENTRY_state = FDSTAT_OK;
        }
    }
    
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    _IosFileListUnlock();                                               /*  ��������, ɾ������ɾ���Ľڵ�*/

#if LW_CFG_MAX_VOLUMES > 0
#if LW_CFG_YAFFS_EN > 0
    API_YaffsDevSync(NULL);                                             /*  Yaffs �豸����ͬ��          */
#endif                                                                  /*  LW_CFG_YAFFS_EN > 0         */

#if LW_CFG_DISKCACHE_EN > 0
    API_DiskCacheSync(LW_NULL);                                         /*  ��д���д��̻���            */
#endif                                                                  /*  LW_CFG_DISKCACHE_EN > 0     */
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
}
/*********************************************************************************************************
** ��������: access
** ��������: �ж��ļ�����Ȩ��
** �䡡��  : pcPath        File or Direction 
**           iMode         mode
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  access (CPCHAR pcPath, INT  iMode)
{
             INT          i, iNum;
    REGISTER INT          iError, iErrCode;
    REGISTER INT          iRead, iWrite, iExec;
    REGISTER INT          iFd = open(pcPath, O_RDONLY | O_PEEKONLY, 0);
             uid_t        uid = geteuid();
             gid_t        gid = getegid();
             gid_t        gidList[NGROUPS_MAX];
             struct stat  statFile;
    
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError   = fstat(iFd, &statFile);
    iErrCode = errno;
    close(iFd);

    if (iError < 0) {
        errno = iErrCode;
        return  (PX_ERROR);
    }
    
    if (uid == 0) {
        iRead  = S_IRUSR | S_IRGRP | S_IROTH;
        iWrite = S_IWUSR | S_IWGRP | S_IWOTH;
        iExec  = S_IXUSR | S_IXGRP | S_IXOTH;

    } else if (uid == statFile.st_uid) {
        iRead  = S_IRUSR;
        iWrite = S_IWUSR;
        iExec  = S_IXUSR;

    } else if (gid == statFile.st_gid) {
        iRead  = S_IRGRP;
        iWrite = S_IWGRP;
        iExec  = S_IXGRP;

    } else {
        iNum = getgroups(NGROUPS_MAX, gidList);
        for (i = 0; i < iNum; i++) {
            if (gidList[i] == statFile.st_gid) {
                break;
            }
        }

        if (i < iNum) {
            iRead  = S_IRGRP;
            iWrite = S_IWGRP;
            iExec  = S_IXGRP;

        } else {
            iRead  = S_IROTH;
            iWrite = S_IWOTH;
            iExec  = S_IXOTH;
        }
    }

    if (iMode & R_OK) {
        if ((statFile.st_mode & iRead) == 0) {
            goto    __error_handle;
        }
    }
    
    if (iMode & W_OK) {
        if ((statFile.st_mode & iWrite) == 0) {
            goto    __error_handle;
        }
    }
    
    if (iMode & X_OK) {
        if ((statFile.st_mode & iExec) == 0) {
            goto    __error_handle;
        }
    }
    
    return  (ERROR_NONE);
    
__error_handle:
    errno = EACCES;
    return  (PX_ERROR);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
