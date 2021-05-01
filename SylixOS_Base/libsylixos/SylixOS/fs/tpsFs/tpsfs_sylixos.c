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
** ��   ��   ��: tpsfs_sylixos.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: tpsfs SylixOS FS �ӿ�.

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0
#include "tpsfs_type.h"
#include "tpsfs_error.h"
#include "tpsfs_port.h"
#include "tpsfs_super.h"
#include "tpsfs_trans.h"
#include "tpsfs_btree.h"
#include "tpsfs_inode.h"
#include "tpsfs_dir.h"
#include "tpsfs_super.h"
#include "tpsfs.h"
/*********************************************************************************************************
  TPS Struct
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          TPSVOL_devhdrHdr;                               /*  �豸ͷ                      */
    TPS_DEV             TPSVOL_dev;
    BOOL                TPSVOL_bForceDelete;                            /*  �Ƿ�����ǿ��ж�ؾ�          */
    PTPS_SUPER_BLOCK    TPSVOL_tpsFsVol;                                /*  �ļ�ϵͳ����Ϣ              */
    INT                 TPSVOL_iDrv;                                    /*  ������λ��                  */
    BOOL                TPSVAL_bValid;                                  /*  ��Ч�Ա�־                  */
    LW_OBJECT_HANDLE    TPSVOL_hVolLock;                                /*  �������                    */
    LW_LIST_LINE_HEADER TPSVOL_plineFdNodeHeader;                       /*  fd_node ����                */
    UINT64              TPSVOL_uiTime;                                  /*  ����ʱ�� TPS ��ʽ         */
    INT                 TPSVOL_iFlag;                                   /*  O_RDONLY or O_RDWR          */
    LW_HANDLE           TPSVOL_hThreadFlush;                            /*  ��д�߳̾��                */
    BOOL                TPSVOL_bNeedExit;                               /*  �Ƿ���Ҫ�˳��߳�            */
} TPS_VOLUME;
typedef TPS_VOLUME     *PTPS_VOLUME;

typedef struct {
    PTPS_VOLUME         TPSFIL_ptpsvol;                                 /*  ���ھ���Ϣ                  */
    PTPS_INODE          TPSFIL_pinode;                                  /*  �ļ�inode                   */
    INT                 TPSFIL_iFileType;                               /*  �ļ�����                    */
    CHAR                TPSFIL_cName[1];                                /*  �ļ���                      */
} TPS_FILE;
typedef TPS_FILE       *PTPS_FILE;
/*********************************************************************************************************
  �ڲ�ȫ�ֱ���
*********************************************************************************************************/
static INT              _G_iTpsDrvNum = PX_ERROR;
/*********************************************************************************************************
  inode ��д��� (ms)
*********************************************************************************************************/
#define __TPS_INODE_FLUSH_INTERVAL    2000
/*********************************************************************************************************
  �ļ�����
*********************************************************************************************************/
#define __TPS_FILE_TYPE_NODE          0                                 /*  open ���ļ�               */
#define __TPS_FILE_TYPE_DIR           1                                 /*  open ��Ŀ¼               */
#define __TPS_FILE_TYPE_DEV           2                                 /*  open ���豸               */
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __TPS_FILE_LOCK(ptpsfile)   API_SemaphoreMPend(ptpsfile->TPSFIL_ptpsvol->TPSVOL_hVolLock, \
                                    LW_OPTION_WAIT_INFINITE)
#define __TPS_FILE_UNLOCK(ptpsfile) API_SemaphoreMPost(ptpsfile->TPSFIL_ptpsvol->TPSVOL_hVolLock)
#define __TPS_VOL_LOCK(ptpsvol)     API_SemaphoreMPend(ptpsvol->TPSVOL_hVolLock, LW_OPTION_WAIT_INFINITE)
#define __TPS_VOL_UNLOCK(ptpsvol)   API_SemaphoreMPost(ptpsvol->TPSVOL_hVolLock)
/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)       ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
/*********************************************************************************************************
  ��ӡinode��Ϣ
*********************************************************************************************************/
#ifdef __SYLIXOS_DEBUG
#define TPSFS_IOCTRL_INODE_PRINT     LW_OSIOD('f', 135, BOOL)
#endif
/*********************************************************************************************************
  ������������
*********************************************************************************************************/
static LONG     __tpsFsOpen(PTPS_VOLUME     ptpsvol,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __tpsFsRemove(PTPS_VOLUME     ptpsvol,
                              PCHAR           pcName);
static INT      __tpsFsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __tpsFsRead(PLW_FD_ENTRY   pfdentry,
                            PCHAR          pcBuffer,
                            size_t         stMaxBytes);
static ssize_t  __tpsFsPRead(PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer,
                             size_t         stMaxBytes,
                             off_t      oftPos);
static ssize_t  __tpsFsWrite(PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer,
                             size_t         stNBytes);
static ssize_t  __tpsFsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes,
                              off_t         oftPos);
static INT      __tpsFsLStat(PTPS_VOLUME   ptpsfs,
                             PCHAR         pcName,
                             struct stat  *pstat);
static INT      __tpsFsIoctl(PLW_FD_ENTRY   pfdentry,
                             INT            iRequest,
                             LONG           lArg);
static INT      __tpsFsSymlink(PTPS_VOLUME   ptpsvol,
                               PCHAR         pcName,
                               CPCHAR        pcLinkDst);
static ssize_t  __tpsFsReadlink(PTPS_VOLUME   ptpsvol,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize);
static INT      __tpsFsSync(PLW_FD_ENTRY   pfdentry, BOOL  bFlushCache);
/*********************************************************************************************************
  �ļ�ϵͳ��������
*********************************************************************************************************/
LW_API INT      API_TpsFsDevCreate(PCHAR   pcName, PLW_BLK_DEV  pblkd);
/*********************************************************************************************************
  block��������
*********************************************************************************************************/
INT             __blockIoDevCreate(PLW_BLK_DEV  pblkdNew);
VOID            __blockIoDevDelete(INT  iIndex);
PLW_BLK_DEV     __blockIoDevGet(INT  iIndex);
INT             __blockIoDevReset(INT  iIndex);
INT             __blockIoDevIoctl(INT  iIndex, INT  iCmd, LONG  lArg);
INT             __blockIoDevIsLogic(INT  iIndex);
INT             __blockIoDevFlag(INT     iIndex);
PLW_BLK_DEV     __blockIoDevGet(INT  iIndex);
INT             __blockIoDevRead(INT     iIndex,
                                 VOID   *pvBuffer,
                                 ULONG   ulStartSector,
                                 ULONG   ulSectorCount);
INT             __blockIoDevWrite(INT     iIndex,
                                  VOID   *pvBuffer,
                                  ULONG   ulStartSector,
                                  ULONG   ulSectorCount);
INT             __blockIoDevStatus(INT     iIndex);
/*********************************************************************************************************
** ��������: __tpsFsFlushThread
** ��������: inode ��д�߳�
** �䡡��  : ptpsvol    ��
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �������߳��˳�֮ǰ��Ҫ��ǰ���� detach �����ļ�ϵͳж������, ��Ϊж����������� t_except ����
             �������߳�ɾ���������ܵò���ִ��.
*********************************************************************************************************/
static void  __tpsFsFlushThread (PTPS_VOLUME  ptpsvol)
{
    while (!ptpsvol->TPSVOL_bNeedExit) {
        API_TimeMSleep(__TPS_INODE_FLUSH_INTERVAL);

        __TPS_VOL_LOCK(ptpsvol);
        tpsFsFlushInodes(ptpsvol->TPSVOL_tpsFsVol);
        __TPS_VOL_UNLOCK(ptpsvol);
    }
    
    API_ThreadDetach(API_ThreadIdSelf());
}
/*********************************************************************************************************
** ��������: __tpsFsDiskIndex
** ��������: ��ȡ block I/O index.
** �䡡��  : pdev      �豸�ļ�
** �䡡��  : block io index.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE INT  __tpsFsDiskIndex (PTPS_DEV  pdev)
{
    PTPS_VOLUME  ptpsvol = _LIST_ENTRY(pdev, TPS_VOLUME, TPSVOL_dev);
    
    return  (ptpsvol->TPSVOL_iDrv);
}
/*********************************************************************************************************
** ��������: __tpsFsDiskSectorSize
** ��������: ��ȡ������С
**           pdev               �豸�ļ�
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT  __tpsFsDiskSectorSize (PTPS_DEV pdev)
{
    INT     iDrv      = __tpsFsDiskIndex(pdev);
    ULONG   ulSecSize = 0;

    __blockIoDevIoctl(iDrv, LW_BLKD_GET_SECSIZE, (LONG)&ulSecSize);

    return  ((UINT)ulSecSize);
}
/*********************************************************************************************************
** ��������: __tpsFsDiskSectorCnt
** ��������: ��ȡ������
**           pdev               �豸�ļ�
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT64  __tpsFsDiskSectorCnt (PTPS_DEV pdev)
{
    INT     iDrv     = __tpsFsDiskIndex(pdev);
    ULONG   ulSecCnt = 0;

    __blockIoDevIoctl(iDrv, LW_BLKD_GET_SECNUM, (LONG)&ulSecCnt);

    return  ((UINT64)ulSecCnt);
}
/*********************************************************************************************************
** ��������: __tpsFsDiskRead
** ��������: ��ȡ���ݵ�����
**           pdev               �豸�ļ�
** �䡡��  : ui64StartSector    ��ʼ����
**           uiSectorCnt        ������
** �䡡��  : 0��ʾ�ǳɹ�����0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsDiskRead (PTPS_DEV    pdev,
                             PUCHAR      pucBuf,
                             UINT64      ui64StartSector,
                             UINT64      uiSectorCnt)
{
    INT     iDrv = __tpsFsDiskIndex(pdev);

    return  (__blockIoDevRead(iDrv,
                              (PVOID)pucBuf,
                              (ULONG)ui64StartSector,
                              (ULONG)uiSectorCnt));
}
/*********************************************************************************************************
** ��������: __tpsFsDiskWrite
** ��������: д���ݵ�����
**           pdev               �豸�ļ�
** �䡡��  : ui64StartSector    ��ʼ����
**           uiSectorCnt        ������
**           bSync              �Ƿ���Ҫͬ��
** �䡡��  : 0��ʾ�ǳɹ�����0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsDiskWrite (PTPS_DEV   pdev,
                              PUCHAR     pucBuf,
                              UINT64     ui64StartSector,
                              UINT64     uiSectorCnt,
                              BOOL       bSync)
{
    INT     iDrv = __tpsFsDiskIndex(pdev);
    INT     iRtn;

    iRtn = __blockIoDevWrite((INT)iDrv,
                             (PVOID)pucBuf,
                             (ULONG)ui64StartSector,
                             (ULONG)uiSectorCnt);
    if (iRtn != 0) {
        return  (iRtn);
    }

    if (bSync) {                                                        /* ͬ��д��                     */
        LW_BLK_RANGE blkrange;
        
        blkrange.BLKR_ulStartSector = (ULONG)ui64StartSector;
        blkrange.BLKR_ulEndSector   = (ULONG)(ui64StartSector + uiSectorCnt - 1);

        iRtn = __blockIoDevIoctl((INT)iDrv, FIOSYNCMETA, (LONG)&blkrange);
    }

    return  (iRtn);
}
/*********************************************************************************************************
** ��������: __tpsFsDiskSync
** ��������: ͬ���������ݵ�����
**           pdev               �豸�ļ�
** �䡡��  : ui64StartSector    ��ʼ����
**           uiSectorCnt        ������
** �䡡��  : 0 ���ǳɹ��� ��0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsDiskSync (PTPS_DEV pdev, UINT64 ui64StartSector, UINT64 uiSectorCnt)
{
    INT          iDrv = __tpsFsDiskIndex(pdev);
    LW_BLK_RANGE blkrange;
    
    blkrange.BLKR_ulStartSector = (ULONG)(ui64StartSector);
    blkrange.BLKR_ulEndSector   = (ULONG)(ui64StartSector + uiSectorCnt - 1);

    return  (__blockIoDevIoctl((INT)iDrv, FIOSYNCMETA, (LONG)&blkrange));
}
/*********************************************************************************************************
** ��������: __tpsFsDiskSync
** ��������: ͬ���������ݵ�����
**           pdev               �豸�ļ�
** �䡡��  : ui64StartSector    ��ʼ����
**           uiSectorCnt        ������
** �䡡��  : 0 ���ǳɹ��� ��0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsDiskTrim (PTPS_DEV pdev, UINT64 ui64StartSector, UINT64 uiSectorCnt)
{
    INT          iDrv = __tpsFsDiskIndex(pdev);
    LW_BLK_RANGE blkrange;

    blkrange.BLKR_ulStartSector = (ULONG)(ui64StartSector);
    blkrange.BLKR_ulEndSector   = (ULONG)(ui64StartSector + uiSectorCnt - 1);

    return  (__blockIoDevIoctl((INT)iDrv, FIOTRIM, (LONG)&blkrange));
}
/*********************************************************************************************************
** ��������: API_TpsFsDrvInstall
** ��������: ��װ TPS �ļ�ϵͳ��������
** �䡡��  :
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_TpsFsDrvInstall (VOID)
{
    struct file_operations     fileop;

    if (_G_iTpsDrvNum > 0) {
        return  (ERROR_NONE);
    }

    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __tpsFsOpen;
    fileop.fo_release  = __tpsFsRemove;
    fileop.fo_open     = __tpsFsOpen;
    fileop.fo_close    = __tpsFsClose;
    fileop.fo_read     = __tpsFsRead;
    fileop.fo_read_ex  = __tpsFsPRead;
    fileop.fo_write    = __tpsFsWrite;
    fileop.fo_write_ex = __tpsFsPWrite;
    fileop.fo_lstat    = __tpsFsLStat;
    fileop.fo_ioctl    = __tpsFsIoctl;
    fileop.fo_symlink  = __tpsFsSymlink;
    fileop.fo_readlink = __tpsFsReadlink;

    _G_iTpsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);       /*  ʹ�� NEW_1 ���豸����       */

    DRIVER_LICENSE(_G_iTpsDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iTpsDrvNum,      "Jiang.Taijin");
    DRIVER_DESCRIPTION(_G_iTpsDrvNum, "tpsFs driver.");

    _DebugHandle(__LOGMESSAGE_LEVEL, "SylixOS tpsFs file system installed.\r\n");

    __fsRegister("tpsfs", API_TpsFsDevCreate, LW_NULL, LW_NULL);        /*  ע���ļ�ϵͳ                */

    return  ((_G_iTpsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: API_TpsFsDevCreate
** ��������: ����һ�� TPS �豸, ����: API_TpsFsDevCreate("/ata0", ...);
**           �� sylixos �� yaffs ��ͬ, TPS ÿһ�����Ƕ������豸.
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
**           pblkd             ���豸����
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_TpsFsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    REGISTER PTPS_VOLUME            ptpsvol;
    REGISTER INT                    iBlkdIndex;
             INT                    iErrLevel      = 0;
             UINT                   uiMountFlag    = 0;
             errno_t                iErr           = ERROR_NONE;
             LW_CLASS_THREADATTR    threadattr;

    if (_G_iTpsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "tps Driver invalidate.\r\n");
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

    ptpsvol = (PTPS_VOLUME)__SHEAP_ALLOC(sizeof(TPS_VOLUME));
    if (ptpsvol == LW_NULL) {
        __blockIoDevDelete(iBlkdIndex);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(ptpsvol, sizeof(TPS_VOLUME));                             /*  ��վ���ƿ�                */

    ptpsvol->TPSVOL_dev.DEV_SectorSize    = __tpsFsDiskSectorSize;
    ptpsvol->TPSVOL_dev.DEV_SectorCnt     = __tpsFsDiskSectorCnt;
    ptpsvol->TPSVOL_dev.DEV_ReadSector    = __tpsFsDiskRead;
    ptpsvol->TPSVOL_dev.DEV_WriteSector   = __tpsFsDiskWrite;
    ptpsvol->TPSVOL_dev.DEV_Sync          = __tpsFsDiskSync;
    ptpsvol->TPSVOL_dev.DEV_Trim          = __tpsFsDiskTrim;

    ptpsvol->TPSVOL_bForceDelete = LW_FALSE;                            /*  ������ǿ��ж�ؾ�            */
    ptpsvol->TPSVOL_iDrv     = iBlkdIndex;                              /*  ��¼����λ��                */
    ptpsvol->TPSVAL_bValid   = LW_TRUE;                                 /*  ����Ч                      */
    ptpsvol->TPSVOL_hVolLock = API_SemaphoreMCreate("tpsvol_lock",
                               LW_PRIO_DEF_CEILING,
                               LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                               LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                               LW_NULL);
    if (!ptpsvol->TPSVOL_hVolLock) {                                    /*  �޷���������                */
        iErrLevel = 2;
        goto    __error_handle;
    }
    ptpsvol->TPSVOL_plineFdNodeHeader = LW_NULL;                        /*  û���ļ�����              */
    ptpsvol->TPSVOL_uiTime = TPS_UTC_TIME();                            /*  ��õ�ǰʱ��                */

    if (pblkd->BLKD_iFlag == O_RDONLY) {                                /*  ����ģʽת����tpsfsģʽ     */
        uiMountFlag = TPS_MOUNT_FLAG_READ;

    } else if (pblkd->BLKD_iFlag == O_WRONLY) {
        uiMountFlag = TPS_MOUNT_FLAG_WRITE;

    } else {
        uiMountFlag = TPS_MOUNT_FLAG_WRITE | TPS_MOUNT_FLAG_READ;
    }

    iErr = tpsFsMount(&ptpsvol->TPSVOL_dev, uiMountFlag, &ptpsvol->TPSVOL_tpsFsVol);
    if (iErr != ERROR_NONE) {                                           /*  �����ļ�ϵͳ                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "mount tpsfs failed.\r\n");
        iErrLevel = 3;
        goto    __error_handle;
    }

    /*
     *  ���� inode ��д�߳�
     */
    threadattr = API_ThreadAttrGetDefault();
    threadattr.THREADATTR_pvArg           = (void *)ptpsvol;
    threadattr.THREADATTR_stStackByteSize = LW_CFG_THREAD_DISKCACHE_STK_SIZE;
    threadattr.THREADATTR_ucPriority      = LW_PRIO_T_SERVICE;
    threadattr.THREADATTR_ulOption       |= LW_OPTION_OBJECT_GLOBAL;

    ptpsvol->TPSVOL_bNeedExit    = LW_FALSE;
    ptpsvol->TPSVOL_hThreadFlush = API_ThreadCreate("t_tpsfs",
                                                    (PTHREAD_START_ROUTINE)__tpsFsFlushThread,
                                                    &threadattr, LW_NULL);
                                                                        /*  �������������߳�            */
    ptpsvol->TPSVOL_iFlag = pblkd->BLKD_iFlag;

    if (iosDevAddEx(&ptpsvol->TPSVOL_devhdrHdr, pcName, _G_iTpsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  ��װ�ļ�ϵͳ�豸            */
        iErr = (INT)API_GetLastError();
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
        ptpsvol->TPSVOL_bNeedExit = LW_TRUE;
        API_ThreadWakeup(ptpsvol->TPSVOL_hThreadFlush);
        API_ThreadJoin(ptpsvol->TPSVOL_hThreadFlush, LW_NULL);          /*  ��д�߳��˳�                */
        tpsFsUnmount(ptpsvol->TPSVOL_tpsFsVol);                         /*  ж�ع��ص��ļ�ϵͳ          */
    }
    if (iErrLevel > 2) {
        API_SemaphoreMDelete(&ptpsvol->TPSVOL_hVolLock);
    }
    if (iErrLevel > 1) {
        __blockIoDevDelete(iBlkdIndex);
    }

    __SHEAP_FREE(ptpsvol);

    _ErrorHandle(iErr);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_TpsFsDevDelete
** ��������: ɾ��һ�� TPS �豸, ����: API_TpsFsDevDelete("/mnt/ata0");
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_TpsFsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  ������豸, �����ж���豸  */
        return  (unlink(pcName));

    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __tpsFsPathBuildLink
** ��������: ���������ļ�������������Ŀ��
** �䡡��  : promfs           �ļ�ϵͳ
**           promdnt          �ļ��ṹ
**           pcDest           �������
**           stSize           ��������С
**           pcPrefix         ǰ׺
**           pcTail           ��׺
** �䡡��  : < 0 ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsPathBuildLink (PTPS_FILE    ptpsfile,  PCHAR        pcDest,
                                  size_t       stSize,    PCHAR        pcPrefix,
                                  PCHAR        pcTail)
{
    CHAR        cLink[PATH_MAX + 1] = {0};
    TPS_SIZE_T  szRead;

    if (tpsFsRead(ptpsfile->TPSFIL_pinode, (PUCHAR)cLink,
                  0, PATH_MAX + 1, &szRead) == ERROR_NONE) {
        return  (_PathBuildLink(pcDest, stSize,
                                ptpsfile->TPSFIL_ptpsvol->TPSVOL_devhdrHdr.DEVHDR_pcName,
                                pcPrefix, cLink, pcTail));
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __tpsFsOpen
** ��������: TPS FS open ����
** �䡡��  : ptpsvol          ����ƿ�
**           pcName           �ļ���
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  __tpsFsOpen (PTPS_VOLUME     ptpsvol,
                          PCHAR           pcName,
                          INT             iFlags,
                          INT             iMode)
{
    REGISTER PTPS_FILE      ptpsfile;
    REGISTER ULONG          ulError;
             PLW_FD_NODE    pfdnode;
             BOOL           bIsNew;
             struct stat    statGet;
             PCHAR          pcTail = LW_NULL;
             errno_t        iErr   = ERROR_NONE;

    if (pcName == LW_NULL) {                                            /*  ���ļ���                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (__blockIoDevFlag(ptpsvol->TPSVOL_iDrv) == O_RDONLY) {           /*  �˾�д����, ����ֻ��״̬    */
        if (iFlags & (O_CREAT | O_TRUNC | O_RDWR | O_WRONLY)) {
            _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
            return  (PX_ERROR);
        }
    }
    if (iFlags & O_CREAT) {
        if (ptpsvol->TPSVOL_iFlag == O_RDONLY) {
            _ErrorHandle(EROFS);                                        /*  ֻ���ļ�ϵͳ                */
            return  (PX_ERROR);
        }

        if (__fsCheckFileName(pcName)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }

        if (S_ISFIFO(iMode) ||
            S_ISBLK(iMode)  ||
            S_ISCHR(iMode)) {                                           /*  ���ﲻ���� socket �ļ�      */
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  ��֧��������Щ��ʽ          */
            return  (PX_ERROR);
        }
    }

    ptpsfile = (PTPS_FILE)__SHEAP_ALLOC(sizeof(TPS_FILE) +
                                        lib_strlen(pcName));            /*  �����ļ��ڴ�                */
    if (ptpsfile == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_strcpy(ptpsfile->TPSFIL_cName, pcName);                         /*  ��¼�ļ���                  */

    ptpsfile->TPSFIL_ptpsvol = ptpsvol;                                 /*  ��¼����Ϣ                  */

    ulError = __TPS_FILE_LOCK(ptpsfile);
    if ((ptpsvol->TPSVAL_bValid == LW_FALSE) ||
        (ulError != ERROR_NONE)) {                                      /*  �����ڱ�ж��                */
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }

    if ((iMode & S_IFMT) == 0) {
        iMode |= S_IFREG;
    }

    iErr = tpsFsOpen(ptpsvol->TPSVOL_tpsFsVol, pcName, iFlags,
                     iMode, &pcTail, &ptpsfile->TPSFIL_pinode);
    if (iErr != ERROR_NONE) {
        if (__STR_IS_ROOT(pcName)) {                                    /*  δ��ʽ���豸��·��          */
            ptpsfile->TPSFIL_iFileType = __TPS_FILE_TYPE_DEV;           /*  �������豸, δ��ʽ��      */
            tpsFsStat(&ptpsvol->TPSVOL_devhdrHdr, ptpsvol->TPSVOL_tpsFsVol, 
                      ptpsfile->TPSFIL_pinode, &statGet);
            goto    __file_open_ok;
            
        } else {
            __TPS_FILE_UNLOCK(ptpsfile);
            __SHEAP_FREE(ptpsfile);
            _ErrorHandle(iErr);
            return  (PX_ERROR);
        }
    }

    tpsFsStat(&ptpsvol->TPSVOL_devhdrHdr, ptpsvol->TPSVOL_tpsFsVol,
              ptpsfile->TPSFIL_pinode, &statGet);

    if (S_ISLNK(statGet.st_mode) && pcTail) {
        INT     iFollowLinkType;
        PCHAR   pcSymfile = pcTail;
        PCHAR   pcPrefix;

        if (pcSymfile != pcName) {
            pcSymfile--;
        }

        while (pcSymfile != pcName && *pcSymfile == PX_DIVIDER) {
            pcSymfile--;
        }

        while (pcSymfile != pcName && *pcSymfile != PX_DIVIDER) {
            pcSymfile--;
        }

        if (pcSymfile == pcName) {
            pcPrefix = LW_NULL;                                         /*  û��ǰ׺                    */

        } else {
            pcPrefix = pcName;
            *pcSymfile = PX_EOS;
        }
        if (pcTail && lib_strlen(pcTail)) {
            iFollowLinkType = FOLLOW_LINK_TAIL;                         /*  ����Ŀ���ڲ��ļ�            */
        } else {
            iFollowLinkType = FOLLOW_LINK_FILE;                         /*  �����ļ�����                */
        }

        if (__tpsFsPathBuildLink(ptpsfile, pcName, PATH_MAX + 1,
                                 pcPrefix, pcTail) == ERROR_NONE) {
                                                                        /*  ��������Ŀ��                */
            tpsFsClose(ptpsfile->TPSFIL_pinode);
            __TPS_FILE_UNLOCK(ptpsfile);
            __SHEAP_FREE(ptpsfile);
            return  (iFollowLinkType);

        } else {                                                        /*  ��������ʧ��                */
            tpsFsClose(ptpsfile->TPSFIL_pinode);
            __TPS_FILE_UNLOCK(ptpsfile);
            __SHEAP_FREE(ptpsfile);
            _ErrorHandle(ERROR_IOS_FILE_NOT_SUP);
            return  (PX_ERROR);
        }

    } else if (S_ISDIR(statGet.st_mode)) {
        ptpsfile->TPSFIL_iFileType = __TPS_FILE_TYPE_DIR;

    } else {
        ptpsfile->TPSFIL_iFileType = __TPS_FILE_TYPE_NODE;
    }

__file_open_ok:
    pfdnode = API_IosFdNodeAdd(&ptpsvol->TPSVOL_plineFdNodeHeader,
                               statGet.st_dev,
                               statGet.st_ino,
                               iFlags, statGet.st_mode, 0,
                               0, statGet.st_size,
                               (PVOID)ptpsfile,
                               &bIsNew);                                /*  ����ļ��ڵ�                */
    if (pfdnode == LW_NULL) {                                           /*  �޷����� fd_node �ڵ�       */
        tpsFsClose(ptpsfile->TPSFIL_pinode);
        __TPS_FILE_UNLOCK(ptpsfile);
        __SHEAP_FREE(ptpsfile);
        return  (PX_ERROR);
    }

    LW_DEV_INC_USE_COUNT(&ptpsvol->TPSVOL_devhdrHdr);                   /*  ���¼�����                  */

    if (bIsNew == LW_FALSE) {                                           /*  ���ظ���                  */
        tpsFsClose(ptpsfile->TPSFIL_pinode);
        __TPS_FILE_UNLOCK(ptpsfile);
        __SHEAP_FREE(ptpsfile);
    
    } else {
        __TPS_FILE_UNLOCK(ptpsfile);
    }

    return  ((LONG)pfdnode);
}
/*********************************************************************************************************
** ��������: __tpsFsRemove
** ��������: TPS FS remove ����
** �䡡��  : ptpsvol          ����ƿ�
**           pcName           �ļ���
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsRemove (PTPS_VOLUME     ptpsvol,
                           PCHAR           pcName)
{
    PCHAR          pcTail  = LW_NULL;
    errno_t        iErr    = ERROR_NONE;
    PLW_BLK_DEV    pblkd;
    TPS_FILE       tpsfile;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (__STR_IS_ROOT(pcName)) {                                        /*  ��Ŀ¼�����豸�ļ�          */
        /*
         *  ��Ϊ��д�߳�����������Ϊ��ֹ��API_ThreadJoin�����������˳���д�߳��ڼ���֮ǰִ��
         */
        ptpsvol->TPSVOL_bNeedExit = LW_TRUE;
        API_ThreadWakeup(ptpsvol->TPSVOL_hThreadFlush);
        API_ThreadJoin(ptpsvol->TPSVOL_hThreadFlush, LW_NULL);          /*  ��д�߳��˳�                */

        if (__TPS_VOL_LOCK(ptpsvol) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);                                         /*  ���ڱ���������ж��          */
        }

        if (ptpsvol->TPSVAL_bValid == LW_FALSE) {
            __TPS_VOL_UNLOCK(ptpsvol);
            return  (ERROR_NONE);                                       /*  ���ڱ���������ж��          */
        }

__re_umount_vol:
        if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)ptpsvol)) {              /*  ����Ƿ������ڹ������ļ�    */
            if (!ptpsvol->TPSVOL_bForceDelete) {
                __TPS_VOL_UNLOCK(ptpsvol);
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);                                     /*  ���ļ���, ���ܱ�ж��      */
            }

            ptpsvol->TPSVAL_bValid = LW_FALSE;                          /*  ��ʼж�ؾ�, �ļ����޷���  */

            __TPS_VOL_UNLOCK(ptpsvol);

            _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
            iosDevFileAbnormal(&ptpsvol->TPSVOL_devhdrHdr);             /*  ����������ļ���Ϊ�쳣״̬  */

            __TPS_VOL_LOCK(ptpsvol);
            goto    __re_umount_vol;

        } else {
            ptpsvol->TPSVAL_bValid = LW_FALSE;                          /*  ��ʼж�ؾ�, �ļ����޷���  */
        }

        /*
         *  ���ܳ���ʲô�������, �������ж�ؾ�, ���ﲻ���жϴ���.
         */
        __blockIoDevIoctl(ptpsvol->TPSVOL_iDrv,
                          FIOFLUSH, 0);                                 /*  ��д DISK CACHE             */
        __blockIoDevIoctl(ptpsvol->TPSVOL_iDrv, LW_BLKD_CTRL_POWER,
                          LW_BLKD_POWER_OFF);                           /*  �豸�ϵ�                    */
        __blockIoDevIoctl(ptpsvol->TPSVOL_iDrv, LW_BLKD_CTRL_EJECT,
                          0);                                           /*  ���豸����                  */

        pblkd = __blockIoDevGet(ptpsvol->TPSVOL_iDrv);                  /*  ��ÿ��豸���ƿ�            */
        if (pblkd) {
            __fsDiskLinkCounterDec(pblkd);                              /*  �������Ӵ���                */
        }

        iosDevDelete((LW_DEV_HDR *)ptpsvol);                            /*  IO ϵͳ�Ƴ��豸             */

        tpsFsUnmount(ptpsvol->TPSVOL_tpsFsVol);                         /*  ж�ع��ص��ļ�ϵͳ          */

        __blockIoDevIoctl(ptpsvol->TPSVOL_iDrv,
                          FIOUNMOUNT, 0);                               /*  ִ�еײ���������            */
        __blockIoDevDelete(ptpsvol->TPSVOL_iDrv);                       /*  �����������Ƴ�              */

        API_SemaphoreMDelete(&ptpsvol->TPSVOL_hVolLock);                /*  ɾ������                    */

        __SHEAP_FREE(ptpsvol);                                          /*  �ͷž���ƿ�                */

        _DebugHandle(__LOGMESSAGE_LEVEL, "disk unmount ok.\r\n");

        return  (ERROR_NONE);

    } else {                                                            /*  ɾ���ļ���Ŀ¼              */
        if (__blockIoDevFlag(ptpsvol->TPSVOL_iDrv) == O_RDONLY) {       /*  �˾�д����, ����ֻ��״̬    */
            _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
            return  (PX_ERROR);
        }

        if (__fsCheckFileName(pcName) < 0) {                            /*  ����ļ����Ƿ�Ϸ�          */
            _ErrorHandle(ENOENT);                                       /*  �ļ�δ�ҵ�                  */
            return  (PX_ERROR);
        }

        if (__TPS_VOL_LOCK(ptpsvol) != ERROR_NONE) {
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }

        if (ptpsvol->TPSVAL_bValid == LW_FALSE) {                       /*  �����ڱ�ж��                */
            __TPS_VOL_UNLOCK(ptpsvol);
            _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
            return  (PX_ERROR);
        }

        iErr = tpsFsOpen(ptpsvol->TPSVOL_tpsFsVol, pcName, 0,
                           0, &pcTail, &tpsfile.TPSFIL_pinode);
        if (iErr != ERROR_NONE) {
             __TPS_VOL_UNLOCK(ptpsvol);
             _ErrorHandle(iErr);
             return  (PX_ERROR);
        }
        tpsfile.TPSFIL_ptpsvol = ptpsvol;

        if (S_ISLNK(tpsFsGetmod(tpsfile.TPSFIL_pinode)) &&
            pcTail && lib_strlen(pcTail)) {                             /* ���һ�������ļ�ֱ��ɾ��     */
            PCHAR   pcSymfile = pcTail;
            PCHAR   pcPrefix;

            if (pcSymfile != pcName) {
                pcSymfile--;
            }

            while (pcSymfile != pcName && *pcSymfile == PX_DIVIDER) {
                pcSymfile--;
            }

            while (pcSymfile != pcName && *pcSymfile != PX_DIVIDER) {
                pcSymfile--;
            }

            if (pcSymfile == pcName) {
                pcPrefix = LW_NULL;                                     /*  û��ǰ׺                    */

            } else {
                pcPrefix = pcName;
                *pcSymfile = PX_EOS;
            }

            if (pcTail && lib_strlen(pcTail)) {
                if (__tpsFsPathBuildLink(&tpsfile, pcName, PATH_MAX + 1,
                                         pcPrefix, pcTail) == ERROR_NONE) {
                                                                        /*  ��������Ŀ��                */
                    tpsFsClose(tpsfile.TPSFIL_pinode);
                    __TPS_VOL_UNLOCK(ptpsvol);
                    return  (FOLLOW_LINK_TAIL);
                }
            }
        }

        if (S_ISDIR(tpsFsGetmod(tpsfile.TPSFIL_pinode))) {              /* Ŀ¼�ǿղ�����ɾ��           */
            PTPS_ENTRY    pentry;

            iErr = tpsFsReadDir(tpsfile.TPSFIL_pinode, LW_TRUE, MAX_BLK_NUM, &pentry);
            if (iErr != ENOENT) {
                if (pentry) {
                    tpsFsEntryFree(pentry);
                    iErr = ENOTEMPTY;
                }
                
                tpsFsClose(tpsfile.TPSFIL_pinode);
                __TPS_VOL_UNLOCK(ptpsvol);
                _ErrorHandle(iErr);
                return  (PX_ERROR);
            }
        }

        tpsFsClose(tpsfile.TPSFIL_pinode);

        iErr = tpsFsRemove(ptpsvol->TPSVOL_tpsFsVol, pcName);           /*  ִ��ɾ������                */

        __TPS_VOL_UNLOCK(ptpsvol);

        if (iErr != ERROR_NONE) {
            _ErrorHandle(iErr);
            return  (PX_ERROR);                                         /*  ɾ��ʧ��                    */
        
        } else {
            return  (ERROR_NONE);                                       /*  ɾ����ȷ                    */
        }
    }
}
/*********************************************************************************************************
** ��������: __tpsFsClose
** ��������: TPS FS close ����
** �䡡��  : pfdentry         �ļ����ƿ�
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE     ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    PTPS_VOLUME   ptpsvol  = ptpsfile->TPSFIL_ptpsvol;
    BOOL          bFree    = LW_FALSE;
    BOOL          bRemove  = LW_FALSE;

    if (ptpsfile) {
        if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
            _ErrorHandle(ENXIO);                                        /*  �豸����                    */
            return  (PX_ERROR);
        }

        if (API_IosFdNodeDec(&ptpsvol->TPSVOL_plineFdNodeHeader,
                             pfdnode, &bRemove) == 0) {                 /*  fd_node �Ƿ���ȫ�ͷ�        */
            if (ptpsfile->TPSFIL_pinode) {
                if (bRemove && S_ISDIR(tpsFsGetmod(ptpsfile->TPSFIL_pinode))) {
                    PTPS_ENTRY    pentry;
                    
                    if (tpsFsReadDir(ptpsfile->TPSFIL_pinode, LW_TRUE, MAX_BLK_NUM, &pentry) != ENOENT) {
                        if (pentry) {
                            tpsFsEntryFree(pentry);
                        }

                        bRemove = LW_FALSE;
                    }
                }
                tpsFsClose(ptpsfile->TPSFIL_pinode);
                bFree = LW_TRUE;
            }
        }

        LW_DEV_DEC_USE_COUNT(&ptpsvol->TPSVOL_devhdrHdr);               /*  ���¼�����                  */

        if (bRemove) {
            tpsFsRemove(ptpsvol->TPSVOL_tpsFsVol, ptpsfile->TPSFIL_cName);
        }

        __TPS_FILE_UNLOCK(ptpsfile);

        if (bFree) {
            __SHEAP_FREE(ptpsfile);                                     /*  �ͷ��ڴ�                    */
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __tpsFsRead
** ��������: TPS FS read ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __tpsFsRead (PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer,
                             size_t         stMaxBytes)
{
    PLW_FD_NODE   pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE     ptpsfile  = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    TPS_SIZE_T    szReadNum = 0;
    errno_t       iErr      = ERROR_NONE;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType != __TPS_FILE_TYPE_NODE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (stMaxBytes) {
        iErr = tpsFsRead(ptpsfile->TPSFIL_pinode, (PUCHAR)pcBuffer,
                         pfdentry->FDENTRY_oftPtr, stMaxBytes, &szReadNum);
        if ((iErr == ERROR_NONE) && (szReadNum > 0)) {
            pfdentry->FDENTRY_oftPtr += (off_t)szReadNum;               /*  �����ļ�ָ��                */
        }
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(iErr);
    return  ((ssize_t)szReadNum);
}
/*********************************************************************************************************
** ��������: __tpsFsPRead
** ��������: TPS FS pread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __tpsFsPRead (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stMaxBytes,
                              off_t         oftPos)
{
    PLW_FD_NODE   pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE     ptpsfile  = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    TPS_SIZE_T    szReadNum = 0;
    errno_t       iErr      = ERROR_NONE;

    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    if (ptpsfile->TPSFIL_iFileType != __TPS_FILE_TYPE_NODE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (stMaxBytes) {
        iErr = tpsFsRead(ptpsfile->TPSFIL_pinode, (PUCHAR)pcBuffer,
                         oftPos, stMaxBytes, &szReadNum);
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(iErr);
    return  ((ssize_t)szReadNum);
}
/*********************************************************************************************************
** ��������: __tpsFsWrite
** ��������: TPS FS write ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __tpsFsWrite (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE     ptpsfile   = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    TPS_SIZE_T    szWriteNum = 0;
    errno_t       iErr       = ERROR_NONE;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    if (ptpsfile->TPSFIL_iFileType != __TPS_FILE_TYPE_NODE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  ׷��ģʽ                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  �ƶ���дָ�뵽ĩβ          */
    }

    if (stNBytes) {                                                     /*  �Զ���չ�ļ�                */
        if (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < pfdentry->FDENTRY_oftPtr) {
            UINT uiBufSize = ptpsfile->TPSFIL_pinode->IND_psb->SB_uiBlkSize;
            size_t szWrite;
            PUCHAR pucZoreBuf = (PUCHAR)__SHEAP_ALLOC(uiBufSize);
            if (pucZoreBuf == LW_NULL) {
                __TPS_FILE_UNLOCK(ptpsfile);
                _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
                _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
                return  (PX_ERROR);
            }

            lib_bzero(pucZoreBuf, uiBufSize);
            while (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < pfdentry->FDENTRY_oftPtr) {
                szWrite = (size_t)min(uiBufSize, pfdentry->FDENTRY_oftPtr - tpsFsGetSize(ptpsfile->TPSFIL_pinode));
                if (tpsFsWrite(ptpsfile->TPSFIL_pinode, pucZoreBuf,
                               tpsFsGetSize(ptpsfile->TPSFIL_pinode),
                               szWrite, &szWriteNum) != ERROR_NONE) {
                    break;
                }
            }
            __SHEAP_FREE(pucZoreBuf);

            pfdnode->FDNODE_oftSize = tpsFsGetSize(ptpsfile->TPSFIL_pinode);
            if (pfdnode->FDNODE_oftSize < pfdentry->FDENTRY_oftPtr) {
                __TPS_FILE_UNLOCK(ptpsfile);
                _ErrorHandle(ENOSPC);
                return  (PX_ERROR);
            }
        }

        iErr = tpsFsWrite(ptpsfile->TPSFIL_pinode, (PUCHAR)pcBuffer,
                          pfdentry->FDENTRY_oftPtr, stNBytes, &szWriteNum);
        if ((iErr == ERROR_NONE) && (szWriteNum > 0)) {
            pfdentry->FDENTRY_oftPtr += (off_t)szWriteNum;              /*  �����ļ�ָ��                */
            pfdnode->FDNODE_oftSize   = tpsFsGetSize(ptpsfile->TPSFIL_pinode);
        }
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(iErr);
    return  ((ssize_t)szWriteNum);
}
/*********************************************************************************************************
** ��������: __tpsFsPWrite
** ��������: TPS FS pwrite ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __tpsFsPWrite (PLW_FD_ENTRY  pfdentry,
                               PCHAR         pcBuffer,
                               size_t        stNBytes,
                               off_t         oftPos)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE     ptpsfile   = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    TPS_SIZE_T    szWriteNum = 0;
    errno_t       iErr       = ERROR_NONE;

    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    if (ptpsfile->TPSFIL_iFileType != __TPS_FILE_TYPE_NODE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (stNBytes) {
        if (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < oftPos) {           /*  �Զ���չ�ļ�                */
            UINT uiBufSize = ptpsfile->TPSFIL_pinode->IND_psb->SB_uiBlkSize;
            size_t szWrite;
            PUCHAR pucZoreBuf = (PUCHAR)__SHEAP_ALLOC(uiBufSize);
            if (pucZoreBuf == LW_NULL) {
                __TPS_FILE_UNLOCK(ptpsfile);
                _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
                _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
                return  (PX_ERROR);
            }

            lib_bzero(pucZoreBuf, uiBufSize);
            while (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < oftPos) {
                szWrite = (size_t)min(uiBufSize, oftPos - tpsFsGetSize(ptpsfile->TPSFIL_pinode));
                if (tpsFsWrite(ptpsfile->TPSFIL_pinode, pucZoreBuf,
                               tpsFsGetSize(ptpsfile->TPSFIL_pinode),
                               szWrite, &szWriteNum) != ERROR_NONE) {
                    break;
                }
            }
            __SHEAP_FREE(pucZoreBuf);

            pfdnode->FDNODE_oftSize = tpsFsGetSize(ptpsfile->TPSFIL_pinode);
            if (pfdnode->FDNODE_oftSize < oftPos) {
                __TPS_FILE_UNLOCK(ptpsfile);
                _ErrorHandle(ENOSPC);
                return  (PX_ERROR);
            }
        }

        iErr = tpsFsWrite(ptpsfile->TPSFIL_pinode, (PUCHAR)pcBuffer,
                          oftPos, stNBytes, &szWriteNum);
        if ((iErr == ERROR_NONE) && (szWriteNum > 0)) {
            pfdnode->FDNODE_oftSize = tpsFsGetSize(ptpsfile->TPSFIL_pinode);
        }
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(iErr);
    return  ((ssize_t)szWriteNum);
}
/*********************************************************************************************************
** ��������: __tpsFsSeek
** ��������: TPS FS �ļ���λ
** �䡡��  : pfdentry            �ļ����ƿ�
**           oftPos              ��λ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsSeek (PLW_FD_ENTRY  pfdentry, off_t  oftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE      ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType != __TPS_FILE_TYPE_NODE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    pfdentry->FDENTRY_oftPtr = oftPos;

    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsReadDir
** ��������: TPS FS ���ָ��Ŀ¼��Ϣ
** �䡡��  : pfdentry            �ļ����ƿ�
**           dir                 Ŀ¼�ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE      ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    PTPS_ENTRY     pentry;
    INT            iErr     = ERROR_NONE;
    TPS_OFF_T      off      = 0;
    BOOL           bHash    = LW_TRUE;

    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_NODE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }

    /*
     *  dir_posתbHash + off
     */
    if (dir->dir_pos == 0) {
        off   = MAX_BLK_NUM;
        bHash = LW_TRUE;
    
    } else {
        bHash = (dir->dir_pos & 0x80000000) ? LW_TRUE : LW_FALSE;
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        off = (TPS_OFF_T)DIR_RESV_DATA_PV0(dir);
#else
        off = (UINT)DIR_RESV_DATA_PV1(dir);
        off <<= 32;
        off += (UINT)DIR_RESV_DATA_PV0(dir);
#endif
    }

    tpsFsReadDir(ptpsfile->TPSFIL_pinode, bHash, off, &pentry);
    if (!pentry) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    /*
     *  bHash + offתdir_pos
     */
    dir->dir_pos++;
    dir->dir_pos &= 0x7FFFFFFF;
    if (pentry->ENTRY_bInHash) {
        dir->dir_pos |= 0x80000000;
        if (pentry->ENTRY_offset > 0) {
            off = pentry->ENTRY_offset - 1;
        }
    
    } else {
        off = pentry->ENTRY_offset + pentry->ENTRY_uiLen;
    }

#if LW_CFG_CPU_WORD_LENGHT == 64
    DIR_RESV_DATA_PV0(dir) = (PVOID)off;
#else
    DIR_RESV_DATA_PV0(dir) = (PVOID)(UINT)off;
    DIR_RESV_DATA_PV1(dir) = (PVOID)(UINT)(off >> 32);
#endif

    dir->dir_dirent.d_type = IFTODT(tpsFsGetmod(pentry->ENTRY_pinode));
    dir->dir_dirent.d_shortname[0] = PX_EOS;
    lib_strlcpy(dir->dir_dirent.d_name,
                pentry->ENTRY_pcName,
                sizeof(dir->dir_dirent.d_name));

    tpsFsEntryFree(pentry);
    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramFsLStat
** ��������: ramFs stat ����
** �䡡��  : ptpsfs           tpsfs �ļ�ϵͳ
**           pcName           �ļ���
**           pstat            �ļ�״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsLStat (PTPS_VOLUME  ptpsfs, PCHAR  pcName, struct stat *pstat)
{
    PTPS_INODE          ptpsinode;
    PTPS_SUPER_BLOCK    psb     = ptpsfs->TPSVOL_tpsFsVol;
    errno_t             iErr    = ERROR_NONE;

    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_VOL_LOCK(ptpsfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    iErr = tpsFsOpen(ptpsfs->TPSVOL_tpsFsVol, pcName,
                     0, 0, LW_NULL, &ptpsinode);
    if (iErr == ERROR_NONE) {
        tpsFsStat(&ptpsfs->TPSVOL_devhdrHdr, LW_NULL, ptpsinode, pstat);

    } else if (__STR_IS_ROOT(pcName)) {
        tpsFsStat(&ptpsfs->TPSVOL_devhdrHdr, psb, LW_NULL, pstat);

    } else {
        __TPS_VOL_UNLOCK(ptpsfs);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    tpsFsClose(ptpsinode);
    __TPS_VOL_UNLOCK(ptpsfs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsStatGet
** ��������: TPS FS ����ļ�״̬������
** �䡡��  : pfdentry            �ļ����ƿ�
**           pstat               stat �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsStatGet (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE         pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile  = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    PTPS_INODE          ptpsinode = ptpsfile->TPSFIL_pinode;
    PTPS_SUPER_BLOCK    psb       = ptpsfile->TPSFIL_ptpsvol->TPSVOL_tpsFsVol;

    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    if (__STR_IS_ROOT(ptpsfile->TPSFIL_cName)) {                        /*  Ϊ��Ŀ¼                    */
        tpsFsStat(pfdentry->FDENTRY_pdevhdrHdr, psb, LW_NULL, pstat);

    } else {
        tpsFsStat(pfdentry->FDENTRY_pdevhdrHdr, LW_NULL, ptpsinode, pstat);
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsFormat
** ��������: TPS FS ��ʽ��ý��
** �䡡��  : pfdentry            �ļ����ƿ�
**           lArg                ����
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsFormat (PLW_FD_ENTRY  pfdentry, LONG  lArg)
{
    PLW_FD_NODE          pfdnode         = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE            ptpsfile        = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    PTPS_VOLUME          ptpsvol         = ptpsfile->TPSFIL_ptpsvol;
    UINT                 uiMountFlag     = 0;
    errno_t              iErr            = ERROR_NONE;
    PLW_BLK_DEV          pblkd;

    if (!__STR_IS_ROOT(ptpsfile->TPSFIL_cName)) {                       /*  ����Ƿ�Ϊ�豸�ļ�          */
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)ptpsvol) > 1) {              /*  ����Ƿ������ڹ������ļ�    */
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(ERROR_IOS_TOO_MANY_OPEN_FILES);                    /*  �������ļ���              */
        return  (PX_ERROR);
    }
    
    if (ptpsvol->TPSVOL_tpsFsVol) {
        tpsFsUnmount(ptpsvol->TPSVOL_tpsFsVol);                         /*  ж�ع��ص��ļ�ϵͳ          */
        ptpsvol->TPSVOL_tpsFsVol = LW_NULL;
    }

    (VOID)__blockIoDevIoctl(ptpsvol->TPSVOL_iDrv, FIOCANCEL, 0);        /*  CACHE ֹͣ (����д����)     */

    iErr = __blockIoDevIoctl(ptpsvol->TPSVOL_iDrv,
                             FIODISKFORMAT,
                             lArg);                                     /*  �ײ��ʽ��                  */
    if (iErr < 0) {
        if (__blockIoDevIsLogic(ptpsvol->TPSVOL_iDrv)) {
            iErr = tpsFsFormat(&ptpsvol->TPSVOL_dev, TPS_MIN_BLK_SIZE); /*  �˴���Ϊ�߼����̲���Ҫ������*/
        
        } else {
            iErr = ENXIO;
        }
    }
    
    if (iErr) {                                                         /*  ��ʽ��ʧ��                  */
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    pblkd = __blockIoDevGet(ptpsvol->TPSVOL_iDrv);
    if (pblkd) {
        if (pblkd->BLKD_iFlag == O_RDONLY) {                            /*  ����ģʽת����tpsfsģʽ     */
            uiMountFlag = TPS_MOUNT_FLAG_READ;

        } else if (pblkd->BLKD_iFlag == O_WRONLY) {
            uiMountFlag = TPS_MOUNT_FLAG_WRITE;

        } else {
            uiMountFlag = TPS_MOUNT_FLAG_WRITE | TPS_MOUNT_FLAG_READ;
        }

        iErr = tpsFsMount(&ptpsvol->TPSVOL_dev, uiMountFlag, &ptpsvol->TPSVOL_tpsFsVol);
        if (iErr != ERROR_NONE) {                                       /*  �����ļ�ϵͳ                */
            __TPS_FILE_UNLOCK(ptpsfile);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "mount tpsfs failed.\r\n");
            _ErrorHandle(iErr);
            return  (PX_ERROR);
        }
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    if (iErr) {                                                         /*  ����                        */
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __tpsFsRename
** ��������: TPS FS ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           pcNewName           ������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : Ϊ�˼�С�ڴ��Ƭ�Ĳ���, ���ļ����Ϳ��ƽṹ������ͬһ�ڴ�ֶ�, ��ʱ�޷����� ptpsfile �����
             �ļ���, �����ļ�����ȫ����, ��ʱ, ���ļ������ٴβ���, ����, �û�������� rename() ���������
             �˲���, ���ܶ������� ioctl() ������.
*********************************************************************************************************/
static INT  __tpsFsRename (PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
    errno_t                 iErr    = ERROR_NONE;

             CHAR           cNewPath[PATH_MAX + 1];
    REGISTER PCHAR          pcNewPath = &cNewPath[0];
             PLW_FD_NODE    pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
             PTPS_FILE      ptpsfile   = (PTPS_FILE)pfdnode->FDNODE_pvFile;
             PTPS_VOLUME    ptpsvolNew;

    if (__STR_IS_ROOT(ptpsfile->TPSFIL_cName)) {                        /*  ����Ƿ�Ϊ�豸�ļ�          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  ��֧���豸������            */
        return  (PX_ERROR);
    }
    if (pcNewName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    if (__STR_IS_ROOT(pcNewName)) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_DEV) {            /*  �豸�����ܸ���            */
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    if (ioFullFileNameGet(pcNewName,
                          (LW_DEV_HDR **)&ptpsvolNew,
                          cNewPath) != ERROR_NONE) {                    /*  �����Ŀ¼·��              */
        __TPS_FILE_UNLOCK(ptpsfile);
        return  (PX_ERROR);
    }
    
    if (ptpsvolNew != ptpsfile->TPSFIL_ptpsvol) {                       /*  ����Ϊͬһ����              */
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    if (cNewPath[0] == PX_DIVIDER) {                                    /*  tpsFs �ļ�ϵͳ rename �ĵ�  */
        pcNewPath++;                                                    /*  2 ������������'/'Ϊ��ʼ�ַ� */
    }
    
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_DIR) {
        if (_PathMoveCheck(ptpsfile->TPSFIL_cName, pcNewPath)) {
            __TPS_FILE_UNLOCK(ptpsfile);
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }
    
    iErr = tpsFsMove(ptpsfile->TPSFIL_ptpsvol->TPSVOL_tpsFsVol,
                     ptpsfile->TPSFIL_cName, pcNewPath);
    if (iErr) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(iErr);                                             /*  ����������                  */
        return  (PX_ERROR);
    }
    
    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsWhere
** ��������: TPS FS ����ļ���ǰ��дָ��λ�� (ʹ�ò�����Ϊ����ֵ, �� FIOWHERE ��Ҫ�����в�ͬ)
** �䡡��  : pfdentry            �ļ����ƿ�
**           poftPos             ��дָ��λ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE      ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_NODE) {
        *poftPos = pfdentry->FDENTRY_oftPtr;
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    
    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(ulError);
    return  (iError);
 }
/*********************************************************************************************************
** ��������: __tpsFsNRead
** ��������: TPS FS ����ļ�ʣ�����Ϣ��
** �䡡��  : pfdentry            �ļ����ƿ�
**           piPos               ʣ��������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsNRead (PLW_FD_ENTRY  pfdentry, INT  *piPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE      ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;

    if (piPos == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_NODE) {
        *piPos = (INT)(pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr);
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    
    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __tpsFsNRead64
** ��������: TPS FS ����ļ�ʣ�����Ϣ��
** �䡡��  : pfdentry            �ļ����ƿ�
**           poftPos             ʣ��������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE      ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_NODE) {
        *poftPos = pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr;
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __tpsFsTruncate
** ��������: TPS FS �����ļ���С
** �䡡��  : pfdentry            �ļ����ƿ�
**           oftSize             �ļ���С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsTruncate (PLW_FD_ENTRY  pfdentry, off_t  oftSize)
{
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    INT                 iError   = ERROR_NONE;
    errno_t             iErr     = ERROR_NONE;

    if (oftSize < 0) {                                                  /*  TPS �ļ������� 4GB ��       */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_NODE) {
        if (oftSize > tpsFsGetSize(ptpsfile->TPSFIL_pinode)) {          /*  �Զ���չ�ļ�                */
            UINT        uiBufSize  = ptpsfile->TPSFIL_pinode->IND_psb->SB_uiBlkSize;
            size_t      szWrite;
            TPS_SIZE_T  szWriteNum;
            PUCHAR      pucZoreBuf = (PUCHAR)__SHEAP_ALLOC(uiBufSize);

            if (pucZoreBuf == LW_NULL) {
                __TPS_FILE_UNLOCK(ptpsfile);
                _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
                _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
                return  (PX_ERROR);
            }

            while (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < oftSize) {
                szWrite = (size_t)min(uiBufSize, oftSize -
                                                 tpsFsGetSize(ptpsfile->TPSFIL_pinode));
                if (tpsFsWrite(ptpsfile->TPSFIL_pinode, pucZoreBuf,
                               tpsFsGetSize(ptpsfile->TPSFIL_pinode),
                               szWrite, &szWriteNum) != ERROR_NONE) {
                    break;
                }
            }
            
            pfdnode->FDNODE_oftSize = tpsFsGetSize(ptpsfile->TPSFIL_pinode);

            __SHEAP_FREE(pucZoreBuf);
            if (tpsFsGetSize(ptpsfile->TPSFIL_pinode) < oftSize) {
                __TPS_FILE_UNLOCK(ptpsfile);
                _ErrorHandle(EFBIG);
                return  (PX_ERROR);
            }

            __TPS_FILE_UNLOCK(ptpsfile);
            return  (ERROR_NONE);
        }

        iErr = tpsFsTrunc(ptpsfile->TPSFIL_pinode, oftSize);
        pfdnode->FDNODE_oftSize = tpsFsGetSize(ptpsfile->TPSFIL_pinode);
        iError = (iErr == ERROR_NONE ? ERROR_NONE : PX_ERROR);

    } else {
        iErr   = EISDIR;
        iError = PX_ERROR;
    }
    
    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(iErr);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __tpsFsSync
** ��������: TPS FS ���ļ�����д�����
** �䡡��  : pfdentry            �ļ����ƿ�
**           bFlushCache         �Ƿ�ͬʱ��� CACHE
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsSync (PLW_FD_ENTRY  pfdentry, BOOL  bFlushCache)
{
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    INT                 iError   = ERROR_NONE;
    errno_t             iErr     = ERROR_NONE;

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    /*
     *  ֻ�����ļ���д����д�����������ϲ㴦��
     */
    if (ptpsfile->TPSFIL_iFileType == __TPS_FILE_TYPE_NODE) {
        iErr = tpsFsSync(ptpsfile->TPSFIL_pinode);
        if (iErr != ERROR_NONE) {
            __TPS_FILE_UNLOCK(ptpsfile);
            _ErrorHandle(iErr);                                         /*  ͬ���ļ�ϵͳ����            */
            return  (PX_ERROR);
        }
    }

    if (bFlushCache) {
        iError = __blockIoDevIoctl(ptpsfile->TPSFIL_ptpsvol->TPSVOL_iDrv,
                                   FIOSYNC, 0);                         /*  ��� CACHE ��д����         */
        if (iError < 0) {
            iErr = ERROR_IO_DEVICE_ERROR;                               /*  �豸����, �޷����          */
        }
    }
    __TPS_FILE_UNLOCK(ptpsfile);

    _ErrorHandle(iErr);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __tpsFsChmod
** ��������: TPS FS �����ļ�����
** �䡡��  : pfdentry            �ļ����ƿ�
**           iMode               �µ� mode
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsChmod (PLW_FD_ENTRY  pfdentry, INT  iMode)
{
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    errno_t             iErr     = ERROR_NONE;

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    iErr = tpsFsChmod(ptpsfile->TPSFIL_pinode, iMode);
    if (iErr != ERROR_NONE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    __TPS_FILE_UNLOCK(ptpsfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fatFsTimeset
** ��������: TPS FS �����ļ�ʱ��
** �䡡��  : pfdentry            �ļ����ƿ�
**           utim                utimbuf �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : Ŀǰ�˺����������޸�ʱ��.
*********************************************************************************************************/
static INT  __tpsFsTimeset (PLW_FD_ENTRY  pfdentry, struct utimbuf  *utim)
{
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    errno_t             iErr     = ERROR_NONE;

    if (!utim) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__STR_IS_ROOT(ptpsfile->TPSFIL_cName)) {                        /*  ����Ƿ�Ϊ�豸�ļ�          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  ��֧���豸������            */
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    iErr = tpsFsChtime(ptpsfile->TPSFIL_pinode, utim);
    if (iErr != ERROR_NONE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsChown
** ��������: ramfs chown ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           pusr                �µ������û�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsChown (PLW_FD_ENTRY  pfdentry, LW_IO_USR  *pusr)
{
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    errno_t             iErr     = ERROR_NONE;

    if (!pusr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    iErr = tpsFsChown(ptpsfile->TPSFIL_pinode, pusr->IOU_uid, pusr->IOU_gid);
    if (iErr != ERROR_NONE) {
        __TPS_FILE_UNLOCK(ptpsfile);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsStatfsGet
** ��������: TPS FS ����ļ�ϵͳ״̬������
** �䡡��  : pfdentry            �ļ����ƿ�
**           pstatfs             statfs �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsStatfsGet (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    PLW_FD_NODE         pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE           ptpsfile = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    PTPS_SUPER_BLOCK    psb      = ptpsfile->TPSFIL_ptpsvol->TPSVOL_tpsFsVol;

    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_FILE_LOCK(ptpsfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    lib_bzero(pstatfs, sizeof(struct statfs));

    tpsFsStatfs(psb, pstatfs);

    if (ptpsfile->TPSFIL_ptpsvol->TPSVOL_iFlag == O_RDONLY) {
        pstatfs->f_flag |= ST_RDONLY;
    }

    __TPS_FILE_UNLOCK(ptpsfile);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsSymlink
** ��������: tpsFs �������������ļ�
** �䡡��  : ptpsfs              tpsfs �ļ�ϵͳ
**           pcName              ����ԭʼ�ļ���
**           pcLinkDst           ����Ŀ���ļ���
**           stMaxSize           �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsSymlink (PTPS_VOLUME   ptpsfs,
                            PCHAR         pcName,
                            CPCHAR        pcLinkDst)
{
    errno_t             iErr     = ERROR_NONE;
    PTPS_INODE          pinode;
    TPS_SIZE_T          szRead;

    if (!pcName || !pcLinkDst) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__fsCheckFileName(pcName)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_VOL_LOCK(ptpsfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    iErr = tpsFsOpen(ptpsfs->TPSVOL_tpsFsVol, pcName,
                     O_CREAT | O_EXCL, S_IFLNK | DEFAULT_SYMLINK_PERM,
                     LW_NULL, &pinode);
    if (iErr != ERROR_NONE) {
        __TPS_VOL_UNLOCK(ptpsfs);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    iErr = tpsFsWrite(pinode, (PUCHAR)pcLinkDst, 0, lib_strlen(pcLinkDst), &szRead);
    if ((iErr != ERROR_NONE) || (szRead < lib_strlen(pcLinkDst))) {
        __TPS_VOL_UNLOCK(ptpsfs);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }
    
    tpsFsClose(pinode);

    __TPS_VOL_UNLOCK(ptpsfs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFsReadlink
** ��������: tpsFs ��ȡ���������ļ�����
** �䡡��  : ptpsfs              tpsfs �ļ�ϵͳ
**           pcName              ����ԭʼ�ļ���
**           pcLinkDst           ����Ŀ���ļ���
**           stMaxSize           �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t __tpsFsReadlink (PTPS_VOLUME   ptpsfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize)
{
    errno_t     iErr     = ERROR_NONE;
    PTPS_INODE  pinode;
    TPS_SIZE_T  szLen;

    if (!pcName || !pcLinkDst || !stMaxSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__TPS_VOL_LOCK(ptpsfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    iErr = tpsFsOpen(ptpsfs->TPSVOL_tpsFsVol, pcName, 0, 0, LW_NULL, &pinode);
    if ((iErr != ERROR_NONE) || !S_ISLNK(tpsFsGetmod(pinode))) {
        __TPS_VOL_UNLOCK(ptpsfs);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    iErr = tpsFsRead(pinode, (PUCHAR)pcLinkDst, 0, stMaxSize, &szLen);
    if (iErr != ERROR_NONE) {
        tpsFsClose(pinode);
        __TPS_VOL_UNLOCK(ptpsfs);
        _ErrorHandle(iErr);
        return  (PX_ERROR);
    }

    tpsFsClose(pinode);

    __TPS_VOL_UNLOCK(ptpsfs);

    return  ((ssize_t)szLen);
}
/*********************************************************************************************************
** ��������: __tpsFsIoctl
** ��������: TPS FS ioctl ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           request,            ����
**           arg                 �������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tpsFsIoctl (PLW_FD_ENTRY  pfdentry,
                          INT           iRequest,
                          LONG          lArg)
{
    PLW_FD_NODE    pfdnode      = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PTPS_FILE      ptpsfile     = (PTPS_FILE)pfdnode->FDNODE_pvFile;
    PTPS_VOLUME    ptpsvol      = ptpsfile->TPSFIL_ptpsvol;
    PTPS_INODE     ptpsinode    = ptpsfile->TPSFIL_pinode;
    off_t          oftTemp  = 0;                                        /*  ��ʱ����                    */
    INT            iError;

    switch (iRequest) {                                                 /*  ֻ���ļ��ж�                */

    case FIOCONTIG:
    case FIOTRUNC:
    case FIOLABELSET:
    case FIOATTRIBSET:
    case FIODISKFORMAT:
        if ((pfdentry->FDENTRY_iFlag & O_ACCMODE) == O_RDONLY) {
            _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
            return  (PX_ERROR);
        }
        if (ptpsvol->TPSVOL_iFlag == O_RDONLY) {
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }

    switch (iRequest) {                                                 /*  ֻ���ļ�ϵͳ�ж�            */

    case FIORENAME:
    case FIOTIMESET:
    case FIOCHMOD:
        if ((ptpsvol->TPSVOL_iFlag == O_RDONLY) ||
            (tpsFsGetmod(ptpsinode) == O_RDONLY)) {
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }

    switch (iRequest) {

    case FIODISKFORMAT:                                                 /*  ���ʽ��                    */
        return  (__tpsFsFormat(pfdentry, lArg));
    
    case FIODISKINIT:                                                   /*  ���̳�ʼ��                  */
        return  (__blockIoDevIoctl(ptpsvol->TPSVOL_iDrv, FIODISKINIT, lArg));

    case FIORENAME:                                                     /*  �ļ�������                  */
        return  (__tpsFsRename(pfdentry, (PCHAR)lArg));

    /*
     *  FIOSEEK, FIOWHERE is 64 bit operate.
     */
    case FIOSEEK:                                                       /*  �ļ��ض�λ                  */
        oftTemp = *(off_t *)lArg;
        return  (__tpsFsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  ����ļ���ǰ��дָ��        */
        iError = __tpsFsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIONREAD:                                                      /*  ����ļ�ʣ���ֽ���          */
        return  (__tpsFsNRead(pfdentry, (INT *)lArg));

    case FIONREAD64:                                                    /*  ����ļ�ʣ���ֽ���          */
        iError = __tpsFsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
        return  (__tpsFsStatGet(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  ����ļ�ϵͳ״̬            */
        return  (__tpsFsStatfsGet(pfdentry, (struct statfs *)lArg));

    case FIOREADDIR:                                                    /*  ��ȡһ��Ŀ¼��Ϣ            */
        return  (__tpsFsReadDir(pfdentry, (DIR *)lArg));

    case FIOTIMESET:                                                    /*  �����ļ�ʱ��                */
        return  (__tpsFsTimeset(pfdentry, (struct utimbuf *)lArg));

    /*
     *  FIOTRUNC is 64 bit operate.
     */
    case FIOTRUNC:                                                      /*  �ı��ļ���С                */
        oftTemp = *(off_t *)lArg;
        return  (__tpsFsTruncate(pfdentry, oftTemp));

    case FIOSYNC:                                                       /*  ���ļ������д              */
    case FIODATASYNC:
        return  (__tpsFsSync(pfdentry, LW_TRUE /*  LW_FALSE  ???*/));

    case FIOFLUSH:
        return  (__tpsFsSync(pfdentry, LW_TRUE));                       /*  �ļ��뻺��ȫ����д          */

    case FIOCHMOD:
        return  (__tpsFsChmod(pfdentry, (INT)lArg));                    /*  �ı��ļ�����Ȩ��            */

    case FIOSETFL:                                                      /*  �����µ� flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);

    case FIOCHOWN:                                                      /*  �޸��ļ�������ϵ            */
        return  (__tpsFsChown(pfdentry, (LW_IO_USR *)lArg));

    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "True Power Safe FileSystem";
        return  (ERROR_NONE);

    case FIOFSGETFL:                                                    /*  ��ȡ�ļ�ϵͳȨ��            */
        if ((INT *)lArg == LW_NULL) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        *(INT *)lArg = ptpsvol->TPSVOL_iFlag;
        return  (ERROR_NONE);
        
    case FIOFSSETFL:                                                    /*  �����ļ�ϵͳȨ��            */
        if (geteuid()) {
            _ErrorHandle(EACCES);
            return  (PX_ERROR);
        }
        if (((INT)lArg != O_RDONLY) && ((INT)lArg != O_RDWR)) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        ptpsvol->TPSVOL_iFlag = (INT)lArg;
        KN_SMP_WMB();
        return  (ERROR_NONE);

    case FIOGETFORCEDEL:                                                /*  ǿ��ж���豸�Ƿ�����      */
        *(BOOL *)lArg = ptpsvol->TPSVOL_bForceDelete;
        return  (ERROR_NONE);

    case FIOSETFORCEDEL:                                                /*  ����ǿ��ж��ʹ��            */
        ptpsvol->TPSVOL_bForceDelete = (BOOL)lArg;
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

#ifdef __SYLIXOS_DEBUG
    case TPSFS_IOCTRL_INODE_PRINT:                                      /*  ��ӡinode��Ϣ�����ڵ���     */
        if ((BOOL)lArg) {
            tpsFsInodeDump(ptpsvol->TPSVOL_tpsFsVol->SB_pinodeSpaceMng);/*  ��ӡ���̿ռ����ڵ���Ϣ    */
        
        } else {
            tpsFsInodeDump(ptpsinode);                                  /*  ��ӡ��ͨ�ڵ���Ϣ            */
        }
        return  (ERROR_NONE);
#endif                                                                  /*  __SYLIXOS_DEBUG             */

    default:                                                            /*  �޷�ʶ�������              */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (PX_ERROR);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
