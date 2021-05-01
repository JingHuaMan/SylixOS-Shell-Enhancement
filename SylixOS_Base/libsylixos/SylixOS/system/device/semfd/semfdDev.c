/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: semfdDev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2019 年 02 月 23 日
**
** 描        述: 信号量设备实现.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SEMFD_EN > 0)
#include "sys/semfd.h"
#include "fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  驱动程序全局变量
*********************************************************************************************************/
static INT              _G_iSemfdDrvNum = PX_ERROR;
static LW_SEMFD_DEV     _G_semfddev;
static LW_OBJECT_HANDLE _G_hSemfdSelMutex;
/*********************************************************************************************************
  hash
*********************************************************************************************************/
extern INT      __hashHorner(CPCHAR  pcKeyword, INT  iTableSize);
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
static LONG     _semfdOpen(PLW_SEMFD_DEV    psemfddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _semfdRemove(PLW_SEMFD_DEV  psemfddev, PCHAR  pcName);
static INT      _semfdClose(PLW_SEMFD_FILE  psemfdfil);
static ssize_t  _semfdRead(PLW_SEMFD_FILE   psemfdfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static ssize_t  _semfdWrite(PLW_SEMFD_FILE  psemfdfil, PCHAR  pcBuffer, size_t  stNBytes);
static INT      _semfdIoctl(PLW_SEMFD_FILE  psemfdfil, INT    iRequest, LONG  lArg);
/*********************************************************************************************************
  设备互斥访问
*********************************************************************************************************/
#define SEMFD_DEV_LOCK()    API_SemaphoreMPend(_G_semfddev.SEMFD_ulMutex, LW_OPTION_WAIT_INFINITE)
#define SEMFD_DEV_UNLOCK()  API_SemaphoreMPost(_G_semfddev.SEMFD_ulMutex)
/*********************************************************************************************************
** 函数名称: API_SemfdDrvInstall
** 功能描述: 安装 semfd 驱动程序
** 输　入  : NONE
** 输　出  : 驱动是否安装成功
** 全局变量:
** 调用模块:
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
** 函数名称: API_SemfdDevCreate
** 功能描述: 安装 semfd 设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量:
** 调用模块:
                                           API 函数
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
** 函数名称: _semfdCheckFileName
** 功能描述: 检查文件名是否合法
** 输　入  : pcName        文件名
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  _semfdCheckFileName (CPCHAR  pcName)
{
    if (lib_index(pcName, PX_DIVIDER)) {
        return  (PX_ERROR);
    }

    return  (__fsCheckFileName(pcName));
}
/*********************************************************************************************************
** 函数名称: _semfdFindInode
** 功能描述: 寻找 semfd 节点
** 输　入  : pcName        文件名
** 输　出  : semfd 节点
** 全局变量:
** 调用模块:
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
** 函数名称: _semfdCreateInode
** 功能描述: 创建一个空的 semfd 节点
** 输　入  : pcName        文件名
**           mode          mode_t
** 输　出  : semfd 节点
** 全局变量:
** 调用模块:
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
** 函数名称: _semfdDeleteInode
** 功能描述: 删除 semfd 节点
** 输　入  : psemfdinode      semfd 节点
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: _semfdSetInode
** 功能描述: 设置 semfd 节点
** 输　入  : psemfdinode      semfd 节点
**           param            信号量参数
** 输　出  : ERROR
** 全局变量:
** 调用模块:
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
** 函数名称: _bsemfdOpen
** 功能描述: 打开 semfd 设备
** 输　入  : psemfddev        semfd 设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量:
** 调用模块:
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

    if (*pcName == PX_DIVIDER) {                                        /*  去掉首部目录分隔符          */
        pcName++;
    }

    if (iFlags & O_CREAT) {                                             /*  创建操作                    */
        if (_semfdCheckFileName(pcName)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        if (!S_ISCHR(iMode)) {
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  必须是 FIFO                 */
            return  (PX_ERROR);
        }
    }

    if (*pcName == PX_EOS) {                                            /*  打开设备                    */
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

    } else {                                                            /*  打开一个文件                */
        if (iFlags & O_DIRECTORY) {
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }

        SEMFD_DEV_LOCK();
        psemfdinode = _semfdFindInode(pcName);
        if (psemfdinode) {
            if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {              /*  排他性创建                  */
                SEMFD_DEV_UNLOCK();
                _ErrorHandle(EEXIST);
                return  (PX_ERROR);
            }

            if (_IosCheckPermissions(iFlags, LW_FALSE,
                                     psemfdinode->SEMFDI_mode,
                                     psemfdinode->SEMFDI_uid,
                                     psemfdinode->SEMFDI_gid) < ERROR_NONE) {
                SEMFD_DEV_UNLOCK();
                _ErrorHandle(EACCES);                                   /*  没有权限                    */
                return  (PX_ERROR);
            }

        } else {                                                        /*  没有找到节点                */
            if (iFlags & O_CREAT) {
                psemfdinode = _semfdCreateInode(pcName, iMode);         /*  创建一个新的 inode          */
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
                _semfdDeleteInode(psemfdinode);                         /*  删除新创建的节点            */
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
** 函数名称: _semfdRemove
** 功能描述: 删除 inode 文件
** 输　入  : psemfddev        semfd 设备
**           pcName           inode 名
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  _semfdRemove (PLW_SEMFD_DEV  psemfddev, PCHAR  pcName)
{
    PLW_SEMFD_INODE psemfdinode;
    INT             iKey;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (*pcName == PX_DIVIDER) {                                        /*  去掉首部目录分隔符          */
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
            _ErrorHandle(EACCES);                                       /*  没有权限                    */
            return  (PX_ERROR);
        }

        if (psemfdinode->SEMFDI_iOpenNum) {
            SEMFD_DEV_UNLOCK();
            _ErrorHandle(EBUSY);                                        /*  文件正在操作                */
            return  (PX_ERROR);
        }

        iKey = __hashHorner(psemfdinode->SEMFDI_cName, LW_SEMFD_DEV_HASH);
        _List_Line_Del(&psemfdinode->SEMFDI_lineManage, &_G_semfddev.SEMFD_plineInode[iKey]);
        _semfdDeleteInode(psemfdinode);

    } else {                                                            /*  未找到                      */
        SEMFD_DEV_UNLOCK();
        _ErrorHandle(ENOENT);                                           /*  文件正在操作                */
        return  (PX_ERROR);
    }
    SEMFD_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _semfdClose
** 功能描述: 关闭 semfd 文件
** 输　入  : psemfdfil        semfd 文件
** 输　出  : ERROR
** 全局变量:
** 调用模块:
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
** 函数名称: _semfdRead
** 功能描述: 读 semfd 设备
** 输　入  : psemfdfil        semfd 文件
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 读取字节数
** 全局变量:
** 调用模块:
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

    if (psemfdfil->SEMFDF_iFlag & O_NONBLOCK) {                         /*  非阻塞 IO                   */
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = psemfdfil->SEMFDF_ulTimeout;
    }

    if (!psemfdfil->SEMFDF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    psemfdinode = psemfdfil->SEMFDF_pinode;
    if (!psemfdinode->SEMFDI_ulSem) {                                   /*  没有初始化                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    ulError = API_SemaphorePend(psemfdinode->SEMFDI_ulSem, ulTimeout);  /*  等待信号量                  */
    if (ulError) {
        if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
            _ErrorHandle(EAGAIN);
        }
        return  (0);
    }

    if (psemfdinode->SEMFDI_ulOption & LW_OPTION_WAIT_PRIORITY) {
        SEL_WAKE_UP_PRIO(&psemfdinode->SEMFDI_selwulist, SELWRITE);     /*  可写                        */
    } else {
        SEL_WAKE_UP_FIFO(&psemfdinode->SEMFDI_selwulist, SELWRITE);
    }

    *pcBuffer = 1;

    return  (1);
}
/*********************************************************************************************************
** 函数名称: _semfdWrite
** 功能描述: 写 semfd 设备
** 输　入  : psemfdfil        semfd 文件
**           pcBuffer         将要写入的数据指针
**           stNBytes         写入数据大小
** 输　出  : 写入字节数
** 全局变量:
** 调用模块:
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
    if (!psemfdinode->SEMFDI_ulSem) {                                   /*  没有初始化                  */
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
        SEL_WAKE_UP_PRIO(&psemfdinode->SEMFDI_selwulist, SELREAD);      /*  可读                        */
    } else {
        SEL_WAKE_UP_FIFO(&psemfdinode->SEMFDI_selwulist, SELREAD);
    }

    return  (1);
}
/*********************************************************************************************************
** 函数名称: _semfdReadDir
** 功能描述: semfd 设备获得指定目录信息
** 输　入  : psemfdfil           semfd 文件
**           dir                 目录结构
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
        iError = PX_ERROR;                                              /*  没有多余的节点              */

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
** 函数名称: _semfdNFreeFnode
** 功能描述: semfd 获得文件中空闲字节数
** 输　入  : psemfdfil           semfd 文件
**           piNFree             数据字节数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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

    if (!psemfdfil->SEMFDF_pinode->SEMFDI_ulSem) {                      /*  没有初始化                  */
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
** 函数名称: _bsemfdNReadFnode
** 功能描述: semfd 获得文件中剩余的数据字节数
** 输　入  : psemfdfil           semfd 文件
**           piNRead             数据字节数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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

    if (!psemfdfil->SEMFDF_pinode->SEMFDI_ulSem) {                      /*  没有初始化                  */
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
** 函数名称: _semfdFlushFnode
** 功能描述: semfd 清除文件缓存
** 输　入  : psemfdfil           semfd 文件
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  _semfdFlushFnode (PLW_SEMFD_FILE  psemfdfil)
{
    if (!psemfdfil->SEMFDF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (!psemfdfil->SEMFDF_pinode->SEMFDI_ulSem) {                      /*  没有初始化                  */
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
** 函数名称: _semfdSetFnode
** 功能描述: semfd 设置文件缓存
** 输　入  : psemfdfil           semfd 文件
**           param               信号量参数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
** 函数名称: _semfdGetFnode
** 功能描述: semfd 获取文件缓存
** 输　入  : psemfdfil           semfd 文件
**           param               信号量参数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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

    if (!psemfdfil->SEMFDF_pinode->SEMFDI_ulSem) {                      /*  没有初始化                  */
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
** 函数名称: _semfdStat
** 功能描述: semfd 获取文件属性
** 输　入  : psemfdfil           semfd 文件
**           pstat               文件属性
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
** 函数名称: _semfdStatfs
** 功能描述: semfd 获取文件系统属性
** 输　入  : psemfdfil           semfd 文件
**           pstatfs             文件系统属性
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
** 函数名称: _semfdSelect
** 功能描述: semfd 检测文件状态
** 输　入  : psemfdfil           semfd 文件
**           pselwunNode         select 节点
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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

    if (!psemfdfil->SEMFDF_pinode->SEMFDI_ulSem) {                      /*  没有初始化                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    psemfdinode = psemfdfil->SEMFDF_pinode;
    SEL_WAKE_NODE_ADD(&psemfdinode->SEMFDI_selwulist, pselwunNode);

    _semfdGetFnode(psemfdfil, &param);                                  /*  获取状态                    */

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
** 函数名称: _semfdUnselect
** 功能描述: semfd 取消检测文件状态
** 输　入  : psemfdfil           semfd 文件
**           pselwunNode         select 节点
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
** 函数名称: _semfdIoctl
** 功能描述: 控制 semfd 文件
** 输　入  : psemfdfil        semfd 文件
**           iRequest         功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量:
** 调用模块:
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

    case FIORTIMEOUT:                                                   /*  设置读超时时间              */
        {
            struct timeval *ptvTimeout = (struct timeval *)lArg;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  获得 tick 数量              */
                psemfdfil->SEMFDF_ulTimeout = ulTick;
            } else {
                psemfdfil->SEMFDF_ulTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;

    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
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
