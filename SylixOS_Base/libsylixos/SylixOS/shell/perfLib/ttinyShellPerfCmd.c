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
** ��   ��   ��: ttinyShellPerfCmd.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 07 �� 28 ��
**
** ��        ��: ���ܷ�������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_SHELL_EN > 0) && (LW_CFG_SYSPERF_EN > 0) && (LW_CFG_SHELL_PERF_TRACE_EN > 0)
#include "sys/sysperf.h"
/*********************************************************************************************************
** ��������: __tshellPerfCmdPerfStart
** ��������: ϵͳ���� "perfstart"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellPerfCmdPerfStart (INT  iArgC, PCHAR  ppcArgV[])
{
    UINT  uiPipeLen       = 1024;
    UINT  uiPerfNum       = 20;
    UINT  uiRefreshPeriod = 10000;
    
    if (iArgC > 1) {
        if (sscanf(ppcArgV[1], "%u", &uiPipeLen) != 1) {
            fprintf(stderr, "option error\n");
            return  (PX_ERROR);
        }
        if ((uiPipeLen < 128) || (uiPipeLen > 4096)) {
            fprintf(stderr, "pipe len must in 128 ~ 4096.\n");
            return  (PX_ERROR);
        }
    }
    
    if (iArgC > 2) {
        if (sscanf(ppcArgV[2], "%u", &uiPerfNum) != 1) {
            fprintf(stderr, "option error\n");
            return  (PX_ERROR);
        }
        if ((uiPerfNum < 10) || (uiPerfNum > 30)) {
            fprintf(stderr, "perfnum must in 10 ~ 30.\n");
            return  (PX_ERROR);
        }
    }
    
    if (iArgC > 3) {
        if (sscanf(ppcArgV[3], "%u", &uiRefreshPeriod) != 1) {
            fprintf(stderr, "option error\n");
            return  (PX_ERROR);
        }
        if (uiRefreshPeriod < 1) {
            fprintf(stderr, "refresh period must bigger than 1 second.\n");
            return  (PX_ERROR);
        }
    }
    
    if (API_SysPerfStart(uiPipeLen, uiPerfNum, uiRefreshPeriod)) {
        fprintf(stderr, "start performance statistics error: %s.\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    printf("performance statistics start checking...\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellPerfCmdPerfStop
** ��������: ϵͳ���� "perfstop"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellPerfCmdPerfStop (INT  iArgC, PCHAR  ppcArgV[])
{
    if (API_SysPerfStop()) {
        fprintf(stderr, "stop performance statistics error: %s.\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    printf("performance statistics stop.\n");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellPerfCmdPerfRefresh
** ��������: ϵͳ���� "perfrefresh"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellPerfCmdPerfRefresh (INT  iArgC, PCHAR  ppcArgV[])
{
    if (API_SysPerfRefresh()) {
        fprintf(stderr, "refresh performance statistics error: %s.\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    printf("performance statistics refreshed.\n");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellPerfCmdPerfShow
** ��������: ϵͳ���� "perfshow"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellPerfCmdPerfShow (INT  iArgC, PCHAR  ppcArgV[])
{
#if LW_CFG_CPU_WORD_LENGHT == 64
    static PCHAR            pcPerfTraceHdr = \
    "                                       Performance Statistics\n\n" LW_TSHELL_COLOR_LIGHT_RED
    "            Because 'static' function NOT in symbol table so if SAMPLE in 'static' function,\n"
    "                 'POSSIBLE FUNCTION' will be the nearest symbol with sample address.\n\n" LW_TSHELL_COLOR_NONE
    "      NAME         TID    PID  CPU       SYMBOL             SAMPLE       TIME CONSUME     POSSIBLE FUNCTION\n"
    "---------------- ------- ----- --- ------------------ ------------------ ------------ -------------------------\n";
#else
    static PCHAR            pcPerfTraceHdr = \
    "                                  Performance Statistics\n\n" LW_TSHELL_COLOR_LIGHT_RED
    "       Because 'static' function NOT in symbol table so if SAMPLE in 'static' function,\n"
    "            'POSSIBLE FUNCTION' will be the nearest symbol with sample address.\n\n" LW_TSHELL_COLOR_NONE
    "      NAME         TID    PID  CPU   SYMBOL     SAMPLE   TIME CONSUME     POSSIBLE FUNCTION\n"
    "---------------- ------- ----- --- ---------- ---------- ------------ -------------------------\n";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
    
    LW_SYSPERF_INFO     sysperf[20];
    CHAR                cThreadName[LW_CFG_OBJECT_NAME_SIZE];
    INT                 i, iNum, iRet;
    ULONG               ulTotal;
    
    API_TShellScrClear(STD_OUT);
    printf(pcPerfTraceHdr);
    
    for (;;) {
        iNum = API_SysPerfInfo(sysperf, 20);
        if (iNum <= 0) {
            sleep(1);
        }
        
        API_TShellScrClear(STD_OUT);
        printf(pcPerfTraceHdr);
        
        ulTotal = 0;
        for (i = 0; i < iNum; i++) {
            ulTotal += sysperf[i].PERF_ulCounter;
        }
        
        for (i = 0; i < iNum; i++) {
            if (API_ThreadGetName(sysperf[i].PERF_ulThread, cThreadName)) {
                lib_strcpy(cThreadName, "<unknown>");
            }
        
            if ((i == 0) || (sysperf[i].PERF_ulCounter * 10) > (ulTotal << 1)) {
                API_TShellColorStart2(LW_TSHELL_COLOR_LIGHT_RED, STD_OUT);
            
            } else if ((sysperf[i].PERF_ulCounter * 10) > (ulTotal)) {
                API_TShellColorStart2(LW_TSHELL_COLOR_YELLOW, STD_OUT);
            
            } else if ((sysperf[i].PERF_ulCounter * 10) > (ulTotal >> 1)) {
                API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
            }
        
#if LW_CFG_CPU_WORD_LENGHT == 64
            iRet = printf("%-16s %7lx %5d %3ld 0x%016lx 0x%016lx %12lu %s\n", 
#else
            iRet = printf("%-16s %7lx %5d %3ld 0x%08lx 0x%08lx %12lu %s\n", 
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
                          cThreadName,
                          sysperf[i].PERF_ulThread,
                          sysperf[i].PERF_pid,
                          sysperf[i].PERF_ulCPUId,
                          sysperf[i].PERF_ulSymAddr,
                          sysperf[i].PERF_ulSamAddr,
                          sysperf[i].PERF_ulCounter,
                          sysperf[i].PERF_cFunc);
                          
            API_TShellColorEnd(STD_OUT);
            
            if (iRet <= 0) {
                goto    __out;
            }
        }
        
        sleep(1);
    }
    
__out:
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellPerfCmdInit
** ��������: ��ʼ���ļ�ϵͳ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __tshellPerfCmdInit (VOID)
{
    API_TShellKeywordAdd("perfstart", __tshellPerfCmdPerfStart);
    API_TShellFormatAdd("perfstart",  " [pipe buffer len] [performance save node] [refresh period]");
    API_TShellHelpAdd("perfstart",    "start performance statistics.\n"
                                      "pipe buffer len      : 128 ~ 4096\n"
                                      "performance save node: 10 ~ 30\n"
                                      "refresh period       : >= 1 second(s).\n");
    
    API_TShellKeywordAdd("perfstop", __tshellPerfCmdPerfStop);
    API_TShellHelpAdd("perfstop",    "stop performance statistics.\n");
    
    API_TShellKeywordAdd("perfrefresh", __tshellPerfCmdPerfRefresh);
    API_TShellHelpAdd("perfrefresh",    "refresh performance statistics.\n");
    
    API_TShellKeywordAddEx("perfs", __tshellPerfCmdPerfShow, LW_OPTION_KEYWORD_SYNCBG);
    API_TShellHelpAdd("perfs",    "show performance statistics.\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
                                                                        /*  LW_CFG_SYSPERF_EN > 0       */
                                                                        /*  LW_CFG_SHELL_PERF_TRACE_EN  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
