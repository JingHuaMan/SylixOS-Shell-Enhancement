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
** ��   ��   ��: CoroutineStackCheck.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 06 �� 19 ��
**
** ��        ��: ����Э�̹����(Э����һ���������Ĳ���ִ�е�λ). 
                 ���һ��Э�̵Ķ�ջʹ����!!, ��Э�̵ĸ�ϵ�߳̽���ʱ����ʹ�� STK_CHK ѡ��.
** BUG:
2009.04.06  ʹ���µĿ����ñ�־���жϿ��ж�ջ��, �������׼ȷ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_COROUTINE_EN > 0
/*********************************************************************************************************
** ��������: API_CoroutineStackCheck
** ��������: ���һ��Э�̵Ķ�ջʹ����!!, ��Э�̵ĸ�ϵ�߳̽���ʱ����ʹ�� STK_CHK ѡ��.
** �䡡��  : pvCrcb                        Э�̾��
**           pstFreeByteSize               ���ж�ջ��С   (��Ϊ LW_NULL)
**           pstUsedByteSize               ʹ�ö�ջ��С   (��Ϊ LW_NULL)
**           pstCrcbByteSize               Э�̿��ƿ��С (��Ϊ LW_NULL)
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG   API_CoroutineStackCheck (PVOID      pvCrcb,
                                 size_t    *pstFreeByteSize,
                                 size_t    *pstUsedByteSize,
                                 size_t    *pstCrcbByteSize)
{
    REGISTER PLW_CLASS_COROUTINE   pcrcb = (PLW_CLASS_COROUTINE)pvCrcb;
    REGISTER size_t                stTotal;
    REGISTER size_t                stFree = 0;
    
    REGISTER PLW_STACK             pstkButtom;

    if (!pcrcb) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine handle invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    stTotal = pcrcb->COROUTINE_stStackSize;                             /*  �ܴ�С                      */
    
#if CPU_STK_GROWTH == 0                                                 /*  Ѱ�Ҷ�ջͷβ                */
    for (pstkButtom = pcrcb->COROUTINE_pstkStackBottom;
         *pstkButtom == _K_stkFreeFlag;
         pstkButtom--,
         stFree++);
#else
    for (pstkButtom = pcrcb->COROUTINE_pstkStackBottom;
         *pstkButtom == _K_stkFreeFlag;
         pstkButtom++,
         stFree++);
#endif
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (pstFreeByteSize) {
        *pstFreeByteSize = stFree * sizeof(LW_STACK);
    }
    
    if (pstUsedByteSize) {
        *pstUsedByteSize = (stTotal - stFree) * sizeof(LW_STACK);
    }
    
    if (pstCrcbByteSize) {
        *pstCrcbByteSize = sizeof(LW_CLASS_COROUTINE);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
