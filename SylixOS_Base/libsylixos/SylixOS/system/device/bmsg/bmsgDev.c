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
** ��   ��   ��: bmsgDev.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2019 �� 02 �� 02 ��
**
** ��        ��: �б߽���Ϣ�豸ʵ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_BMSG_EN > 0)
#include "sys/bmsgfd.h"
#include "fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  ��������ȫ�ֱ���
*********************************************************************************************************/
static INT              _G_iBmsgDrvNum = PX_ERROR;
static LW_BMSG_DEV      _G_bmsgdev;
static LW_OBJECT_HANDLE _G_hBmsgSelMutex;
/*********************************************************************************************************
  hash
*********************************************************************************************************/
extern INT      __hashHorner(CPCHAR  pcKeyword, INT  iTableSize);
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LONG     _bmsgOpen(PLW_BMSG_DEV    pbmsgdev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _bmsgRemove(PLW_BMSG_DEV  pbmsgdev, PCHAR  pcName);
static INT      _bmsgClose(PLW_BMSG_FILE  pbmsgfil);
static ssize_t  _bmsgRead(PLW_BMSG_FILE   pbmsgfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static ssize_t  _bmsgWrite(PLW_BMSG_FILE  pbmsgfil, PCHAR  pcBuffer, size_t  stNBytes);
static INT      _bmsgIoctl(PLW_BMSG_FILE  pbmsgfil, INT    iRequest, LONG  lArg);
/*********************************************************************************************************
  �豸�������
*********************************************************************************************************/
#define BMSG_DEV_LOCK()     API_SemaphoreMPend(_G_bmsgdev.BMSGD_ulMutex, LW_OPTION_WAIT_INFINITE)
#define BMSG_DEV_UNLOCK()   API_SemaphoreMPost(_G_bmsgdev.BMSGD_ulMutex)
/*********************************************************************************************************
  bmsg ��д������֪ͨ
*********************************************************************************************************/
#define BMSG_FILE_PEND_READ(f, to)  API_SemaphoreBPend((f)->BMSGF_pinode->BMSGI_ulReadLock, to)
#define BMSG_FILE_POST_READ(f)      API_SemaphoreBPost((f)->BMSGF_pinode->BMSGI_ulReadLock)
#define BMSG_FILE_PEND_WRITE(f, to) API_SemaphoreBPend((f)->BMSGF_pinode->BMSGI_ulWriteLock, to)
#define BMSG_FILE_POST_WRITE(f)     API_SemaphoreBPost((f)->BMSGF_pinode->BMSGI_ulWriteLock)
/*********************************************************************************************************
** ��������: API_BmsgDrvInstall
** ��������: ��װ bmsg ��Ϣ�豸��������
** �䡡��  : NONE
** �䡡��  : �����Ƿ�װ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_BmsgDrvInstall (VOID)
{
    if (_G_iBmsgDrvNum <= 0) {
        _G_iBmsgDrvNum  = iosDrvInstall(_bmsgOpen,
                                        _bmsgRemove,
                                        _bmsgOpen,
                                        _bmsgClose,
                                        _bmsgRead,
                                        _bmsgWrite,
                                        _bmsgIoctl);
        DRIVER_LICENSE(_G_iBmsgDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iBmsgDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iBmsgDrvNum, "block message driver.");
    }

    if (_G_hBmsgSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hBmsgSelMutex =  API_SemaphoreMCreate("bmsgsel_lock", LW_PRIO_DEF_CEILING,
                                                 LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                 LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                 LW_NULL);
    }

    return  ((_G_iBmsgDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** ��������: API_BmsgDevCreate
** ��������: ��װ bmsg ��Ϣ�豸
** �䡡��  : NONE
** �䡡��  : �豸�Ƿ񴴽��ɹ�
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_BmsgDevCreate (VOID)
{
    if (_G_iBmsgDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }

    _G_bmsgdev.BMSGD_plineFile = LW_NULL;
    _G_bmsgdev.BMSGD_ulMutex   = API_SemaphoreMCreate("bmsgd_lock", LW_PRIO_DEF_CEILING,
                                                      LW_OPTION_WAIT_PRIORITY |
                                                      LW_OPTION_DELETE_SAFE |
                                                      LW_OPTION_INHERIT_PRIORITY |
                                                      LW_OPTION_OBJECT_GLOBAL,
                                                      LW_NULL);
    if (_G_bmsgdev.BMSGD_ulMutex == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    if (iosDevAddEx(&_G_bmsgdev.BMSGD_devhdrHdr, LW_BMSG_DEV_PATH,
                    _G_iBmsgDrvNum, DT_DIR) != ERROR_NONE) {
        API_SemaphoreMDelete(&_G_bmsgdev.BMSGD_ulMutex);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgCheckFileName
** ��������: ����ļ����Ƿ�Ϸ�
** �䡡��  : pcName        �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgCheckFileName (CPCHAR  pcName)
{
    if (lib_index(pcName, PX_DIVIDER)) {
        return  (PX_ERROR);
    }

    return  (__fsCheckFileName(pcName));
}
/*********************************************************************************************************
** ��������: _bmsgFindInode
** ��������: Ѱ�� bmsg �ڵ�
** �䡡��  : pcName        �ļ���
** �䡡��  : bmsg �ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_BMSG_INODE  _bmsgFindInode (CPCHAR  pcName)
{
    INT             iKey;
    PLW_LIST_LINE   plineTemp;
    PLW_BMSG_INODE  pbmsginode;

    iKey = __hashHorner(pcName, LW_BMSG_DEV_HASH);
    for (plineTemp  = _G_bmsgdev.BMSGD_plineInode[iKey];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pbmsginode = _LIST_ENTRY(plineTemp, LW_BMSG_INODE, BMSGI_lineManage);
        if (!lib_strcmp(pbmsginode->BMSGI_cName, pcName)) {
            return  (pbmsginode);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: _bmsgCreateInode
** ��������: ����һ���յ� bmsg �ڵ�
** �䡡��  : pcName        �ļ���
**           mode          mode_t
** �䡡��  : bmsg �ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_BMSG_INODE  _bmsgCreateInode (CPCHAR  pcName, mode_t  mode)
{
    PLW_BMSG_INODE  pbmsginode;

    pbmsginode = (PLW_BMSG_INODE)__SHEAP_ALLOC(sizeof(LW_BMSG_INODE) + lib_strlen(pcName));
    if (!pbmsginode) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    lib_bzero(pbmsginode, sizeof(LW_BMSG_INODE));
    lib_strcpy(pbmsginode->BMSGI_cName, pcName);

    pbmsginode->BMSGI_selwulist.SELWUL_hListLock = _G_hBmsgSelMutex;

    pbmsginode->BMSGI_mode = mode;
    pbmsginode->BMSGI_time = lib_time(LW_NULL);
    pbmsginode->BMSGI_uid  = getuid();
    pbmsginode->BMSGI_gid  = getgid();

    pbmsginode->BMSGI_ulReadLock = API_SemaphoreBCreate("bmsg_rlock", LW_FALSE,
                                                        LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (!pbmsginode->BMSGI_ulReadLock) {
        __SHEAP_FREE(pbmsginode);
        return  (LW_NULL);
    }

    pbmsginode->BMSGI_ulWriteLock = API_SemaphoreBCreate("bmsg_wlock", LW_FALSE,
                                                         LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (!pbmsginode->BMSGI_ulReadLock) {
        API_SemaphoreBDelete(&pbmsginode->BMSGI_ulReadLock);
        __SHEAP_FREE(pbmsginode);
        return  (LW_NULL);
    }

    return  (pbmsginode);
}
/*********************************************************************************************************
** ��������: _bmsgDeleteInode
** ��������: ɾ�� bmsg �ڵ�
** �䡡��  : pbmsginode       bmsg �ڵ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  _bmsgDeleteInode (PLW_BMSG_INODE  pbmsginode)
{
    if (pbmsginode->BMSGI_pbmsg) {
        _bmsgDelete(pbmsginode->BMSGI_pbmsg);
    }

    API_SemaphoreBDelete(&pbmsginode->BMSGI_ulReadLock);
    API_SemaphoreBDelete(&pbmsginode->BMSGI_ulWriteLock);

    SEL_WAKE_UP_TERM(&pbmsginode->BMSGI_selwulist);

    __SHEAP_FREE(pbmsginode);
}
/*********************************************************************************************************
** ��������: _bmsgSetInode
** ��������: ���� bmsg �ڵ�
** �䡡��  : pbmsginode       bmsg �ڵ�
**           param            �������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgSetInode (PLW_BMSG_INODE  pbmsginode, struct bmsg_param *param)
{
    PLW_BMSG  bmsg;

    if ((param->total_size < 16) || !param->atomic_size ||
        (param->atomic_size > (param->total_size - 2))) {               /*  ���Ȼ�ռ�����ֽ�            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (param->atomic_size > UINT16_MAX) {
        _ErrorHandle(EMSGSIZE);
        return  (PX_ERROR);
    }

    if (pbmsginode->BMSGI_pbmsg &&
        !(param->param_flags & BMSGFD_PARAM_FLAG_FORCE)) {              /*  �Ѿ������ҷ�ǿ��            */
        _ErrorHandle(EALREADY);
        return  (PX_ERROR);
    }

    bmsg = _bmsgCreate(param->total_size);
    if (!bmsg) {
        return  (PX_ERROR);
    }

    if (pbmsginode->BMSGI_pbmsg) {
        _bmsgDelete(pbmsginode->BMSGI_pbmsg);
    }

    pbmsginode->BMSGI_pbmsg       = bmsg;
    pbmsginode->BMSGI_stAtSize    = param->atomic_size;
    pbmsginode->BMSGI_iAutoUnlink = param->auto_unlink;

    API_SemaphoreBClear(pbmsginode->BMSGI_ulReadLock);
    API_SemaphoreBPost(pbmsginode->BMSGI_ulWriteLock);
    SEL_WAKE_UP_ALL(&pbmsginode->BMSGI_selwulist, SELWRITE);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgFlushInode
** ��������: ��� bmsg �ڵ���Ϣ
** �䡡��  : pbmsginode       bmsg �ڵ�
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgFlushInode (PLW_BMSG_INODE  pbmsginode)
{
    PLW_BMSG  bmsg = pbmsginode->BMSGI_pbmsg;

    if (bmsg) {
        _bmsgFlush(bmsg);
        API_SemaphoreBClear(pbmsginode->BMSGI_ulReadLock);
        API_SemaphoreBPost(pbmsginode->BMSGI_ulWriteLock);
        SEL_WAKE_UP_ALL(&pbmsginode->BMSGI_selwulist, SELWRITE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgOpen
** ��������: �� bmsg �豸
** �䡡��  : pbmsgdev         bmsg �豸
**           pcName           ����
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  _bmsgOpen (PLW_BMSG_DEV  pmsgfddev,
                        PCHAR         pcName,
                        INT           iFlags,
                        INT           iMode)
{
    BOOL           bNewInode = LW_FALSE;
    INT            iKey;
    PLW_BMSG_FILE  pbmsgfil;
    PLW_BMSG_INODE pbmsginode;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (*pcName == PX_DIVIDER) {                                        /*  ȥ���ײ�Ŀ¼�ָ���          */
        pcName++;
    }

    if (iFlags & O_CREAT) {                                             /*  ��������                    */
        if (_bmsgCheckFileName(pcName)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        if (!S_ISFIFO(iMode)) {
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  ������ FIFO                 */
            return  (PX_ERROR);
        }
    }

    if (*pcName == PX_EOS) {                                            /*  ���豸                    */
        if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {
            _ErrorHandle(EEXIST);
            return  (PX_ERROR);
        }

        pbmsgfil = (PLW_BMSG_FILE)__SHEAP_ALLOC(sizeof(LW_BMSG_FILE));
        if (!pbmsgfil) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        pbmsgfil->BMSGF_iFlag      = iFlags;
        pbmsgfil->BMSGF_pinode     = LW_NULL;
        pbmsgfil->BMSGF_ulRTimeout = LW_OPTION_WAIT_INFINITE;
        pbmsgfil->BMSGF_ulWTimeout = LW_OPTION_WAIT_INFINITE;

        BMSG_DEV_LOCK();
        _List_Line_Add_Ahead(&pbmsgfil->BMSGF_lineManage, &_G_bmsgdev.BMSGD_plineFile);
        BMSG_DEV_UNLOCK();

    } else {                                                            /*  ��һ���ܵ�                */
        if (iFlags & O_DIRECTORY) {
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }

        BMSG_DEV_LOCK();
        pbmsginode = _bmsgFindInode(pcName);
        if (pbmsginode) {
            if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {              /*  �����Դ���                  */
                BMSG_DEV_UNLOCK();
                _ErrorHandle(EEXIST);
                return  (PX_ERROR);
            }

            if (_IosCheckPermissions(iFlags, LW_FALSE,
                                     pbmsginode->BMSGI_mode,
                                     pbmsginode->BMSGI_uid,
                                     pbmsginode->BMSGI_gid) < ERROR_NONE) {
                BMSG_DEV_UNLOCK();
                _ErrorHandle(EACCES);                                   /*  û��Ȩ��                    */
                return  (PX_ERROR);
            }

        } else {                                                        /*  û���ҵ��ڵ�                */
            if (iFlags & O_CREAT) {
                pbmsginode = _bmsgCreateInode(pcName, iMode);           /*  ����һ���µ� inode          */
                if (!pbmsginode) {
                    BMSG_DEV_UNLOCK();
                    return  (PX_ERROR);
                }
                bNewInode = LW_TRUE;

            } else {
                BMSG_DEV_UNLOCK();
                _ErrorHandle(ENOENT);
                return  (PX_ERROR);
            }
        }

        pbmsgfil = (PLW_BMSG_FILE)__SHEAP_ALLOC(sizeof(LW_BMSG_FILE));
        if (!pbmsgfil) {
            if (bNewInode) {
                _bmsgDeleteInode(pbmsginode);                           /*  ɾ���´����Ľڵ�            */
            }
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        pbmsgfil->BMSGF_iFlag      = iFlags;
        pbmsgfil->BMSGF_pinode     = pbmsginode;
        pbmsgfil->BMSGF_ulRTimeout = LW_OPTION_WAIT_INFINITE;
        pbmsgfil->BMSGF_ulWTimeout = LW_OPTION_WAIT_INFINITE;

        _List_Line_Add_Ahead(&pbmsgfil->BMSGF_lineManage, &_G_bmsgdev.BMSGD_plineFile);
        if (bNewInode) {
            pbmsginode->BMSGI_iOpenNum = 1;
            iKey = __hashHorner(pcName, LW_BMSG_DEV_HASH);
            _List_Line_Add_Ahead(&pbmsginode->BMSGI_lineManage, &_G_bmsgdev.BMSGD_plineInode[iKey]);

        } else {
            pbmsginode->BMSGI_iOpenNum++;
        }
        BMSG_DEV_UNLOCK();
    }

    return  ((LONG)pbmsgfil);
}
/*********************************************************************************************************
** ��������: _bmsgRemove
** ��������: ɾ�� inode �ļ�
** �䡡��  : pbmsgdev          bmsg �豸
**           pcName            ��Ҫɾ�����ļ�
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgRemove (PLW_BMSG_DEV  pbmsgdev, PCHAR  pcName)
{
    PLW_BMSG_INODE pbmsginode;
    INT            iKey;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (*pcName == PX_DIVIDER) {                                        /*  ȥ���ײ�Ŀ¼�ָ���          */
        pcName++;
    }

    BMSG_DEV_LOCK();
    pbmsginode = _bmsgFindInode(pcName);
    if (pbmsginode) {
        if (_IosCheckPermissions(O_WRONLY, LW_FALSE,
                                 pbmsginode->BMSGI_mode,
                                 pbmsginode->BMSGI_uid,
                                 pbmsginode->BMSGI_gid) < ERROR_NONE) {
            BMSG_DEV_UNLOCK();
            _ErrorHandle(EACCES);                                       /*  û��Ȩ��                    */
            return  (PX_ERROR);
        }

        if (pbmsginode->BMSGI_iOpenNum) {
            BMSG_DEV_UNLOCK();
            _ErrorHandle(EBUSY);                                        /*  �ļ����ڲ���                */
            return  (PX_ERROR);
        }

        iKey = __hashHorner(pbmsginode->BMSGI_cName, LW_BMSG_DEV_HASH);
        _List_Line_Del(&pbmsginode->BMSGI_lineManage, &_G_bmsgdev.BMSGD_plineInode[iKey]);
        _bmsgDeleteInode(pbmsginode);

    } else {                                                            /*  δ�ҵ�                      */
        BMSG_DEV_UNLOCK();
        _ErrorHandle(ENOENT);                                           /*  �ļ����ڲ���                */
        return  (PX_ERROR);
    }
    BMSG_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgClose
** ��������: �ر� bmsg �ļ�
** �䡡��  : pbmsgfil          bmsg �ļ�
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgClose (PLW_BMSG_FILE  pbmsgfil)
{
    PLW_BMSG_INODE pbmsginode;
    INT            iKey;

    BMSG_DEV_LOCK();
    if (pbmsgfil->BMSGF_pinode) {
        pbmsginode = pbmsgfil->BMSGF_pinode;
        pbmsginode->BMSGI_iOpenNum--;
        if (!pbmsginode->BMSGI_iOpenNum && pbmsginode->BMSGI_iAutoUnlink) {
            iKey = __hashHorner(pbmsginode->BMSGI_cName, LW_BMSG_DEV_HASH);
            _List_Line_Del(&pbmsginode->BMSGI_lineManage, &_G_bmsgdev.BMSGD_plineInode[iKey]);
            _bmsgDeleteInode(pbmsginode);
        }
    }
    _List_Line_Del(&pbmsgfil->BMSGF_lineManage, &_G_bmsgdev.BMSGD_plineFile);
    BMSG_DEV_UNLOCK();

    __SHEAP_FREE(pbmsgfil);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgRead
** ��������: �� bmsg �豸
** �䡡��  : pbmsgfil         bmsgfd �ļ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : ��ȡ�ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  _bmsgRead (PLW_BMSG_FILE   pbmsgfil,
                           PCHAR           pcBuffer,
                           size_t          stMaxBytes)
{
    PLW_BMSG_INODE  pbmsginode;
    ULONG           ulTimeout;
    size_t          stMsgLen;
    ssize_t         sstRet;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!stMaxBytes) {
        return  (0);
    }

    if (pbmsgfil->BMSGF_iFlag & O_NONBLOCK) {                           /*  ������ IO                   */
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = pbmsgfil->BMSGF_ulRTimeout;
    }

    if (!pbmsgfil->BMSGF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    pbmsginode = pbmsgfil->BMSGF_pinode;
    if (!pbmsginode->BMSGI_pbmsg) {                                     /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    for (;;) {
        if (BMSG_FILE_PEND_READ(pbmsgfil, ulTimeout)) {                 /*  �ȴ����ݿɶ�                */
            _ErrorHandle(EAGAIN);
            return  (0);
        }

        BMSG_DEV_LOCK();
        stMsgLen = (size_t)_bmsgNBytesNext(pbmsginode->BMSGI_pbmsg);
        if (stMsgLen > stMaxBytes) {
            BMSG_DEV_UNLOCK();
            BMSG_FILE_POST_READ(pbmsgfil);
            _ErrorHandle(EMSGSIZE);                                     /*  ������̫С                  */
            return  (PX_ERROR);

        } else if (stMsgLen) {
            break;                                                      /*  ���ݿɶ�                    */
        }
        BMSG_DEV_UNLOCK();
    }
                                                                        /*  ��ȡ����                    */
    sstRet = (ssize_t)_bmsgGet(pbmsginode->BMSGI_pbmsg, pcBuffer, stMaxBytes);

    if (_bmsgNBytes(pbmsginode->BMSGI_pbmsg)) {
        BMSG_FILE_POST_READ(pbmsgfil);                                  /*  ���ݿɶ�                    */
        SEL_WAKE_UP_ALL(&pbmsginode->BMSGI_selwulist, SELREAD);
    }

    if (_bmsgFreeByte(pbmsginode->BMSGI_pbmsg) >= pbmsginode->BMSGI_stAtSize) {
        BMSG_FILE_POST_WRITE(pbmsgfil);                                 /*  ���ݿ�д                    */
        SEL_WAKE_UP_ALL(&pbmsginode->BMSGI_selwulist, SELWRITE);
    }
    BMSG_DEV_UNLOCK();

    return  (sstRet);
}
/*********************************************************************************************************
** ��������: _bmsgWrite
** ��������: д bmsg �豸
** �䡡��  : pbmsgfil         bmsg �ļ�
**           pcBuffer         ��Ҫд�������ָ��
**           stNBytes         д�����ݴ�С
** �䡡��  : д���ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  _bmsgWrite (PLW_BMSG_FILE  pbmsgfil,
                            PCHAR          pcBuffer,
                            size_t         stNBytes)
{
    PLW_BMSG_INODE  pbmsginode;
    ULONG           ulTimeout;
    ssize_t         sstRet;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!stNBytes) {
        return  (0);
    }

    if (pbmsgfil->BMSGF_iFlag & O_NONBLOCK) {                           /*  ������ IO                   */
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = pbmsgfil->BMSGF_ulRTimeout;
    }

    if (!pbmsgfil->BMSGF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    pbmsginode = pbmsgfil->BMSGF_pinode;
    if (!pbmsginode->BMSGI_pbmsg) {                                     /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    if (stNBytes > pbmsginode->BMSGI_stAtSize) {                        /*  �������ݹ���                */
        _ErrorHandle(EMSGSIZE);
        return  (PX_ERROR);
    }

    for (;;) {
        if (BMSG_FILE_PEND_WRITE(pbmsgfil, ulTimeout)) {                /*  �ȴ����ݿ�д                */
            _ErrorHandle(EAGAIN);
            return  (0);
        }

        BMSG_DEV_LOCK();
        if (_bmsgFreeByte(pbmsginode->BMSGI_pbmsg) >= stNBytes) {       /*  ���㹻�ռ��д              */
            break;
        }
        BMSG_DEV_UNLOCK();
    }
                                                                        /*  д������                    */
    sstRet = (ssize_t)_bmsgPut(pbmsginode->BMSGI_pbmsg, pcBuffer, stNBytes);

    if (_bmsgFreeByte(pbmsginode->BMSGI_pbmsg) >= pbmsginode->BMSGI_stAtSize) {
        BMSG_FILE_POST_WRITE(pbmsgfil);                                 /*  ���ݿ�д                    */
        SEL_WAKE_UP_ALL(&pbmsginode->BMSGI_selwulist, SELWRITE);
    }

    BMSG_FILE_POST_READ(pbmsgfil);                                      /*  ���ݿɶ�                    */
    SEL_WAKE_UP_ALL(&pbmsginode->BMSGI_selwulist, SELREAD);
    BMSG_DEV_UNLOCK();

    return  (sstRet);
}
/*********************************************************************************************************
** ��������: _bmsgReadDir
** ��������: bmsg �豸���ָ��Ŀ¼��Ϣ
** �䡡��  : pbmsgfil            bmsg �ļ�
**           dir                 Ŀ¼�ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgReadDir (PLW_BMSG_FILE  pbmsgfil, DIR  *dir)
{
    INT             i, iCnt;
    INT             iError = ERROR_NONE;
    LONG            iStart;
    PLW_LIST_LINE   plineTemp;
    PLW_BMSG_INODE  pbmsginode;

    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pbmsgfil->BMSGF_pinode) {
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }

    iStart = dir->dir_pos;

    BMSG_DEV_LOCK();
    iCnt = 0;
    for (i = 0; i < LW_BMSG_DEV_HASH; i++) {
        for (plineTemp  = _G_bmsgdev.BMSGD_plineInode[i];
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
        pbmsginode = _LIST_ENTRY(plineTemp, LW_BMSG_INODE, BMSGI_lineManage);
        dir->dir_pos++;

        lib_strlcpy(dir->dir_dirent.d_name,
                    pbmsginode->BMSGI_cName,
                    sizeof(dir->dir_dirent.d_name));

        dir->dir_dirent.d_type = DT_FIFO;
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }
    BMSG_DEV_UNLOCK();

    return  (iError);
}
/*********************************************************************************************************
** ��������: _bmsgNFreeFnode
** ��������: bmsg ����ļ��п����ֽ���
** �䡡��  : pbmsgfil            bmsg �ļ�
**           piNFree             �����ֽ���
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgNFreeFnode (PLW_BMSG_FILE  pbmsgfil, INT  *piNFree)
{
    if (!piNFree) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pbmsgfil->BMSGF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (!pbmsgfil->BMSGF_pinode->BMSGI_pbmsg) {                         /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    BMSG_DEV_LOCK();
    *piNFree = _bmsgFreeByte(pbmsgfil->BMSGF_pinode->BMSGI_pbmsg);
    BMSG_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgNReadFnode
** ��������: bmsg ����ļ���ʣ��������ֽ���
** �䡡��  : pbmsgfil            bmsg �ļ�
**           piNRead             �����ֽ���
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgNReadFnode (PLW_BMSG_FILE  pbmsgfil, INT  *piNRead)
{
    if (!piNRead) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pbmsgfil->BMSGF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (!pbmsgfil->BMSGF_pinode->BMSGI_pbmsg) {                         /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    BMSG_DEV_LOCK();
    *piNRead = _bmsgNBytes(pbmsgfil->BMSGF_pinode->BMSGI_pbmsg);
    BMSG_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgNNextFnode
** ��������: bmsg ����ļ�����һ�ζ�ȡ�����ֽ���
** �䡡��  : pbmsgfil            bmsg �ļ�
**           piNRead             �����ֽ���
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgNNextFnode (PLW_BMSG_FILE  pbmsgfil, INT  *piNNext)
{
    if (!piNNext) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pbmsgfil->BMSGF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (!pbmsgfil->BMSGF_pinode->BMSGI_pbmsg) {                         /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    BMSG_DEV_LOCK();
    *piNNext = _bmsgNBytesNext(pbmsgfil->BMSGF_pinode->BMSGI_pbmsg);
    BMSG_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgFlushFnode
** ��������: bmsg ����ļ�����
** �䡡��  : pbmsgfil            bmsg �ļ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgFlushFnode (PLW_BMSG_FILE  pbmsgfil)
{
    INT  iRet;

    if (!pbmsgfil->BMSGF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    BMSG_DEV_LOCK();
    iRet = _bmsgFlushInode(pbmsgfil->BMSGF_pinode);
    BMSG_DEV_UNLOCK();

    return  (iRet);
}
/*********************************************************************************************************
** ��������: _bmsgSetFnode
** ��������: bmsg �����ļ�����
** �䡡��  : pbmsgfil            bmsg �ļ�
**           param               �������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgSetFnode (PLW_BMSG_FILE  pbmsgfil, struct bmsg_param *param)
{
    INT  iRet;

    if (!param) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pbmsgfil->BMSGF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    BMSG_DEV_LOCK();
    iRet = _bmsgSetInode(pbmsgfil->BMSGF_pinode, param);
    BMSG_DEV_UNLOCK();

    return  (iRet);
}
/*********************************************************************************************************
** ��������: _bmsgGetFnode
** ��������: bmsg ��ȡ�ļ�����
** �䡡��  : pbmsgfil            bmsg �ļ�
**           param               �������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgGetFnode (PLW_BMSG_FILE  pbmsgfil, struct bmsg_param *param)
{
    if (!param) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pbmsgfil->BMSGF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    BMSG_DEV_LOCK();
    if (pbmsgfil->BMSGF_pinode->BMSGI_pbmsg) {
        param->total_size  = (UINT32)_bmsgSizeGet(pbmsgfil->BMSGF_pinode->BMSGI_pbmsg);
        param->atomic_size = (UINT32)pbmsgfil->BMSGF_pinode->BMSGI_stAtSize;

    } else {
        param->total_size  = 0;
        param->atomic_size = 0;
    }
    BMSG_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgBindFnode
** ��������: bmsg �ļ����°� inode
** �䡡��  : pbmsgfil            bmsg �ļ�
**           pcName              inode ����
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgBindFnode (PLW_BMSG_FILE  pbmsgfil, CPCHAR  pcName)
{
    INT             iKey;
    PLW_BMSG_INODE  pbmsginode, pbmsginodeOld;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((pbmsgfil->BMSGF_iFlag & O_ACCMODE) != O_WRONLY) {
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    BMSG_DEV_LOCK();
    pbmsginode = _bmsgFindInode(pcName);
    if (pbmsginode) {
        if (pbmsgfil->BMSGF_pinode) {
            pbmsginodeOld = pbmsgfil->BMSGF_pinode;
            pbmsginodeOld->BMSGI_iOpenNum--;
            if (!pbmsginodeOld->BMSGI_iOpenNum && pbmsginodeOld->BMSGI_iAutoUnlink) {
                iKey = __hashHorner(pbmsginodeOld->BMSGI_cName, LW_BMSG_DEV_HASH);
                _List_Line_Del(&pbmsginodeOld->BMSGI_lineManage, &_G_bmsgdev.BMSGD_plineInode[iKey]);
                _bmsgDeleteInode(pbmsginodeOld);
            }
        }
        pbmsgfil->BMSGF_pinode = pbmsginode;
        pbmsginode->BMSGI_iOpenNum++;
    }
    BMSG_DEV_UNLOCK();

    if (!pbmsginode) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgUnbindFnode
** ��������: bmsg �ļ������ inode
** �䡡��  : pbmsgfil            bmsg �ļ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgUnbindFnode (PLW_BMSG_FILE  pbmsgfil)
{
    INT             iKey;
    PLW_BMSG_INODE  pbmsginodeOld;

    BMSG_DEV_LOCK();
    if (pbmsgfil->BMSGF_pinode) {
        pbmsginodeOld = pbmsgfil->BMSGF_pinode;
        pbmsginodeOld->BMSGI_iOpenNum--;
        if (!pbmsginodeOld->BMSGI_iOpenNum && pbmsginodeOld->BMSGI_iAutoUnlink) {
            iKey = __hashHorner(pbmsginodeOld->BMSGI_cName, LW_BMSG_DEV_HASH);
            _List_Line_Del(&pbmsginodeOld->BMSGI_lineManage, &_G_bmsgdev.BMSGD_plineInode[iKey]);
            _bmsgDeleteInode(pbmsginodeOld);
        }
        pbmsgfil->BMSGF_pinode = LW_NULL;
    }
    BMSG_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgStat
** ��������: bmsg ��ȡ�ļ�����
** �䡡��  : pbmsgfil            bmsg �ļ�
**           pstat               �ļ�����
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgStat (PLW_BMSG_FILE  pbmsgfil, struct stat *pstat)
{
    PLW_BMSG_INODE  pbmsginode;

    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pbmsgfil->BMSGF_pinode) {
        pbmsginode        = pbmsgfil->BMSGF_pinode;
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_bmsgdev.BMSGD_devhdrHdr);
        pstat->st_ino     = (ino_t)pbmsginode;
        pstat->st_mode    = pbmsginode->BMSGI_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = pbmsginode->BMSGI_uid;
        pstat->st_gid     = pbmsginode->BMSGI_gid;
        pstat->st_rdev    = 1;
        pstat->st_atime   = pbmsginode->BMSGI_time;
        pstat->st_mtime   = pbmsginode->BMSGI_time;
        pstat->st_ctime   = pbmsginode->BMSGI_time;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;

        BMSG_DEV_LOCK();
        if (pbmsginode->BMSGI_pbmsg) {
            pstat->st_size = (size_t)_bmsgNBytes(pbmsginode->BMSGI_pbmsg);
        } else {
            pstat->st_size = 0;
        }
        BMSG_DEV_UNLOCK();

    } else {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_bmsgdev.BMSGD_devhdrHdr);
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
** ��������: _bmsgStatfs
** ��������: bmsg ��ȡ�ļ�ϵͳ����
** �䡡��  : pbmsgfil            bmsg �ļ�
**           pstatfs             �ļ�ϵͳ����
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgStatfs (PLW_BMSG_FILE  pbmsgfil, struct statfs *pstatfs)
{
    INT             i;
    size_t          stTotal = 0;
    PLW_LIST_LINE   plineTemp;
    PLW_BMSG_INODE  pbmsginode;

    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    BMSG_DEV_LOCK();
    for (i = 0; i < LW_BMSG_DEV_HASH; i++) {
        for (plineTemp  = _G_bmsgdev.BMSGD_plineInode[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {

            pbmsginode = _LIST_ENTRY(plineTemp, LW_BMSG_INODE, BMSGI_lineManage);
            if (pbmsginode->BMSGI_pbmsg) {
                stTotal += _bmsgSizeGet(pbmsginode->BMSGI_pbmsg);       /*  ���ĵ����ڴ���              */
            }
        }
    }
    BMSG_DEV_UNLOCK();

    pstatfs->f_type   = 0;
    pstatfs->f_bsize  = 512;
    pstatfs->f_blocks = stTotal >> 9;
    pstatfs->f_bfree  = 0;
    pstatfs->f_bavail = 1;

    pstatfs->f_files  = 0;
    pstatfs->f_ffree  = 0;

#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatfs->f_fsid.val[0] = (int32_t)((addr_t)&_G_bmsgdev >> 32);
    pstatfs->f_fsid.val[1] = (int32_t)((addr_t)&_G_bmsgdev & 0xffffffff);
#else
    pstatfs->f_fsid.val[0] = (int32_t)&_G_bmsgdev;
    pstatfs->f_fsid.val[1] = 0;
#endif

    pstatfs->f_flag    = 0;
    pstatfs->f_namelen = PATH_MAX;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgSelect
** ��������: bmsg ����ļ�״̬
** �䡡��  : pbmsgfil            bmsg �ļ�
**           pselwunNode         select �ڵ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgSelect (PLW_BMSG_FILE  pbmsgfil, PLW_SEL_WAKEUPNODE  pselwunNode)
{
    PLW_BMSG_INODE  pbmsginode;

    if (!pselwunNode) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pbmsgfil->BMSGF_pinode) {
        if (pselwunNode->SELWUN_seltypType != SELEXCEPT) {
            SEL_WAKE_UP(pselwunNode);
        }
        return  (ERROR_NONE);
    }

    if (!pbmsgfil->BMSGF_pinode->BMSGI_pbmsg) {                         /*  û�г�ʼ��                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    pbmsginode = pbmsgfil->BMSGF_pinode;
    SEL_WAKE_NODE_ADD(&pbmsginode->BMSGI_selwulist, pselwunNode);

    BMSG_DEV_LOCK();
    switch (pselwunNode->SELWUN_seltypType) {

    case SELREAD:
        if (pbmsginode->BMSGI_pbmsg &&
            _bmsgNBytes(pbmsginode->BMSGI_pbmsg)) {
            SEL_WAKE_UP(pselwunNode);
        }
        break;

    case SELWRITE:
        if (pbmsginode->BMSGI_pbmsg &&
            (_bmsgFreeByte(pbmsginode->BMSGI_pbmsg) >= pbmsginode->BMSGI_stAtSize)) {
            SEL_WAKE_UP(pselwunNode);
        }
        break;

    case SELEXCEPT:
        break;
    }
    BMSG_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgUnselect
** ��������: bmsg ȡ������ļ�״̬
** �䡡��  : pbmsgfil            bmsg �ļ�
**           pselwunNode         select �ڵ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgUnselect (PLW_BMSG_FILE  pbmsgfil, PLW_SEL_WAKEUPNODE  pselwunNode)
{
    PLW_BMSG_INODE  pbmsginode;

    if (!pselwunNode) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pbmsgfil->BMSGF_pinode) {
        if (pselwunNode->SELWUN_seltypType != SELEXCEPT) {
            LW_SELWUN_SET_READY(pselwunNode);
        }
        return  (ERROR_NONE);
    }

    pbmsginode = pbmsgfil->BMSGF_pinode;
    SEL_WAKE_NODE_DELETE(&pbmsginode->BMSGI_selwulist, pselwunNode);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _bmsgIoctl
** ��������: ���� bmsg �ļ�
** �䡡��  : pbmsgfil         bmsg �ļ�
**           iRequest         ����
**           lArg             ����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  _bmsgIoctl (PLW_BMSG_FILE   pbmsgfil,
                        INT             iRequest,
                        LONG            lArg)
{
    INT  iRet = ERROR_NONE;

    switch (iRequest) {

    case FIOSEEK:
    case FIOWHERE:
        iRet = PX_ERROR;
        _ErrorHandle(ESPIPE);
        break;

    case FIONBIO:
        if (*(INT *)lArg) {
            pbmsgfil->BMSGF_iFlag |= O_NONBLOCK;
        } else {
            pbmsgfil->BMSGF_iFlag &= ~O_NONBLOCK;
        }
        break;

    case FIONFREE:
        return  (_bmsgNFreeFnode(pbmsgfil, (INT *)lArg));

    case FIONREAD:
        return  (_bmsgNReadFnode(pbmsgfil, (INT *)lArg));

    case FIOBMSGNNEXT:
        return  (_bmsgNNextFnode(pbmsgfil, (INT *)lArg));

    case FIOFLUSH:
        return  (_bmsgFlushFnode(pbmsgfil));

    case FIOBMSGGET:
        return  (_bmsgGetFnode(pbmsgfil, (struct bmsg_param *)lArg));

    case FIOBMSGSET:
        return  (_bmsgSetFnode(pbmsgfil, (struct bmsg_param *)lArg));

    case FIOBMSGBIND:
        return  (_bmsgBindFnode(pbmsgfil, (CPCHAR)lArg));

    case FIOBMSGUNBIND:
        return  (_bmsgUnbindFnode(pbmsgfil));

    case FIOFSTATGET:
        return  (_bmsgStat(pbmsgfil, (struct stat *)lArg));

    case FIOFSTATFSGET:
        return  (_bmsgStatfs(pbmsgfil, (struct statfs *)lArg));

    case FIOREADDIR:
        return  (_bmsgReadDir(pbmsgfil, (DIR *)lArg));

    case FIOSELECT:
        return  (_bmsgSelect(pbmsgfil, (PLW_SEL_WAKEUPNODE)lArg));

    case FIOUNSELECT:
        return  (_bmsgUnselect(pbmsgfil, (PLW_SEL_WAKEUPNODE)lArg));

    case FIORTIMEOUT:                                                   /*  ���ö���ʱʱ��              */
        {
            struct timeval *ptvTimeout = (struct timeval *)lArg;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  ��� tick ����              */
                pbmsgfil->BMSGF_ulRTimeout = ulTick;
            } else {
                pbmsgfil->BMSGF_ulRTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;

    case FIOWTIMEOUT:
        {
            struct timeval *ptvTimeout = (struct timeval *)lArg;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  ��� tick ����              */
                pbmsgfil->BMSGF_ulWTimeout = ulTick;
            } else {
                pbmsgfil->BMSGF_ulWTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;

    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "Bmsg IPC Service";
        return  (ERROR_NONE);

    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }

    return  (iRet);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_BMSG_EN > 0          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
