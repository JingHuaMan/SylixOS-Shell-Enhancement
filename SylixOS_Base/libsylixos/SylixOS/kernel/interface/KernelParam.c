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
** ��   ��   ��: KernelParam.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 08 �� 08 ��
**
** ��        ��: ����ϵͳ�ں��������������ļ���
**
** BUG:
2014.09.09  ncpus ȡֵ��ΧΪ [1 ~ LW_CFG_MAX_PROCESSORS].
2017.08.13  ����Գ�ʼ״̬�ĳ�ʼ��.
*********************************************************************************************************/
#define  __KERNEL_NCPUS_SET
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ں˲���
*********************************************************************************************************/
static CHAR         _K_cKernelStartParam[1024];
#if LW_CFG_DEVICE_EN > 0
extern LW_API INT   API_RootFsMapInit(CPCHAR  pcMap);
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
/*********************************************************************************************************
** ��������: API_KernelStartParam
** ��������: ϵͳ�ں���������
** �䡡��  : pcParam       ��������, ���Կո�ֿ���һ���ַ����б�ͨ������������ʽ:
                           ncpus=1          CPU ���� (x86/64 ƽ̨���Բ�����, ����ϵͳ���Զ�̽��)
                           dlog=no          DEBUG LOG ��Ϣ��ӡ
                           derror=yes       DEBUG ERROR ��Ϣ��ӡ
                           kfpu=no          �ں�̬��Ӳ����Э������֧�� (�Ƽ�Ϊ no)
                           kdsp=no          �ں�̬�� DSP Э������֧�� (�Ƽ�Ϊ no)
                           heapchk=yes      �ڴ��Խ����
                           hz=100           ϵͳ tick Ƶ��, Ĭ��Ϊ 100 (�Ƽ� 100 ~ 10000 �м�)
                           hhz=100          ���ٶ�ʱ��Ƶ��, Ĭ���� hz ��ͬ (�� BSP ֧��)
                           irate=5          Ӧ�ö�ʱ���ֱ���, Ĭ��Ϊ 5 �� tick. (�Ƽ� 1 ~ 10 �м�)
                           hpsec=1          �Ȳ��ѭ�������ʱ��, ��λ: �� (�Ƽ� 1 ~ 5 ��)
                           bugreboot=yes    �ں�̽�⵽ bug ʱ�Ƿ��Զ�����.
                           rebootto=10      ������ʱʱ��.
                           fsched=no        SMP ϵͳ�ں˿��ٵ���
                           smt=no           SMT �������
                           sldepcache=no    spin lock ������ cache ʹ��. (ARM)
                           noitmr=no        ��֧�� ITIMER_REAL/ITIMER_VIRTUAL/ITIMER_PROF,
                                            �����˶����Ƶȸ�ʵʱ��Ӧ��, ����Ϊ yes ��� tick �ٶ�
                           tmcvtsimple=no   ͨ�� timespec ת�� tick ��ʱ, �Ƿ�ʹ�ü�ת����.
                                            ���� Lite ���ʹ������ɲ��� simple ת����.
                           netlockfifo=no   ���绥����ʹ�� FIFO ˳��ȴ�.
                           autorectcb=no    POSIX �� detach �߳�ɾ����, �Ƿ񲻵ȴ� join �� detach �Զ�ɾ�� TCB
                           textro=no        Kernel TEXT ��ֻ��

                           rfsmap=/boot:[*],/:[*],...   ���Ǹ��ļ�ϵͳӳ���ϵѡ��, �ö��Ÿ���, 
                                                        /boot /etc /tmp /apps ... Ϊ��ѡӳ��, 
                                                        / Ϊ����ӳ��.
                           
                                            ���� /boot:/media/hdd0 ��ʾ�� /boot Ŀ¼ӳ�䵽 /media/hdd0
                                                 /apps:/media/hdd2 ��ʾ�� /apps Ŀ¼ӳ�䵽 /media/hdd2
                                                 /:/media/hdd1 ��ʾ����Ŀ¼����ӳ�䵽 /media/hdd1
                                                 /:/dev/ram    ��ʾ����Ŀ¼����ӳ�䵽 ramfs ��.
                                                 
                                                 ע��: /dev/ram ����ֻ��ʹ���� /: ӳ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                       API ����
*********************************************************************************************************/
LW_API
ULONG  API_KernelStartParam (CPCHAR  pcParam)
{
    CHAR        cParamBuffer[1024];                                     /*  �������Ȳ��ó��� 1024 �ֽ�  */
    PCHAR       pcDelim = " ";
    PCHAR       pcLast;
    PCHAR       pcTok;
    
    if (LW_SYS_STATUS_IS_RUNNING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel is already start.\r\n");
        _ErrorHandle(ERROR_KERNEL_RUNNING);
        return  (ERROR_KERNEL_RUNNING);
    }
    
    LW_KERN_BUG_REBOOT_EN_SET(LW_TRUE);                                 /*  ��ʼ״̬                    */

    lib_strlcpy(cParamBuffer,         pcParam, sizeof(cParamBuffer));
    lib_strlcpy(_K_cKernelStartParam, pcParam, sizeof(_K_cKernelStartParam));

    pcTok = lib_strtok_r(cParamBuffer, pcDelim, &pcLast);
    while (pcTok) {
        if (lib_strncmp(pcTok, "ncpus=", 6) == 0) {                     /*  CPU ����                    */
            INT     iCpus = lib_atoi(&pcTok[6]);
            if (iCpus > 0) {
                if (iCpus > LW_CFG_MAX_PROCESSORS) {
                    _K_ulNCpus = LW_CFG_MAX_PROCESSORS;
                } else {
                    _K_ulNCpus = (ULONG)iCpus;
                }
            }
            
#if LW_CFG_LOGMESSAGE_EN > 0
        } else if (lib_strncmp(pcTok, "kdlog=", 6) == 0) {              /*  �Ƿ�ʹ���ں� log ��ӡ       */
            if (pcTok[6] == 'n') {
                _K_pfuncKernelDebugLog = LW_NULL;
            } else {
                _K_pfuncKernelDebugLog = bspDebugMsg;
            }
#endif                                                                  /*  LW_CFG_LOGMESSAGE_EN > 0    */

#if LW_CFG_ERRORMESSAGE_EN > 0
        } else if (lib_strncmp(pcTok, "kderror=", 8) == 0) {            /*  �Ƿ�ʹ���ں˴����ӡ        */
            if (pcTok[8] == 'n') {
                _K_pfuncKernelDebugError = LW_NULL;
            } else {
                _K_pfuncKernelDebugError = bspDebugMsg;
            }
#endif                                                                  /*  LW_CFG_ERRORMESSAGE_EN > 0  */
        
        } else if (lib_strncmp(pcTok, "kfpu=", 5) == 0) {               /*  �Ƿ�ʹ���ں˸���֧��        */
            if (pcTok[5] == 'n') {
                LW_KERN_FPU_EN_SET(LW_FALSE);
            } else {
#if LW_CFG_INTER_FPU == 0
                _BugHandle(LW_TRUE, LW_TRUE, 
                           "Please configure LW_CFG_INTER_FPU with 1 in kernel_cfg.h\r\n");
#else
#ifdef LW_CFG_CPU_ARCH_MIPS                                             /*  MIPS ƽ̨������ kfpu ����   */
                _BugHandle(LW_TRUE, LW_TRUE, 
                           "SylixOS do not support kfpu on MIPS!\r\n");
#else
                LW_KERN_FPU_EN_SET(LW_TRUE);
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */
#endif                                                                  /*  LW_CFG_INTER_FPU > 0        */
            }

        } else if (lib_strncmp(pcTok, "kdsp=", 5) == 0) {               /*  �Ƿ�ʹ���ں� DSP ֧��       */
            if (pcTok[5] == 'n') {
                LW_KERN_DSP_EN_SET(LW_FALSE);
            } else {
#if LW_CFG_INTER_DSP == 0
                _BugHandle(LW_TRUE, LW_TRUE,
                           "Please configure LW_CFG_INTER_DSP with 1 in kernel_cfg.h\r\n");
#else
                LW_KERN_DSP_EN_SET(LW_TRUE);
#endif                                                                  /*  LW_CFG_INTER_DSP > 0        */
            }

        } else if (lib_strncmp(pcTok, "bugreboot=", 10) == 0) {         /*  ̽�⵽ bug ʱ�Ƿ��Զ�����   */
            if (pcTok[10] == 'n') {
                LW_KERN_BUG_REBOOT_EN_SET(LW_FALSE);
            } else {
                LW_KERN_BUG_REBOOT_EN_SET(LW_TRUE);
            }
            
        } else if (lib_strncmp(pcTok, "rebootto=", 9) == 0) {           /*  ������ʱ                    */
            INT     iToSec = lib_atoi(&pcTok[9]);
            if ((iToSec >= 0) && (iToSec < 1000)) {
                LW_REBOOT_TO_SEC = (ULONG)iToSec;
            }
            
        } else if (lib_strncmp(pcTok, "heapchk=", 8) == 0) {            /*  �Ƿ���ж��ڴ�Խ����      */
            if (pcTok[8] == 'n') {
                _K_bHeapCrossBorderEn = LW_FALSE;
            } else {
                _K_bHeapCrossBorderEn = LW_TRUE;
            }
        
        } else if (lib_strncmp(pcTok, "hz=", 3) == 0) {                 /*  tick Ƶ��                   */
            ULONG   ulHz = (ULONG)lib_atol(&pcTok[3]);
            if (ulHz >= 100 && ulHz <= 10000) {                         /*  10ms ~ 100us                */
                LW_TICK_HZ = ulHz;
                LW_NSEC_PER_TICK = __TIMEVAL_NSEC_MAX / ulHz;
            }
        
        } else if (lib_strncmp(pcTok, "hhz=", 4) == 0) {                /*  �߶ȶ�ʱ��Ƶ��              */
            ULONG   ulHz = (ULONG)lib_atol(&pcTok[4]);
            if (ulHz >= 100 && ulHz <= 100000) {                        /*  10ms ~ 10us                 */
                LW_HTIMER_HZ = ulHz;
            }
        
        } else if (lib_strncmp(pcTok, "irate=", 6) == 0) {              /*  Ӧ�ö�ʱ���ֱ���            */
            ULONG   ulRate = (ULONG)lib_atol(&pcTok[6]);
            if (ulRate >= 1 && ulRate <= 10) {                          /*  1 ~ 10 ticks                */
                LW_ITIMER_RATE = ulRate;
            }
        
        } else if (lib_strncmp(pcTok, "hpsec=", 6) == 0) {              /*  �Ȳ��ѭ���������          */
            ULONG   ulSec = (ULONG)lib_atol(&pcTok[6]);
            if (ulSec >= 1 && ulSec <= 10) {                            /*  1 ~ 10 ticks                */
                LW_HOTPLUG_SEC = ulSec;
            }
        }
        
#if LW_CFG_SMP_EN > 0
          else if (lib_strncmp(pcTok, "fsched=", 7) == 0) {             /*  SMP ���ٵ���                */
            if (pcTok[7] == 'n') {
                LW_KERN_SMP_FSCHED_EN_SET(LW_FALSE);
            } else {
                LW_KERN_SMP_FSCHED_EN_SET(LW_TRUE);
            }
        
        } 
#if LW_CFG_CPU_ARCH_SMT > 0
          else if (lib_strncmp(pcTok, "smt=", 4) == 0) {                /*  smt �������                */
            if (pcTok[4] == 'n') {
                LW_KERN_SMT_BSCHED_EN_SET(LW_FALSE);
            } else {
                LW_KERN_SMT_BSCHED_EN_SET(LW_TRUE);
            }
        }
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
          else if (lib_strncmp(pcTok, "noitmr=", 7) == 0) {             /*  ��֧�� itimer               */
            if (pcTok[7] == 'n') {
                LW_KERN_NO_ITIMER_EN_SET(LW_FALSE);
            } else {
                LW_KERN_NO_ITIMER_EN_SET(LW_TRUE);
            }

        } else if (lib_strncmp(pcTok, "tmcvtsimple=", 12) == 0) {       /*  �Ƿ�ʹ�ü򵥷���ת��        */
            if (pcTok[12] == 'n') {
                LW_KERN_TMCVT_SIMPLE_EN_SET(LW_FALSE);
            } else {
                LW_KERN_TMCVT_SIMPLE_EN_SET(LW_TRUE);
            }
        } 
        
#if LW_CFG_NET_EN > 0
          else if (lib_strncmp(pcTok, "netlockfifo=", 12) == 0) {       /*  ������ FIFO �ȴ�            */
            if (pcTok[12] == 'n') {
                LW_KERN_NET_LOCK_FIFO_SET(LW_FALSE);
            } else {
                LW_KERN_NET_LOCK_FIFO_SET(LW_TRUE);
            }
        }
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
          else if (lib_strncmp(pcTok, "autorectcb=", 11) == 0) {        /*  �Զ�ɾ�� TCB                */
            if (pcTok[11] == 'n') {
                LW_KERN_AUTO_REC_TCB_SET(LW_FALSE);
            } else {
                LW_KERN_AUTO_REC_TCB_SET(LW_TRUE);
            }

        } else if (lib_strncmp(pcTok, "textro=", 7) == 0) {             /*  �Զ�ɾ�� TCB                */
            if (pcTok[7] == 'n') {
                LW_KERN_TEXT_RO_SET(LW_FALSE);
            } else {
                LW_KERN_TEXT_RO_SET(LW_TRUE);
            }
        }

#if LW_CFG_DEVICE_EN > 0
          else if (lib_strncmp(pcTok, "rfsmap=", 7) == 0) {             /*  ���ļ�ϵͳӳ��              */
            API_RootFsMapInit(&pcTok[7]);
        }
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */

#if LW_CFG_CACHE_EN > 0
          else if (lib_strncmp(pcTok, "rebootcacheen=", 14) == 0) {     /*  ����ʱ���� CACHE ʹ��       */
              if (pcTok[14] == 'n') {
                  LW_KERN_REBOOT_CACHE_EN_SET(LW_FALSE);
              } else {
                  LW_KERN_REBOOT_CACHE_EN_SET(LW_TRUE);
              }
        }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

#if LW_CFG_VMM_EN > 0
          else if (lib_strncmp(pcTok, "rebootvmmen=", 12) == 0) {       /*  ����ʱ���� VMM ʹ��         */
              if (pcTok[12] == 'n') {
                  LW_KERN_REBOOT_VMM_EN_SET(LW_FALSE);
              } else {
                  LW_KERN_REBOOT_VMM_EN_SET(LW_TRUE);
              }
        }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

#ifdef __ARCH_KERNEL_PARAM
          else {
            __ARCH_KERNEL_PARAM(pcTok);                                 /*  ��ϵ�ṹ��ز���            */
        }
#endif                                                                  /*  __ARCH_KERNEL_PARAM         */
        
        pcTok = lib_strtok_r(LW_NULL, pcDelim, &pcLast);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_KernelStartParamGet
** ��������: ���ϵͳ�ں���������
** �䡡��  : pcParam       ��������
**           stLen         ����������
** �䡡��  : ʵ�ʳ���
** ȫ�ֱ���: 
** ����ģ��: 
                                       API ����
*********************************************************************************************************/
LW_API
ssize_t  API_KernelStartParamGet (PCHAR  pcParam, size_t  stLen)
{
    if (!pcParam || !stLen) {
        _ErrorHandle(ERROR_KERNEL_BUFFER_NULL);
        return  (0);
    }

    return  ((ssize_t)lib_strlcpy(pcParam, _K_cKernelStartParam, stLen));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
