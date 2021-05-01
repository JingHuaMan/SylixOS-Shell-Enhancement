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
** ��   ��   ��: InterVectorIsr.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 02 ��
**
** ��        ��: �����ж��ܷ���

** BUG
2007.06.06  �������Ƕ����Ϣ��¼���ܡ�
2010.08.03  ʹ�� interrupt vector spinlock ��Ϊ�����.
2011.03.31  ���� vector queue �����ж�����֧��.
2013.07.19  ����˼��жϴ����֧.
2013.08.28  �����ں��¼�������.
2014.04.21  �жϱ�������ڱ���������жϷ�����.
2014.05.09  ������жϼ������Ĵ���.
2014.05.09  ��� SMP �жϴ����ٶ�.
2016.04.14  ֧�� GJB7714 �޷���ֵ�ж�.
2018.07.30  ֻ�� QUEUE �����ж�������Ҫ spinlock ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �жϲ����ص�
*********************************************************************************************************/
#if LW_CFG_INTER_MEASURE_HOOK_EN > 0
extern VOIDFUNCPTR  _K_pfuncInterVectorMeasureEnter;
extern VOIDFUNCPTR  _K_pfuncInterVectorMeasureExit;
#define INTER_VECTOR_MEASURE_ENTER(a, b, c, d)                  \
        if (_K_pfuncInterVectorMeasureEnter) {                  \
            LW_SOFUNC_PREPARE(_K_pfuncInterVectorMeasureEnter); \
            _K_pfuncInterVectorMeasureEnter(a, b, c, d);        \
        }
#define INTER_VECTOR_MEASURE_EXIT(a, b, c, d)                   \
        if (_K_pfuncInterVectorMeasureExit) {                   \
            LW_SOFUNC_PREPARE(_K_pfuncInterVectorMeasureExit);  \
            _K_pfuncInterVectorMeasureExit(a, b, c, d);         \
        }
#else
#define INTER_VECTOR_MEASURE_ENTER(a, b, c, d)
#define INTER_VECTOR_MEASURE_EXIT(a, b, c, d)
#endif                                                                  /*  LW_CFG_INTER_MEASURE_HOOK_EN*/
/*********************************************************************************************************
  �жϷ���
*********************************************************************************************************/
#if LW_CFG_GJB7714_INT_EN > 0
#define INTER_VECTOR_SVC_HANDLE()                                                                   \
    if (pidesc->IDESC_ulFlag & LW_IRQ_FLAG_GJB7714) {                                               \
        piaction->IACT_pfuncIsr(piaction->IACT_pvArg, ulVector);                                    \
        irqret = LW_IRQ_NONE;                                                                       \
    } else {                                                                                        \
        irqret = piaction->IACT_pfuncIsr(piaction->IACT_pvArg, ulVector);                           \
    }
#else
#define INTER_VECTOR_SVC_HANDLE()                                                                   \
    irqret = piaction->IACT_pfuncIsr(piaction->IACT_pvArg, ulVector);
#endif

#define INTER_VECTOR_SVC(BREAK)                                                                     \
{                                                                                                   \
    INTER_VECTOR_MEASURE_ENTER(&tv, ulVector, LW_CPU_GET_ID(pcpu), piaction->IACT_cInterName);      \
    INTER_VECTOR_SVC_HANDLE();                                                                      \
    if (LW_IRQ_RETVAL(irqret)) {                                                                    \
        INTER_VECTOR_MEASURE_EXIT(&tv, ulVector, LW_CPU_GET_ID(pcpu), piaction->IACT_cInterName);   \
        piaction->IACT_iIntCnt[LW_CPU_GET_ID(pcpu)]++;                                              \
        if (piaction->IACT_pfuncClear) {                                                            \
            piaction->IACT_pfuncClear(piaction->IACT_pvArg, ulVector);                              \
        }                                                                                           \
        if (LW_IRQ_RETBREAK(irqret)) {                                                              \
            BREAK                                                                                   \
        }                                                                                           \
    }                                                                                               \
}
/*********************************************************************************************************
** ��������: API_InterVectorIsr
** ��������: �����ж��ܷ���
** �䡡��  : ulVector                      �ж������� (arch �㺯����Ҫ��֤�˲�����ȷ)
** �䡡��  : �жϷ���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲢ�������ж�Ƕ��, ����Ҫ arch ����ֲ����֧��.
                                           API ����
*********************************************************************************************************/
LW_API
irqreturn_t  API_InterVectorIsr (ULONG  ulVector)
{
    PLW_CLASS_CPU       pcpu;
    PLW_LIST_LINE       plineTemp;
    PLW_CLASS_INTDESC   pidesc;
    PLW_CLASS_INTACT    piaction;
    irqreturn_t         irqret = LW_IRQ_NONE;
    
#if LW_CFG_INTER_MEASURE_HOOK_EN > 0
    struct timespec     tv;
#endif
           
    pcpu = LW_CPU_GET_CUR();                                            /*  �жϴ��������, ����ı� CPU*/
    
#if LW_CFG_CPU_INT_HOOK_EN > 0
    __LW_CPU_INT_ENTER_HOOK(ulVector, pcpu->CPU_ulInterNesting);
#endif                                                                  /*  LW_CFG_CPU_INT_HOOK_EN > 0  */
    
#if LW_CFG_SMP_EN > 0
    if (pcpu->CPU_ulIPIVector == ulVector) {                            /*  �˼��ж�                    */
        _SmpProcIpi(pcpu);
        if (pcpu->CPU_pfuncIPIClear) {
            pcpu->CPU_pfuncIPIClear(pcpu->CPU_pvIPIArg, ulVector);      /*  ����˼��ж�                */
        }
    } else
#endif                                                                  /*  LW_CFG_SMP_EN               */
    {
        pidesc = LW_IVEC_GET_IDESC(ulVector);
        if (pidesc->IDESC_ulFlag & LW_IRQ_FLAG_QUEUE) {
#if LW_CFG_SMP_EN > 0
            LW_SPIN_LOCK(&pidesc->IDESC_slLock);                        /*  ��ס spinlock               */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
            for (plineTemp  = pidesc->IDESC_plineAction;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                piaction = (PLW_CLASS_INTACT)plineTemp;
                INTER_VECTOR_SVC(break;);
            }

#if LW_CFG_SMP_EN > 0
            LW_SPIN_UNLOCK(&pidesc->IDESC_slLock);                      /*  ���� spinlock               */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
        } else {
            piaction = (PLW_CLASS_INTACT)pidesc->IDESC_plineAction;
            if (piaction) {
                INTER_VECTOR_SVC(;);
            
            } else {
                _DebugFormat(__ERRORMESSAGE_LEVEL, "interrupt vector: %ld no service.\r\n", ulVector);
            }
        }
    }
    
#if LW_CFG_CPU_INT_HOOK_EN > 0
    __LW_CPU_INT_EXIT_HOOK(ulVector, pcpu->CPU_ulInterNesting);
#endif                                                                  /*  LW_CFG_CPU_INT_HOOK_EN > 0  */
                      
    return  (irqret);
}
/*********************************************************************************************************
** ��������: API_InterVectorIpiEx
** ��������: ���ú˼��ж�����   
             BSP ��ϵͳ����ǰ���ô˺���, ÿ�� CPU ��Ҫ����, SylixOS ����ͬ�� CPU �˼��ж�������ͬ.
** �䡡��  : 
**           ulCPUId                       CPU ID
**           ulIPIVector                   �˼��ж�������
**           pfuncClear                    �˼��ж��������
**           pvArg                         �˼��жϲ���
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

LW_API
VOID  API_InterVectorIpiEx (ULONG  ulCPUId, ULONG  ulIPIVector, FUNCPTR  pfuncClear, PVOID  pvArg)
{
    PLW_CLASS_CPU   pcpu;
    
    if (ulCPUId < LW_CFG_MAX_PROCESSORS) {
        pcpu = LW_CPU_GET(ulCPUId);
        pcpu->CPU_ulIPIVector   = ulIPIVector;
        pcpu->CPU_pfuncIPIClear = pfuncClear;
        pcpu->CPU_pvIPIArg      = pvArg;
    }
}
/*********************************************************************************************************
** ��������: API_InterVectorIpi
** ��������: ���ú˼��ж�����   
             BSP ��ϵͳ����ǰ���ô˺���, ÿ�� CPU ��Ҫ����, SylixOS ����ͬ�� CPU �˼��ж�������ͬ.
** �䡡��  : 
**           ulCPUId                       CPU ID
**           ulIPIVector                   �˼��ж�������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_InterVectorIpi (ULONG  ulCPUId, ULONG  ulIPIVector)
{
    API_InterVectorIpiEx(ulCPUId, ulIPIVector, LW_NULL, LW_NULL);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
