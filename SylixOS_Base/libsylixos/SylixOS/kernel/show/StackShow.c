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
** ��   ��   ��: StackShow.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 04 �� 01 ��
**
** ��        ��: ��ʾ���е��̵߳Ķ�ջ��Ϣ.

** BUG
2008.04.27  ulFree �� ulUsed ��ʾ����.
2009.04.07  ����ʹ�ðٷֱ���.
2009.04.11  ���� SMP ��Ϣ.
2012.12.11  tid ��ʾ 7 λ�Ϳ�����
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static const CHAR   _G_cThreadStackInfoHdr[] = "\n\
       NAME        TID   PRI STK USE  STK FREE USED\n\
---------------- ------- --- -------- -------- ----\n";
static const CHAR   _G_cCPUStackInfoHdr[] = "\n\
CPU STK USE  STK FREE USED\n\
--- -------- -------- ----\n";
/*********************************************************************************************************
** ��������: API_StackShow
** ��������: ��ʾ���е��̵߳� CPU ռ������Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
VOID    API_StackShow (VOID)
{
    REGISTER INT              i;
    REGISTER PLW_CLASS_TCB    ptcb;
    
             size_t           stFreeByteSize;
             size_t           stUsedByteSize;
             ULONG            ulError;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    printf("thread stack usage show >>\n");
    printf(_G_cThreadStackInfoHdr);                                     /*  ��ӡ��ӭ��Ϣ                */
    
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
        ptcb = _K_ptcbTCBIdTable[i];                                    /*  ��� TCB ���ƿ�             */
        if (ptcb == LW_NULL) {                                          /*  �̲߳�����                  */
            continue;
        }
        
        ulError = API_ThreadStackCheck(ptcb->TCB_ulId, 
                                       &stFreeByteSize,
                                       &stUsedByteSize,
                                       LW_NULL);
        if (ulError) {
            continue;                                                   /*  ��������                    */
        }
        
        printf("%-16s %7lx %3d %8zd %8zd %3zd%%\n",
                      ptcb->TCB_cThreadName,                            /*  �߳���                      */
                      ptcb->TCB_ulId,                                   /*  �̴߳������                */
                      ptcb->TCB_ucPriority,                             /*  ���ȼ�                      */
                      stUsedByteSize,
                      stFreeByteSize,
                      ((stUsedByteSize * 100) / (stUsedByteSize + stFreeByteSize)));
    }
    
    printf("\ninterrupt stack usage show >>\n");
    printf(_G_cCPUStackInfoHdr);                                        /*  ��ӡ��ӭ��Ϣ                */
    
    LW_CPU_FOREACH (i) {                                                /*  ��ӡ���� CPU ���ж�ջ���   */
        API_InterStackCheck((ULONG)i,
                            &stFreeByteSize,
                            &stUsedByteSize);
        printf("%3d %8zd %8zd %3zd%%\n",
                      i, stUsedByteSize, stFreeByteSize, 
                      ((stUsedByteSize * 100) / (stUsedByteSize + stFreeByteSize)));
    }
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
