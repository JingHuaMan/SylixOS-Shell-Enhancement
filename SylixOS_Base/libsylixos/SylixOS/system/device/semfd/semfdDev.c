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
** ��   ��   ��: semfdDev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2019 �� 02 �� 23 ��
**
** ��        ��: �ź����豸ʵ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SEMFD_EN > 0)
#include "sys/semfd.h"
#include "fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  ��������ȫ�ֱ���
*********************************************************************************************************/
static INT              _G_iSemfdDrvNum = PX_ERROR;
static LW_SEMFD_DEV     _G_semfddev;
static LW_OBJECT_HANDLE _G_hSemfdSelMutex;
/*********************************************************************************************************
  hash
*********************************************************************************************************/
extern INT      __hashHorner(CPCHAR  pcKeyword, INT  iTableSize);
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LONG     _semfdOpen(PLW_SEMFD_DEV    psemfddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _semfdRemove(PLW_SEMFD_DEV  psemfddev, PCHAR  pcName);
static INT      _semfdClose(PLW_SEMFD_FILE  psemfdfil);
static ssize_t  _semfdRead(PLW_SEMFD_FILE   psemfdfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static ssize_t  _semfdWrite(PLW_SEMFD_FILE  psemfdfil, PCHAR  pcBuffer, size_t  stNBytes);
static INT      _semfdIoctl(PLW_SEMFD_FILE  psemfdfil, INT    iRequest, LONG  lArg);
/*********************************************************************************************************
  �豸�������
*********************************************************************************************************/
#define SEMFD_DEV_LOCK()    API_SemaphoreMPend(_G_semfddev.SEMFD_ulMutex, LW_OPTION_WAIT_INFINITE)
#define SEMFD_DEV_UNLOCK()  API_SemaphoreMPost(_G_semfddev.SEMFD_ulMutex)
/*********************************************************************************************************
** ��������: API_SemfdDrvInstall
** ��������: ��װ semfd ��������
** �䡡��  : NONE
** �䡡��  : �����Ƿ�װ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_SemfdDrvInstall (VOID)
{
    if (_G_iSemfdDrvNum <= 0) {
        _G_iSemfdDrvNum  = iosDrvInstall(_semfdOpen,
                                         _semfdRemove,
                                         _semfdOpen,
                                         _semfdClose,
                                         _semfdRead,
                                         _semfdWrite,
                                         _semfdIoctl);
        DRIVER_LICENSE(_G_iSemfdDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iSemfdDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iSemfdDrvNum, "semaphore file driver.");
    }

    if (_G_hSemfdSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hSemfdSelMutex =  API_SemaphoreMCreate("semfdsel_lock", LW_PRIO_DEF_CEILING,
                                                  LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                  LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                  LW_NULL);
    }

    return  ((_G_iSemfdDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_SemfdDevCreate
** ��������: ��װ semfd �豸
** �䡡��  : NONE
** �䡡��  : �豸�Ƿ񴴽��ɹ�
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_SemfdDevCreate (VOID)
{
    if (_G_iSemfdDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }

    _G_semfddev.SEMFD_plineFile = LW_NULL;
    _G_semfddev.SEMFD_ulMutex   = API_SemaphoreMCreate("semfd_lock", LW_PRIO_DEF_CEILING,
                                                       LW_OPTION_WAIT_PRIORITY |
                                                       LW_OPTION_DELETE_SAFE |
                                                       LW_OPTION_INHERIT_PRIORITY |
                                                       LW_OPTION_OBJECT_GLOBAL,
                                                       LW_NULL);
    if (_G_semfddev.SEMFD_ulMutex == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    if (iosDevAddEx(&_G_semfddev.SEMFD_devhdrHdr, LW_SEMFD_DEV_PATH,
                    _G_iSemfdDrvNum, DT_DIR) != ERROR_NONE) {
        API_SemaphoreMDelete(&_G_semfddev.SEMFD_ulMutex);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _semfdCheckFileName
** ��������: ����ļ����Ƿ�Ϸ�
** �䡡��  : pcName        �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdCheckFileName (CPCHAR  pcName)
{
    if (lib_index(pcName, PX_DIVIDER)) {
        return  (PX_ERROR);
    }

    return  (__fsCheckFileName(pcName));
}
/*********************************************************************************************************
** ��������: _semfdFindInode
** ��������: Ѱ�� semfd �ڵ�
** �䡡��  : pcName        �ļ���
** �䡡��  : semfd �ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_SEMFD_INODE  _semfdFindInode (CPCHAR  pcName)
{
    INT              iKey;
    PLW_LIST_LINE    plineTemp;
    PLW_SEMFD_INODE  psemfdinode;

    iKey = __hashHorner(pcName, LW_SEMFD_DEV_HASH);
    for (plineTemp  = _G_semfddev.SEMFD_plineInode[iKey];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        psemfdinode = _LIST_ENTRY(plineTemp, LW_SEMFD_INODE, SEMFDI_lineManage);
        if (!lib_strcmp(psemfdinode->SEMFDI_cName, pcName)) {
            return  (psemfdinode);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: _semfdCreateInode
** ��������: ����һ���յ� semfd �ڵ�
** �䡡��  : pcName        �ļ���
**           mode          mode_t
** �䡡��  : semfd �ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_SEMFD_INODE  _semfdCreateInode (CPCHAR  pcName, mode_t  mode)
{
    PLW_SEMFD_INODE  psemfdinode;

    psemfdinode = (PLW_SEMFD_INODE)__SHEAP_ALLOC(sizeof(LW_SEMFD_INODE) + lib_strlen(pcName));
    if (!psemfdinode) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    lib_bzero(psemfdinode, sizeof(LW_SEMFD_INODE));
    lib_strcpy(psemfdinode->SEMFDI_cName, pcName);

    psemfdinode->SEMFDI_selwulist.SELWUL_hListLock = _G_hSemfdSelMutex;

    psemfdinode->SEMFDI_mode = mode;
    psemfdinode->SEMFDI_time = lib_time(LW_NULL);
    psemfdinode->SEMFDI_uid  = getuid();
    psemfdinode->SEMFDI_gid  = getgid();

    return  (psemfdinode);
}
/*********************************************************************************************************
** ��������: _semfdDeleteInode
** ��������: ɾ�� semfd �ڵ�
** �䡡��  : psemfdinode      semfd �ڵ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  _semfdDeleteInode (PLW_SEMFD_INODE  psemfdinode)
{
    if (psemfdinode->SEMFDI_ulSem) {
        API_SemaphoreDelete(&psemfdinode->SEMFDI_ulSem);
    }

    SEL_WAKE_UP_TERM(&psemfdinode->SEMFDI_selwulist);

    __SHEAP_FREE(psemfdinode);
}
/*********************************************************************************************************
** ��������: _semfdSetInode
** ��������: ���� semfd �ڵ�
** �䡡��  : psemfdinode      semfd �ڵ�
**           param            �ź�������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdSetInode (PLW_SEMFD_INODE  psemfdinode, struct semfd_param *param)
{
    ULONG   ulOpt = LW_OPTION_SIGNAL_INTER | LW_OPTION_OBJECT_GLOBAL | LW_OPTION_INHERIT_PRIORITY;

    if (psemfdinode->SEMFDI_ulSem) {
        _ErrorHandle(EALREADY);
        return  (PX_ERROR);
    }

    if ((param->sem_value > INT32_MAX) ||
        (param->sem_max > INT32_MAX)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ulOpt |= param->sem_opts;

    switch (param->sem_type) {

    case SEMFD_TYPE_BINARY:
        psemfdinode->SEMFDI_ulSem = API_SemaphoreBCreate("semfd",
                                                         (param->sem_value ? LW_TRUE : LW_FALSE),
                                                         ulOpt, LW_NULL);
        break;

    case SEMFD_TYPE_COUNTING:
        psemfdinode->SEMFDI_ulSem = API_SemaphoreCCreate("semfd",
                                                         param->sem_value, param->sem_max,
                                                         ulOpt, LW_NULL);
        break;

    case SEMFD_TYPE_MUTEX:
        psemfdinode->SEMFDI_ulSem = API_SemaphoreMCreate("mutex",
                                                         LW_PRIO_DEF_CEILING,
                                                         ulOpt, LW_NULL);
        break;

    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!psemfdinode->SEMFDI_ulSem) {
        return  (PX_ERROR);
    }

    psemfdinode->SEMFDI_iAutoUnlink = param->auto_unlink;
    psemfdinode->SEMFDI_ulOption    = ulOpt;

    SEL_WAKE_UP_ALL(&psemfdinode->SEMFDI_selwulist, SELWRITE);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bsemfdOpen
** ��������: �� semfd �豸
** �䡡��  : psemfddev        semfd �豸
**           pcName           ����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  _semfdOpen (PLW_SEMFD_DEV  psemfddev,
                         PCHAR          pcName,
                         INT            iFlags,
                         INT            iMode)
{
    BOOL            bNewInode = LW_FALSE;
    INT             iKey;
    PLW_SEMFD_FILE  psemfdfil;
    PLW_SEMFD_INODE psemfdinode;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (*pcName == PX_DIVIDER) {                                        /*  ȥ���ײ�Ŀ¼�ָ���          */
        pcName++;
    }

    if (iFlags & O_CREAT) {                                             /*  ��������                    */
        if (_semfdCheckFileName(pcName)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        if (!S_ISCHR(iMode)) {
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  ������ FIFO                 */
            return  (PX_ERROR);
        }
    }

    if (*pcName == PX_EOS) {                                            /*  ���豸                    */
        if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {
            _ErrorHandle(EEXIST);
            return  (PX_ERROR);
        }

        psemfdfil = (PLW_SEMFD_FILE)__SHEAP_ALLOC(sizeof(LW_SEMFD_FILE));
        if (!psemfdfil) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        psemfdfil->SEMFDF_iFlag  = iFlags;
        psemfdfil->SEMFDF_pinode = LW_NULL;

        SEMFD_DEV_LOCK();
        _List_Line_Add_Ahead(&psemfdfil->SEMFDF_lineManage, &_G_semfddev.SEMFD_plineFile);
        SEMFD_DEV_UNLOCK();

    } else {                                                            /*  ��һ���ļ�                */
        if (iFlags & O_DIRECTORY) {
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }

        SEMFD_DEV_LOCK();
        psemfdinode = _semfdFindInode(pcName);
        if (psemfdinode) {
            if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {              /*  �����Դ���                  */
                SEMFD_DEV_UNLOCK();
                _ErrorHandle(EEXIST);
                return  (PX_ERROR);
            }

            if (_IosCheckPermissions(iFlags, LW_FALSE,
                                     psemfdinode->SEMFDI_mode,
                                     psemfdinode->SEMFDI_uid,
                                     psemfdinode->SEMFDI_gid) < ERROR_NONE) {
                SEMFD_DEV_UNLOCK();
                _ErrorHandle(EACCES);                                   /*  û��Ȩ��                    */
                return  (PX_ERROR);
            }

        } else {                                                        /*  û���ҵ��ڵ�                */
            if (iFlags & O_CREAT) {
                psemfdinode = _semfdCreateInode(pcName, iMode);         /*  ����һ���µ� inode          */
                if (!psemfdinode) {
                    SEMFD_DEV_UNLOCK();
                    return  (PX_ERROR);
                }
                bNewInode = LW_TRUE;

            } else {
                SEMFD_DEV_UNLOCK();
                _ErrorHandle(ENOENT);
                return  (PX_ERROR);
            }
        }

        psemfdfil = (PLW_SEMFD_FILE)__SHEAP_ALLOC(sizeof(LW_SEMFD_FILE));
        if (!psemfdfil) {
            if (bNewInode) {
                _semfdDeleteInode(psemfdinode);                         /*  ɾ���´����Ľڵ�            */
            }
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        psemfdfil->SEMFDF_iFlag  = iFlags;
        psemfdfil->SEMFDF_pinode = psemfdinode;

        _List_Line_Add_Ahead(&psemfdfil->SEMFDF_lineManage, &_G_semfddev.SEMFD_plineFile);
        if (bNewInode) {
            psemfdinode->SEMFDI_iOpenNum = 1;
            iKey = __hashHorner(pcName, LW_SEMFD_DEV_HASH);
            _List_Line_Add_Ahead(&psemfdinode->SEMFDI_lineManage, &_G_semfddev.SEMFD_plineInode[iKey]);

        } else {
            psemfdinode->SEMFDI_iOpenNum++;
        }
        SEMFD_DEV_UNLOCK();
    }

    return  ((LONG)psemfdfil);
}
/*********************************************************************************************************
** ��������: _semfdRemove
** ��������: ɾ�� inode �ļ�
** �䡡��  : psemfddev        semfd �豸
**           pcName           inode ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdRemove (PLW_SEMFD_DEV  psemfddev, PCHAR  pcName)
{
    PLW_SEMFD_INODE psemfdinode;
    INT             iKey;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (*pcName == PX_DIVIDER) {                                        /*  ȥ���ײ�Ŀ¼�ָ���          */
        pcName++;
    }

    SEMFD_DEV_LOCK();
    psemfdinode = _semfdFindInode(pcName);
    if (psemfdinode) {
        if (_IosCheckPermissions(O_WRONLY, LW_FALSE,
                                 psemfdinode->SEMFDI_mode,
                                 psemfdinode->SEMFDI_uid,
                                 psemfdinode->SEMFDI_gid) < ERROR_NONE) {
            SEMFD_DEV_UNLOCK();
            _ErrorHandle(EACCES);                                       /*  û��Ȩ��                    */
            return  (PX_ERROR);
        }

        if (psemfdinode->SEMFDI_iOpenNum) {
            SEMFD_DEV_UNLOCK();
            _ErrorHandle(EBUSY);                                        /*  �ļ����ڲ���                */
            return  (PX_ERROR);
        }

        iKey = __hashHorner(psemfdinode->SEMFDI_cName, LW_SEMFD_DEV_HASH);
        _List_Line_Del(&psemfdinode->SEMFDI_lineManage, &_G_semfddev.SEMFD_plineInode[iKey]);
        _semfdDeleteInode(psemfdinode);

    } else {                                                            /*  δ�ҵ�                      */
        SEMFD_DEV_UNLOCK();
        _ErrorHandle(ENOENT);                                           /*  �ļ����ڲ���                */
        return  (PX_ERROR);
    }
    SEMFD_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _semfdClose
** ��������: �ر� semfd �ļ�
** �䡡��  : psemfdfil        semfd �ļ�
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdClose (PLW_SEMFD_FILE  psemfdfil)
{
    PLW_SEMFD_INODE psemfdinode;
    INT             iKey;

    SEMFD_DEV_LOCK();
    if (psemfdfil->SEMFDF_pinode) {
        psemfdinode = psemfdfil->SEMFDF_pinode;
        psemfdinode->SEMFDI_iOpenNum--;
        if (!psemfdinode->SEMFDI_iOpenNum && psemfdinode->SEMFDI_iAutoUnlink) {
            iKey = __hashHorner(psemfdinode->SEMFDI_cName, LW_SEMFD_DEV_HASH);
            _List_Line_Del(&psemfdinode->SEMFDI_lineManage, &_G_semfddev.SEMFD_plineInode[iKey]);
            _semfdDeleteInode(psemfdinode);
        }
    }
    _List_Line_Del(&psemfdfil->SEMFDF_lineManage, &_G_semfddev.SEMFD_plineFile);
    SEMFD_DEV_UNLOCK();

    __SHEAP_FREE(psemfdfil);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _semfdRead
** ��������: �� semfd �豸
** �䡡��  : psemfdfil        semfd �ļ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : ��ȡ�ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  _semfdRead (PLW_SEMFD_FILE  psemfdfil,
                            PCHAR           pcBuffer,
                            size_t          stMaxBytes)
{
    PLW_SEMFD_INODE  psemfdinode;
    ULONG            ulTimeout;
    ULONG            ulError;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!stMaxBytes) {
        return  (0);
    }

    if (psemfdfil->SEMFDF_iFlag & O_NONBLOCK) {                         /*  ������ IO                   */
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = psemfdfil->SEMFDF_ulTimeout;
    }

    if (!psemfdfil->SEMFDF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    psemfdinode = psemfdfil->SEMFDF_pinode;
    if (!psemfdinode->SEMFDI_ulSem) {                                   /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    ulError = API_SemaphorePend(psemfdinode->SEMFDI_ulSem, ulTimeout);  /*  �ȴ��ź���                  */
    if (ulError) {
        if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
            _ErrorHandle(EAGAIN);
        }
        return  (0);
    }

    if (psemfdinode->SEMFDI_ulOption & LW_OPTION_WAIT_PRIORITY) {
        SEL_WAKE_UP_PRIO(&psemfdinode->SEMFDI_selwulist, SELWRITE);     /*  ��д                        */
    } else {
        SEL_WAKE_UP_FIFO(&psemfdinode->SEMFDI_selwulist, SELWRITE);
    }

    *pcBuffer = 1;

    return  (1);
}
/*********************************************************************************************************
** ��������: _semfdWrite
** ��������: д semfd �豸
** �䡡��  : psemfdfil        semfd �ļ�
**           pcBuffer         ��Ҫд�������ָ��
**           stNBytes         д�����ݴ�С
** �䡡��  : д���ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  _semfdWrite (PLW_SEMFD_FILE  psemfdfil,
                             PCHAR           pcBuffer,
                             size_t          stNBytes)
{
    PLW_SEMFD_INODE  psemfdinode;
    ULONG            ulError;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!stNBytes) {
        return  (0);
    }

    if (!psemfdfil->SEMFDF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    psemfdinode = psemfdfil->SEMFDF_pinode;
    if (!psemfdinode->SEMFDI_ulSem) {                                   /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    ulError = API_SemaphorePost(psemfdinode->SEMFDI_ulSem);
    if (ulError) {
        if (ulError == ERROR_EVENT_FULL) {
            _ErrorHandle(EFBIG);
        }
        return  (0);
    }

    if (psemfdinode->SEMFDI_ulOption & LW_OPTION_WAIT_PRIORITY) {
        SEL_WAKE_UP_PRIO(&psemfdinode->SEMFDI_selwulist, SELREAD);      /*  �ɶ�                        */
    } else {
        SEL_WAKE_UP_FIFO(&psemfdinode->SEMFDI_selwulist, SELREAD);
    }

    return  (1);
}
/*********************************************************************************************************
** ��������: _semfdReadDir
** ��������: semfd �豸���ָ��Ŀ¼��Ϣ
** �䡡��  : psemfdfil           semfd �ļ�
**           dir                 Ŀ¼�ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdReadDir (PLW_SEMFD_FILE  psemfdfil, DIR  *dir)
{
    INT              i, iCnt;
    INT              iError = ERROR_NONE;
    LONG             iStart;
    PLW_LIST_LINE    plineTemp;
    PLW_SEMFD_INODE  psemfdinode;

    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (psemfdfil->SEMFDF_pinode) {
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }

    iStart = dir->dir_pos;

    SEMFD_DEV_LOCK();
    iCnt = 0;
    for (i = 0; i < LW_SEMFD_DEV_HASH; i++) {
        for (plineTemp  = _G_semfddev.SEMFD_plineInode[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            if (iCnt >= iStart) {
                goto    __out;
            }
            iCnt++;
        }
    }

__out:
    if (plineTemp == LW_NULL) {
        _ErrorHandle(ENOENT);
        iError = PX_ERROR;                                              /*  û�ж���Ľڵ�              */

    } else {
        psemfdinode = _LIST_ENTRY(plineTemp, LW_SEMFD_INODE, SEMFDI_lineManage);
        dir->dir_pos++;

        lib_strlcpy(dir->dir_dirent.d_name,
                    psemfdinode->SEMFDI_cName,
                    sizeof(dir->dir_dirent.d_name));

        dir->dir_dirent.d_type = DT_CHR;
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }
    SEMFD_DEV_UNLOCK();

    return  (iError);
}
/*********************************************************************************************************
** ��������: _semfdNFreeFnode
** ��������: semfd ����ļ��п����ֽ���
** �䡡��  : psemfdfil           semfd �ļ�
**           piNFree             �����ֽ���
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdNFreeFnode (PLW_SEMFD_FILE  psemfdfil, INT  *piNFree)
{
    BOOL    bValue     = LW_FALSE;
    ULONG   ulValue    = 0;
    ULONG   ulMaxValue = 0;

    if (!piNFree) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!psemfdfil->SEMFDF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (!psemfdfil->SEMFDF_pinode->SEMFDI_ulSem) {                      /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    *piNFree = 0;

    switch (_ObjectGetClass(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem)) {

    case _OBJECT_SEM_B:
        API_SemaphoreBStatus(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem, &bValue, LW_NULL, LW_NULL);
        if (!bValue) {
            *piNFree = 1;
        }
        break;

    case _OBJECT_SEM_C:
        API_SemaphoreCStatusEx(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem, &ulValue, LW_NULL, LW_NULL, &ulMaxValue);
        if (ulValue < ulMaxValue) {
            *piNFree = (INT)(ulMaxValue - ulValue);
        }
        break;

    case _OBJECT_SEM_M:
        API_SemaphoreMStatus(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem, &bValue, LW_NULL, LW_NULL);
        if (!bValue) {
            *piNFree = 1;
        }
        break;

    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bsemfdNReadFnode
** ��������: semfd ����ļ���ʣ��������ֽ���
** �䡡��  : psemfdfil           semfd �ļ�
**           piNRead             �����ֽ���
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdNReadFnode (PLW_SEMFD_FILE  psemfdfil, INT  *piNRead)
{
    BOOL    bValue  = LW_FALSE;
    ULONG   ulValue = 0;

    if (!piNRead) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!psemfdfil->SEMFDF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (!psemfdfil->SEMFDF_pinode->SEMFDI_ulSem) {                      /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    *piNRead = 0;

    switch (_ObjectGetClass(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem)) {

    case _OBJECT_SEM_B:
        API_SemaphoreBStatus(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem, &bValue, LW_NULL, LW_NULL);
        if (bValue) {
            *piNRead = 1;
        }
        break;

    case _OBJECT_SEM_C:
        API_SemaphoreCStatus(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem, &ulValue, LW_NULL, LW_NULL);
        if (ulValue) {
            *piNRead = (INT)ulValue;
        }
        break;

    case _OBJECT_SEM_M:
        API_SemaphoreMStatus(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem, &bValue, LW_NULL, LW_NULL);
        if (bValue) {
            *piNRead = 1;
        }
        break;

    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _semfdFlushFnode
** ��������: semfd ����ļ�����
** �䡡��  : psemfdfil           semfd �ļ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdFlushFnode (PLW_SEMFD_FILE  psemfdfil)
{
    if (!psemfdfil->SEMFDF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (!psemfdfil->SEMFDF_pinode->SEMFDI_ulSem) {                      /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    switch (_ObjectGetClass(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem)) {

    case _OBJECT_SEM_B:
        API_SemaphoreBClear(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem);
        break;

    case _OBJECT_SEM_C:
        API_SemaphoreCClear(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem);
        break;

    case _OBJECT_SEM_M:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);

    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _semfdSetFnode
** ��������: semfd �����ļ�����
** �䡡��  : psemfdfil           semfd �ļ�
**           param               �ź�������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdSetFnode (PLW_SEMFD_FILE  psemfdfil, struct semfd_param *param)
{
    INT  iRet;

    if (!param) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!psemfdfil->SEMFDF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    SEMFD_DEV_LOCK();
    iRet = _semfdSetInode(psemfdfil->SEMFDF_pinode, param);
    SEMFD_DEV_UNLOCK();

    return  (iRet);
}
/*********************************************************************************************************
** ��������: _semfdGetFnode
** ��������: semfd ��ȡ�ļ�����
** �䡡��  : psemfdfil           semfd �ļ�
**           param               �ź�������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdGetFnode (PLW_SEMFD_FILE  psemfdfil, struct semfd_param *param)
{
    BOOL    bValue     = LW_FALSE;
    ULONG   ulValue    = 0;
    ULONG   ulMaxValue = 0;
    ULONG   ulOpt      = 0;

    if (!param) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!psemfdfil->SEMFDF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (!psemfdfil->SEMFDF_pinode->SEMFDI_ulSem) {                      /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    switch (_ObjectGetClass(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem)) {

    case _OBJECT_SEM_B:
        API_SemaphoreBStatus(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem, &bValue, &ulOpt, LW_NULL);
        param->sem_type  = SEMFD_TYPE_BINARY;
        param->sem_opts  = ulOpt;
        param->sem_value = (UINT32)bValue;
        param->sem_max   = 1;
        break;

    case _OBJECT_SEM_C:
        API_SemaphoreCStatusEx(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem, &ulValue, &ulOpt, LW_NULL, &ulMaxValue);
        param->sem_type  = SEMFD_TYPE_COUNTING;
        param->sem_opts  = ulOpt;
        param->sem_value = (UINT32)ulValue;
        param->sem_max   = (UINT32)ulMaxValue;
        break;

    case _OBJECT_SEM_M:
        API_SemaphoreMStatus(psemfdfil->SEMFDF_pinode->SEMFDI_ulSem, &bValue, &ulOpt, LW_NULL);
        param->sem_type  = SEMFD_TYPE_MUTEX;
        param->sem_opts  = ulOpt;
        param->sem_value = (UINT32)bValue;
        param->sem_max   = 1;
        break;

    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _semfdStat
** ��������: semfd ��ȡ�ļ�����
** �䡡��  : psemfdfil           semfd �ļ�
**           pstat               �ļ�����
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdStat (PLW_SEMFD_FILE  psemfdfil, struct stat *pstat)
{
    PLW_SEMFD_INODE   psemfdinode;
    INT               iNRead = 0;

    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (psemfdfil->SEMFDF_pinode) {
        psemfdinode       = psemfdfil->SEMFDF_pinode;
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_semfddev.SEMFD_devhdrHdr);
        pstat->st_ino     = (ino_t)psemfdinode;
        pstat->st_mode    = psemfdinode->SEMFDI_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = psemfdinode->SEMFDI_uid;
        pstat->st_gid     = psemfdinode->SEMFDI_gid;
        pstat->st_rdev    = 1;
        pstat->st_atime   = psemfdinode->SEMFDI_time;
        pstat->st_mtime   = psemfdinode->SEMFDI_time;
        pstat->st_ctime   = psemfdinode->SEMFDI_time;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;

        _semfdNReadFnode(psemfdfil, &iNRead);
        pstat->st_size = (size_t)iNRead;

    } else {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_semfddev.SEMFD_devhdrHdr);
        pstat->st_ino     = (ino_t)0;
        pstat->st_mode    = S_IFDIR | DEFAULT_DIR_PERM;
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_atime   = API_RootFsTime(LW_NULL);
        pstat->st_mtime   = API_RootFsTime(LW_NULL);
        pstat->st_ctime   = API_RootFsTime(LW_NULL);
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _semfdStatfs
** ��������: semfd ��ȡ�ļ�ϵͳ����
** �䡡��  : psemfdfil           semfd �ļ�
**           pstatfs             �ļ�ϵͳ����
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdStatfs (PLW_SEMFD_FILE  psemfdfil, struct statfs *pstatfs)
{
    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pstatfs->f_type   = 0;
    pstatfs->f_bsize  = 512;
    pstatfs->f_blocks = 0;
    pstatfs->f_bfree  = 0;
    pstatfs->f_bavail = 1;

    pstatfs->f_files  = 0;
    pstatfs->f_ffree  = 0;

#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatfs->f_fsid.val[0] = (int32_t)((addr_t)&_G_semfddev >> 32);
    pstatfs->f_fsid.val[1] = (int32_t)((addr_t)&_G_semfddev & 0xffffffff);
#else
    pstatfs->f_fsid.val[0] = (int32_t)&_G_semfddev;
    pstatfs->f_fsid.val[1] = 0;
#endif

    pstatfs->f_flag    = 0;
    pstatfs->f_namelen = PATH_MAX;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _semfdSelect
** ��������: semfd ����ļ�״̬
** �䡡��  : psemfdfil           semfd �ļ�
**           pselwunNode         select �ڵ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdSelect (PLW_SEMFD_FILE  psemfdfil, PLW_SEL_WAKEUPNODE  pselwunNode)
{
    PLW_SEMFD_INODE     psemfdinode;
    struct semfd_param  param;

    if (!pselwunNode) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!psemfdfil->SEMFDF_pinode) {
        if (pselwunNode->SELWUN_seltypType != SELEXCEPT) {
            SEL_WAKE_UP(pselwunNode);
        }
        return  (ERROR_NONE);
    }

    if (!psemfdfil->SEMFDF_pinode->SEMFDI_ulSem) {                      /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    psemfdinode = psemfdfil->SEMFDF_pinode;
    SEL_WAKE_NODE_ADD(&psemfdinode->SEMFDI_selwulist, pselwunNode);

    _semfdGetFnode(psemfdfil, &param);                                  /*  ��ȡ״̬                    */

    switch (pselwunNode->SELWUN_seltypType) {

    case SELREAD:
        if (param.sem_value) {
            SEL_WAKE_UP(pselwunNode);
        }
        break;

    case SELWRITE:
        if (param.sem_value < param.sem_max) {
            SEL_WAKE_UP(pselwunNode);
        }
        break;

    case SELEXCEPT:
        break;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _semfdUnselect
** ��������: semfd ȡ������ļ�״̬
** �䡡��  : psemfdfil           semfd �ļ�
**           pselwunNode         select �ڵ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdUnselect (PLW_SEMFD_FILE  psemfdfil, PLW_SEL_WAKEUPNODE  pselwunNode)
{
    PLW_SEMFD_INODE  psemfdinode;

    if (!pselwunNode) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!psemfdfil->SEMFDF_pinode) {
        if (pselwunNode->SELWUN_seltypType != SELEXCEPT) {
            LW_SELWUN_SET_READY(pselwunNode);
        }
        return  (ERROR_NONE);
    }

    psemfdinode = psemfdfil->SEMFDF_pinode;
    SEL_WAKE_NODE_DELETE(&psemfdinode->SEMFDI_selwulist, pselwunNode);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _semfdIoctl
** ��������: ���� semfd �ļ�
** �䡡��  : psemfdfil        semfd �ļ�
**           iRequest         ����
**           lArg             ����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _semfdIoctl (PLW_SEMFD_FILE  psemfdfil,
                         INT             iRequest,
                         LONG            lArg)
{
    INT  iRet = ERROR_NONE;

    switch (iRequest) {

    case FIONBIO:
        if (*(INT *)lArg) {
            psemfdfil->SEMFDF_iFlag |= O_NONBLOCK;
        } else {
            psemfdfil->SEMFDF_iFlag &= ~O_NONBLOCK;
        }
        break;

    case FIONFREE:
        return  (_semfdNFreeFnode(psemfdfil, (INT *)lArg));

    case FIONREAD:
        return  (_semfdNReadFnode(psemfdfil, (INT *)lArg));

    case FIOFLUSH:
        return  (_semfdFlushFnode(psemfdfil));

    case FIOSEMFDGET:
        return  (_semfdGetFnode(psemfdfil, (struct semfd_param *)lArg));

    case FIOSEMFDSET:
        return  (_semfdSetFnode(psemfdfil, (struct semfd_param *)lArg));

    case FIOFSTATGET:
        return  (_semfdStat(psemfdfil, (struct stat *)lArg));

    case FIOFSTATFSGET:
        return  (_semfdStatfs(psemfdfil, (struct statfs *)lArg));

    case FIOREADDIR:
        return  (_semfdReadDir(psemfdfil, (DIR *)lArg));

    case FIOSELECT:
        return  (_semfdSelect(psemfdfil, (PLW_SEL_WAKEUPNODE)lArg));

    case FIOUNSELECT:
        return  (_semfdUnselect(psemfdfil, (PLW_SEL_WAKEUPNODE)lArg));

    case FIORTIMEOUT:                                                   /*  ���ö���ʱʱ��              */
        {
            struct timeval *ptvTimeout = (struct timeval *)lArg;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  ��� tick ����              */
                psemfdfil->SEMFDF_ulTimeout = ulTick;
            } else {
                psemfdfil->SEMFDF_ulTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;

    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "Semaphore file IPC Service";
        return  (ERROR_NONE);

    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }

    return  (iRet);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SEMFD_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
