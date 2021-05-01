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
** ��   ��   ��: MsgQueueShow.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 03 �� 03 ��
**
** ��        ��: ��ʾָ������Ϣ������Ϣ, (��ӡ����׼����ն���)
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0 && LW_CFG_MSGQUEUE_EN > 0
/*********************************************************************************************************
** ��������: API_MsgQueueShow
** ��������: ��ʾָ������Ϣ������Ϣ
** �䡡��  : ulId         ��Ϣ���о��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
VOID   API_MsgQueueShow (LW_OBJECT_HANDLE  ulId)
{
    REGISTER ULONG      ulErrorCode;
    
             ULONG      ulMaxMsgNum;
             ULONG      ulCounter;
             size_t     stMsgLen;
             ULONG      ulOption;
             ULONG      ulThreadBlockNum;
             size_t     stMaxMsgLen;
             
             CHAR       cMsgQueueName[LW_CFG_OBJECT_NAME_SIZE];

             PCHAR      pcWaitType;

    API_MsgQueueGetName(ulId, cMsgQueueName);                           /*  �������                    */
    
    ulErrorCode = API_MsgQueueStatusEx(ulId,
                                       &ulMaxMsgNum,
                                       &ulCounter,
                                       &stMsgLen,
                                       &ulOption,
                                       &ulThreadBlockNum,
                                       &stMaxMsgLen);                   /*  ��õ�ǰ״̬                */
    if (ulErrorCode) {
        fprintf(stderr, "\nInvalid MsgQueue id: 0x%08lx\n", ulId);
        return;                                                         /*  ��������                    */
    }
    
    pcWaitType = (ulOption & LW_OPTION_WAIT_PRIORITY)
               ? "PRIORITY" : "FIFO";
    
    printf("MsgQueue show >>\n\n");                                     /*  ��ӡ������Ϣ                */
    printf("%-20s: %-10s\n",    "MsgQueue Name",        cMsgQueueName);
    printf("%-20s: 0x%-10lx\n", "MsgQueue Id",          ulId);
    printf("%-20s: %-10ld\n",   "MsgQueue Max Msgs",    ulMaxMsgNum);
    printf("%-20s: %-10ld\n",   "MsgQueue N Msgs",      ulCounter);
    printf("%-20s: %-10zd\n",   "MsgQueue Max Msg Len", stMaxMsgLen);
    printf("%-20s: %-10s\n",    "Thread Queuing",       pcWaitType);
    printf("%-20s: %-10ld\n",   "Pended Threads",       ulThreadBlockNum);
    
    printf("\n");
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
                                                                        /*  LW_CFG_MSGQUEUE_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
