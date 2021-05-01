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
** ��   ��   ��: SemaphoreShow.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 03 �� 02 ��
**
** ��        ��: ��ʾָ�����ź�����Ϣ, (��ӡ����׼����ն���)

** BUG
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.04.08  ����� SMP ��˵�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
#if (LW_CFG_SEM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
/*********************************************************************************************************
** ��������: API_SemaphoreShow
** ��������: ��ʾָ�����ź�����Ϣ
** �䡡��  : ulId         �ź������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
VOID    API_SemaphoreShow (LW_OBJECT_HANDLE  ulId)
{
             INTREG                 iregInterLevel;
    REGISTER ULONG                  ulObjectClass;
    REGISTER ULONG                  ulErrorCode;
    REGISTER UINT16                 usIndex;
    REGISTER PLW_CLASS_EVENT        pevent;
             LW_CLASS_EVENT         event;
    
             BOOL                   bValue;
             ULONG                  ulValue;
             ULONG                  ulMaxValue;
             ULONG                  ulWValue;
             ULONG                  ulRWCnt;
             ULONG                  ulOption;
             ULONG                  ulThreadNum;
             
             PCHAR                  pcType;
             PCHAR                  pcWaitType;
             PCHAR                  pcValue = LW_NULL;
              CHAR                  cOwner[LW_CFG_OBJECT_NAME_SIZE];
              
              CHAR                  cValueStr[32];
    
    ulObjectClass = _ObjectGetClass(ulId);                              /*  ����ź������������        */
    usIndex       = _ObjectGetIndex(ulId);
    
    switch (ulObjectClass) {
    
#if LW_CFG_SEMB_EN > 0
    case _OBJECT_SEM_B:
        ulErrorCode = API_SemaphoreBStatus(ulId,
                                           &bValue,
                                           &ulOption,
                                           &ulThreadNum);
        pcType  = "BINARY";
        pcValue = (bValue) ? "FULL" : "EMPTY";
        break;
#endif                                                                  /*  LW_CFG_SEMB_EN > 0          */

#if LW_CFG_SEMC_EN > 0
    case _OBJECT_SEM_C:
        ulErrorCode = API_SemaphoreCStatusEx(ulId,
                                             &ulValue,
                                             &ulOption,
                                             &ulThreadNum,
                                             &ulMaxValue);
        if (ulMaxValue == 1) {
            pcType  = "BINARY(COUNTER)";
        } else {
            pcType  = "COUNTER";
        }
        pcValue = &cValueStr[0];
        sprintf(pcValue, "%lu", ulValue);
        break;
#endif                                                                  /*  LW_CFG_SEMC_EN > 0          */

#if LW_CFG_SEMM_EN > 0
    case _OBJECT_SEM_M:
        ulErrorCode = API_SemaphoreMStatus(ulId,
                                           &bValue,
                                           &ulOption,
                                           &ulThreadNum);
        pcType  = "MUTEX";
        pcValue = (bValue) ? "FULL" : "EMPTY";
        break;
#endif                                                                  /*  LW_CFG_SEMM_EN > 0          */

#if LW_CFG_SEMRW_EN > 0
    case _OBJECT_SEM_RW:
        ulErrorCode = API_SemaphoreRWStatus(ulId,
                                            &ulRWCnt,
                                            &ulValue,
                                            &ulWValue,
                                            &ulOption, LW_NULL);
        pcType      = "RW";
        ulThreadNum = ulValue + ulWValue;
        break;
#endif                                                                  /*  LW_CFG_SEMRW_EN > 0         */
    
    default:
        fprintf(stderr, "\nInvalid semaphore id: 0x%08lx\n", ulId);
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  ������ʹ���                */
        return;
    }
    
    if (ulErrorCode != ERROR_NONE) {
        fprintf(stderr, "\nInvalid semaphore id: 0x%08lx\n", ulId);
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  ������ʹ���                */
        return;
    }
    
    pevent = &_K_eventBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    if (pevent->EVENT_ucType == LW_TYPE_EVENT_UNUSED) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        fprintf(stderr, "\nInvalid semaphore id: 0x%08lx\n", ulId);
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  ������ʹ���                */
        return;
    }
    event = *pevent;                                                    /*  ������Ϣ                    */
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�                    */
    
    pcWaitType = (event.EVENT_ulOption & LW_OPTION_WAIT_PRIORITY)
               ? "PRIORITY" : "FIFO";
    
    printf("Semaphore show >>\n\n");                                    /*  ��ӡ������Ϣ                */
    printf("%-20s: %-10s\n",    "Semaphore Name", event.EVENT_cEventName);
    printf("%-20s: 0x%-10lx\n", "Semaphore Id",   ulId);
    printf("%-20s: %-10s\n",    "Semaphore Type", pcType);
    printf("%-20s: %-10s\n",    "Thread Queuing", pcWaitType);
    printf("%-20s: %-10ld\n",   "Pended Threads", ulThreadNum);
    
    if (ulObjectClass == _OBJECT_SEM_M) {                               /*  �����ź���                  */
        PCHAR       pcSafeMode;
        PCHAR       pcMethod;
        
        if (bValue == LW_FALSE) {
            API_ThreadGetName(((PLW_CLASS_TCB)(event.EVENT_pvTcbOwn))->TCB_ulId,
                              cOwner);
        } else {
            lib_strcpy(cOwner, "NO THREAD");
        }
        
        pcSafeMode = (event.EVENT_ulOption & LW_OPTION_DELETE_SAFE)
                   ? "SEM_DELETE_SAFE" : "SEM_INVERSION_SAFE";          /*  �Ƿ�ʹ�ð�ȫģʽ            */
        pcMethod   = (event.EVENT_ulOption & LW_OPTION_INHERIT_PRIORITY)
                   ? "INHERIT_PRIORITY" : "PRIORITY_CEILING";
                   
        printf("%-20s: %-10s\n",  "Owner", cOwner);                     /*  ӵ����                      */
        printf("%-20s: 0x%lx\t%s\t%s\n", "Options", event.EVENT_ulOption, 
               pcSafeMode, pcMethod);
                      
        if (!(event.EVENT_ulOption & LW_OPTION_INHERIT_PRIORITY)) {
            printf("%-20s: %-10d\n", "Ceiling", 
                   event.EVENT_ucCeilingPriority);                      /*  �컨�����ȼ�                */
        }
        
    } else if (ulObjectClass == _OBJECT_SEM_RW) {                       /*  ��д�ź���                  */
        PCHAR       pcSafeMode;
        
        if (bValue == LW_FALSE) {
            API_ThreadGetName(((PLW_CLASS_TCB)(event.EVENT_pvTcbOwn))->TCB_ulId,
                              cOwner);
        } else {
            if (ulRWCnt) {
                lib_strcpy(cOwner, "READERS");
            } else {
                lib_strcpy(cOwner, "NO THREAD");
            }
        }
        
        pcSafeMode = (event.EVENT_ulOption & LW_OPTION_DELETE_SAFE)
                   ? "SEM_DELETE_SAFE" : "SEM_INVERSION_SAFE";          /*  �Ƿ�ʹ�ð�ȫģʽ            */
                   
        printf("%-20s: %-10s\n", "Owner", cOwner);                      /*  ӵ����                      */
        printf("%-20s: 0x%lx\t%s\n", "Options", event.EVENT_ulOption, 
               pcSafeMode);
                                                                        /*  ��ǰ����ֵ                  */
        printf("%-20s: %lu\n", "Read/Write Counter", ulRWCnt);
        printf("%-20s: %lu\n", "Read Pend", ulValue);
        printf("%-20s: %lu\n", "Write Pend", ulWValue);
        
    } else {
        if (ulObjectClass == _OBJECT_SEM_C) {
            printf("%-20s: %-10lu\n",   "Semaphore Max Value", 
                   event.EVENT_ulMaxCounter);                           /*  ������ֵ                  */
        }
        
        printf("%-20s: %-10s\n",   "Semaphore Value", pcValue);         /*  ��ǰ����ֵ                  */
    }
    
    printf("\n");
}

#endif                                                                  /*  (LW_CFG_SEM_EN > 0) &&      */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
