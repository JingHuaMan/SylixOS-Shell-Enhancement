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
** ��   ��   ��: ioDir.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 19 ��
**
** ��        ��: ϵͳĿ¼������.

** BUG:
2009.03.10  errno ������ʹ�� _ErrorHandle().
2009.07.21  mkdir() �� mode ������ʱδʹ��.
2009.08.26  Ŀ¼�������� readdir_r() �̰߳�ȫ����, ��Ҫ dir �����.
2011.05.15  ���� rmdir �� utime ����Ϊ const ����.
2012.03.11  ���� futimes() utimes() ����.
2012.09.21  ���� dirfd() ����.
2012.12.11  dir �������ԭʼ��Դ����.
2012.12.22  ʹ�� freedir ���ͷŽ���û�йرյ� DIR, freedir �в��ر��ļ�������, 
            ��Ϊ���ս��̻����ں��Ѿ����Ƿ���ʱ���̵��ļ���������, �ļ��Ĺر�ͳһ�ɽ��̻��������.
2013.01.08  ����Ŀ¼��Ҫ�� O_EXCL �������������Դ���.
2013.03.12  ���� readdir64 �� readdir64_r.
2018.12.14  utimes utime ʹ�� O_RDONLY ѡ����ļ� (Ȩ��Ӧ��ͬһ��Ȩ�޿������ж�).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
** ��������: mkdir
** ��������: ����һ���µ�Ŀ¼
** �䡡��  : pcDirName     Ŀ¼��
**           mode          ��ʽ
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  mkdir (CPCHAR  pcDirName, mode_t  mode)
{
    REGISTER INT    iFd;
    
    mode &= ~S_IFMT;
    
    iFd = open(pcDirName, O_RDWR | O_CREAT | O_EXCL, S_IFDIR | mode);   /*  �����Դ���                  */
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    close(iFd);
    
    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: rmdir
** ��������: ɾ��һ�����ڵ�Ŀ¼
** �䡡��  : pcDirName     Ŀ¼��
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  rmdir (CPCHAR  pcDirName)
{
    return  (unlink(pcDirName));
}
/*********************************************************************************************************
** ��������: dirfd
** ��������: �� DIR �ṹ�л�ȡĿ¼�ļ�������
** �䡡��  : pdir     Ŀ¼���ƿ�
** �䡡��  : �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  dirfd (DIR  *pdir)
{
    if (!pdir) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (pdir->dir_fd);
}
/*********************************************************************************************************
** ��������: freedir
** ��������: �ͷ�һ��DIR�ṹ, ���ر��ļ�, �ɻ�����ͳһ�ر��ļ�
             (���ڽ�����Դ����, ������ʹ�� closedir ��Ϊ�ļ��������Ѳ��ǽ������̵��ļ���������)
** �䡡��  : pdir     Ŀ¼���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  freedir (DIR  *pdir)
{
    if (pdir) {
        API_SemaphoreBDelete(&pdir->dir_lock);
        
        __resDelRawHook(&pdir->dir_resraw);
    
        __SHEAP_FREE(pdir);
    }
}

#endif
/*********************************************************************************************************
** ��������: opendir
** ��������: ��һ�����ڵ�Ŀ¼
** �䡡��  : pcDirName     Ŀ¼��
** �䡡��  : Ŀ¼���ƿ�ָ��, ���󷵻� NULL, �������� errno ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
DIR  *opendir (CPCHAR   pcPathName)
{
    REGISTER INT            iFd;
    REGISTER INT            iError;
             struct stat    statFile;
             DIR           *pdir;
    
    iFd = open(pcPathName, O_RDONLY | O_DIRECTORY, 0);
    if (iFd < 0) {
        return  (LW_NULL);
    }
    
    iError = fstat(iFd, &statFile);
    if (iError < 0) {
        close(iFd);
        return  (LW_NULL);
    }
    
    if (!S_ISDIR(statFile.st_mode)) {                                   /*  ����Ƿ�ΪĿ¼�ļ�          */
        close(iFd);
        errno = ENOTDIR;
        return  (LW_NULL);
    }
    
    pdir = (DIR *)__SHEAP_ALLOC(sizeof(DIR));                           /*  ���� DIR �ṹĿ¼           */
    if (!pdir) {
        close(iFd);
        errno = ERROR_SYSTEM_LOW_MEMORY;
        return  (LW_NULL);
    }
    lib_bzero(pdir, sizeof(DIR));
    
    pdir->dir_fd    = iFd;
    pdir->dir_pos   = 0;
    pdir->dir_lock  = API_SemaphoreBCreate("dir_lock", LW_TRUE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pdir->dir_lock == LW_OBJECT_HANDLE_INVALID) {                   /*  �Ƿ�ɹ�����������          */
        close(iFd);
        __SHEAP_FREE(pdir);
        return  (LW_NULL);
    }
                                                                        /*  ����ԭʼ��Դ��              */
    __resAddRawHook(&pdir->dir_resraw, (VOIDFUNCPTR)freedir, pdir, 0, 0, 0, 0, 0);
    
    return  (pdir);
}
/*********************************************************************************************************
** ��������: closedir
** ��������: �ر�һ���Ѿ��򿪵�Ŀ¼
** �䡡��  : pdir     Ŀ¼���ƿ�
** �䡡��  : ERROR_NONE ��ʾ��ȷ, ������ʾ����.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  closedir (DIR  *pdir)
{
    REGISTER INT   iError;

    if (!pdir) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iError = close(pdir->dir_fd);
    if (iError < 0) {
        return  (PX_ERROR);
    }
    pdir->dir_fd = PX_ERROR;                                            /*  �����Ѿ��ر�                */
    
    API_SemaphoreBDelete(&pdir->dir_lock);
    
    __resDelRawHook(&pdir->dir_resraw);
    
    __SHEAP_FREE(pdir);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: readdir
** ��������: ��ȡһ���Ѿ��򿪵�Ŀ¼�ĵ�����Ϣ
** �䡡��  : pdir     Ŀ¼���ƿ�
** �䡡��  : ����Ŀ��Ϣָ��, ���󷵻� NULL, �������� errno ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
struct dirent *readdir (DIR  *pdir)
{
    REGISTER INT   iError;

    if (!pdir) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    API_SemaphoreBPend(pdir->dir_lock, LW_OPTION_WAIT_INFINITE);
    iError = ioctl(pdir->dir_fd, FIOREADDIR, (LONG)pdir);
    API_SemaphoreBPost(pdir->dir_lock);
    
    if (iError < 0) {
        return  (LW_NULL);
    }
    
    return  (&pdir->dir_dirent);
}
/*********************************************************************************************************
** ��������: readdir64
** ��������: ��ȡһ���Ѿ��򿪵�Ŀ¼�ĵ�����Ϣ
** �䡡��  : pdir     Ŀ¼���ƿ�
** �䡡��  : ����Ŀ��Ϣָ��, ���󷵻� NULL, �������� errno ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
struct dirent64 *readdir64 (DIR  *pdir)
{
    return  ((struct dirent64 *)readdir(pdir));
}
/*********************************************************************************************************
** ��������: readdir_r
** ��������: ��ȡһ���Ѿ��򿪵�Ŀ¼�ĵ�����Ϣ (������)
** �䡡��  : pdir              Ŀ¼���ƿ�
**           pdirentEntry      ��õ�Ŀ¼��Ŀ��Ϣ����
**           ppdirentResult    ���ɹ�ʱ, ��ָ��ָ�� pdirentEntry, ������ĩλʱ, ��ָ��Ϊ NULL.
** �䡡��  : ERROR_NONE ��ʾ��ȷ, ������ʾ����.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  readdir_r (DIR             *pdir, 
                struct dirent   *pdirentEntry,
                struct dirent  **ppdirentResult)
{
    REGISTER INT   iError;

    if (!pdir || !pdirentEntry || !ppdirentResult) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    API_SemaphoreBPend(pdir->dir_lock, LW_OPTION_WAIT_INFINITE);
    iError = ioctl(pdir->dir_fd, FIOREADDIR, (LONG)pdir);
    if (iError < 0) {
        API_SemaphoreBPost(pdir->dir_lock);
        *ppdirentResult = LW_NULL;
        return  (PX_ERROR);
    }
    *pdirentEntry   = pdir->dir_dirent;
    *ppdirentResult = pdirentEntry;
    API_SemaphoreBPost(pdir->dir_lock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: readdir64_r
** ��������: ��ȡһ���Ѿ��򿪵�Ŀ¼�ĵ�����Ϣ (������)
** �䡡��  : pdir              Ŀ¼���ƿ�
**           pdirent64Entry    ��õ�Ŀ¼��Ŀ��Ϣ����
**           ppdirent64Result  ���ɹ�ʱ, ��ָ��ָ�� pdirentEntry, ������ĩλʱ, ��ָ��Ϊ NULL.
** �䡡��  : ERROR_NONE ��ʾ��ȷ, ������ʾ����.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  readdir64_r (DIR               *pdir, 
                  struct dirent64   *pdirent64Entry,
                  struct dirent64  **ppdirent64Result)
{
    return  (readdir_r(pdir, (struct dirent *)pdirent64Entry, (struct dirent **)ppdirent64Result));
}
/*********************************************************************************************************
** ��������: rewinddir
** ��������: ��λ��ǰĿ¼ָ��
** �䡡��  : pdir     Ŀ¼���ƿ�
** �䡡��  : ERROR_NONE ��ʾ��ȷ, ������ʾ����.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  rewinddir (DIR   *pdir)
{
    if (!pdir) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    API_SemaphoreBPend(pdir->dir_lock, LW_OPTION_WAIT_INFINITE);
    pdir->dir_pos = 0;                                                  /*  �˻ص�ԭ��                  */
    API_SemaphoreBPost(pdir->dir_lock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: futimes
** ��������: �����ļ�ʱ��
** �䡡��  : iFd        �ļ�������
**           tvp        ʱ��
** �䡡��  : ERROR_NONE ��ʾ��ȷ, ������ʾ����.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  futimes (INT iFd, struct timeval tvp[2])
{
    struct utimbuf  utimbNow;
    
    if (!tvp) {
        utimbNow.actime  = lib_time(LW_NULL);
        utimbNow.modtime = utimbNow.actime;
    
    } else {
        utimbNow.actime  = tvp[0].tv_sec;
        utimbNow.modtime = tvp[1].tv_sec;
    }
    
    return  (ioctl(iFd, FIOTIMESET, (LONG)&utimbNow));
}
/*********************************************************************************************************
** ��������: utimes
** ��������: �����ļ�ʱ��
** �䡡��  : iFd        �ļ�������
**           tvp        ʱ��
** �䡡��  : ERROR_NONE ��ʾ��ȷ, ������ʾ����.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  utimes (CPCHAR  pcFile, struct timeval tvp[2])
{
    REGISTER INT            iError;
    REGISTER INT            iFd;
    
    iFd = open(pcFile, O_RDONLY | O_PEEKONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = futimes(iFd, tvp);
    
    close(iFd);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: utime
** ��������: �����ļ�ʱ��
** �䡡��  : pcFile     �ļ���
**           utimbNew   �µ��ļ�ʱ��
** �䡡��  : ERROR_NONE ��ʾ��ȷ, ������ʾ����.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  utime (CPCHAR  pcFile, const struct utimbuf *utimbNew)
{
    REGISTER INT            iError;
    REGISTER INT            iFd;
    struct utimbuf          utimbNow;
    
    iFd = open(pcFile, O_RDONLY | O_PEEKONLY, 0);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    if (!utimbNew) {
        utimbNew = &utimbNow;
        utimbNow.actime  = lib_time(LW_NULL);
        utimbNow.modtime = utimbNow.actime;
    }
    
    iError = ioctl(iFd, FIOTIMESET, (LONG)utimbNew);
    
    close(iFd);
    
    return  (iError);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
