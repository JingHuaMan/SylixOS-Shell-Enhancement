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
** ��   ��   ��: procFs.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 03 ��
**
** ��        ��: proc �ļ�ϵͳ.

** BUG:
2009.12.11  �ڴ�����������ʱ, ��Ӧ�ü��� proc ���Ĵ���.
2009.12.29  ����ע��.
2010.05.05  proc �ļ�ϵͳ�� f_type Ϊ PROC_SUPER_MAGIC.
2010.09.10  ����� d_type ��֧��.
2011.08.11  ѹ��Ŀ¼�������ļ�ϵͳ���, ����ϵͳ�Ѵ�����.
2012.03.10  __procFsReadDir() ��Ŀ¼��ȡ��Ϻ�, errno = ENOENT.
2012.04.01  proc �ļ�ϵͳ�����������ʵ��ļ�Ȩ�޹���.
2012.08.16  ������ pread ����.
2012.08.26  �����˷������ӵĹ���.
2012.11.09  __procFsReadlink() ������Ч�ֽ���.
            lstat() �����ļ�ʱ��ʾ��ȷ�ĳ���.
2013.01.21  �Ƴ�����Ȩ���ж�, ���� IO ϵͳͳһ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_type.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "procFs.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
/*********************************************************************************************************
  �ڲ���Ϣ�ڵ��ʼ��
*********************************************************************************************************/
#if LW_CFG_PROCFS_KERNEL_INFO > 0
extern  VOID  __procFsKernelInfoInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_KERNEL_INFO   */
#if LW_CFG_PROCFS_HOOK_INFO > 0
extern VOID   __procFsHookInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_HOOK_INFO > 0 */
#if LW_CFG_PROCFS_BSP_INFO > 0
extern  VOID  __procFsBspInfoInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_BSP_INFO      */
extern  VOID  __procFssupInit(VOID);
#if LW_CFG_POWERM_EN > 0
extern  VOID  __procFsPowerInit(VOID);
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */
/*********************************************************************************************************
  procfs �ļ����ƿ鼰ʱ���
*********************************************************************************************************/
static LW_DEV_HDR       _G_devhdrProc;                                  /*  procfs �豸                 */
static time_t           _G_timeProcFs;                                  /*  procfs ����ʱ��             */
/*********************************************************************************************************
  procfs ���豸��
*********************************************************************************************************/
static INT              _G_iProcDrvNum = PX_ERROR;
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
PLW_PROCFS_NODE  __procFsFindNode(CPCHAR            pcName, 
                                  PLW_PROCFS_NODE  *pp_pfsnFather, 
                                  BOOL             *pbRoot,
                                  BOOL             *pbLast,
                                  PCHAR            *ppcTail);
VOID             __procFsRemoveNode(PLW_PROCFS_NODE  p_pfsn);
/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)       ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
/*********************************************************************************************************
  ������������
*********************************************************************************************************/
static LONG     __procFsOpen(PLW_DEV_HDR  pdevhdr,
                          PCHAR           pcName,
                          INT             iFlags,
                          INT             iMode);
static INT      __procFsClose(PLW_PROCFS_NODE  p_pfsn);
static ssize_t  __procFsRead(PLW_PROCFS_NODE  p_pfsn, 
                             PCHAR            pcBuffer, 
                             size_t           stMaxBytes);
static ssize_t  __procFsPRead(PLW_PROCFS_NODE  p_pfsn, 
                              PCHAR            pcBuffer, 
                              size_t           stMaxBytes,
                              off_t            oftPos);
static ssize_t  __procFsWrite(PLW_PROCFS_NODE  p_pfsn, 
                              PCHAR            pcBuffer, 
                              size_t           stBytes);
static ssize_t  __procFsPWrite(PLW_PROCFS_NODE  p_pfsn, 
                               PCHAR            pcBuffer, 
                               size_t           stBytes,
                               off_t            oftPos);
static INT      __procFsLStatGet(PLW_DEV_HDR pdevhdr, 
                                 PCHAR  pcName, 
                                 struct stat *pstat);
static INT      __procFsIoctl(PLW_PROCFS_NODE  p_pfsn, 
                          INT              iRequest,
                          LONG             lArg);
static ssize_t  __procFsReadlink(PLW_DEV_HDR    pdevhdrHdr,
                                 PCHAR          pcName,
                                 PCHAR          pcLinkDst,
                                 size_t         stMaxSize);
/*********************************************************************************************************
** ��������: API_ProcFsDrvInstall
** ��������: ��װ procfs �ļ�ϵͳ��������
** �䡡��  : 
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_ProcFsDrvInstall (VOID)
{
    struct file_operations     fileop;

    if (_G_iProcDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    if (_G_ulProcFsLock == LW_OBJECT_HANDLE_INVALID) {
        _G_ulProcFsLock = API_SemaphoreMCreate("proc_lock", LW_PRIO_DEF_CEILING, LW_OPTION_WAIT_PRIORITY |
                                               LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                               LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __procFsOpen;
    fileop.fo_release  = LW_NULL;
    fileop.fo_open     = __procFsOpen;
    fileop.fo_close    = __procFsClose;
    fileop.fo_read     = __procFsRead;
    fileop.fo_read_ex  = __procFsPRead;
    fileop.fo_write    = __procFsWrite;
    fileop.fo_write_ex = __procFsPWrite;
    fileop.fo_lstat    = __procFsLStatGet;
    fileop.fo_ioctl    = __procFsIoctl;
    fileop.fo_readlink = __procFsReadlink;
    
    _G_iProcDrvNum = iosDrvInstallEx(&fileop);
     
    DRIVER_LICENSE(_G_iProcDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iProcDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iProcDrvNum, "procfs driver.");
    
    return  ((_G_iProcDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: API_ProcFsDevCreate
** ��������: ���� proc fs.
** �䡡��  : NONE
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_ProcFsDevCreate (VOID)
{
    static BOOL     bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return  (ERROR_NONE);
    }
    
    if (iosDevAddEx(&_G_devhdrProc, "/proc", _G_iProcDrvNum, DT_DIR) != ERROR_NONE) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    bIsInit = LW_TRUE;
    
#if LW_CFG_PROCFS_BSP_INFO > 0
    __procFsBspInfoInit();
#endif                                                                  /*  LW_CFG_PROCFS_BSP_INFO      */

#if LW_CFG_PROCFS_KERNEL_INFO > 0
    __procFsKernelInfoInit();                                           /*  �����ں���Ϣ�ڵ�            */
#endif                                                                  /*  LW_CFG_PROCFS_KERNEL_INFO   */

#if LW_CFG_PROCFS_HOOK_INFO > 0
    __procFsHookInit();
#endif                                                                  /*  LW_CFG_PROCFS_HOOK_INFO > 0 */

    __procFssupInit();
    
#if LW_CFG_POWERM_EN > 0
    __procFsPowerInit();
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

    lib_time(&_G_timeProcFs);                                           /*  �� UTC ��Ϊʱ���׼         */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __procFsOpen
** ��������: procfs ��һ���ļ� (�Ǵ���)
** �䡡��  : pdevhdr          procfs �豸
**           pcName           �ļ���
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : proc �ڵ� (LW_NULL ��ʾ proc ���ڵ�)
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  __procFsOpen (PLW_DEV_HDR     pdevhdr,
                           PCHAR           pcName,
                           INT             iFlags,
                           INT             iMode)
{
    PLW_PROCFS_NODE  p_pfsn;
    BOOL             bIsRoot;
    PCHAR            pcTail = LW_NULL;
    
    if (pcName == LW_NULL) {                                            /*  ���ļ���                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    if ((iFlags & O_CREAT) || (iFlags & O_TRUNC)) {                     /*  ���ܴ���                    */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    if ((iFlags & O_ACCMODE) != O_RDONLY) {                             /*  ������д (Ŀǰ)             */
        _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
        return  (PX_ERROR);
    }
    
    __LW_PROCFS_LOCK();                                                 /*  �� procfs                   */
    p_pfsn = __procFsFindNode(pcName, LW_NULL, &bIsRoot, LW_NULL, &pcTail);
    if (p_pfsn == LW_NULL) {                                            /*  Ϊ�ҵ��ڵ�                  */
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        if (bIsRoot) {
            LW_DEV_INC_USE_COUNT(&_G_devhdrProc);                       /*  ���¼�����                  */
            return  ((LONG)LW_NULL);
        } else {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
    }
    if (p_pfsn->PFSN_bReqRemove) {
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (p_pfsn->PFSN_iOpenNum) {                                        /*  ��֧���ظ���,             */
                                                                        /*  �ļ�ָ��ֻ��һ��            */
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    if (_IosCheckPermissions(iFlags, LW_FALSE,
                             p_pfsn->PFSN_mode,
                             p_pfsn->PFSN_uid,
                             p_pfsn->PFSN_gid)) {                       /*  Ȩ�޼��                    */
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        _ErrorHandle(EACCES);
        return  (PX_ERROR);
    }
    if ((iFlags & O_DIRECTORY) && 
        !S_ISDIR(p_pfsn->PFSN_mode) && 
        !S_ISLNK(p_pfsn->PFSN_mode)) {
        __LW_PROCFS_UNLOCK();
        _ErrorHandle(ENOTDIR);
        return  (PX_ERROR);
    }
    p_pfsn->PFSN_pfsnmMessage.PFSNM_oftPtr = 0;                         /*  �ļ�ָ���ԭλ              */
    p_pfsn->PFSN_iOpenNum++;
    __LW_PROCFS_UNLOCK();                                               /*  ���� procfs                 */
    
    if (p_pfsn && S_ISLNK(p_pfsn->PFSN_mode)) {                         /*  �����ļ�����                */
        INT     iError;
        INT     iFollowLinkType;
        PCHAR   pcSymfile = pcTail - lib_strlen(p_pfsn->PFSN_pcName) - 1;
        PCHAR   pcPrefix;
        
        if (*pcSymfile != PX_DIVIDER) {
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
        
        iError = _PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                LW_NULL, pcPrefix, 
                                (PCHAR)p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer, 
                                pcTail);
        
        __LW_PROCFS_LOCK();                                             /*  �� procfs                   */
        p_pfsn->PFSN_iOpenNum--;
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        
        if (iError) {
            return  (PX_ERROR);                                         /*  �޷�����������Ŀ��Ŀ¼      */
        } else {
            return  (iFollowLinkType);
        }
    }
    
    p_pfsn->PFSN_time = lib_time(LW_NULL);                              /*  �� UTC ʱ����Ϊʱ���׼     */
    
    LW_DEV_INC_USE_COUNT(&_G_devhdrProc);                               /*  ���¼�����                  */

    return  ((LONG)p_pfsn);
}
/*********************************************************************************************************
** ��������: __procFsClose
** ��������: procfs �ر�һ���ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __procFsClose (PLW_PROCFS_NODE  p_pfsn)
{
    if (p_pfsn) {
        API_ProcFsFreeNodeBuffer(p_pfsn);                               /*  �ͷŻ���                    */
        __LW_PROCFS_LOCK();                                             /*  �� procfs                   */
        p_pfsn->PFSN_iOpenNum--;
        if (p_pfsn->PFSN_iOpenNum == 0) {
            if (p_pfsn->PFSN_bReqRemove) {                              /*  ����ɾ���ڵ�                */
                __procFsRemoveNode(p_pfsn);                             /*  ɾ���ڵ�                    */
            }
        }
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
    }
    LW_DEV_DEC_USE_COUNT(&_G_devhdrProc);                               /*  ���¼�����                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __procFsRead
** ��������: procfs ��ȡһ���ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
** �䡡��  : ʵ�ʶ�ȡ�ĸ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsRead (PLW_PROCFS_NODE  p_pfsn, 
                              PCHAR            pcBuffer, 
                              size_t           stMaxBytes)
{
             ssize_t    sstNum = 0;
    REGISTER size_t     stBufferSize;                                   /*  �ļ���������С              */

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     *  ���ﲻ��Ҫ����, ��Ϊ�ļ����ر�, �ڵ㲻�����ͷ�.
     */
    if (!p_pfsn || !S_ISREG(p_pfsn->PFSN_mode)) {                       /*  ֻ�ܶ�ȡ reg �ļ�           */
        _ErrorHandle(EBADF);                                            /*  �ļ����ܶ�                  */
        return  (PX_ERROR);
    }
    
    if (!stMaxBytes) {
        return  (0);
    }
    
    if (p_pfsn->PFSN_p_pfsnoFuncs &&
        p_pfsn->PFSN_p_pfsnoFuncs->PFSNO_pfuncRead) {
        stBufferSize = p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize;    /*  ��õ�ǰ�����С            */
        
        /*
         *  �� PFSNM_ulNeedSize Ϊ 0 ʱ��ʾ, �ļ�Ԥ�ڴ�С��ȷ��, 
         *  ��Ҫ�ļ����������Լ�������������ļ������ڴ�.
         */
        if (stBufferSize < p_pfsn->PFSN_pfsnmMessage.PFSNM_stNeedSize) {/*  ����̫С                    */
            if (API_ProcFsAllocNodeBuffer(p_pfsn, 
                    p_pfsn->PFSN_pfsnmMessage.PFSNM_stNeedSize)) {
                                                                        /*  �����ļ�����                */
                _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
                return  (0);
            }
            p_pfsn->PFSN_pfsnmMessage.PFSNM_stRealSize = 0;             /*  û���㹻�Ļ���֤���ļ���û��*/
                                                                        /*  ��ʼ����                    */
        }
        
        sstNum = p_pfsn->PFSN_p_pfsnoFuncs->PFSNO_pfuncRead(p_pfsn,
                                                pcBuffer,
                                                stMaxBytes,
                                                p_pfsn->PFSN_pfsnmMessage.PFSNM_oftPtr);
        if (sstNum > 0) {
            p_pfsn->PFSN_pfsnmMessage.PFSNM_oftPtr += (off_t)sstNum;    /*  �����ļ�ָ��                */
        }
    } else {
        _ErrorHandle(ENOSYS);
    }

    return  (sstNum);
}
/*********************************************************************************************************
** ��������: __procFsPRead
** ��������: procfs ��ȡһ���ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oftPos        λ��
** �䡡��  : ʵ�ʶ�ȡ�ĸ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsPRead (PLW_PROCFS_NODE  p_pfsn, 
                               PCHAR            pcBuffer, 
                               size_t           stMaxBytes,
                               off_t            oftPos)
{
             ssize_t    sstNum = 0;
    REGISTER size_t     stBufferSize;                                   /*  �ļ���������С              */

    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     *  ���ﲻ��Ҫ����, ��Ϊ�ļ����ر�, �ڵ㲻�����ͷ�.
     */
    if (!p_pfsn || !S_ISREG(p_pfsn->PFSN_mode)) {                       /*  ֻ�ܶ�ȡ reg �ļ�           */
        _ErrorHandle(EISDIR);                                           /*  Ŀ¼���ܶ�                  */
        return  (PX_ERROR);
    }
    
    if (!stMaxBytes) {
        return  (0);
    }
    
    if (p_pfsn->PFSN_p_pfsnoFuncs &&
        p_pfsn->PFSN_p_pfsnoFuncs->PFSNO_pfuncRead) {
        stBufferSize = p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize;    /*  ��õ�ǰ�����С            */

        /*
         *  �� PFSNM_ulNeedSize Ϊ 0 ʱ��ʾ, �ļ�Ԥ�ڴ�С��ȷ��, 
         *  ��Ҫ�ļ����������Լ�������������ļ������ڴ�.
         */
        if (stBufferSize < p_pfsn->PFSN_pfsnmMessage.PFSNM_stNeedSize) {/*  ����̫С                    */
            if (API_ProcFsAllocNodeBuffer(p_pfsn, 
                    p_pfsn->PFSN_pfsnmMessage.PFSNM_stNeedSize)) {
                                                                        /*  �����ļ�����                */
                _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
                return  (0);
            }
            p_pfsn->PFSN_pfsnmMessage.PFSNM_stRealSize = 0;             /*  û���㹻�Ļ���֤���ļ���û��*/
                                                                        /*  ��ʼ����                    */
        }
        
        sstNum = p_pfsn->PFSN_p_pfsnoFuncs->PFSNO_pfuncRead(p_pfsn,
                                                pcBuffer,
                                                stMaxBytes,
                                                oftPos);
    } else {
        _ErrorHandle(ENOSYS);
    }

    return  (sstNum);
}
/*********************************************************************************************************
** ��������: __procFsWrite
** ��������: procfs д��һ���ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stBytes       ��Ҫд����ֽ���
** �䡡��  : ʵ��д��ĸ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsWrite (PLW_PROCFS_NODE  p_pfsn, 
                               PCHAR            pcBuffer, 
                               size_t           stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** ��������: __procFsPWrite
** ��������: procfs д��һ���ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stBytes       ��Ҫд����ֽ���
**           oftPos        λ��
** �䡡��  : ʵ��д��ĸ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsPWrite (PLW_PROCFS_NODE  p_pfsn, 
                                PCHAR            pcBuffer, 
                                size_t           stBytes,
                                off_t            oftPos)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** ��������: __procFsStatGet
** ��������: procfs ��ȡһ���ļ�����
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pstat         ��д���ļ�����
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __procFsStatGet (PLW_PROCFS_NODE  p_pfsn, struct stat *pstat)
{
    if (p_pfsn == LW_NULL) {                                            /*  procfs ���ڵ�               */
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_devhdrProc);
        pstat->st_ino     = (ino_t)0;                                   /*  ��Ŀ¼                      */
        pstat->st_mode    = (0644 | S_IFDIR);                           /*  Ĭ������                    */
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;                                          /*  owner is root               */
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = _G_timeProcFs;
        pstat->st_mtime   = _G_timeProcFs;
        pstat->st_ctime   = _G_timeProcFs;
    
    } else {                                                            /*  �ӽڵ�                      */
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_devhdrProc);
        pstat->st_ino     = (ino_t)p_pfsn;
        pstat->st_mode    = p_pfsn->PFSN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = p_pfsn->PFSN_uid;
        pstat->st_gid     = p_pfsn->PFSN_gid;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize;
        pstat->st_blocks  = 1;
        pstat->st_atime   = p_pfsn->PFSN_time;
        pstat->st_mtime   = p_pfsn->PFSN_time;
        pstat->st_ctime   = p_pfsn->PFSN_time;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __procFsLStatGet
** ��������: procfs ��ȡһ���ļ�����
** �䡡��  : pdevhdr       procfs �豸
**           pcName        �ļ���
**           pstat         ��д���ļ�����
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __procFsLStatGet (PLW_DEV_HDR pdevhdr, PCHAR  pcName, struct stat *pstat)
{
    PLW_PROCFS_NODE  p_pfsn;
    BOOL             bIsRoot;
    PCHAR            pcTail = LW_NULL;
    
    if (pcName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    __LW_PROCFS_LOCK();                                                 /*  �� procfs                   */
    p_pfsn = __procFsFindNode(pcName, LW_NULL, &bIsRoot, LW_NULL, &pcTail);
    if (p_pfsn == LW_NULL) {                                            /*  Ϊ�ҵ��ڵ�                  */
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (p_pfsn->PFSN_bReqRemove) {
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (p_pfsn) {
        p_pfsn->PFSN_time = lib_time(LW_NULL);                          /*  ��ǰ UTC ʱ��               */
    
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_devhdrProc);
        pstat->st_ino     = (ino_t)p_pfsn;
        pstat->st_mode    = p_pfsn->PFSN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = p_pfsn->PFSN_uid;
        pstat->st_gid     = p_pfsn->PFSN_gid;
        pstat->st_rdev    = 1;
        
        if (S_ISLNK(p_pfsn->PFSN_mode)) {
            pstat->st_size = lib_strlen((PCHAR)p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer);
        } else {
            pstat->st_size = 0;
        }
        
        pstat->st_blksize = p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize;
        pstat->st_blocks  = 1;
        pstat->st_atime   = p_pfsn->PFSN_time;
        pstat->st_mtime   = p_pfsn->PFSN_time;
        pstat->st_ctime   = p_pfsn->PFSN_time;
        
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        
        return  (ERROR_NONE);
    
    } else {
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __procFsStatfsGet
** ��������: procfs ��ȡ�ļ�ϵͳ����
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pstatfs       ��д���ļ�ϵͳ����
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __procFsStatfsGet (PLW_PROCFS_NODE  p_pfsn, struct statfs *pstatfs)
{
    if (pstatfs) {
        pstatfs->f_type   = PROC_SUPER_MAGIC;
        pstatfs->f_bsize  = 0;
        pstatfs->f_blocks = 1;
        pstatfs->f_bfree  = 0;
        pstatfs->f_bavail = 1;
        
        pstatfs->f_files  = (long)_G_pfsrRoot.PFSR_ulFiles;
        pstatfs->f_ffree  = 0;
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        pstatfs->f_fsid.val[0] = (int32_t)((addr_t)&_G_pfsrRoot >> 32);
        pstatfs->f_fsid.val[1] = (int32_t)((addr_t)&_G_pfsrRoot & 0xffffffff);
#else
        pstatfs->f_fsid.val[0] = (int32_t)&_G_pfsrRoot;
        pstatfs->f_fsid.val[1] = 0;
#endif
        
        pstatfs->f_flag    = ST_NOSUID;
        pstatfs->f_namelen = PATH_MAX;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __procFsReadDir
** ��������: procfs ��ȡĿ¼��
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pstatfs       ��д���ļ�ϵͳ����
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __procFsReadDir (PLW_PROCFS_NODE  p_pfsn, DIR  *dir)
{
             INT                i;
             INT                iError = ERROR_NONE;
    REGISTER LONG               iStart;
             PLW_PROCFS_NODE    p_pfsnTemp;
    
             PLW_LIST_LINE      plineTemp;
             PLW_LIST_LINE      plineHeader;                            /*  ��ǰĿ¼ͷ                  */
    
    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __LW_PROCFS_LOCK();                                                 /*  �� procfs                   */
    if (p_pfsn == LW_NULL) {                                            /*  procfs ���ڵ�               */
        plineHeader = _G_pfsrRoot.PFSR_plineSon;                        /*  �Ӹ�Ŀ¼��ʼ����            */
    } else {
        if (!S_ISDIR(p_pfsn->PFSN_mode)) {
            __LW_PROCFS_UNLOCK();                                       /*  ���� procfs                 */
            _ErrorHandle(ENOTDIR);                                      /*  ����һ��Ŀ¼                */
            return  (PX_ERROR);
        }
        plineHeader = p_pfsn->PFSN_plineSon;                            /*  �ӵ�һ�����ӿ�ʼѰ��        */
    }

    iStart = dir->dir_pos;
        
    for ((plineTemp  = plineHeader), (i = 0); 
         (plineTemp != LW_NULL) && (i < iStart); 
         (plineTemp  = _list_line_get_next(plineTemp)), (i++));         /*  ����                        */
    
    if (plineTemp == LW_NULL) {
        _ErrorHandle(ENOENT);
        iError = PX_ERROR;                                              /*  û�ж���Ľڵ�              */
    
    } else {
        p_pfsnTemp = _LIST_ENTRY(plineTemp, LW_PROCFS_NODE, PFSN_lineBrother);
        dir->dir_pos++;
        lib_strlcpy(dir->dir_dirent.d_name, 
                    p_pfsnTemp->PFSN_pcName, 
                    sizeof(dir->dir_dirent.d_name));                    /*  �����ļ���                  */
        dir->dir_dirent.d_shortname[0] = PX_EOS;
        dir->dir_dirent.d_type = IFTODT(p_pfsnTemp->PFSN_mode);         /*  ת��Ϊ d_type               */
    }
    __LW_PROCFS_UNLOCK();                                               /*  ���� procfs                 */

    return  (iError);
}
/*********************************************************************************************************
** ��������: __procFsIoctl
** ��������: procfs ioctl ����
** �䡡��  : p_pfsn             �ڵ���ƿ�
**           request,           ����
**           arg                �������
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __procFsIoctl (PLW_PROCFS_NODE  p_pfsn, 
                           INT              iRequest,
                           LONG             lArg)
{
    switch (iRequest) {
    
    /*
     *  FIOSEEK, FIOWHERE is 64 bit operate.
     */
    case FIOSEEK:                                                       /*  �ļ��ض�λ                  */
        if (p_pfsn && lArg) {
            __LW_PROCFS_LOCK();                                         /*  �� procfs                   */
            p_pfsn->PFSN_pfsnmMessage.PFSNM_oftPtr = *(off_t *)lArg;
            __LW_PROCFS_UNLOCK();                                       /*  ���� procfs                 */
        }
        return  (ERROR_NONE);
    
    case FIOWHERE:                                                      /*  ����ļ���ǰ��дָ��        */
        if (lArg) {
            *(off_t *)lArg = p_pfsn->PFSN_pfsnmMessage.PFSNM_oftPtr;
        }
        return  (ERROR_NONE);
    
    case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
        return  (__procFsStatGet(p_pfsn, (struct stat *)lArg));
        
    case FIOFSTATFSGET:                                                 /*  ����ļ�ϵͳ״̬            */
        return  (__procFsStatfsGet(p_pfsn, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  ��ȡһ��Ŀ¼��Ϣ            */
        return  (__procFsReadDir(p_pfsn, (DIR *)lArg));
    
    case FIOSYNC:                                                       /*  ���ļ������д              */
    case FIODATASYNC:
    case FIOFLUSH:
        return  (ERROR_NONE);
        
    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "PROC FileSystem";
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
        
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __procFsReadlink
** ��������: procfs read link ����
** �䡡��  : pdevhdr                       procfs �豸
**           pcName                        ����ԭʼ�ļ���
**           pcLinkDst                     ����Ŀ���ļ���
**           stMaxSize                     �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsReadlink (PLW_DEV_HDR    pdevhdr,
                                  PCHAR          pcName,
                                  PCHAR          pcLinkDst,
                                  size_t         stMaxSize)
{
    PLW_PROCFS_NODE  p_pfsn;
    size_t           stLen;
    
    if (pcName == LW_NULL) {                                            /*  ���ļ���                    */
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    __LW_PROCFS_LOCK();                                                 /*  �� procfs                   */
    p_pfsn = __procFsFindNode(pcName, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    if (p_pfsn == LW_NULL) {                                            /*  Ϊ�ҵ��ڵ�                  */
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (p_pfsn->PFSN_bReqRemove) {
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (!S_ISLNK(p_pfsn->PFSN_mode)) {
        __LW_PROCFS_UNLOCK();                                           /*  ���� procfs                 */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    stLen = lib_strlen((PCHAR)p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer);
    lib_strncpy(pcLinkDst, (PCHAR)p_pfsn->PFSN_pfsnmMessage.PFSNM_pvBuffer, stMaxSize);
    if (stLen > stMaxSize) {                                            /*  ������������                */
        stLen = stMaxSize;                                              /*  ������Ч�ֽ���              */
    }
    
    __LW_PROCFS_UNLOCK();                                               /*  ���� procfs                 */
    
    return  ((ssize_t)stLen);
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
