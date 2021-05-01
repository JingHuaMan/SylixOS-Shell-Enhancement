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
** ��   ��   ��: lib_panic.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 03 �� 02 ��
**
** ��        ��: panic()
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
** ��������: panic
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  panic (const char  *pcFormat, ...)
{
    va_list           arglist;
    char              cName[LW_CFG_OBJECT_NAME_SIZE] = "";
    LW_OBJECT_HANDLE  ulMe = API_ThreadIdSelf();

    API_ThreadGetName(ulMe, cName);

    va_start(arglist, pcFormat);
    
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    fprintf(stderr, "[panic] ");
    vfprintf(stderr, pcFormat, arglist);
    fprintf(stderr, "thread %lx[%s] clock: %lld\n", ulMe, cName, API_TimeGet64());
    fflush(stdout);
    fflush(stderr);
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_FIO_LIB_EN > 0)     */
    if (getpid() > 0) {                                                 /*  ������ panic                */
#if LW_CFG_SIGNAL_EN > 0
        kill(getpid(), SIGKILL);
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
        API_ThreadForceDelete(&ulMe, (PVOID)EXIT_FAILURE);

    } else {
#if LW_CFG_PANIC_FUNC > 0
        API_ThreadSuspend(ulMe);
#else
        API_KernelReboot(LW_REBOOT_WARM);                               /*  ϵͳ��������                */
#endif                                                                  /*  LW_CFG_PANIC_FUNC > 0       */
    }

    va_end(arglist);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
