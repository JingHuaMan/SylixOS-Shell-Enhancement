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
** ��   ��   ��: spipe.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 27 ��
**
** ��        ��: �����ַ���ʽ�ܵ��ڲ�ͷ�ļ� (STREAM PIPE)

** BUG
2007.04.08  �����˶Բü��ĺ�֧��
2007.11.20  ���� select ����.
2012.08.25  ����ܵ����˹رռ��:
            1: ������˹ر�, д���������յ� SIGPIPE �ź�, write ���᷵�� -1.
            2: ���д�˹ر�, ���������������������, Ȼ���ٴζ����� 0.
*********************************************************************************************************/

#ifndef __SPIPE_H
#define __SPIPE_H

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SPIPE_EN > 0)

/*********************************************************************************************************
  PIPE RING BUFFER (���λ���)
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    PCHAR                 RINGBUFFER_pcBuffer;                           /*  ����������ַ               */
    PCHAR                 RINGBUFFER_pcInPtr;                            /*  ����ָ��                   */
    PCHAR                 RINGBUFFER_pcOutPtr;                           /*  ���ָ��                   */
    size_t                RINGBUFFER_stTotalBytes;                       /*  ��������С                 */
    size_t                RINGBUFFER_stMsgBytes;                         /*  ��Ч������                 */
} LW_RING_BUFFER;
typedef LW_RING_BUFFER   *PLW_RING_BUFFER;

/*********************************************************************************************************
  SPIPE DEVICE
*********************************************************************************************************/

typedef struct {
    LW_DEV_HDR            SPIPEDEV_devhdrHdr;                           /*  �豸ͷ                      */
    LW_RING_BUFFER        SPIPEDEV_ringbufferBuffer;                    /*  ���λ�����                  */
    LW_SEL_WAKEUPLIST     SPIPEDEV_selwulList;                          /*  �ȴ���                      */
    
    LW_OBJECT_HANDLE      SPIPEDEV_hOpLock;                             /*  ������                      */
    LW_OBJECT_HANDLE      SPIPEDEV_hReadLock;                           /*  ����                        */
    LW_OBJECT_HANDLE      SPIPEDEV_hWriteLock;                          /*  д��                        */
    
    UINT                  SPIPEDEV_uiReadCnt;                           /*  ���˴�����                */
    UINT                  SPIPEDEV_uiWriteCnt;                          /*  д�˴�����                */
    BOOL                  SPIPEDEV_bUnlinkReq;                          /*  �����һ�ιر�ʱɾ��        */
    
    ULONG                 SPIPEDEV_ulRTimeout;                          /*  ��������ʱʱ��              */
    ULONG                 SPIPEDEV_ulWTimeout;                          /*  д������ʱʱ��              */
    
    INT                   SPIPEDEV_iAbortFlag;                          /*  abort �쳣��־              */
    time_t                SPIPEDEV_timeCreate;                          /*  ����ʱ��                    */
} LW_SPIPE_DEV;
typedef LW_SPIPE_DEV     *PLW_SPIPE_DEV;

/*********************************************************************************************************
  SPIPE FILE
*********************************************************************************************************/

typedef struct {
    PLW_SPIPE_DEV        SPIPEFIL_pspipedev;
    INT                  SPIPEFIL_iFlags;                               /*  ��������                    */
    INT                  SPIPEFIL_iMode;                                /*  ������ʽ                    */
    INT                  SPIPEFIL_iExtMode;
} LW_SPIPE_FILE;
typedef LW_SPIPE_FILE   *PLW_SPIPE_FILE;

/*********************************************************************************************************
  INTERNAL FUNCTION
*********************************************************************************************************/

LONG       _SpipeOpen( PLW_SPIPE_DEV  pspipedev, PCHAR  pcName,   INT  iFlags, INT  iMode);
INT        _SpipeRemove(PLW_SPIPE_DEV pspipedev, PCHAR  pcName);
INT        _SpipeClose(PLW_SPIPE_FILE pspipefil);
ssize_t    _SpipeRead( PLW_SPIPE_FILE pspipefil, PCHAR  pcBuffer, size_t stMaxBytes);
ssize_t    _SpipeWrite(PLW_SPIPE_FILE pspipefil, PCHAR  pcBuffer, size_t stNBytes);
INT        _SpipeIoctl(PLW_SPIPE_FILE pspipefil, INT    iRequest, INT  *piArgPtr);

/*********************************************************************************************************
  GLOBAL VAR
*********************************************************************************************************/

#ifdef  __SPIPE_MAIN_FILE
#define __SPIPE_EXT
#else
#define __SPIPE_EXT    extern
#endif

#ifndef __SPIPE_MAIN_FILE
__SPIPE_EXT    INT     _G_iSpipeDrvNum;                                 /*  �Ƿ�װ���˹ܵ�����          */
#else
__SPIPE_EXT    INT     _G_iSpipeDrvNum = PX_ERROR;
#endif

#ifndef __SPIPE_MAIN_FILE
__SPIPE_EXT    ULONG   _G_ulSpipeLockOpt;                               /*  ���ź�������                */
#else
__SPIPE_EXT    ULONG   _G_ulSpipeLockOpt = (LW_OPTION_WAIT_PRIORITY
                                         |  LW_OPTION_DELETE_SAFE
                                         |  LW_OPTION_INHERIT_PRIORITY);
#endif

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  SPIPE API
*********************************************************************************************************/

LW_API INT          API_SpipeDrvInstall(VOID);
LW_API INT          API_SpipeDevCreate(PCHAR  pcName, size_t  stBufferByteSize);
LW_API INT          API_SpipeDevDelete(PCHAR  pcName, BOOL bForce);

/*********************************************************************************************************
  API
*********************************************************************************************************/

#define spipeDevCreate  API_SpipeDevCreate
#define spipeDevDelete  API_SpipeDevDelete
#define spipeDrv        API_SpipeDrvInstall

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SPIPE_EN > 0)       */
#endif                                                                  /*  __SPIPE_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
