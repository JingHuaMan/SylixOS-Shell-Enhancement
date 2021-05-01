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
** ��   ��   ��: InterShow.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 03 �� 04 ��
**
** ��        ��: ��ӡ����ϵͳ�ж���������Ϣ, (��ӡ����׼����ն���)

** BUG:
2009.06.25  ������ж�״̬�Ĵ�ӡ.
2009.12.11  ����ӡ��Ч���ж�.
2010.02.24  �޸Ĵ�ӡ�ַ�.
2011.03.31  ֧�� inter queue ���ʹ�ӡ.
2013.12.12  ����Ϊ�µ������ж�ϵͳ.
2014.05.09  ������ж������Ĵ�ӡ.
2015.08.19  �����ӡ CPU �ŷ�Χ����.
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
  ��ʾ�ж�������Ĺ�����, ����ִ��ɾ������
*********************************************************************************************************/
extern LW_OBJECT_HANDLE    _K_ulInterShowLock;
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define INTER_SHOWLOCK_LOCK()       \
        API_SemaphoreMPend(_K_ulInterShowLock, LW_OPTION_WAIT_INFINITE)
#define INTER_SHOWLOCK_UNLOCK()     \
        API_SemaphoreMPost(_K_ulInterShowLock)
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT == 64
static const CHAR   _G_cInterInfoHdr1[] = "\n\
 IRQ      NAME            ENTRY            CLEAR      ENABLE RND PREEMPT PRIO";
static const CHAR   _G_cInterInfoHdr2[] = "\n\
---- -------------- ---------------- ---------------- ------ --- ------- ----";
#else
static const CHAR   _G_cInterInfoHdr1[] = "\n\
 IRQ      NAME       ENTRY    CLEAR   ENABLE RND PREEMPT PRIO";
static const CHAR   _G_cInterInfoHdr2[] = "\n\
---- -------------- -------- -------- ------ --- ------- ----";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */

#if LW_CFG_INTER_INFO > 0
static const CHAR   _G_cNestingInfoHdr[] = "\n\
 CPU  MAX NESTING      IPI\n\
----- ----------- -------------\n";
#endif                                                                  /*  LW_CFG_INTER_INFO > 0       */
/*********************************************************************************************************
** ��������: API_InterShow
** ��������: ��ʾ�ж����������������
** �䡡��  : ulCPUStart        ��Ҫ��ʾ��ϸ��Ϣ����ʼ CPU ��
**           ulCPUEnd          ��Ҫ��ʾ��ϸ��Ϣ�Ľ��� CPU ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
VOID   API_InterShow (ULONG  ulCPUStart, ULONG  ulCPUEnd)
{
    ULONG      i, j;
    BOOL       bIsEnable = LW_FALSE;
    PCHAR      pcIsEnable;
    PCHAR      pcRnd;
    PCHAR      pcPreem;
    ULONG      ulFlag;
    UINT       uiPrio = 0;
    INT        iRet;
    
    PLW_CLASS_INTDESC  pidesc;
    PLW_CLASS_INTACT   piaction;
    PLW_LIST_LINE      plineTemp;
    
    if ((ulCPUStart >= LW_NCPUS) || 
        (ulCPUEnd   >= LW_NCPUS) || 
        (ulCPUStart > ulCPUEnd)) {
        printf("CPU range error.>>\n");
        return;
    }
        
    printf("interrupt vector show >>\n");
    printf(_G_cInterInfoHdr1);                                          /*  ��ӡ��ӭ��Ϣ                */
    for (i = ulCPUStart; i <= ulCPUEnd; i++) {
        printf("     CPU%2ld    ", i);
    }
    
    printf(_G_cInterInfoHdr2);
    for (i = ulCPUStart; i <= ulCPUEnd; i++) {
        printf(" -------------");
    }
    printf("\n");
    
    if (_K_ulInterShowLock == LW_OBJECT_HANDLE_INVALID) {
        return;                                                         /*  ��û�������κ��ж�����      */
    }
    
    INTER_SHOWLOCK_LOCK();
    
    for (i = 0; i < LW_CFG_MAX_INTER_SRC; i++) {
        API_InterVectorGetFlag((ULONG)i, &ulFlag);
        API_InterVectorIsEnable((ULONG)i, &bIsEnable);
        iRet = API_InterVectorGetPriority((ULONG)i, &uiPrio);
        
        pcIsEnable = (bIsEnable)                        ? "true" : "false";
        pcRnd      = (ulFlag & LW_IRQ_FLAG_SAMPLE_RAND) ? "yes"  : "";
        pcPreem    = (ulFlag & LW_IRQ_FLAG_PREEMPTIVE)  ? "yes"  : "";
        
        pidesc = LW_IVEC_GET_IDESC(i);
        
        for (plineTemp  = pidesc->IDESC_plineAction;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            piaction = _LIST_ENTRY(plineTemp, LW_CLASS_INTACT, IACT_plineManage);

#if LW_CFG_CPU_WORD_LENGHT == 64
            printf("%4ld %-14s %16lx %16lx %-6s %-3s %-7s ",
#else
            printf("%4ld %-14s %8lx %8lx %-6s %-3s %-7s ",
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
                   i, 
                   piaction->IACT_cInterName, 
                   (ULONG)piaction->IACT_pfuncIsr, 
                   (ULONG)piaction->IACT_pfuncClear, 
                   pcIsEnable, 
                   pcRnd, 
                   pcPreem);

            if (iRet) {                                                 /*  ��ӡ���ȼ�                  */
                printf(" N/A ");
            } else {
                printf("%4d ", uiPrio);
            }

            for (j = ulCPUStart; j <= ulCPUEnd; j++) {                  /*  ��ӡ�жϼ���                */
                printf("%13lld ", piaction->IACT_iIntCnt[j]);
            }
            printf("\n");
        }
    }
    
    INTER_SHOWLOCK_UNLOCK();
    
    printf("\n");
#if LW_CFG_INTER_INFO > 0
    printf("interrupt nesting show >>\n");
    printf(_G_cNestingInfoHdr);                                         /*  ��ӡ��ӭ��Ϣ                */
    
    LW_CPU_FOREACH (i) {
#if LW_CFG_SMP_EN > 0
        printf("%5ld %11ld %13lld\n", i, LW_CPU_GET_NESTING_MAX(i), LW_CPU_GET_IPI_CNT(i));
#else
        printf("%5ld %11ld Not SMP\n", i, LW_CPU_GET_NESTING_MAX(i));
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    }
    
    printf("\n");
#endif                                                                  /*  LW_CFG_INTER_INFO > 0       */
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
