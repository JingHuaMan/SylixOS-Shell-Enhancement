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
** ��   ��   ��: tty.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 02 �� 04 ��
**
** ��        ��: TTY �豸ͷ�ļ�.

** BUG:
2009.08.18  ����д����������.
*********************************************************************************************************/

#ifndef __TTY_H
#define __TTY_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0)
/*********************************************************************************************************
  TY_DEV_WINSIZE
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    UINT16                  TYDEVWINS_usRow;
    UINT16                  TYDEVWINS_usCol;
    UINT16                  TYDEVWINS_usXPixel;
    UINT16                  TYDEVWINS_usYPixel;
} TY_DEV_WINSIZE;

/*********************************************************************************************************
  TY_DEV_RD_STATE
*********************************************************************************************************/

typedef struct {
    BOOL                    TYDEVRDSTAT_bXoff;                          /*  �����Ƿ� XOFF ��          */
    BOOL                    TYDEVRDSTAT_bPending;                       /*  XON/OFF�Ƿ���Ҫ����         */
    BOOL                    TYDEVRDSTAT_bCanceled;                      /*  ����������ֹ��              */
    BOOL                    TYDEVRDSTAT_bFlushingRdBuf;                 /*  �����ؼ�����ʱ�ı�־        */
    CHAR                    TYDEVRDSTAT_cLastRecv;                      /*  OPT_CRMOD ����һ�����յ��ַ�*/
} TY_DEV_RD_STATE;

/*********************************************************************************************************
  TY_DEV_WR_STATE
*********************************************************************************************************/

typedef struct {
    BOOL                    TYDEVWRSTAT_bBusy;                          /*  �Ƿ����ڷ�������            */
    BOOL                    TYDEVWRSTAT_bXoff;                          /*  �����Ƿ� XOFF ��          */
    BOOL                    TYDEVWRSTAT_bCR;                            /*  CR �ַ��Ƿ���Ҫ�����       */
    BOOL                    TYDEVWRSTAT_bCanceled;                      /*  д��������ֹ��              */
    BOOL                    TYDEVWRSTAT_bFlushingWrtBuf;                /*  �����ؼ�����ʱ�ı�־        */
} TY_DEV_WR_STATE;

/*********************************************************************************************************
  TY_DEV
*********************************************************************************************************/

typedef struct {
    LW_DEV_HDR              TYDEV_devhdrHdr;                            /*  I/O ϵͳ�ӿ��豸ͷ          */
    
    VX_RING_ID              TYDEV_vxringidRdBuf;                        /*  ���뻷�λ�����              */
    VX_RING_ID              TYDEV_vxringidWrBuf;                        /*  ������λ�����              */
    
    LW_OBJECT_HANDLE        TYDEV_hRdSyncSemB;                          /*  ��ͬ���ź���                */
    LW_OBJECT_HANDLE        TYDEV_hWrtSyncSemB;                         /*  дͬ���ź���                */
    LW_OBJECT_HANDLE        TYDEV_hDrainSyncSemB;                       /*  �������ͬ���ź���          */
    LW_OBJECT_HANDLE        TYDEV_hMutexSemM;                           /*  ��������ź���              */
    
    TY_DEV_RD_STATE         TYDEV_tydevrdstat;                          /*  ��״̬                      */
    TY_DEV_WR_STATE         TYDEV_tydevwrstat;                          /*  д״̬                      */
    
    INT                     TYDEV_iWrtThreshold;                        /*  д��������������            */
    
    UINT8                   TYDEV_ucInNBytes;                           /*  ���µ�û�н��������е��ַ���*/
    UINT8                   TYDEV_ucInBytesLeft;                        /*  ���µ�û�н���������ʣ���  */
                                                                        /*  �ַ���                      */
    FUNCPTR                 TYDEV_pfuncTxStartup;                       /*  �������͵ĺ���ָ��          */
    FUNCPTR                 TYDEV_pfuncProtoHook;                       /*  ʹ������Э��ջʱ�Ļص�����  */
    
    INT                     TYDEV_iProtoArg;                            /*  ʹ��Э��ջʱ�Ĳ���,         */
    INT                     TYDEV_iOpt;                                 /*  ��ǰ�ն˵Ĺ���������ѡ��    */
    
    ULONG                   TYDEV_ulRTimeout;                           /*  ��������ʱʱ��              */
    ULONG                   TYDEV_ulWTimeout;                           /*  д������ʱʱ��              */
    
    INT                     TYDEV_iAbortFlag;                           /*  abort ��־                  */
    
    FUNCPTR                 TYDEV_pfuncCtrlC;                           /*  control-C hook function     */
    PVOID                   TYDEV_pvArgCtrlC;                           /*  control-C hook function     */
    
    LW_SEL_WAKEUPLIST       TYDEV_selwulList;                           /*  select() �ȴ���             */
    time_t                  TYDEV_timeCreate;                           /*  ����ʱ��                    */

    CHAR                    TYDEV_cCtlChars[19];                        /*  termios �����ַ� NCCS       */
    TY_DEV_WINSIZE          TYDEV_tydevwins;                            /*  ���ڴ�С                    */

    LW_SPINLOCK_DEFINE     (TYDEV_slLock);                              /*  ������                      */
} TY_DEV;
typedef TY_DEV             *TY_DEV_ID;

/*********************************************************************************************************
  TYCO_DEV
*********************************************************************************************************/

typedef struct {
    TY_DEV                  TYCODEV_tydevTyDev;                         /*  TY �豸                     */
    SIO_CHAN               *TYCODEV_psiochan;                           /*  ͬ��I/Oͨ���Ĺ��ܺ���       */
} TYCO_DEV;

/*********************************************************************************************************
  TTY INTERNAL FUNCTION
*********************************************************************************************************/

INT                         _TyDevInit(TY_DEV_ID  ptyDev, size_t stRdBufSize, size_t stWrtBufSize,
                                       FUNCPTR    pfunctxStartup);
INT                         _TyDevRemove(TY_DEV_ID  ptyDev);

INT                         _TyIRd(    TY_DEV_ID  ptyDev, CHAR   cInchar);
INT                         _TyITx(    TY_DEV_ID  ptyDev, PCHAR  pcChar);
ssize_t                     _TyRead(   TY_DEV_ID  ptyDev, PCHAR  pcBuffer, size_t  stMaxBytes);
ssize_t                     _TyWrite(  TY_DEV_ID  ptyDev, PCHAR  pcBuffer, size_t  stNBytes);
INT                         _TyIoctl(  TY_DEV_ID  ptyDev, INT    iRequest, LONG lArg);

/*********************************************************************************************************
  TTY KERNEL API
*********************************************************************************************************/

LW_API INT                  API_TtyDrvInstall(VOID);
LW_API INT                  API_TtyDevCreate(PCHAR     pcName,
                                             SIO_CHAN *psiochan,
                                             size_t    stRdBufSize,
                                             size_t    stWrtBufSize);
LW_API INT                  API_TtyDevRemove(PCHAR   pcName, BOOL  bForce);

#define ttyDrv              API_TtyDrvInstall
#define ttyDevCreate        API_TtyDevCreate
#define ttyDevRemove        API_TtyDevRemove

/*********************************************************************************************************
  GLOBAL VAR
*********************************************************************************************************/

#ifdef  __TYCO_MAIN_FILE
#define __TYCO_EXT
#else
#define __TYCO_EXT    extern
#endif                                                                  /*  __TYCO_MAIN_FILE            */
#ifndef __TYCO_MAIN_FILE
__TYCO_EXT      INT         _G_iTycoDrvNum;                             /*  �Ƿ�װ����TY����            */
#else
__TYCO_EXT      INT         _G_iTycoDrvNum = PX_ERROR;
#endif                                                                  /*  __TYCO_MAIN_FILE            */
#ifndef __TYCO_MAIN_FILE
__TYCO_EXT      ULONG       _G_ulMutexOptionsTyLib;
#else
__TYCO_EXT      ULONG       _G_ulMutexOptionsTyLib = (LW_OPTION_WAIT_PRIORITY
                                                   |  LW_OPTION_DELETE_SAFE
                                                   |  LW_OPTION_INHERIT_PRIORITY);
#endif                                                                  /*  __TYCO_MAIN_FILE            */

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  TTY API
*********************************************************************************************************/

LW_API VOID                 API_TyAbortFuncSet(FUNCPTR pfuncAbort);
LW_API VOID                 API_TyAbortSet(CHAR        cAbort);
LW_API VOID                 API_TyBackspaceSet(CHAR    cBackspace);
LW_API VOID                 API_TyDeleteLineSet(CHAR   cDeleteLine);
LW_API VOID                 API_TyEOFSet(CHAR  cEOF);
LW_API VOID                 API_TyMonitorTrapSet(CHAR  cMonitorTrap);

/*********************************************************************************************************
  API
*********************************************************************************************************/

#define tyAbortFuncSet      API_TyAbortFuncSet
#define tyAbortSet          API_TyAbortSet
#define tyBackspaceSet      API_TyBackspaceSet
#define tyDeleteLineSet     API_TyDeleteLineSet
#define tyEOFSet            API_TyEOFSet
#define tyMonitorTrapSet    API_TyMonitorTrapSet

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
#endif                                                                  /*  __TTY_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
