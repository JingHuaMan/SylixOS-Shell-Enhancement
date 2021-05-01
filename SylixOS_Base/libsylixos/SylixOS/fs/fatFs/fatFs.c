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
** ��   ��   ��: fatFs.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 26 ��
**
** ��        ��: FAT �ļ�ϵͳ�� IO ϵͳ�ӿڲ���.

** BUG
2008.12.02  �ڴ��� FAT �豸ʱ, ��Ҫ��λ�豸.
2008.12.04  f_chmod_ex() ���ĸ�������ҪΪ 0xFF
2009.02.19  __fatFsMkMode() ֧�� O_CREAT | O_EXCL.
2009.03.05  DWORD ����ȷ��Ϊ 32 λ, ULONG Ϊ 32/64 ���ϸ��� CPU ����ȷ��.
2009.03.10  ������ errno ϵͳ, ��صĵط��������޸�.
2009.03.16  ���� __fatFsStatfsGet() ����.
            ��������ʱ, ��Ҫ���ȴ򿪵�Դ.
            ж�ؾ�ʱ���Ȼ�д���� CACHE , Ȼ��ϵ�, ����ǿ��ƶ�������Ҫ����.
2009.03.21  ���ܳ���ʲô�������, �������ж�ؾ�, remove ���ﲻ���ж������������.
2009.03.27  ���ļ�ʱ, �����ֻ������ж�.
2009.04.17  �������ļ�ϵͳ, ͬʱ���˴����.
2009.04.17  Ŀǰ��֧�־��Ĳ���...
            FILINFO ���ļ���������Ҫ�ȳ�ʼ��.
2009.04.17  readdir ���볤�ļ���֧��.
2009.04.19  __fatFsTruncate() ��ȷ����ʱ�����ļ���дָ��λ�ò��仯.
2009.04.22  ��ж�ؾ�ʱ, ���ر������ļ�, ���ǽ��й��ļ���Ϊ�쳣״̬.
2009.05.02  __fatFsClusterSizeGet() ����Ϊ __fatFsClusterSizeCal() ��ʽ��ȷ�����̴ش�Сʱ, ��Ҫ����������
            С.
2009.06.05  �����ļ����Ᵽ��, ��һ���ļ���д��ʽ��, �����ܱ���. ���ļ�������ʽ��, ���ܱ���������
            �ķ�ʽ��. 
2009.06.10  �� FIOMOVE �� FIORENAME �ϲ�����.
            �����ļ�ʱ��ʱ, Ҫ����ļ��Ŀɶ�д��.
2009.06.18  �����ļ����жϵ�����, stat ����ʱ, mode ��Ҫ���ļ�����ϲ���.
2009.07.05  ʹ�� fsCommon �ṩ�ĺ��������ļ����ж�.
2009.07.06  ��ж�ش���ʱ, ��Ҫ������������ӽ��д���.
2009.07.09  ֧�� 64 bit �ļ�ָ��.
2009.08.26  readdir, pos Ϊ 0 ʱ, ������Ҫ rewind ����.
2009.09.03  ������ʱ, ��¼����ʱ����Ϣ.
2009.10.22  read write ����ֵΪ ssize_t.
2009.10.28  ����ƿ��Ա: FATVOL_fatfsVol ����ʱһ��Ҫ����! ����auto_mount()�ٵ�f_syncfs()����û�и�ʽ����
            ���̿��ܵ���ϵͳ����!
2009.11.18  ��������, ��ѡ�� errno ���� sylixos �ڲ� error ������� posix ����.
2009.12.01  �����˲���ϵͳע�ắ��.
2010.01.08  ��ϵͳ��ʹ���Զ�Ŀ¼ѹ��ʱ, �� FAT �򿪺͹ر�ʱ��Ҫʹ��Ŀ¼ѹ��.
2010.01.11  �����������ļ�����Ҫ��ѹ��.
2010.01.14  ֧�� FIOSETFL ����.
2010.03.09  ���� open �д���Ŀ¼ʱ���� f_sync �� bug!
2010.09.10  __fatFsReadDir() �м���� d_type �ֶε�֧��.
2011.03.06  ���� gcc 4.5.1 ��� warning.
2011.03.27  ����Դ����ļ�����֧������ж�.
2011.03.29  �� iosDevAddEx() ����ʱ��Ҫ��¼��������.
2011.06.14  __fatFsTruncate() �������ܸ���������ȷ���ݵ�����.
2011.08.11  ѹ��Ŀ¼�������ļ�ϵͳ���, ����ϵͳ�Ѵ�����.
2011.11.21  �����ļ�ϵͳ��Ӧ�Ľӿ�.
2012.03.10  ���� __fatFsReadDir() ����ȡ����ʱ���� ENOENT ����.
2012.06.29  �� inode �������� __filInfoToStat().
2012.08.16  ���� pread �� pwrite ����, ���ǲ��Ǻ�����, �Ѿ����� FatFs ���߼��� f_pread �� f_pwrite ����.
2012.09.01  ���벻��ǿ��ж�ؾ�ı���.
2012.09.25  fat ������ socket �����ļ�. (����ͨ�ļ�������)
2012.10.20  �������� errno.
2012.12.08  ע����ļ�ϵͳ����Ϊ vfat.
2012.12.14  __FAT_FILE_LOCK() ��Ҫ�жϷ���ֵ, �������Ӱ�ȫ, �ʺ��Ȳ�ξ�.
2013.01.08  fatfs ʹ������ NEW_1 �豸��������. ��������֧���ļ���.
            fat �е��ļ�Ψһ�ı�ž������ڵĴ� + Ŀ¼ƫ��, �������� 32bit + 16bit ����û�а취���� 32bit
            inode ���, ��������ʹ��һ�� hash ��, ���������Ѿ����������ļ�ͨ�� unique ������� inode ,
            �������ǰ�Ѿ����������ļ����ȡ���Ѿ�������� unique, �������Ա�֤ÿһ���ļ����ж�Ӧ��Ψһ
            �� unique ���, ���ܺ�ʱ��������ļ�, �����Ŷ���ȷ����, Ҳ��Ψһ��.
2013.01.18  ���ж�ֻ���ļ�ϵͳ���� O_CREAT �Ĳ������ܾ�.
2013.01.21  �ṩ shell �������� fat Ĭ�ϵ� uid gid.
2013.04.11  ����ж�ؾ�ʱ, uniq ������û��ж��.
2013.06.24  ���� fat �ļ�ϵͳ, ͬʱ֧�־��.
2014.06.24  FAT �豸 64 Ϊ���к�Ϊ -1.
2014.12.31  ֧�� ff10.c �ļ�ϵͳ.
2016.09.18  ֧�� exFAT �ļ�ϵͳ��ʽ.
2017.04.27  fat �ڲ�ʹ�� O_RDWR ��, ��ֹ���ش�ʱ�ڲ�Ȩ���жϴ���.
2017.09.22  ��ʽ��������ʹ�� exFAT ϵͳ, ��ֹ BIOS ����������.
2017.12.27  ���� rename ���� POSIX ��׼.
2019.01.16  ���� open ��� EEXIST ���� errno ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/unique/unique.h"
/*********************************************************************************************************
  SHELL
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "../SylixOS/shell/include/ttiny_shell.h"
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_FATFS_EN > 0)
#include "ff.h"
/*********************************************************************************************************
  �ڲ��ṹ (�����ʹ�� inode ���ܻ��Լ�ڴ�, �ӿ�����ٶ�)
*********************************************************************************************************/
#define __FAT_FILE_INOD_EN              1                               /*  �Ƿ����� FAT inode ����     */
#define __FAT_FILE_HASH_SIZE            64                              /*  inode hash size             */
#define __FAT_FILE_UNIQ_SIZE            1024                            /*  ���Լ�¼ 1024 * 8 �� inode  */

typedef struct {
    LW_DEV_HDR          FATVOL_devhdrHdr;                               /*  �豸ͷ                      */
    BOOL                FATVOL_bForceDelete;                            /*  �Ƿ�����ǿ��ж�ؾ�          */
    FATFS               FATVOL_fatfsVol;                                /*  �ļ�ϵͳ����Ϣ              */
    INT                 FATVOL_iDrv;                                    /*  ������λ��                  */
    BOOL                FATVAL_bValid;                                  /*  ��Ч�Ա�־                  */
    LW_OBJECT_HANDLE    FATVOL_hVolLock;                                /*  �������                    */
    
    LW_LIST_LINE_HEADER FATVOL_plineFdNodeHeader;                       /*  fd_node ����                */
    LW_LIST_LINE_HEADER FATVOL_plineHashHeader[__FAT_FILE_HASH_SIZE];   /*  �����Ѿ����������ļ� hash   */
    PLW_UNIQUE_POOL     FATVOL_puniqPool;                               /*  inode ������                */
    
    UINT32              FATVOL_uiTime;                                  /*  ����ʱ�� FAT ��ʽ         */
    INT                 FATVOL_iFlag;                                   /*  O_RDONLY or O_RDWR          */
} FAT_VOLUME;
typedef FAT_VOLUME     *PFAT_VOLUME;

typedef struct {
    PFAT_VOLUME         FATFIL_pfatvol;                                 /*  ���ھ���Ϣ                  */
    union {
        FIL             FFTM_file;                                      /*  �ļ�������Ϣ                */
        FATDIR          FFTM_fatdir;                                    /*  Ŀ¼������Ϣ                */
    } FATFIL_fftm;
    INT                 FATFIL_iFileType;                               /*  �ļ�����                    */
    UINT64              FATFIL_u64Uniq;                                 /*  64bits �� + Ŀ¼ƫ��        */
    ino_t               FATFIL_inode;                                   /*  inode ���                  */
    CHAR                FATFIL_cName[1];                                /*  �ļ���                      */
} FAT_FILE;
typedef FAT_FILE       *PFAT_FILE;
/*********************************************************************************************************
  ���� FAT �޷�ֱ�ӻ�ȡ inode ���, ����Ϊÿһ���������������ļ�����һ��ͳһ�ı�, ��¼��Ӧ���ļ� inode
  �ظ���ͬһ���ļ����ʹ��Ψһһ�� inode.
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        FATHIS_lineManage;
    UINT64              FATHIS_u64Uniq;                                 /*  64bits �� + Ŀ¼ƫ��        */
    ino_t               FATHIS_inode;                                   /*  inode ���                  */
} FAT_FILE_HIS;
typedef FAT_FILE_HIS   *PFAT_FILE_HIS;
/*********************************************************************************************************
  �ļ�����
*********************************************************************************************************/
#define __FAT_FILE_TYPE_NODE            0                               /*  open ���ļ�               */
#define __FAT_FILE_TYPE_DIR             1                               /*  open ��Ŀ¼               */
#define __FAT_FILE_TYPE_DEV             2                               /*  open ���豸               */
/*********************************************************************************************************
  FAT ���豸�����ļ�ϵͳ����
*********************************************************************************************************/
static INT              _G_iFatDrvNum      = PX_ERROR;
static PCHAR            _G_pcFat12FsString = "FAT12 FileSystem";
static PCHAR            _G_pcFat16FsString = "FAT16 FileSystem";
static PCHAR            _G_pcFat32FsString = "FAT32 FileSystem";
static PCHAR            _G_pcExFatFsString = "exFAT FileSystem";
/*********************************************************************************************************
  FAT Ĭ�� ugid
*********************************************************************************************************/
static uid_t            _G_uidFatDefault;
static gid_t            _G_gidFatDefault;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
INT             __blockIoDevCreate(PLW_BLK_DEV  pblkdNew);
VOID            __blockIoDevDelete(INT  iIndex);
PLW_BLK_DEV     __blockIoDevGet(INT  iIndex);
INT             __blockIoDevReset(INT  iIndex);
INT             __blockIoDevIoctl(INT  iIndex, INT  iCmd, LONG  lArg);
INT             __blockIoDevIsLogic(INT  iIndex);
INT             __blockIoDevFlag(INT     iIndex);
/*********************************************************************************************************
  ����ת����������
*********************************************************************************************************/
VOID            __filInfoToStat(PLW_DEV_HDR  pdevhdr,
                                FILINFO     *filinfo, 
                                FATFS       *fatfs,
                                struct stat *pstat, 
                                ino_t        ino);
INT             __fsInfoToStatfs(FATFS         *fatfs,
                                 INT            iFlag,
                                 struct statfs *pstatfs, 
                                 INT            iDrv);
mode_t          __fsAttrToMode(BYTE  ucAttr);
BYTE            __fsModeToAttr(mode_t  iMode);
UINT32          __timeToFatTime(time_t  *ptime);
UINT32          get_fattime(void);
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __FAT_FILE_LOCK(pfatfile)   API_SemaphoreMPend(pfatfile->FATFIL_pfatvol->FATVOL_hVolLock, \
                                    LW_OPTION_WAIT_INFINITE)
#define __FAT_FILE_UNLOCK(pfatfile) API_SemaphoreMPost(pfatfile->FATFIL_pfatvol->FATVOL_hVolLock)
#define __FAT_VOL_LOCK(pfatvol)     API_SemaphoreMPend(pfatvol->FATVOL_hVolLock, LW_OPTION_WAIT_INFINITE)
#define __FAT_VOL_UNLOCK(pfatvol)   API_SemaphoreMPost(pfatvol->FATVOL_hVolLock)
/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)       ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
/*********************************************************************************************************
  ������������
*********************************************************************************************************/
static INT      __fatFsCheck(PLW_BLK_DEV  pblkd, UINT8  ucPartType);
static LONG     __fatFsOpen(PFAT_VOLUME     pfatvol,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __fatFsRemove(PFAT_VOLUME     pfatvol,
                              PCHAR           pcName);
static INT      __fatFsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __fatFsRead(PLW_FD_ENTRY   pfdentry,
                            PCHAR          pcBuffer, 
                            size_t         stMaxBytes);
static ssize_t  __fatFsPRead(PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer, 
                             size_t         stMaxBytes,
                             off_t      oftPos);
static ssize_t  __fatFsWrite(PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer, 
                             size_t         stNBytes);
static ssize_t  __fatFsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer, 
                              size_t        stNBytes,
                              off_t         oftPos);
static INT      __fatFsIoctl(PLW_FD_ENTRY   pfdentry,
                             INT            iRequest,
                             LONG           lArg);
static INT      __fatFsSync(PLW_FD_ENTRY   pfdentry, BOOL  bFlushCache);
/*********************************************************************************************************
  �ļ�ϵͳ��������
*********************************************************************************************************/
LW_API INT      API_FatFsDevCreate(PCHAR   pcName, PLW_BLK_DEV  pblkd);
/*********************************************************************************************************
** ��������: __tshellFatUGID
** ��������: ϵͳ���� "fatugid"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellFatUGID (INT  iArgC, PCHAR  ppcArgV[])
{
    uid_t   uid;
    gid_t   gid;

    if (iArgC != 3) {
        printf("vfat current uid: %u gid: %u\n", _G_uidFatDefault, _G_gidFatDefault);
        return  (ERROR_NONE);
    
    } else {
        if (sscanf(ppcArgV[1], "%u", &uid) != 1) {
            fprintf(stderr, "fatugid [uid] [gid]\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[2], "%u", &gid) != 1) {
            fprintf(stderr, "fatugid [uid] [gid]\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        __KERNEL_MODE_PROC(
            _G_uidFatDefault = uid;
            _G_gidFatDefault = gid;
        );
        
        return  (ERROR_NONE);
    }
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: API_FatFsDrvInstall
** ��������: ��װ FAT �ļ�ϵͳ��������
** �䡡��  : 
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_FatFsDrvInstall (VOID)
{
    struct file_operations     fileop;

    if (_G_iFatDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __fatFsOpen;
    fileop.fo_release  = __fatFsRemove;
    fileop.fo_open     = __fatFsOpen;
    fileop.fo_close    = __fatFsClose;
    fileop.fo_read     = __fatFsRead;
    fileop.fo_read_ex  = __fatFsPRead;
    fileop.fo_write    = __fatFsWrite;
    fileop.fo_write_ex = __fatFsPWrite;
    fileop.fo_ioctl    = __fatFsIoctl;
    
    _G_iFatDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);       /*  ʹ�� NEW_1 ���豸����       */
     
    DRIVER_LICENSE(_G_iFatDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iFatDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iFatDrvNum, "FAT12/16/32 driver.");
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "microsoft FAT file system installed.\r\n");
                                                                        /*  ע���ļ�ϵͳ                */
    __fsRegister("vfat", API_FatFsDevCreate, (FUNCPTR)__fatFsCheck, LW_NULL);
    
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("fatugid", __tshellFatUGID);
    API_TShellFormatAdd("fatugid", " [uid] [gid]");
    API_TShellHelpAdd("fatugid", "set/get fat volume default uid gid.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    return  ((_G_iFatDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: API_FatFsDevCreate
** ��������: ����һ�� FAT �豸, ����: API_FatFsDevCreate("/ata0", ...); 
**           �� sylixos �� yaffs ��ͬ, FAT ÿһ�����Ƕ������豸.
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
**           pblkd             ���豸����
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_FatFsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    REGISTER PFAT_VOLUME     pfatvol;
    REGISTER INT             iBlkdIndex;
    REGISTER FRESULT         fresError;
             INT             iErrLevel = 0;
             ULONG           ulError   = ERROR_NONE;

    if (_G_iFatDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "fat Driver invalidate.\r\n");
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
    
    pfatvol = (PFAT_VOLUME)__SHEAP_ALLOC(sizeof(FAT_VOLUME));
    if (pfatvol == LW_NULL) {
        __blockIoDevDelete(iBlkdIndex);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pfatvol, sizeof(FAT_VOLUME));                             /*  ��վ���ƿ�                */
    
#if __FAT_FILE_INOD_EN > 0
    pfatvol->FATVOL_puniqPool = API_FsUniqueCreate(__FAT_FILE_UNIQ_SIZE, 1);
    if (pfatvol->FATVOL_puniqPool == LW_NULL) {                         /*  ���� inode ������           */
        iErrLevel = 1;
        goto    __error_handle;
    }
#endif                                                                  /*  �Ƿ����� inode ������       */
    
    pfatvol->FATVOL_bForceDelete = LW_FALSE;                            /*  ������ǿ��ж�ؾ�            */
    
    pfatvol->FATVOL_iDrv     = iBlkdIndex;                              /*  ��¼����λ��                */
    pfatvol->FATVAL_bValid   = LW_TRUE;                                 /*  ����Ч                      */
    pfatvol->FATVOL_hVolLock = API_SemaphoreMCreate("fatvol_lock", 
                               LW_PRIO_DEF_CEILING,
                               LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                               LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                               LW_NULL);
    if (!pfatvol->FATVOL_hVolLock) {                                    /*  �޷���������                */
        iErrLevel = 2;
        goto    __error_handle;
    }
    pfatvol->FATVOL_plineFdNodeHeader = LW_NULL;                        /*  û���ļ�����              */
    pfatvol->FATVOL_uiTime = get_fattime();                             /*  ��õ�ǰʱ��                */
    
    fresError = f_mount_ex(&pfatvol->FATVOL_fatfsVol, (BYTE)iBlkdIndex);/*  �����ļ�ϵͳ                */
    if (fresError != FR_OK) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "fat driver table full.\r\n");
        ulError = ERROR_IOS_DRIVER_GLUT;
        iErrLevel = 3;
        goto    __error_handle;
    }
    
    pfatvol->FATVOL_iFlag = pblkd->BLKD_iFlag;
    
    if (iosDevAddEx(&pfatvol->FATVOL_devhdrHdr, pcName, _G_iFatDrvNum, DT_DIR)
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
        f_mount_ex(LW_NULL, (BYTE)iBlkdIndex);                          /*  ж�ع��ص��ļ�ϵͳ          */
    }
    if (iErrLevel > 2) {
        API_SemaphoreMDelete(&pfatvol->FATVOL_hVolLock);
    }
    if (iErrLevel > 1) {
        __blockIoDevDelete(iBlkdIndex);
    }
#if __FAT_FILE_INOD_EN > 0
    if (iErrLevel > 0) {
        API_FsUniqueDelete(pfatvol->FATVOL_puniqPool);
    }
#endif                                                                  /*  __FAT_FILE_INOD_EN          */
    __SHEAP_FREE(pfatvol);
    
    _ErrorHandle(ulError);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_FatFsDevDelete
** ��������: ɾ��һ�� FAT �豸, ����: API_FatFsDevDelete("/mnt/ata0");
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_FatFsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  ������豸, �����ж���豸  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __fatFsCheck
** ��������: ��� exFAT �����Ƿ���Ч
** �䡡��  : pblkd             ���豸
**           ucPartType        �������� (0x07 is NTFS or exFAT)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsCheck (PLW_BLK_DEV  pblkd, UINT8  ucPartType)
{
    PUCHAR  pucBuffer;
    ULONG   ulSecSize;
    
    if (!pblkd || (ucPartType != 0x07)) {
        return  (ERROR_NONE);
    }
    
    pblkd->BLKD_pfuncBlkIoctl(pblkd, LW_BLKD_CTRL_POWER, LW_BLKD_POWER_ON);
    pblkd->BLKD_pfuncBlkReset(pblkd);
    pblkd->BLKD_pfuncBlkIoctl(pblkd, FIODISKINIT);
    
    ulSecSize = pblkd->BLKD_ulBytesPerSector;
    if (!ulSecSize) {
        pblkd->BLKD_pfuncBlkIoctl(pblkd, LW_BLKD_GET_SECSIZE, &ulSecSize);
    }
    if (!ulSecSize) {
        return  (PX_ERROR);
    }
    
    pucBuffer = (PUCHAR)__SHEAP_ALLOC((size_t)ulSecSize);
    if (!pucBuffer) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    if (pblkd->BLKD_pfuncBlkRd(pblkd, pucBuffer, 0, 1) < 0) {
        __SHEAP_FREE(pucBuffer);
        return  (PX_ERROR);
    }
    
    if (lib_memcmp(pucBuffer, "\xEB\x76\x90" "EXFAT   ", 11)) {         /*  ������ exFAT �ļ�ϵͳ       */
        __SHEAP_FREE(pucBuffer);
        return  (PX_ERROR);
    }
    
    __SHEAP_FREE(pucBuffer);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fatFsHisAdd
** ��������: FAT FS ��һ���Ѿ����������ļ�������ʷ��¼
** �䡡��  : pfatfile          �ļ�
** �䡡��  : inode ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ino_t  __fatFsHisAdd (PFAT_FILE  pfatfile)
{
#if __FAT_FILE_INOD_EN > 0
    INT                  iHash        = (INT)(pfatfile->FATFIL_u64Uniq % __FAT_FILE_HASH_SIZE);
    LW_LIST_LINE_HEADER *pplineHeader = &pfatfile->FATFIL_pfatvol->FATVOL_plineHashHeader[iHash];
    PLW_LIST_LINE        plineTemp;
    PFAT_FILE_HIS        pfathis;
    
    for (plineTemp  = *pplineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pfathis = _LIST_ENTRY(plineTemp, FAT_FILE_HIS, FATHIS_lineManage);
        if (pfathis->FATHIS_u64Uniq == pfatfile->FATFIL_u64Uniq) {
            break;
        }
    }
    
    if (plineTemp) {
        return  (pfathis->FATHIS_inode);
    }
    
    pfathis = (PFAT_FILE_HIS)__SHEAP_ALLOC(sizeof(FAT_FILE_HIS));
    if (pfathis == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (0);
    }
    
    pfathis->FATHIS_inode = API_FsUniqueAlloc(pfatfile->FATFIL_pfatvol->FATVOL_puniqPool);
    if (!API_FsUniqueIsVal(pfatfile->FATFIL_pfatvol->FATVOL_puniqPool, pfathis->FATHIS_inode)) {
        __SHEAP_FREE(pfathis);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (0);
    }
    pfathis->FATHIS_u64Uniq = pfatfile->FATFIL_u64Uniq;                 /*  ����� + ƫ��               */
    
    _List_Line_Add_Tail(&pfathis->FATHIS_lineManage, pplineHeader);
    
    return  (pfathis->FATHIS_inode);
#else

    return  (1);
#endif                                                                  /*  __FAT_FILE_INOD_EN          */
}
/*********************************************************************************************************
** ��������: __fatFsHisGet
** ��������: FAT FS ɾ��һ�������е���ʷ��¼ (ж�ؾ�ʱʹ��)
** �䡡��  : pfatvol          ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fatFsHisDel (PFAT_VOLUME  pfatvol)
{
#if __FAT_FILE_INOD_EN > 0
    INT             i;
    PLW_LIST_LINE   plineTemp;
    PLW_LIST_LINE   plineDel;
    
    for (i = 0; i < __FAT_FILE_HASH_SIZE; i++) {
        plineTemp = pfatvol->FATVOL_plineHashHeader[i];
        while (plineTemp) {
            plineDel  = plineTemp;
            plineTemp = _list_line_get_next(plineTemp);
            __SHEAP_FREE(plineDel);                                     /*  ���ﲻ�����˳�����, ֱ���ͷ�*/
        }
        pfatvol->FATVOL_plineHashHeader[i] = LW_NULL;
    }
#endif                                                                  /*  __FAT_FILE_INOD_EN          */
}
/*********************************************************************************************************
** ��������: __fatFsMkMode
** ��������: FAT FS ת���ļ���������
** �䡡��  : iFlags           posix ��������
** �䡡��  : FatFs ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BYTE  __fatFsMkMode (INT  iFlags)
{
    REGISTER BYTE   ucMode = 0;
    
    if (iFlags & O_WRONLY) {                                            /*  ȷ���ļ�ϵͳ��дȨ��        */
        ucMode = FA_WRITE;
    } else if (iFlags & O_RDWR) {
        ucMode = FA_READ | FA_WRITE;
    } else {
        ucMode = FA_READ;
    }
     
    if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {                      /*  �������, ���½�, ���ش���  */
        ucMode |= FA_CREATE_NEW;                                        /*  �������, �����½�          */
    } else if (iFlags & O_CREAT) {
        ucMode |= FA_OPEN_ALWAYS;                                       /*  �����, �����½�          */
    }

    if (iFlags & O_TRUNC) {                                             /*  �������ļ����ض�            */
        if ((iFlags & O_RDWR) || (iFlags & O_WRONLY)) {
            ucMode |= FA_CREATE_ALWAYS;                                 /*  ���Ǵ������ļ�, ����trunc   */
        }
    }
    
    return  (ucMode);
}
/*********************************************************************************************************
** ��������: __fatFsGetError
** ��������: FAT FS ������ת��Ϊ posix ��׼
** �䡡��  : fresError           FatFs �����
** �䡡��  : POSIX �����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __fatFsGetError (FRESULT    fresError)
{
    switch (fresError) {
    
    case FR_OK:
        return  (ERROR_NONE);
    
    case FR_NOT_READY:
        return  (EIO);
    
    case FR_NO_FILE:
        return  (ENOENT);
    
    case FR_NO_PATH:
        return  (ENOENT);
    
    case FR_INVALID_NAME:
        return  (EFAULT);
    
    case FR_INVALID_DRIVE:
        return  (ENODEV);
    
    case FR_DENIED:
        return  (EACCES);
    
    case FR_EXIST:
        return  (EEXIST);
    
    case FR_DISK_ERR:
        return  (EIO);
    
    case FR_WRITE_PROTECTED:
        return  (EWRPROTECT);
    
    case FR_NOT_ENABLED:
        return  (EOVERFLOW);
    
    case FR_NO_FILESYSTEM:
        return  (EFORMAT);
    
    case FR_INVALID_OBJECT:
        return  (EBADF);
    
    case FR_MKFS_ABORTED:
        return  (EIO);
    
    case FR_TIMEOUT:
        return  (ETIMEDOUT);
    
    case FR_LOCKED:
        return  (EBUSY);
    
    case FR_NOT_ENOUGH_CORE:
        return  (ENOMEM);
    
    case FR_TOO_MANY_OPEN_FILES:
        return  (EMFILE);
    
    case FR_INVALID_PARAMETER:
        return  (EINVAL);
        
    default:
        return  (EIO);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fatFsGetInfo
** ��������: FAT FS �����ʱ�Ļ�������
** �䡡��  : pfatfile         fat�ļ�
**           pstat64          64bit �ļ�����
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsGetInfo (PFAT_FILE  pfatfile, mode_t  *pmode, off_t  *poftSize)
{
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {           /*  ��ͨ�ļ�                    */
        *pmode    = S_IFREG | S_IRWXU | S_IRWXO | S_IRWXO;              /*  ��ʱ����Ϊ����Ȩ�޶�֧��    */
        *poftSize = f_size(&pfatfile->FATFIL_fftm.FFTM_file);
    
    } else if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DIR) {     /*  Ŀ¼�ļ�                    */
        *pmode    = S_IFDIR | S_IRWXU | S_IRWXO | S_IRWXO;              /*  ��ʱ����Ϊ����Ȩ�޶�֧��    */
        *poftSize = 0;
    
    } else {                                                            /*  ��Ŀ¼                      */
        *pmode    = S_IFDIR | S_IRWXU | S_IRWXO | S_IRWXO;              /*  ��ʱ����Ϊ����Ȩ�޶�֧��    */
        *poftSize = 0;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fatFsCloseFile
** ��������: FAT FS �ر��ļ�����
** �䡡��  : pfatfile         fat�ļ�
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fatFsCloseFile (PFAT_FILE  pfatfile)
{
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {           /*  ��׼ FAT �ļ�               */
        f_close(&pfatfile->FATFIL_fftm.FFTM_file);                      /*  FAT �ر��ļ�                */
    
    } else if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DIR) {     /*  Ŀ¼                        */
        f_closedir(&pfatfile->FATFIL_fftm.FFTM_fatdir);
    }
}
/*********************************************************************************************************
** ��������: __fatFsSeekFile
** ��������: FAT FS ִ���ڲ��ļ�ָ�����
** �䡡��  : pfatfile         fat�ļ�
**           oftPtr           �ļ�ָ��
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsSeekFile (PFAT_FILE  pfatfile, off_t  oftPtr)
{
    FRESULT       fresError = FR_OK;
    ULONG         ulError   = ERROR_NONE;
    
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {           /*  ��׼ FAT �ļ�               */
        if (f_tell(&pfatfile->FATFIL_fftm.FFTM_file) == oftPtr) {       /*  ����Ҫ seek                 */
            return  (ERROR_NONE);
        }
        
        fresError = f_lseek(&pfatfile->FATFIL_fftm.FFTM_file, oftPtr);
        if ((fresError == FR_OK) && 
            (f_tell(&pfatfile->FATFIL_fftm.FFTM_file) == oftPtr)) {
            return  (ERROR_NONE);
        }
        ulError = __fatFsGetError(fresError);                           /*  ת��������                */
    
    } else {
        ulError = EISDIR;
    }
    
    _ErrorHandle(ulError);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __fatFsOpen
** ��������: FAT FS open ����
** �䡡��  : pfatvol          ����ƿ�
**           pcName           �ļ���
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  __fatFsOpen (PFAT_VOLUME     pfatvol,
                          PCHAR           pcName,
                          INT             iFlags,
                          INT             iMode)
{
    REGISTER PFAT_FILE      pfatfile;
    REGISTER BYTE           ucMode;
    REGISTER FRESULT        fresError;
    REGISTER FRESULT        fresStat;
    REGISTER ULONG          ulError;
             PLW_FD_NODE    pfdnode;
             BOOL           bIsNew;
             FILINFO        fileinfo;
             off_t          oftSize;


    if (pcName == LW_NULL) {                                            /*  ���ļ���                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (__blockIoDevFlag(pfatvol->FATVOL_iDrv) == O_RDONLY) {       /*  �˾�д����, ����ֻ��״̬    */
            if (iFlags & (O_CREAT | O_TRUNC | O_RDWR | O_WRONLY)) {
                _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
                return  (PX_ERROR);
            }
        }
        if (iFlags & O_CREAT) {
            if (pfatvol->FATVOL_iFlag == O_RDONLY) {
                _ErrorHandle(EROFS);                                    /*  ֻ���ļ�ϵͳ                */
                return  (PX_ERROR);
            }
            if (__fsCheckFileName(pcName)) {
                _ErrorHandle(ENOENT);
                return  (PX_ERROR);
            }
            if (S_ISFIFO(iMode) || 
                S_ISBLK(iMode)  ||
                S_ISCHR(iMode)) {                                       /*  ���ﲻ���� socket �ļ�      */
                _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                /*  ��֧��������Щ��ʽ          */
                return  (PX_ERROR);
            }
        }
    
        ucMode = __fatFsMkMode(iFlags);                                 /*  �ļ���������                */

        pfatfile = (PFAT_FILE)__SHEAP_ALLOC(sizeof(FAT_FILE) + 
                                            lib_strlen(pcName));        /*  �����ļ��ڴ�                */
        if (pfatfile == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_strcpy(pfatfile->FATFIL_cName, pcName);                     /*  ��¼�ļ���                  */
    
        pfatfile->FATFIL_pfatvol = pfatvol;                             /*  ��¼����Ϣ                  */
        
        ulError = __FAT_FILE_LOCK(pfatfile);
        if ((pfatvol->FATVAL_bValid == LW_FALSE) ||
            (ulError != ERROR_NONE)) {                                  /*  �����ڱ�ж��                */
            __FAT_FILE_UNLOCK(pfatfile);
            __SHEAP_FREE(pfatfile);
            _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
            return  (PX_ERROR);
        }
        
        if ((iFlags & O_CREAT) && S_ISDIR(iMode)) {                     /*  ����Ŀ¼                    */
            fresError = f_mkdir_ex(&pfatvol->FATVOL_fatfsVol, pcName);
            if ((fresError != FR_OK) && (iFlags & O_EXCL)) {
                __FAT_FILE_UNLOCK(pfatfile);
                __SHEAP_FREE(pfatfile);
                ulError = __fatFsGetError(fresError);                   /*  ת��������                */
                _ErrorHandle(ulError);
                return  (PX_ERROR);
            }
        }
        
        fresError = f_open_ex(&pfatvol->FATVOL_fatfsVol, 
                              &pfatfile->FATFIL_fftm.FFTM_file,
                              &pfatfile->FATFIL_u64Uniq,
                              pcName, ucMode);
        pfatfile->FATFIL_iFileType = __FAT_FILE_TYPE_NODE;              /*  ���Ա� close ��             */
        
        if (fresError != FR_OK) {                                       /*  ��ʧ��                    */
            fresStat = f_stat_ex(&pfatvol->FATVOL_fatfsVol,
                                 pcName, &fileinfo);                    /*  ����Ƿ�ΪĿ¼�ļ�          */
            if ((fresStat == FR_OK) &&
                (fileinfo.fattrib & AM_DIR)) {                          /*  ��·��ΪĿ¼                */
                f_opendir_ex(&pfatvol->FATVOL_fatfsVol, 
                             &pfatfile->FATFIL_fftm.FFTM_fatdir,
                             &pfatfile->FATFIL_u64Uniq, pcName);        /*  ��Ŀ¼                    */
                pfatfile->FATFIL_iFileType = __FAT_FILE_TYPE_DIR;       /*  Ŀ¼�ļ�                    */
                goto    __file_open_ok;                                 /*  �ļ�������                */
            }
            
            if (__STR_IS_ROOT(pfatfile->FATFIL_cName)) {
                pfatfile->FATFIL_iFileType = __FAT_FILE_TYPE_DEV;       /*  �������豸, δ��ʽ��      */
                pfatfile->FATFIL_u64Uniq   = (UINT64)~0;                /*  �����κ��ļ��ظ�(��Ŀ¼)    */
                goto    __file_open_ok;                                 /*  �ļ�������                */
            }
            
            __FAT_FILE_UNLOCK(pfatfile);
            __SHEAP_FREE(pfatfile);
            ulError = __fatFsGetError(fresError);                       /*  ת��������                */
            _ErrorHandle(ulError);
            return  (PX_ERROR);
        
        } else {                                                        /*  ��ͨ�ļ��򿪳ɹ�            */
            if (iFlags & O_DIRECTORY) {
                f_close(&pfatfile->FATFIL_fftm.FFTM_file);
                __FAT_FILE_UNLOCK(pfatfile);
                __SHEAP_FREE(pfatfile);
                _ErrorHandle(ENOTDIR);
                return  (PX_ERROR);
            }
            pfatfile->FATFIL_fftm.FFTM_file.flag |= (FA_READ | FA_WRITE);
            f_sync(&pfatfile->FATFIL_fftm.FFTM_file);                   /*  ��д����(����ʱ�����Ϣ)    */
        }
        
__file_open_ok:
        if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DEV) {
            pfatfile->FATFIL_inode = 0;                                 /*  �����κ��ļ��ظ�(��Ŀ¼)    */

        } else {
            pfatfile->FATFIL_inode = __fatFsHisAdd(pfatfile);
            if (pfatfile->FATFIL_inode == 0) {                          /*  �޷���ȡ inode              */
                goto    __file_open_error;
            }
        }
        
        __fatFsGetInfo(pfatfile, &iMode, &oftSize);                     /*  ���һЩ������Ϣ            */
        
        pfdnode = API_IosFdNodeAdd(&pfatvol->FATVOL_plineFdNodeHeader,
                                   (dev_t)&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                                   pfatfile->FATFIL_u64Uniq,            /*  ����ʹ�� ��+ƫ����Ϊʶ����  */
                                   iFlags, iMode, 0, 0, oftSize, 
                                   (PVOID)pfatfile,
                                   &bIsNew);                            /*  ����ļ��ڵ�                */
        if (pfdnode == LW_NULL) {                                       /*  �޷����� fd_node �ڵ�       */
            goto    __file_open_error;
        }
        
        if ((iFlags & O_TRUNC) && ((iFlags & O_ACCMODE) != O_RDONLY)) { /*  ��Ҫ�ض�                    */
            if (bIsNew == LW_FALSE) {                                   /*  �������ظ���ȷ�����      */
                API_IosFdNodeDec(&pfatvol->FATVOL_plineFdNodeHeader,
                                 pfdnode, LW_NULL);
                _ErrorHandle(EBUSY);
                goto    __file_open_error;
            
            } else if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
                f_truncate(&pfatfile->FATFIL_fftm.FFTM_file);           /*  �ض�Ϊ 0                    */
            }
        }
        
        LW_DEV_INC_USE_COUNT(&pfatvol->FATVOL_devhdrHdr);               /*  ���¼�����                  */
        
        if (bIsNew == LW_FALSE) {                                       /*  ���ظ���                  */
            __fatFsCloseFile(pfatfile);
            __FAT_FILE_UNLOCK(pfatfile);
            __SHEAP_FREE(pfatfile);
        
        } else {
            __FAT_FILE_UNLOCK(pfatfile);
        }
        
        return  ((LONG)pfdnode);
    }
    
__file_open_error:
    __fatFsCloseFile(pfatfile);
    __FAT_FILE_UNLOCK(pfatfile);
    __SHEAP_FREE(pfatfile);
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __fatFsRemove
** ��������: FAT FS remove ����
** �䡡��  : pfatvol          ����ƿ�
**           pcName           �ļ���
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsRemove (PFAT_VOLUME     pfatvol,
                           PCHAR           pcName)
{
    REGISTER FRESULT        fresError;
    REGISTER ULONG          ulError = ERROR_NONE;
             PLW_BLK_DEV    pblkd;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (__STR_IS_ROOT(pcName)) {                                    /*  ��Ŀ¼�����豸�ļ�          */
            ulError = __FAT_VOL_LOCK(pfatvol);
            if (ulError) {
                _ErrorHandle(ENXIO);
                return  (PX_ERROR);                                     /*  ���ڱ���������ж��          */
            }
            
            if (pfatvol->FATVAL_bValid == LW_FALSE) {
                __FAT_VOL_UNLOCK(pfatvol);
                return  (ERROR_NONE);                                   /*  ���ڱ���������ж��          */
            }
            
__re_umount_vol:
            if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pfatvol)) {          /*  ����Ƿ������ڹ������ļ�    */
                if (!pfatvol->FATVOL_bForceDelete) {
                    __FAT_VOL_UNLOCK(pfatvol);
                    _ErrorHandle(EBUSY);
                    return  (PX_ERROR);                                 /*  ���ļ���, ���ܱ�ж��      */
                }
                
                pfatvol->FATVAL_bValid = LW_FALSE;                      /*  ��ʼж�ؾ�, �ļ����޷���  */
                
                __FAT_VOL_UNLOCK(pfatvol);
                
                _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
                iosDevFileAbnormal(&pfatvol->FATVOL_devhdrHdr);         /*  ����������ļ���Ϊ�쳣״̬  */
                
                __FAT_VOL_LOCK(pfatvol);
                goto    __re_umount_vol;
                
            } else {
                pfatvol->FATVAL_bValid = LW_FALSE;                      /*  ��ʼж�ؾ�, �ļ����޷���  */
            }
            
            /*
             *  ���ܳ���ʲô�������, �������ж�ؾ�, ���ﲻ���жϴ���.
             */
            f_syncfs(&pfatvol->FATVOL_fatfsVol);                        /*  ��д�ļ�ϵͳ�����          */
            
            __blockIoDevIoctl(pfatvol->FATVOL_iDrv,
                              FIOFLUSH, 0);                             /*  ��д DISK CACHE             */
            __blockIoDevIoctl(pfatvol->FATVOL_iDrv, LW_BLKD_CTRL_POWER, 
                              LW_BLKD_POWER_OFF);                       /*  �豸�ϵ�                    */
            __blockIoDevIoctl(pfatvol->FATVOL_iDrv, LW_BLKD_CTRL_EJECT,
                              0);                                       /*  ���豸����                  */
            
            pblkd = __blockIoDevGet(pfatvol->FATVOL_iDrv);              /*  ��ÿ��豸���ƿ�            */
            if (pblkd) {
                __fsDiskLinkCounterDec(pblkd);                          /*  �������Ӵ���                */
            }
            
            iosDevDelete((LW_DEV_HDR *)pfatvol);                        /*  IO ϵͳ�Ƴ��豸             */
            
            f_mount_ex(LW_NULL, (BYTE)pfatvol->FATVOL_iDrv);            /*  ж�ع��ص��ļ�ϵͳ          */
            __blockIoDevIoctl(pfatvol->FATVOL_iDrv,
                              FIOUNMOUNT, 0);                           /*  ִ�еײ���������            */
            __blockIoDevDelete(pfatvol->FATVOL_iDrv);                   /*  �����������Ƴ�              */
            
            __fatFsHisDel(pfatvol);                                     /*  �ͷ�������ʷ��¼            */
            
#if __FAT_FILE_INOD_EN > 0
            API_FsUniqueDelete(pfatvol->FATVOL_puniqPool);              /*  ж�� uniq ������            */
#endif                                                                  /*  __FAT_FILE_INOD_EN          */
            
            API_SemaphoreMDelete(&pfatvol->FATVOL_hVolLock);            /*  ɾ������                    */
            
            __SHEAP_FREE(pfatvol);                                      /*  �ͷž���ƿ�                */
            
            _DebugHandle(__LOGMESSAGE_LEVEL, "disk unmount ok.\r\n");
            
            return  (ERROR_NONE);
        
        } else {                                                        /*  ɾ���ļ���Ŀ¼              */
            if (__blockIoDevFlag(pfatvol->FATVOL_iDrv) == O_RDONLY) {   /*  �˾�д����, ����ֻ��״̬    */
                _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
                return  (PX_ERROR);
            }
            
            if (__fsCheckFileName(pcName) < 0) {                        /*  ����ļ����Ƿ�Ϸ�          */
                _ErrorHandle(ENOENT);                                   /*  �ļ�δ�ҵ�                  */
                return  (PX_ERROR);
            }
            
            ulError = __FAT_VOL_LOCK(pfatvol);
            if ((pfatvol->FATVAL_bValid == LW_FALSE) ||
                (ulError != ERROR_NONE)) {                              /*  �����ڱ�ж��                */
                __FAT_VOL_UNLOCK(pfatvol);
                _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
                return  (PX_ERROR);
            }
            
            fresError = f_unlink_ex(&pfatvol->FATVOL_fatfsVol,
                                    pcName);                            /*  ִ��ɾ������                */
            __FAT_VOL_UNLOCK(pfatvol);
            
            ulError = __fatFsGetError(fresError);                       /*  ת��������                */
            _ErrorHandle(ulError);
            
            if (ulError) {
                return  (PX_ERROR);                                     /*  ɾ��ʧ��                    */
            } else {
                return  (ERROR_NONE);                                   /*  ɾ����ȷ                    */
            }
        }
    }
}
/*********************************************************************************************************
** ��������: __fatFsClose
** ��������: FAT FS close ����
** �䡡��  : pfdentry         �ļ����ƿ�
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE     pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    PFAT_VOLUME   pfatvol  = pfatfile->FATFIL_pfatvol;
    BOOL          bFree    = LW_FALSE;
    BOOL          bRemove  = LW_FALSE;

    if (pfatfile) {
        if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
            _ErrorHandle(ENXIO);                                        /*  �豸����                    */
            return  (PX_ERROR);
        }
        
        if (API_IosFdNodeDec(&pfatvol->FATVOL_plineFdNodeHeader,
                             pfdnode, &bRemove) == 0) {                 /*  fd_node �Ƿ���ȫ�ͷ�        */
            __fatFsCloseFile(pfatfile);
            bFree = LW_TRUE;
        }
        
        LW_DEV_DEC_USE_COUNT(&pfatvol->FATVOL_devhdrHdr);               /*  ���¼�����                  */
        
        if (bRemove) {
            f_unlink_ex(&pfatvol->FATVOL_fatfsVol, 
                        pfatfile->FATFIL_cName);                        /*  ɾ���ļ�                    */
        }
        
        __FAT_FILE_UNLOCK(pfatfile);
        
        if (bFree) {
            __SHEAP_FREE(pfatfile);                                     /*  �ͷ��ڴ�                    */
        }
        
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __fatFsRead
** ��������: FAT FS read ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __fatFsRead (PLW_FD_ENTRY   pfdentry,
                             PCHAR          pcBuffer, 
                             size_t         stMaxBytes)
{
    PLW_FD_NODE   pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE     pfatfile  = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT       fresError = FR_OK;
    ULONG         ulError   = ERROR_NONE;
    UINT          uiReadNum = 0;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_iFileType != __FAT_FILE_TYPE_NODE) {
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes) {
        if (__fatFsSeekFile(pfatfile, pfdentry->FDENTRY_oftPtr)) {      /*  �����ڲ��ļ�ָ��            */
            __FAT_FILE_UNLOCK(pfatfile);
            return  (PX_ERROR);
        }
        
        fresError = f_read(&pfatfile->FATFIL_fftm.FFTM_file,
                           (PVOID)pcBuffer, 
                           (UINT)stMaxBytes, 
                           &uiReadNum);
        if ((fresError == FR_OK) && (uiReadNum > 0)) {
            pfdentry->FDENTRY_oftPtr += (off_t)uiReadNum;               /*  �����ļ�ָ��                */
        }
        
        ulError = __fatFsGetError(fresError);                           /*  ת��������                */
    }
    
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  ((ssize_t)uiReadNum);
}
/*********************************************************************************************************
** ��������: __fatFsPRead
** ��������: FAT FS pread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __fatFsPRead (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer, 
                              size_t        stMaxBytes,
                              off_t         oftPos)
{
    PLW_FD_NODE   pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE     pfatfile  = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT       fresError = FR_OK;
    ULONG         ulError   = ERROR_NONE;
    UINT          uiReadNum = 0;
    
    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_iFileType != __FAT_FILE_TYPE_NODE) {
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes) {
        if (__fatFsSeekFile(pfatfile, oftPos)) {                        /*  �����ڲ��ļ�ָ��            */
            __FAT_FILE_UNLOCK(pfatfile);
            return  (PX_ERROR);
        }
        
        fresError = f_read(&pfatfile->FATFIL_fftm.FFTM_file,
                           (PVOID)pcBuffer, 
                           (UINT)stMaxBytes, 
                           &uiReadNum);
                           
        ulError = __fatFsGetError(fresError);                           /*  ת��������                */
    }
    
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  ((ssize_t)uiReadNum);
}
/*********************************************************************************************************
** ��������: __fatFsWrite
** ��������: FAT FS write ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __fatFsWrite (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer, 
                              size_t        stNBytes)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE     pfatfile   = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT       fresError  = FR_OK;
    ULONG         ulError    = ERROR_NONE;
    UINT          uiWriteNum = 0;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_pfatvol->FATVOL_iFlag == O_RDONLY) {
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_iFileType != __FAT_FILE_TYPE_NODE) {
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  ׷��ģʽ                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  �ƶ���дָ�뵽ĩβ          */
    }
    
    if (stNBytes) {
        if (__fatFsSeekFile(pfatfile, pfdentry->FDENTRY_oftPtr)) {      /*  �����ڲ��ļ�ָ��            */
            __FAT_FILE_UNLOCK(pfatfile);
            return  (PX_ERROR);
        }
        
        fresError = f_write(&pfatfile->FATFIL_fftm.FFTM_file,
                            (CPVOID)pcBuffer, 
                            (UINT)stNBytes, 
                            &uiWriteNum);
        if ((fresError == FR_OK) && (uiWriteNum > 0)) {
            pfdentry->FDENTRY_oftPtr += (off_t)uiWriteNum;              /*  �����ļ�ָ��                */
            pfdnode->FDNODE_oftSize   = (off_t)f_size(&pfatfile->FATFIL_fftm.FFTM_file);
        }                                                               /*  �����ļ���С                */
        
        ulError = __fatFsGetError(fresError);                           /*  ת��������                */
    }
    
    __FAT_FILE_UNLOCK(pfatfile);
    
    if ((ulError == ERROR_NONE) && stNBytes) {
        if (pfdentry->FDENTRY_iFlag & (O_SYNC | O_DSYNC)) {             /*  ��Ҫ����ͬ��                */
            ulError = __fatFsSync(pfdentry, LW_TRUE);
        }
    }
    
    _ErrorHandle(ulError);
    return  ((ssize_t)uiWriteNum);
}
/*********************************************************************************************************
** ��������: __fatFsPWrite
** ��������: FAT FS pwrite ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __fatFsPWrite (PLW_FD_ENTRY  pfdentry,
                               PCHAR         pcBuffer, 
                               size_t        stNBytes,
                               off_t         oftPos)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE     pfatfile   = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT       fresError  = FR_OK;
    ULONG         ulError    = ERROR_NONE;
    UINT          uiWriteNum = 0;

    if (!pcBuffer || !stNBytes || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_pfatvol->FATVOL_iFlag == O_RDONLY) {
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EROFS);
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_iFileType != __FAT_FILE_TYPE_NODE) {
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stNBytes) {
        if (__fatFsSeekFile(pfatfile, oftPos)) {                        /*  �����ڲ��ļ�ָ��            */
            __FAT_FILE_UNLOCK(pfatfile);
            return  (PX_ERROR);
        }
        
        fresError = f_write(&pfatfile->FATFIL_fftm.FFTM_file,
                            (CPVOID)pcBuffer, 
                            (UINT)stNBytes, 
                            &uiWriteNum);
        if ((fresError == FR_OK) && (uiWriteNum > 0)) {
            pfdnode->FDNODE_oftSize   = (off_t)f_size(&pfatfile->FATFIL_fftm.FFTM_file);
        }                                                               /*  �����ļ���С                */
        
        ulError = __fatFsGetError(fresError);                           /*  ת��������                */
    }
    
    __FAT_FILE_UNLOCK(pfatfile);
    
    if ((ulError == ERROR_NONE) && stNBytes) {
        if (pfdentry->FDENTRY_iFlag & (O_SYNC | O_DSYNC)) {             /*  ��Ҫ����ͬ��                */
            ulError = __fatFsSync(pfdentry, LW_TRUE);
        }
    }
    
    _ErrorHandle(ulError);
    return  ((ssize_t)uiWriteNum);
}
/*********************************************************************************************************
** ��������: __fatFsClusterSizeCal
** ��������: ͨ�����̵�������ȷ�����̵Ĵش�С
** �䡡��  : pfatfile        fat�ļ�
**           pulClusterSize  �ش�С 
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsClusterSizeCal (PFAT_FILE   pfatfile, ULONG  *pulClusterSize)
{
    ULONG       ulSecNum  = 0;
    ULONG       ulSecSize = 0;
    INT         iError;
    
#define __FAT_CLUSTER_CAL_BASE(ulSecSize)   ((ulSecSize / 512) * LW_CFG_KB_SIZE)
                                                                        /*  �ش�С����                  */

    iError = __blockIoDevIoctl(pfatfile->FATFIL_pfatvol->FATVOL_iDrv,
                               LW_BLKD_GET_SECNUM,
                               (LONG)&ulSecNum);                        /*  ����豸��������            */
    if (iError < 0) {
        _ErrorHandle(ERROR_IO_DEVICE_ERROR);
        return  (PX_ERROR);
    }
    iError = __blockIoDevIoctl(pfatfile->FATFIL_pfatvol->FATVOL_iDrv,
                               LW_BLKD_GET_SECSIZE,
                               (LONG)&ulSecSize);                       /*  ��ô���������С            */
    if ((iError < 0) || (ulSecSize < 512)) {
        _ErrorHandle(ERROR_IO_DEVICE_ERROR);
        return  (PX_ERROR);
    }
    
    if (ulSecNum >= 0x800000) {                                         /*  4 GB ���ϴ���               */
        *pulClusterSize = 8 * __FAT_CLUSTER_CAL_BASE(ulSecSize);
    
    } else if (ulSecNum >= 0x200000) {                                  /*  1 GB ���ϴ���               */
        *pulClusterSize = 4 * __FAT_CLUSTER_CAL_BASE(ulSecSize);
    
    } else if (ulSecNum >= 0x100000) {                                  /*  512 MB ���ϴ���             */
        *pulClusterSize = 2 * __FAT_CLUSTER_CAL_BASE(ulSecSize);
    
    } else {
        *pulClusterSize = 1 * __FAT_CLUSTER_CAL_BASE(ulSecSize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fatFsFormat
** ��������: FAT FS ��ʽ��ý��
** �䡡��  : pfdentry            �ļ����ƿ�
**           lArg                ����
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsFormat (PLW_FD_ENTRY  pfdentry, LONG  lArg)
{
    PLW_FD_NODE pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE   pfatfile  = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    ULONG       ulClusterSize;
    FRESULT     fresError;
    INT         iError;
    ULONG       ulError;
    
    if (!__STR_IS_ROOT(pfatfile->FATFIL_cName)) {                       /*  ����Ƿ�Ϊ�豸�ļ�          */
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {           /*  �Ƿ���Ҫ��ǰ�ر��ļ�        */
        pfatfile->FATFIL_iFileType =  __FAT_FILE_TYPE_DEV;              /*  ����Ҫ�ٴιر�              */
        f_close(&pfatfile->FATFIL_fftm.FFTM_file);
    }
    
    iError = __fatFsClusterSizeCal(pfatfile, &ulClusterSize);           /*  ��ôش�С                  */
    if (iError < 0) {
        __FAT_FILE_UNLOCK(pfatfile);
        return  (PX_ERROR);
    }
    
    if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pfatfile->FATFIL_pfatvol)
        > 1) {                                                          /*  ����Ƿ������ڹ������ļ�    */
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(ERROR_IOS_TOO_MANY_OPEN_FILES);                    /*  �������ļ���              */
        return  (PX_ERROR);
    }
    
    (VOID)__blockIoDevIoctl(pfatfile->FATFIL_pfatvol->FATVOL_iDrv,
                            FIOCANCEL, 0);                              /*  CACHE ֹͣ (����д����)     */
    
    iError = __blockIoDevIoctl(pfatfile->FATFIL_pfatvol->FATVOL_iDrv,
                               FIODISKFORMAT,
                               lArg);                                   /*  �ײ��ʽ��                  */
    if (iError < 0) {
        PVOID  pvWork = __SHEAP_ALLOC(_MAX_SS);                         /*  ���仺����                  */
        
        if (pvWork == LW_NULL) {
            __FAT_FILE_UNLOCK(pfatfile);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        if (__blockIoDevIsLogic(pfatfile->FATFIL_pfatvol->FATVOL_iDrv)) {
            fresError = f_mkfs((BYTE)pfatfile->FATFIL_pfatvol->FATVOL_iDrv, 
                               (BYTE)(FM_SFD | FM_FAT | FM_FAT32),
                               (UINT16)ulClusterSize,
                               pvWork, _MAX_SS);                        /*  �˴���Ϊ�߼����̲���Ҫ������*/
        } else {
            fresError = f_mkfs((BYTE)pfatfile->FATFIL_pfatvol->FATVOL_iDrv, 
                               (BYTE)(FM_FAT | FM_FAT32),
                               (UINT16)ulClusterSize, 
                               pvWork, _MAX_SS);                        /*  ��ʽ�����з�����            */
        }
        
        __SHEAP_FREE(pvWork);                                           /*  �ͷŻ�����                  */
        
        ulError = __fatFsGetError(fresError);                           /*  ת��������                */
    
    } else {
        ulError = ERROR_NONE;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    if (ulError) {                                                      /*  ����                        */
        return  (PX_ERROR);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __fatFsRename
** ��������: FAT FS ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           pcNewName           ������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : Ϊ�˼�С�ڴ��Ƭ�Ĳ���, ���ļ����Ϳ��ƽṹ������ͬһ�ڴ�ֶ�, ��ʱ�޷����� pfatfile �����
             �ļ���, �����ļ�����ȫ����, ��ʱ, ���ļ������ٴβ���, ����, �û�������� rename() ���������
             �˲���, ���ܶ������� ioctl() ������.
*********************************************************************************************************/
static INT  __fatFsRename (PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
    REGISTER FRESULT        fresError;
    REGISTER INT            iError  = ERROR_NONE;
    REGISTER ULONG          ulError = ERROR_NONE;
    
             CHAR           cNewPath[PATH_MAX + 1];
    REGISTER PCHAR          pcNewPath = &cNewPath[0];
             PLW_FD_NODE    pfdnode   = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
             PFAT_FILE      pfatfile  = (PFAT_FILE)pfdnode->FDNODE_pvFile;
             PFAT_VOLUME    pfatvolNew;
             FILINFO        fileinfo;

    if (__STR_IS_ROOT(pfatfile->FATFIL_cName)) {                        /*  ����Ƿ�Ϊ�豸�ļ�          */
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
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DEV) {            /*  �豸�����ܸ���            */
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    if (ioFullFileNameGet(pcNewName, 
                          (LW_DEV_HDR **)&pfatvolNew, 
                          cNewPath) != ERROR_NONE) {                    /*  �����Ŀ¼·��              */
        __FAT_FILE_UNLOCK(pfatfile);
        return  (PX_ERROR);
    }
    
    if (pfatvolNew != pfatfile->FATFIL_pfatvol) {                       /*  ����Ϊͬһ����              */
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    if (cNewPath[0] == PX_DIVIDER) {                                    /*  FatFs �ļ�ϵͳ rename �ĵ�  */
        pcNewPath++;                                                    /*  2 ������������'/'Ϊ��ʼ�ַ� */
    }
    
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        f_sync(&pfatfile->FATFIL_fftm.FFTM_file);

    } else if (_PathMoveCheck(pfatfile->FATFIL_cName, pcNewPath)) {     /*  �����Ŀ¼������          */
        __FAT_FILE_UNLOCK(pfatfile);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    fresError = f_rename_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                            pfatfile->FATFIL_cName, pcNewPath);         /*  ��ʼ����������              */
    if (fresError == FR_EXIST) {
        if (f_stat_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                      pcNewPath, &fileinfo)) {
            __FAT_FILE_UNLOCK(pfatfile);
            _ErrorHandle(EIO);
            return  (PX_ERROR);
        }
        if ((pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) &&
            (fileinfo.fattrib & AM_DIR)) {
            __FAT_FILE_UNLOCK(pfatfile);
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        }
        if ((pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DIR) &&
            !(fileinfo.fattrib & AM_DIR)) {
            __FAT_FILE_UNLOCK(pfatfile);
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
        
        fresError = f_unlink_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                                pcNewPath);                             /*  ɾ��Ŀ��                    */
        if (fresError == FR_DENIED) {
            if (fileinfo.fattrib & AM_RDO) {
                ulError = EROFS;
            } else {
                ulError = ENOTEMPTY;
            }
            __FAT_FILE_UNLOCK(pfatfile);
            _ErrorHandle(ulError);
            return  (PX_ERROR);
        }
        
        fresError = f_rename_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                                pfatfile->FATFIL_cName, pcNewPath);     /*  ����������                  */
    }
    
    if (fresError) {
        ulError = __fatFsGetError(fresError);
        iError  = PX_ERROR;
    
    } else {
        pfdnode->FDNODE_inode64 = (ino64_t)-1;                          /*  ��ʧЧ (�ȴ��� close)       */
        iError  = ERROR_NONE;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __fatFsSeek
** ��������: FAT FS �ļ���λ
** �䡡��  : pfdentry            �ļ����ƿ�
**           oftPos              ��λ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsSeek (PLW_FD_ENTRY  pfdentry, off_t  oftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_pfatvol->FATVOL_iFlag == O_RDONLY) {
        if (oftPos > pfdnode->FDNODE_oftSize) {                         /*  ֻ���ļ�ϵͳ���������ļ�    */
            __FAT_FILE_UNLOCK(pfatfile);
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
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
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __fatFsWhere
** ��������: FAT FS ����ļ���ǰ��дָ��λ�� (ʹ�ò�����Ϊ����ֵ, �� FIOWHERE ��Ҫ�����в�ͬ)
** �䡡��  : pfdentry            �ļ����ƿ�
**           poftPos             ��дָ��λ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        *poftPos = pfdentry->FDENTRY_oftPtr;
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __fatFsNRead
** ��������: FAT FS ����ļ�ʣ�����Ϣ��
** �䡡��  : pfdentry            �ļ����ƿ�
**           piPos               ʣ��������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsNRead (PLW_FD_ENTRY  pfdentry, INT  *piPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (piPos == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        *piPos = (INT)(pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr);
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __fatFsNRead64
** ��������: FAT FS ����ļ�ʣ�����Ϣ��
** �䡡��  : pfdentry            �ļ����ƿ�
**           poftPos             ʣ��������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        *poftPos = pfdnode->FDNODE_oftSize - pfdentry->FDENTRY_oftPtr;
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __fatFsVolLabel
** ��������: FAT FS �ļ�ϵͳ��괦����
** �䡡��  : pfdentry            �ļ����ƿ�
**           pcLabel             ��껺��
**           bSet                ���û��ǻ�ȡ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsVolLabel (PLW_FD_ENTRY  pfdentry, PCHAR  pcLabel, BOOL  bSet)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    ULONG          ulError;
    
    if (pcLabel == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    if (bSet) {
        fresError = f_setlabel_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol, pcLabel);
    } else {
        fresError = f_getlabel_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                                  "", pcLabel, LW_NULL);
    }
    ulError = __fatFsGetError(fresError);                               /*  ת��������                */
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    
    if (ulError) {
        return  (PX_ERROR);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __fatFsStatGet
** ��������: FAT FS ����ļ�״̬������
** �䡡��  : pfdentry            �ļ����ƿ�
**           pstat               stat �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsStatGet (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    FILINFO        fileinfo;

    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    lib_bzero(&fileinfo, sizeof(FILINFO));                              /*  ����                        */
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    fresError = f_stat_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                          (CPCHAR)pfatfile->FATFIL_cName,
                          &fileinfo);
    ulError = __fatFsGetError(fresError);                               /*  ת��������                */
    if (ulError != ERROR_NONE) {
        if (__STR_IS_ROOT(pfatfile->FATFIL_cName)) {                    /*  Ϊ��Ŀ¼                    */
            fileinfo.fsize    = 0;
            /*
             *  ��¼��Ĵ���ʱ��. FATVOL_uiTime Ϊ FAT ʱ���ʽ
             *  fdate Ϊ FATVOL_uiTime �� 16 λ, ftime Ϊ�� 16 λ.
             *  (FATVOL_uiTime & 0xFFFF) ���������.
             */
            fileinfo.fcdate   = (WORD)(pfatfile->FATFIL_pfatvol->FATVOL_uiTime >> 16);
            fileinfo.fctime   = (WORD)(pfatfile->FATFIL_pfatvol->FATVOL_uiTime & 0xFFFF);
            fileinfo.fdate    = fileinfo.fcdate;
            fileinfo.ftime    = fileinfo.fctime;
            fileinfo.fattrib  = AM_DIR;
            fileinfo.fname[0] = PX_ROOT;
            fileinfo.fname[1] = PX_EOS;
            ulError           = ERROR_NONE;
        } else {
            iError = PX_ERROR;
        }
    }
    
    if (iError == ERROR_NONE) {                                         /*  ת��Ϊ POSIX ��׼�ṹ       */
        fileinfo.fsize = pfdnode->FDNODE_oftSize;
        __filInfoToStat(pfdentry->FDENTRY_pdevhdrHdr,
                        &fileinfo, 
                        &pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                        pstat,
                        pfatfile->FATFIL_inode);
        pstat->st_uid = _G_uidFatDefault;
        pstat->st_gid = _G_gidFatDefault;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __fatFsStatfsGet
** ��������: FAT FS ����ļ�ϵͳ״̬������
** �䡡��  : pfdentry            �ļ����ƿ�
**           pstatfs             statfs �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsStatfsGet (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    INT            iError   = ERROR_NONE;
    
    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    if (__fsInfoToStatfs(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                         pfatfile->FATFIL_pfatvol->FATVOL_iFlag,
                         pstatfs,
                         pfatfile->FATFIL_pfatvol->FATVOL_iDrv) < 0) {
        iError = PX_ERROR;
    
    } else {
        if (pfatfile->FATFIL_pfatvol->FATVOL_iFlag == O_RDONLY) {
            pstatfs->f_flag |= ST_RDONLY;
        }
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __fatFsReadDir
** ��������: FAT FS ���ָ��Ŀ¼��Ϣ
** �䡡��  : pfdentry            �ļ����ƿ�
**           dir                 Ŀ¼�ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
             
    mode_t         mode;
    FILINFO        fileinfo;

    if (dir == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DEV) {            /*  ��Ŀ¼��ʽ                  */
        f_opendir_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol, 
                     &pfatfile->FATFIL_fftm.FFTM_fatdir,
                     &pfatfile->FATFIL_u64Uniq,
                     PX_STR_ROOT);                                      /*  ��Ŀ¼                    */
        pfatfile->FATFIL_iFileType =  __FAT_FILE_TYPE_DIR;              /*  ת��ΪĿ¼��ʽ              */
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    if (pfatfile->FATFIL_iFileType != __FAT_FILE_TYPE_DIR) {
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    pfatfile->FATFIL_fftm.FFTM_fatdir.dptr = (UINT16)dir->dir_pos;
    if (dir->dir_pos == 0) {
        f_readdir(&pfatfile->FATFIL_fftm.FFTM_fatdir, LW_NULL);         /*  rewind dir                  */
    }
    
    fresError = f_readdir(&pfatfile->FATFIL_fftm.FFTM_fatdir,
                          &fileinfo);
    ulError = __fatFsGetError(fresError);                               /*  ת��������                */
    if (ulError != ERROR_NONE) {                                        /*  ��������                    */
        iError  = PX_ERROR;
    
    } else if (fileinfo.fname[0] == PX_EOS) {                           /*  Ŀ¼����                    */
        iError  = PX_ERROR;                                             /*  ������ errno                */
    
    } else {
        dir->dir_pos = pfatfile->FATFIL_fftm.FFTM_fatdir.dptr;          /*  ��¼�´εص�                */
        
        /*
         *  �����ļ���
         */
        lib_strcpy(dir->dir_dirent.d_shortname, fileinfo.altname);
        if (fileinfo.fname[0]) {
            lib_strcpy(dir->dir_dirent.d_name, fileinfo.fname);         /*  �����ļ���                  */
        } else {
            lib_strcpy(dir->dir_dirent.d_name, fileinfo.altname);       /*  �����ļ���                  */
        }
        
        /*
         *  �����ļ�����
         */
        mode = __fsAttrToMode(fileinfo.fattrib);
        dir->dir_dirent.d_type = IFTODT(mode);                          /*  ת��Ϊ d_type               */
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __fatFsTimeset
** ��������: FAT FS �����ļ�ʱ��
** �䡡��  : pfdentry            �ļ����ƿ�
**           utim                utimbuf �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : Ŀǰ�˺����������޸�ʱ��.
*********************************************************************************************************/
static INT  __fatFsTimeset (PLW_FD_ENTRY  pfdentry, struct utimbuf  *utim)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    UINT32         dwTime;
    FILINFO        fileinfo;

    if (__STR_IS_ROOT(pfatfile->FATFIL_cName)) {                        /*  ����Ƿ�Ϊ�豸�ļ�          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  ��֧���豸������            */
        return  (PX_ERROR);
    }
    
    if (utim == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    dwTime = __timeToFatTime(&utim->modtime);
    fileinfo.fdate = (UINT16)(dwTime >> 16);
    fileinfo.ftime = (UINT16)(dwTime & 0xFFFF);
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    fresError = f_utime_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol,
                           (CPCHAR)pfatfile->FATFIL_cName,
                           &fileinfo);
    ulError = __fatFsGetError(fresError);                               /*  ת��������                */
    if (ulError != ERROR_NONE) {
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __fatFsFillZero
** ��������: FAT FS ��� 0.
** �䡡��  : pfatfile            FAT �ļ����ƿ�
**           oftSize             �ļ���С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fatFsFillZero (PFAT_FILE  pfatfile, off_t  oftSize)
{
    static const CHAR     cZero[2048];                                  /*  ���� C ����ʱ����           */
    
    REGISTER INT          i;
    REGISTER INT          iTimes  = (INT)(oftSize >> 11);
    REGISTER UINT         uiSpace = (INT)(oftSize &  2047);
             UINT         uiWrNum;
    
    for (i = 0; i < iTimes; i++) {
        f_write(&pfatfile->FATFIL_fftm.FFTM_file, cZero, sizeof(cZero), &uiWrNum);
    }
    
    if (uiSpace) {
        f_write(&pfatfile->FATFIL_fftm.FFTM_file, cZero, uiSpace, &uiWrNum);
    }
}
/*********************************************************************************************************
** ��������: __fatFsTruncate
** ��������: FAT FS �����ļ���С
** �䡡��  : pfdentry            �ļ����ƿ�
**           oftSize             �ļ���С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsTruncate (PLW_FD_ENTRY  pfdentry, off_t  oftSize)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    off_t          oftOldSize;
    off_t          oftNewSize;
             
    if ((oftSize < 0) || (oftSize > (off_t)((DWORD)(~0)))) {            /*  FAT �ļ������� 4GB ��       */
        _ErrorHandle(EOVERFLOW);
        return  (PX_ERROR);
    
    } else {
        oftNewSize = oftSize;
    }

    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        oftOldSize = f_size(&pfatfile->FATFIL_fftm.FFTM_file);          /*  ��õ�ǰ�ļ���С            */
        
        if (oftSize > pfdnode->FDNODE_oftSize) {                        /*  ��Ҫ�Ŵ��ļ�                */
            fresError = f_lseek(&pfatfile->FATFIL_fftm.FFTM_file, oftNewSize);
            if (fresError == FR_OK) {                                   /*  POSIX �涨�¿ռ������� 0  */
                f_lseek(&pfatfile->FATFIL_fftm.FFTM_file, oftOldSize);  /*  �ļ�ָ���ƶ��� old size ��  */
                __fatFsFillZero(pfatfile, oftNewSize - oftOldSize);
            }
        
        } else if (oftSize < pfdnode->FDNODE_oftSize) {                 /*  ��С�ļ�                    */
            fresError = f_lseek(&pfatfile->FATFIL_fftm.FFTM_file, oftNewSize);
            if (fresError == FR_OK) {                                   /*  ��ָ��λ�ýض��ļ�          */
                fresError = f_truncate(&pfatfile->FATFIL_fftm.FFTM_file);
            }
        
        } else {                                                        /*  û�б仯                    */
            fresError = FR_OK;
        }
                                                                        /*  �����ļ���С                */
        pfdnode->FDNODE_oftSize = f_size(&pfatfile->FATFIL_fftm.FFTM_file);
        
        ulError = __fatFsGetError(fresError);                           /*  ת��������                */
    
    } else {
        ulError = EISDIR;
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __fatFsSync
** ��������: FAT FS ���ļ�����д�����
** �䡡��  : pfdentry            �ļ����ƿ�
**           bFlushCache         �Ƿ�ͬʱ��� CACHE
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsSync (PLW_FD_ENTRY  pfdentry, BOOL  bFlushCache)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    if (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) {
        fresError = f_sync(&pfatfile->FATFIL_fftm.FFTM_file);           /*  ��ջ���                    */
    } else {                                                            /*  ���ļ�ϵͳ�ڻ������ݻ�д    */
        fresError = f_syncfs(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol);
    }
    ulError = __fatFsGetError(fresError);                               /*  ת��������                */
    
    if (bFlushCache) {
        iError = __blockIoDevIoctl(pfatfile->FATFIL_pfatvol->FATVOL_iDrv,
                                   FIOSYNC, 0);                         /*  ��� CACHE ��д����         */
        if (iError < 0) {
            ulError = ERROR_IO_DEVICE_ERROR;                            /*  �豸����, �޷����          */
        }
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __fatFsSync
** ��������: FAT FS ���ļ�����д�����
** �䡡��  : pfdentry            �ļ����ƿ�
**           iMode               �µ� mode
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsChmod (PLW_FD_ENTRY  pfdentry, INT  iMode)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    FRESULT        fresError;
    INT            iError   = ERROR_NONE;
    ULONG          ulError  = ERROR_NONE;
    BYTE           ucAttr;
    
    iMode |= S_IRUSR;
    
    if (__FAT_FILE_LOCK(pfatfile) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    if ((pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_NODE) ||
        (pfatfile->FATFIL_iFileType == __FAT_FILE_TYPE_DIR)) {
        ucAttr = __fsModeToAttr(iMode);
        fresError = f_chmod_ex(&pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol, 
                               pfatfile->FATFIL_cName, 
                               ucAttr, (BYTE)0xFF);
        ulError = __fatFsGetError(fresError);                           /*  ת��������                */
    } else {
        ulError = ENOSYS;
        iError  = PX_ERROR;
    }
    __FAT_FILE_UNLOCK(pfatfile);
    
    _ErrorHandle(ulError);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __fatFsIoctl
** ��������: FAT FS ioctl ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           request,            ����
**           arg                 �������
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fatFsIoctl (PLW_FD_ENTRY  pfdentry,
                          INT           iRequest,
                          LONG          lArg)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PFAT_FILE      pfatfile = (PFAT_FILE)pfdnode->FDNODE_pvFile;
    INT            iError;
    off_t          oftTemp  = 0;                                        /*  ��ʱ����                    */
    
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
        if (pfatfile->FATFIL_pfatvol->FATVOL_iFlag == O_RDONLY) {
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
	}
	
	switch (iRequest) {                                                 /*  ֻ���ļ�ϵͳ�ж�            */
    
    case FIORENAME:
    case FIOTIMESET:
    case FIOCHMOD:
        if (pfatfile->FATFIL_pfatvol->FATVOL_iFlag == O_RDONLY) {
            _ErrorHandle(EROFS);
            return  (PX_ERROR);
        }
    }

    switch (iRequest) {
    
    case FIODISKFORMAT:                                                 /*  ���ʽ��                    */
        return  (__fatFsFormat(pfdentry, lArg));
    
    case FIODISKINIT:                                                   /*  ���̳�ʼ��                  */
        return  (__blockIoDevIoctl(pfatfile->FATFIL_pfatvol->FATVOL_iDrv,
                                   FIODISKINIT, lArg));
    
    case FIORENAME:                                                     /*  �ļ�������                  */
        return  (__fatFsRename(pfdentry, (PCHAR)lArg));
    
    /*
     *  FIOSEEK, FIOWHERE is 64 bit operate.
     */
    case FIOSEEK:                                                       /*  �ļ��ض�λ                  */
        oftTemp = *(off_t *)lArg;
        return  (__fatFsSeek(pfdentry, oftTemp));
    
    case FIOWHERE:                                                      /*  ����ļ���ǰ��дָ��        */
        iError = __fatFsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
    
    case FIONREAD:                                                      /*  ����ļ�ʣ���ֽ���          */
        return  (__fatFsNRead(pfdentry, (INT *)lArg));
    
    case FIONREAD64:                                                    /*  ����ļ�ʣ���ֽ���          */
        iError = __fatFsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
    
    case FIOLABELGET:                                                   /*  ��ȡ���                    */
        return  (__fatFsVolLabel(pfdentry, (PCHAR)lArg, LW_FALSE));
    
    case FIOLABELSET:                                                   /*  ���þ��                    */
        return  (__fatFsVolLabel(pfdentry, (PCHAR)lArg, LW_TRUE));
    
    case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
        return  (__fatFsStatGet(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  ����ļ�ϵͳ״̬            */
        return  (__fatFsStatfsGet(pfdentry, (struct statfs *)lArg));
        
    case FIOREADDIR:                                                    /*  ��ȡһ��Ŀ¼��Ϣ            */
        return  (__fatFsReadDir(pfdentry, (DIR *)lArg));
    
    case FIOTIMESET:                                                    /*  �����ļ�ʱ��                */
        return  (__fatFsTimeset(pfdentry, (struct utimbuf *)lArg));

    /*
     *  FIOTRUNC is 64 bit operate.
     */
    case FIOTRUNC:                                                      /*  �ı��ļ���С                */
        oftTemp = *(off_t *)lArg;
        return  (__fatFsTruncate(pfdentry, oftTemp));
        
    case FIOSYNC:                                                       /*  ���ļ������д              */
    case FIODATASYNC:
        return  (__fatFsSync(pfdentry, LW_TRUE /*  LW_FALSE  ???*/));
        
    case FIOFLUSH:
        return  (__fatFsSync(pfdentry, LW_TRUE));                       /*  �ļ��뻺��ȫ����д          */
        
    case FIOCHMOD:
        return  (__fatFsChmod(pfdentry, (INT)lArg));                    /*  �ı��ļ�����Ȩ��            */
        
    case FIOSETFL:                                                      /*  �����µ� flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);
        
    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        switch (pfatfile->FATFIL_pfatvol->FATVOL_fatfsVol.fs_type) {
        
        case FS_FAT12:
            *(PCHAR *)lArg = _G_pcFat12FsString;
            break;
            
        case FS_FAT16:
            *(PCHAR *)lArg = _G_pcFat16FsString;
            break;
            
        case FS_FAT32:
            *(PCHAR *)lArg = _G_pcFat32FsString;
            break;
            
        case FS_EXFAT:
            *(PCHAR *)lArg = _G_pcExFatFsString;
            break;
            
        default:
            *(PCHAR *)lArg = "Unknown FAT Type";
        }
        return  (ERROR_NONE);
        
    case FIOFSGETFL:                                                    /*  ��ȡ�ļ�ϵͳȨ��            */
        if ((INT *)lArg == LW_NULL) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        *(INT *)lArg = pfatfile->FATFIL_pfatvol->FATVOL_iFlag;
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
        pfatfile->FATFIL_pfatvol->FATVOL_iFlag = (INT)lArg;
        KN_SMP_WMB();
        return  (ERROR_NONE);
        
    case FIOGETFORCEDEL:                                                /*  ǿ��ж���豸�Ƿ�����      */
        *(BOOL *)lArg = pfatfile->FATFIL_pfatvol->FATVOL_bForceDelete;
        return  (ERROR_NONE);
        
    case FIOSETFORCEDEL:                                                /*  ����ǿ��ж��ʹ��            */
        pfatfile->FATFIL_pfatvol->FATVOL_bForceDelete = (BOOL)lArg;
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
#endif                                                                  /*  LW_CFG_FS_SELECT_EN > 0     */

    default:                                                            /*  �޷�ʶ�������              */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    return  (PX_ERROR);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_FATFS_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
