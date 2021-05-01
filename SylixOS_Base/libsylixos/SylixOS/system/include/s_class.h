/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: s_class.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 13 ��
**
** ��        ��: ����ϵͳϵͳ���ؼ����Ͷ����ļ���

** BUG
2007.04.08  ɾ���ü�����ƣ����͵Ķ��壬�Ͳü��޹�
2008.03.23  ������ 2008 ̨������쵼��ѡ�ٺ�ĵ�һ��, ��ϣ��������ƽͳһ.
            �Ľ� I/O ϵͳ, ���� BSD �� Linux ϵͳ����ع���.
2009.02.13  ���쿪ʼ�������Ľ� I/O ϵͳ, �޸�Ϊ���� UNIX/BSD I/O ����ģʽ, ����ʹ�������㷨, �ǳ��� API
            ����ʱ�临�Ӷ� O(1) ʱ�临�Ӷ�.
2009.08.31  �����Ӧ���豸�Ĺ��Ĺ���ģ��.
2009.10.02  �豸ͷ�򿪴���ʹ�� atomic_t ��.
2009.11.21  ���� io ��������.
2009.12.14  ����������ӿ��м��� symlink ����. ע��: ��������ֻ�в����ļ�ϵͳ֧��(���ֹ���)! 
2010.09.10  file_operations ���� fstat ��������.
2011.07.28  file_operations ���� mmap ��������.
2011.08.07  file_operations ���� lstat ��������.
2012.10.19  LW_FD_NODE �������ü��� FDNODE_ulRef, ���� mmap() ʱ��������. munmap() ʱ���ټ���, ������Ϊ 0
            ʱ�ر��ļ�.
2012.11.21  �ļ��ṹ�м����� unlink ��־, ��ʼ֧���ļ�û�йر�ʱ�� unlink ����.
2012.12.21  ����ȵĸ����ļ���������
            ����һ���µĻ���: FDENTRY_bRemoveReq ��ʾ�����ʱ�ļ��ṹ����æ, �������Ƴ�ɾ������һ���ڵ�,
            ����µĻ����±����ļ��⹹��ʱ, �������� IO ϵͳ.
2013.01.09  LW_IO_ENV �м��� umask ����.
2013.05.27  fd entry ����ȥ�����������������ʵ�ļ���.
2014.07.19  �����µĵ�Դ�������.
*********************************************************************************************************/

#ifndef __S_CLASS_H
#define __S_CLASS_H

/*********************************************************************************************************
  IO ����
*********************************************************************************************************/

typedef struct {
    mode_t               IOE_modeUMask;                                 /*  umask                       */
    CHAR                 IOE_cDefPath[MAX_FILENAME_LENGTH];             /*  Ĭ�ϵ�ǰ·��                */
} LW_IO_ENV;
typedef LW_IO_ENV       *PLW_IO_ENV;

/*********************************************************************************************************
  IO �û���Ϣ
*********************************************************************************************************/

typedef struct {
    uid_t                IOU_uid;
    gid_t                IOU_gid;
} LW_IO_USR;
typedef LW_IO_USR       *PLW_IO_USR;

/*********************************************************************************************************
  ��Դ����
  
  SylixOS ��Դ�����Ϊ���󲿷�:
  
  1: CPU ���Ĺ���
  2: ���蹦�Ĺ���
  
  CPU ���ķ�Ϊ�����ܼ�: 1: ��������   2: ʡ��ģʽ (PowerSaving) 3: ����ģʽ(Sleep)
  
  ��������: CPU ����ִ��ָ��
  ʡ��ģʽ: ���о��е�Դ�����ܵ��豸���� PowerSaving ģʽ, ͬʱ CPU ����, ��� CPU ������һ�� CPU ����.
  ����ģʽ: ϵͳ����, ���о��е�Դ�����ܵ��豸���� Suspend ģʽ, ϵͳ��Ҫͨ��ָ���¼�����. 
            ����ģʽϵͳ���Ӹ�λ�������ָ�, ��Ҫ bootloader/BIOS �������.

  
  ���蹦�Ĺ����Ϊ����״̬: 1: �������� 2: �豸�ر� 3: ʡ��ģʽ 4: �򿪵��豸��ʱ�䲻ʹ��
  
  ��������: �豸����, ����������� pmDevOn() �����Դ������������ͨ�豸��Դ��ʱ��, ��ʼ����.
  �豸�ر�: �豸���ر�, ����������� pmDevOff() �����Դ�����������Ͽ��豸��Դ��ʱ��, ֹͣ����.
  ʡ��ģʽ: ϵͳ����ʡ��ģʽ, ������ܺ��豸����ʡ��ģʽ, PMDF_pfuncPowerSavingEnter ���ᱻ����.
  
  �򿪵��豸��ʱ�䲻ʹ��: �豸���Ĺ���Ԫ���п��Ź�������, һ������ʱ�䳬������, ϵͳ���Զ����� 
                          PMDF_pfuncIdleEnter �����豸��Ϊ idle ״̬.
                          Ӧ�ó������ͨ�� FIOSETWATCHDOG ������ idle ʱ�䲢����˹���.
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0

struct lw_pma_funcs;
struct lw_pmd_funcs;

typedef struct {
    LW_LIST_LINE         PMA_lineManage;                                /*  ��������                    */
    UINT                 PMA_uiMaxChan;                                 /*  ��Դ����ͨ������            */
    struct lw_pma_funcs *PMA_pmafunc;                                   /*  ��Դ������������������      */
    PVOID                PMA_pvReserve[8];
    CHAR                 PMA_cName[1];                                  /*  ��Դ��������������          */
} LW_PM_ADAPTER;
typedef LW_PM_ADAPTER   *PLW_PM_ADAPTER;

typedef struct {
    LW_LIST_LINE         PMD_lineManage;                                /*  ��������                    */
    PLW_PM_ADAPTER       PMD_pmadapter;                                 /*  ��Դ����������              */
    UINT                 PMD_uiChannel;                                 /*  ��Ӧ��Դ����������ͨ����    */
    PVOID                PMD_pvReserve[8];

    PCHAR                PMD_pcName;                                    /*  ����ڵ���                  */
    PVOID                PMD_pvBus;                                     /*  ������Ϣ (������������ʹ��) */
    PVOID                PMD_pvDev;                                     /*  �豸��Ϣ (������������ʹ��) */
    
    UINT                 PMD_uiStatus;                                  /*  ��ʼΪ 0                    */
#define LW_PMD_STAT_NOR     0
#define LW_PMD_STAT_IDLE    1

    LW_CLASS_WAKEUP_NODE PMD_wunTimer;                                  /*  ����ʱ�����                */
#define PMD_bInQ         PMD_wunTimer.WUN_bInQ
#define PMD_ulCounter    PMD_wunTimer.WUN_ulCounter
    
    struct lw_pmd_funcs *PMD_pmdfunc;                                   /*  ��Դ������������������      */
} LW_PM_DEV;
typedef LW_PM_DEV       *PLW_PM_DEV;

/*********************************************************************************************************
  ��Դ��������
*********************************************************************************************************/

typedef struct lw_pma_funcs {
    INT                (*PMAF_pfuncOn)(PLW_PM_ADAPTER  pmadapter, 
                                       PLW_PM_DEV      pmdev);          /*  ���豸��Դ��ʱ��          */
    INT                (*PMAF_pfuncOff)(PLW_PM_ADAPTER  pmadapter, 
                                        PLW_PM_DEV      pmdev);         /*  �ر��豸��Դ��ʱ��          */
    INT                (*PMAF_pfuncIsOn)(PLW_PM_ADAPTER  pmadapter, 
                                         PLW_PM_DEV      pmdev,
                                         BOOL           *pbIsOn);       /*  �Ƿ��                    */
    PVOID                PMAF_pvReserve[16];                            /*  ����                        */
} LW_PMA_FUNCS;
typedef LW_PMA_FUNCS    *PLW_PMA_FUNCS;

typedef struct lw_pmd_funcs {
    INT                (*PMDF_pfuncSuspend)(PLW_PM_DEV  pmdev);         /*  CPU ����                    */
    INT                (*PMDF_pfuncResume)(PLW_PM_DEV  pmdev);          /*  CPU �ָ�                    */
    
    INT                (*PMDF_pfuncPowerSavingEnter)(PLW_PM_DEV  pmdev);/*  ϵͳ����ʡ��ģʽ            */
    INT                (*PMDF_pfuncPowerSavingExit)(PLW_PM_DEV  pmdev); /*  ϵͳ�˳�ʡ��ģʽ            */
    
    INT                (*PMDF_pfuncIdleEnter)(PLW_PM_DEV  pmdev);       /*  �豸��ʱ�䲻ʹ�ý������    */
    INT                (*PMDF_pfuncIdleExit)(PLW_PM_DEV  pmdev);        /*  �豸�˳�����                */
    
    INT                (*PMDF_pfuncCpuPower)(PLW_PM_DEV  pmdev);        /*  CPU �ı���Ƶ�ܼ�            */
    PVOID                PMDF_pvReserve[16];                            /*  ����                        */
} LW_PMD_FUNCS;
typedef LW_PMD_FUNCS    *PLW_PMD_FUNCS;

#endif                                                                  /*  LW_CFG_POWERM_EN            */
/*********************************************************************************************************
  �豸ͷ
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE               DEVHDR_lineManage;                       /*  �豸ͷ��������              */
    UINT16                     DEVHDR_usDrvNum;                         /*  ���豸��                    */
    UINT16                     DEVHDR_usDevNum;                         /*  ���豸��                    */
    PCHAR                      DEVHDR_pcName;                           /*  �豸����                    */
    UCHAR                      DEVHDR_ucType;                           /*  �豸 dirent d_type          */
    atomic_t                   DEVHDR_atomicOpenNum;                    /*  �򿪵Ĵ���                  */
    PVOID                      DEVHDR_pvReserve;                        /*  ����                        */
} LW_DEV_HDR;
typedef LW_DEV_HDR            *PLW_DEV_HDR;
typedef LW_DEV_HDR             DEV_HDR;

#define LW_DEV_MAKE_STDEV(hdr)  \
        (((dev_t)((PLW_DEV_HDR)(hdr))->DEVHDR_usDrvNum << 16) | ((PLW_DEV_HDR)(hdr))->DEVHDR_usDevNum)

/*********************************************************************************************************
  �豸ʹ�ü���
*********************************************************************************************************/

static LW_INLINE INT  LW_DEV_INC_USE_COUNT(PLW_DEV_HDR  pdevhdrHdr)
{
    return  ((pdevhdrHdr) ? (API_AtomicInc(&pdevhdrHdr->DEVHDR_atomicOpenNum)) : (PX_ERROR));
}

static LW_INLINE INT  LW_DEV_DEC_USE_COUNT(PLW_DEV_HDR  pdevhdrHdr)
{
    return  ((pdevhdrHdr) ? (API_AtomicDec(&pdevhdrHdr->DEVHDR_atomicOpenNum)) : (PX_ERROR));
}

static LW_INLINE INT  LW_DEV_GET_USE_COUNT(PLW_DEV_HDR  pdevhdrHdr)
{
    return  ((pdevhdrHdr) ? (API_AtomicGet(&pdevhdrHdr->DEVHDR_atomicOpenNum)) : (PX_ERROR));
}

/*********************************************************************************************************
  �����������֤
*********************************************************************************************************/

typedef struct {
    PCHAR                      DRVLIC_pcLicense;                        /*  ���֤                      */
    PCHAR                      DRVLIC_pcAuthor;                         /*  ����                        */
    PCHAR                      DRVLIC_pcDescription;                    /*  ����                        */
} LW_DRV_LICENSE;

/*********************************************************************************************************
  ������������
*********************************************************************************************************/

#define LW_DRV_TYPE_ORIG      0                                         /*  ԭʼ�豸����, ���� VxWorks  */
#define LW_DRV_TYPE_SOCKET    1                                         /*  SOCKET ���豸��������       */
#define LW_DRV_TYPE_NEW_1     2                                         /*  NEW_1 ���豸��������        */

/*********************************************************************************************************
  ����������ƿ� (���豸) (ֻ��ʹ�� SylixOS �µ�Ŀ¼�����ܲ���ʹ�÷�������)
*********************************************************************************************************/

typedef struct {
    LONGFUNCPTR                DEVENTRY_pfuncDevCreate;                 /*  ��������                    */
    FUNCPTR                    DEVENTRY_pfuncDevDelete;                 /*  ɾ������                    */
    
    LONGFUNCPTR                DEVENTRY_pfuncDevOpen;                   /*  �򿪺���                    */
    FUNCPTR                    DEVENTRY_pfuncDevClose;                  /*  �رպ���                    */
    
    SSIZETFUNCPTR              DEVENTRY_pfuncDevRead;                   /*  ���豸����                  */
    SSIZETFUNCPTR              DEVENTRY_pfuncDevWrite;                  /*  д�豸����                  */
    
    SSIZETFUNCPTR              DEVENTRY_pfuncDevReadEx;                 /*  ���豸������չ����          */
    SSIZETFUNCPTR              DEVENTRY_pfuncDevWriteEx;                /*  ���豸������չ����          */
    
    FUNCPTR                    DEVENTRY_pfuncDevIoctl;                  /*  �豸���ƺ���                */
    FUNCPTR                    DEVENTRY_pfuncDevSelect;                 /*  select ����                 */
    OFFTFUNCPTR                DEVENTRY_pfuncDevLseek;                  /*  lseek ����                  */
    
    FUNCPTR                    DEVENTRY_pfuncDevFstat;                  /*  fstat ����                  */
    FUNCPTR                    DEVENTRY_pfuncDevLstat;                  /*  lstat ����                  */
    
    FUNCPTR                    DEVENTRY_pfuncDevSymlink;                /*  ���������ļ�                */
    SSIZETFUNCPTR              DEVENTRY_pfuncDevReadlink;               /*  ��ȡ�����ļ�                */
    
    FUNCPTR                    DEVENTRY_pfuncDevMmap;                   /*  �ļ�ӳ��                    */
    FUNCPTR                    DEVENTRY_pfuncDevUnmap;                  /*  ӳ�����                    */
    
    BOOL                       DEVENTRY_bInUse;                         /*  �Ƿ�ʹ��                  */
    INT                        DEVENTRY_iType;                          /*  �豸��������                */
    
    UINT16                     DEVENTRY_usDevNum;                       /*  ���豸�ŷ����              */
    LW_DRV_LICENSE             DEVENTRY_drvlicLicense;                  /*  �����������֤              */
} LW_DEV_ENTRY;
typedef LW_DEV_ENTRY          *PLW_DEV_ENTRY;

/*********************************************************************************************************
  �豸�ļ��������ƿ�
*********************************************************************************************************/

typedef struct file_operations {
    struct module              *owner;
    
    long                      (*fo_create)();                           /*  DEVENTRY_pfuncDevCreate     */
    int                       (*fo_release)();                          /*  DEVENTRY_pfuncDevDelete     */
    
    long                      (*fo_open)();                             /*  DEVENTRY_pfuncDevOpen       */
    int                       (*fo_close)();                            /*  DEVENTRY_pfuncDevClose      */
    
    ssize_t                   (*fo_read)();                             /*  DEVENTRY_pfuncDevRead       */
    ssize_t                   (*fo_read_ex)();                          /*  DEVENTRY_pfuncDevReadEx     */
    
    ssize_t                   (*fo_write)();                            /*  DEVENTRY_pfuncDevWrite      */
    ssize_t                   (*fo_write_ex)();                         /*  DEVENTRY_pfuncDevWriteEx    */
    
    int                       (*fo_ioctl)();                            /*  DEVENTRY_pfuncDevIoctl      */
    int                       (*fo_select)();                           /*  DEVENTRY_pfuncDevSelect     */
    
    int                       (*fo_lock)();                             /*  not support now             */
    off_t                     (*fo_lseek)();                            /*  DEVENTRY_pfuncDevLseek      */
    
    int                       (*fo_fstat)();                            /*  DEVENTRY_pfuncDevFstat      */
    int                       (*fo_lstat)();                            /*  DEVENTRY_pfuncDevLstat      */
    
    int                       (*fo_symlink)();                          /*  DEVENTRY_pfuncDevSymlink    */
    ssize_t                   (*fo_readlink)();                         /*  DEVENTRY_pfuncDevReadlink   */
    
    int                       (*fo_mmap)();                             /*  DEVENTRY_pfuncDevMmap       */
    int                       (*fo_unmap)();                            /*  DEVENTRY_pfuncDevUnmmap     */
    
    ULONG                       fo_pad[16];                             /*  reserve                     */
} FILE_OPERATIONS;
typedef FILE_OPERATIONS        *PFILE_OPERATIONS;

/*********************************************************************************************************
  fo_mmap �� fo_unmap ��ַ��Ϣ����
*********************************************************************************************************/

typedef struct {
    PVOID                       DMAP_pvAddr;                            /*  ��ʼ��ַ                    */
    size_t                      DMAP_stLen;                             /*  �ڴ泤�� (��λ:�ֽ�)        */
    off_t                       DMAP_offPages;                          /*  �ļ�ӳ��ƫ���� (��λ:ҳ��)  */
    ULONG                       DMAP_ulFlag;                            /*  ��ǰ�����ַ VMM ����       */
} LW_DEV_MMAP_AREA;
typedef LW_DEV_MMAP_AREA       *PLW_DEV_MMAP_AREA;

/*********************************************************************************************************
  SylixOS I/O ϵͳ�ṹ
  
  (ORIG ������)
  
       0                       1                       N
  +---------+             +---------+             +---------+
  | FD_DESC |             | FD_DESC |     ...     | FD_DESC |
  +---------+             +---------+             +---------+
       |                       |                       |
       |                       |                       |
       \-----------------------/                       |
                   |                                   |
                   |                                   |
             +------------+                      +------------+
  HEADER ->  |  FD_ENTERY |   ->    ...   ->     |  FD_ENTERY |  ->  NULL
             +------------+                      +------------+
                   |                                   |
                   |                                   |
                   |                                   |
             +------------+                      +------------+
             |  DEV_HDR   |                      |  DEV_HDR   |
             +------------+                      +------------+
                   |                                   |
                   |                                   |
                   \-----------------------------------/
                                    |
                                    |
                              +------------+
                              |   DRIVER   |
                              +------------+
  
  (NEW_1 ������)
  
       0                       1                       N
  +---------+             +---------+             +---------+
  | FD_DESC |             | FD_DESC |     ...     | FD_DESC |
  +---------+             +---------+             +---------+
       |                       |                       |
       |                       |                       |
       \-----------------------/                       |
                   |                                   |
                   |                                   |
             +------------+                      +------------+
  HEADER ->  |  FD_ENTERY |   ->    ...   ->     |  FD_ENTERY |  ->  NULL
             +------------+                      +------------+
                   |                 |                 |
                   |                 |                 |
                   \-----------------/                 |
                             |                         |
                             |                         |
                      +------------+             +------------+
                      |   FD_NODE  |    ...      |   FD_NODE  |
                      | lockf->... |             | lockf->... |
                      +------------+             +------------+
                             |           |             |
                             |           |             |
                             \-----------/             |
                                   |                   |
                                   |                   |
                            +------------+       +------------+
                            |  DEV_HDR   |       |  DEV_HDR   |
                            +------------+       +------------+
                                   |                   |
                                   |                   |
                                   \-------------------/
                                             |
                                             |
                                       +------------+
                                       |   DRIVER   |
                                       +------------+
*********************************************************************************************************/
/*********************************************************************************************************
  �ļ���¼��
  ֻ�� NEW_1 ����߼����豸�������Ϳ���֧���ļ���¼��.
*********************************************************************************************************/

typedef struct __fd_lockf {
    LW_OBJECT_HANDLE           FDLOCK_ulBlock;                          /*  ������                      */
    INT16                      FDLOCK_usFlags;                          /*  F_WAIT F_FLOCK F_POSIX      */
    INT16                      FDLOCK_usType;                           /*  F_RDLCK F_WRLCK             */
    off_t                      FDLOCK_oftStart;                         /*  start of the lock           */
    off_t                      FDLOCK_oftEnd;                           /*  end of the lock (-1=EOF)    */
    pid_t                      FDLOCK_pid;                              /*  resource holding process    */
    
    struct  __fd_lockf       **FDLOCK_ppfdlockHead;                     /*  back to the head of list    */
    struct  __fd_lockf        *FDLOCK_pfdlockNext;                      /*  Next lock on this fd_node,  */
                                                                        /*  or blocking lock            */
    
    LW_LIST_LINE_HEADER        FDLOCK_plineBlockHd;                     /*  �����ڵ�ǰ���ļ�¼������    */
    LW_LIST_LINE               FDLOCK_lineBlock;                        /*  ����ȴ����� header is lockf*/
    LW_LIST_LINE               FDLOCK_lineBlockQ;                       /*  ��������, header is fd_node */
} LW_FD_LOCKF;
typedef LW_FD_LOCKF           *PLW_FD_LOCKF;

#define F_WAIT                 0x10                                     /* wait until lock is granted   */
#define F_FLOCK                0x20                                     /* BSD semantics for lock       */
#define F_POSIX                0x40                                     /* POSIX semantics for lock     */
#define F_ABORT                0x80                                     /* lock wait abort!             */

/*********************************************************************************************************
  �ļ��ڵ� 
  
  ֻ�� NEW_1 ����߼����豸�������ͻ��õ��˽ṹ
  һ�� dev_t �� һ�� ino_t ��ӦΨһһ��ʵ���ļ�, ����ϵͳͳ��δ�ͬһ��ʵ���ļ�ʱ, ֻ��һ���ļ��ڵ�
  ��� fd_entry ͬʱָ������ڵ�.
  ��Ҫ˵������ FDNODE_oftSize �ֶ���Ҫ NEW_1 ���������Լ���ά��.
  
  fd node ������ʱ, ��������д, Ҳ������ɾ��. ���ر��ļ�����ļ��� lock ���Զ����ͷ�.
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE               FDNODE_lineManage;                       /*  ͬһ�豸 fd_node ����       */
    
    LW_OBJECT_HANDLE           FDNODE_ulSem;                            /*  �ڲ�������                  */
    dev_t                      FDNODE_dev;                              /*  �豸                        */
    ino64_t                    FDNODE_inode64;                          /*  inode (64bit Ϊ�˼�����)    */
    mode_t                     FDNODE_mode;                             /*  �ļ� mode                   */
    uid_t                      FDNODE_uid;                              /*  �ļ������û���Ϣ            */
    gid_t                      FDNODE_gid;
    
    off_t                      FDNODE_oftSize;                          /*  ��ǰ�ļ���С                */
    
    struct  __fd_lockf        *FDNODE_pfdlockHead;                      /*  ��һ����                    */
    LW_LIST_LINE_HEADER        FDNODE_plineBlockQ;                      /*  ��ǰ�������ļ�¼������      */
    
    BOOL                       FDNODE_bRemove;                          /*  �Ƿ����ļ�δ�ر�ʱ�� unlink */
    ULONG                      FDNODE_ulLock;                           /*  ����, ������д, ������ɾ��  */
    ULONG                      FDNODE_ulRef;                            /*  fd_entry ���ô� fd_node ����*/
    PVOID                      FDNODE_pvFile;                           /*  ����ʹ�ô˱�����ʾ�ļ�      */
    PVOID                      FDNODE_pvFsExtern;                       /*  �ļ�ϵͳ��չʹ��            */
} LW_FD_NODE;
typedef LW_FD_NODE            *PLW_FD_NODE;

/*********************************************************************************************************
  �ļ�״̬
*********************************************************************************************************/

typedef enum {
    FDSTAT_OK = 0,                                                      /*  �ļ�����                    */
    FDSTAT_SYNC = 1,                                                    /*  �ļ�����ִ�� sync ����      */
    FDSTAT_REQCLOSE = 2,                                                /*  ����ִ������ close ����     */
    FDSTAT_CLOSING = 3,                                                 /*  ����ִ������ close ����     */
    FDSTAT_CLOSED = 4                                                   /*  �ļ��ѹر� (�����Ѳ�����)   */
} LW_FD_STATE;

#define LW_FD_STATE_IS_ABNORMITY(state) \
        ((state) >= FDSTAT_CLOSING)

/*********************************************************************************************************
  �ļ��ṹ (�ļ���)
  FDENTRY_ulCounter �����������ж�Ӧ�� LW_FD_DESC �������ܺ�. 
  ���û�� dup ��, �� FDENTRY_ulCounter Ӧ���� FDDESC_ulRef ��ͬ.
  
  ����� ORIG ��������:
  �� open �����������ײ���Ϊ FDENTRY_lValue.
  
  ����� NEW_1 ��������:
  �� open �����������ײ���Ϊ fd_entry ����, FDENTRY_lValue Ϊ open ����ֵ���ײ�������Ϊ LW_FD_NODE.
  ��Ҫ˵������ FDENTRY_oftPtr �ֶ���Ҫ���������Լ���ά��.
*********************************************************************************************************/

typedef struct {
    PLW_DEV_HDR                FDENTRY_pdevhdrHdr;                      /*  �豸ͷ                      */
    PCHAR                      FDENTRY_pcName;                          /*  �ļ���                      */
    PCHAR                      FDENTRY_pcRealName;                      /*  ȥ���������ӵ���ʵ�ļ���    */
    LW_LIST_LINE               FDENTRY_lineManage;                      /*  �ļ�������Ϣ������          */
    
#define FDENTRY_pfdnode        FDENTRY_lValue
    LONG                       FDENTRY_lValue;                          /*  ���������ڲ�����            */
                                                                        /*  ���Ϊ NEW_1 ������Ϊfd_node*/
    INT                        FDENTRY_iType;                           /*  �ļ����� (���������ж�)     */
    INT                        FDENTRY_iFlag;                           /*  �ļ�����                    */
    LW_FD_STATE                FDENTRY_state;                           /*  �ļ�״̬                    */
    ULONG                      FDENTRY_ulCounter;                       /*  �����ü�����                */
    off_t                      FDENTRY_oftPtr;                          /*  �ļ���ǰָ��                */
                                                                        /*  ֻ�� NEW_1 ����߼�����ʹ�� */
    BOOL                       FDENTRY_bRemoveReq;                      /*  ɾ������                    */
} LW_FD_ENTRY;
typedef LW_FD_ENTRY           *PLW_FD_ENTRY;

/*********************************************************************************************************
  �ļ���������
*********************************************************************************************************/

typedef struct {
    PLW_FD_ENTRY               FDDESC_pfdentry;                         /*  �ļ��ṹ                    */
    BOOL                       FDDESC_bCloExec;                         /*  FD_CLOEXEC                  */
    ULONG                      FDDESC_ulRef;                            /*  ��Ӧ�ļ������������ü���    */
} LW_FD_DESC;
typedef LW_FD_DESC            *PLW_FD_DESC;

/*********************************************************************************************************
    �̳߳�
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO               TPCB_monoResrcList;                      /*  ��Դ����                    */
    PTHREAD_START_ROUTINE      TPCB_pthreadStartAddress;                /*  �̴߳�����ڵ�              */
    PLW_LIST_RING              TPCB_pringFirstThread;                   /*  ��һ���߳�                  */
    LW_CLASS_THREADATTR        TPCB_threakattrAttr;                     /*  �߳̽������Կ�              */
    
    UINT16                     TPCB_usMaxThreadCounter;                 /*  ����߳�����                */
    UINT16                     TPCB_usThreadCounter;                    /*  ��ǰ�߳�����                */
    
    LW_OBJECT_HANDLE           TPCB_hMutexLock;                         /*  ������                      */
    UINT16                     TPCB_usIndex;                            /*  �����±�                    */
                                                                        /*  ����                        */
    CHAR                       TPCB_cThreadPoolName[LW_CFG_OBJECT_NAME_SIZE];
} LW_CLASS_THREADPOOL;
typedef LW_CLASS_THREADPOOL   *PLW_CLASS_THREADPOOL;

#endif                                                                  /*  __S_CLASS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
