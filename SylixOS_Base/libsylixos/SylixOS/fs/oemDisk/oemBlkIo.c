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
** ��   ��   ��: oemBlkIo.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 10 �� 14 ��
**
** ��        ��: OEM �����Զ����� /dev/blk/xxx �ļ�.
**
** ע        ��: blk io �ļ����뵥�̶߳�д, �Ƕ��̰߳�ȫ.
**
** BUG:
2018.06.11  �жδ���ʱ, ��������������д, ���Ч��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_OEMDISK_EN > 0
/*********************************************************************************************************
  �ṹ
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR              BLKIO_devhdrHdr;                            /*  I/O ϵͳ�ӿ��豸ͷ          */
    LW_LIST_LINE            BLKIO_lineManage;
    PLW_BLK_DEV             BLKIO_pblkDev;
    CPVOID                  BLKIO_pvOemDisk;

    ULONG                   BLKIO_ulSecSize;
    INT                     BLKIO_ulSecShift;
    ULONG                   BLKIO_ulNSec;

    INT                     BLKIO_iFlags;
    off_t                   BLKIO_oftPtr;
    off_t                   BLKIO_oftSize;
    time_t                  BLKIO_timeCreate;

    UINT8                  *BLKIO_pucSec;
} LW_BLKIO_DEV;
typedef LW_BLKIO_DEV       *PLW_BLKIO_DEV;
/*********************************************************************************************************
  �ڲ�ȫ�ֱ���
*********************************************************************************************************/
static INT                  _G_iBlkIoDrvNum = PX_ERROR;
static LW_LIST_LINE_HEADER  _G_plineBlkIo   = LW_NULL;
static LW_OBJECT_HANDLE     _G_ulBlkIoLock;
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define BLKIO_LOCK()                API_SemaphoreMPend(_G_ulBlkIoLock, LW_OPTION_WAIT_INFINITE)
#define BLKIO_UNLOCK()              API_SemaphoreMPost(_G_ulBlkIoLock)

#define BLKIO_READ(buf, sec, n)     (pblkDev->BLKD_pfuncBlkRd(pblkDev, buf, sec, n))
#define BLKIO_WRITE(buf, sec, n)    (pblkDev->BLKD_pfuncBlkWrt(pblkDev, buf, sec, n))
/*********************************************************************************************************
** ��������: __blkIoFsOpen
** ��������: �� blk io �豸 (ֻ�ܴ�һ��)
** �䡡��  : pdevblk                       blk io �豸
**           pcName                        �豸��
**           iFlags                        O_RDONLY / O_WRONLY / O_RDWR ...
**           iMode                         ����
** �䡡��  : ���ƿ�ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  __blkIoFsOpen (PLW_BLKIO_DEV  pdevblk, PCHAR  pcName, INT  iFlags, INT  iMode)
{
    if (LW_DEV_INC_USE_COUNT(&pdevblk->BLKIO_devhdrHdr) == 1) {
        iFlags &= O_ACCMODE;
        if ((iFlags == O_WRONLY) || (iFlags == O_RDWR)) {
            if (__fsDiskLinkCounterGet(pdevblk->BLKIO_pblkDev)) {
                LW_DEV_DEC_USE_COUNT(&pdevblk->BLKIO_devhdrHdr);
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            }
        }
        pdevblk->BLKIO_iFlags = iFlags;
        pdevblk->BLKIO_oftPtr = 0;
        return  ((LONG)pdevblk);

    } else {
        LW_DEV_DEC_USE_COUNT(&pdevblk->BLKIO_devhdrHdr);
        _ErrorHandle(EBUSY);                                            /*  ֻ�����һ��              */
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __blkIoFsClose
** ��������: ��ȡ blk io �豸״̬
** �䡡��  : pdevblk                       blk io �豸
**           pcName                        �豸��
**           pstat                         �豸״̬
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __blkIoFsLStat (PLW_BLKIO_DEV  pdevblk, PCHAR  pcName, struct stat  *pstat)
{
    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pstat->st_dev     = LW_DEV_MAKE_STDEV(&pdevblk->BLKIO_devhdrHdr);
    pstat->st_ino     = (ino_t)0;
    pstat->st_mode    = (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IFBLK);
    pstat->st_nlink   = 1;
    pstat->st_uid     = 0;
    pstat->st_gid     = 0;
    pstat->st_rdev    = 0;
    pstat->st_size    = pdevblk->BLKIO_oftSize;
    pstat->st_blksize = pdevblk->BLKIO_ulSecSize;
    pstat->st_blocks  = pdevblk->BLKIO_ulNSec;

    pstat->st_atime   = pdevblk->BLKIO_timeCreate;
    pstat->st_mtime   = pdevblk->BLKIO_timeCreate;
    pstat->st_ctime   = pdevblk->BLKIO_timeCreate;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __blkIoFsClose
** ��������: �ر� blk io �豸
** �䡡��  : pdevblk,                      blk io �豸
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __blkIoFsClose (PLW_BLKIO_DEV  pdevblk)
{
    if (LW_DEV_GET_USE_COUNT(&pdevblk->BLKIO_devhdrHdr)) {
        LW_DEV_DEC_USE_COUNT(&pdevblk->BLKIO_devhdrHdr);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __blkIoFsPRead
** ��������: ��ȡ blk io �豸
** �䡡��  : pdevblk                       blk io �豸
**           pcName                        �豸��
**           stMaxBytes                    ��ȡ�ֽ���
**           oft                           �ļ�ƫ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __blkIoFsPRead (PLW_BLKIO_DEV  pdevblk, PCHAR  pcBuffer, size_t stMaxBytes, off_t oft)
{
    ULONG           ulStartSec, ulStartOff;
    ULONG           ulEndSec, ulEndOff;
    ULONG           ulSecMask = pdevblk->BLKIO_ulSecSize - 1;
    off_t           oftLeft   = pdevblk->BLKIO_oftSize - oft;
    PLW_BLK_DEV     pblkDev   = pdevblk->BLKIO_pblkDev;

    if (!pcBuffer || !stMaxBytes || (oft >= pdevblk->BLKIO_oftSize)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }

    if ((off_t)stMaxBytes > oftLeft) {
        stMaxBytes = (size_t)oftLeft;
    }

    if (stMaxBytes == 0) {
        return  (0);
    }

    ulStartSec = oft >> pdevblk->BLKIO_ulSecShift;
    ulStartOff = oft &  ulSecMask;

    ulEndSec = (oft + stMaxBytes) >> pdevblk->BLKIO_ulSecShift;
    ulEndOff = (oft + stMaxBytes) & ulSecMask;

    if (ulStartSec == ulEndSec) {
        if (BLKIO_READ(pdevblk->BLKIO_pucSec, ulStartSec, 1) < 0) {
            return  (PX_ERROR);
        }
        lib_memcpy(pcBuffer, pdevblk->BLKIO_pucSec + ulStartOff, (ulEndOff - ulStartOff));

    } else {
        if (ulStartOff) {
            if (BLKIO_READ(pdevblk->BLKIO_pucSec, ulStartSec, 1) < 0) {
                return  (PX_ERROR);
            }
            lib_memcpy(pcBuffer, pdevblk->BLKIO_pucSec + ulStartOff, pdevblk->BLKIO_ulSecSize - ulStartOff);
            pcBuffer += pdevblk->BLKIO_ulSecSize - ulStartOff;
            ulStartSec++;
        }

        if (ulEndSec > ulStartSec) {
            if (BLKIO_READ(pcBuffer, ulStartSec, ulEndSec - ulStartSec) < 0) {
                return  (PX_ERROR);
            }
            pcBuffer  += (pdevblk->BLKIO_ulSecSize * (ulEndSec - ulStartSec));
            ulStartSec = ulEndSec;
        }

        if (ulEndOff) {
            if (BLKIO_READ(pdevblk->BLKIO_pucSec, ulEndSec, 1) < 0) {
                return  (PX_ERROR);
            }
            lib_memcpy(pcBuffer, pdevblk->BLKIO_pucSec, ulEndOff);
        }
    }

    return  ((ssize_t)stMaxBytes);
}
/*********************************************************************************************************
** ��������: __blkIoFsRead
** ��������: ��ȡ blk io �豸
** �䡡��  : pdevblk                       blk io �豸
**           pcBuffer                      ������
**           stMaxBytes                    ��ȡ�ֽ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __blkIoFsRead (PLW_BLKIO_DEV  pdevblk, PCHAR  pcBuffer, size_t stMaxBytes)
{
    ssize_t   stRet;

    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (0);
    }

    stRet = __blkIoFsPRead(pdevblk, pcBuffer, stMaxBytes, pdevblk->BLKIO_oftPtr);
    if (stRet > 0) {
        pdevblk->BLKIO_oftPtr += stRet;
    }

    return  (stRet);
}
/*********************************************************************************************************
** ��������: __blkIoFsPWrite
** ��������: д blk io �豸
** �䡡��  : pdevblk                       blk io �豸
**           pcBuffer                      ������
**           stNBytes                      д���ֽ���
**           oft                           ƫ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __blkIoFsPWrite (PLW_BLKIO_DEV  pdevblk, PCHAR  pcBuffer, size_t stNBytes, off_t oft)
{
    ULONG           ulStartSec, ulStartOff;
    ULONG           ulEndSec, ulEndOff;
    ULONG           ulSecMask = pdevblk->BLKIO_ulSecSize - 1;
    off_t           oftLeft   = pdevblk->BLKIO_oftSize - oft;
    PLW_BLK_DEV     pblkDev   = pdevblk->BLKIO_pblkDev;

    if (!pcBuffer || !stNBytes || (oft >= pdevblk->BLKIO_oftSize)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }

    if ((off_t)stNBytes > oftLeft) {
        stNBytes = (size_t)oftLeft;
    }

    if (stNBytes == 0) {
        return  (0);
    }

    ulStartSec = oft >> pdevblk->BLKIO_ulSecShift;
    ulStartOff = oft &  ulSecMask;

    ulEndSec = (oft + stNBytes) >> pdevblk->BLKIO_ulSecShift;
    ulEndOff = (oft + stNBytes) & ulSecMask;

    if (ulStartSec == ulEndSec) {
        if (BLKIO_READ(pdevblk->BLKIO_pucSec, ulStartSec, 1) < 0) {
            return  (PX_ERROR);
        }
        lib_memcpy(pdevblk->BLKIO_pucSec + ulStartOff,
                   pcBuffer, (ulEndOff - ulStartOff));
        if (BLKIO_WRITE(pdevblk->BLKIO_pucSec, ulStartSec, 1) < 0) {
            return  (PX_ERROR);
        }

    } else {
        if (ulStartOff) {
            if (BLKIO_READ(pdevblk->BLKIO_pucSec, ulStartSec, 1) < 0) {
                return  (PX_ERROR);
            }
            lib_memcpy(pdevblk->BLKIO_pucSec + ulStartOff,
                       pcBuffer, pdevblk->BLKIO_ulSecSize - ulStartOff);
            if (BLKIO_WRITE(pdevblk->BLKIO_pucSec, ulStartSec, 1) < 0) {
                return  (PX_ERROR);
            }
            pcBuffer += pdevblk->BLKIO_ulSecSize - ulStartOff;
            ulStartSec++;
        }

        if (ulEndSec > ulStartSec) {
            if (BLKIO_WRITE(pcBuffer, ulStartSec, ulEndSec - ulStartSec) < 0) {
                return  (PX_ERROR);
            }
            pcBuffer  += (pdevblk->BLKIO_ulSecSize * (ulEndSec - ulStartSec));
            ulStartSec = ulEndSec;
        }

        if (ulEndOff) {
            if (BLKIO_READ(pdevblk->BLKIO_pucSec, ulEndSec, 1) < 0) {
                return  (PX_ERROR);
            }
            lib_memcpy(pdevblk->BLKIO_pucSec, pcBuffer, ulEndOff);
            if (BLKIO_WRITE(pdevblk->BLKIO_pucSec, ulEndSec, 1) < 0) {
                return  (PX_ERROR);
            }
        }
    }

    return  ((ssize_t)stNBytes);
}
/*********************************************************************************************************
** ��������: __blkIoFsWrite
** ��������: д blk io �豸
** �䡡��  : pdevblk                       blk io �豸
**           pcBuffer                      ������
**           stNBytes                      д���ֽ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __blkIoFsWrite (PLW_BLKIO_DEV  pdevblk, PCHAR  pcBuffer, size_t stNBytes)
{
    ssize_t   stRet;

    if (!pcBuffer || !stNBytes) {
        _ErrorHandle(EINVAL);
        return  (0);
    }

    stRet = __blkIoFsPWrite(pdevblk, pcBuffer, stNBytes, pdevblk->BLKIO_oftPtr);
    if (stRet > 0) {
        pdevblk->BLKIO_oftPtr += stRet;
    }

    return  (stRet);
}
/*********************************************************************************************************
** ��������: __blkIoFsIoctl
** ��������: ���� blk io �豸
** �䡡��  : pdevblk                       blk io �豸
**           iRequest                      ����
**           lArg                          ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __blkIoFsIoctl (PLW_BLKIO_DEV  pdevblk, INT  iRequest, LONG  lArg)
{
    off_t        oftTemp;
    struct stat *pstat;

    switch (iRequest) {

    case FIOSEEK:                                                       /*  �ļ��ض�λ                  */
        oftTemp = *(off_t *)lArg;
        if (oftTemp > pdevblk->BLKIO_oftSize) {
            _ErrorHandle(ENOSPC);
            return  (PX_ERROR);
        }
        pdevblk->BLKIO_oftPtr = oftTemp;
        return  (ERROR_NONE);

    case FIOWHERE:                                                      /*  ����ļ���ǰ��дָ��        */
        *(off_t *)lArg = pdevblk->BLKIO_oftPtr;
        return  (ERROR_NONE);

    case FIONREAD:                                                      /*  ����ļ�ʣ���ֽ���          */
        *(INT *)lArg = pdevblk->BLKIO_oftSize - pdevblk->BLKIO_oftPtr;
        return  (ERROR_NONE);

    case FIONREAD64:                                                    /*  ����ļ�ʣ���ֽ���          */
        *(off_t *)lArg = pdevblk->BLKIO_oftSize - pdevblk->BLKIO_oftPtr;
        return  (ERROR_NONE);

    case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
        pstat = (struct stat *)lArg;
        if (!pstat) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        pstat->st_dev     = LW_DEV_MAKE_STDEV(&pdevblk->BLKIO_devhdrHdr);
        pstat->st_ino     = (ino_t)0;
        pstat->st_mode    = (S_IRUSR | S_IWUSR | S_IFBLK);
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 0;
        pstat->st_size    = pdevblk->BLKIO_oftSize;
        pstat->st_blksize = pdevblk->BLKIO_ulSecSize;
        pstat->st_blocks  = pdevblk->BLKIO_ulNSec;

        pstat->st_atime   = pdevblk->BLKIO_timeCreate;
        pstat->st_mtime   = pdevblk->BLKIO_timeCreate;
        pstat->st_ctime   = pdevblk->BLKIO_timeCreate;

        return  (ERROR_NONE);

    case FIOSYNC:                                                       /*  ���ļ������д              */
    case FIOFLUSH:
    case FIODATASYNC:
    case FIOSYNCMETA:
        pdevblk->BLKIO_pblkDev->BLKD_pfuncBlkIoctl(pdevblk->BLKIO_pblkDev, FIOFLUSH);
        return  (ERROR_NONE);

    case FIOUNMOUNT:
    case FIODISKINIT:
    case LW_BLKD_CTRL_RESET:
        return  (ERROR_NONE);

    case FIOTRIM:
        pdevblk->BLKIO_pblkDev->BLKD_pfuncBlkIoctl(pdevblk->BLKIO_pblkDev, FIOTRIM, lArg);
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

    case LW_BLKD_GET_SECNUM:
        *(ULONG *)lArg = pdevblk->BLKIO_ulNSec;
        return  (ERROR_NONE);

    case LW_BLKD_GET_SECSIZE:
        *(ULONG *)lArg = pdevblk->BLKIO_ulSecSize;
        return  (ERROR_NONE);

    case LW_BLKD_GET_BLKSIZE:
        if (pdevblk->BLKIO_pblkDev->BLKD_ulBytesPerBlock) {
            *(ULONG *)lArg = pdevblk->BLKIO_pblkDev->BLKD_ulBytesPerBlock;
        } else {
            return  (pdevblk->BLKIO_pblkDev->BLKD_pfuncBlkIoctl(pdevblk->BLKIO_pblkDev,
                                                                LW_BLKD_GET_BLKSIZE, lArg));
        }
        return  (ERROR_NONE);

    case LW_BLKD_CTRL_STATUS:
        return  (pdevblk->BLKIO_pblkDev->BLKD_pfuncBlkStatusChk(pdevblk->BLKIO_pblkDev));
        
    case LW_BLKD_CTRL_OEMDISK:
        *(CPVOID *)lArg = pdevblk->BLKIO_pvOemDisk;
        return  (ERROR_NONE);
        
    case LW_BLKD_CTRL_INFO:
        return  (pdevblk->BLKIO_pblkDev->BLKD_pfuncBlkIoctl(pdevblk->BLKIO_pblkDev,
                                                            LW_BLKD_CTRL_INFO, lArg));

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __blkIoFsDrvInstall
** ��������: ��װ blk io ��������
** �䡡��  : NONE
** �䡡��  : ERROR or OK.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __blkIoFsDrvInstall (VOID)
{
    struct file_operations     fileop;

    if (_G_iBlkIoDrvNum > 0) {
        return  (ERROR_NONE);
    }

    if (_G_ulBlkIoLock == LW_OBJECT_HANDLE_INVALID) {
        _G_ulBlkIoLock =  API_SemaphoreMCreate("blkio_lock", LW_PRIO_DEF_CEILING,
                                               LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                               LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                               LW_NULL);
    }

    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner       = THIS_MODULE;
    fileop.fo_open     = __blkIoFsOpen;
    fileop.fo_close    = __blkIoFsClose;
    fileop.fo_read     = __blkIoFsRead;
    fileop.fo_read_ex  = __blkIoFsPRead;
    fileop.fo_write    = __blkIoFsWrite;
    fileop.fo_write_ex = __blkIoFsPWrite;
    fileop.fo_lstat    = __blkIoFsLStat;
    fileop.fo_ioctl    = __blkIoFsIoctl;

    _G_iBlkIoDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_ORIG);

    DRIVER_LICENSE(_G_iBlkIoDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iBlkIoDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iBlkIoDrvNum, "blk io driver.");

    return  ((_G_iBlkIoDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: API_OemBlkIoCreate
** ��������: ����һ�� blk io �豸
** �䡡��  : pcBlkDev           ��Ҫ���ɵĿ��豸�ļ� ����: /dev/blk/sata0
**           pblkdDisk          ���豸����
**           pvOemDisk          OEM DISK ��ؽṹ
** �䡡��  : ERROR or OK.
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_OemBlkIoCreate (CPCHAR  pcBlkDev, PLW_BLK_DEV  pblkdDisk, CPVOID  pvOemDisk)
{
    PLW_BLKIO_DEV  pdevblk;
    ULONG          ulSecSize = 0ul;
    ULONG          ulNSec    = 0ul;
    INT            iShift;

    if (!pcBlkDev || !pblkdDisk) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ulSecSize = pblkdDisk->BLKD_ulBytesPerSector;
    if (!ulSecSize) {
        if (pblkdDisk->BLKD_pfuncBlkIoctl(pblkdDisk,
                                          LW_BLKD_GET_SECSIZE,
                                          &ulSecSize) && !ulSecSize) {
            return  (PX_ERROR);
        }
    }

    ulNSec = pblkdDisk->BLKD_ulNSector;
    if (!ulNSec) {
        if (pblkdDisk->BLKD_pfuncBlkIoctl(pblkdDisk,
                                          LW_BLKD_GET_SECNUM,
                                          &ulNSec) && !ulNSec) {
            return  (PX_ERROR);
        }
    }

    iShift = archFindLsb(ulSecSize);
    if (iShift <= 0 ||  iShift > 16) {
        _ErrorHandle(E2BIG);
        return  (PX_ERROR);
    } else {
        iShift--;
    }

    pdevblk = (PLW_BLKIO_DEV)__SHEAP_ALLOC(sizeof(LW_BLKIO_DEV) + ulSecSize);
    if (pdevblk == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pdevblk, sizeof(LW_BLKIO_DEV));

    pdevblk->BLKIO_pblkDev    = pblkdDisk;
    pdevblk->BLKIO_pvOemDisk  = pvOemDisk;
    pdevblk->BLKIO_ulSecSize  = ulSecSize;
    pdevblk->BLKIO_ulSecShift = (ULONG)iShift;
    pdevblk->BLKIO_ulNSec     = ulNSec;

    pdevblk->BLKIO_iFlags     = 0;
    pdevblk->BLKIO_oftPtr     = 0;
    pdevblk->BLKIO_oftSize    = (off_t)ulNSec * ulSecSize;
    pdevblk->BLKIO_timeCreate = lib_time(LW_NULL);

    pdevblk->BLKIO_pucSec = ((UINT8 *)pdevblk) + sizeof(LW_BLKIO_DEV);

    if (iosDevAddEx(&pdevblk->BLKIO_devhdrHdr, pcBlkDev, _G_iBlkIoDrvNum, DT_BLK)
        != ERROR_NONE) {
        __SHEAP_FREE(pdevblk);
        return  (PX_ERROR);
    }

    BLKIO_LOCK();
    _List_Line_Add_Ahead(&pdevblk->BLKIO_lineManage, &_G_plineBlkIo);
    BLKIO_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_OemBlkIoDelete
** ��������: ɾ�� blk io �豸
** �䡡��  : pblkdDisk          ���豸����
** �䡡��  : ERROR_NONE.
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_OemBlkIoDelete (PLW_BLK_DEV  pblkdDisk)
{
    PLW_BLKIO_DEV  pdevblk;
    PLW_LIST_LINE  plineTemp;

    BLKIO_LOCK();
    for (plineTemp  = _G_plineBlkIo;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pdevblk = _LIST_ENTRY(plineTemp, LW_BLKIO_DEV, BLKIO_lineManage);
        if (pdevblk->BLKIO_pblkDev == pblkdDisk) {
            break;
        }
    }
    if (plineTemp == LW_NULL) {
        BLKIO_UNLOCK();
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }

    if (LW_DEV_GET_USE_COUNT(&pdevblk->BLKIO_devhdrHdr)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "remove block I/O device in busy status.\r\n");
        iosDevFileAbnormal(&pdevblk->BLKIO_devhdrHdr);
    }

    iosDevDelete(&pdevblk->BLKIO_devhdrHdr);
    _List_Line_Del(&pdevblk->BLKIO_lineManage, &_G_plineBlkIo);
    BLKIO_UNLOCK();

    __SHEAP_FREE(pdevblk);

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
