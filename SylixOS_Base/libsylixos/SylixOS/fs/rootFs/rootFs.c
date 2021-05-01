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
** ��   ��   ��: rootFs.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 08 �� 26 ��
**
** ��        ��: ��Ŀ¼�ļ�ϵͳ.

** BUG:
2009.09.15  ����ʱ������.
2009.10.22  read write ����ֵΪ ssize_t.
2009.12.14  ����ּ���Ŀ¼�������.
2009.12.15  ���� remove ɾ�������ļ��Ĵ���.
2010.09.10  ֧�� d_type.
2011.05.16  �����������Ӵ���Ŀ¼����.
2011.06.11  ���� __rootFsTimeSet() �����һ��ͬ�� rtc ʱ��ʱ, ���� root fs ʱ��.
2011.07.11  �������ļ�ϵͳʱ, ���ʱ��û�����ù�, �����¼��ǰ����ʱ��.
2011.08.07  ����� lstat() ��֧��.
2011.08.11  ѹ��Ŀ¼�������ļ�ϵͳ���, ����ϵͳ�Ѵ�����.
2011.12.13  ʹ�� LW_CFG_SPIPE_DEFAULT_SIZE ��Ϊ������ fifo ��С.
2012.03.10  __rootFsReadDir ��Ŀ¼��ȡ��Ϻ�, ���� errno == ENOENT.
2012.06.29  __rootFsLStatGet Ϊ�����ļ�ʱ, ��С����ȷ.
2012.09.25  ����� AF_UNIX ���� sock �ļ���֧��. mknod
2012.12.27  ����һЩ����.
2013.01.22  ���ļ�ϵͳ���� chmod ֧��.
2013.03.16  ����� reg �ļ���֧��.
2013.03.30  �������ļ�ϵͳ�ڷ��������ڴ����ڵ�Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "rootFsLib.h"
#include "rootFs.h"
/*********************************************************************************************************
  rootfs �ļ����ƿ鼰ʱ��� (����Ĭ�����е��豸ʹ�� root fs ����ʱ��ʱ����Ϊ�Լ���ʱ���׼)
*********************************************************************************************************/
#if LW_CFG_PATH_VXWORKS == 0
       LW_DEV_HDR       _G_devhdrRoot;                                  /*  rootfs �豸                 */
#else
static LW_DEV_HDR       _G_devhdrRoot;                                  /*  rootfs �豸                 */
#endif                                                                  /*  LW_CFG_PATH_VXWORKS         */
static time_t           _G_timeRootFs = 0;
/*********************************************************************************************************
  rootfs ���豸��
*********************************************************************************************************/
static INT              _G_iRootDrvNum = PX_ERROR;
/*********************************************************************************************************
  ������������
*********************************************************************************************************/
static LONG     __rootFsOpen(PLW_DEV_HDR     pdevhdr,
                             PCHAR           pcName,
                             INT             iFlags,
                             INT             iMode);
static INT      __rootFsRemove(PLW_DEV_HDR     pdevhdr,
                               PCHAR           pcName);
static INT      __rootFsClose(LW_DEV_HDR    *pdevhdr);
static ssize_t  __rootFsRead(LW_DEV_HDR     *pdevhdr,
                             PCHAR           pcBuffer, 
                             size_t          stMaxBytes);
static ssize_t  __rootFsWrite(LW_DEV_HDR *pdevhdr,
                              PCHAR       pcBuffer, 
                              size_t      stNBytes);
static INT      __rootFsLStatGet(LW_DEV_HDR *pdevhdr, 
                                 PCHAR       pcName, 
                                 struct stat *pstat);
static INT      __rootFsIoctl(LW_DEV_HDR *pdevhdr,
                              INT         iRequest,
                              LONG        lArg);
static INT      __rootFsSymlink(PLW_DEV_HDR     pdevhdr,
                                PCHAR           pcName,
                                CPCHAR          pcLinkDst);
static ssize_t  __rootFsReadlink(PLW_DEV_HDR    pdevhdr,
                                 PCHAR          pcName,
                                 PCHAR          pcLinkDst,
                                 size_t         stMaxSize);
/*********************************************************************************************************
** ��������: API_RootFsDrvInstall
** ��������: ��װ rootfs �ļ�ϵͳ��������
** �䡡��  : 
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_RootFsDrvInstall (VOID)
{
    struct file_operations     fileop;

    if (_G_iRootDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __rootFsOpen;
    fileop.fo_release  = __rootFsRemove;
    fileop.fo_open     = __rootFsOpen;
    fileop.fo_close    = __rootFsClose;
    fileop.fo_read     = __rootFsRead;
    fileop.fo_write    = __rootFsWrite;
    fileop.fo_lstat    = __rootFsLStatGet;
    fileop.fo_ioctl    = __rootFsIoctl;

    /*
     *  ����ϵͳʹ���µ� SylixOS �ּ�Ŀ¼����ʱ, ���ṩ�����ļ�֧��
     */
    fileop.fo_symlink  = __rootFsSymlink;
    fileop.fo_readlink = __rootFsReadlink;
    
    _G_iRootDrvNum = iosDrvInstallEx(&fileop);
     
    DRIVER_LICENSE(_G_iRootDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iRootDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iRootDrvNum, "rootfs driver.");
    
    return  ((_G_iRootDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: API_RootFsDevCreate
** ��������: ���� root fs.
** �䡡��  : NONE
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_RootFsDevCreate (VOID)
{
    static BOOL     bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return  (ERROR_NONE);
    }
    
    if (iosDevAddEx(&_G_devhdrRoot, PX_STR_ROOT, _G_iRootDrvNum, DT_DIR) != ERROR_NONE) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    bIsInit = LW_TRUE;
    
    if (_G_timeRootFs == 0) {                                           /*  û�����ù�                  */
        lib_time(&_G_timeRootFs);                                       /*  �� UTC ʱ����Ϊʱ���׼     */
    }
    
#if LW_CFG_PATH_VXWORKS == 0
    _G_rfsrRoot.RFSR_time      = _G_timeRootFs;
    _G_rfsrRoot.RFSR_stMemUsed = 0;
    _G_rfsrRoot.RFSR_ulFiles   = 0;
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_RootFsTime
** ��������: ��� root fs ʱ���׼
** �䡡��  : time      ʱ��ָ��
** �䡡��  : ʱ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
time_t  API_RootFsTime (time_t  *time)
{
    if (time) {
        *time = _G_timeRootFs;
    }
    
    return  (_G_timeRootFs);
}
/*********************************************************************************************************
** ��������: __rootFsTimeSet
** ��������: ���� root fs ʱ���׼ (ֻ������ϵͳ����ʱ����)
** �䡡��  : time      ʱ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
VOID  __rootFsTimeSet (time_t  *time)
{
    if (time) {
        _G_timeRootFs = *time;
    }
}
/*********************************************************************************************************
** ��������: __rootFsFixName
** ��������: root fs �����ļ���
** �䡡��  : pcName           �ļ���
** �䡡��  : ������ĳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_PATH_VXWORKS == 0                                            /*  ��Ҫ�ṩ�ּ�Ŀ¼����        */

INT  __rootFsFixName (PCHAR  pcName)
{
    size_t  stLen;
    
    /*
     *  ���� iosDevFind() �����Ը��豸����Ĺ���, ������Ҫ������Ŀ¼����
     */
    stLen = lib_strnlen(pcName, (PATH_MAX - 1));
    if (stLen == 0) {                                                   /*  match �豸ʱ�˵��� /        */
        pcName[0] = PX_ROOT;
        pcName[1] = PX_EOS;
        stLen     = 1;
    
    } else if (pcName[0] != PX_ROOT) {
        /*
         *  SylixOS lib_memcpy() ���л�������ֹ���Ƶ�����.
         */
        lib_memcpy(&pcName[1], &pcName[0], stLen + 1);                  /*  �������һ���ֽ�            */
        pcName[0]  = PX_ROOT;                                           /*  ���������                  */
        stLen     += 1;
    }
    
    return  (stLen);
}

#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
/*********************************************************************************************************
** ��������: __rootFsOpen
** ��������: root fs open ����
** �䡡��  : pdevhdr          root fs �豸
**           pcName           �ļ���
**           iFlags           ��ʽ
**           iMode            ����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  __rootFsOpen (PLW_DEV_HDR     pdevhdr,
                           PCHAR           pcName,
                           INT             iFlags,
                           INT             iMode)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  ��Ҫ�ṩ�ּ�Ŀ¼����        */
    /*
     *  �ļ���չ����Ϊ�ڵ�ָ��
     */
    PLW_ROOTFS_NODE    prfsnFather;
    PLW_ROOTFS_NODE    prfsn;
    BOOL               bIsRoot;
    BOOL               bIsLast;
    PCHAR              pcTail = LW_NULL;
    INT                iError;
    

    if (pcName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    __rootFsFixName(pcName);                                            /*  �����ļ���                  */
    
__re_find:
    __LW_ROOTFS_LOCK();                                                 /*  ���� rootfs                 */
    prfsn = __rootFsFindNode(pcName, &prfsnFather, &bIsRoot, &bIsLast, &pcTail);
                                                                        /*  ��ѯ�豸                    */
    if (prfsn) {
        prfsn->RFSN_iOpenNum++;
        if (prfsn->RFSN_iNodeType != LW_ROOTFS_NODE_TYPE_LNK) {         /*  ���������ļ�                */
            if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {              /*  ���������ļ�                */
                prfsn->RFSN_iOpenNum--;
                __LW_ROOTFS_UNLOCK();                                   /*  ���� rootfs                 */
                _ErrorHandle(EEXIST);                                   /*  �Ѿ������ļ�                */
                return  (PX_ERROR);
            
            } else if ((iFlags & O_DIRECTORY) && 
                       (prfsn->RFSN_iNodeType != LW_ROOTFS_NODE_TYPE_DIR)) {
                prfsn->RFSN_iOpenNum--;
                __LW_ROOTFS_UNLOCK();                                   /*  ���� rootfs                 */
                _ErrorHandle(ENOTDIR);                                  /*  ����Ŀ¼                    */
                return  (PX_ERROR);
            
            } else {
                __LW_ROOTFS_UNLOCK();                                   /*  ���� rootfs                 */
                LW_DEV_INC_USE_COUNT(&_G_devhdrRoot);                   /*  ���¼�����                  */
                return  ((LONG)prfsn);
            }
        }
    
    } else if ((iFlags & O_CREAT) && bIsLast) {                         /*  ��Ҫ�����ڵ�                */
        __LW_ROOTFS_UNLOCK();                                           /*  ���� rootfs                 */
        
#if LW_CFG_MAX_VOLUMES > 0
        if (__fsCheckFileName(pcName)) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
        
        if (S_ISDIR(iMode)) {
            iError = rootFsMakeDir(pcName, iMode);
        
        } else if (S_ISSOCK(iMode)) {
            iError = rootFsMakeSock(pcName, iMode);
        
        } else if (S_ISREG(iMode) || !(iMode & S_IFMT)) {
            iError = rootFsMakeReg(pcName, iMode);
        
#if LW_CFG_SPIPE_EN > 0
        } else if (S_ISFIFO(iMode)) {
            spipeDrv();
            iError = spipeDevCreate(pcName, LW_CFG_SPIPE_DEFAULT_SIZE);
#endif                                                                  /*  LW_CFG_SPIPE_EN > 0         */
        } else {
            _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                     /*  ��֧�ִ����������͵��ļ�    */
            return  (PX_ERROR);
        }
        
        if ((iError < 0) && (iFlags & O_EXCL)) {                        /*  ����ʧ��, �Ѿ�д�� errno    */
            return  (PX_ERROR);
        }
        
        iFlags &= ~O_CREAT;                                             /*  ���� O_CREAT O_EXCL �жϴ���*/
        
        goto    __re_find;
    }
    __LW_ROOTFS_UNLOCK();                                               /*  ���� rootfs                 */
    
    if (prfsn) {                                                        /*  �����ļ�����                */
        INT     iError;
        INT     iFollowLinkType;
        PCHAR   pcSymfile = pcTail - lib_strlen(prfsn->RFSN_rfsnv.RFSNV_pcName) - 1;
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
                                LW_NULL, pcPrefix, prfsn->RFSN_pcLink, pcTail);
        
        __LW_ROOTFS_LOCK();                                             /*  ���� rootfs                 */
        prfsn->RFSN_iOpenNum--;
        __LW_ROOTFS_UNLOCK();                                           /*  ���� rootfs                 */
        
        if (iError) {
            return  (PX_ERROR);                                         /*  �޷�����������Ŀ��Ŀ¼      */
        } else {
            return  (iFollowLinkType);
        }
    }
    
    if (bIsRoot) {
        LW_DEV_INC_USE_COUNT(&_G_devhdrRoot);                           /*  ���¼�����                  */
        return  ((LONG)LW_NULL);                                        /*  ��Ŀ¼                      */
    
    } else {
        _ErrorHandle(ENOENT);                                           /*  û���ҵ��ļ�                */
        return  (PX_ERROR);
    }
    
#else                                                                   /*  VxWorks ����Ŀ¼����        */
    /*
     *  �ļ���չ����Ϊ���豸ָ��
     */
    if (lib_strcmp(pcName, PX_STR_ROOT) && (pcName[0] != PX_EOS)) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if ((iFlags & O_CREAT) || (iFlags & O_TRUNC)) {                     /*  ���ܴ���                    */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        return  (PX_ERROR);
    }
    
    if ((iFlags & O_ACCMODE) != O_RDONLY) {                             /*  ������д                    */
        _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
        return  (PX_ERROR);
    }
    
    LW_DEV_INC_USE_COUNT(&_G_devhdrRoot);                               /*  ���¼�����                  */
        
    return  ((LONG)&_G_devhdrRoot);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
}
/*********************************************************************************************************
** ��������: __rootFsRemove
** ��������: root fs remove ����
** �䡡��  : pdevhdr
**           pcName           �ļ���
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rootFsRemove (PLW_DEV_HDR     pdevhdr,
                            PCHAR           pcName)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  ��Ҫ�ṩ�ּ�Ŀ¼����        */
    /*
     *  �����е� delete ����һ��Ҫ�ж� cFullFileName ����, ���Ϊ�����ļ�����Ҫ�������ж�
     *  cFullFileName ָ�����Ӷ�����ڲ��ļ���Ŀ¼ʱ, Ӧ�÷��� FOLLOW_LINK_TAIL ����ɾ��
     *  ������Ŀ��(��ʵĿ��)�е��ļ���Ŀ¼, �� cFullFileName ָ����������ļ�����, ��ô
     *  Ӧ��ֱ��ɾ�������ļ�����, ��Ӧ�÷��� FOLLOW_LINK_TAIL ɾ������Ŀ��! �м�! �м�!
     */
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
        
    } else {
        PLW_ROOTFS_NODE    prfsn;
        BOOL               bIsRoot;
        PCHAR              pcTail = LW_NULL;
        
        __rootFsFixName(pcName);                                        /*  �����ļ���                  */
        
        __LW_ROOTFS_LOCK();                                             /*  ���� rootfs                 */
        prfsn = __rootFsFindNode(pcName, LW_NULL, &bIsRoot, LW_NULL, &pcTail);
                                                                        /*  ��ѯ�豸                    */
        if (prfsn) {
            if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_DEV) {     /*  ����������ж���豸          */
                __LW_ROOTFS_UNLOCK();                                   /*  ���� rootfs                 */
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            
            } else if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_LNK) {
                size_t  stLenTail = 0;
                
                if (pcTail) {
                    stLenTail = lib_strlen(pcTail);                     /*  ȷ�� tail ����              */
                }
                /*
                 *  û��β��, ������Ҫɾ�������ļ�����, ����ɾ�������ļ�����.
                 *  ��ôֱ��ʹ�� API_RootFsRemoveNode ɾ������.
                 */
                if (stLenTail) {
                    PCHAR   pcSymfile = pcTail - lib_strlen(prfsn->RFSN_rfsnv.RFSNV_pcName) - 1;
                    PCHAR   pcPrefix;
                    
                    if (*pcSymfile != PX_DIVIDER) {
                        pcSymfile--;
                    }
                    if (pcSymfile == pcName) {
                        pcPrefix = LW_NULL;                             /*  û��ǰ׺                    */
                    } else {
                        pcPrefix = pcName;
                        *pcSymfile = PX_EOS;
                    }
                    
                    if (_PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                       LW_NULL, pcPrefix, prfsn->RFSN_pcLink, pcTail) < ERROR_NONE) {
                        __LW_ROOTFS_UNLOCK();                           /*  ���� rootfs                 */
                        return  (PX_ERROR);                             /*  �޷�����������Ŀ��Ŀ¼      */
                    
                    } else {
                        __LW_ROOTFS_UNLOCK();                           /*  ���� rootfs                 */
                        return  (FOLLOW_LINK_TAIL);                     /*  �����ļ��ڲ��ļ�            */
                    }
                }
            }
        }
        __LW_ROOTFS_UNLOCK();                                           /*  ���� rootfs                 */
        
        return  (API_RootFsRemoveNode(pcName));
    }
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rootFsClose
** ��������: root fs close ����
** �䡡��  : pdevhdr
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rootFsClose (LW_DEV_HDR     *pdevhdr)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  ��Ҫ�ṩ�ּ�Ŀ¼����        */
    /*
     *  ����Ϊ�ڵ�ָ��
     */
    PLW_ROOTFS_NODE    prfsn = (PLW_ROOTFS_NODE)pdevhdr;
    
    __LW_ROOTFS_LOCK();                                                 /*  ���� rootfs                 */
    if (prfsn) {
        prfsn->RFSN_iOpenNum--;
    }
    __LW_ROOTFS_UNLOCK();                                               /*  ���� rootfs                 */
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */

    LW_DEV_DEC_USE_COUNT(&_G_devhdrRoot);                               /*  ���¼�����                  */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rootFsRead
** ��������: root fs read ����
** �䡡��  : pdevhdr
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __rootFsRead (LW_DEV_HDR *pdevhdr,
                              PCHAR       pcBuffer, 
                              size_t      stMaxBytes)
{
    _ErrorHandle(ENOSYS);                                               /*  ���ļ���֧��                */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __rootFsWrite
** ��������: root fs write ����
** �䡡��  : pdevhdr
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __rootFsWrite (LW_DEV_HDR *pdevhdr,
                               PCHAR       pcBuffer, 
                               size_t      stNBytes)
{
    _ErrorHandle(ENOSYS);                                               /*  ���ļ���֧��                */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __rootFsStatGet
** ��������: root fs ����ļ�״̬������
** �䡡��  : pdevhdr
**           pstat               stat �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rootFsStatGet (LW_DEV_HDR *pdevhdr, struct stat *pstat)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  ��Ҫ�ṩ�ּ�Ŀ¼����        */
    /*
     *  ����Ϊ�ڵ�ָ��
     */
    PLW_ROOTFS_NODE    prfsn = (PLW_ROOTFS_NODE)pdevhdr;
    
    if (pstat == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (prfsn) {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_devhdrRoot);
        pstat->st_ino     = (ino_t)prfsn;
        pstat->st_mode    = prfsn->RFSN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = prfsn->RFSN_uid;
        pstat->st_gid     = prfsn->RFSN_gid;
        pstat->st_rdev    = 1;
        if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_LNK) {
            pstat->st_size = lib_strlen(prfsn->RFSN_pcLink);
        } else {
            pstat->st_size = 0;
        }
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        if (prfsn->RFSN_time == (time_t)(PX_ERROR)) {
            pstat->st_atime = API_RootFsTime(LW_NULL);
            pstat->st_mtime = API_RootFsTime(LW_NULL);
            pstat->st_ctime = API_RootFsTime(LW_NULL);
        } else {
            pstat->st_atime = prfsn->RFSN_time;                         /*  �ڵ㴴����׼ʱ��            */
            pstat->st_mtime = prfsn->RFSN_time;
            pstat->st_ctime = prfsn->RFSN_time;
        }
    } else {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_devhdrRoot);
        pstat->st_ino     = (ino_t)0;                                   /*  ��Ŀ¼                      */
        pstat->st_mode    = (DEFAULT_FILE_PERM | S_IFDIR);              /*  Ĭ������                    */
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = API_RootFsTime(LW_NULL);                    /*  Ĭ��ʹ�� root fs ��׼ʱ��   */
        pstat->st_mtime   = API_RootFsTime(LW_NULL);
        pstat->st_ctime   = API_RootFsTime(LW_NULL);
    }

#else
    if (pstat == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_devhdrRoot);
    pstat->st_ino     = (ino_t)0;                                       /*  ��Ŀ¼                      */
    pstat->st_mode    = (0444 | S_IFDIR);                               /*  Ĭ������                    */
    pstat->st_nlink   = 1;
    pstat->st_uid     = 0;
    pstat->st_gid     = 0;
    pstat->st_rdev    = 1;
    pstat->st_size    = 0;
    pstat->st_blksize = 0;
    pstat->st_blocks  = 0;
    pstat->st_atime   = API_RootFsTime(LW_NULL);                        /*  Ĭ��ʹ�� root fs ��׼ʱ��   */
    pstat->st_mtime   = API_RootFsTime(LW_NULL);
    pstat->st_ctime   = API_RootFsTime(LW_NULL);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rootFsLStatGet
** ��������: root fs ����ļ�״̬������ (����������ļ����ȡ�����ļ�������)
** �䡡��  : pdevhdr             �豸ͷ
**           pstat               stat �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rootFsLStatGet (LW_DEV_HDR *pdevhdr, PCHAR  pcName, struct stat *pstat)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  ��Ҫ�ṩ�ּ�Ŀ¼����        */
    /*
     *  �ļ���չ����Ϊ�ڵ�ָ��
     */
    PLW_ROOTFS_NODE    prfsnFather;
    PLW_ROOTFS_NODE    prfsn;
    BOOL               bIsRoot;
    PCHAR              pcTail = LW_NULL;
    

    if (pcName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    __rootFsFixName(pcName);                                            /*  �����ļ���                  */
    
    __LW_ROOTFS_LOCK();                                                 /*  ���� rootfs                 */
    prfsn = __rootFsFindNode(pcName, &prfsnFather, &bIsRoot, LW_NULL, &pcTail);
                                                                        /*  ��ѯ�豸                    */
    if (prfsn) {                                                        /*  һ������ FOLLOW_LINK_TAIL   */
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_devhdrRoot);
        pstat->st_ino     = (ino_t)prfsn;
        pstat->st_mode    = prfsn->RFSN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = prfsn->RFSN_uid;
        pstat->st_gid     = prfsn->RFSN_gid;
        pstat->st_rdev    = 1;
        if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_LNK) {
            pstat->st_size = lib_strlen(prfsn->RFSN_pcLink);
        } else {
            pstat->st_size = 0;
        }
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        if (prfsn->RFSN_time == (time_t)(PX_ERROR)) {
            pstat->st_atime = API_RootFsTime(LW_NULL);
            pstat->st_mtime = API_RootFsTime(LW_NULL);
            pstat->st_ctime = API_RootFsTime(LW_NULL);
        } else {
            pstat->st_atime = prfsn->RFSN_time;                         /*  �ڵ㴴����׼ʱ��            */
            pstat->st_mtime = prfsn->RFSN_time;
            pstat->st_ctime = prfsn->RFSN_time;
        }
        __LW_ROOTFS_UNLOCK();                                           /*  ���� rootfs                 */
    
    } else {
        __LW_ROOTFS_UNLOCK();                                           /*  ���� rootfs                 */
        
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
#else
    if (pstat == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_devhdrRoot);
    pstat->st_ino     = (dev_t)0;                                       /*  ��Ŀ¼                      */
    pstat->st_mode    = (0666 | S_IFDIR);                               /*  Ĭ������                    */
    pstat->st_nlink   = 1;
    pstat->st_uid     = 0;
    pstat->st_gid     = 0;
    pstat->st_rdev    = 1;
    pstat->st_size    = 0;
    pstat->st_blksize = 0;
    pstat->st_blocks  = 0;
    pstat->st_atime   = API_RootFsTime(LW_NULL);                        /*  Ĭ��ʹ�� root fs ��׼ʱ��   */
    pstat->st_mtime   = API_RootFsTime(LW_NULL);
    pstat->st_ctime   = API_RootFsTime(LW_NULL);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rootFsStatfsGet
** ��������: root fs ����ļ�ϵͳ״̬������
** �䡡��  : pdevhdr
**           pstatfs             statfs �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rootFsStatfsGet (LW_DEV_HDR *pdevhdr, struct statfs *pstatfs)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  ��Ҫ�ṩ�ּ�Ŀ¼����        */
    if (pstatfs) {
        pstatfs->f_type   = 0;
        pstatfs->f_bsize  = (long)_G_rfsrRoot.RFSR_stMemUsed;           /*  ���ļ�ϵͳ�ڴ�ʹ����        */
        pstatfs->f_blocks = 1;
        pstatfs->f_bfree  = 0;
        pstatfs->f_bavail = 1;
        
        pstatfs->f_files  = (long)_G_rfsrRoot.RFSR_ulFiles;             /*  �ļ�����                    */
        pstatfs->f_ffree  = 0;
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        pstatfs->f_fsid.val[0] = (int32_t)((addr_t)pdevhdr >> 32);
        pstatfs->f_fsid.val[1] = (int32_t)((addr_t)pdevhdr & 0xffffffff);
#else
        pstatfs->f_fsid.val[0] = (int32_t)pdevhdr;
        pstatfs->f_fsid.val[1] = 0;
#endif
        
        pstatfs->f_flag    = 0;
        pstatfs->f_namelen = PATH_MAX;
    }
#else
    REGISTER PLW_DEV_HDR            pdevhdrTemp;
             PLW_LIST_LINE          plineTemp;
             long                   lSize  = 0;
             INT                    iCount = 0;

    _IosLock();
    for (plineTemp  = _S_plineDevHdrHeader; 
         plineTemp != LW_NULL; 
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pdevhdrTemp = _LIST_ENTRY(plineTemp, LW_DEV_HDR, DEVHDR_lineManage);
        
        lSize += (long)lib_strlen(pdevhdr->DEVHDR_pcName);
        iCount++;
    }
    _IosUnlock();
    
    lSize += (long)(iCount * sizeof(LW_DEV_HDR));
    
    if (pstatfs) {
        pstatfs->f_type   = 0;
        pstatfs->f_bsize  = lSize;
        pstatfs->f_blocks = 1;
        pstatfs->f_bfree  = 0;
        pstatfs->f_bavail = 1;
        
        pstatfs->f_files  = 0;
        pstatfs->f_ffree  = 0;
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        pstatfs->f_fsid.val[0] = (int32_t)((addr_t)pdevhdr >> 32);
        pstatfs->f_fsid.val[1] = (int32_t)((addr_t)pdevhdr & 0xffffffff);
#else
        pstatfs->f_fsid.val[0] = (int32_t)pdevhdr;
        pstatfs->f_fsid.val[1] = 0;
#endif
        
        pstatfs->f_flag    = 0;
        pstatfs->f_namelen = PATH_MAX;
    }
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rootFsReadDir
** ��������: root fs ���ָ��Ŀ¼��Ϣ
** �䡡��  : pdevhdr
**           dir                 Ŀ¼�ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rootFsReadDir (LW_DEV_HDR *pdevhdr, DIR  *dir)
{
             INT                i;
             INT                iError = ERROR_NONE;
    REGISTER LONG               iStart;
             PLW_LIST_LINE      plineTemp;

#if LW_CFG_PATH_VXWORKS == 0                                            /*  ��Ҫ�ṩ�ּ�Ŀ¼����        */
             PLW_LIST_LINE      plineHeader;
             PLW_ROOTFS_NODE    prfsn = (PLW_ROOTFS_NODE)pdevhdr;
             PLW_ROOTFS_NODE    prfsnTemp;
    
    if (dir == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __LW_ROOTFS_LOCK();                                                 /*  ���� rootfs                 */
    if (prfsn == LW_NULL) {
        plineHeader = _G_rfsrRoot.RFSR_plineSon;
    
    } else {
        if (prfsn->RFSN_iNodeType != LW_ROOTFS_NODE_TYPE_DIR) {
            __LW_ROOTFS_UNLOCK();                                       /*  ���� rootfs                 */
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
        plineHeader = prfsn->RFSN_plineSon;
    }
    
    iStart = dir->dir_pos;

    for ((plineTemp  = plineHeader), (i = 0); 
         (plineTemp != LW_NULL) && (i < iStart); 
         (plineTemp  = _list_line_get_next(plineTemp)), (i++));         /*  ����                        */
    
    if (plineTemp == LW_NULL) {
        _ErrorHandle(ENOENT);
        iError = PX_ERROR;                                              /*  û�ж���Ľڵ�              */
    
    } else {
        prfsnTemp = _LIST_ENTRY(plineTemp, LW_ROOTFS_NODE, RFSN_lineBrother);
        dir->dir_pos++;
        /*
         *  �����ļ���
         */
        if (prfsnTemp->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_DEV) {     /*  �豸�ļ�                    */
            /*
             *  ע��, �豸��������Ŀ¼����
             */
            PCHAR       pcDevName = lib_rindex(prfsnTemp->RFSN_rfsnv.RFSNV_pdevhdr->DEVHDR_pcName, 
                                               PX_DIVIDER);
            if (pcDevName == LW_NULL) {
                pcDevName =  prfsnTemp->RFSN_rfsnv.RFSNV_pdevhdr->DEVHDR_pcName;
            } else {
                pcDevName++;                                            /*  ���� /                      */
            }
            
            lib_strlcpy(dir->dir_dirent.d_name, 
                        pcDevName, 
                        sizeof(dir->dir_dirent.d_name));                /*  �����ļ���                  */
            
            dir->dir_dirent.d_type = 
                prfsnTemp->RFSN_rfsnv.RFSNV_pdevhdr->DEVHDR_ucType;     /*  �豸 d_type                 */
        
        } else {
            /*
             *  Ŀ¼�ļ��������ļ�.
             */
            lib_strlcpy(dir->dir_dirent.d_name, 
                        prfsnTemp->RFSN_rfsnv.RFSNV_pcName, 
                        sizeof(dir->dir_dirent.d_name));                /*  �����ļ���                  */
        
            dir->dir_dirent.d_type = IFTODT(prfsnTemp->RFSN_mode);      /*  rootfs �ڵ� type            */
        }
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }
    __LW_ROOTFS_UNLOCK();                                               /*  ���� rootfs                 */
    
#else
    REGISTER PLW_DEV_HDR        pdevhdrTemp;
             
    _IosLock();
    for ((plineTemp  = _S_plineDevHdrHeader), (i = 0); 
         (plineTemp != LW_NULL) && (i < iStart); 
         (plineTemp  = _list_line_get_next(plineTemp)), (i++));         /*  ����                        */

    if (plineTemp == LW_NULL) {
        iError = PX_ERROR;                                              /*  û�ж���Ľڵ�              */
    
    } else {
        pdevhdrTemp = _LIST_ENTRY(plineTemp, LW_DEV_HDR, DEVHDR_lineManage);
        if (lib_strcmp(pdevhdrTemp->DEVHDR_pcName, PX_STR_ROOT) == 0) { /*  ���Ը�Ŀ¼�豸              */
            plineTemp  = _list_line_get_next(plineTemp);                /*  �����һ���ڵ�              */
            dir->dir_pos++;
            if (plineTemp == LW_NULL) {
                _ErrorHandle(ENOENT);
                iError = PX_ERROR;                                      /*  û�ж���Ľڵ�              */
                goto    __scan_over;
            } else {
                pdevhdrTemp = _LIST_ENTRY(plineTemp, LW_DEV_HDR, DEVHDR_lineManage);
            }
        }
        
        dir->dir_pos++;
        lib_strlcpy(dir->dir_dirent.d_name, 
                    &pdevhdrTemp->DEVHDR_pcName[1], 
                    sizeof(dir->dir_dirent.d_name));                    /*  �����ļ���                  */
        dir->dir_dirent.d_type = pdevhdrTemp->DEVHDR_ucType;
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }
__scan_over:
    _IosUnlock();
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __rootFsChmod
** ��������: root fs chmod ����
** �䡡��  : pdevhdr
**           iMode               mode_t
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rootFsChmod (LW_DEV_HDR *pdevhdr, INT  iMode)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  ��Ҫ�ṩ�ּ�Ŀ¼����        */
    PLW_ROOTFS_NODE    prfsn = (PLW_ROOTFS_NODE)pdevhdr;
    
    if (iMode & S_IFMT) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iMode |= S_IRUSR;
    
    prfsn->RFSN_mode = iMode | (prfsn->RFSN_mode & S_IFMT);
    
    return  (ERROR_NONE);
    
#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
}
/*********************************************************************************************************
** ��������: __rootFsIoctl
** ��������: root fs ioctl ����
** �䡡��  : pdevhdr            
**           request,           ����
**           arg                �������
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rootFsIoctl (LW_DEV_HDR *pdevhdr,
                           INT        iRequest,
                           LONG       lArg)
{
    switch (iRequest) {
    
    case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
        return  (__rootFsStatGet(pdevhdr, (struct stat *)lArg));
        
    case FIOFSTATFSGET:                                                 /*  ����ļ�ϵͳ״̬            */
        return  (__rootFsStatfsGet(pdevhdr, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  ��ȡһ��Ŀ¼��Ϣ            */
        return  (__rootFsReadDir(pdevhdr, (DIR *)lArg));
    
    case FIOSYNC:                                                       /*  ���ļ������д              */
    case FIODATASYNC:
    case FIOFLUSH:
        return  (ERROR_NONE);
        
    case FIOCHMOD:
        return  (__rootFsChmod(pdevhdr, (INT)lArg));                    /*  �ı��ļ�����Ȩ��            */
        
    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "ROOT FileSystem";
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
        break;
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __rootFsSymlink
** ��������: root fs symlink ����
** �䡡��  : pdevhdr            
**           pcName             �����������ļ�
**           pcLinkDst          ����Ŀ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rootFsSymlink (PLW_DEV_HDR     pdevhdr,
                             PCHAR           pcName,
                             CPCHAR          pcLinkDst)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  ��Ҫ�ṩ�ּ�Ŀ¼����        */
    if ((pcName == LW_NULL) || (pcLinkDst == LW_NULL)) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
#if LW_CFG_MAX_VOLUMES > 0
    if (__fsCheckFileName(pcName)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
    
    __rootFsFixName(pcName);                                            /*  �����ļ���                  */
    
    return  (API_RootFsMakeNode(pcName, 
                                LW_ROOTFS_NODE_TYPE_LNK, 
                                LW_ROOTFS_NODE_OPT_NONE, 
                                DEFAULT_SYMLINK_PERM,
                                (PVOID)(pcLinkDst)));
#else
    _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
}
/*********************************************************************************************************
** ��������: __rootFsReadlink
** ��������: root fs read link ����
** �䡡��  : pdevhdr                       �豸ͷ
**           pcName                        ����ԭʼ�ļ���
**           pcLinkDst                     ����Ŀ���ļ���
**           stMaxSize                     �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __rootFsReadlink (PLW_DEV_HDR    pdevhdr,
                                  PCHAR          pcName,
                                  PCHAR          pcLinkDst,
                                  size_t         stMaxSize)
{
#if LW_CFG_PATH_VXWORKS == 0                                            /*  ��Ҫ�ṩ�ּ�Ŀ¼����        */
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    __rootFsFixName(pcName);                                            /*  �����ļ���                  */
    
    return  (__rootFsReadNode(pcName, pcLinkDst, stMaxSize));
    
#else
    _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_PATH_VXWORKS == 0    */
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
