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
** ��   ��   ��: pipe.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 27 ��
**
** ��        ��: ���� VxWorks ���ݹܵ��ڲ�ͷ�ļ�

** BUG
2007.04.08  �����˶Բü��ĺ�֧��
2007.11.20  ���� select ����.
*********************************************************************************************************/

#ifndef __PIPE_H
#define __PIPE_H

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PIPE_EN > 0)

/*********************************************************************************************************
  PIPE DEVICE
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    LW_DEV_HDR             PIPEDEV_devhdrHdr;
    LW_SEL_WAKEUPLIST      PIPEDEV_selwulList;                          /*  �ȴ���                      */

    LW_OBJECT_HANDLE       PIPEDEV_hMsgQueue;
    
    INT                    PIPEDEV_iFlags;                              /*  ��������                    */
    INT                    PIPEDEV_iMode;                               /*  ������ʽ                    */
    
    ULONG                  PIPEDEV_ulRTimeout;                          /*  ��������ʱʱ��              */
    ULONG                  PIPEDEV_ulWTimeout;                          /*  д������ʱʱ��              */

    INT                    PIPEDEV_iAbortFlag;                          /*  abort ��־                  */
    time_t                 PIPEDEV_timeCreate;                          /*  ����ʱ��                    */
} LW_PIPE_DEV;
typedef LW_PIPE_DEV       *PLW_PIPE_DEV;

/*********************************************************************************************************
  INTERNAL FUNCTION (_pipedev Ϊ���ͣ����� ppipedev ǰ׺�����������죬ppipedev ӦΪ p_pipedev)
*********************************************************************************************************/

LONG    _PipeOpen( PLW_PIPE_DEV  p_pipedev, PCHAR  pcName,   INT  iFlags, INT  iMode);
INT     _PipeClose(PLW_PIPE_DEV  p_pipedev);
ssize_t _PipeRead( PLW_PIPE_DEV  p_pipedev, PCHAR  pcBuffer, size_t  stMaxBytes);
ssize_t _PipeWrite(PLW_PIPE_DEV  p_pipedev, PCHAR  pcBuffer, size_t  stNBytes);
INT     _PipeIoctl(PLW_PIPE_DEV  p_pipedev, INT    iRequest, INT  *piArgPtr);

/*********************************************************************************************************
  GLOBAL VAR
*********************************************************************************************************/

#ifdef  __PIPE_MAIN_FILE
#define __PIPE_EXT
#else
#define __PIPE_EXT    extern
#endif

#ifndef __PIPE_MAIN_FILE
__PIPE_EXT    INT     _G_iPipeDrvNum;                                   /*  �Ƿ�װ���˹ܵ�����          */
#else
__PIPE_EXT    INT     _G_iPipeDrvNum = PX_ERROR;
#endif

#ifndef __PIPE_MAIN_FILE
__PIPE_EXT    ULONG   _G_ulPipeLockOpt;                                 /*  ���ź�������                */
#else
__PIPE_EXT    ULONG   _G_ulPipeLockOpt = LW_OPTION_WAIT_PRIORITY;
#endif

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  PIPE API
*********************************************************************************************************/

LW_API INT          API_PipeDrvInstall(VOID);
LW_API INT          API_PipeDevCreate(PCHAR  pcName, ULONG  ulNMessages, size_t  stNBytes);
LW_API INT          API_PipeDevDelete(PCHAR  pcName, BOOL bForce);

/*********************************************************************************************************
  API
*********************************************************************************************************/

#define pipeDevCreate   API_PipeDevCreate
#define pipeDevDelete   API_PipeDevDelete
#define pipeDrv         API_PipeDrvInstall

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_PIPE_EN > 0)        */
#endif                                                                  /*  __PIPE_H                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
