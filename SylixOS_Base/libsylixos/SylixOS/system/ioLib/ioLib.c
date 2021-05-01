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
** ��   ��   ��: ioLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 13 ��
**
** ��        ��: ϵͳ IO �ڲ����ܺ����⣬

** BUG
2007.09.12  ����Բü���֧��
2007.09.24  ���� _DebugHandle() ���ܡ�
2008.01.16  �޸��� IO ϵͳ���ź���������.
2008.03.23  �޸Ĵ����ʽ.
2008.10.17  ��ϵͳ�����������߹ر�ʱ, ���Ƴ����е��豸.
2009.02.13  �޸Ĵ�������Ӧ�µ� I/O ϵͳ�ṹ��֯.
2009.03.16  _IosDeleteAll() ��Ҫ�����д򿪵��ļ���д����.
2009.06.25  _IosDeleteAll() ��Ҫ�Ա�ͷ���������ж�.
2009.08.27  ��ǰ·��Ĭ��Ϊ "/" (root fs)
2009.11.21  ��������ɾ���ص�.
            ���� io ������ʼ��.
2009.12.13  ����֧�� VxWorks ʽ�ĵ����豸Ŀ¼����� SylixOS �µķּ�Ŀ¼����.
2010.07.10  /dev/null �豸֧�ִ���رղ���.
2012.10.25  ���� _IosEnvInherit() ����, ��������ѡ���Եļ̳�ϵͳ��ǰĿ¼���ߵ�ǰ����Ŀ¼.
2013.01.09  IO �����м���� umask ��֧��.
2013.01.21  �������Ȩ��.
2013.01.22  ���� zero �豸����.
2013.08.14  _IosThreadDelete() �м���������ļ�����Ϣ���Ƴ�.
2013.09.29  _IosDeleteAll() �м���� PCI �豸�ĸ�λ.
2013.11.20  _IosInit() ����� eventfd, timerfd, signalfd �ĳ�ʼ��.
2016.10.22  ��ϵͳ�������.
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#define  __SYSTEM_MAIN_FILE
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����Բü���֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "limits.h"
/*********************************************************************************************************
  �ļ�ϵͳ���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
#include "../SylixOS/fs/include/fs_fs.h"
#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
/*********************************************************************************************************
  ���̻���
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  �Ƿ���Ҫ�µķּ�Ŀ¼����
*********************************************************************************************************/
#if LW_CFG_PATH_VXWORKS == 0
#include "../SylixOS/fs/rootFs/rootFs.h"
#include "../SylixOS/fs/rootFs/rootFsLib.h"
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
/*********************************************************************************************************
  �ļ�ϵͳ��ʼ������
*********************************************************************************************************/
#if LW_CFG_OEMDISK_EN > 0
extern VOID    API_OemDiskMountInit(VOID);
#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_MOUNT_EN > 0)
extern VOID    API_MountInit(VOID);
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_MOUNT_EN > 0         */
/*********************************************************************************************************
  �ڲ�ȫ�ֱ���
*********************************************************************************************************/
static  LW_DEV_HDR            _G_devhdrNull;                            /*  �����豸ͷ                  */
static  LW_DEV_HDR            _G_devhdrZero;                            /*  zero�豸ͷ                  */
static  LW_OBJECT_HANDLE      _G_hIosMSemaphore;                        /*  IO(s) ��                    */
/*********************************************************************************************************
** ��������: _IosLock
** ��������: ����IO(s)ϵͳ�ٽ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _IosLock (VOID)
{
    API_SemaphoreMPend(_G_hIosMSemaphore, LW_OPTION_WAIT_INFINITE);
}
/*********************************************************************************************************
** ��������: _IosUnlock
** ��������: �˳�IO(s)ϵͳ�ٽ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _IosUnlock (VOID)
{
    API_SemaphoreMPost(_G_hIosMSemaphore);
}
/*********************************************************************************************************
** ��������: _IosCheckPermissions
** ��������: �жϷ���Ȩ��
** �䡡��  : iFlag            �� flag
**           bExec            �Ƿ��ִ��
**           mode             �ļ� mode
**           uid              �ļ� uid
**           gid              �ļ� gid
** �䡡��  : �Ƿ�ӵ�в���Ȩ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _IosCheckPermissions (INT  iFlag, BOOL  bExec, mode_t  mode, uid_t  uid, gid_t  gid)
{
    uid_t       uidExecCur = geteuid();
    gid_t       gidExecCur = getegid();
    mode_t      modeNeed = 0;
    
    INT         i;
    INT         iNum;
    gid_t       gidList[NGROUPS_MAX];
    
    if (uidExecCur == 0) {                                              /*  �����û�                    */
        return  (ERROR_NONE);
    }
    
    if (uidExecCur == uid) {
        mode = (mode & S_IRWXU) >> 6;                                   /*  owner permission            */
        
    } else if (gidExecCur == gid) {
        mode = (mode & S_IRWXG) >> 3;                                   /*  group permission            */
    
    } else {
        iNum = getgroups(NGROUPS_MAX, gidList);                         /*  �鿴��������Ϣ              */
        for (i = 0; i < iNum; i++) {
            if (gidList[i] == gid) {                                    /*  ��������ͬ������            */
                break;
            }
        }
        
        if (i < iNum) {
            mode = (mode & S_IRWXG) >> 3;                               /*  group permission            */
        
        } else {
            mode = (mode & S_IRWXO);                                    /*  other permission            */
        }
    }
    
    if (iFlag & (O_WRONLY | O_RDWR | O_TRUNC)) {                        /*  write                       */
        modeNeed |= S_IWOTH;
        
    } else {
        modeNeed |= S_IROTH;
    }
    
    if (bExec) {
        modeNeed |= S_IXOTH;
    }
    
    if ((modeNeed & mode) == modeNeed) {                                /*  ����Ƿ�����Ȩ��            */
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: _IosNullOpen
** ��������: ϵͳ�������豸�����򿪺���
** �䡡��  : pdevhdr       �豸ͷ
**           pcName        ����
**           iFlags        �򿪱�־
**           iMode         mode_t
** �䡡��  : �ļ�ָ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  _IosNullOpen (LW_DEV_HDR   *pdevhdr, 
                           PCHAR         pcName,   
                           INT           iFlags, 
                           INT           iMode)
{
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    } else {
        return  ((LONG)pdevhdr);
    }
}
/*********************************************************************************************************
** ��������: _IosNullClose
** ��������: ϵͳ�������豸�����رպ���
** �䡡��  : pdevhdr       �豸ͷ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _IosNullClose (LW_DEV_HDR   *pdevhdr)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _IosNullWrite
** ��������: ϵͳ�������豸����д����
** �䡡��  : pdevhdr       �豸ͷ
**           pcBuffer      ��Ҫд�������
**           stNByte       ��Ҫд��ĳ���
** �䡡��  : д�볤��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _IosNullWrite (LW_DEV_HDR  *pdevhdr, CPCHAR  pcBuffer, size_t  stNByte)
{
    (VOID)pdevhdr;
    (VOID)pcBuffer;
    
    return  ((ssize_t)stNByte);
}
/*********************************************************************************************************
** ��������: _IosNullIoctl
** ��������: ϵͳ�������豸���� ioctl ����
** �䡡��  : pdevhdr       �豸ͷ
**           iRequest      ��������
**           lArg          �������
** �䡡��  : ����ִ�н��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _IosNullIoctl (LW_DEV_HDR *pdevhdr,
                           INT         iRequest,
                           LONG        lArg)
{
    struct stat     *pstat = (struct stat *)lArg;

    if (iRequest == FIOFSTATGET) {
        if (!pstat) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        if (pdevhdr == &_G_devhdrNull) {
            pstat->st_dev     = LW_DEV_MAKE_STDEV(pdevhdr);
            pstat->st_ino     = 0;
            pstat->st_mode    = S_IWUSR | S_IWGRP | S_IWOTH | S_IFCHR;  /*  Ĭ������                    */
            pstat->st_nlink   = 0;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            pstat->st_atime   = API_RootFsTime(LW_NULL);                /*  Ĭ��ʹ�� root fs ��׼ʱ��   */
            pstat->st_mtime   = API_RootFsTime(LW_NULL);
            pstat->st_ctime   = API_RootFsTime(LW_NULL);
        
        } else {
            pstat->st_dev     = LW_DEV_MAKE_STDEV(pdevhdr);
            pstat->st_ino     = 0;
            pstat->st_mode    = S_IRUSR | S_IRGRP | S_IROTH | S_IFCHR;  /*  Ĭ������                    */
            pstat->st_nlink   = 0;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            pstat->st_atime   = API_RootFsTime(LW_NULL);                /*  Ĭ��ʹ�� root fs ��׼ʱ��   */
            pstat->st_mtime   = API_RootFsTime(LW_NULL);
            pstat->st_ctime   = API_RootFsTime(LW_NULL);
        }
    }

    _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: _IosZeroRead
** ��������: ϵͳ zero �豸����������
** �䡡��  : pdevhdr       �豸ͷ
**           pcBuffer      ��Ҫ��������ݻ���
**           stSize        �����С
** �䡡��  : ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  _IosZeroRead (LW_DEV_HDR  *pdevhdr, PCHAR  pcBuffer, size_t  stSize)
{
    if (!pcBuffer || !stSize) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    lib_bzero(pcBuffer, stSize);
    
    return  ((ssize_t)stSize);
}
/*********************************************************************************************************
** ��������: _IosEnvGlobalInit
** ��������: ��ʼ��ȫ�� IO ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _IosEnvGlobalInit (VOID)
{
    _S_ioeIoGlobalEnv.IOE_modeUMask = 0;                                /*  umask 0                     */
    lib_strcpy(_S_ioeIoGlobalEnv.IOE_cDefPath, PX_STR_ROOT);            /*  ��ʼ��Ϊ��Ŀ¼              */
}
/*********************************************************************************************************
** ��������: _IosEnvCreate
** ��������: ����һ�� IO ����
** �䡡��  : NONE
** �䡡��  : ������ IO �������ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_IO_ENV  _IosEnvCreate (VOID)
{
    PLW_IO_ENV      pioe;
    
    pioe = (PLW_IO_ENV)__SHEAP_ALLOC(sizeof(LW_IO_ENV));
    if (pioe == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(pioe, sizeof(LW_IO_ENV));
    
    return  (pioe);
}
/*********************************************************************************************************
** ��������: _IosEnvDelete
** ��������: �ͷ�һ�� IO ����
** �䡡��  : pioe      IO �������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _IosEnvDelete (PLW_IO_ENV  pioe)
{
    if (pioe) {
        __SHEAP_FREE(pioe);
    }
}
/*********************************************************************************************************
** ��������: _IosEnvGetDefault
** ��������: ��õ�ǰĬ�ϵ� I/O ����
** �䡡��  : NONE
** �䡡��  : ��õ�ǰʹ�õ� I/O �������ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_IO_ENV  _IosEnvGetDef (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
#if LW_CFG_MODULELOADER_EN > 0
    PLW_IO_ENV      pioe;
#endif
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  ���ж��е���                */
        return  (&_S_ioeIoGlobalEnv);                                   /*  ʹ��ȫ�� IO ����            */
    
    } else {
        LW_TCB_GET_CUR_SAFE(ptcbCur);                                   /*  ��ǰ������ƿ�              */
        
#if LW_CFG_MODULELOADER_EN > 0
        pioe = vprocIoEnvGet(ptcbCur);                                  /*  ��õ�ǰ���� IO ����        */
        if (pioe) {
            return  (pioe);
        }
#endif
        
        if (ptcbCur->TCB_pvIoEnv) {
            return  ((PLW_IO_ENV)ptcbCur->TCB_pvIoEnv);
                                                                        /*  ʹ��˽�� IO ����            */
        } else {
            return  (&_S_ioeIoGlobalEnv);
        }
    }
}
/*********************************************************************************************************
** ��������: _IosEnvInherit
** ��������: �̳е�ǰ�� I/O ����
** �䡡��  : pioe      I/O �������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _IosEnvInherit (PLW_IO_ENV  pioe)
{
    PLW_IO_ENV  pioeDef = _IosEnvGetDef();

    if (pioe) {
        pioe->IOE_modeUMask = pioeDef->IOE_modeUMask;
        lib_strcpy(pioe->IOE_cDefPath, pioeDef->IOE_cDefPath);
    }
}
/*********************************************************************************************************
** ��������: _IosDeleteAll
** ��������: �Ƴ����в���ϵͳ�е��豸
** �䡡��  : iRebootType   ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _IosDeleteAll (INT  iRebootType)
{
    REGISTER PLW_DEV_HDR    pdevhdr;
             PLW_LIST_LINE  plineTemp;

    _IosLock();
    for (plineTemp  = _S_plineDevHdrHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pdevhdr = _LIST_ENTRY(plineTemp, LW_DEV_HDR, DEVHDR_lineManage);
        _IosUnlock();
        
        API_IosDevFileAbnormal(pdevhdr);                                /*  ��Ӧ�ļ�ǿ�ƹر�            */
        
        _IosLock();
    }
    _IosUnlock();
    
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
    API_DiskCacheSync(LW_NULL);                                         /*  ��д���д��̻���            */
#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
}
/*********************************************************************************************************
** ��������: _IosThreadDelete
** ��������: �߳�ɾ���ص�
** �䡡��  : ulId      �߳� ID
**           pvRetVal  ����ֵ
**           ptcbDel   �߳̿��ƿ�
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _IosThreadDelete (LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcbDel)
{
    extern VOID  __fdLockfCleanupHook(PLW_CLASS_TCB  ptcbDel);

    if (ptcbDel && ptcbDel->TCB_pvIoEnv) {
        _IosEnvDelete((PLW_IO_ENV)ptcbDel->TCB_pvIoEnv);                /*  ɾ��˽�� io ����            */
        ptcbDel->TCB_pvIoEnv = LW_NULL;                                 /*  ��ֹ�������������          */
    }
    
    __fdLockfCleanupHook(ptcbDel);                                      /*  ɾ�������ļ���              */
}
/*********************************************************************************************************
** ��������: _IosInit
** ��������: ��ʼ�� IO ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _IosInit (VOID)
{
#if LW_CFG_DEVICE_EN > 0
    INT     iNullDrv;
    INT     iZeroDrv;
    
    _IosEnvGlobalInit();                                                /*  ��ʼ�� io ����              */
    
    _S_iIoMaxLinkLevels = LINK_MAX;                                     /*  Ĭ����༶�� LINK_MAX ��    */
    
    lib_bzero(_S_deventryTbl, sizeof(_S_deventryTbl));                  /*  ������������              */
    
    _S_plineDevHdrHeader = LW_NULL;                                     /*  ��ʼ���豸ͷ����            */
    
    _G_hIosMSemaphore = API_SemaphoreMCreate("ios_mutex", LW_PRIO_DEF_CEILING, 
                                             LW_OPTION_WAIT_PRIORITY | 
                                             LW_OPTION_DELETE_SAFE | 
                                             LW_OPTION_INHERIT_PRIORITY |
                                             LW_OPTION_OBJECT_GLOBAL, 
                                             LW_NULL);
    if (!_G_hIosMSemaphore) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "io system mutex can not be create.\r\n");
        return  (PX_ERROR);
    }
    
    iNullDrv = API_IosDrvInstall(_IosNullOpen, 
                                 LW_NULL,
                                 _IosNullOpen,
                                 _IosNullClose,
                                 LW_NULL,
                                 _IosNullWrite,
                                 _IosNullIoctl);                        /*  ���ȴ��� NULL �豸����      */
    
    DRIVER_LICENSE(iNullDrv,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(iNullDrv,      "Han.hui");
    DRIVER_DESCRIPTION(iNullDrv, "null device driver.");
    
    iZeroDrv = API_IosDrvInstall(_IosNullOpen, 
                                 LW_NULL,
                                 _IosNullOpen,
                                 _IosNullClose,
                                 _IosZeroRead,
                                 LW_NULL,
                                 _IosNullIoctl);                        /*  ���� ZERO �豸����          */
                                 
    DRIVER_LICENSE(iZeroDrv,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(iZeroDrv,      "Han.hui");
    DRIVER_DESCRIPTION(iZeroDrv, "zero device driver.");
                                 
#if LW_CFG_PATH_VXWORKS == 0                                            /*  �Ƿ�ּ�Ŀ¼����            */
    rootFsDrv();                                                        /*  ��װ rootFs ����            */
    rootFsDevCreate();                                                  /*  �������豸                  */
    
    API_RootFsMakeNode("/dev", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME, 
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  �豸Ŀ¼                    */
    API_RootFsMakeNode("/dev/pty", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME,
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  �����ն��豸                */
    API_RootFsMakeNode("/dev/pipe", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME,
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  �ܵ��豸                    */
    API_RootFsMakeNode("/dev/input", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME,
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  �����豸                    */
    API_RootFsMakeNode("/dev/blk", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME,
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  blk raw �豸                */
    API_RootFsMakeNode("/mnt", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME, 
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  �ļ�ϵͳ����Ŀ¼            */
    API_RootFsMakeNode("/media", LW_ROOTFS_NODE_TYPE_DIR, LW_ROOTFS_NODE_OPT_ROOTFS_TIME, 
                       DEFAULT_DIR_PERM, LW_NULL);                      /*  �Ȳ�δ洢������Ŀ¼        */
                                                                        /*  unix ���� null �豸·��     */
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    (VOID)API_IosDevAddEx(&_G_devhdrNull, "/dev/null", iNullDrv, DT_CHR); 
    (VOID)API_IosDevAddEx(&_G_devhdrZero, "/dev/zero", iZeroDrv, DT_CHR);
    
    (VOID)API_SystemHookAdd((LW_HOOK_FUNC)_IosThreadDelete, 
                            LW_OPTION_THREAD_DELETE_HOOK);              /*  �߳�ɾ��ʱ�ص�              */
    (VOID)API_SystemHookAdd((LW_HOOK_FUNC)_IosDeleteAll, 
                            LW_OPTION_KERNEL_REBOOT);                   /*  ��װϵͳ�ر�ʱ, �ص�����    */
    
#if LW_CFG_EVENTFD_EN > 0
    eventfdDrv();
    eventfdDevCreate();
#endif                                                                  /*  LW_CFG_EVENTFD_EN > 0       */

#if LW_CFG_BMSG_EN > 0
    bmsgDrv();
    bmsgDevCreate();
#endif                                                                  /*  LW_CFG_BMSG_EN > 0          */
    
#if LW_CFG_SEMFD_EN > 0
    semfdDrv();
    semfdDevCreate();
#endif                                                                  /*  LW_CFG_SEMFD_EN > 0         */

#if LW_CFG_PTIMER_EN > 0 && LW_CFG_TIMERFD_EN > 0
    timerfdDrv();
    timerfdDevCreate();
    hstimerfdDrv();
    hstimerfdDevCreate();
#endif                                                                  /*  LW_CFG_TIMERFD_EN > 0       */
    
#if LW_CFG_SIGNALFD_EN > 0
    signalfdDrv();
    signalfdDevCreate();
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
    
#if LW_CFG_GPIO_EN > 0
    gpiofdDrv();
    gpiofdDevCreate();
#endif                                                                  /*  LW_CFG_GPIO_EN > 0          */
    
#if LW_CFG_OEMDISK_EN > 0
    API_OemDiskMountInit();
#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_MOUNT_EN > 0)
    API_MountInit();
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_MOUNT_EN > 0         */
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
