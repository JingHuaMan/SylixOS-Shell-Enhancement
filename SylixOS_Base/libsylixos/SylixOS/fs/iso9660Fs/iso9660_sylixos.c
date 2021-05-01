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
** 文   件   名: iso9660_sylixos.c
**
** 创   建   人: Tiger.Jiang (蒋太金)
**
** 文件创建日期: 2018 年 09 月 15 日
**
** 描        述: ISO9660 文件系统与 IO 系统接口部分.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/unique/unique.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_ISO9660FS_EN > 0)
#include "include/types.h"
#include "include/cdio.h"
#include "include/ds.h"
#include "iso9660_port.h"
#include "iso9660_sylixos.h"
#include "include/iso9660.h"
#include "include/bytesex.h"
/*********************************************************************************************************
  内部结构 (如果不使用 inode 功能会节约内存, 加快操作速度)
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          ISOVOL_devhdrHdr;                               /*  设备头                      */
    BOOL                ISOVOL_bForceDelete;                            /*  是否允许强制卸载卷          */
    iso9660_t          *ISOVOL_iso9660Vol;                              /*  文件系统卷信息              */
    INT                 ISOVOL_iDrv;                                    /*  驱动表位置                  */
    BOOL                ISOVAL_bValid;                                  /*  有效性标志                  */
    LW_OBJECT_HANDLE    ISOVOL_hVolLock;                                /*  卷操作锁                    */
    
    LW_LIST_LINE_HEADER ISOVOL_plineFdNodeHeader;                       /*  fd_node 链表                */
    UINT32              ISOVOL_uiTime;                                  /*  卷创建时间 ISO9660 格式     */
    INT                 ISOVOL_iFlag;                                   /*  O_RDONLY or O_RDWR          */
} ISO_VOLUME;
typedef ISO_VOLUME     *PISO_VOLUME;

typedef struct {
    PISO_VOLUME         ISOFIL_pisovol;                                 /*  所在卷信息                  */
    iso9660_stat_t     *ISOFIL_pstat;
    INT                 ISOFIL_iFileType;                               /*  文件类型                    */
    UINT64              ISOFIL_u64Uniq;                                 /*  64bits 簇 + 目录偏移        */
    ino_t               ISOFIL_inode;                                   /*  inode 编号                  */
    UCHAR               ISOFIL_ucBuff[ISO_BLOCKSIZE];                   /*  文件缓冲                    */
    CHAR                ISOFIL_cName[1];                                /*  文件名                      */
} ISO_FILE_T;
typedef ISO_FILE_T     *PISO_FILE_T;
/*********************************************************************************************************
  文件类型
*********************************************************************************************************/
#define __ISO_FILE_TYPE_NODE            0                               /*  open 打开文件               */
#define __ISO_FILE_TYPE_DIR             1                               /*  open 打开目录               */
#define __ISO_FILE_TYPE_DEV             2                               /*  open 打开设备               */
/*********************************************************************************************************
  ISO9660 主设备号与文件系统类型
*********************************************************************************************************/
static INT  _G_iIsoDrvNum = PX_ERROR;
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
INT             __blockIoDevCreate(PLW_BLK_DEV  pblkdNew);
VOID            __blockIoDevDelete(INT  iIndex);
PLW_BLK_DEV     __blockIoDevGet(INT  iIndex);
INT             __blockIoDevReset(INT  iIndex);
INT             __blockIoDevIoctl(INT  iIndex, INT  iCmd, LONG  lArg);
INT             __blockIoDevIsLogic(INT  iIndex);
INT             __blockIoDevFlag(INT  iIndex);
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __ISO_FILE_LOCK(pisofile)   API_SemaphoreMPend(pisofile->ISOFIL_pisovol->ISOVOL_hVolLock, \
                                    LW_OPTION_WAIT_INFINITE)
#define __ISO_FILE_UNLOCK(pisofile) API_SemaphoreMPost(pisofile->ISOFIL_pisovol->ISOVOL_hVolLock)
#define __ISO_VOL_LOCK(pisovol)     API_SemaphoreMPend(pisovol->ISOVOL_hVolLock, LW_OPTION_WAIT_INFINITE)
#define __ISO_VOL_UNLOCK(pISOvol)   API_SemaphoreMPost(pisovol->ISOVOL_hVolLock)
/*********************************************************************************************************
  检测路径字串是否为根目录或者直接指向设备
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)       ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
/*********************************************************************************************************
  驱动程序声明
*********************************************************************************************************/
static INT      __iso9660FsProbe(PLW_BLK_DEV  pblkd, UINT8  *pucPartType);
static LONG     __iso9660FsOpen(PISO_VOLUME     pisovol,
                                PCHAR           pcName,
                                INT             iFlags,
                                INT             iMode);
static INT      __iso9660FsRemove(PISO_VOLUME   pisovol,
                                  PCHAR         pcName);
static INT      __iso9660FsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __iso9660FsRead(PLW_FD_ENTRY   pfdentry,
                                PCHAR          pcBuffer,
                                size_t         stMaxBytes);
static ssize_t  __iso9660FsPRead(PLW_FD_ENTRY   pfdentry,
                                 PCHAR          pcBuffer,
                                 size_t         stMaxBytes,
                                 off_t          oftPos);
static ssize_t  __iso9660FsWrite(PLW_FD_ENTRY   pfdentry,
                                 PCHAR          pcBuffer,
                                 size_t         stNBytes);
static ssize_t  __iso9660FsPWrite(PLW_FD_ENTRY  pfdentry,
                                  PCHAR         pcBuffer,
                                  size_t        stNBytes,
                                  off_t         oftPos);
static INT      __iso9660FsIoctl(PLW_FD_ENTRY   pfdentry,
                                 INT            iRequest,
                                 LONG           lArg);
/*********************************************************************************************************
  文件系统创建函数
*********************************************************************************************************/
LW_API INT  API_Iso9660FsDevCreate(PCHAR   pcName, PLW_BLK_DEV  pblkd);
/*********************************************************************************************************
** 函数名称: API_Iso9660FsDrvInstall
** 功能描述: 安装 ISO9660 文件系统驱动程序
** 输　入  : 
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_Iso9660FsDrvInstall (VOID)
{
    struct file_operations  fileop;

    if (_G_iIsoDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __iso9660FsOpen;
    fileop.fo_release  = __iso9660FsRemove;
    fileop.fo_open     = __iso9660FsOpen;
    fileop.fo_close    = __iso9660FsClose;
    fileop.fo_read     = __iso9660FsRead;
    fileop.fo_read_ex  = __iso9660FsPRead;
    fileop.fo_write    = __iso9660FsWrite;
    fileop.fo_write_ex = __iso9660FsPWrite;
    fileop.fo_ioctl    = __iso9660FsIoctl;
    
    _G_iIsoDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);       /*  使用 NEW_1 型设备驱动       */

    DRIVER_LICENSE(_G_iIsoDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iIsoDrvNum,      "Tiger.Jiang");
    DRIVER_DESCRIPTION(_G_iIsoDrvNum, "ISO9660 driver.");
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "ISO9660 file system installed.\r\n");
                                                                        /*  注册文件系统                */
    __fsRegister("iso9660", API_Iso9660FsDevCreate, LW_NULL, (FUNCPTR)__iso9660FsProbe);
    
    return  ((_G_iIsoDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: __iso9660FsCheck
** 功能描述: 检测分区是否为 ISO9660 格式
** 输　入  : pblkd             块设备
**           pucPartType       返回分区类型
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __iso9660FsProbe (PLW_BLK_DEV  pblkd, UINT8  *pucPartType)
{
    ULONG          ulSecSize;
    iso9660_pvd_t *ppvd;

    if (!pblkd) {
        return  (ERROR_NONE);
    }

    pblkd->BLKD_pfuncBlkIoctl(pblkd, LW_BLKD_CTRL_POWER, LW_BLKD_POWER_ON);
    pblkd->BLKD_pfuncBlkReset(pblkd);
    pblkd->BLKD_pfuncBlkIoctl(pblkd, FIODISKINIT);

    ulSecSize = pblkd->BLKD_ulBytesPerSector;
    if (!ulSecSize) {
        pblkd->BLKD_pfuncBlkIoctl(pblkd, LW_BLKD_GET_SECSIZE, &ulSecSize);
    }
    if (ulSecSize != ISO_BLOCKSIZE) {
        return  (PX_ERROR);
    }

    ppvd = (iso9660_pvd_t *)__SHEAP_ALLOC((size_t)ulSecSize);
    if (!ppvd) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    if (pblkd->BLKD_pfuncBlkRd(pblkd, ppvd, ISO_PVD_SECTOR, 1) < 0) {
        __SHEAP_FREE(ppvd);
        return  (PX_ERROR);
    }

    if (ISO_VD_PRIMARY != from_711(ppvd->type)) {
        __SHEAP_FREE(ppvd);
        return  (PX_ERROR);
    }

    if (lib_strncmp(ppvd->id, ISO_STANDARD_ID, lib_strlen(ISO_STANDARD_ID))) {
        __SHEAP_FREE(ppvd);
        return  (PX_ERROR);
    }

    *pucPartType = LW_DISK_PART_TYPE_ISO9660;

    __SHEAP_FREE(ppvd);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_Iso9660FsDevCreate
** 功能描述: 创建一个 ISO 设备, 例如: API_Iso9660FsDevCreate("/ata0", ...);
**           与 sylixos 的 yaffs 不同, ISO 每一个卷都是独立的设备.
** 输　入  : pcName            设备名(设备挂接的节点地址)
**           pblkd             块设备驱动
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_Iso9660FsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    REGISTER PISO_VOLUME     pisovol;
    REGISTER INT             iBlkdIndex;
             INT             iErrLevel = 0;
             ULONG           ulError   = ERROR_NONE;

    if (_G_iIsoDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ISO9660 Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    if (pblkd == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "block device invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    if ((pcName == LW_NULL) || __STR_IS_ROOT(pcName)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "volume name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }

    if ((pblkd->BLKD_iLogic == 0) && (pblkd->BLKD_uiLinkCounter)) {     /*  物理设备被引用后不允许加载  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "logic device has already mount.\r\n");
        _ErrorHandle(ERROR_IO_ACCESS_DENIED);
        return  (PX_ERROR);
    }
    
    iBlkdIndex = __blockIoDevCreate(pblkd);                             /*  加入块设备驱动表            */
    if (iBlkdIndex == -1) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "block device invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);

    } else if (iBlkdIndex == -2) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "block device table full.\r\n");
        _ErrorHandle(ERROR_IOS_DRIVER_GLUT);
        return  (PX_ERROR);
    }

    pisovol = (PISO_VOLUME)__SHEAP_ALLOC(sizeof(ISO_VOLUME));
    if (pisovol == LW_NULL) {
        __blockIoDevDelete(iBlkdIndex);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pisovol, sizeof(ISO_VOLUME));                             /*  清空卷控制块                */
    
    pisovol->ISOVOL_bForceDelete = LW_FALSE;                            /*  不允许强制卸载卷            */
    
    pisovol->ISOVOL_iDrv     = iBlkdIndex;                              /*  记录驱动位置                */
    pisovol->ISOVAL_bValid   = LW_TRUE;                                 /*  卷有效                      */
    pisovol->ISOVOL_hVolLock = API_SemaphoreMCreate("iso9660vol_lock",
                               LW_PRIO_DEF_CEILING,
                               LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                               LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                               LW_NULL);
    if (!pisovol->ISOVOL_hVolLock) {                                    /*  无法创建卷锁                */
        iErrLevel = 2;
        goto    __error_handle;
    }
    pisovol->ISOVOL_plineFdNodeHeader = LW_NULL;                        /*  没有文件被打开              */
    pisovol->ISOVOL_uiTime = lib_time(LW_NULL);                         /*  获得当前时间                */

    pisovol->ISOVOL_iso9660Vol = iso9660_open_ext(iBlkdIndex, ISO_EXTENSION_ALL);
    if (!pisovol->ISOVOL_iso9660Vol) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "iso9660 driver table full.\r\n");
        ulError = ERROR_IOS_DRIVER_GLUT;
        iErrLevel = 3;
        goto    __error_handle;
    }
    
    pisovol->ISOVOL_iFlag = pblkd->BLKD_iFlag;

    if (iosDevAddEx(&pisovol->ISOVOL_devhdrHdr, pcName, _G_iIsoDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  安装文件系统设备            */
        ulError = API_GetLastError();
        iErrLevel = 4;
        goto    __error_handle;
    }
    __fsDiskLinkCounterAdd(pblkd);                                      /*  增加块设备链接              */
    
    __blockIoDevIoctl(iBlkdIndex, LW_BLKD_CTRL_POWER, LW_BLKD_POWER_ON);/*  打开电源                    */
    __blockIoDevReset(iBlkdIndex);                                      /*  复位磁盘接口                */
    __blockIoDevIoctl(iBlkdIndex, FIODISKINIT, 0);                      /*  初始化磁盘                  */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "disk \"%s\" mount ok.\r\n", pcName);
    
    return  (ERROR_NONE);
    
    /* 
     *  ERROR OCCUR
     */
__error_handle:
    if (iErrLevel > 3) {
        iso9660_close(pisovol->ISOVOL_iso9660Vol);                      /*  卸载挂载的文件系统          */
    }
    if (iErrLevel > 2) {
        API_SemaphoreMDelete(&pisovol->ISOVOL_hVolLock);
    }
    if (iErrLevel > 1) {
        __blockIoDevDelete(iBlkdIndex);
    }
    __SHEAP_FREE(pisovol);
    
    _ErrorHandle(ulError);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_Iso9660FsDevDelete
** 功能描述: 删除一个 ISO 设备, 例如: API_Iso9660FsDevDelete("/mnt/ata0");
** 输　入  : pcName            设备名(设备挂接的节点地址)
** 输　出  : < 0 表示失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_Iso9660FsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  如果是设备, 这里就卸载设备  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __iso9660FsGetInfo
** 功能描述: ISO9660 FS 获得临时的基本属性
** 输　入  : pstat            iso9660文件
**           pmode            64bit 文件属性
** 输　出  : ERROR or NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __iso9660FsGetInfo (iso9660_stat_t  *pstat, mode_t  *pmode)
{
    if (pstat == LW_NULL || pstat->type != _STAT_DIR) {                 /*  普通文件                    */
        *pmode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH |                /*  临时设置为所有权限都支持    */
                 S_IXUSR | S_IXGRP | S_IXOTH;
    } else {                                                            /*  目录文件                    */
        *pmode = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH |                /*  临时设置为所有权限都支持    */
                 S_IXUSR | S_IXGRP | S_IXOTH;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsOpen
** 功能描述: ISO9660 FS open 操作
** 输　入  : pisovol          卷控制块
**           pcName           文件名
**           iFlags           方式
**           iMode            方法
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  __iso9660FsOpen (PISO_VOLUME     pisovol,
                              PCHAR           pcName,
                              INT             iFlags,
                              INT             iMode)
{
    REGISTER PISO_FILE_T    pisofile;
    REGISTER ULONG          ulError;
             PLW_FD_NODE    pfdnode;
             BOOL           bIsNew;

    if (pcName == LW_NULL) {                                            /*  无文件名                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & (O_CREAT | O_TRUNC | O_RDWR | O_WRONLY)) {
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }

        pisofile = (PISO_FILE_T)__SHEAP_ALLOC(sizeof(ISO_FILE_T) +
                                              lib_strlen(pcName));      /*  创建文件内存                */
        if (pisofile == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_strcpy(pisofile->ISOFIL_cName, pcName);                     /*  记录文件名                  */
    
        pisofile->ISOFIL_pisovol = pisovol;                             /*  记录卷信息                  */
        
        ulError = __ISO_FILE_LOCK(pisofile);
        if ((pisovol->ISOVAL_bValid == LW_FALSE) ||
            (ulError != ERROR_NONE)) {                                  /*  卷正在被卸载                */
            __ISO_FILE_UNLOCK(pisofile);
            __SHEAP_FREE(pisofile);
            _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
            return  (PX_ERROR);
        }

        pisofile->ISOFIL_pstat = iso9660_ifs_stat_translate(pisovol->ISOVOL_iso9660Vol, pcName);
        if (!pisofile->ISOFIL_pstat) {
            __ISO_FILE_UNLOCK(pisofile);
            __SHEAP_FREE(pisofile);
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);

        }  else if (__STR_IS_ROOT(pcName)) {
            pisofile->ISOFIL_iFileType = __ISO_FILE_TYPE_DEV;

        }  else if (pisofile->ISOFIL_pstat->type == _STAT_DIR) {
            pisofile->ISOFIL_iFileType = __ISO_FILE_TYPE_DIR;

        } else {
            pisofile->ISOFIL_iFileType = __ISO_FILE_TYPE_NODE;
            if (iFlags & O_DIRECTORY) {
                iso9660_stat_free(pisofile->ISOFIL_pstat);
                __ISO_FILE_UNLOCK(pisofile);
                __SHEAP_FREE(pisofile);
                _ErrorHandle(ENOTDIR);
                return  (PX_ERROR);
            }
        }
    }

    __iso9660FsGetInfo(pisofile->ISOFIL_pstat, &iMode);

    pfdnode = API_IosFdNodeAdd(&pisovol->ISOVOL_plineFdNodeHeader,
                               (dev_t)pisovol->ISOVOL_iso9660Vol,
                               (ino64_t)(pisofile->ISOFIL_pstat != LW_NULL ?
                               (ino64_t)pisofile->ISOFIL_pstat->lsn : 1),
                               iFlags, iMode, 0,
                               0, pisofile->ISOFIL_pstat->size,
                               (PVOID)pisofile,
                               &bIsNew);                                /*  添加文件节点                */
    if (pfdnode == LW_NULL) {                                           /*  无法创建 fd_node 节点       */
        iso9660_stat_free(pisofile->ISOFIL_pstat);
        __ISO_FILE_UNLOCK(pisofile);
        __SHEAP_FREE(pisofile);
        return  (PX_ERROR);
    }

    LW_DEV_INC_USE_COUNT(&pisovol->ISOVOL_devhdrHdr);                   /*  更新计数器                  */

    if (bIsNew == LW_FALSE) {                                           /*  有重复打开                  */
        iso9660_stat_free(pisofile->ISOFIL_pstat);
        __ISO_FILE_UNLOCK(pisofile);
        __SHEAP_FREE(pisofile);
    
    } else {
        __ISO_FILE_UNLOCK(pisofile);
    }

    return  ((LONG)pfdnode);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsRemove
** 功能描述: ISO9660 FS remove 操作
** 输　入  : pisovol          卷控制块
**           pcName           文件名
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __iso9660FsRemove (PISO_VOLUME     pisovol,
                               PCHAR           pcName)
{
    REGISTER ULONG          ulError = ERROR_NONE;
             PLW_BLK_DEV    pblkd;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);

    } else {
        if (__STR_IS_ROOT(pcName)) {                                    /*  根目录或者设备文件          */
            ulError = __ISO_VOL_LOCK(pisovol);
            if (ulError) {
                _ErrorHandle(ENXIO);
                return  (PX_ERROR);                                     /*  正在被其他任务卸载          */
            }

            if (pisovol->ISOVAL_bValid == LW_FALSE) {
                __ISO_VOL_UNLOCK(pisovol);
                return  (ERROR_NONE);                                   /*  正在被其他任务卸载          */
            }

__re_umount_vol:
            if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pisovol)) {          /*  检查是否有正在工作的文件    */
                if (!pisovol->ISOVOL_bForceDelete) {
                    __ISO_VOL_UNLOCK(pisovol);
                    _ErrorHandle(EBUSY);
                    return  (PX_ERROR);                                 /*  有文件打开, 不能被卸载      */
                }

                pisovol->ISOVAL_bValid = LW_FALSE;                      /*  开始卸载卷, 文件再无法打开  */

                __ISO_VOL_UNLOCK(pisovol);

                _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
                iosDevFileAbnormal(&pisovol->ISOVOL_devhdrHdr);         /*  将所有相关文件设为异常状态  */

                __ISO_VOL_LOCK(pisovol);
                goto    __re_umount_vol;

            } else {
                pisovol->ISOVAL_bValid = LW_FALSE;                      /*  开始卸载卷, 文件再无法打开  */
            }

            /*
             *  不管出现什么物理错误, 必须可以卸载卷, 这里不再判断错误.
             */
            __blockIoDevIoctl(pisovol->ISOVOL_iDrv, LW_BLKD_CTRL_POWER,
                              LW_BLKD_POWER_OFF);                       /*  设备断电                    */
            __blockIoDevIoctl(pisovol->ISOVOL_iDrv, LW_BLKD_CTRL_EJECT,
                              0);                                       /*  将设备弹出                  */

            pblkd = __blockIoDevGet(pisovol->ISOVOL_iDrv);              /*  获得块设备控制块            */
            if (pblkd) {
                __fsDiskLinkCounterDec(pblkd);                          /*  减少链接次数                */
            }

            iosDevDelete((LW_DEV_HDR *)pisovol);                        /*  IO 系统移除设备             */

            iso9660_close(pisovol->ISOVOL_iso9660Vol);
            __blockIoDevIoctl(pisovol->ISOVOL_iDrv,
                              FIOUNMOUNT, 0);                           /*  执行底层脱离任务            */
            __blockIoDevDelete(pisovol->ISOVOL_iDrv);                   /*  在驱动表中移除              */

            API_SemaphoreMDelete(&pisovol->ISOVOL_hVolLock);            /*  删除卷锁                    */

            __SHEAP_FREE(pisovol);                                      /*  释放卷控制块                */

            _DebugHandle(__LOGMESSAGE_LEVEL, "disk unmount ok.\r\n");

            return  (ERROR_NONE);

        } else {                                                        /*  删除文件或目录              */
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }
}
/*********************************************************************************************************
** 函数名称: __iso9660FsClose
** 功能描述: ISO9660 FS close 操作
** 输　入  : pfdentry         文件控制块
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __iso9660FsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T   pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    PISO_VOLUME   pisovol  = pisofile->ISOFIL_pisovol;
    BOOL          bFree    = LW_FALSE;
    BOOL          bRemove  = LW_FALSE;

    if (pisofile) {
        if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
            _ErrorHandle(ENXIO);                                        /*  设备出错                    */
            return  (PX_ERROR);
        }
        
        if (API_IosFdNodeDec(&pisovol->ISOVOL_plineFdNodeHeader,
                             pfdnode, &bRemove) == 0) {                 /*  fd_node 是否完全释放        */
            iso9660_stat_free(pisofile->ISOFIL_pstat);
            bFree = LW_TRUE;
        }
        
        LW_DEV_DEC_USE_COUNT(&pisovol->ISOVOL_devhdrHdr);               /*  更新计数器                  */
        
        __ISO_FILE_UNLOCK(pisofile);
        
        if (bFree) {
            __SHEAP_FREE(pisofile);                                     /*  释放内存                    */
        }
        
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsRead
** 功能描述: ISO9660 FS read 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  __iso9660FsRead (PLW_FD_ENTRY   pfdentry,
                          PCHAR          pcBuffer,
                          size_t         stMaxBytes)
{
    PLW_FD_NODE   pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T   pisofile  = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    PISO_VOLUME   pisovol   = pisofile->ISOFIL_pisovol;
    UINT          uiReadNum = 0;
    UINT          uiRead;
    off_t         offPos;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pisofile->ISOFIL_iFileType != __ISO_FILE_TYPE_NODE) {
        __ISO_FILE_UNLOCK(pisofile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes) {
        for (offPos = pfdentry->FDENTRY_oftPtr;
             offPos < pisofile->ISOFIL_pstat->size && uiReadNum < stMaxBytes;
             offPos += uiRead) {
            if (ISO_BLOCKSIZE != iso9660_iso_seek_read(pisovol->ISOVOL_iso9660Vol,
                                                       pisofile->ISOFIL_ucBuff,
                                                       pisofile->ISOFIL_pstat->lsn +
                                                       (offPos >> ISO_BLOCK_SHIFT), 1)) {
                __ISO_FILE_UNLOCK(pisofile);
                _ErrorHandle(EIO);
                return  (PX_ERROR);
            }

            uiRead = min((stMaxBytes - uiReadNum), ISO_BLOCKSIZE);
            uiRead = min((pisofile->ISOFIL_pstat->size - offPos), uiRead);
            uiRead = min((ISO_BLOCKSIZE - (offPos & ISO_BLOCK_MASK)), uiRead);
            lib_memcpy(pcBuffer + uiReadNum,
                       &pisofile->ISOFIL_ucBuff[offPos & ISO_BLOCK_MASK],
                       uiRead);
            uiReadNum += uiRead;
        }
    }

    pfdentry->FDENTRY_oftPtr += uiReadNum;

    __ISO_FILE_UNLOCK(pisofile);
    
    return  ((ssize_t)uiReadNum);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsPRead
** 功能描述: ISO9660 FS pread 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  __iso9660FsPRead (PLW_FD_ENTRY  pfdentry,
                           PCHAR         pcBuffer,
                           size_t        stMaxBytes,
                           off_t         oftPos)
{
    PLW_FD_NODE   pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T   pisofile  = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    PISO_VOLUME   pisovol   = pisofile->ISOFIL_pisovol;
    UINT          uiReadNum = 0;
    UINT          uiRead;
    off_t         offPos;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pisofile->ISOFIL_iFileType != __ISO_FILE_TYPE_NODE) {
        __ISO_FILE_UNLOCK(pisofile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes) {
        for (offPos = pfdentry->FDENTRY_oftPtr;
             offPos < pisofile->ISOFIL_pstat->size && uiReadNum < stMaxBytes;
             offPos += uiRead) {
            if (ISO_BLOCKSIZE != iso9660_iso_seek_read(pisovol->ISOVOL_iso9660Vol,
                                                       pisofile->ISOFIL_ucBuff,
                                                       pisofile->ISOFIL_pstat->lsn +
                                                       (offPos >> ISO_BLOCK_SHIFT), 1)) {
                __ISO_FILE_UNLOCK(pisofile);
                _ErrorHandle(EIO);
                return  (PX_ERROR);
            }

            uiRead = min((stMaxBytes - uiReadNum), ISO_BLOCKSIZE);
            uiRead = min((pisofile->ISOFIL_pstat->size - offPos), uiRead);
            uiRead = min((ISO_BLOCKSIZE - (offPos & ISO_BLOCK_MASK)), uiRead);
            lib_memcpy(pcBuffer + uiReadNum,
                       &pisofile->ISOFIL_ucBuff[offPos & ISO_BLOCK_MASK],
                       uiRead);
            uiReadNum += uiRead;
        }
    }

    __ISO_FILE_UNLOCK(pisofile);
    
    return  ((ssize_t)uiReadNum);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsWrite
** 功能描述: ISO9660 FS write 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __iso9660FsWrite (PLW_FD_ENTRY  pfdentry,
                                  PCHAR         pcBuffer,
                                  size_t        stNBytes)
{
    _ErrorHandle(EROFS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsPWrite
** 功能描述: ISO9660 FS pwrite 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __iso9660FsPWrite (PLW_FD_ENTRY  pfdentry,
                                   PCHAR         pcBuffer,
                                   size_t        stNBytes,
                                   off_t         oftPos)
{
    _ErrorHandle(EROFS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsSeek
** 功能描述: ISO9660 FS 文件定位
** 输　入  : pfdentry            文件控制块
**           oftPos              定位点
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __iso9660FsSeek (PLW_FD_ENTRY  pfdentry, off_t  oftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T    pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    if (oftPos > pfdnode->FDNODE_oftSize) {                             /*  只读文件系统不能扩大文件    */
        __ISO_FILE_UNLOCK(pisofile);
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    }

    if (pisofile->ISOFIL_iFileType == __ISO_FILE_TYPE_NODE) {
        if (oftPos < 0) {
            ulError = EOVERFLOW;
            iError  = PX_ERROR;
        
        } else {
            pfdentry->FDENTRY_oftPtr = oftPos;
        }
    
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __ISO_FILE_UNLOCK(pisofile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsWhere
** 功能描述: ISO9660 FS 获得文件当前读写指针位置 (使用参数作为返回值, 与 FIOWHERE 的要求稍有不同)
** 输　入  : pfdentry            文件控制块
**           poftPos             读写指针位置
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __iso9660FsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T    pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (pisofile->ISOFIL_iFileType == __ISO_FILE_TYPE_NODE) {
        *poftPos = pfdentry->FDENTRY_oftPtr;
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __ISO_FILE_UNLOCK(pisofile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsNRead
** 功能描述: ISO9660 FS 获得文件剩余的信息量
** 输　入  : pfdentry            文件控制块
**           piPos               剩余数据量
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __iso9660FsNRead (PLW_FD_ENTRY  pfdentry, INT  *piPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T    pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (piPos == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (pisofile->ISOFIL_iFileType == __ISO_FILE_TYPE_NODE) {
        *piPos = (INT)(pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr);
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __ISO_FILE_UNLOCK(pisofile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsNRead64
** 功能描述: ISO9660 FS 获得文件剩余的信息量
** 输　入  : pfdentry            文件控制块
**           poftPos             剩余数据量
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __iso9660FsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T      pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    if (pisofile->ISOFIL_iFileType == __ISO_FILE_TYPE_NODE) {
        *poftPos = pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr;
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __ISO_FILE_UNLOCK(pisofile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsVolLabel
** 功能描述: ISO9660 FS 文件系统卷标处理函数
** 输　入  : pfdentry            文件控制块
**           pcLabel             卷标缓存
**           bSet                设置还是获取
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __iso9660FsVolLabel (PLW_FD_ENTRY  pfdentry, PCHAR  pcLabel, BOOL  bSet)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T    pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    PCHAR          pcVolId;
    ULONG          ulError;
    
    if (pcLabel == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    if (iso9660_ifs_get_volume_id(pisofile->ISOFIL_pisovol->ISOVOL_iso9660Vol, &pcVolId)) {
        lib_strcpy(pcLabel, pcVolId);
        cdio_free(pcVolId);
        ulError = ERROR_NONE;

    } else {
        lib_strcpy(pcLabel, "");
        ulError = EIO;
    }

    __ISO_FILE_UNLOCK(pisofile);
    
    _ErrorHandle(ulError);
    if (ulError) {
        return  (PX_ERROR);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __iso9660FsStatGet
** 功能描述: ISO9660 FS 获得文件状态及属性
** 输　入  : pfdentry            文件控制块
**           pstat               stat 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __iso9660FsStatGet (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T    pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    iso9660_t     *piso     = pisofile->ISOFIL_pisovol->ISOVOL_iso9660Vol;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    struct tm      tmIso;
    mode_t         mode;

    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    if (__STR_IS_ROOT(pisofile->ISOFIL_cName)) {                        /*  为根目录                    */
        pstat->st_dev     = LW_DEV_MAKE_STDEV(pfdentry->FDENTRY_pdevhdrHdr);
        pstat->st_ino     = 0;
        pstat->st_mode    = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH;
        pstat->st_nlink   = 0;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        iso9660_get_ltime(&piso->pvd.creation_date, &tmIso);
        pstat->st_ctime   = lib_mktime(&tmIso);
        iso9660_get_ltime(&piso->pvd.modification_date, &tmIso);
        pstat->st_mtime   = lib_mktime(&tmIso);
        iso9660_get_ltime(&piso->pvd.expiration_date, &tmIso);
        pstat->st_atime   = lib_mktime(&tmIso);
        pstat->st_blksize = piso->pvd.logical_block_size;
        pstat->st_blocks  = 0;

    } else {
        __iso9660FsGetInfo(pisofile->ISOFIL_pstat, &mode);
        pstat->st_dev     = LW_DEV_MAKE_STDEV(pfdentry->FDENTRY_pdevhdrHdr);
        pstat->st_ino     = pisofile->ISOFIL_pstat->lsn;
        pstat->st_mode    = mode;
        pstat->st_nlink   = 0;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = pisofile->ISOFIL_pstat->size;
        pstat->st_atime   = lib_mktime(&pisofile->ISOFIL_pstat->tm);
        pstat->st_mtime   = lib_mktime(&pisofile->ISOFIL_pstat->tm);
        pstat->st_ctime   = lib_mktime(&pisofile->ISOFIL_pstat->tm);
        pstat->st_blksize = (blksize_t)piso->i_framesize;
        pstat->st_blocks  = 0;
    }

    __ISO_FILE_UNLOCK(pisofile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsStatfsGet
** 功能描述: ISO9660 FS 获得文件系统状态及属性
** 输　入  : pfdentry            文件控制块
**           pstatfs             statfs 结构
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __iso9660FsStatfsGet (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T    pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    iso9660_t     *piso     = pisofile->ISOFIL_pisovol->ISOVOL_iso9660Vol;
    INT            iError   = ERROR_NONE;

    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }

    pstatfs->f_bsize  = (long)piso->i_framesize;
    pstatfs->f_blocks = (long)iso9660_get_pvd_space_size(&piso->pvd);
    pstatfs->f_bfree  = 0;
    pstatfs->f_bavail = 0;
    pstatfs->f_flag   = ST_RDONLY;

    __ISO_FILE_UNLOCK(pisofile);

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsReadDir
** 功能描述: ISO9660 FS 获得指定目录信息
** 输　入  : pfdentry            文件控制块
**           dir                 目录结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __iso9660FsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T    pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    iso9660_t     *piso     = pisofile->ISOFIL_pisovol->ISOVOL_iso9660Vol;

    CdioISO9660FileList_t *pfilelist;
    CdioListNode_t        *pnode;
    iso9660_stat_t        *pstat;
    INT                    i    = 0;

    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (pisofile->ISOFIL_iFileType == __ISO_FILE_TYPE_NODE) {
        __ISO_FILE_UNLOCK(pisofile);
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }

    pfilelist = iso9660_ifs_readdir(piso, pisofile->ISOFIL_cName);
    if (LW_NULL == pfilelist) {
        __ISO_FILE_UNLOCK(pisofile);
        _ErrorHandle(EFTYPE);
        return  (PX_ERROR);
    }
    
    if (dir->dir_pos >= _cdio_list_length(pfilelist)) {
        iso9660_filelist_free(pfilelist);
        _ErrorHandle(ENOENT);
        __ISO_FILE_UNLOCK(pisofile);
        return  (PX_ERROR);
    }

    _CDIO_LIST_FOREACH (pnode, pfilelist) {
        pstat = _cdio_list_node_data(pnode);
        if (i < dir->dir_pos) {
            i++;
            continue;
        }

        if ((lib_strcmp(pstat->filename, ".") == 0) ||
            (lib_strcmp(pstat->filename, "..") == 0)) {
            i++;
            dir->dir_pos++;
            continue;
        }

        dir->dir_pos++;
        lib_strcpy(dir->dir_dirent.d_name, pstat->filename);
        dir->dir_dirent.d_shortname[0] = PX_EOS;
        if (pstat->type == _STAT_DIR) {
            dir->dir_dirent.d_type = IFTODT(S_IFDIR);
        } else {
            dir->dir_dirent.d_type = IFTODT(S_IFREG);
        }
        break;
    }

    iso9660_filelist_free(pfilelist);

    __ISO_FILE_UNLOCK(pisofile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __iso9660FsIoctl
** 功能描述: ISO9660 FS ioctl 操作
** 输　入  : pfdentry            文件控制块
**           request,            命令
**           arg                 命令参数
** 输　出  : 驱动相关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __iso9660FsIoctl (PLW_FD_ENTRY  pfdentry,
                              INT           iRequest,
                              LONG          lArg)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T      pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    INT            iError;
    off_t          oftTemp  = 0;                                        /*  临时变量                    */

    switch (iRequest) {

    case FIOCONTIG:
    case FIOATTRIBSET:
    case FIODISKFORMAT:                                                 /*  卷格式化                    */
    case FIOTIMESET:                                                    /*  设置文件时间                */
    case FIOTRUNC:                                                      /*  改变文件大小                */
    case FIORENAME:                                                     /*  文件重命名                  */
    case FIOCHMOD:
    case FIOLABELSET:                                                   /*  设置卷标                    */
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    
    case FIODISKINIT:                                                   /*  磁盘初始化                  */
        return  (__blockIoDevIoctl(pisofile->ISOFIL_pisovol->ISOVOL_iDrv,
                                   FIODISKINIT, lArg));
    
    /*
     *  FISOEEK, FIOWHERE is 64 bit operate.
     */
    case FIOSEEK:                                                       /*  文件重定位                  */
        oftTemp = *(off_t *)lArg;
        return  (__iso9660FsSeek(pfdentry, oftTemp));
    
    case FIOWHERE:                                                      /*  获得文件当前读写指针        */
        iError = __iso9660FsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
    
    case FIONREAD:                                                      /*  获得文件剩余字节数          */
        return  (__iso9660FsNRead(pfdentry, (INT *)lArg));
    
    case FIONREAD64:                                                    /*  获得文件剩余字节数          */
        iError = __iso9660FsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIOSYNC:                                                       /*  将文件缓存回写              */
    case FIODATASYNC:
    case FIOFLUSH:
        return  (ERROR_NONE);

    case FIOLABELGET:                                                   /*  获取卷标                    */
        return  (__iso9660FsVolLabel(pfdentry, (PCHAR)lArg, LW_FALSE));
    
    case FIOFSTATGET:                                                   /*  获得文件状态                */
        return  (__iso9660FsStatGet(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  获得文件系统状态            */
        return  (__iso9660FsStatfsGet(pfdentry, (struct statfs *)lArg));
        
    case FIOREADDIR:                                                    /*  获取一个目录信息            */
        return  (__iso9660FsReadDir(pfdentry, (DIR *)lArg));
        
    case FIOSETFL:                                                      /*  设置新的 flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);
        
    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
        *(PCHAR *)lArg = "ISO9660 CD-ROM FileSystem";
        return  (ERROR_NONE);
        
    case FIOGETFORCEDEL:                                                /*  强制卸载设备是否被允许      */
        *(BOOL *)lArg = pisofile->ISOFIL_pisovol->ISOVOL_bForceDelete;
        return  (ERROR_NONE);
        
    case FIOSETFORCEDEL:                                                /*  设置强制卸载使能            */
        pisofile->ISOFIL_pisovol->ISOVOL_bForceDelete = (BOOL)lArg;
        return  (ERROR_NONE);
        
    case FIOFSGETFL:                                                    /*  获取文件系统权限            */
        if ((INT *)lArg == LW_NULL) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        *(INT *)lArg = pisofile->ISOFIL_pisovol->ISOVOL_iFlag;
        return  (ERROR_NONE);

    case FIOFSSETFL:                                                    /*  设置文件系统权限            */
        if ((INT)lArg != O_RDONLY) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        pisofile->ISOFIL_pisovol->ISOVOL_iFlag = (INT)lArg;
        KN_SMP_WMB();
        return  (ERROR_NONE);

#if LW_CFG_FS_SELECT_EN > 0
    case FIOSELECT:
        if (((PLW_SEL_WAKEUPNODE)lArg)->SELWUN_seltypType != SELEXCEPT) {
            SEL_WAKE_UP((PLW_SEL_WAKEUPNODE)lArg);                      /*  唤醒节点                    */
        }
        return  (ERROR_NONE);

    case FIOUNSELECT:
        if (((PLW_SEL_WAKEUPNODE)lArg)->SELWUN_seltypType != SELEXCEPT) {
            LW_SELWUN_SET_READY((PLW_SEL_WAKEUPNODE)lArg);              /*  设置为允许                  */
        }
        return  (ERROR_NONE);
#endif

    default:                                                            /*  无法识别的命令              */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    return  (PX_ERROR);
}

#endif                                                                  /*  (LW_CFG_ISO9660FS_EN > 0)   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
