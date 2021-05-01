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
** ��   ��   ��: InterEnterExit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ����ϵͳ�ں��жϳ��ں�����

** BUG
2008.01.04  �޸Ĵ����ʽ��ע��.
2009.04.29  ���� SMP ֧��.
2011.02.22  ֱ�ӵ��� _SchedInt() �����жϵ���.
2012.09.05  API_InterEnter() ��һ�ν����ж�ʱ, ��Ҫ���浱ǰ����� FPU ������.
            API_InterExit() ���û�в�������, ��ָ����ж������ FPU ������.
2012.09.23  ���� INT ENTER �� INT EXIT �ص�.
2013.07.17  �����������ͨ���˼��ж����.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2013.07.19  �ϲ���ͨ CPU �жϺͺ˼��жϳ���, �������ֺ˼��ж�����ͨ�жϳ���.
2013.12.12  ���ﲻ�ٴ����ж� hook.
2015.05.14  �Ż��жϽ������˳�����.
2017.04.28  �жϷ��ز�����ȫ���� API_InterExit() ��������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  macro
*********************************************************************************************************/
#if LW_CFG_INTER_FPU > 0
#define __INTER_FPU_CTX(nest)      &pcpu->CPU_fpuctxContext[nest]
#endif                                                                  /*  LW_CFG_INTER_FPU > 0        */
#if LW_CFG_INTER_DSP > 0
#define __INTER_DSP_CTX(nest)      &pcpu->CPU_dspctxContext[nest]
#endif                                                                  /*  LW_CFG_INTER_DSP > 0        */
/*********************************************************************************************************
** ��������: __fpuInterEnter
** ��������: �����ж�״̬ FPU ���� (�ڹ��жϵ�����±�����)
** �䡡��  : pcpu  ��ǰ CPU ���ƿ�
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_CPU_FPU_EN > 0) && (LW_CFG_INTER_FPU > 0)

static VOID  __fpuInterEnter (PLW_CLASS_CPU  pcpu)
{
    PLW_CLASS_TCB  ptcbCur;
    ULONG          ulInterNesting = pcpu->CPU_ulInterNesting;
    
    if (ulInterNesting == 1) {                                          /*  ������̬�����ж�            */
        ptcbCur = pcpu->CPU_ptcbTCBCur;
        if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
            __ARCH_FPU_SAVE(ptcbCur->TCB_pvStackFP);                    /*  ���浱ǰ���ж��߳� FPU CTX  */
        
        } else {
            __ARCH_FPU_RESTORE(__INTER_FPU_CTX(ulInterNesting - 1));    /*  ʹ�ܵ�ǰ�ж��� FPU          */
        }

    } else {
        REGISTER ULONG  ulOldNest = ulInterNesting - 1;
        __ARCH_FPU_SAVE(__INTER_FPU_CTX(ulOldNest));
    }
}
/*********************************************************************************************************
** ��������: __fpuInterExit
** ��������: �˳��ж�״̬ FPU ���� (�ڹ��жϵ�����±�����)
** �䡡��  : pcpu  ��ǰ CPU ���ƿ�
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __fpuInterExit (PLW_CLASS_CPU  pcpu)
{
    PLW_CLASS_TCB  ptcbCur;
    ULONG          ulInterNesting = pcpu->CPU_ulInterNesting;
    
    if (ulInterNesting == 0) {                                          /*  �˳�������״̬              */
        ptcbCur = pcpu->CPU_ptcbTCBCur;
        if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
            __ARCH_FPU_RESTORE(ptcbCur->TCB_pvStackFP);                 /*  û�в�������, ��ָ� FPU CTX*/
        
        } else {
            __ARCH_FPU_DISABLE();                                       /*  ����ִ�е�������Ҫ FPU    */
        }

    } else {                                                            /*  �˳������ж���            */
        __ARCH_FPU_RESTORE(__INTER_FPU_CTX(ulInterNesting));
    }
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
                                                                        /*  LW_CFG_INTER_FPU > 0        */
/*********************************************************************************************************
** ��������: __dspInterEnter
** ��������: �����ж�״̬ DSP ���� (�ڹ��жϵ�����±�����)
** �䡡��  : pcpu  ��ǰ CPU ���ƿ�
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if (LW_CFG_CPU_DSP_EN > 0) && (LW_CFG_INTER_DSP > 0)

static VOID  __dspInterEnter (PLW_CLASS_CPU  pcpu)
{
    PLW_CLASS_TCB  ptcbCur;
    ULONG          ulInterNesting = pcpu->CPU_ulInterNesting;

    if (ulInterNesting == 1) {                                          /*  ������̬�����ж�            */
        ptcbCur = pcpu->CPU_ptcbTCBCur;
        if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_DSP) {
            __ARCH_DSP_SAVE(ptcbCur->TCB_pvStackDSP);                   /*  ���浱ǰ���ж��߳� DSP CTX  */

        } else {
            __ARCH_DSP_RESTORE(__INTER_DSP_CTX(ulInterNesting - 1));    /*  ʹ�ܵ�ǰ�ж��� DSP          */
        }

    } else {
        REGISTER ULONG  ulOldNest = ulInterNesting - 1;
        __ARCH_DSP_SAVE(__INTER_DSP_CTX(ulOldNest));
    }
}
/*********************************************************************************************************
** ��������: __dspInterExit
** ��������: �˳��ж�״̬ DSP ���� (�ڹ��жϵ�����±�����)
** �䡡��  : pcpu  ��ǰ CPU ���ƿ�
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __dspInterExit (PLW_CLASS_CPU  pcpu)
{
    PLW_CLASS_TCB  ptcbCur;
    ULONG          ulInterNesting = pcpu->CPU_ulInterNesting;

    if (ulInterNesting == 0) {                                          /*  �˳�������״̬              */
        ptcbCur = pcpu->CPU_ptcbTCBCur;
        if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_DSP) {
            __ARCH_DSP_RESTORE(ptcbCur->TCB_pvStackDSP);                /*  û�в�������, ��ָ� DSP CTX*/

        } else {
            __ARCH_DSP_DISABLE();                                       /*  ����ִ�е�������Ҫ DSP    */
        }

    } else {                                                            /*  �˳������ж���            */
        __ARCH_DSP_RESTORE(__INTER_DSP_CTX(ulInterNesting));
    }
}

#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
                                                                        /*  LW_CFG_INTER_DSP > 0        */
/*********************************************************************************************************
** ��������: API_InterEnter
** ��������: �ں��ж���ں��� (�ڹ��жϵ�����±�����)
** �䡡��  : 
** �䡡��  : �жϲ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG    API_InterEnter (ARCH_REG_T  reg0,
                         ARCH_REG_T  reg1,
                         ARCH_REG_T  reg2,
                         ARCH_REG_T  reg3)
{
    PLW_CLASS_CPU  pcpu;
    
    pcpu = LW_CPU_GET_CUR();
    pcpu->CPU_ulInterNesting++;

#if !defined(__SYLIXOS_ARM_ARCH_M__) || (LW_CFG_CORTEX_M_SVC_SWITCH > 0)
    archIntCtxSaveReg(pcpu, reg0, reg1, reg2, reg3);
#endif

#if (LW_CFG_CPU_FPU_EN > 0) && (LW_CFG_INTER_FPU > 0)
    if (LW_KERN_FPU_EN_GET()) {                                         /*  �ж�״̬����ʹ�ø�������    */
        __fpuInterEnter(pcpu);
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
                                                                        /*  LW_CFG_INTER_FPU > 0        */
#if (LW_CFG_CPU_DSP_EN > 0) && (LW_CFG_INTER_DSP > 0)
    if (LW_KERN_DSP_EN_GET()) {                                         /*  �ж�״̬����ʹ�� DSP        */
        __dspInterEnter(pcpu);
    }
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
                                                                        /*  LW_CFG_INTER_DSP > 0        */
    return  (pcpu->CPU_ulInterNesting);
}
/*********************************************************************************************************
** ��������: API_InterExit
** ��������: �ں��жϳ��ں��� (�ڹ��жϵ�����±�����)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
VOID    API_InterExit (VOID)
{
    PLW_CLASS_CPU  pcpu;
    
    pcpu = LW_CPU_GET_CUR();
    
#if LW_CFG_INTER_INFO > 0
    if (pcpu->CPU_ulInterNestingMax < pcpu->CPU_ulInterNesting) {
        pcpu->CPU_ulInterNestingMax = pcpu->CPU_ulInterNesting;
    }
#endif                                                                  /*  LW_CFG_INTER_INFO > 0       */

    if (pcpu->CPU_ulInterNesting) {                                     /*  ϵͳ�ж�Ƕ�ײ���--          */
        pcpu->CPU_ulInterNesting--;
    }
    
    if (pcpu->CPU_ulInterNesting) {                                     /*  �鿴ϵͳ�Ƿ����ж�Ƕ����    */
#if (LW_CFG_CPU_FPU_EN > 0) && (LW_CFG_INTER_FPU > 0)                   /*  �ָ���һ�ȼ��ж� FPU CTX    */
        if (LW_KERN_FPU_EN_GET()) {
            __fpuInterExit(pcpu);
        }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
                                                                        /*  LW_CFG_INTER_FPU > 0        */
#if (LW_CFG_CPU_DSP_EN > 0) && (LW_CFG_INTER_DSP > 0)                   /*  �ָ���һ�ȼ��ж� DSP CTX    */
        if (LW_KERN_DSP_EN_GET()) {
            __dspInterExit(pcpu);
        }
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
        return;                                                         /*  LW_CFG_INTER_DSP > 0        */
    }
    
    __KERNEL_SCHED_INT(pcpu);                                           /*  �ж��еĵ���                */
    
#if (LW_CFG_CPU_FPU_EN > 0) && (LW_CFG_INTER_FPU > 0)
    if (LW_KERN_FPU_EN_GET()) {
        __fpuInterExit(pcpu);
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
                                                                        /*  LW_CFG_INTER_FPU > 0        */
#if (LW_CFG_CPU_DSP_EN > 0) && (LW_CFG_INTER_DSP > 0)
    if (LW_KERN_DSP_EN_GET()) {
        __dspInterExit(pcpu);
    }
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
                                                                        /*  LW_CFG_INTER_DSP > 0        */

#if !defined(__SYLIXOS_ARM_ARCH_M__) || (LW_CFG_CORTEX_M_SVC_SWITCH > 0)
    archIntCtxLoad(pcpu);                                               /*  �жϷ��� (��ǰ���� CTX ����)*/
#endif
}
/*********************************************************************************************************
** ��������: API_InterExitNoSched
** ��������: �ں��жϳ��ں��� (�ڹ��жϵ�����±�����), �������������, �� ARMv7M ʹ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__) && (LW_CFG_CORTEX_M_SVC_SWITCH > 0)

LW_API
BOOL    API_InterExitNoSched (VOID)
{
    PLW_CLASS_CPU  pcpu;
    BOOL           bNeedSched;

    pcpu = LW_CPU_GET_CUR();

#if LW_CFG_INTER_INFO > 0
    if (pcpu->CPU_ulInterNestingMax < pcpu->CPU_ulInterNesting) {
        pcpu->CPU_ulInterNestingMax = pcpu->CPU_ulInterNesting;
    }
#endif                                                                  /*  LW_CFG_INTER_INFO > 0       */

    if (pcpu->CPU_ulInterNesting) {                                     /*  ϵͳ�ж�Ƕ�ײ���--          */
        pcpu->CPU_ulInterNesting--;
    }

    bNeedSched = __KERNEL_SCHED_INT_CHECK(pcpu);

#if (LW_CFG_CPU_FPU_EN > 0) && (LW_CFG_INTER_FPU > 0)
    if (LW_KERN_FPU_EN_GET()) {
        __fpuInterExit(pcpu);
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
                                                                        /*  LW_CFG_INTER_FPU > 0        */
#if (LW_CFG_CPU_DSP_EN > 0) && (LW_CFG_INTER_DSP > 0)
    if (LW_KERN_DSP_EN_GET()) {
        __dspInterExit(pcpu);
    }
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
                                                                        /*  LW_CFG_INTER_DSP > 0        */
    return  (bNeedSched);
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
