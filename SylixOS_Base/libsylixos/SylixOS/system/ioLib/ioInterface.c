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
** ��   ��   ��: ioInterface.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 16 ��
**
** ��        ��: ϵͳ IO ���ܺ����⣬API ����

** BUG
2007.09.12  ����Բü���֧�֡�
2007.09.24  ���� _DebugHandle() ���ܡ�
2008.05.31  �� ioctl �� 3 ����������Ϊ long ��, ����Ӧ 64 λϵͳָ������.
2008.06.08  �� I/O safe ����, ��Ϊ posix �涨�Ķ��� cancel ��ʽ��Ҫ��.
2008.06.27  �޸� errhandle ��ŵ�����.
2008.07.23  ���� ttyname() api ����.
2008.08.10  �������˿�ʼ���һ����������ϵͳ, ����ܼ���, ף���˻�ɹ�!
            ���� API_Dup2(), ����������������� I/O �ض���.
2008.09.28  ���������ߺŷɴ�, ��Ҫ���ص���, Ԥף���سɹ�!
            ���� _IoCreateOrOpen() �ڴ������ʱ��֤ errno ��������ȷ��.
            ������ API_IoLseek() ��һ�� BUG.
2008.10.31  _IoCreateOrOpen() ���������޷����ļ�ʱ, ��������.
2009.02.13  �޸Ĵ�������Ӧ�µ� I/O ϵͳ�ṹ��֯.
2009.02.19  �޸��� _IoCreateOrOpen() �Դ�ʧ�ܵĴ���.
2009.03.10  errno������ʹ�� _ErrorHandle.
2009.03.13  open, ioctl ��Ϊ�ɱ䳤��������.
            creat ��Ϊ�� unix ������ȫ��ͬ.
2009.05.19  API_IoRename() Ӧ���� O_WRONLY ���ļ�.
2009.06.07  API_IoOpen() ����������Ĭ��Ϊ DEFAULT_FILE_PERM.
2009.06.30  API_Ioctl() ���� va_end ����.
2009.07.01  ���� fchdir() ����.
            writev, readv ��Ҫ�����߳��첽״̬����.
2009.07.09  API_IoLseek() ֧�� 64 bit �ļ�. SEEK_END ������̽ FIONREAD64 ����, �������, ��ʹ�� fstat.
2009.07.25  _IoCreateOrOpen() ���ⴴ��û���κ�Ȩ�޵��ļ�.
            _IoCreateOrOpen() ���� creat ���ò�������.
2009.10.12  dup(), dup2() ��Ӧ�ö��豸���������в���.
2009.10.22  �޸� read write ����������ֵ����.
2009.11.21  ���� ioPrivateEnv() ����.
2009.11.22  ���� API_IoFullFileNameGet2() ����, ������Ŀ¼ѹ��.
            IoFullFileNameGet ������Ҫ��·����βȥ������� / ����.
2009.11.23  open �������� LW_CFG_PATH_AUTO_CONDENSE ����, ���Խ��豸��˵��豸���Ŀ¼ . �� .. �����Զ�
            ѹ��, ���� open(/DEVICE/./aaa/../bbb) ���Զ���Ϊ open(/DEVICE/bbb)
2009.12.04  ����ע��.
2009.12.14  API_IoDefPathCat() ����ѹ��Ŀ¼��ȷ���豸����Ч��.
2009.12.15  ���� _IoOpen() �����������ļ��Ĵ������.
2010.01.21  ���� ttyname_r() ����.
2010.08.03  ֧�� SMP ���.
2011.05.16  ���� rename() ������Ϊ�������ӵ�����.
            API_IoFullFileNameGet() API_IoFullFileNameGet2() �����豸Ϊ���ļ�ϵͳ���µ� tail ȱ�� '/' ��ʼ
            �Ĵ���.
2011.08.07  ���������Է������ӵ�֧��, ʹ��֧������Ŀ����������Ŀ¼.
2011.08.11  ȥ�� API_IoFullFileNameGet2 ����, ��Ϊ����ϵͳ�Ӵ�Ĭ�Ͻ���Ŀ¼ѹ��.
2011.08.15  �ش����: ������ posix ����ĺ����Ժ�����ʽ(�Ǻ�)����.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2012.04.11  ���� 64bit �ļ���׼����.
2012.06.28  ������ makedev major minor ������ linux ���ݺ���.
2012.08.16  ���� pread �� pwrite �ӿ�, ��Ҫ��Ӧ�豸(�ļ�ϵͳ)��������֧��.
2012.09.25  mknod() ��Ҫ�� O_EXCL ����.
2012.10.16  ���� realpath() ����.
2012.10.24  fchdir() ���뱣֤��Ŀ¼�ļ���������.
            _IoOpen() �� full name get ���������Ϊ�������� fdset, �����ļ��������ڲ������ļ�����·��.
2012.11.21  unlink() ������������򿪵��ļ������ñ�־, ���ļ��ر�ʱ, �Զ� unlink()
2012.12.21  �����Ǵ�˵�е�����ĩ��, �һ���ά�� SylixOS ��.
            *StdSet �� *StdGet ����ֻ׼���ں��߳�, �������̲߳�������.
2012.12.24  ���� dup2kernel ���̿��԰��ļ������� dup ���ں���.
2013.01.03  ����ʹ�� _IosFileClose ������������ close ����.
2013.01.05  ���� flock ����.
2013.01.09  ���� umask ����.
2013.01.17  open ���� O_CLOEXEC / O_SHLOCK �� O_EXLOCK ��֧��.
2013.03.12  ���� open64.
2013.05.31  unlink ����ÿһ�ζ���Ҫ��鵱ǰ�Ѿ����򿪵��ļ�.
2013.07.17  open ������� O_NONBLOCK ѡ��, ��¼��Ҳ�Ƿ�������.
            ֧�� O_NOFOLLOW ѡ��.
2013.11.18  ���� dupminfd ֧��.
2017.08.11  ioFullFileNameGet() �ڲ���Ҫ���� errno.
*********************************************************************************************************/
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  VAR & MACRO
*********************************************************************************************************/
#define STD_VALID(fd)     (((fd) >= 0) && ((fd) < 3))
static INT _G_iIoStdFd[3] = {PX_ERROR, PX_ERROR, PX_ERROR};             /*  ȫ�ֱ�׼��������ļ�        */
/*********************************************************************************************************
** ��������: _IoOpen
** ��������: �������һ���ļ�
** �䡡��  : pcName                        �ļ���
**           iFlag                         ��ʽ         O_RDONLY  O_WRONLY  O_RDWR  O_CREAT
**           iMode                         UNIX MODE
** �䡡��  : �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT _IoOpen (PCHAR            pcName,
                    INT              iFlag,
                    INT              iMode,
                    BOOL             bCreate)
{
             PLW_DEV_HDR    pdevhdrHdr;
             PLW_IO_ENV     pioeDef;
    
    REGISTER LONG           lValue;
    REGISTER INT            iFd = (PX_ERROR);
    
             CHAR           cFullFileName[MAX_FILENAME_LENGTH];
             
    REGISTER INT            iErrLevel  = 0;
             INT            iLinkCount = 0;
             
             ULONG          ulError = ERROR_NONE;
             
    pioeDef = _IosEnvGetDef();
    iMode = (iMode & (~pioeDef->IOE_modeUMask)) | S_IRUSR;              /*  ���뱣֤�������ܶ�          */

    __THREAD_CANCEL_POINT();

    if (iFlag & O_DIRECTORY) {
        if ((iFlag & O_ACCMODE) != O_RDONLY) {
            ulError   = EISDIR;
            iErrLevel = 1;
            goto    __error_handle;
        }
    }

    if (pcName == LW_NULL) {                                            /*  ����ļ���                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        ulError   = EFAULT;                                             /*  Bad address                 */
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    if (pcName[0] == PX_EOS) {
        ulError   = ENOENT;
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    if (lib_strcmp(pcName, ".") == 0) {                                 /*  �˵���ǰĿ¼                */
        pcName++;
    }
    
    if (ioFullFileNameGet(pcName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        ulError   = API_GetLastError();
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    iFd = iosFdNew(pdevhdrHdr, cFullFileName, PX_ERROR, iFlag);         /*  ��ʱ��ȷ�����ļ���������    */
    if (iFd == PX_ERROR) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not create.\r\n");
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    for (;;) {
        if (bCreate) {
            lValue = iosCreate(pdevhdrHdr, cFullFileName, iFlag, iMode);
        } else {
            lValue = iosOpen(pdevhdrHdr, cFullFileName, iFlag, iMode);
        }
        
        if (lValue == PX_ERROR) {
            ulError   = API_GetLastError();
            iErrLevel = 2;
            goto    __error_handle;
        }
        
        /*
         *  ��������������� FOLLOW_LINK_????, cFullFileName�ڲ�һ����Ŀ��ľ��Ե�ַ, ����/��ʼ���ļ���.
         */
        if ((lValue != FOLLOW_LINK_FILE) && 
            (lValue != FOLLOW_LINK_TAIL)) {                             /*  �������ļ�ֱ���˳�          */
            break;
        
        } else {
            if (((iFlag & O_NOFOLLOW) && (lValue == FOLLOW_LINK_FILE)) ||
                (iLinkCount++ > _S_iIoMaxLinkLevels)) {                 /*  �������������              */
                ulError   = ELOOP;                                      /*  �����ļ�����̫��            */
                iErrLevel = 2;
                goto    __error_handle;
            }
        }
        
        if (ioFullFileNameGet(cFullFileName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
            ulError   = EXDEV;
            iErrLevel = 2;
            goto    __error_handle;
        }
    }

    if (iosFdSet(iFd, pdevhdrHdr, lValue, iFlag, FDSTAT_OK) != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not config.\r\n");
        ulError   = API_GetLastError();
        iErrLevel = 3;
        goto    __error_handle;
    }

    if (iLinkCount) {                                                   /*  �ڲ����ڷ�������            */
        if (iosFdRealName(iFd, cFullFileName)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "file can not set real name.\r\n");
            ulError   = API_GetLastError();
            iErrLevel = 3;
            goto    __error_handle;
        }
    }
    
    if (pdevhdrHdr->DEVHDR_ucType == DT_FIFO) {                         /*  �ܵ��豸                    */
        if (iosIoctl(iFd, FIOPIPEBLOCK, 0)) {                           /*  ̽���Ƿ���Ҫ open ����      */
            ulError   = API_GetLastError();
            iErrLevel = 3;
            goto    __error_handle;
        }
    }

    if ((iFlag & O_SHLOCK) || (iFlag & O_EXLOCK)) {                     /*  ��Ҫֱ�Ӽ���                */
        REGISTER PLW_FD_ENTRY  pfdentry;
        pfdentry = _IosFileGet(iFd, LW_FALSE);
        if (pfdentry && (pfdentry->FDENTRY_iType == LW_DRV_TYPE_NEW_1)) {
            INT  iType = (iFlag & O_EXLOCK) ? LOCK_EX : LOCK_SH;
            if (iFlag & O_NONBLOCK) {
                iType |= LOCK_NB;                                       /*  �������ȴ���¼��            */
            }
            if (_FdLockfProc(pfdentry, iType, __PROC_GET_PID_CUR())) {
                close(iFd);                                             /*  ����ʧ��                    */
                iFd = PX_ERROR;
            }
        }
    }
    
    if (iFd >= 0) {
        if (iFlag & O_CLOEXEC) {
            API_IosFdSetCloExec(iFd, FD_CLOEXEC);                       /*  ��Ҫ FD_CLOEXEC ����        */
        }
        MONITOR_EVT_INT2(MONITOR_EVENT_ID_IO, (bCreate ? MONITOR_EVENT_IO_CREAT : MONITOR_EVENT_IO_OPEN), 
                         iFlag, iMode, pcName);
    }
    
    return  (iFd);
    
__error_handle:
    if ((iErrLevel > 2) && (iFlag & O_CREAT)) {
        /*
         *  ����Ŀǰ��Ӧ��ɾ������ļ�. 
         *  iosDelete(pdevhdrHdr, cFullFileName);
         */
    }
    if (iErrLevel > 1) {
        iosFdFree(iFd);
    }
    if (iErrLevel > 0) {
        iFd = (PX_ERROR);
    }
    
    _ErrorHandle(ulError);
    return  (iFd);
}
/*********************************************************************************************************
** ��������: mknod
** ��������: create a new file named by the pathname to which the argument path points.
** �䡡��  : pcFifoName    Ŀ¼��
**           mode          ����
**           dev           Ŀǰδʹ��
** �䡡��  : ERROR_NONE    û�д���
**           ����ֵ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  mknod (CPCHAR  pcNodeName, mode_t  mode, dev_t dev)
{
    REGISTER INT    iFd;
    
    iFd = open(pcNodeName, O_RDWR | O_CREAT | O_EXCL, mode | DEFAULT_DIR_PERM);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    close(iFd);
    
    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: creat
** ��������: ����һ���ļ�
** �䡡��  : pcName                        �ļ���
**           iMode                         ����
** �䡡��  : �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  creat (CPCHAR  cpcName, INT  iMode)
{
    return  (_IoOpen((PCHAR)cpcName, (O_WRONLY | O_CREAT | O_TRUNC), iMode, LW_TRUE));
}
/*********************************************************************************************************
** ��������: open
** ��������: ��(����)һ���ļ�
** �䡡��  : pcName                        �ļ���
**           iFlag                         ��ʽ         O_RDONLY  O_WRONLY  O_RDWR  O_CREAT ...
**           ...                           ����ģʽ, �����´����ļ�ʱ, �˲�����Ч.
** �䡡��  : �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  open (CPCHAR  cpcName, INT  iFlag, ...)
{
    INT         iMode = DEFAULT_FILE_PERM;
    INT         iRet;
    va_list     varlist;
    
    va_start(varlist, iFlag);
    if (iFlag & O_CREAT) {                                              /*  �������                    */
        iMode = va_arg(varlist, INT);
    }
    
    iRet = _IoOpen((PCHAR)cpcName, iFlag, iMode, LW_FALSE);

    va_end(varlist);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: open64
** ��������: ��(����)һ���ļ�
** �䡡��  : pcName                        �ļ���
**           iFlag                         ��ʽ         O_RDONLY  O_WRONLY  O_RDWR  O_CREAT ...
**           ...                           ����ģʽ, �����´����ļ�ʱ, �˲�����Ч.
** �䡡��  : �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  open64 (CPCHAR  cpcName, INT  iFlag, ...)
{
    INT         iMode = DEFAULT_FILE_PERM;
    INT         iRet;
    va_list     varlist;
    
    va_start(varlist, iFlag);
    if (iFlag & O_CREAT) {                                              /*  �������                    */
        iMode = va_arg(varlist, INT);
    }
    
    iRet = _IoOpen((PCHAR)cpcName, iFlag | O_LARGEFILE, iMode, LW_FALSE);

    va_end(varlist);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: unlink
** ��������: delete a file (ANSI: unlink() )
** �䡡��  : 
**           pcName                        �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  unlink (CPCHAR       pcName)
{
    PLW_DEV_HDR    pdevhdrHdr;
    CHAR           cFullFileName[MAX_FILENAME_LENGTH];
             
    INT            iRet;
    INT            iLinkCount = 0;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    if (pcName[0] == PX_EOS) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (ioFullFileNameGet(pcName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    /*
     *  �����е� delete ����һ��Ҫ�ж� cFullFileName ����, ���Ϊ�����ļ�����Ҫ�������ж�
     *  cFullFileName ָ�����Ӷ�����ڲ��ļ���Ŀ¼ʱ, Ӧ�÷��� FOLLOW_LINK_TAIL ����ɾ��
     *  ������Ŀ��(��ʵĿ��)�е��ļ���Ŀ¼, �� cFullFileName ָ����������ļ�����, ��ô
     *  Ӧ��ֱ��ɾ�������ļ�����, ��Ӧ�÷��� FOLLOW_LINK_???? ɾ������Ŀ��! �м�! �м�!
     */
    for (;;) {
        iRet = iosFdUnlink(pdevhdrHdr, cFullFileName);                  /*  ������ڴ򿪵��ļ�          */
        if (iRet == 1) {
            _ErrorHandle(EBUSY);                                        /*  �ļ�������, ����ɾ��        */
            return  (PX_ERROR);
        
        } else if (iRet == ERROR_NONE) {                                /*  ɾ�������ӳ��Զ�ִ��        */
            return  (ERROR_NONE);
        }
        
        iRet = iosDelete(pdevhdrHdr, cFullFileName);
        if (iRet != FOLLOW_LINK_TAIL) {                                 /*  �������ļ�ֱ���˳�          */
            break;
        
        } else {
            if (iLinkCount++ > _S_iIoMaxLinkLevels) {                   /*  �����ļ�����̫��            */
                _ErrorHandle(ELOOP);
                return  (PX_ERROR);
            }
        }
        
        /*
         *  ��������������� FOLLOW_LINK_????, cFullFileName�ڲ�һ����Ŀ��ľ��Ե�ַ, ����/��ʼ���ļ���.
         */
        if (ioFullFileNameGet(cFullFileName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
    }
    
    MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_UNLINK, pcName);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: rename
** ��������: rename a file (ANSI: rename() )
** �䡡��  : 
**           pcOldName                     ���ļ���
**           pcNewName                     ���ļ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  rename (CPCHAR       pcOldName, CPCHAR       pcNewName)
{
    REGISTER LONG   lValue;
    PLW_FD_ENTRY    pfdentry;
    PLW_DEV_HDR     pdevhdrHdr;
    CHAR            cFullFileName[MAX_FILENAME_LENGTH];
    
    INT             iLinkCount = 1;

    REGISTER INT    iFd;
    REGISTER INT    iRet;
             ULONG  ulErrNo;
    
    if ((pcOldName == LW_NULL) || (pcNewName == LW_NULL)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    iFd = open(pcOldName, O_RDONLY | O_PEEKONLY, 0);                    /*  read or write only? XXX     */
    if (iFd < 0) {                                                      /*  open failed                 */
        return  (PX_ERROR);
    }
    
    if (ioFullFileNameGet(pcNewName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        ulErrNo = API_GetLastError();
        close(iFd);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(ulErrNo);
        return  (PX_ERROR);
    }
    
    pfdentry = _IosFileNew(pdevhdrHdr, cFullFileName);                  /*  ����һ����ʱ�� fd_entry     */
    if (pfdentry == LW_NULL) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    for (;;) {
        lValue = iosOpen(pdevhdrHdr, cFullFileName, O_RDWR, 0);
        if ((lValue != FOLLOW_LINK_FILE) && 
            (lValue != FOLLOW_LINK_TAIL)) {                             /*  �������ļ�ֱ���˳�          */
            break;
        
        } else {
            if (iLinkCount++ > _S_iIoMaxLinkLevels) {                   /*  �����ļ�����̫��            */
                _IosFileDelete(pfdentry);
                close(iFd);
                _ErrorHandle(ELOOP);
                return  (PX_ERROR);
            }
        }
    
        /*
         *  ��������������� FOLLOW_LINK_????, cFullFileName�ڲ�һ����Ŀ��ľ��Ե�ַ, ����/��ʼ���ļ���.
         */
        if (ioFullFileNameGet(cFullFileName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
            _IosFileDelete(pfdentry);
            close(iFd);
            _ErrorHandle(EXDEV);
            return  (PX_ERROR);
        }
    }
    
    if (lValue != PX_ERROR) {
        _IosFileSet(pfdentry, pdevhdrHdr, lValue, O_RDONLY, FDSTAT_CLOSING);
        _IosFileClose(pfdentry);                                        /*  �ر�                        */
    }
    
    _IosFileDelete(pfdentry);                                           /*  ɾ����ʱ�� fd_entry         */

    if (_PathBuildLink(cFullFileName, MAX_FILENAME_LENGTH, 
                       LW_NULL, LW_NULL,
                       pdevhdrHdr->DEVHDR_pcName, cFullFileName) < ERROR_NONE) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iFd, FIORENAME, (LONG)cFullFileName);
    
    close(iFd);
    
    if (iRet == ERROR_NONE) {
        MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_MOVE_FROM, pcOldName);
        MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_MOVE_TO,   pcNewName);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: close
** ��������: close a file (ANSI: close() )
** �䡡��  : 
**           iFd                           �ļ�������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  close (INT  iFd)
{
    REGISTER INT    iRet;
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    iRet = iosClose(iFd);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: read
** ��������: read bytes from a file or device (ANSI: read() )
** �䡡��  : 
**           iFd                           �ļ�������
**           pvBuffer                      ���ջ�����
**           stMaxBytes                    ���ջ�������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  read (INT     iFd,
               PVOID   pvBuffer,
               size_t  stMaxBytes)
{
    REGISTER ssize_t  sstRet;
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    sstRet = iosRead(iFd, (PCHAR)pvBuffer, stMaxBytes);

    return  (sstRet);
}
/*********************************************************************************************************
** ��������: pread
** ��������: read bytes from a file or device with a given position (ANSI: pread() )
** �䡡��  : 
**           iFd                           �ļ�������
**           pvBuffer                      ���ջ�����
**           stMaxBytes                    ���ջ�������С
**           oftPos                        ָ��λ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  pread (INT     iFd,
                PVOID   pvBuffer,
                size_t  stMaxBytes,
                off_t   oftPos)
{
    REGISTER ssize_t  sstRet;
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    sstRet = iosPRead(iFd, (PCHAR)pvBuffer, stMaxBytes, oftPos);

    return  (sstRet);
}
/*********************************************************************************************************
** ��������: pread64
** ��������: read bytes from a file or device with a given position (ANSI: pread() )
** �䡡��  : 
**           iFd                           �ļ�������
**           pvBuffer                      ���ջ�����
**           stMaxBytes                    ���ջ�������С
**           oftPos                        ָ��λ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  pread64 (INT     iFd,
                  PVOID   pvBuffer,
                  size_t  stMaxBytes,
                  off64_t oftPos)
{
    return  (pread(iFd, pvBuffer, stMaxBytes, oftPos));
}
/*********************************************************************************************************
** ��������: write
** ��������: write bytes from a file or device (ANSI: write() )
** �䡡��  : 
**           iFd                           �ļ�������
**           pvBuffer                      ������Ҫд���ļ����ݵĻ�����
**           stNBytes                      д����ֽ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  write (INT     iFd,
                CPVOID  pvBuffer,
                size_t  stNBytes)
{
    REGISTER ssize_t  sstRet;
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    sstRet = iosWrite(iFd, (CPCHAR)pvBuffer, stNBytes);
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: pwrite
** ��������: write bytes from a file or device with a given position (ANSI: pwrite() )
** �䡡��  : 
**           iFd                           �ļ�������
**           pvBuffer                      ������Ҫд���ļ����ݵĻ�����
**           stNBytes                      д����ֽ���
**           oftPos                        ָ��λ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  pwrite (INT     iFd,
                 CPVOID  pvBuffer,
                 size_t  stNBytes,
                 off_t   oftPos)
{
    REGISTER ssize_t  sstRet;
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    sstRet = iosPWrite(iFd, (CPCHAR)pvBuffer, stNBytes, oftPos);

    return  (sstRet);
}
/*********************************************************************************************************
** ��������: pwrite64
** ��������: write bytes from a file or device with a given position (ANSI: pwrite() )
** �䡡��  : 
**           iFd                           �ļ�������
**           pvBuffer                      ������Ҫд���ļ����ݵĻ�����
**           stNBytes                      д����ֽ���
**           oftPos                        ָ��λ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  pwrite64 (INT     iFd,
                   CPVOID  pvBuffer,
                   size_t  stNBytes,
                   off64_t oftPos)
{
    return  (pwrite(iFd, pvBuffer, stNBytes, oftPos));
}
/*********************************************************************************************************
** ��������: ioctl
** ��������: perform an I/O control function (  ANSI: ioctl() )
** �䡡��  : 
**           iFd                           �ļ�������
**           iFunction                     ����
**           lArg                          ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  ioctl (INT   iFd,
            INT   iFunction,
            ...)
{
    va_list     varlist;
    LONG        lArg;                                                   /*  can store a void *          */
    INT         iRet;
    
    va_start(varlist, iFunction);
    lArg = va_arg(varlist, LONG);
    
    iRet = iosIoctl(iFd, iFunction, lArg);
    
    va_end(varlist);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: lseek
** ��������: set a file read/write pointer
** �䡡��  : 
**           iFd                           �ļ�������
**           oftOffset                     ƫ����
**           iWhence                       ��λ��׼
** �䡡��  : ��ǰ�ļ�ָ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
off_t  lseek (INT      iFd,
              off_t    oftOffset,
              INT      iWhence)
{
             off_t      oftWhere;
             off_t      oftNBytes;
             
             INT        iOldErr;
    REGISTER INT        iError;
    REGISTER off_t      oftRetVal;
      struct stat       statFile;

    iOldErr = errno;
    oftRetVal = API_IosLseek(iFd, oftOffset, iWhence);
    if (oftRetVal != PX_ERROR) {                                        /*  ���ȿ�����������            */
        return  (oftRetVal);
    } else {
        errno = iOldErr;                                                /*  �ָ� errno                  */
    }
    
    switch (iWhence) {
    
    case SEEK_SET:
        /*
         *  �Ե�ǰָ��Ϊָ��, ֱ������ָ��.
         */
        if (oftOffset < 0) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        return  ((ioctl(iFd, FIOSEEK, (LONG)&oftOffset) == 0) ? oftOffset : (PX_ERROR));
        
    case SEEK_CUR:
        /*
         *  ��õ�ǰ�ļ�ָ��.
         */
        iError = ioctl(iFd, FIOWHERE, (LONG)&oftWhere);
        if (iError == (PX_ERROR)) {
            return  (PX_ERROR);
        }
        
        /*
         *  ����ƫ���������������ļ�ָ��.
         */
        oftOffset += oftWhere;
        if (oftOffset < 0) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        return  ((ioctl(iFd, FIOSEEK, (LONG)&oftOffset) == 0) ? oftOffset : (PX_ERROR));
    
    case SEEK_END:
        /*
         *  ��õ�ǰ�ļ�ָ��.
         */
        iError = ioctl(iFd, FIOWHERE, (LONG)&oftWhere);
        if (iError == (PX_ERROR)) {
            goto    __fiofstat;
        }
        
        /*
         *  ���ʣ�����ݴ�С.
         */
        if (ioctl(iFd, FIONREAD64, (LONG)&oftNBytes) == (PX_ERROR)) {
            goto    __fiofstat;
        }
        
        oftOffset += (oftWhere + oftNBytes);
        if (oftOffset < 0) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        return  ((ioctl(iFd, FIOSEEK, (LONG)&oftOffset) == 0) ? oftOffset : (PX_ERROR));
        
__fiofstat:
        /*
         *  ����ļ�����
         */
        iError = fstat(iFd, &statFile);
        if (iError == (PX_ERROR)) {
            return  (PX_ERROR);
        }
        
        /*
         *  �����µ�ָ��λ�ò����������ļ�ָ��.
         */
        oftOffset += statFile.st_size;
        if (oftOffset < 0) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        return  ((ioctl(iFd, FIOSEEK, (LONG)&oftOffset) == 0) ? oftOffset : (PX_ERROR));
    
    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: lseek64
** ��������: set a file read/write pointer
** �䡡��  : 
**           iFd                           �ļ�������
**           oftOffset                     ƫ����
**           iWhence                       ��λ��׼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
off64_t  lseek64 (INT      iFd,
                  off64_t  oftOffset,
                  INT      iWhence)
{
    return  (lseek(iFd, oftOffset, iWhence));
}
/*********************************************************************************************************
** ��������: llseek
** ��������: set a file read/write pointer
** �䡡��  : 
**           iFd                           �ļ�������
**           oftOffset                     ƫ����
**           iWhence                       ��λ��׼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
off64_t  llseek (INT      iFd,
                 off64_t  oftOffset,
                 INT      iWhence)
{
    return  (lseek(iFd, oftOffset, iWhence));
}
/*********************************************************************************************************
** ��������: readv
** ��������: read data from a device into scattered buffers  (��������֧�ֻ��λ�����)
** �䡡��  : 
**           iFd                           �ļ�������
**           piovec                        ���ջ������б�
**           iIovcnt                       ���ջ������б��л������ĸ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  readv (INT             iFd,
                struct iovec    *piovec,
                INT             iIovcnt)
{
    REGISTER INT          iI;
    REGISTER PCHAR        pcBuf;
    REGISTER ssize_t      sstBytesToRead;
    REGISTER ssize_t      sstTotalBytesRead = 0;
    REGISTER ssize_t      sstBytesRead;
             ssize_t      sstRet = 0;
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    for (iI = 0; iI < iIovcnt; iI++) {                                  /*  ��ʼ�������������          */
    
        pcBuf          = (PCHAR)piovec[iI].iov_base;
        sstBytesToRead = (ssize_t)piovec[iI].iov_len;
        
        while (sstBytesToRead > 0) {                                    /*  ��ǰ�������Ƿ�����          */
            
            sstBytesRead = iosRead(iFd, pcBuf, (size_t)sstBytesToRead);
            
            if (sstBytesRead < 0) {                                     /*  ����                        */
                if (sstTotalBytesRead > 0) {
                    sstRet = sstTotalBytesRead;
                } else {
                    sstRet = PX_ERROR;
                }
                goto    __read_over;
            }
            
            if (sstBytesRead == 0) {                                    /*  �ļ��Ѿ�û������            */
                sstRet = sstTotalBytesRead;
                goto    __read_over;
            }
            
            sstTotalBytesRead += sstBytesRead;
            sstBytesToRead    -= sstBytesRead;
            
            pcBuf += sstBytesRead;
        }
    }
    sstRet = sstTotalBytesRead;

__read_over:
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: writev
** ��������: read data from a device into scattered buffers  (��������֧�ֻ��λ�����)
** �䡡��  : 
**           iFd                           �ļ�������
**           piovec                        ���ͻ������б�
**           uiMaxBytes                    ���ͻ������б��л������ĸ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  writev (INT                    iFd,
                 const struct iovec    *piovec,
                 INT                    iIovcnt)
{
    REGISTER INT          iI;
    REGISTER PCHAR        pcData;
    REGISTER ssize_t      sstBytesToWrite;
    REGISTER ssize_t      sstTotalBytesWriten = 0;
    REGISTER ssize_t      sstBytesWriten;
             ssize_t      sstRet = 0;
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    for (iI = 0; iI < iIovcnt; iI++) {                                  /*  ��ʼ�������������          */
        
        pcData          = (PCHAR)piovec[iI].iov_base;
        sstBytesToWrite = (ssize_t)piovec[iI].iov_len;
        
        while (sstBytesToWrite > 0) {
            
            sstBytesWriten = iosWrite(iFd, pcData, (size_t)sstBytesToWrite);
            
            if (sstBytesWriten < 0) {
                if (sstTotalBytesWriten > 0) {
                    sstRet = sstTotalBytesWriten;
                } else {
                    sstRet = PX_ERROR;
                }
                goto    __write_over;
            }
            
            sstTotalBytesWriten += sstBytesWriten;
            sstBytesToWrite     -= sstBytesWriten;
            
            pcData += sstBytesWriten;
        }
    }
    sstRet = sstTotalBytesWriten;
    
__write_over:
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: API_IoFullFileNameGet
** ��������: ��������ļ���
** �䡡��  : 
**           pcPathName                    ·����
**           ppdevhdr                      �豸ͷ˫ָ�� (��ȡ��·����Ӧ���豸)
**           pcFullFileName                ����·����   (�������: �ü����豸����, �������豸�������ִ�)
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  :  pcFullFileName �����������Ҫ�� PATH_MAX + 1 ���ֽڵĿռ�!
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IoFullFileNameGet (CPCHAR         pcPathName,
                            PLW_DEV_HDR   *ppdevhdr,
                            PCHAR          pcFullFileName)
{
    size_t   stFullLen;
    PCHAR    pcTail;
    CHAR     cFullPathName[MAX_FILENAME_LENGTH];
    
    lib_bzero(cFullPathName, MAX_FILENAME_LENGTH);                      /*  CLEAR                       */
    
    if (_PathCat(_PathGetDef(), pcPathName, cFullPathName) != ERROR_NONE) {
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    /*
     *  ��Ҫ�ڽ�β���������� / ����, ����: /aaa/bbb/ccc/ ӦΪ /aaa/bbb/ccc
     */
    stFullLen = lib_strlen(cFullPathName);
    if (stFullLen > 1) {                                                /*  ������� "/" ������ȥ       */
        if (cFullPathName[stFullLen - 1] == PX_DIVIDER) {
            cFullPathName[stFullLen - 1] =  PX_EOS;                     /*  ȥ��ĩβ�� /                */
        }
        _PathCondense(cFullPathName);                                   /*  ȥ�� ../ ./                 */
    }
    
    *ppdevhdr = iosDevFind(cFullPathName, &pcTail);
    if ((*ppdevhdr) == LW_NULL) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    /*
     *  ��� ppdevhdr == rootfs dev header ��, pcTail ��ʼλ��û�� '/' �ַ�, 
     *  �˴���Ҫ�������ڸ��ļ�ϵͳ�豸��Ϊ "/" ����������.
     */
    if (pcTail && ((*pcTail != PX_EOS) && (*pcTail != PX_DIVIDER))) {
        lib_strlcpy(&pcFullFileName[1], pcTail, MAX_FILENAME_LENGTH - 1);
        pcFullFileName[0] = PX_ROOT;
    
    } else {
        lib_strlcpy(pcFullFileName, pcTail, MAX_FILENAME_LENGTH);
    }
    
    return  ((INT)ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IoPathCondense
** ��������: ��һ������ . �� .. ��Ŀ¼ѹ��Ϊ��׼·��
** �䡡��  : pcPath            ����δѹ����·��
**           pcPathCondense    ���ѹ�����·��
**           stSize            ��������С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IoPathCondense (CPCHAR  pcPath, PCHAR  pcPathCondense, size_t  stSize)
{
    size_t   stFullLen;
    CHAR     cFullPathName[MAX_FILENAME_LENGTH];
    
    lib_bzero(cFullPathName, MAX_FILENAME_LENGTH);                      /*  CLEAR                       */
    
    if (_PathCat(_PathGetDef(), pcPath, cFullPathName) != ERROR_NONE) {
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    /*
     *  ��Ҫ�ڽ�β���������� / ����, ����: /aaa/bbb/ccc/ ӦΪ /aaa/bbb/ccc
     */
    stFullLen = lib_strlen(cFullPathName);
    if (stFullLen > 1) {                                                /*  ������� "/" ������ȥ       */
        if (cFullPathName[stFullLen - 1] == PX_DIVIDER) {
            cFullPathName[stFullLen - 1] =  PX_EOS;                     /*  ȥ��ĩβ�� /                */
        }
        _PathCondense(cFullPathName);                                   /*  ȥ�� ../ ./                 */
    }
    
    lib_strlcpy(pcPathCondense, cFullPathName, stSize);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IoPrivateEnv
** ��������: ��ǰ�߳̽��� io ˽�л��� 
             ��ǰ����ӵ��˽�е����Ŀ¼, ��Ҫ����ϵͳ����, �û���Ҫ����ʹ��, 
** �䡡��  : NONE
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺�����������ں��߳�, �����ڵ��߳̽���������, �����ڵ��̹߳��������ͳһ�����Ŀ¼.
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IoPrivateEnv (VOID)
{
             PLW_CLASS_TCB    ptcbCur;
    REGISTER PLW_IO_ENV       pioe;

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  ���ж��е���                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    if (ptcbCur->TCB_pvIoEnv) {
        return  (ERROR_NONE);                                           /*  �Ѿ�����˽�� IO ����        */
    }
    
    pioe = _IosEnvCreate();                                             /*  ���� io ����                */
    if (pioe == LW_NULL) {
        return  (PX_ERROR);
    }
    
    _IosEnvInherit(pioe);                                               /*  �����ȼ̳в��ܱ��� IO ����  */
    
    __KERNEL_MODE_PROC(
        ptcbCur->TCB_pvIoEnv = (PVOID)pioe;                             /*  ���浱ǰ IO ����            */
    );
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IoDefPathSet
** ��������: �趨��ǰ����·�� (�����Ǿ���·��, �� "/" ��Ϊ��ʼ)
** �䡡��  : pcName                        ��Ĭ��Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_IoDefPathSet (CPCHAR       pcName)
{
    PCHAR    pcTail = (PCHAR)pcName;
    
    if (iosDevFind(pcName, &pcTail) == LW_NULL) {
        return  (PX_ERROR);
    }
    
    if (pcTail == pcName) {                                             /*  ����һ����Ч���豸����      */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    if (lib_strnlen(pcName, MAX_FILENAME_LENGTH) >= MAX_FILENAME_LENGTH) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_IO_NAME_TOO_LONG);
        return  (PX_ERROR);
    }
    
    lib_strcpy(_PathGetDef(), pcName);
    
    MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_CHDIR, pcName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IoDefPathCat
** ��������: concatenate to current default path
** �䡡��  : pcName                        �����µ�Ĭ��Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_IoDefPathCat (CPCHAR  pcName)
{
    CHAR    cNewPath[MAX_FILENAME_LENGTH];
    PCHAR   pcTail;
    
    /*
     *  pcName ������豸�� cNewPath Ϊ���豸, �������ӵ� _PathGetDef() ����.
     */
    if (_PathCat(_PathGetDef(), pcName, cNewPath) != ERROR_NONE) {
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }

    /*
     *  ѹ��Ŀ¼, ȥ�� . �� ..
     */
    _PathCondense(cNewPath);

    /*
     *  ���Ҫѹ��Ŀ¼��ĺϷ���
     */
    iosDevFind(cNewPath, &pcTail);
    if (pcTail == cNewPath) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    lib_strlcpy(_PathGetDef(), cNewPath, MAX_FILENAME_LENGTH);          /*  ��Ϊ�µĵ�ǰĿ¼            */
    
    MONITOR_EVT(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_CHDIR, cNewPath);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IoDefPathGet
** ��������: get the current default path
** �䡡��  : pcName                        ���Ĭ��Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_IoDefPathGet (PCHAR  pcName)
{
    lib_strcpy(pcName, _PathGetDef());
}
/*********************************************************************************************************
** ��������: chdir
** ��������: �趨��ǰ����·�� (����Ϊ���·��)
** �䡡��  : pcName                        ��Ĭ��Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  chdir (CPCHAR  pcName)
{
    struct stat   statGet;
    
    if (stat(pcName, &statGet) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (!S_ISDIR(statGet.st_mode)) {                                    /*  ��Ŀ¼                      */
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    return  (API_IoDefPathCat(pcName));
}
/*********************************************************************************************************
** ��������: cd
** ��������: �趨��ǰ����·�� (����Ϊ���·��)
** �䡡��  : pcName                        ����Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  cd (CPCHAR  pcName)
{
    struct stat   statGet;
    
    if (stat(pcName, &statGet) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (!S_ISDIR(statGet.st_mode)) {                                    /*  ��Ŀ¼                      */
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    return  (API_IoDefPathCat(pcName));
}
/*********************************************************************************************************
** ��������: fchdir
** ��������: �����ļ����������ļ��趨Ϊ��ǰ����·��
** �䡡��  : iFd                           �ļ�������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  fchdir (INT  iFd)
{
    REGISTER PLW_FD_ENTRY  pfdentry;
             struct stat   statGet;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if (pfdentry == LW_NULL) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    if ((pfdentry->FDENTRY_iFlag & O_WRONLY) ||
        (pfdentry->FDENTRY_iFlag & O_RDWR)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file is not read only.\r\n");
        _ErrorHandle(EACCES);
        return  (PX_ERROR);
    }
    
    if (fstat(iFd, &statGet) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (!S_ISDIR(statGet.st_mode)) {                                    /*  �� dir �ļ�������           */
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    return  (API_IoDefPathSet(pfdentry->FDENTRY_pcName));               /*  ���õ�ǰ����Ŀ¼            */
}
/*********************************************************************************************************
** ��������: chroot
** ��������: �ı��Ŀ¼, SylixOS ��ʱ��֧��.
** �䡡��  : pcPath                        ��Ŀ¼·��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  chroot (CPCHAR  pcPath)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: getcwd
** ��������: get the current default path (POSIX)
** �䡡��  : 
**           pcBuffer                      ���Ĭ��Ŀ¼
**           stByteSize                    ��������С
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PCHAR  getcwd (PCHAR  pcBuffer, size_t  stByteSize)
{
    if (stByteSize == 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "stByteSize invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    if (stByteSize < lib_strlen(_PathGetDef()) + 1) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "stByteSize invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    if (!pcBuffer) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcBuffer invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    lib_strncpy(pcBuffer, _PathGetDef(), stByteSize);                   /*  use strncpy                 */
    
    return  (pcBuffer);
}
/*********************************************************************************************************
** ��������: getwd
** ��������: get the current default path
** �䡡��  : pcName                        ���Ĭ��Ŀ¼
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PCHAR  getwd (PCHAR  pcName)
{
    if (!pcName) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    lib_strcpy(pcName, _PathGetDef());
    return  (pcName);
}
/*********************************************************************************************************
** ��������: API_IoDefDevGet
** ��������: get current default device
** �䡡��  : pcDevName                     ����豸��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_IoDefDevGet (PCHAR  pcDevName)
{
    REGISTER PLW_DEV_HDR        pdevhdrHdr;
             PCHAR              pcTail;
             
    pdevhdrHdr = iosDevFind(_PathGetDef(), &pcTail);
    if (pdevhdrHdr == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "default device invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        *pcDevName = PX_EOS;
    
    } else {
        lib_strcpy(pcDevName, pdevhdrHdr->DEVHDR_pcName);
    }
}
/*********************************************************************************************************
** ��������: API_IoDefDirGet
** ��������: get current default directory
** �䡡��  : pcDirName                     ���Ŀ¼��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_IoDefDirGet (PCHAR  pcDirName)
{
    PCHAR        pcTail;
    
    if (iosDevFind(_PathGetDef(), &pcTail) == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "dir invalidate.\r\n");
        _ErrorHandle(ENOENT);
        *pcDirName = PX_EOS;
    
    } else {
        lib_strcpy(pcDirName, pcTail);
    }
}
/*********************************************************************************************************
** ��������: API_IoGlobalStdSet
** ��������: set the file descriptor for global standard input/output/error
** �䡡��  : 
**           iStdFd                        ������     STD_IN  STD_OUT  STD_ERR
**           iNewFd                        �ļ�������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_IoGlobalStdSet (INT  iStdFd, INT  iNewFd)
{
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ���ں�����֧��            */
        _ErrorHandle(EINVAL);
        return;
    }

    if (STD_VALID(iStdFd)) {
        _G_iIoStdFd[iStdFd] = iNewFd;
    }
}
/*********************************************************************************************************
** ��������: API_IoGlobalStdGet
** ��������: get the file descriptor for global standard input/output/error
** �䡡��  : 
**           iStdFd                        ������    STD_IN  STD_OUT  STD_ERR
** �䡡��  : �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_IoGlobalStdGet (INT  iStdFd)
{
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  �ں�����                    */
        return  (iStdFd);                                               /*  ֱ�ӷ����ļ�������          */
    }

    return  ((STD_VALID(iStdFd) ? (_G_iIoStdFd[iStdFd]) : (PX_ERROR)));
}
/*********************************************************************************************************
** ��������: API_IoTaskStdSet
** ��������: set the file descriptor for task standard input/output/error
** �䡡��  : 
**           ulId                          ������
**           iStdFd                        ������    STD_IN  STD_OUT  STD_ERR
**           iNewFd                        �ļ�������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ֻ�������ں�����, �������̲߳�֧��.
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_IoTaskStdSet (LW_OBJECT_HANDLE  ulId,
                        INT               iStdFd,
                        INT               iNewFd)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ���ں�����֧��            */
        _ErrorHandle(EINVAL);
        return;
    }
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return;
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _ErrorHandle(ERROR_THREAD_NULL);
        return;
    }
#endif
    
    iregInterLevel = __KERNEL_ENTER_IRQ();
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return;
    }
    
    if (STD_VALID(iStdFd)) {
        ptcb = __GET_TCB_FROM_INDEX(usIndex);
        ptcb->TCB_iTaskStd[iStdFd] = iNewFd;
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);
}
/*********************************************************************************************************
** ��������: API_IoTaskStdGet
** ��������: get the file descriptor for task standard input/output/error
** �䡡��  : 
**           ulId                          ������
**           iStdFd                        ������    STD_IN  STD_OUT  STD_ERR
** �䡡��  : �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �ڽ����пɻ�ȡ��ǰ�����׼ IO ������Ϣ, ������������ں����񴴽�, ��̳��ں����� 0 1 2 �ļ�.

                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_IoTaskStdGet (LW_OBJECT_HANDLE  ulId,
                       INT               iStdFd)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    
    REGISTER INT                   iTaskFd;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (PX_ERROR);
    }
#endif
    
    iregInterLevel = __KERNEL_ENTER_IRQ();
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (PX_ERROR);
    }
    
    if (STD_VALID(iStdFd)) {
        ptcb = __GET_TCB_FROM_INDEX(usIndex);
        iTaskFd = ptcb->TCB_iTaskStd[iStdFd];
        
        if (STD_VALID(iTaskFd)) {
            __KERNEL_EXIT_IRQ(iregInterLevel);
            return  (_G_iIoStdFd[iTaskFd]);
        
        } else {
            __KERNEL_EXIT_IRQ(iregInterLevel);
            return  (iTaskFd);
        }
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: isatty
** ��������: return whether the underlying driver is a tty device
** �䡡��  : 
**           iFd                           �ļ�������
** �䡡��  : BOOL                          �Ƿ���tty�豸
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
BOOL  isatty (INT  iFd)
{   
    REGISTER INT     iRet;
             BOOL    bIsTty;
    
    iRet = ioctl(iFd, FIOISATTY, (LONG)&bIsTty);
    
    if ((iRet < 0) || (bIsTty == LW_FALSE)) {
        return  (LW_FALSE);
    } else {
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** ��������: ttyname
** ��������: ���ص�ǰ�ն��豸������.
** �䡡��  : 
**           iFd                           �ļ�������
** �䡡��  : �ն˵�����, ������ն�, �����ļ�����������, �򷵻� NULL.
**           errno = EBADF   ��ʾ�ļ��Ƿ�
**           errno = ENOTTY  ���ļ����������ն��豸������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PCHAR   ttyname (INT  iFd)
{
    REGISTER BOOL          bIsTty;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    bIsTty = isatty(iFd);
    if (bIsTty == LW_FALSE) {
        errno = ENOTTY;
        return  (LW_NULL);
    }
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    return  (pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_pcName);              /*  ʹ���豸��                  */
}
/*********************************************************************************************************
** ��������: ttyname_r
** ��������: ���ص�ǰ�ն��豸������ (������).
** �䡡��  : 
**           iFd                           �ļ�������
**           pcBuffer                      ���ֻ���
**           stLen                         �����С
** �䡡��  : �ն˵�����, ������ն�, �����ļ�����������, �򷵻� NULL.
**           errno = EBADF   ��ʾ�ļ��Ƿ�
**           errno = EINVAL  ��������
**           errno = ENOTTY  ���ļ����������ն��豸������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PCHAR  ttyname_r (INT  iFd, PCHAR  pcBuffer, size_t  stLen)
{
    REGISTER BOOL          bIsTty;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    if ((pcBuffer == LW_NULL) ||
        (stLen    == 0)) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    bIsTty = isatty(iFd);
    if (bIsTty == LW_FALSE) {
        errno = ENOTTY;
        return  (LW_NULL);
    }

    pfdentry = _IosFileGet(iFd, LW_FALSE);
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    lib_strlcpy(pcBuffer, pfdentry->FDENTRY_pdevhdrHdr->DEVHDR_pcName, (INT)stLen);
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    return  (pcBuffer);
}
/*********************************************************************************************************
** ��������: dup2kernel
** ��������: �ӽ����︴��һ���ļ����������ں���, 
**           ���ҹ���ͬһ���ļ�����.
** �䡡��  : iFd                           �ļ�������
** �䡡��  : >=0 ��ʾ��ȷ���ļ�������
**           -1 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  dup2kernel (INT  iFd)
{
    REGISTER PLW_FD_ENTRY  pfdentry;
             INT           iFdNew;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    __KERNEL_SPACE_ENTER();
    iFdNew = _IosFileDup(pfdentry, 0);
    __KERNEL_SPACE_EXIT();
    if (iFdNew < 0) {
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        _ErrorHandle(EMFILE);
        return  (PX_ERROR);
    }
    __LW_FD_CREATE_HOOK(iFdNew, 0);                                     /*  �ں��ļ�������              */
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    MONITOR_EVT_INT3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_DUP, iFd, iFdNew, 1, LW_NULL);
    
    return  (iFdNew);
}
/*********************************************************************************************************
** ��������: dupminfd
** ��������: ����һ���ļ�������, ��dup�������ص��ļ�������һ���ǵ�ǰ�����ļ��������е���С��ֵ
**           ���ҹ���ͬһ���ļ�����.
** �䡡��  : iFd                           �ļ�������
**           iMinFd                        ��С�ļ�������
** �䡡��  : >=0 ��ʾ��ȷ���ļ�������
**           -1 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  dupminfd (INT  iFd, INT  iMinFd)
{
    REGISTER PLW_FD_ENTRY  pfdentry;
             INT           iFdNew;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    iFdNew = _IosFileDup(pfdentry, iMinFd);
    if (iFdNew < 0) {
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        _ErrorHandle(EMFILE);
        return  (PX_ERROR);
    }
    __LW_FD_CREATE_HOOK(iFdNew, __PROC_GET_PID_CUR());
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    MONITOR_EVT_INT3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_DUP, iFd, iFdNew, 0, LW_NULL);
    
    return  (iFdNew);
}
/*********************************************************************************************************
** ��������: dup
** ��������: ����һ���ļ�������, ��dup�������ص��ļ�������һ���ǵ�ǰ�����ļ��������е���С��ֵ
**           ���ҹ���ͬһ���ļ�����.
** �䡡��  : iFd                           �ļ�������
** �䡡��  : >=0 ��ʾ��ȷ���ļ�������
**           -1 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  dup (INT  iFd)
{
    return  (dupminfd(iFd, 0));
}
/*********************************************************************************************************
** ��������: dup2
** ��������: POSIX dup2()
** �䡡��  : iFd1                          �ļ�������1
**           iFd2                          �ļ�������2
** �䡡��  : >=0 ��ʾ��ȷ���ļ�������
**           -1 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  dup2 (INT  iFd1, INT  iFd2)
{
    REGISTER PLW_FD_ENTRY  pfdentry1;
    REGISTER PLW_FD_ENTRY  pfdentry2;
    
    if (__PROC_GET_PID_CUR() == 0) {                                    /*  dup2 ֻ�ڽ����б�֧��       */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    if (iFd2 == iFd1) {                                                 /*  �������ļ���������ͬ        */
        _ErrorHandle(ENOTSUP);                                          /*  ��֧��                      */
        return  (PX_ERROR);
    }
    
    pfdentry1 = _IosFileGet(iFd1, LW_FALSE);
    pfdentry2 = _IosFileGet(iFd2, LW_FALSE);
    if ((pfdentry1 == LW_NULL) || (pfdentry1->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd1);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    if (pfdentry1 == pfdentry2) {
        return  (iFd2);
    }
    
__re_check:                                                             /*  ���¼�� FD2 �Ƿ񱻹ر�     */
    _IosLock();                                                         /*  ���� IO �ٽ���              */
    pfdentry2 = _IosFileGet(iFd2, LW_FALSE);
    if (pfdentry2) {
        INT  iRef = _IosFileRefGet(iFd2);
        if (iRef > 1) {
            _IosUnlock();                                               /*  �˳� IO �ٽ���              */
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        close(iFd2);                                                    /*  �ر� fd2                    */
        goto    __re_check;
    }
    iFd2 = _IosFileDup2(pfdentry1, iFd2);
    if (iFd2 < 0) {
        _IosUnlock();                                                   /*  �˳� IO �ٽ���              */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    __LW_FD_CREATE_HOOK(iFd2, __PROC_GET_PID_CUR());
    _IosUnlock();                                                       /*  �˳� IO �ٽ���              */
    
    MONITOR_EVT_INT3(MONITOR_EVENT_ID_IO, MONITOR_EVENT_IO_DUP, iFd1, iFd2, 0, LW_NULL);
    
    return  (iFd2);
}
/*********************************************************************************************************
  ע��: һ�º��� sylixos δʹ��, �����ڼ��� linux ����.
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: makedev
** ��������: sylixos δʹ��, �����ڼ��� linux ����
** �䡡��  : 
**           major      ���豸��
**           minor      ���豸��
** �䡡��  : dev_t
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
dev_t   makedev (uint_t  major, uint_t minor)
{
    return  (dev_t)((major << 20) | minor);
}
/*********************************************************************************************************
** ��������: major
** ��������: sylixos δʹ��, �����ڼ��� linux ����
** �䡡��  : 
**           dev        �豸��
** �䡡��  : ���豸��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
uint_t  major (dev_t dev)
{
    return  (uint_t)(dev >> 20);
}
/*********************************************************************************************************
** ��������: minor
** ��������: sylixos δʹ��, �����ڼ��� linux ����
** �䡡��  : 
**           dev        �豸��
** �䡡��  : ���豸��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
uint_t  minor (dev_t dev)
{
    return  (uint_t)(dev & 0xfffff);
}
/*********************************************************************************************************
** ��������: realpath
** ��������: �˺������������� pcPath ��ָ�����·��ת���ɾ���·������ڲ��� pcResolvedPath 
             ��ָ���ַ��������ָ���� (��ʱ����֧���滻�ڲ��ķ�������)
** �䡡��  : pcPath             ����·��
**           pcResolvedPath     ��ȡ����·�������ַ (ע��, ���ٱ�֤ PATH_MAX + 1 ���ֽ�)
** �䡡��  : ����·��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PCHAR  realpath (CPCHAR  pcPath, PCHAR  pcResolvedPath)
{
    CHAR            cFullPathName[MAX_FILENAME_LENGTH];
    PLW_DEV_HDR     pdevhdrHdr;
    size_t          stDevNameLen;

    if (!pcPath || !pcResolvedPath) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    if (ioFullFileNameGet(pcPath, &pdevhdrHdr, cFullPathName) != ERROR_NONE) {
        return  (LW_NULL);
    }
    
    if (cFullPathName[0] == PX_EOS) {
        lib_strcpy(pcResolvedPath, pdevhdrHdr->DEVHDR_pcName);
    
    } else {
        stDevNameLen = lib_strlen(pdevhdrHdr->DEVHDR_pcName);
        if (stDevNameLen == 1) {                                        /*  ��Ŀ¼�豸 "/"              */
            lib_strcpy(pcResolvedPath, cFullPathName);                  /*  �������·���ļ���          */
        
        } else {
            lib_strcpy(pcResolvedPath, pdevhdrHdr->DEVHDR_pcName);
            lib_strcpy(&pcResolvedPath[stDevNameLen], cFullPathName);
        }
    }
    
    return  (pcResolvedPath);
}
/*********************************************************************************************************
** ��������: flock
** ��������: ��ʽ�ļ�������, ���������ļ�, �����Ҫ�ļ��ֽ���, ����Ҫʹ�� fcntl ����.
** �䡡��  : iFd                �ļ�������
**           iOperation         �������� LOCK_SH / LOCK_EX / LOCK_UN ( | LOCK_NB ��ʾ������)
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  flock (INT iFd, INT iOperation)
{
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    return  (_FdLockfProc(pfdentry, iOperation, __PROC_GET_PID_CUR()));
}
/*********************************************************************************************************
** ��������: lockf
** ��������: POSIX �ṩ����һ���ļ���
** �䡡��  : iFd                �ļ�������
**           iCmd               F_ULOCK     Unlock locked sections.
**                              F_LOCK      Lock a section for exclusive use.
**                              F_TLOCK     Test and lock a section for exclusive use.
**                              F_TEST      Test a section for locks by other processes.
**           oftLen             Ҫ��������Դ���ļ��е�ǰƫ������ʼ��
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����û���ж��Ƿ�ӵ��дȨ��
                                           API ����
*********************************************************************************************************/
LW_API 
INT  lockf (INT iFd, INT iCmd, off_t oftLen)
{
             INT           iIoCtlCmd;
             INT           iError;
      struct flock         fl;
    REGISTER PLW_FD_ENTRY  pfdentry;
    
    pfdentry = _IosFileGet(iFd, LW_FALSE);
    if ((pfdentry == LW_NULL) || (pfdentry->FDENTRY_ulCounter == 0)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "file descriptor invalidate: %d.\r\n", iFd);
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);
        return  (PX_ERROR);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    fl.l_whence = SEEK_CUR;
    fl.l_start  = 0;
    fl.l_len    = oftLen;
    fl.l_pid    = __PROC_GET_PID_CUR();
    
    switch (iCmd) {
    
    case F_ULOCK:
        fl.l_type = F_UNLCK;
        iIoCtlCmd = FIOSETLK;                                           /*  ��������Ϊ����״̬          */
        break;
        
    case F_LOCK:
        fl.l_type = F_WRLCK;
        iIoCtlCmd = FIOSETLKW;                                          /*  ��������Ϊ����״̬          */
        break;
        
    case F_TLOCK:
        fl.l_type = F_WRLCK;
        iIoCtlCmd = FIOSETLK;                                           /*  ������������                */
        break;
    
    case F_TEST:
        fl.l_type = F_WRLCK;
        iIoCtlCmd = FIOGETLK;                                           /*  ���ָ���������״̬        */
        break;
    
    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iError = _FdLockfIoctl(pfdentry, iIoCtlCmd, (struct flock *)&fl);
    if ((iError == ERROR_NONE) && (iCmd == F_TEST)) {
        if (fl.l_type == F_UNLCK) {                                     /*  ��������Է���              */
            return  (ERROR_NONE);
        
        } else {                                                        /*  �������������            */
            _ErrorHandle(EACCES);
            return  (PX_ERROR);
        }
    } else {
        return  (iError);
    }
}
/*********************************************************************************************************
** ��������: umask
** ��������: �����ļ�����������
** �䡡��  : modeMask            �µ�������
** �䡡��  : ԭ�ȵ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
mode_t  umask (mode_t modeMask)
{
    PLW_IO_ENV  pioeDef = _IosEnvGetDef();
    mode_t      modeOld;
    
    __KERNEL_ENTER();
    modeOld = pioeDef->IOE_modeUMask;
    pioeDef->IOE_modeUMask = modeMask & 0777;                           /*  ֻ�ܲ��� rwxrwxrwx λ       */
    __KERNEL_EXIT();
    
    return  (modeOld);
}
/*********************************************************************************************************
** ��������: fisbusy
** ��������: ��ȡָ���ļ��Ƿ񱻴�
** �䡡��  : pcName        �ļ�·��
**           bBusy         �ļ��Ƿ񱻴�
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  fisbusy (CPCHAR  pcName, BOOL  *bBusy)
{
    PLW_DEV_HDR    pdevhdrHdr;
    size_t         stDevNameLen;
    CHAR           cRealName[MAX_FILENAME_LENGTH];
    CHAR           cFullFileName[MAX_FILENAME_LENGTH];

    if (bBusy == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }

    if (pcName[0] == PX_EOS) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }

    if (ioFullFileNameGet(pcName, &pdevhdrHdr, cFullFileName) != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (cFullFileName[0] == PX_EOS) {
        lib_strcpy(cRealName, pdevhdrHdr->DEVHDR_pcName);

    } else {
        stDevNameLen = lib_strlen(pdevhdrHdr->DEVHDR_pcName);
        if (stDevNameLen == 1) {                                        /*  ��Ŀ¼�豸 "/"              */
            lib_strcpy(cRealName, cFullFileName);                       /*  �������·���ļ���          */

        } else {
            lib_strcpy(cRealName, pdevhdrHdr->DEVHDR_pcName);
            lib_strcpy(&cRealName[stDevNameLen], cFullFileName);
        }
    }

    *bBusy = iosFdIsBusy(cRealName);                                    /*  �鿴�ļ��Ƿ񱻴�          */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
