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
** ��   ��   ��: iso9660_sylixos.c
**
** ��   ��   ��: Tiger.Jiang (��̫��)
**
** �ļ���������: 2018 �� 09 �� 15 ��
**
** ��        ��: ISO9660 �ļ�ϵͳ�� IO ϵͳ�ӿڲ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/unique/unique.h"
/*********************************************************************************************************
  �ü���
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
  �ڲ��ṹ (�����ʹ�� inode ���ܻ��Լ�ڴ�, �ӿ�����ٶ�)
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          ISOVOL_devhdrHdr;                               /*  �豸ͷ                      */
    BOOL                ISOVOL_bForceDelete;                            /*  �Ƿ�����ǿ��ж�ؾ�          */
    iso9660_t          *ISOVOL_iso9660Vol;                              /*  �ļ�ϵͳ����Ϣ              */
    INT                 ISOVOL_iDrv;                                    /*  ������λ��                  */
    BOOL                ISOVAL_bValid;                                  /*  ��Ч�Ա�־                  */
    LW_OBJECT_HANDLE    ISOVOL_hVolLock;                                /*  �������                    */
    
    LW_LIST_LINE_HEADER ISOVOL_plineFdNodeHeader;                       /*  fd_node ����                */
    UINT32              ISOVOL_uiTime;                                  /*  ����ʱ�� ISO9660 ��ʽ     */
    INT                 ISOVOL_iFlag;                                   /*  O_RDONLY or O_RDWR          */
} ISO_VOLUME;
typedef ISO_VOLUME     *PISO_VOLUME;

typedef struct {
    PISO_VOLUME         ISOFIL_pisovol;                                 /*  ���ھ���Ϣ                  */
    iso9660_stat_t     *ISOFIL_pstat;
    INT                 ISOFIL_iFileType;                               /*  �ļ�����                    */
    UINT64              ISOFIL_u64Uniq;                                 /*  64bits �� + Ŀ¼ƫ��        */
    ino_t               ISOFIL_inode;                                   /*  inode ���                  */
    UCHAR               ISOFIL_ucBuff[ISO_BLOCKSIZE];                   /*  �ļ�����                    */
    CHAR                ISOFIL_cName[1];                                /*  �ļ���                      */
} ISO_FILE_T;
typedef ISO_FILE_T     *PISO_FILE_T;
/*********************************************************************************************************
  �ļ�����
*********************************************************************************************************/
#define __ISO_FILE_TYPE_NODE            0                               /*  open ���ļ�               */
#define __ISO_FILE_TYPE_DIR             1                               /*  open ��Ŀ¼               */
#define __ISO_FILE_TYPE_DEV             2                               /*  open ���豸               */
/*********************************************************************************************************
  ISO9660 ���豸�����ļ�ϵͳ����
*********************************************************************************************************/
static INT  _G_iIsoDrvNum = PX_ERROR;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
INT             __blockIoDevCreate(PLW_BLK_DEV  pblkdNew);
VOID            __blockIoDevDelete(INT  iIndex);
PLW_BLK_DEV     __blockIoDevGet(INT  iIndex);
INT             __blockIoDevReset(INT  iIndex);
INT             __blockIoDevIoctl(INT  iIndex, INT  iCmd, LONG  lArg);
INT             __blockIoDevIsLogic(INT  iIndex);
INT             __blockIoDevFlag(INT  iIndex);
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __ISO_FILE_LOCK(pisofile)   API_SemaphoreMPend(pisofile->ISOFIL_pisovol->ISOVOL_hVolLock, \
                                    LW_OPTION_WAIT_INFINITE)
#define __ISO_FILE_UNLOCK(pisofile) API_SemaphoreMPost(pisofile->ISOFIL_pisovol->ISOVOL_hVolLock)
#define __ISO_VOL_LOCK(pisovol)     API_SemaphoreMPend(pisovol->ISOVOL_hVolLock, LW_OPTION_WAIT_INFINITE)
#define __ISO_VOL_UNLOCK(pISOvol)   API_SemaphoreMPost(pisovol->ISOVOL_hVolLock)
/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)       ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
/*********************************************************************************************************
  ������������
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
  �ļ�ϵͳ��������
*********************************************************************************************************/
LW_API INT  API_Iso9660FsDevCreate(PCHAR   pcName, PLW_BLK_DEV  pblkd);
/*********************************************************************************************************
** ��������: API_Iso9660FsDrvInstall
** ��������: ��װ ISO9660 �ļ�ϵͳ��������
** �䡡��  : 
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
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
    
    _G_iIsoDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);       /*  ʹ�� NEW_1 ���豸����       */

    DRIVER_LICENSE(_G_iIsoDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iIsoDrvNum,      "Tiger.Jiang");
    DRIVER_DESCRIPTION(_G_iIsoDrvNum, "ISO9660 driver.");
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "ISO9660 file system installed.\r\n");
                                                                        /*  ע���ļ�ϵͳ                */
    __fsRegister("iso9660", API_Iso9660FsDevCreate, LW_NULL, (FUNCPTR)__iso9660FsProbe);
    
    return  ((_G_iIsoDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: __iso9660FsCheck
** ��������: �������Ƿ�Ϊ ISO9660 ��ʽ
** �䡡��  : pblkd             ���豸
**           pucPartType       ���ط�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: API_Iso9660FsDevCreate
** ��������: ����һ�� ISO �豸, ����: API_Iso9660FsDevCreate("/ata0", ...);
**           �� sylixos �� yaffs ��ͬ, ISO ÿһ�����Ƕ������豸.
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
**           pblkd             ���豸����
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
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

    if ((pblkd->BLKD_iLogic == 0) && (pblkd->BLKD_uiLinkCounter)) {     /*  �����豸�����ú��������  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "logic device has already mount.\r\n");
        _ErrorHandle(ERROR_IO_ACCESS_DENIED);
        return  (PX_ERROR);
    }
    
    iBlkdIndex = __blockIoDevCreate(pblkd);                             /*  ������豸������            */
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
    lib_bzero(pisovol, sizeof(ISO_VOLUME));                             /*  ��վ���ƿ�                */
    
    pisovol->ISOVOL_bForceDelete = LW_FALSE;                            /*  ������ǿ��ж�ؾ�            */
    
    pisovol->ISOVOL_iDrv     = iBlkdIndex;                              /*  ��¼����λ��                */
    pisovol->ISOVAL_bValid   = LW_TRUE;                                 /*  ����Ч                      */
    pisovol->ISOVOL_hVolLock = API_SemaphoreMCreate("iso9660vol_lock",
                               LW_PRIO_DEF_CEILING,
                               LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                               LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                               LW_NULL);
    if (!pisovol->ISOVOL_hVolLock) {                                    /*  �޷���������                */
        iErrLevel = 2;
        goto    __error_handle;
    }
    pisovol->ISOVOL_plineFdNodeHeader = LW_NULL;                        /*  û���ļ�����              */
    pisovol->ISOVOL_uiTime = lib_time(LW_NULL);                         /*  ��õ�ǰʱ��                */

    pisovol->ISOVOL_iso9660Vol = iso9660_open_ext(iBlkdIndex, ISO_EXTENSION_ALL);
    if (!pisovol->ISOVOL_iso9660Vol) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "iso9660 driver table full.\r\n");
        ulError = ERROR_IOS_DRIVER_GLUT;
        iErrLevel = 3;
        goto    __error_handle;
    }
    
    pisovol->ISOVOL_iFlag = pblkd->BLKD_iFlag;

    if (iosDevAddEx(&pisovol->ISOVOL_devhdrHdr, pcName, _G_iIsoDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  ��װ�ļ�ϵͳ�豸            */
        ulError = API_GetLastError();
        iErrLevel = 4;
        goto    __error_handle;
    }
    __fsDiskLinkCounterAdd(pblkd);                                      /*  ���ӿ��豸����              */
    
    __blockIoDevIoctl(iBlkdIndex, LW_BLKD_CTRL_POWER, LW_BLKD_POWER_ON);/*  �򿪵�Դ                    */
    __blockIoDevReset(iBlkdIndex);                                      /*  ��λ���̽ӿ�                */
    __blockIoDevIoctl(iBlkdIndex, FIODISKINIT, 0);                      /*  ��ʼ������                  */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "disk \"%s\" mount ok.\r\n", pcName);
    
    return  (ERROR_NONE);
    
    /* 
     *  ERROR OCCUR
     */
__error_handle:
    if (iErrLevel > 3) {
        iso9660_close(pisovol->ISOVOL_iso9660Vol);                      /*  ж�ع��ص��ļ�ϵͳ          */
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
** ��������: API_Iso9660FsDevDelete
** ��������: ɾ��һ�� ISO �豸, ����: API_Iso9660FsDevDelete("/mnt/ata0");
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_Iso9660FsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  ������豸, �����ж���豸  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __iso9660FsGetInfo
** ��������: ISO9660 FS �����ʱ�Ļ�������
** �䡡��  : pstat            iso9660�ļ�
**           pmode            64bit �ļ�����
** �䡡��  : ERROR or NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __iso9660FsGetInfo (iso9660_stat_t  *pstat, mode_t  *pmode)
{
    if (pstat == LW_NULL || pstat->type != _STAT_DIR) {                 /*  ��ͨ�ļ�                    */
        *pmode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH |                /*  ��ʱ����Ϊ����Ȩ�޶�֧��    */
                 S_IXUSR | S_IXGRP | S_IXOTH;
    } else {                                                            /*  Ŀ¼�ļ�                    */
        *pmode = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH |                /*  ��ʱ����Ϊ����Ȩ�޶�֧��    */
                 S_IXUSR | S_IXGRP | S_IXOTH;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __iso9660FsOpen
** ��������: ISO9660 FS open ����
** �䡡��  : pisovol          ����ƿ�
**           pcName           �ļ���
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
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

    if (pcName == LW_NULL) {                                            /*  ���ļ���                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & (O_CREAT | O_TRUNC | O_RDWR | O_WRONLY)) {
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }

        pisofile = (PISO_FILE_T)__SHEAP_ALLOC(sizeof(ISO_FILE_T) +
                                              lib_strlen(pcName));      /*  �����ļ��ڴ�                */
        if (pisofile == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_strcpy(pisofile->ISOFIL_cName, pcName);                     /*  ��¼�ļ���                  */
    
        pisofile->ISOFIL_pisovol = pisovol;                             /*  ��¼����Ϣ                  */
        
        ulError = __ISO_FILE_LOCK(pisofile);
        if ((pisovol->ISOVAL_bValid == LW_FALSE) ||
            (ulError != ERROR_NONE)) {                                  /*  �����ڱ�ж��                */
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
                               &bIsNew);                                /*  ����ļ��ڵ�                */
    if (pfdnode == LW_NULL) {                                           /*  �޷����� fd_node �ڵ�       */
        iso9660_stat_free(pisofile->ISOFIL_pstat);
        __ISO_FILE_UNLOCK(pisofile);
        __SHEAP_FREE(pisofile);
        return  (PX_ERROR);
    }

    LW_DEV_INC_USE_COUNT(&pisovol->ISOVOL_devhdrHdr);                   /*  ���¼�����                  */

    if (bIsNew == LW_FALSE) {                                           /*  ���ظ���                  */
        iso9660_stat_free(pisofile->ISOFIL_pstat);
        __ISO_FILE_UNLOCK(pisofile);
        __SHEAP_FREE(pisofile);
    
    } else {
        __ISO_FILE_UNLOCK(pisofile);
    }

    return  ((LONG)pfdnode);
}
/*********************************************************************************************************
** ��������: __iso9660FsRemove
** ��������: ISO9660 FS remove ����
** �䡡��  : pisovol          ����ƿ�
**           pcName           �ļ���
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
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
        if (__STR_IS_ROOT(pcName)) {                                    /*  ��Ŀ¼�����豸�ļ�          */
            ulError = __ISO_VOL_LOCK(pisovol);
            if (ulError) {
                _ErrorHandle(ENXIO);
                return  (PX_ERROR);                                     /*  ���ڱ���������ж��          */
            }

            if (pisovol->ISOVAL_bValid == LW_FALSE) {
                __ISO_VOL_UNLOCK(pisovol);
                return  (ERROR_NONE);                                   /*  ���ڱ���������ж��          */
            }

__re_umount_vol:
            if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pisovol)) {          /*  ����Ƿ������ڹ������ļ�    */
                if (!pisovol->ISOVOL_bForceDelete) {
                    __ISO_VOL_UNLOCK(pisovol);
                    _ErrorHandle(EBUSY);
                    return  (PX_ERROR);                                 /*  ���ļ���, ���ܱ�ж��      */
                }

                pisovol->ISOVAL_bValid = LW_FALSE;                      /*  ��ʼж�ؾ�, �ļ����޷���  */

                __ISO_VOL_UNLOCK(pisovol);

                _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
                iosDevFileAbnormal(&pisovol->ISOVOL_devhdrHdr);         /*  ����������ļ���Ϊ�쳣״̬  */

                __ISO_VOL_LOCK(pisovol);
                goto    __re_umount_vol;

            } else {
                pisovol->ISOVAL_bValid = LW_FALSE;                      /*  ��ʼж�ؾ�, �ļ����޷���  */
            }

            /*
             *  ���ܳ���ʲô�������, �������ж�ؾ�, ���ﲻ���жϴ���.
             */
            __blockIoDevIoctl(pisovol->ISOVOL_iDrv, LW_BLKD_CTRL_POWER,
                              LW_BLKD_POWER_OFF);                       /*  �豸�ϵ�                    */
            __blockIoDevIoctl(pisovol->ISOVOL_iDrv, LW_BLKD_CTRL_EJECT,
                              0);                                       /*  ���豸����                  */

            pblkd = __blockIoDevGet(pisovol->ISOVOL_iDrv);              /*  ��ÿ��豸���ƿ�            */
            if (pblkd) {
                __fsDiskLinkCounterDec(pblkd);                          /*  �������Ӵ���                */
            }

            iosDevDelete((LW_DEV_HDR *)pisovol);                        /*  IO ϵͳ�Ƴ��豸             */

            iso9660_close(pisovol->ISOVOL_iso9660Vol);
            __blockIoDevIoctl(pisovol->ISOVOL_iDrv,
                              FIOUNMOUNT, 0);                           /*  ִ�еײ���������            */
            __blockIoDevDelete(pisovol->ISOVOL_iDrv);                   /*  �����������Ƴ�              */

            API_SemaphoreMDelete(&pisovol->ISOVOL_hVolLock);            /*  ɾ������                    */

            __SHEAP_FREE(pisovol);                                      /*  �ͷž���ƿ�                */

            _DebugHandle(__LOGMESSAGE_LEVEL, "disk unmount ok.\r\n");

            return  (ERROR_NONE);

        } else {                                                        /*  ɾ���ļ���Ŀ¼              */
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }
}
/*********************************************************************************************************
** ��������: __iso9660FsClose
** ��������: ISO9660 FS close ����
** �䡡��  : pfdentry         �ļ����ƿ�
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
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
            _ErrorHandle(ENXIO);                                        /*  �豸����                    */
            return  (PX_ERROR);
        }
        
        if (API_IosFdNodeDec(&pisovol->ISOVOL_plineFdNodeHeader,
                             pfdnode, &bRemove) == 0) {                 /*  fd_node �Ƿ���ȫ�ͷ�        */
            iso9660_stat_free(pisofile->ISOFIL_pstat);
            bFree = LW_TRUE;
        }
        
        LW_DEV_DEC_USE_COUNT(&pisovol->ISOVOL_devhdrHdr);               /*  ���¼�����                  */
        
        __ISO_FILE_UNLOCK(pisofile);
        
        if (bFree) {
            __SHEAP_FREE(pisofile);                                     /*  �ͷ��ڴ�                    */
        }
        
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __iso9660FsRead
** ��������: ISO9660 FS read ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
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
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
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
** ��������: __iso9660FsPRead
** ��������: ISO9660 FS pread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
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
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
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
** ��������: __iso9660FsWrite
** ��������: ISO9660 FS write ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __iso9660FsWrite (PLW_FD_ENTRY  pfdentry,
                                  PCHAR         pcBuffer,
                                  size_t        stNBytes)
{
    _ErrorHandle(EROFS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __iso9660FsPWrite
** ��������: ISO9660 FS pwrite ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __iso9660FsSeek
** ��������: ISO9660 FS �ļ���λ
** �䡡��  : pfdentry            �ļ����ƿ�
**           oftPos              ��λ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __iso9660FsSeek (PLW_FD_ENTRY  pfdentry, off_t  oftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T    pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    if (oftPos > pfdnode->FDNODE_oftSize) {                             /*  ֻ���ļ�ϵͳ���������ļ�    */
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
** ��������: __iso9660FsWhere
** ��������: ISO9660 FS ����ļ���ǰ��дָ��λ�� (ʹ�ò�����Ϊ����ֵ, �� FIOWHERE ��Ҫ�����в�ͬ)
** �䡡��  : pfdentry            �ļ����ƿ�
**           poftPos             ��дָ��λ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __iso9660FsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T    pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
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
** ��������: __iso9660FsNRead
** ��������: ISO9660 FS ����ļ�ʣ�����Ϣ��
** �䡡��  : pfdentry            �ļ����ƿ�
**           piPos               ʣ��������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
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
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
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
** ��������: __iso9660FsNRead64
** ��������: ISO9660 FS ����ļ�ʣ�����Ϣ��
** �䡡��  : pfdentry            �ļ����ƿ�
**           poftPos             ʣ��������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __iso9660FsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T      pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__ISO_FILE_LOCK(pisofile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
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
** ��������: __iso9660FsVolLabel
** ��������: ISO9660 FS �ļ�ϵͳ��괦����
** �䡡��  : pfdentry            �ļ����ƿ�
**           pcLabel             ��껺��
**           bSet                ���û��ǻ�ȡ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
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
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
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
** ��������: __iso9660FsStatGet
** ��������: ISO9660 FS ����ļ�״̬������
** �䡡��  : pfdentry            �ļ����ƿ�
**           pstat               stat �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
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
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    if (__STR_IS_ROOT(pisofile->ISOFIL_cName)) {                        /*  Ϊ��Ŀ¼                    */
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
** ��������: __iso9660FsStatfsGet
** ��������: ISO9660 FS ����ļ�ϵͳ״̬������
** �䡡��  : pfdentry            �ļ����ƿ�
**           pstatfs             statfs �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
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
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
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
** ��������: __iso9660FsReadDir
** ��������: ISO9660 FS ���ָ��Ŀ¼��Ϣ
** �䡡��  : pfdentry            �ļ����ƿ�
**           dir                 Ŀ¼�ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
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
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
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
** ��������: __iso9660FsIoctl
** ��������: ISO9660 FS ioctl ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           request,            ����
**           arg                 �������
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __iso9660FsIoctl (PLW_FD_ENTRY  pfdentry,
                              INT           iRequest,
                              LONG          lArg)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PISO_FILE_T      pisofile = (PISO_FILE_T)pfdnode->FDNODE_pvFile;
    INT            iError;
    off_t          oftTemp  = 0;                                        /*  ��ʱ����                    */

    switch (iRequest) {

    case FIOCONTIG:
    case FIOATTRIBSET:
    case FIODISKFORMAT:                                                 /*  ���ʽ��                    */
    case FIOTIMESET:                                                    /*  �����ļ�ʱ��                */
    case FIOTRUNC:                                                      /*  �ı��ļ���С                */
    case FIORENAME:                                                     /*  �ļ�������                  */
    case FIOCHMOD:
    case FIOLABELSET:                                                   /*  ���þ��                    */
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    
    case FIODISKINIT:                                                   /*  ���̳�ʼ��                  */
        return  (__blockIoDevIoctl(pisofile->ISOFIL_pisovol->ISOVOL_iDrv,
                                   FIODISKINIT, lArg));
    
    /*
     *  FISOEEK, FIOWHERE is 64 bit operate.
     */
    case FIOSEEK:                                                       /*  �ļ��ض�λ                  */
        oftTemp = *(off_t *)lArg;
        return  (__iso9660FsSeek(pfdentry, oftTemp));
    
    case FIOWHERE:                                                      /*  ����ļ���ǰ��дָ��        */
        iError = __iso9660FsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
    
    case FIONREAD:                                                      /*  ����ļ�ʣ���ֽ���          */
        return  (__iso9660FsNRead(pfdentry, (INT *)lArg));
    
    case FIONREAD64:                                                    /*  ����ļ�ʣ���ֽ���          */
        iError = __iso9660FsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIOSYNC:                                                       /*  ���ļ������д              */
    case FIODATASYNC:
    case FIOFLUSH:
        return  (ERROR_NONE);

    case FIOLABELGET:                                                   /*  ��ȡ���                    */
        return  (__iso9660FsVolLabel(pfdentry, (PCHAR)lArg, LW_FALSE));
    
    case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
        return  (__iso9660FsStatGet(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  ����ļ�ϵͳ״̬            */
        return  (__iso9660FsStatfsGet(pfdentry, (struct statfs *)lArg));
        
    case FIOREADDIR:                                                    /*  ��ȡһ��Ŀ¼��Ϣ            */
        return  (__iso9660FsReadDir(pfdentry, (DIR *)lArg));
        
    case FIOSETFL:                                                      /*  �����µ� flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);
        
    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "ISO9660 CD-ROM FileSystem";
        return  (ERROR_NONE);
        
    case FIOGETFORCEDEL:                                                /*  ǿ��ж���豸�Ƿ�����      */
        *(BOOL *)lArg = pisofile->ISOFIL_pisovol->ISOVOL_bForceDelete;
        return  (ERROR_NONE);
        
    case FIOSETFORCEDEL:                                                /*  ����ǿ��ж��ʹ��            */
        pisofile->ISOFIL_pisovol->ISOVOL_bForceDelete = (BOOL)lArg;
        return  (ERROR_NONE);
        
    case FIOFSGETFL:                                                    /*  ��ȡ�ļ�ϵͳȨ��            */
        if ((INT *)lArg == LW_NULL) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        *(INT *)lArg = pisofile->ISOFIL_pisovol->ISOVOL_iFlag;
        return  (ERROR_NONE);

    case FIOFSSETFL:                                                    /*  �����ļ�ϵͳȨ��            */
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
            SEL_WAKE_UP((PLW_SEL_WAKEUPNODE)lArg);                      /*  ���ѽڵ�                    */
        }
        return  (ERROR_NONE);

    case FIOUNSELECT:
        if (((PLW_SEL_WAKEUPNODE)lArg)->SELWUN_seltypType != SELEXCEPT) {
            LW_SELWUN_SET_READY((PLW_SEL_WAKEUPNODE)lArg);              /*  ����Ϊ����                  */
        }
        return  (ERROR_NONE);
#endif

    default:                                                            /*  �޷�ʶ�������              */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    return  (PX_ERROR);
}

#endif                                                                  /*  (LW_CFG_ISO9660FS_EN > 0)   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
