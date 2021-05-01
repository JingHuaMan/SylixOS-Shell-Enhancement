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
** ��   ��   ��: signalPrivate.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 06 �� 03 ��
**
** ��        ��: ����ϵͳ�ź��ڲ�˽����Ϣ����
*********************************************************************************************************/

#ifndef __SIGNALPRIVATE_H
#define __SIGNALPRIVATE_H

#if LW_CFG_SIGNAL_EN > 0

#undef sigaction                                                        /*  ����ṹ�������ͻ          */
#undef sigvec                                                           /*  ����ṹ�������ͻ          */

/*********************************************************************************************************
  ע��:
  �ź����������:
  
  �ź�����: ��ǰ����� kill �źŵ���, �ұ�����, ��ô�źŽ��ò�������. ����, һ�����������, �������յ�����
            �� kill ���ź�, ���ᱻִ��һ��. 
            ��ǰ����� queue �źŵ���, �ұ�����, ��ô�źŽ��ò�������. ����, һ�����������, �����εĹ�����
            ���յ������ź�, �����ж��ٴ�.
            ��ǰ����������źŵ���, �� timer, aio, msgq �ȵ�, ��ֱ�Ӻ���.
            
  �źź���: ���źŵĴ�����Ϊ����ʱ, ���յ����źŻ�ֱ������, ��ȫ��������״̬.
*********************************************************************************************************/

/*********************************************************************************************************
  ʵ���źŷ��ͺ�������״̬
*********************************************************************************************************/

typedef enum {
    SEND_OK,                                                            /*  ���ͳɹ�                    */
    SEND_ERROR,                                                         /*  ����ʧ��                    */
    SEND_INFO,                                                          /*  �� wait info ����ȴ�����   */
    SEND_IGN,                                                           /*  �źű����Ե�                */
    SEND_BLOCK                                                          /*  �źű�����                  */
} LW_SEND_VAL;

/*********************************************************************************************************
  �źŵȴ���Ϣ
*********************************************************************************************************/

typedef struct __sig_wait {
    sigset_t              SIGWT_sigset;
    struct siginfo        SIGWT_siginfo;
} LW_CLASS_SIGWAIT;

/*********************************************************************************************************
  �ź��߳�������
*********************************************************************************************************/

typedef struct __sig_context {
    sigset_t              SIGCTX_sigsetMask;                            /*  ��ǰ�ź�����λ              */
    sigset_t              SIGCTX_sigsetPending;                         /*  ��ǰ���ڱ������޷����е��ź�*/
    sigset_t              SIGCTX_sigsetKill;                            /*  �� kill ���͵������ε��ź�  */
    
    struct sigaction      SIGCTX_sigaction[NSIG];                       /*  ���е��źſ��ƿ�            */
    LW_LIST_RING_HEADER   SIGCTX_pringSigQ[NSIG];                       /*  ���ڱ������޷����е��ź��Ŷ�*/
    stack_t               SIGCTX_stack;                                 /*  �û�ָ�����źŶ�ջ���      */

    LW_CLASS_SIGWAIT     *SIGCTX_sigwait;                               /*  �ȴ���Ϣ                    */
    
#if LW_CFG_SIGNALFD_EN > 0
    BOOL                  SIGCTX_bRead;                                 /*  �Ƿ��ڶ� signalfd           */
    sigset_t              SIGCTX_sigsetFdw;                             /*  ���ڵȴ��� sigset           */
    LW_SEL_WAKEUPLIST     SIGCTX_selwulist;                             /*  signalfd select list        */
#endif
} LW_CLASS_SIGCONTEXT;
typedef LW_CLASS_SIGCONTEXT *PLW_CLASS_SIGCONTEXT;

/*********************************************************************************************************
  �ź������ȴ��������Ϣ
*********************************************************************************************************/

typedef struct {
    LW_LIST_RING          SIGPEND_ringSigQ;                             /*  ��������                    */
    
    struct siginfo        SIGPEND_siginfo;                              /*  �ź������Ϣ                */
    PLW_CLASS_SIGCONTEXT  SIGPEND_psigctx;                              /*  �ź�������                  */
    UINT                  SIGPEND_uiTimes;                              /*  �������Ĵ���                */
    
    INT                   SIGPEND_iNotify;                              /*  sigevent.sigev_notify       */
} LW_CLASS_SIGPEND;
typedef LW_CLASS_SIGPEND    *PLW_CLASS_SIGPEND;

/*********************************************************************************************************
  SIGNAL CONTRL MESSAGE (ÿ����һ���ź� _doSignal() �����Զ�����һ�����½ṹ����, ����ڶ�ջ��϶)
*********************************************************************************************************/

typedef struct {
    ARCH_REG_CTX          SIGCTLMSG_archRegCtx;                         /*  �Ĵ���������                */
    INT                   SIGCTLMSG_iSchedRet;                          /*  �źŵ���������ֵ            */
    INT                   SIGCTLMSG_iKernelSpace;                       /*  �����ź��ǵ��ں˿ռ����    */
                                                                        /*  �ź��˳�ʱ��Ҫ����֮ǰ��״̬*/
    sigset_t              SIGCTLMSG_sigsetMask;                         /*  �źž���˳���Ҫ�ָ�������  */
    struct siginfo        SIGCTLMSG_siginfo;                            /*  �ź������Ϣ                */
    ULONG                 SIGCTLMSG_ulLastError;                        /*  �����ԭʼ�����            */
    UINT8                 SIGCTLMSG_ucWaitTimeout;                      /*  �����ԭʼ timeout ���     */
    UINT8                 SIGCTLMSG_ucIsEventDelete;                    /*  �¼��Ƿ�ɾ��              */
    
#if LW_CFG_CPU_FPU_EN > 0
    LW_FPU_CONTEXT       *SIGCTLMSG_pfpuctx;                            /*  FPU ������                  */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_CPU_DSP_EN > 0
    LW_DSP_CONTEXT       *SIGCTLMSG_pdspctx;                            /*  DSP ������                  */
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
} LW_CLASS_SIGCTLMSG;
typedef LW_CLASS_SIGCTLMSG  *PLW_CLASS_SIGCTLMSG;

/*********************************************************************************************************
  SIGNAL CONTRL MESSAGE �ṹ��С����
*********************************************************************************************************/

#define __SIGCTLMSG_SIZE_ALIGN      ROUND_UP(sizeof(LW_CLASS_SIGCTLMSG), sizeof(LW_STACK))
#define __SIGFPUCTX_SIZE_ALIGN      ROUND_UP(sizeof(LW_FPU_CONTEXT),     sizeof(LW_STACK))
#define __SIGDSPCTX_SIZE_ALIGN      ROUND_UP(sizeof(LW_DSP_CONTEXT),     sizeof(LW_STACK))

/*********************************************************************************************************
  UNMASK SIG
*********************************************************************************************************/

#define __SIGNO_UNMASK              (__sigmask(SIGKILL) |       \
                                     __sigmask(SIGSTOP) |       \
                                     __sigmask(SIGFPE)  |       \
                                     __sigmask(SIGILL)  |       \
                                     __sigmask(SIGBUS)  |       \
                                     __sigmask(SIGSEGV))

/*********************************************************************************************************
  UNCATCH SIG
*********************************************************************************************************/

#define __SIGNO_UNCATCH             (__sigmask(SIGKILL) |       \
                                     __sigmask(SIGSTOP))
                                     
/*********************************************************************************************************
  EXIT SIG
*********************************************************************************************************/

#define __SIGNO_MUST_EXIT           (__sigmask(SIGKILL) |       \
                                     __sigmask(SIGTERM) |       \
                                     __sigmask(SIGFPE)  |       \
                                     __sigmask(SIGILL)  |       \
                                     __sigmask(SIGBUS)  |       \
                                     __sigmask(SIGSEGV))
                                     
/*********************************************************************************************************
  Is si_code is stop
*********************************************************************************************************/

#define __SI_CODE_STOP(code)        (((code) == CLD_STOPPED) || \
                                     ((code) == CLD_CONTINUED))

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
#endif                                                                  /*  __SIGNALPRIVATE_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
