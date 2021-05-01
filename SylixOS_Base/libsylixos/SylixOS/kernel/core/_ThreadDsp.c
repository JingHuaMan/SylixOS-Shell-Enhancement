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
** ��   ��   ��: _ThreadDsp.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 01 �� 10 ��
**
** ��        ��: ����ϵͳ�߳� DSP ��ع��ܿ�.
**
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _ThreadDspSwitch
** ��������: �߳� DSP �л� (�ڹر��ж�״̬�±�����)
** �䡡��  : bIntSwitch    �Ƿ�Ϊ _SchedInt() ���ж�״̬�µĵ��Ⱥ�������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_CPU_DSP_EN > 0

VOID  _ThreadDspSwitch (BOOL bIntSwitch)
{
    PLW_CLASS_TCB   ptcbCur;
    PLW_CLASS_TCB   ptcbHigh;
    REGISTER BOOL   bDisable = LW_FALSE;
    
    LW_TCB_GET_CUR(ptcbCur);
    LW_TCB_GET_HIGH(ptcbHigh);
    
#if LW_CFG_INTER_DSP > 0
    if (LW_KERN_DSP_EN_GET()) {                                         /*  �ж�״̬֧�� DSP            */
        /*
         *  �����ں�֧�� DSP ����, �жϺ����ᱣ�浱ǰ����� DSP ������
         *  ����������жϻ����ĵ��Ⱥ���, ����Ҫ���浱ǰ���� DSP ������
         */
        if (bIntSwitch == LW_FALSE) {
            if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_DSP) {
                __ARCH_DSP_SAVE(ptcbCur->TCB_pvStackDSP);               /*  ��Ҫ���浱ǰ DSP CTX        */
                bDisable = LW_TRUE;
            }
        } else {
            bDisable = LW_TRUE;
        }
    } else 
#endif                                                                  /*  LW_CFG_INTER_DSP > 0        */
    {
        /*
         *  �����ж�״̬��֧�� DSP ����, �жϺ����в���� DSP ���������κβ���
         *  ���ﲻ���� _Sched() ���� _SchedInt() ����Ҫ���浱ǰ����� DSP ������
         */
        if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_DSP) {
            __ARCH_DSP_SAVE(ptcbCur->TCB_pvStackDSP);                   /*  ��Ҫ���浱ǰ DSP CTX        */
            bDisable = LW_TRUE;
        }
    }

    if (ptcbHigh->TCB_ulOption & LW_OPTION_THREAD_USED_DSP) {
        __ARCH_DSP_RESTORE(ptcbHigh->TCB_pvStackDSP);                   /*  ��Ҫ�ָ������� DSP CTX      */
        
    } else if (bDisable) {
        __ARCH_DSP_DISABLE();                                           /*  ��������Ҫ DSP ֧��       */
    }
}
/*********************************************************************************************************
** ��������: _ThreadDspSave
** ��������: �߳� DSP ���� (�ڹر��ж�״̬�±�����)
** �䡡��  : bIntSwitch    �Ƿ�Ϊ _SchedInt() ���ж�״̬�µĵ��Ⱥ�������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

VOID  _ThreadDspSave (PLW_CLASS_TCB   ptcbCur, BOOL bIntSwitch)
{
    REGISTER BOOL   bDisable = LW_FALSE;

#if LW_CFG_INTER_DSP > 0
    if (LW_KERN_DSP_EN_GET()) {                                         /*  �ж�״̬֧�� DSP            */
        /*
         *  �����ں�֧�� DSP ����, �жϺ����ᱣ�浱ǰ����� DSP ������
         *  ����������жϻ����ĵ��Ⱥ���, ����Ҫ���浱ǰ���� DSP ������
         */
        if (bIntSwitch == LW_FALSE) {
            if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_DSP) {
                __ARCH_DSP_SAVE(ptcbCur->TCB_pvStackDSP);               /*  ��Ҫ���浱ǰ DSP CTX        */
                bDisable = LW_TRUE;
            }
        } else {
            bDisable = LW_TRUE;
        }
    } else 
#endif                                                                  /*  LW_CFG_INTER_DSP > 0        */
    {
        /*
         *  �����ж�״̬��֧�� DSP ����, �жϺ����в���� DSP ���������κβ���
         *  ���ﲻ���� _Sched() ���� _SchedInt() ����Ҫ���浱ǰ����� DSP ������
         */
        if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_DSP) {
            __ARCH_DSP_SAVE(ptcbCur->TCB_pvStackDSP);                   /*  ��Ҫ���浱ǰ DSP CTX        */
            bDisable = LW_TRUE;
        }
    }
    
    if (bDisable) {
        __ARCH_DSP_DISABLE();                                           /*  ������Ҫ DSP ֧��           */
    }
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
