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
** ��   ��   ��: KernelDsp.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 01 �� 10 ��
**
** ��        ��: ��ʼ���ں� DSP.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_KernelDspPrimaryInit
** ��������: ��ʼ�� DSP (������ bsp ��ʼ���ص��е���)
** �䡡��  : pcMachineName ʹ�õĴ���������
**           pcDspName     ʹ�õ� DSP ����.
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_CPU_DSP_EN > 0

LW_API
VOID  API_KernelDspPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcDspName)
{
#if LW_CFG_INTER_DSP > 0
    INT     i, j;
#endif                                                                  /*  LW_CFG_INTER_DSP > 0        */
    
    if (LW_SYS_STATUS_GET()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel already running.\r\n");
        return;
    }

    archDspPrimaryInit(pcMachineName, pcDspName);                       /*  ��ʼ�� DSP ��Ԫ             */
    
#if LW_CFG_INTER_DSP > 0
    if (LW_KERN_DSP_EN_GET()) {                                         /*  �ж�״̬����ʹ�� DSP        */
        __ARCH_DSP_ENABLE();                                            /*  ������Ҫ�ڵ�ǰ DSP �������� */
                                                                        /*  ʹ�� DSP, ����ϵͳ�ں���Ҫ  */
    } else 
#endif                                                                  /*  LW_CFG_INTER_DSP > 0        */
    {
        __ARCH_DSP_DISABLE();
    }
    
#if LW_CFG_INTER_DSP > 0
    for (i = 0; i < LW_CFG_MAX_PROCESSORS; i++) {
        for (j = 0; j < LW_CFG_MAX_INTER_SRC; j++) {                    /*  ��ʼ��, ���ǲ�ʹ�� DSP      */
            __ARCH_DSP_CTX_INIT((PVOID)&(LW_CPU_GET(i)->CPU_dspctxContext[j]));
        }
    }
#endif                                                                  /*  LW_CFG_INTER_DSP > 0        */
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "DSP initilaized.\r\n");
}
/*********************************************************************************************************
** ��������: API_KernelDspSecondaryInit
** ��������: ��ʼ�� DSP (������ bsp ��ʼ���ص��е���)
** �䡡��  : pcMachineName ʹ�õĴ���������
**           pcDspName     ʹ�õ� DSP ����.
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

LW_API
VOID  API_KernelDspSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcDspName)
{
    archDspSecondaryInit(pcMachineName, pcDspName);
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "secondary DSP initilaized.\r\n");
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
