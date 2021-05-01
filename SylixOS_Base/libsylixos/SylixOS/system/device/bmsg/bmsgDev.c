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
** 文   件   名: bmsgDev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2019 年 02 月 02 日
**
** 描        述: 有边界消息设备实现.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_BMSG_EN > 0)
#include "sys/bmsgfd.h"
#include "fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  驱动程序全局变量
*********************************************************************************************************/
static INT              _G_iBmsgDrvNum = PX_ERROR;
static LW_BMSG_DEV      _G_bmsgdev;
static LW_OBJECT_HANDLE _G_hBmsgSelMutex;
/*********************************************************************************************************
  hash
*********************************************************************************************************/
extern INT      __hashHorner(CPCHAR  pcKeyword, INT  iTableSize);
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
static LONG     _bmsgOpen(PLW_BMSG_DEV    pbmsgdev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _bmsgRemove(PLW_BMSG_DEV  pbmsgdev, PCHAR  pcName);
static INT      _bmsgClose(PLW_BMSG_FILE  pbmsgfil);
static ssize_t  _bmsgRead(PLW_BMSG_FILE   pbmsgfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static ssize_t  _bmsgWrite(PLW_BMSG_FILE  pbmsgfil, PCHAR  pcBuffer, size_t  stNBytes);
static INT      _bmsgIoctl(PLW_BMSG_FILE  pbmsgfil, INT    iRequest, LONG  lArg);
/*********************************************************************************************************
  设备互斥访问
*********************************************************************************************************/
#define BMSG_DEV_LOCK()     API_SemaphoreMPend(_G_bmsgdev.BMSGD_ulMutex, LW_OPTION_WAIT_INFINITE)
#define BMSG_DEV_UNLOCK()   API_SemaphoreMPost(_G_bmsgdev.BMSGD_ulMutex)
/*********************************************************************************************************
  bmsg 读写阻塞与通知
*********************************************************************************************************/
#define BMSG_FILE_PEND_READ(f, to)  API_SemaphoreBPend((f)->BMSGF_pinode->BMSGI_ulReadLock, to)
#define BMSG_FILE_POST_READ(f)      API_SemaphoreBPost((f)->BMSGF_pinode->BMSGI_ulReadLock)
#define BMSG_FILE_PEND_WRITE(f, to) API_SemaphoreBPend((f)->BMSGF_pinode->BMSGI_ulWriteLock, to)
#define BMSG_FILE_POST_WRITE(f)     API_SemaphoreBPost((f)->BMSGF_pinode->BMSGI_ulWriteLock)
/*********************************************************************************************************
** 函数名称: API_BmsgDrvInstall
** 功能描述: 安装 bmsg 消息设备驱动程序
** 输　入  : NONE
** 输　出  : 驱动是否安装成功
** 全局变量:
** 调用模块:
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
** 函数名称: API_BmsgDevCreate
** 功能描述: 安装 bmsg 消息设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量:
** 调用模块:
                                           API 函数
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
** 函数名称: _bmsgCheckFileName
** 功能描述: 检查文件名是否合法
** 输　入  : pcName        文件名
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  _bmsgCheckFileName (CPCHAR  pcName)
{
    if (lib_index(pcName, PX_DIVIDER)) {
        return  (PX_ERROR);
    }

    return  (__fsCheckFileName(pcName));
}
/*********************************************************************************************************
** 函数名称: _bmsgFindInode
** 功能描述: 寻找 bmsg 节点
** 输　入  : pcName        文件名
** 输　出  : bmsg 节点
** 全局变量:
** 调用模块:
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
** 函数名称: _bmsgCreateInode
** 功能描述: 创建一个空的 bmsg 节点
** 输　入  : pcName        文件名
**           mode          mode_t
** 输　出  : bmsg 节点
** 全局变量:
** 调用模块:
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
** 函数名称: _bmsgDeleteInode
** 功能描述: 删除 bmsg 节点
** 输　入  : pbmsginode       bmsg 节点
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: _bmsgSetInode
** 功能描述: 设置 bmsg 节点
** 输　入  : pbmsginode       bmsg 节点
**           param            缓存参数
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  _bmsgSetInode (PLW_BMSG_INODE  pbmsginode, struct bmsg_param *param)
{
    PLW_BMSG  bmsg;

    if ((param->total_size < 16) || !param->atomic_size ||
        (param->atomic_size > (param->total_size - 2))) {               /*  长度会占两个字节            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (param->atomic_size > UINT16_MAX) {
        _ErrorHandle(EMSGSIZE);
        return  (PX_ERROR);
    }

    if (pbmsginode->BMSGI_pbmsg &&
        !(param->param_flags & BMSGFD_PARAM_FLAG_FORCE)) {              /*  已经存在且非强制            */
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
** 函数名称: _bmsgFlushInode
** 功能描述: 清除 bmsg 节点信息
** 输　入  : pbmsginode       bmsg 节点
** 输　出  : ERROR
** 全局变量:
** 调用模块:
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
** 函数名称: _bmsgOpen
** 功能描述: 打开 bmsg 设备
** 输　入  : pbmsgdev         bmsg 设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量:
** 调用模块:
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

    if (*pcName == PX_DIVIDER) {                                        /*  去掉首部目录分隔符          */
        pcName++;
    }

    if (iFlags & O_CREAT) {                                             /*  创建操作                    */
        if (_bmsgCheckFileName(pcName)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        if (!S_ISFIFO(iMode)) {
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  必须是 FIFO                 */
            return  (PX_ERROR);
        }
    }

    if (*pcName == PX_EOS) {                                            /*  打开设备                    */
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

    } else {                                                            /*  打开一条管道                */
        if (iFlags & O_DIRECTORY) {
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }

        BMSG_DEV_LOCK();
        pbmsginode = _bmsgFindInode(pcName);
        if (pbmsginode) {
            if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {              /*  排他性创建                  */
                BMSG_DEV_UNLOCK();
                _ErrorHandle(EEXIST);
                return  (PX_ERROR);
            }

            if (_IosCheckPermissions(iFlags, LW_FALSE,
                                     pbmsginode->BMSGI_mode,
                                     pbmsginode->BMSGI_uid,
                                     pbmsginode->BMSGI_gid) < ERROR_NONE) {
                BMSG_DEV_UNLOCK();
                _ErrorHandle(EACCES);                                   /*  没有权限                    */
                return  (PX_ERROR);
            }

        } else {                                                        /*  没有找到节点                */
            if (iFlags & O_CREAT) {
                pbmsginode = _bmsgCreateInode(pcName, iMode);           /*  创建一个新的 inode          */
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
                _bmsgDeleteInode(pbmsginode);                           /*  删除新创建的节点            */
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
** 函数名称: _bmsgRemove
** 功能描述: 删除 inode 文件
** 输　入  : pbmsgdev          bmsg 设备
**           pcName            需要删除的文件
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  _bmsgRemove (PLW_BMSG_DEV  pbmsgdev, PCHAR  pcName)
{
    PLW_BMSG_INODE pbmsginode;
    INT            iKey;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (*pcName == PX_DIVIDER) {                                        /*  去掉首部目录分隔符          */
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
            _ErrorHandle(EACCES);                                       /*  没有权限                    */
            return  (PX_ERROR);
        }

        if (pbmsginode->BMSGI_iOpenNum) {
            BMSG_DEV_UNLOCK();
            _ErrorHandle(EBUSY);                                        /*  文件正在操作                */
            return  (PX_ERROR);
        }

        iKey = __hashHorner(pbmsginode->BMSGI_cName, LW_BMSG_DEV_HASH);
        _List_Line_Del(&pbmsginode->BMSGI_lineManage, &_G_bmsgdev.BMSGD_plineInode[iKey]);
        _bmsgDeleteInode(pbmsginode);

    } else {                                                            /*  未找到                      */
        BMSG_DEV_UNLOCK();
        _ErrorHandle(ENOENT);                                           /*  文件正在操作                */
        return  (PX_ERROR);
    }
    BMSG_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _bmsgClose
** 功能描述: 关闭 bmsg 文件
** 输　入  : pbmsgfil          bmsg 文件
** 输　出  : ERROR
** 全局变量:
** 调用模块:
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
** 函数名称: _bmsgRead
** 功能描述: 读 bmsg 设备
** 输　入  : pbmsgfil         bmsgfd 文件
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 读取字节数
** 全局变量:
** 调用模块:
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

    if (pbmsgfil->BMSGF_iFlag & O_NONBLOCK) {                           /*  非阻塞 IO                   */
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = pbmsgfil->BMSGF_ulRTimeout;
    }

    if (!pbmsgfil->BMSGF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    pbmsginode = pbmsgfil->BMSGF_pinode;
    if (!pbmsginode->BMSGI_pbmsg) {                                     /*  没有初始化                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    for (;;) {
        if (BMSG_FILE_PEND_READ(pbmsgfil, ulTimeout)) {                 /*  等待数据可读                */
            _ErrorHandle(EAGAIN);
            return  (0);
        }

        BMSG_DEV_LOCK();
        stMsgLen = (size_t)_bmsgNBytesNext(pbmsginode->BMSGI_pbmsg);
        if (stMsgLen > stMaxBytes) {
            BMSG_DEV_UNLOCK();
            BMSG_FILE_POST_READ(pbmsgfil);
            _ErrorHandle(EMSGSIZE);                                     /*  缓冲区太小                  */
            return  (PX_ERROR);

        } else if (stMsgLen) {
            break;                                                      /*  数据可读                    */
        }
        BMSG_DEV_UNLOCK();
    }
                                                                        /*  读取数据                    */
    sstRet = (ssize_t)_bmsgGet(pbmsginode->BMSGI_pbmsg, pcBuffer, stMaxBytes);

    if (_bmsgNBytes(pbmsginode->BMSGI_pbmsg)) {
        BMSG_FILE_POST_READ(pbmsgfil);                                  /*  数据可读                    */
        SEL_WAKE_UP_ALL(&pbmsginode->BMSGI_selwulist, SELREAD);
    }

    if (_bmsgFreeByte(pbmsginode->BMSGI_pbmsg) >= pbmsginode->BMSGI_stAtSize) {
        BMSG_FILE_POST_WRITE(pbmsgfil);                                 /*  数据可写                    */
        SEL_WAKE_UP_ALL(&pbmsginode->BMSGI_selwulist, SELWRITE);
    }
    BMSG_DEV_UNLOCK();

    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: _bmsgWrite
** 功能描述: 写 bmsg 设备
** 输　入  : pbmsgfil         bmsg 文件
**           pcBuffer         将要写入的数据指针
**           stNBytes         写入数据大小
** 输　出  : 写入字节数
** 全局变量:
** 调用模块:
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

    if (pbmsgfil->BMSGF_iFlag & O_NONBLOCK) {                           /*  非阻塞 IO                   */
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = pbmsgfil->BMSGF_ulRTimeout;
    }

    if (!pbmsgfil->BMSGF_pinode) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    pbmsginode = pbmsgfil->BMSGF_pinode;
    if (!pbmsginode->BMSGI_pbmsg) {                                     /*  没有初始化                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    if (stNBytes > pbmsginode->BMSGI_stAtSize) {                        /*  操作数据过大                */
        _ErrorHandle(EMSGSIZE);
        return  (PX_ERROR);
    }

    for (;;) {
        if (BMSG_FILE_PEND_WRITE(pbmsgfil, ulTimeout)) {                /*  等待数据可写                */
            _ErrorHandle(EAGAIN);
            return  (0);
        }

        BMSG_DEV_LOCK();
        if (_bmsgFreeByte(pbmsginode->BMSGI_pbmsg) >= stNBytes) {       /*  有足够空间可写              */
            break;
        }
        BMSG_DEV_UNLOCK();
    }
                                                                        /*  写入数据                    */
    sstRet = (ssize_t)_bmsgPut(pbmsginode->BMSGI_pbmsg, pcBuffer, stNBytes);

    if (_bmsgFreeByte(pbmsginode->BMSGI_pbmsg) >= pbmsginode->BMSGI_stAtSize) {
        BMSG_FILE_POST_WRITE(pbmsgfil);                                 /*  数据可写                    */
        SEL_WAKE_UP_ALL(&pbmsginode->BMSGI_selwulist, SELWRITE);
    }

    BMSG_FILE_POST_READ(pbmsgfil);                                      /*  数据可读                    */
    SEL_WAKE_UP_ALL(&pbmsginode->BMSGI_selwulist, SELREAD);
    BMSG_DEV_UNLOCK();

    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: _bmsgReadDir
** 功能描述: bmsg 设备获得指定目录信息
** 输　入  : pbmsgfil            bmsg 文件
**           dir                 目录结构
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
        iError = PX_ERROR;                                              /*  没有多余的节点              */

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
** 函数名称: _bmsgNFreeFnode
** 功能描述: bmsg 获得文件中空闲字节数
** 输　入  : pbmsgfil            bmsg 文件
**           piNFree             数据字节数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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

    if (!pbmsgfil->BMSGF_pinode->BMSGI_pbmsg) {                         /*  没有初始化                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    BMSG_DEV_LOCK();
    *piNFree = _bmsgFreeByte(pbmsgfil->BMSGF_pinode->BMSGI_pbmsg);
    BMSG_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _bmsgNReadFnode
** 功能描述: bmsg 获得文件中剩余的数据字节数
** 输　入  : pbmsgfil            bmsg 文件
**           piNRead             数据字节数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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

    if (!pbmsgfil->BMSGF_pinode->BMSGI_pbmsg) {                         /*  没有初始化                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    BMSG_DEV_LOCK();
    *piNRead = _bmsgNBytes(pbmsgfil->BMSGF_pinode->BMSGI_pbmsg);
    BMSG_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _bmsgNNextFnode
** 功能描述: bmsg 获得文件中下一次读取数据字节数
** 输　入  : pbmsgfil            bmsg 文件
**           piNRead             数据字节数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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

    if (!pbmsgfil->BMSGF_pinode->BMSGI_pbmsg) {                         /*  没有初始化                  */
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    BMSG_DEV_LOCK();
    *piNNext = _bmsgNBytesNext(pbmsgfil->BMSGF_pinode->BMSGI_pbmsg);
    BMSG_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _bmsgFlushFnode
** 功能描述: bmsg 清除文件缓存
** 输　入  : pbmsgfil            bmsg 文件
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
** 函数名称: _bmsgSetFnode
** 功能描述: bmsg 设置文件缓存
** 输　入  : pbmsgfil            bmsg 文件
**           param               缓存参数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
** 函数名称: _bmsgGetFnode
** 功能描述: bmsg 获取文件缓存
** 输　入  : pbmsgfil            bmsg 文件
**           param               缓存参数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
** 函数名称: _bmsgBindFnode
** 功能描述: bmsg 文件重新绑定 inode
** 输　入  : pbmsgfil            bmsg 文件
**           pcName              inode 名字
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
** 函数名称: _bmsgUnbindFnode
** 功能描述: bmsg 文件解除绑定 inode
** 输　入  : pbmsgfil            bmsg 文件
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
** 函数名称: _bmsgStat
** 功能描述: bmsg 获取文件属性
** 输　入  : pbmsgfil            bmsg 文件
**           pstat               文件属性
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
** 函数名称: _bmsgStatfs
** 功能描述: bmsg 获取文件系统属性
** 输　入  : pbmsgfil            bmsg 文件
**           pstatfs             文件系统属性
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
                stTotal += _bmsgSizeGet(pbmsginode->BMSGI_pbmsg);       /*  消耗的总内存数              */
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
** 函数名称: _bmsgSelect
** 功能描述: bmsg 检测文件状态
** 输　入  : pbmsgfil            bmsg 文件
**           pselwunNode         select 节点
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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

    if (!pbmsgfil->BMSGF_pinode->BMSGI_pbmsg) {                         /*  没有初始化                  */
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
** 函数名称: _bmsgUnselect
** 功能描述: bmsg 取消检测文件状态
** 输　入  : pbmsgfil            bmsg 文件
**           pselwunNode         select 节点
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
** 函数名称: _bmsgIoctl
** 功能描述: 控制 bmsg 文件
** 输　入  : pbmsgfil         bmsg 文件
**           iRequest         功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量:
** 调用模块:
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

    case FIORTIMEOUT:                                                   /*  设置读超时时间              */
        {
            struct timeval *ptvTimeout = (struct timeval *)lArg;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  获得 tick 数量              */
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
                ulTick = __timevalToTick(ptvTimeout);                   /*  获得 tick 数量              */
                pbmsgfil->BMSGF_ulWTimeout = ulTick;
            } else {
                pbmsgfil->BMSGF_ulWTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;

    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
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
