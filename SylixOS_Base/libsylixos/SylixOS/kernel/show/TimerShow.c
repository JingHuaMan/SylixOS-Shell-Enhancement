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
** ��   ��   ��: TimerShow.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 06 �� 03 ��
**
** ��        ��: ��ʱ���������Ϣ (��ӡ���ն���).
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0 && ((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0))
/*********************************************************************************************************
** ��������: API_TimerShow
** ��������: ��ʾ��ʱ���������Ϣ
** �䡡��  : ulId         ��ʱ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
VOID    API_TimerShow (LW_OBJECT_HANDLE  ulId)
{
    REGISTER ULONG      ulError;
             BOOL       bTimerRunning;
             ULONG      ulOption;
             ULONG      ulCounter;
             ULONG      ulInterval;
             CHAR       cTimerName[LW_CFG_OBJECT_NAME_SIZE] = "";

    ulError = API_TimerStatus(ulId, &bTimerRunning, &ulOption, &ulCounter, &ulInterval);
    if (ulError) {
        return;                                                         /*  �����˴���                  */
    }
    API_TimerGetName(ulId, cTimerName);
    
    printf("timer show >>\n\n");
    printf("timer name    : %s\n",  cTimerName);
    printf("timer running : %s\n",  bTimerRunning ? "true" : "false");
    printf("timer option  : %lu\n", ulOption);
    printf("timer cnt     : %lu\n", ulCounter);
    printf("timer interval: %lu\n", ulInterval);
    
    printf("\n");
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
                                                                        /*  LW_CFG_HTIMER_EN > 0        */
                                                                        /*  LW_CFG_ITIMER_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
