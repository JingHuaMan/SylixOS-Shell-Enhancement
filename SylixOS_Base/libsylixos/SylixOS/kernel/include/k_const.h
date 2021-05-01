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
** ��   ��   ��: k_const.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 12 ��
**
** ��        ��: ����ϵͳ���Ƴ������塣

** BUG:
2009.07.28  ���¶����ڴ��С���㷽��.
*********************************************************************************************************/

#ifndef __K_CONST_H
#define __K_CONST_H

/*********************************************************************************************************
  ȫ�־�����
*********************************************************************************************************/

#define LW_GLOBAL_RDY_PCBBMAP()     (&(_K_pcbbmapGlobalReady))
#define LW_GLOBAL_RDY_BMAP()        (&(_K_pcbbmapGlobalReady.PCBM_bmap))
#define LW_GLOBAL_RDY_PPCB(prio)    (&(_K_pcbbmapGlobalReady.PCBM_pcb[prio]))

/*********************************************************************************************************
  TCB ���ֽ������壬������ֶ�����ֽ���
*********************************************************************************************************/

#define __SEGMENT_BLOCK_SIZE_ALIGN  ROUND_UP(sizeof(LW_CLASS_SEGMENT), LW_CFG_HEAP_ALIGNMENT)

#if LW_CFG_COROUTINE_EN > 0
#define __CRCB_SIZE_ALIGN           ROUND_UP(sizeof(LW_CLASS_COROUTINE), sizeof(LW_STACK))
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

/*********************************************************************************************************
  ��С��ջ���ֽ���
*********************************************************************************************************/

#define __STK_MINMUM_BYTE_SIZE      (ARCH_STK_MIN_WORD_SIZE * sizeof(LW_STACK))

/*********************************************************************************************************
  EVENT WAIT PRIORITY QUEUE
*********************************************************************************************************/

#define __EVENT_Q_SIZE              8                                   /*  Q HASH SIZE                 */
#define __EVENT_Q_EPRIO             32                                  /*  256 / 8                     */
#define __EVENT_Q_SHIFT             5                                   /*  2 ^ 5 = 32                  */

/*********************************************************************************************************
  �ں�����
*********************************************************************************************************/

#define LW_KERN_FLAG_INT_FPU            0x0001                          /*  �ж����Ƿ����ʹ�ø���      */
#define LW_KERN_FLAG_BUG_REBOOT         0x0002                          /*  �ں�̽�⵽ BUG �Ƿ���Ҫ���� */
#define LW_KERN_FLAG_SMP_FSCHED         0x0004                          /*  SMP �Ƿ�ʹ�ܿ��ٵ���        */
                                                                        /*  ���ٵ��Ƚ����������ĺ˼��ж�*/
#define LW_KERN_FLAG_SMT_BSCHED         0x0008                          /*  SMT �������                */
#define LW_KERN_FLAG_NO_ITIMER          0x0010                          /*  ��֧�� ITIMER_REAL          */
                                                                        /*         ITIMER_VIRTUAL       */
                                                                        /*         ITIMER_PROF          */
                                                                        /*  ��� tick �ж�ִ���ٶ�      */
#define LW_KERN_FLAG_TMCVT_SIMPLE       0x0020                          /*  timespec ת��Ϊ tick ����   */
#define LW_KERN_FLAG_INT_DSP            0x0040                          /*  �ж����Ƿ����ʹ�� DSP      */
#define LW_KERN_FLAG_NET_LOCK_FIFO      0x0080                          /*  ������ FIFO �ȴ�            */
#define LW_KERN_FLAG_AUTO_REC_TCB       0x0100                          /*  �Զ����� TCB ���� 1.9.7 ֮ǰ*/
                                                                        /*  �汾, �� detach �߳�ɾ����  */
                                                                        /*  �����ͷ� TCB                */
#define LW_KERN_FLAG_TEXT_RO            0x0200                          /*  �ں� TEXT ֻ��              */
#define LW_KERN_FLAG_REBOOT_VMM_EN      0x0400                          /*  �ں�����ʱ���� VMM ʹ��     */
#define LW_KERN_FLAG_REBOOT_CACHE_EN    0x0800                          /*  �ں�����ʱ���� CACHE ʹ��   */

/*********************************************************************************************************
  �ں��Ƿ�֧�ָ���״̬
*********************************************************************************************************/

#define LW_KERN_FPU_EN_SET(en)                              \
        do {                                                \
            if (en) {                                       \
                _K_ulKernFlags |= LW_KERN_FLAG_INT_FPU;     \
            } else {                                        \
                _K_ulKernFlags &= ~LW_KERN_FLAG_INT_FPU;    \
            }                                               \
        } while (0)
#define LW_KERN_FPU_EN_GET()        (_K_ulKernFlags & LW_KERN_FLAG_INT_FPU)

/*********************************************************************************************************
  �ں��Ƿ�֧�� DSP ״̬
*********************************************************************************************************/

#define LW_KERN_DSP_EN_SET(en)                              \
        do {                                                \
            if (en) {                                       \
                _K_ulKernFlags |= LW_KERN_FLAG_INT_DSP;     \
            } else {                                        \
                _K_ulKernFlags &= ~LW_KERN_FLAG_INT_DSP;    \
            }                                               \
        } while (0)
#define LW_KERN_DSP_EN_GET()        (_K_ulKernFlags & LW_KERN_FLAG_INT_DSP)

/*********************************************************************************************************
  �ں� bug ��⵽ bug ���Ƿ�����
*********************************************************************************************************/

#define LW_KERN_BUG_REBOOT_EN_SET(en)                       \
        do {                                                \
            if (en) {                                       \
                _K_ulKernFlags |= LW_KERN_FLAG_BUG_REBOOT;  \
            } else {                                        \
                _K_ulKernFlags &= ~LW_KERN_FLAG_BUG_REBOOT; \
            }                                               \
        } while (0)
#define LW_KERN_BUG_REBOOT_EN_GET() (_K_ulKernFlags & LW_KERN_FLAG_BUG_REBOOT)

/*********************************************************************************************************
  �ں� SMP ϵͳ���ٵ���
*********************************************************************************************************/

#define LW_KERN_SMP_FSCHED_EN_SET(en)                       \
        do {                                                \
            if (en) {                                       \
                _K_ulKernFlags |= LW_KERN_FLAG_SMP_FSCHED;  \
            } else {                                        \
                _K_ulKernFlags &= ~LW_KERN_FLAG_SMP_FSCHED; \
            }                                               \
        } while (0)
#define LW_KERN_SMP_FSCHED_EN_GET() (_K_ulKernFlags & LW_KERN_FLAG_SMP_FSCHED)

/*********************************************************************************************************
  �ں� SMT ϵͳ�����Ż�
*********************************************************************************************************/
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)

#define LW_KERN_SMT_BSCHED_EN_SET(en)                       \
        do {                                                \
            if (en) {                                       \
                _K_ulKernFlags |= LW_KERN_FLAG_SMT_BSCHED;  \
            } else {                                        \
                _K_ulKernFlags &= ~LW_KERN_FLAG_SMT_BSCHED; \
            }                                               \
        } while (0)
#define LW_KERN_SMT_BSCHED_EN_GET() (_K_ulKernFlags & LW_KERN_FLAG_SMT_BSCHED)

#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
/*********************************************************************************************************
  ��֧�� ITIMER_REAL, ITIMER_VIRTUAL, ITIMER_PROF
*********************************************************************************************************/

#define LW_KERN_NO_ITIMER_EN_SET(en)                        \
        do {                                                \
            if (en) {                                       \
                _K_ulKernFlags |= LW_KERN_FLAG_NO_ITIMER;   \
            } else {                                        \
                _K_ulKernFlags &= ~LW_KERN_FLAG_NO_ITIMER;  \
            }                                               \
        } while (0)
#define LW_KERN_NO_ITIMER_EN_GET()  (_K_ulKernFlags & LW_KERN_FLAG_NO_ITIMER)

/*********************************************************************************************************
  timespec ת��Ϊ tick ��ʱ���� (SIMPLE �ٶȿ쾫�Ȼ���ʧһ�� tick)
*********************************************************************************************************/

#define LW_KERN_TMCVT_SIMPLE_EN_SET(en)                         \
        do {                                                    \
            if (en) {                                           \
                _K_ulKernFlags |= LW_KERN_FLAG_TMCVT_SIMPLE;    \
            } else {                                            \
                _K_ulKernFlags &= ~LW_KERN_FLAG_TMCVT_SIMPLE;   \
            }                                                   \
        } while (0)
#define LW_KERN_TMCVT_SIMPLE_EN_GET()  (_K_ulKernFlags & LW_KERN_FLAG_TMCVT_SIMPLE)

/*********************************************************************************************************
  ������ FIFO �ȴ� (Ĭ��Ϊ PRIO �ȴ�)
*********************************************************************************************************/

#define LW_KERN_NET_LOCK_FIFO_SET(en)                           \
        do {                                                    \
            if (en) {                                           \
                _K_ulKernFlags |= LW_KERN_FLAG_NET_LOCK_FIFO;   \
            } else {                                            \
                _K_ulKernFlags &= ~LW_KERN_FLAG_NET_LOCK_FIFO;  \
            }                                                   \
        } while (0)
#define LW_KERN_NET_LOCK_FIFO_GET()  (_K_ulKernFlags & LW_KERN_FLAG_NET_LOCK_FIFO)

/*********************************************************************************************************
  �� detach ����ɾ����, �Ƿ񲻵ȴ� join �Զ�ɾ�� TCB (Ĭ��Ϊ false ���� POSIX ��׼)
*********************************************************************************************************/

#define LW_KERN_AUTO_REC_TCB_SET(en)                            \
        do {                                                    \
            if (en) {                                           \
                _K_ulKernFlags |= LW_KERN_FLAG_AUTO_REC_TCB;    \
            } else {                                            \
                _K_ulKernFlags &= ~LW_KERN_FLAG_AUTO_REC_TCB;   \
            }                                                   \
        } while (0)
#define LW_KERN_AUTO_REC_TCB_GET()  (_K_ulKernFlags & LW_KERN_FLAG_AUTO_REC_TCB)

/*********************************************************************************************************
  �ں˴����ֻ��
*********************************************************************************************************/

#define LW_KERN_TEXT_RO_SET(en)                             \
        do {                                                \
            if (en) {                                       \
                _K_ulKernFlags |= LW_KERN_FLAG_TEXT_RO;     \
            } else {                                        \
                _K_ulKernFlags &= ~LW_KERN_FLAG_TEXT_RO;    \
            }                                               \
        } while (0)
#define LW_KERN_TEXT_RO_GET()   (_K_ulKernFlags & LW_KERN_FLAG_TEXT_RO)

/*********************************************************************************************************
  �ں�����ʱ�Ƿ񱣳� VMM ʹ��
*********************************************************************************************************/

#define LW_KERN_REBOOT_VMM_EN_SET(en)                           \
        do {                                                    \
            if (en) {                                           \
                _K_ulKernFlags |= LW_KERN_FLAG_REBOOT_VMM_EN;   \
            } else {                                            \
                _K_ulKernFlags &= ~LW_KERN_FLAG_REBOOT_VMM_EN;  \
            }                                                   \
        } while (0)
#define LW_KERN_REBOOT_VMM_EN_GET()     (_K_ulKernFlags & LW_KERN_FLAG_REBOOT_VMM_EN)

/*********************************************************************************************************
  �ں�����ʱ�Ƿ񱣳� CACHE ʹ��
*********************************************************************************************************/

#define LW_KERN_REBOOT_CACHE_EN_SET(en)                         \
        do {                                                    \
            if (en) {                                           \
                _K_ulKernFlags |= LW_KERN_FLAG_REBOOT_CACHE_EN; \
            } else {                                            \
                _K_ulKernFlags &= ~LW_KERN_FLAG_REBOOT_CACHE_EN;\
            }                                                   \
        } while (0)
#define LW_KERN_REBOOT_CACHE_EN_GET()   (_K_ulKernFlags & LW_KERN_FLAG_REBOOT_CACHE_EN)

/*********************************************************************************************************
  ϵͳ״̬
*********************************************************************************************************/

#define LW_SYS_STATUS_SET(status)   (_K_ucSysStatus = status)
#define LW_SYS_STATUS_GET()         (_K_ucSysStatus)

#define LW_SYS_STATUS_INIT          0
#define LW_SYS_STATUS_RUNNING       1

#define LW_SYS_STATUS_IS_INIT()     (_K_ucSysStatus == LW_SYS_STATUS_INIT)
#define LW_SYS_STATUS_IS_RUNNING()  (_K_ucSysStatus == LW_SYS_STATUS_RUNNING)

/*********************************************************************************************************
  �ж�������
*********************************************************************************************************/

#define LW_IVEC_GET_IDESC(vector)       \
        (&_K_idescTable[vector])

#define LW_IVEC_GET_FLAG(vector)        \
        (_K_idescTable[vector].IDESC_ulFlag)
        
#define LW_IVEC_SET_FLAG(vector, flag)  \
        (_K_idescTable[vector].IDESC_ulFlag = (flag))

/*********************************************************************************************************
  ������
*********************************************************************************************************/

#if LW_CFG_CDUMP_EN > 0
#define LW_KERNEL_HEAP_MIN_SIZE     (LW_CFG_KB_SIZE + LW_CFG_CDUMP_CALL_STACK_DEPTH)
#else
#define LW_KERNEL_HEAP_MIN_SIZE     LW_CFG_KB_SIZE
#endif

#define LW_SYSTEM_HEAP_MIN_SIZE     LW_CFG_KB_SIZE

/*********************************************************************************************************
  ������Ϣ�ݴ�����
*********************************************************************************************************/

#if LW_CFG_CDUMP_EN > 0
#define LW_KERNEL_HEAP_START(a)     ((PVOID)a)
#define LW_KERNEL_HEAP_SIZE(s)      ((size_t)s - LW_CFG_CDUMP_BUF_SIZE)
#define LW_KERNEL_CDUMP_START(a, s) ((PVOID)((addr_t)a + LW_KERNEL_HEAP_SIZE(s)))
#define LW_KERNEL_CDUMP_SIZE(s)     LW_CFG_CDUMP_BUF_SIZE
#else
#define LW_KERNEL_HEAP_START(a)     ((PVOID)a)
#define LW_KERNEL_HEAP_SIZE(s)      ((size_t)s)
#endif

/*********************************************************************************************************
  ���Եȼ�����
*********************************************************************************************************/

#define __LOGMESSAGE_LEVEL          0x1                                 /*  ϵͳ����״̬��Ϣ            */
#define __ERRORMESSAGE_LEVEL        0x2                                 /*  ϵͳ������Ϣ                */
#define __BUGMESSAGE_LEVEL          0x4                                 /*  ����ϵͳ BUG ��Ϣ           */
#define __PRINTMESSAGE_LEVEL        0x8                                 /*  ֱ�Ӵ�ӡ�����Ϣ            */
#define __ALL_LEVEL                 0xf                                 /*  ��������                    */

/*********************************************************************************************************
  CONST posix
*********************************************************************************************************/

#define PX_ROOT                     '/'
#define PX_STR_ROOT                 "/"
#define PX_DIVIDER                  PX_ROOT
#define PX_STR_DIVIDER              PX_STR_ROOT

#endif                                                                  /*  __K_CONST_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
