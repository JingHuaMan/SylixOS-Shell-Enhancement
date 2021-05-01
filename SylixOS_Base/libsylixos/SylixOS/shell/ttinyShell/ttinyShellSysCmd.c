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
** ��   ��   ��: ttinyShellSysCmd.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 07 �� 27 ��
**
** ��        ��: һ����С�͵� shell ϵͳ, ϵͳ�ڲ������.

** BUG
2008.10.12  �������� shell ϵͳ����.
2008.12.23  ���� vmm ��ʾָ��.
2009.02.14  ���� vars ��ʾ����, ��ʾϵͳ��������.
2009.02.19  ���� sync ����.
2009.03.11  help ��ʽ�ִ�����ʾ��ɫ����ʾ.
2009.04.09  ������Ϣ����, �ź���, �¼���, �����ڴ��������Ϣ��ʾ.
2009.05.30  ������ help ��ʾ����� BUG.
2009.06.17  time �����������ʵ UTC ʱ��.
2009.06.30  help -s �����м���Ը�ʽ�ִ�����ʾ.
2009.07.02  ���� sprio ����.
2009.07.03  ������ GCC �����ľ���.
2009.08.08  �����Ǳ�������һ���������, ���ǻ����Ǹ�����������������.
            __tshellSysCmdHelp() ���յ� q ʱ, �˳�ǰ��Ҫ��ս��ջ���.
2009.10.27  ����� printk ��ӡ��Ϣ���������.
            ���� buss ����
2009.11.07  cpuus ���� -n ����.
2009.11.19  ���� open close ����, ����Ҫ����, control-C ��, �п������ļ�����������(û�� cleanup push), 
            ����ʹ�� close ǿ�ƹر�.
2009.11.21  ���� user ����, ���Զ��û����й���(�򵥹���).
2009.12.11  ���� devs -a ѡ��.
2010.01.04  ����ʹ�����µ� TOD ʱ��, ������� hwclock ��������ͬ��Ӳ��ʱ��.
2010.01.16  ���� shstack ����.
2010.07.10  ���� __tshellSysCmdTimes() �� __tshellSysCmdHwclock() ʹ�� asctime_r() �����뺯��.
2011.02.17  ���� date ����, ֧�� shell ����ʱ��.
2011.02.22  ���� sigqueue ����, �����Ŷ��ź�.
2011.05.17  ����� vmm ������ֹͳ����Ϣ��ӡ����.
2011.06.11  ����ʱ��ͬ������, �� TZ ����������ͬ��ʱ����Ϣ.
2011.06.23  ʹ���µ�ɫ��ת��.
2011.07.03  �� varsave �� varload ʱû��ָ������ʱ, Ĭ��ʹ�� __TTINY_SHELL_VAR_FILE
2012.03.31  ��ʱȡ�� user ����, δ�������û�����.
2012.09.05  cpuus ����Լ��ʱ������ù���.
2012.10.19  vars �������Ա���ֱ�����õĴ�ӡ.
2012.11.09  ���� shutdown ����.
2012.12.11  ts ���������ʾָ�������ڵ��߳�.
2012.12.22  ���� fdentrys ����.
2013.01.07  open ����� inode ���ļ���С�Ĵ�ӡ.
2013.06.13  ���� renice ����.
2013.08.27  ���� monitor ����.
2013.09.30  ���� lspci ����.
2014.07.11  ���� color ����.
2014.10.10  help ����֧��ͨ���ƥ��.
2014.10.12  ����������ɾ�������Ϊ vardel.
            free ����Ϊ��ʾ��ǰ�ڴ�ʹ�����.
2014.11.11  ���� CPU �׺Ͷ�����.
2017.12.02  ���� cdump ����.
2020.02.22  ���� pagefaultlimit ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "limits.h"
#include "unistd.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "sys/ioctl.h"
#include "sys/vproc.h"
#include "crypt.h"
#include "pwd.h"
#include "ttinyShell.h"
#include "ttinyShellLib.h"
#include "ttinyShellSysCmd.h"
/*********************************************************************************************************
  �����ڲ�֧��
*********************************************************************************************************/
#include "../SylixOS/shell/ttinyVar/ttinyVarLib.h"
#include "../SylixOS/posix/include/px_resource.h"
#if LW_CFG_POSIX_EN > 0
#include "pthread.h"
#include "pthread_np.h"
#include "sched.h"
#include "fnmatch.h"
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
#include "time.h"
#endif
/*********************************************************************************************************
** ��������: __tshellSysCmdArgs
** ��������: ϵͳ���� "args"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdArgs (INT  iArgC, PCHAR  ppcArgV[])
{
    REGISTER INT    i;
    
    for (i = 0; i < iArgC; i++) {
        printf("arg %3d is %s\n", i + 1, ppcArgV[i]);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdEcho
** ��������: ϵͳ���� "echo"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdEcho (INT  iArgC, PCHAR  ppcArgV[])
{
    REGISTER INT   i;
    
    for (i = 1; i < iArgC; i++) {
        printf("%s ", ppcArgV[i]);
    }
    printf("\n");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdSleep
** ��������: ϵͳ���� "sleep"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdSleep (INT  iArgC, PCHAR  ppcArgV[])
{
#define SLEEP_UNIT_SEC  0
#define SLEEP_UNIT_MIN  1
#define SLEEP_UNIT_HOUR 2
#define SLEEP_UNIT_DAY  3

    PCHAR   pcUnit;
    INT     iUnit = SLEEP_UNIT_SEC;
    CHAR    cParam[64];
    ULONG   ulParam;
    UINT    uiSec;
    
    if (iArgC < 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    lib_strlcpy(cParam, ppcArgV[1], sizeof(cParam));
    
    pcUnit = lib_strchr(cParam, 's');
    if (pcUnit) {
        iUnit   = SLEEP_UNIT_SEC;
        *pcUnit = PX_EOS;
        goto    __sleep;
    }
    
    pcUnit = lib_strchr(cParam, 'm');
    if (pcUnit) {
        iUnit   = SLEEP_UNIT_MIN;
        *pcUnit = PX_EOS;
        goto    __sleep;
    }
    
    pcUnit = lib_strchr(cParam, 'h');
    if (pcUnit) {
        iUnit   = SLEEP_UNIT_HOUR;
        *pcUnit = PX_EOS;
        goto    __sleep;
    }
    
    pcUnit = lib_strchr(cParam, 'd');
    if (pcUnit) {
        iUnit   = SLEEP_UNIT_DAY;
        *pcUnit = PX_EOS;
        goto    __sleep;
    }
    
__sleep:
    ulParam = lib_strtoul(cParam, LW_NULL, 10);
    switch (iUnit) {
    
    case SLEEP_UNIT_MIN:
        uiSec = (UINT)(ulParam * 60);
        break;
    
    case SLEEP_UNIT_HOUR:
        uiSec = (UINT)(ulParam * 3600);
        break;
    
    case SLEEP_UNIT_DAY:
        uiSec = (UINT)(ulParam * 86400);
        break;
    
    case SLEEP_UNIT_SEC:
    default:
        uiSec = (UINT)ulParam;
        break;
    }

    return  ((INT)sleep(uiSec));
}
/*********************************************************************************************************
** ��������: __tshellSetTtyOpt
** ��������: ���� TTY Ӳ������
** �䡡��  : iFd           �ļ�������
**           pcOpt         ���� ����: 115200,n,8,1
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSetTtyOpt (INT  iFd, PCHAR  pcOpt)
{
    LONG   lBaud;
    INT    iHwOpt;
    CHAR   cParity;
    INT    iBits, iStop, iNum;
    
    if ((iFd < 0) || (pcOpt == LW_NULL)) {
__opt_error:
        fprintf(stderr, "tty hardware option error, \n"
                        "eg. /dev/ttyS1:115200,n,8,1\n"
                        "    /dev/ttyS1:115200,o,7,1\n"
                        "    /dev/ttyS1:9600,e,8,1\n");
        return  (PX_ERROR);
    }
    
    iNum = sscanf(pcOpt, "%ld,%c,%d,%d", &lBaud, &cParity, &iBits, &iStop);
    if (iNum < 1) {
        goto    __opt_error;
    }
    
    if ((lBaud < SIO_BAUD_110) || (lBaud > SIO_BAUD_256000)) {
        fprintf(stderr, "baud rate error: must be in 110 ~ 256000\n");
        return  (PX_ERROR);
    }
    
    ioctl(iFd, SIO_BAUD_SET, lBaud);
    
    if (iNum > 1) {
        iHwOpt = CLOCAL | CREAD | CS8 | HUPCL;
        
        switch (lib_tolower(cParity)) {
        
        case 'o':
            iHwOpt |= PARENB;
            iHwOpt |= PARODD;
            break;
            
        case 'e':
            iHwOpt |= PARENB;
            iHwOpt &= ~PARODD;
            break;
            
        case 'n':
            break;
            
        default:
            fprintf(stderr, "parity error: must be: 'n': NONE 'e': EVEN 'o': ODD\n");
            return  (PX_ERROR);
        }
        
        if (iNum > 2) {
            switch (iBits) {
            
            case 5:
                iHwOpt &= ~CSIZE;
                iHwOpt |= CS5;
                break;
                
            case 6:
                iHwOpt &= ~CSIZE;
                iHwOpt |= CS6;
                break;
            
            case 7:
                iHwOpt &= ~CSIZE;
                iHwOpt |= CS7;
                break;
            
            case 8:
                break;
            
            default:
                fprintf(stderr, "bits set error: must be in 5 ~ 8\n");
                return  (PX_ERROR);
            }
            
            if (iNum > 3) {
                switch (iStop) {
                
                case 1:
                    break;
                
                case 2:
                    iHwOpt |= STOPB;
                    break;
                
                default:
                    fprintf(stderr, "stop bits set error: must be in 1 or 2\n");
                    return  (PX_ERROR);
                }
            }
        }
    
        ioctl(iFd, SIO_HW_OPTS_SET, iHwOpt);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdShell
** ��������: ϵͳ���� "shell"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdShell (INT  iArgC, PCHAR  ppcArgV[])
{
    INT    iFd;
    PCHAR  pcDiv;
    ULONG  ulOption = LW_OPTION_TSHELL_VT100       | 
                      LW_OPTION_TSHELL_AUTHEN      | 
                      LW_OPTION_TSHELL_PROMPT_FULL | 
                      LW_OPTION_TSHELL_CLOSE_FD;
    
    if (iArgC < 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    if (iArgC > 2) {
        if (lib_strcmp(ppcArgV[2], "nologin") == 0) {
            ulOption &= ~LW_OPTION_TSHELL_AUTHEN;
        }
    }
    
    pcDiv = lib_strchr(ppcArgV[1], ':');
    if (pcDiv) {
        *pcDiv++ = PX_EOS;
    }
    
    iFd = open(ppcArgV[1], O_RDWR);
    if (iFd < 0) {
        fprintf(stderr, "open file return: %d error: %s\n", iFd, lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    if (!isatty(iFd)) {
        fprintf(stderr, "this file is not a tty device.\n");
        close(iFd);
        return  (PX_ERROR);
    }
    
    if (pcDiv) {
        if (__tshellSetTtyOpt(iFd, pcDiv) < ERROR_NONE) {
            fprintf(stderr, "set tty option error.\n");
            close(iFd);
            return  (PX_ERROR);
        }
    }
    
    if (API_TShellCreate(iFd, ulOption) == LW_OBJECT_HANDLE_INVALID) {
        fprintf(stderr, "can not create shell: %s\n", lib_strerror(errno));
        close(iFd);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __helpPrintKeyword
** ��������: ��ӡһ��ϵͳ�ڽ��ؼ�����Ϣ
** �䡡��  : pskwNode      �ؼ���
**           bDetails      �Ƿ��ӡ��ϸ��Ϣ
** �䡡��  : ������ֽ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __helpPrintKeyword (__PTSHELL_KEYWORD  pskwNode, BOOL  bDetails)
{
    INT   iChars = 0;

    if (bDetails && pskwNode->SK_pcHelpString) {
        iChars += printf("%s", pskwNode->SK_pcHelpString);              /*  ��ӡ������Ϣ                */
    }

    if (bDetails) {
        iChars += printf("%s", pskwNode->SK_pcKeyword);                 /*  ��ӡ�ؼ��ֶ���              */
    } else {
        iChars += printf("%-20s", pskwNode->SK_pcKeyword);
    }
    
    if (pskwNode->SK_pcFormatString) {
        API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
        iChars += printf("%s", pskwNode->SK_pcFormatString);            /*  ��ӡ��ʽ��Ϣ                */
        API_TShellColorEnd(STD_OUT);
    }
    
    printf("\n");
    
    return  (iChars);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdHelp
** ��������: ϵͳ���� "help"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdHelp (INT  iArgC, PCHAR  ppcArgV[])
{
#define __HELP_BUFF_SIZE    20

    REGISTER INT            i, iChars;
    REGISTER ULONG          ulGetNum, ulNum, ulTotal;
             CHAR           cData;
    
    __PTSHELL_KEYWORD       pskwNodeStart = LW_NULL;
    __PTSHELL_KEYWORD       pskwNode[__HELP_BUFF_SIZE];

    if (iArgC == 1) {                                                   /*  ��Ҫ��ʾ������Ϣ            */
        struct winsize      ws;
        
        if (ioctl(STD_OUT, TIOCGWINSZ, &ws) || (ws.ws_row < 24) || (ws.ws_col < 80)) {
            ws.ws_row = 24;
            ws.ws_col = 80;
        }
        
        ws.ws_row -= 2;
        ulTotal    = 0;
        do {
            ulNum = ws.ws_row - ulTotal;
            if (ulNum > __HELP_BUFF_SIZE) {
                ulNum = __HELP_BUFF_SIZE;
            }
        
            ulGetNum = __tshellKeywordList(pskwNodeStart, pskwNode, (INT)ulNum);
            for (i = 0; i < ulGetNum; i++) {
                iChars   = __helpPrintKeyword(pskwNode[i], LW_FALSE);
                ulTotal += (ULONG)(iChars / ws.ws_col) + 1;
            }

            if (ulTotal == ws.ws_row) {
                printf("\nPress <ENTER> to continue, <Q> to quit: ");   /*  ��Ҫ����                    */
                fflush(stdout);
                read(0, &cData, 1);
                if (cData == 'q' || cData == 'Q') {                     /*  �Ƿ���Ҫ�˳�                */
                    ioctl(STD_IN, FIOFLUSH);                            /*  ��ս��ջ���                */
                    break;                                              /*  �˳�                        */
                }
                ulTotal = 0;
            }

            pskwNodeStart = pskwNode[ulGetNum - 1];
        } while ((ulGetNum == ulNum) && (pskwNodeStart != LW_NULL));
    
    } else {
        PCHAR   pcKeyword = LW_NULL;
    
        if (iArgC == 2) {
            pcKeyword = ppcArgV[1];
        } else if (iArgC == 3) {
            if (lib_strcmp("-s", ppcArgV[1])) {
                fprintf(stderr, "option error.\n");
                return  (PX_ERROR);
            }
            pcKeyword = ppcArgV[2];
        } else {
            return  (PX_ERROR);
        }
        
        if (lib_strchr(pcKeyword, '*') ||
            lib_strchr(pcKeyword, '?')) {                               /*  ���� shell ͨ���           */

#if LW_CFG_POSIX_EN > 0
            INT     iRet;
            do {
                ulGetNum = __tshellKeywordList(pskwNodeStart,
                                               pskwNode, 1);
                if (ulGetNum) {
                    iRet = fnmatch(pcKeyword, pskwNode[0]->SK_pcKeyword, FNM_PATHNAME);
                    if (iRet == ERROR_NONE) {
                        __helpPrintKeyword(pskwNode[0], LW_FALSE);
                    }
                    pskwNodeStart = pskwNode[0];
                }
            } while (ulGetNum);
#else
            fprintf(stderr, "sylixos do not have fnmatch().\n");
            return  (-ERROR_TSHELL_EKEYWORD);
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
        } else if (ERROR_NONE == __tshellKeywordFind(pcKeyword, &pskwNode[0])) {
            __helpPrintKeyword(pskwNode[0], LW_TRUE);
            
        } else {
            return  (-ERROR_TSHELL_EKEYWORD);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdFree
** ��������: ϵͳ���� "free"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdFree (INT  iArgC, PCHAR  ppcArgV[])
{
    API_RegionShow(0);
    
#if LW_CFG_VMM_EN > 0
    API_VmmPhysicalShow();
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdVardel
** ��������: ϵͳ���� "vardel"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdVardel (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC != 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    if (ERROR_NONE == API_TShellVarDelete(ppcArgV[1])) {
        return  (ERROR_NONE);
    } else {
        return  (-ERROR_TSHELL_EVAR);
    }
}
/*********************************************************************************************************
** ��������: __tshellSysCmdSem
** ��������: ϵͳ���� "sem"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdSem (INT  iArgC, PCHAR  ppcArgV[])
{
#if (LW_CFG_SEM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    LW_OBJECT_HANDLE        ulId = LW_OBJECT_HANDLE_INVALID;

    if (iArgC != 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    sscanf(ppcArgV[1], "%lx", &ulId);
    
    API_SemaphoreShow(ulId);
#endif                                                                  /*  (LW_CFG_SEM_EN > 0) &&      */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdPart
** ��������: ϵͳ���� "part"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdPart (INT  iArgC, PCHAR  ppcArgV[])
{
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
    LW_OBJECT_HANDLE        ulId = LW_OBJECT_HANDLE_INVALID;

    if (iArgC != 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    sscanf(ppcArgV[1], "%lx", &ulId);
    
    API_PartitionShow(ulId);
#endif                                                                  /*  LW_CFG_PARTITION_EN > 0     */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdEventSet
** ��������: ϵͳ���� "eventset"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdEventSet (INT  iArgC, PCHAR  ppcArgV[])
{
#if LW_CFG_FIO_LIB_EN > 0 && LW_CFG_EVENTSET_EN > 0
    LW_OBJECT_HANDLE        ulId = LW_OBJECT_HANDLE_INVALID;

    if (iArgC != 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    sscanf(ppcArgV[1], "%lx", &ulId);
    
    API_EventSetShow(ulId);
#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
                                                                        /*  LW_CFG_EVENTSET_EN > 0      */
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdMsgq
** ��������: ϵͳ���� "msgq"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdMsgq (INT  iArgC, PCHAR  ppcArgV[])
{
#if LW_CFG_FIO_LIB_EN > 0 && LW_CFG_MSGQUEUE_EN > 0
    LW_OBJECT_HANDLE        ulId = LW_OBJECT_HANDLE_INVALID;

    if (iArgC != 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    sscanf(ppcArgV[1], "%lx", &ulId);
    
    API_MsgQueueShow(ulId);
#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
                                                                        /*  LW_CFG_MSGQUEUE_EN > 0      */
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdTs
** ��������: ϵͳ���� "ts"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdTs (INT  iArgC, PCHAR  ppcArgV[])
{
    pid_t   pid = 0;

    if (iArgC < 2) {
        API_ThreadShow();
        return  (ERROR_NONE);
    }
    
    if (sscanf(ppcArgV[1], "%d", &pid) != 1) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }

    API_ThreadShowEx(pid);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdTp
** ��������: ϵͳ���� "tp"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdTp (INT  iArgC, PCHAR  ppcArgV[])
{
    pid_t   pid = 0;

    if (iArgC < 2) {
        API_ThreadPendShow();
        return  (ERROR_NONE);
    }
    
    if (sscanf(ppcArgV[1], "%d", &pid) != 1) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }

    API_ThreadPendShowEx(pid);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdWjs
** ��������: ϵͳ���� "wjs"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellSysCmdWjs (INT  iArgC, PCHAR  ppcArgV[])
{
    pid_t   pid = 0;

    if (iArgC < 2) {
        API_ThreadWjShow();
        return  (ERROR_NONE);
    }

    if (sscanf(ppcArgV[1], "%d", &pid) != 1) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }

    API_ThreadWjShowEx(pid);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdSs
** ��������: ϵͳ���� "ss"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdSs (INT  iArgC, PCHAR  ppcArgV[])
{
    API_StackShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdCpuus
** ��������: ϵͳ���� "cpuus"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdCpuus (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iTime  = 1;
    INT     iTimes = 1;
    INT     i;
    
    if (iArgC >= 3) {
        for (i = 1; i < iArgC - 1; i += 2) {
            if (lib_strcmp(ppcArgV[i], "-n") == 0) {
                sscanf(ppcArgV[i + 1], "%d", &iTimes);                  /*  ָ����ʾ����                */
            
            } else if (lib_strcmp(ppcArgV[i], "-t") == 0) {
                sscanf(ppcArgV[i + 1], "%d", &iTime);                   /*  ���ʱ��                    */
            }
        }
    }

    API_CPUUsageShow(iTime, iTimes);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdX86
** ��������: ϵͳ���� "x86"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#ifdef LW_CFG_CPU_ARCH_X86

static INT  __tshellSysCmdX86 (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC == 1) {
        x86CpuIdShow();

    } else if (lib_strcmp(ppcArgV[1], "cpuid") == 0) {
        x86CpuIdShow();

    } else if (lib_strcmp(ppcArgV[1], "cputop") == 0) {
        x86CpuTopologyShow();

    } else if (lib_strcmp(ppcArgV[1], "mps") == 0) {
        x86MpApicDataShow();

    } else if (lib_strcmp(ppcArgV[1], "bios") == 0) {
        x86MpBiosShow();

    } else if (lib_strcmp(ppcArgV[1], "ioint") == 0) {
        x86MpBiosIoIntMapShow();

    } else if (lib_strcmp(ppcArgV[1], "loint") == 0) {
        x86MpBiosLocalIntMapShow();

    } else if (lib_strcmp(ppcArgV[1], "acpi") == 0) {
        x86AcpiMpTableShow();
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_ARCH_X86         */
/*********************************************************************************************************
** ��������: __tshellSysCmdInts
** ��������: ϵͳ���� "ints"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdInts (INT  iArgC, PCHAR  ppcArgV[])
{
    ULONG  ulCPUStart = 0;
    ULONG  ulCPUEnd   = LW_NCPUS - 1;
    
    if (iArgC > 1) {
        sscanf(ppcArgV[1], "%ld", &ulCPUStart);
    }
    
    if (iArgC > 2) {
        sscanf(ppcArgV[2], "%ld", &ulCPUEnd);
    }
    
    API_InterShow(ulCPUStart, ulCPUEnd);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdMems
** ��������: ϵͳ���� "mems"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdMems (INT  iArgC, PCHAR  ppcArgV[])
{
    LW_OBJECT_HANDLE   ulId = LW_OBJECT_HANDLE_INVALID;
    
    if (iArgC == 1) {
        API_RegionShow(0);
    
    } else if (iArgC == 2) {
        if (ppcArgV[1][0] < '0' ||
            ppcArgV[1][0] > '9') {
            fprintf(stderr, "option error.\n");
            return  (PX_ERROR);
        }
        sscanf(ppcArgV[1], "%lx", &ulId);
        
        API_RegionShow(ulId);
    } else {
        fprintf(stderr, "option error.\n");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdKill
** ��������: ϵͳ���� "kill"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0

static INT  __tshellSysCmdKill (INT  iArgC, PCHAR  ppcArgV[])
{
    LW_OBJECT_HANDLE  ulId    = LW_OBJECT_HANDLE_INVALID;
    INT               iSigNum = SIGKILL;

    if (iArgC == 2) {
        if (ppcArgV[1][0] < '0' ||
            ppcArgV[1][0] > '9') {
            fprintf(stderr, "option error.\n");
            return  (PX_ERROR);
        }
        
        sscanf(ppcArgV[1], "%lx", &ulId);
        if (API_ObjectGetClass(ulId) != _OBJECT_THREAD) {
            sscanf(ppcArgV[1], "%ld", &ulId);                           /*  ���� id                     */
        }
        return  (kill(ulId, SIGKILL));
    
    } else if (iArgC == 4) {
        if (lib_strcmp(ppcArgV[1], "-n")) {
            fprintf(stderr, "option error.\n");
            return  (PX_ERROR);
        }
        
        sscanf(ppcArgV[2], "%d",  &iSigNum);
        sscanf(ppcArgV[3], "%lx", &ulId);
        if (API_ObjectGetClass(ulId) != _OBJECT_THREAD) {
            sscanf(ppcArgV[3], "%ld", &ulId);                           /*  ���� id                     */
        }
        return  (kill(ulId, iSigNum));
    
    } else {
        fprintf(stderr, "option error.\n");
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __tshellSysCmdSigqueue
** ��������: ϵͳ���� "sigqueue"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdSigqueue (INT  iArgC, PCHAR  ppcArgV[])
{
    LW_OBJECT_HANDLE  ulId    = LW_OBJECT_HANDLE_INVALID;
    INT               iSigNum = SIGKILL;
    union sigval      sigvalue;
    
    sigvalue.sival_int = 0;
    
    if (iArgC == 2) {
        if (ppcArgV[1][0] < '0' ||
            ppcArgV[1][0] > '9') {
            fprintf(stderr, "option error.\n");
            return  (PX_ERROR);
        }
        
        sscanf(ppcArgV[1], "%lx", &ulId);
        if (API_ObjectGetClass(ulId) != _OBJECT_THREAD) {
            sscanf(ppcArgV[1], "%ld", &ulId);                           /*  ���� id                     */
        }
        return  (sigqueue(ulId, SIGKILL, sigvalue));
    
    } else if (iArgC == 4) {
        if (lib_strcmp(ppcArgV[1], "-n")) {
            fprintf(stderr, "option error.\n");
            return  (PX_ERROR);
        }
        
        sscanf(ppcArgV[2], "%d",  &iSigNum);
        sscanf(ppcArgV[3], "%lx", &ulId);
        if (API_ObjectGetClass(ulId) != _OBJECT_THREAD) {
            sscanf(ppcArgV[1], "%ld", &ulId);                           /*  ���� id                     */
        }
        return  (sigqueue(ulId, iSigNum, sigvalue));
    
    } else {
        fprintf(stderr, "option error.\n");
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
** ��������: __tshellSysCmdTimes
** ��������: ϵͳ���� "times"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdTimes (INT  iArgC, PCHAR  ppcArgV[])
{
    BOOL        bIsUTC = LW_FALSE;
    time_t      tim;
    struct tm   tmTime;
    CHAR        cTimeBuffer[32];
    
    lib_time(&tim);
    
    if (iArgC == 2) {
        if (lib_strcmp(ppcArgV[1], "-utc") == 0) {
            bIsUTC = LW_TRUE;
        }
    }
    
    if (bIsUTC) {
        lib_gmtime_r(&tim, &tmTime);
        printf("UTC : ");
    } else {
        lib_localtime_r(&tim, &tmTime);
    }
    
    printf("%s", lib_asctime_r(&tmTime, cTimeBuffer));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdTzsync
** ��������: ϵͳ���� "tzsync"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdTzsync (INT  iArgC, PCHAR  ppcArgV[])
{
    lib_tzset();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdDate
** ��������: ϵͳ���� "date"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdDate (INT  iArgC, PCHAR  ppcArgV[])
{
    struct timespec     tv;
    time_t              tim;
    struct tm           tmTime;
    CHAR                cTimeBuffer[32];
    
    lib_time(&tim);                                                     /*  ��õ�ǰϵͳʱ��            */
    lib_localtime_r(&tim, &tmTime);

    if (iArgC == 3) {
        if ((lib_strcmp(ppcArgV[1], "-s") == 0) ||
            (lib_strcmp(ppcArgV[1], "--set") == 0)) {
            if (lib_index(ppcArgV[2], ':')) {                           /*  ʱ������                    */
                if (sscanf(ppcArgV[2], "%d:%d:%d", &tmTime.tm_hour,
                                                   &tmTime.tm_min,
                                                   &tmTime.tm_sec) > 0) {
                                                                        /*  ����Ҫ��һ��ʱ�����        */
                    tv.tv_sec  = lib_timegm(&tmTime) + timezone;        /*  UTC = LOCAL + timezone      */
                    tv.tv_nsec = 0;
                    lib_clock_settime(CLOCK_REALTIME, &tv);
                    
                } else {
                    printf("invalid time '%s'.\n", ppcArgV[2]);
                    return  (PX_ERROR);
                }
            
            } else {                                                    /*  ��������                    */
                if (lib_strnlen(ppcArgV[2], 10) == 8) {
                    PCHAR       pcDate = ppcArgV[2];
                    INT         i;
                    
                    for (i = 0; i < 8; i++) {
                        if (lib_isdigit(pcDate[i]) == 0) {
                            goto    __invalid_date;
                        }
                    }
                    
                    tmTime.tm_year = ((pcDate[0] - '0') * 1000)
                                   + ((pcDate[1] - '0') * 100)
                                   + ((pcDate[2] - '0') * 10)
                                   +  (pcDate[3] - '0');                /*  ��ȡ year ��Ϣ              */
                    if (tmTime.tm_year >  1900) {
                        tmTime.tm_year -= 1900;
                    } else {
                        goto    __invalid_date;
                    }
                    
                    tmTime.tm_mon = ((pcDate[4] - '0') * 10)
                                  +  (pcDate[5] - '0');                 /*  ��ȡ month ��Ϣ             */
                    if ((tmTime.tm_mon) > 0 && (tmTime.tm_mon < 13)) {
                        tmTime.tm_mon -= 1;
                    } else {
                        goto    __invalid_date;
                    }
                    
                    tmTime.tm_mday = ((pcDate[6] - '0') * 10)
                                   +  (pcDate[7] - '0');                /*  ��ȡ mday ��Ϣ              */
                    if ((tmTime.tm_mday < 1) || (tmTime.tm_mday > 31)) {
                        goto    __invalid_date;
                    }
                    
                    tv.tv_sec  = lib_timegm(&tmTime) + timezone;        /*  UTC = LOCAL + timezone      */
                    tv.tv_nsec = 0;
                    lib_clock_settime(CLOCK_REALTIME, &tv);
                
                } else {
__invalid_date:
                    printf("invalid date '%s'.\n", ppcArgV[2]);
                    return  (PX_ERROR);
                }
            }
        } else {
            fprintf(stderr, "option error.\n");
            return  (PX_ERROR);
        }
        
        lib_time(&tim);                                                 /*  ��õ�ǰϵͳʱ��            */
        lib_localtime_r(&tim, &tmTime);
    }
    
    
    printf("%s", lib_asctime_r(&tmTime, cTimeBuffer));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdDrvs
** ��������: ϵͳ���� "drvs"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdDrvs (INT  iArgC, PCHAR  ppcArgV[])
{
    API_IoDrvShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdDrvlics
** ��������: ϵͳ���� "drvlics"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdDrvlics (INT  iArgC, PCHAR  ppcArgV[])
{
    API_IoDrvLicenseShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdDevs
** ��������: ϵͳ���� "devs"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdDevs (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iShowType = 0;

    if (iArgC > 1) {
        if (lib_strncmp(ppcArgV[1], "-a", 3) == 0) {
            iShowType = 1;
        }
    }
    API_IoDevShow(iShowType);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdBuss
** ��������: ϵͳ���� "buss"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdBuss (INT  iArgC, PCHAR  ppcArgV[])
{
    API_BusShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdFiles
** ��������: ϵͳ���� "files"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdFiles (INT  iArgC, PCHAR  ppcArgV[])
{
    API_IoFdShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdFdentrys
** ��������: ϵͳ���� "fdentrys"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdFdentrys (INT  iArgC, PCHAR  ppcArgV[])
{
    API_IoFdentryShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdOpen
** ��������: ϵͳ���� "open"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdOpen (INT  iArgC, PCHAR  ppcArgV[])
{
    INT          iFd;
    INT          iFlag = O_RDONLY;
    INT          iMode = DEFAULT_FILE_PERM;
    struct stat  statBuf;
    
    if (iArgC < 2) {
        return  (PX_ERROR);
    }
    
    if (iArgC == 3) {
        sscanf(ppcArgV[2], "%d", &iFlag);
    
    } else if (iArgC > 3) {
        sscanf(ppcArgV[2], "%x", &iFlag);                               /*  16 ����                     */
        sscanf(ppcArgV[3], "%o", &iMode);                               /*  8 ����                      */
    }
    
    iFd = open(ppcArgV[1], iFlag, iMode);
    if (iFd >= 0) {
        fstat(iFd, &statBuf);
        printf("open file return: %d dev %lx inode %lx size %llu\n",
               iFd, statBuf.st_dev, statBuf.st_ino, statBuf.st_size);
    
    } else {
        fprintf(stderr, "open file return: %d error: %s\n", iFd, lib_strerror(errno));
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdClose
** ��������: ϵͳ���� "close"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdClose (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iFd = PX_ERROR;

    if (iArgC != 2) {
        return  (PX_ERROR);
    } 
    
    sscanf(ppcArgV[1], "%d", &iFd);
    if (iFd >= 0) {
        close(iFd);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdClear
** ��������: ϵͳ���� "clear"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdClear (INT  iArgC, PCHAR  ppcArgV[])
{
    API_TShellScrClear(STD_OUT);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdExit
** ��������: ϵͳ���� "exit"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdExit (INT  iArgC, PCHAR  ppcArgV[])
{
    exit(0);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdVars
** ��������: ϵͳ���� "vars"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdVars (INT  iArgC, PCHAR  ppcArgV[])
{
    PCHAR   pcTitle = "\n"
                      "       VARIABLE      REF                       VALUE\n"
                      "-------------------- --- --------------------------------------------------\n";
    __PTSHELL_VAR   pskvNode[100];
    __PTSHELL_VAR   pskvNodeStart = LW_NULL;
    ULONG           ulNum;
    INT             i;
                      
    printf("variable show >>\n");
    printf(pcTitle);
    
    /*
     *  ��ɨ����ʾ��˲��, ���ܴ�����ɾ������.
     */
    do {
        ulNum = __tshellVarList(pskvNodeStart, pskvNode, 100);
        for (i = 0; i < ulNum; i++) {
            PCHAR  pcRef = "";
            if (pskvNode[i]->SV_ulRefCnt) {
                pcRef = "YES";
            }
            printf("%-20s %-3s %-50s\n", 
                   pskvNode[i]->SV_pcVarName, pcRef,
                   ((pskvNode[i]->SV_pcVarValue) ? (pskvNode[i]->SV_pcVarValue) : ("")));
        }
        pskvNodeStart = pskvNode[ulNum - 1];
    } while (ulNum >= 100);
    
    printf("\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdVarload
** ��������: ϵͳ���� "varload"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdVarload (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iError;
    PCHAR   pcFile;

    if (iArgC < 2) {
        pcFile = __TTINY_SHELL_VAR_FILE;
    } else {
        pcFile = ppcArgV[1];
    }
    
    iError = API_TShellVarLoad(pcFile);
    
    if (iError == ERROR_NONE) {
        API_TShellColorRefresh();                                       /*  ������ɫ����                */
        printf("environment variables load from %s success.\n", pcFile);
    } else {
        fprintf(stderr, "environment variables load from %s fail, error: %s\n",
                pcFile, lib_strerror(errno));
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdVarsave
** ��������: ϵͳ���� "varsave"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdVarsave (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iError;
    PCHAR   pcFile;
    
    if (iArgC < 2) {
        pcFile = __TTINY_SHELL_VAR_FILE;
    } else {
        pcFile = ppcArgV[1];
    }
    
    iError = API_TShellVarSave(pcFile);
    
    if (iError == ERROR_NONE) {
        printf("environment variables save to %s success.\n", pcFile);
    } else {
        fprintf(stderr, "environment variables save to %s fail, error: %s\n",
                pcFile, lib_strerror(errno));
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdColor
** ��������: ϵͳ���� "color"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdColor (INT  iArgC, PCHAR  ppcArgV[])
{
    API_TShellColorRefresh();                                           /*  ������ɫ����                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdSync
** ��������: ϵͳ���� "sync"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdSync (INT  iArgC, PCHAR  ppcArgV[])
{
    sync();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdTty
** ��������: ϵͳ���� "tty"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdTty (INT  iArgC, PCHAR  ppcArgV[])
{
    if (isatty(STD_OUT)) {
        printf("%s\n", ttyname(STD_OUT));
    } else {
        fprintf(stderr, "std file is not a tty device.\n");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdZones
** ��������: ϵͳ���� "zones"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

static INT  __tshellSysCmdZones (INT  iArgC, PCHAR  ppcArgV[])
{
    API_VmmPhysicalShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdVirtuals
** ��������: ϵͳ���� "virtuals"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdVirtuals (INT  iArgC, PCHAR  ppcArgV[])
{
    API_VmmVirtualShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdAborts
** ��������: ϵͳ���� "aborts"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdAborts (INT  iArgC, PCHAR  ppcArgV[])
{
    API_VmmAbortShow();
    
    return  (ERROR_NONE);
}

/*********************************************************************************************************
** ��������: __tshellSysCmdPageFaultLimit
** ��������: ϵͳ���� "pagefaultlimit"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellSysCmdPageFaultLimit (INT  iArgC, PCHAR  ppcArgV[])
{
    LW_VMM_PAGE_FAULT_LIMIT  vpfl;
    size_t stRootWarn, stRootMin, stUserWarn, stUserMin;

    lib_bzero(&vpfl, sizeof(LW_VMM_PAGE_FAULT_LIMIT));

    if (iArgC != 5) {
        API_VmmPageFaultLimit(LW_NULL, &vpfl);
        stRootWarn = vpfl.VPFL_ulRootWarnPages * LW_CFG_VMM_PAGE_SIZE;
        stRootMin  = vpfl.VPFL_ulRootMinPages  * LW_CFG_VMM_PAGE_SIZE;
        stUserWarn = vpfl.VPFL_ulUserWarnPages * LW_CFG_VMM_PAGE_SIZE;
        stUserMin  = vpfl.VPFL_ulUserMinPages  * LW_CFG_VMM_PAGE_SIZE;
        printf("Page fault Warning: %zu MB (root) %zu MB (user)\n",
               stRootWarn / LW_CFG_MB_SIZE, stUserWarn / LW_CFG_MB_SIZE);
        printf("Page fault limit  : %zu MB (root) %zu MB (user)\n",
               stRootMin / LW_CFG_MB_SIZE, stUserMin / LW_CFG_MB_SIZE);

    } else {
        if (sscanf(ppcArgV[1], "%zu", &stRootWarn) != 1) {
            goto    __error;
        }
        if (sscanf(ppcArgV[2], "%zu", &stRootMin) != 1) {
            goto    __error;
        }
        if (sscanf(ppcArgV[3], "%zu", &stUserWarn) != 1) {
            goto    __error;
        }
        if (sscanf(ppcArgV[4], "%zu", &stUserMin) != 1) {
            goto    __error;
        }

        vpfl.VPFL_ulRootWarnPages = (stRootWarn * LW_CFG_MB_SIZE) / LW_CFG_VMM_PAGE_SIZE;
        vpfl.VPFL_ulRootMinPages  = (stRootMin  * LW_CFG_MB_SIZE) / LW_CFG_VMM_PAGE_SIZE;
        vpfl.VPFL_ulUserWarnPages = (stUserWarn * LW_CFG_MB_SIZE) / LW_CFG_VMM_PAGE_SIZE;
        vpfl.VPFL_ulUserMinPages  = (stUserMin  * LW_CFG_MB_SIZE) / LW_CFG_VMM_PAGE_SIZE;
        if (API_VmmPageFaultLimit(&vpfl, LW_NULL)) {
            if (errno == EACCES) {
                fprintf(stderr, "insufficient permissions.\n");
            } else {
                fprintf(stderr, "Page fault limit set error: %s\n", lib_strerror(errno));
            }
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);

__error:
    fprintf(stderr, "argument error.\n");
    return  (PX_ERROR);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** ��������: __tshellSysCmdRestart
** ��������: ϵͳ���� "restart"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_THREAD_RESTART_EN > 0

static INT  __tshellSysCmdRestart (INT  iArgC, PCHAR  ppcArgV[])
{
    LW_OBJECT_HANDLE        ulId  = LW_OBJECT_HANDLE_INVALID;
    ULONG                   ulArg = 0ul;

    if (iArgC != 3) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    sscanf(ppcArgV[1], "%lx", &ulId);
    sscanf(ppcArgV[2], "%lu", &ulArg);
    
    if (API_ThreadRestart(ulId, (PVOID)ulArg) == ERROR_NONE) {
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_THREAD_RESTART_EN    */
/*********************************************************************************************************
** ��������: __tshellSysCmdSprio
** ��������: ϵͳ���� "sprio"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdSprio (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iPrio = PX_ERROR;
    ULONG   ulId  = LW_OBJECT_HANDLE_INVALID;

    if (iArgC != 3) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    sscanf(ppcArgV[1], "%d",  &iPrio);
    sscanf(ppcArgV[2], "%lx", &ulId);
    
    if ((iPrio <  API_KernelGetPriorityMin()) || 
        (iPrio >= API_KernelGetPriorityMax())) {
        fprintf(stderr, "priority invalidate.\n");
        return  (PX_ERROR);
    }
    
    if (API_ThreadSetPriority(ulId, (UINT8)iPrio) != ERROR_NONE) {
        fprintf(stderr, "can not set thread priority error: %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdRenice
** ��������: ϵͳ���� "renice"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdRenice (INT  iArgC, PCHAR  ppcArgV[])
{
#if LW_CFG_POSIX_EN > 0
    id_t    lId   = 0;
    INT     iNice = 0;
    INT     iPrio;
    INT     iWhich;
    INT     iRet;
    
    struct passwd       passwd;
    struct passwd      *ppasswd = LW_NULL;
    
    CHAR                cUserName[MAX_FILENAME_LENGTH];
    
    if (iArgC < 3) {
__arg_error:
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    iNice = lib_atoi(ppcArgV[1]);
    
    if (lib_strcmp(ppcArgV[2], "-p") == 0) {                            /*  PRIO_PROCESS                */
        iWhich = PRIO_PROCESS;
        
        if (iArgC < 4) {
            goto    __arg_error;
        }
        
        lId = lib_atol(ppcArgV[3]);
        
    } else if (lib_strcmp(ppcArgV[2], "-g") == 0) {                     /*  PRIO_PGRP                   */
        iWhich = PRIO_PGRP;
        
        if (iArgC < 4) {
            goto    __arg_error;
        }
        
        lId = lib_atol(ppcArgV[3]);
        
    } else if (lib_strcmp(ppcArgV[2], "-u") == 0) {                     /*  PRIO_USER                   */
        iWhich = PRIO_USER;
        
        if (iArgC < 4) {
            goto    __arg_error;
        }
        
        getpwnam_r(ppcArgV[3], &passwd, cUserName, sizeof(cUserName), &ppasswd);
        if (ppasswd == LW_NULL) {
            goto    __arg_error;
        }
        
        lId = (id_t)ppasswd->pw_uid;
        
    } else {                                                            /*  PRIO_PROCESS                */
        iWhich = PRIO_PROCESS;
        lId = lib_atol(ppcArgV[2]);
    }
    
    iPrio = getpriority(iWhich, lId);
    iRet  = setpriority(iWhich, lId, iPrio + iNice);
    
    if (iRet < ERROR_NONE) {
        fprintf(stderr, "set priority error: %s.\n", lib_strerror(errno));
    }
    return  (iRet);
    
#else
    fprintf(stderr, "do not support posix interface.\n");
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** ��������: __tshellSysCmdLogFileAdd
** ��������: ϵͳ���� "logfileadd"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_LOG_LIB_EN > 0

static INT  __tshellSysCmdLogFileAdd (INT  iArgC, PCHAR  ppcArgV[])
{
    fd_set      fdsetLog;
    INT         iWidth = 0;
    INT         iFdNew = PX_ERROR;

    if (iArgC != 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    API_LogFdGet(&iWidth, &fdsetLog);                                   /*  �����ǰ log �ļ�������     */
    
    sscanf(ppcArgV[1], "%d", &iFdNew);
    if (iFdNew == PX_ERROR) {
        fprintf(stderr, "file error.\n");
        return  (PX_ERROR);
    }
    
    FD_SET(iFdNew, &fdsetLog);
    if (iFdNew >= iWidth) {                                             /*  �ж��ļ�������ļ����      */
        iWidth = iFdNew + 1;
    }
    
    API_LogFdSet(iWidth, &fdsetLog);                                    /*  �����µ��ļ���              */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdLogFileClear
** ��������: ϵͳ���� "logfileclear"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdLogFileClear (INT  iArgC, PCHAR  ppcArgV[])
{
    fd_set      fdsetLog;
    INT         iWidth = 0;
    INT         iFdNew = PX_ERROR;

    if (iArgC != 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    API_LogFdGet(&iWidth, &fdsetLog);                                   /*  �����ǰ log �ļ�������     */
    
    sscanf(ppcArgV[1], "%d", &iFdNew);
    if (iFdNew == PX_ERROR) {
        fprintf(stderr, "file error.\n");
        return  (PX_ERROR);
    }
    
    FD_CLR(iFdNew, &fdsetLog);
    
    /*
     *  �ݲ����� Width.
     */
    API_LogFdSet(iWidth, &fdsetLog);                                    /*  �����µ��ļ���              */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdLogFiles
** ��������: ϵͳ���� "logfiles"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdLogFiles (INT  iArgC, PCHAR  ppcArgV[])
{
#define __LW_LOG_FD_EACH_LINE_NUM       5

             fd_set     fdsetLog;
             INT        iWidth = 0;
             INT        iFdCounter = 0;
    REGISTER INT        iFdTemp;
    REGISTER ULONG      ulPartMask;
    
    API_LogFdGet(&iWidth, &fdsetLog);                                   /*  �����ǰ log �ļ�������     */
    
    printf("log fd(s) include : \n");
    for (iFdTemp = 0; iFdTemp < iWidth; iFdTemp++) {                    /*  ������п�ִ�ж��������ļ�  */
        ulPartMask = 
        fdsetLog.fds_bits[((unsigned)iFdTemp) / NFDBITS];               /*  ��� iFdTemp ���ڵ�������   */
        
        if (ulPartMask == 0) {                                          /*  �������������ļ��޹�      */
            iFdTemp += NFDBITS - 1;                                     /*  ������һ���������ж�        */
        } else if (ulPartMask & (ULONG)(1 << (((unsigned)iFdTemp) % NFDBITS))) {
            iFdCounter++;
            if (iFdCounter > __LW_LOG_FD_EACH_LINE_NUM) {
                printf("%5d\n", iFdTemp);
            } else {
                printf("%5d    ", iFdTemp);
            }
        }
    }
    printf("\n");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdLogLevel
** ��������: ϵͳ���� "loglevel"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdLogLevel (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iNewLogLevel = console_loglevel;

    if (iArgC != 2) {
        printf("printk() log message current level is: %d\n", console_loglevel);
        return  (ERROR_NONE);
    }
    
    sscanf(ppcArgV[1], "%d", &iNewLogLevel);
    if (iNewLogLevel != console_loglevel) {
        console_loglevel = iNewLogLevel;                                /*  �������� printk ��ӡ�ȼ�    */
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */
/*********************************************************************************************************
** ��������: __tshellSysCmdHwclock
** ��������: ϵͳ���� "hwclock"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

static INT  __tshellSysCmdHwclock (INT  iArgC, PCHAR  ppcArgV[])
{
    time_t      time;
    CHAR        cTimeBuffer[32];

    if (iArgC != 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    if (lib_strcmp("--show", ppcArgV[1]) == 0) {                        /*  ��ʾӲ��ʱ��                */
        if (API_RtcGet(&time) < 0) {
            fprintf(stderr, "can not get RTC info. error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        } else {
            printf("%s", lib_ctime_r(&time, cTimeBuffer));
        }
    
    } else if (lib_strcmp("--hctosys", ppcArgV[1]) == 0) {              /*  Ӳ��ʱ����ϵͳʱ��ͬ��      */
        if (API_RtcToSys() < 0) {
            fprintf(stderr, "can not sync RTC to system clock. error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
    
    } else if (lib_strcmp("--systohc", ppcArgV[1]) == 0) {              /*  ϵͳʱ����Ӳ��ʱ��ͬ��      */
        if (API_SysToRtc() < 0) {
            fprintf(stderr, "can not sync system clock to RTC. error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
    
    } else {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_RTC_EN > 0           */
/*********************************************************************************************************
** ��������: __tshellSysCmdShStack
** ��������: ϵͳ���� "shstack"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdShStack (INT  iArgC, PCHAR  ppcArgV[])
{
    size_t  stSize = 0;

    if (iArgC < 2) {
        API_TShellSetStackSize(0, &stSize);
        printf("default shell stack: %zd\n", stSize);
        
    } else {
        sscanf(ppcArgV[1], "%zd", &stSize);
        API_TShellSetStackSize(stSize, LW_NULL);
        printf("default shell stack: %zd\n", stSize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdCrypt
** ��������: ϵͳ���� "crypt"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SHELL_PASS_CRYPT_EN > 0

static INT  __tshellSysCmdCrypt (INT  iArgC, PCHAR  ppcArgV[])
{
    PCHAR   pcRes;
    CHAR    cResBuf[PASS_MAX];

    if (iArgC != 3) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    pcRes = crypt_safe(ppcArgV[1], ppcArgV[2], cResBuf, sizeof(cResBuf));
    if (pcRes) {
        printf("%s\n", pcRes);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SHELL_PASS_CRYPT_EN  */
/*********************************************************************************************************
** ��������: __tshellSysCmdHostname
** ��������: ϵͳ���� "hostname"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdHostname (INT  iArgC, PCHAR  ppcArgV[])
{
    CHAR    cHostName[HOST_NAME_MAX + 1];

    if (iArgC < 2) {
        gethostname(cHostName, sizeof(cHostName));
        printf("hostname is %s\n", cHostName);
    
    } else {
        if (sethostname(ppcArgV[1], lib_strlen(ppcArgV[1]))) {
            fprintf(stderr, "set hostname error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdLogin
** ��������: ϵͳ���� "login"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdLogin (INT  iArgC, PCHAR  ppcArgV[])
{
    __tshellUserAuthen(STDIN_FILENO, LW_TRUE);
    
    printf("\n");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdLogin
** ��������: ϵͳ���� "logout"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdLogout (INT  iArgC, PCHAR  ppcArgV[])
{
    API_TShellScrClear(STD_OUT);

    if (API_TShellLogout()) {
        fprintf(stderr, "error occur: %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdWho
** ��������: ϵͳ���� "who"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdWho (INT  iArgC, PCHAR  ppcArgV[])
{
    CHAR        cUserName[MAX_FILENAME_LENGTH];

    struct passwd   passwd;
    struct passwd  *ppasswd = LW_NULL;
    
    getpwuid_r(getuid(), &passwd, cUserName, sizeof(cUserName), &ppasswd);
    if (ppasswd) {
        printf("user:%s terminal:%s uid:%d gid:%d euid:%d egid:%d\n",
               ppasswd->pw_name,
               ttyname(STD_OUT),
               getuid(),
               getgid(),
               geteuid(),
               getegid());
    
    } else {
        printf("user:%s terminal:%s uid:%d gid:%d euid:%d egid:%d\n",
               "unknown",
               ttyname(STD_OUT),
               getuid(),
               getgid(),
               geteuid(),
               getegid());
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdShutdown
** ��������: ϵͳ���� "shutdown"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdShutdown (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC < 2) {
        printf("[shutdown]Shutdown...\n");
        API_KernelReboot(LW_REBOOT_SHUTDOWN);
        
    } else {
        if (lib_strcmp(ppcArgV[1], "-r") == 0) {
            printf("[shutdown]Shutdown & reboot...\n");
            API_KernelReboot(LW_REBOOT_COLD);
            
        } else if (lib_strcmp(ppcArgV[1], "-h") == 0) {
            printf("[shutdown]Shutdown...\n");
            API_KernelReboot(LW_REBOOT_SHUTDOWN);
            
        } else if (lib_strcmp(ppcArgV[1], "-f") == 0) {
            printf("[shutdown]Force shutdown and reboot...\n");
            API_KernelReboot(LW_REBOOT_FORCE);

        } else {
            fprintf(stderr, "argument error.\n");
            return  (PX_ERROR);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdReboot
** ��������: ϵͳ���� "reboot"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdReboot (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC < 2) {
        printf("[reboot]Reboot...\n");
        API_KernelReboot(LW_REBOOT_COLD);
        
    } else {
        if ((lib_strcmp(ppcArgV[1], "-n") == 0) ||
            (lib_strcmp(ppcArgV[1], "-f") == 0)) {
            printf("[reboot]Force reboot...\n");
            API_KernelReboot(LW_REBOOT_FORCE);
        
        } else {
            fprintf(stderr, "argument error.\n");
            return  (PX_ERROR);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdMonitor
** ��������: ϵͳ���� "monitor"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0

static INT  __tshellSysCmdMonitor (INT  iArgC, PCHAR  ppcArgV[])
{
    static PVOID    pvMonitor = LW_NULL;
    static BOOL     bIsNet;
    static CHAR     pcFileOrIp[MAX_FILENAME_LENGTH];
    static UINT16   usPort;
    static UINT64   u64SubEventAllow[MONITOR_EVENT_ID_MAX + 1];         /*  �¼��˲���                  */
    
           PCHAR    pcDiv;
    
    if (iArgC == 1) {
        if (pvMonitor == LW_NULL) {
            printf("monitor stopped now.\n");
        } else {
            if (bIsNet) {
                printf("monitor update ip: %s port: %d\n", pcFileOrIp, usPort);
            } else {
                printf("monitor update path: %s\n", pcFileOrIp);
            }
        }
        return  (ERROR_NONE);
    }
    
    if (lib_strcmp(ppcArgV[1], "start") == 0) {                         /*  ���������                  */
        INT     i;
        
        if (pvMonitor) {
            printf("monitor has been already started.\n");
            return  (ERROR_NONE);
        }
        if (iArgC < 3) {
            goto    __inval_args;
        }
        
        pcDiv = lib_strchr(ppcArgV[2], ':');
        
        if (pcDiv) {                                                    /*  ���� upload                 */
            lib_memcpy(pcFileOrIp, ppcArgV[2], pcDiv - ppcArgV[2]);
            pcFileOrIp[pcDiv - ppcArgV[2]] = PX_EOS;
            usPort = (UINT16)lib_atoi(&pcDiv[1]);
            
            if (!usPort) {
                goto    __inval_args;
            }
            
#if LW_CFG_NET_EN > 0
            pvMonitor = API_MonitorNetUploadCreate(pcFileOrIp, usPort,
                                                   16 * LW_CFG_KB_SIZE, LW_NULL);
#else
            pvMonitor = LW_NULL;
            errno     = ENOSYS;
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
            if (!pvMonitor) {
                fprintf(stderr, "can not create monitor net upload path error: %s.\n", 
                        lib_strerror(errno));
                return  (PX_ERROR);
            }
            bIsNet = LW_TRUE;
            
        } else {
            lib_strlcpy(pcFileOrIp, ppcArgV[2], MAX_FILENAME_LENGTH);
        
#if LW_CFG_DEVICE_EN > 0
            pvMonitor = API_MonitorFileUploadCreate(pcFileOrIp, O_WRONLY | O_CREAT | O_APPEND, 0666,
                                                    16 * LW_CFG_KB_SIZE, 0, LW_NULL);
#else
            pvMonitor = LW_NULL;
            errno     = ENOSYS;
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
            if (!pvMonitor) {
                fprintf(stderr, "can not create monitor file upload path error: %s.\n", 
                        lib_strerror(errno));
                return  (PX_ERROR);
            }
            bIsNet = LW_FALSE;
        }
        
        for (i = 0; i < MONITOR_EVENT_ID_MAX; i++) {
            API_MonitorUploadSetFilter(pvMonitor, (UINT32)i, u64SubEventAllow[i], 
                                       LW_MONITOR_UPLOAD_SET_EVT_SET);
        }
        
        API_MonitorUploadEnable(pvMonitor);
        
        printf("monitor start.\n");
        return  (ERROR_NONE);
    
    } else if (lib_strcmp(ppcArgV[1], "stop") == 0) {                   /*  ֹͣ�����                  */
        if (!pvMonitor) {
            printf("monitor has been already stopped.\n");
            return  (ERROR_NONE);
        }
        
        if (bIsNet) {
#if LW_CFG_NET_EN > 0
            if (API_MonitorNetUploadDelete(pvMonitor)) {
                fprintf(stderr, "can not stop net upload monitor error: %s.\n", 
                        lib_strerror(errno));
                return  (PX_ERROR);
            
            } else {
                pvMonitor = LW_NULL;
            }
#else
            pvMonitor = LW_NULL;
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
        
        } else {
#if LW_CFG_DEVICE_EN > 0
            if (API_MonitorFileUploadDelete(pvMonitor)) {
                fprintf(stderr, "can not stop file upload monitor error: %s.\n", 
                        lib_strerror(errno));
                return  (PX_ERROR);
            
            } else {
                pvMonitor = LW_NULL;
            }
#else
            pvMonitor = LW_NULL;
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
        }
        
        printf("monitor stop.\n");
        return  (ERROR_NONE);
    
    } else if (lib_strcmp(ppcArgV[1], "filter") == 0) {                 /*  ���ü�����˲���            */
        UINT    uiEvent  = MONITOR_EVENT_ID_MAX + 1;
        UINT64  u64Allow = 0;
        
        if (iArgC < 3) {
            INT     i;
            INT     iNum = 0;
            for (i = 0; i < MONITOR_EVENT_ID_MAX; i++) {
                if (u64SubEventAllow[i]) {
                    printf("%d : %016llx ", i, u64SubEventAllow[i]);
                    iNum++;
                    if (iNum > 1) {
                        iNum = 0;
                        printf("\n");
                    }
                }
            }
            printf("\n");
            return  (ERROR_NONE);
        }
        
        if (iArgC < 4) {
            goto    __inval_args;
        }
        
        if (sscanf(ppcArgV[2], "%u", &uiEvent) != 1) {
            goto    __inval_args;
        }
        
        if (sscanf(ppcArgV[3], "%llx", &u64Allow) != 1) {
            goto    __inval_args;
        }
        
        if (uiEvent > MONITOR_EVENT_ID_MAX) {
            goto    __inval_args;
        }
        
        u64SubEventAllow[uiEvent] = u64Allow;
        
        if (pvMonitor) {
            API_MonitorUploadSetFilter(pvMonitor, (UINT32)uiEvent, u64Allow,
                                       LW_MONITOR_UPLOAD_SET_EVT_SET);
        }
        
        return  (ERROR_NONE);
    
    } else if (lib_strcmp(ppcArgV[1], "pid") == 0) {
        pid_t  pid = 0;
        
        if (!pvMonitor) {
            printf("monitor has been already stopped.\n");
            return  (ERROR_NONE);
        }
        
        if (iArgC < 3) {
            API_MonitorUploadGetPid(pvMonitor, &pid);
            printf("monitor pid is %d\n", pid);
            return  (ERROR_NONE);
        }
    
        pid = lib_atoi(ppcArgV[2]);
        API_MonitorUploadSetPid(pvMonitor, pid);
        
        return  (ERROR_NONE);
    }
    
__inval_args:
    fprintf(stderr, "argument error.\n");
    return  (PX_ERROR);
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
/*********************************************************************************************************
** ��������: __tshellSysCmdLspci
** ��������: ϵͳ���� "pcis"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSysCmdPcis (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iFd;
    CHAR    cBuffer[512];
    ssize_t sstNum;
    
    iFd = open("/proc/pci", O_RDONLY);
    if (iFd < 0) {
        fprintf(stderr, "can not open /proc/pci : %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    do {
        sstNum = read(iFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iFd);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSysCmdAffinity
** ��������: ϵͳ���� "affinity"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_POSIX_EN > 0)

static INT  __tshellSysCmdAffinity (INT  iArgC, PCHAR  ppcArgV[])
{
    LW_OBJECT_HANDLE  ulId = LW_OBJECT_HANDLE_INVALID;
    cpu_set_t         cpuset;
    BOOL              bProcess = LW_FALSE;
    PCHAR             pcTarget;
    ULONG             ulCPUId;
    INT               iRet;
    
    CPU_ZERO(&cpuset);
    
    if (iArgC != 3) {
        INT     iFd;
        CHAR    cBuffer[512];
        ssize_t sstNum;
        
        iFd = open("/proc/kernel/affinity", O_RDONLY);
        if (iFd < 0) {
            fprintf(stderr, "can not open /proc/kernel/affinity : %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        do {
            sstNum = read(iFd, cBuffer, sizeof(cBuffer));
            if (sstNum > 0) {
                write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
            }
        } while (sstNum > 0);
        
        close(iFd);
        
        return  (ERROR_NONE);
    }
    
    if (ppcArgV[1][0] < '0' ||
        ppcArgV[1][0] > '9') {
        fprintf(stderr, "option error.\n");
        return  (PX_ERROR);
    }
    
    sscanf(ppcArgV[1], "%lx", &ulId);
    if (API_ObjectGetClass(ulId) != _OBJECT_THREAD) {
        sscanf(ppcArgV[1], "%ld", &ulId);                               /*  ���� id                     */
        bProcess = LW_TRUE;
        pcTarget = "process";
    
    } else {
        pcTarget = "thread";
    }
    
    if (ppcArgV[2][0] == 'c') {
        if (bProcess) {
            iRet = sched_setaffinity((pid_t)ulId, sizeof(cpu_set_t), &cpuset);
        } else {
            iRet = pthread_setaffinity_np((pthread_t)ulId, sizeof(cpu_set_t), &cpuset);
        }
        if (iRet == ERROR_NONE) {
            printf("affinity clear %s 0x%lx ok.\n", pcTarget, ulId);
        } else {
            fprintf(stderr, "affinity clear %s 0x%lx fail: %s.\n", 
                    pcTarget, ulId, lib_strerror(errno));
        }
    
    } else {
        sscanf(ppcArgV[2], "%ld", &ulCPUId);
        CPU_SET(ulCPUId, &cpuset);
        if (bProcess) {
            iRet = sched_setaffinity((pid_t)ulId, sizeof(cpu_set_t), &cpuset);
        } else {
            iRet = pthread_setaffinity_np((pthread_t)ulId, sizeof(cpu_set_t), &cpuset);
        }
        if (iRet == ERROR_NONE) {
            printf("affinity set %s 0x%lx to cpu %ld ok.\n", 
                   pcTarget, ulId, ulCPUId);
        } else {
            fprintf(stderr, "affinity set %s 0x%lx to cpu %ld fail: %s.\n", 
                    pcTarget, ulId, ulCPUId, lib_strerror(errno));
        }
    }
        
    if (iRet) {
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
                                                                        /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
** ��������: __tshellSysCmdCpuAffinity
** ��������: ϵͳ���� "cpuaffinity"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static INT  __tshellSysCmdCpuAffinity (INT  iArgC, PCHAR  ppcArgV[])
{
    INT             iId;
    INT             iEn;
    LW_CLASS_CPUSET cpuset;

    if (iArgC != 3) {
        INT     iFd;
        CHAR    cBuffer[512];
        ssize_t sstNum;
        
        iFd = open("/proc/kernel/cpu_affinity", O_RDONLY);
        if (iFd < 0) {
            fprintf(stderr, "can not open /proc/kernel/cpu_affinity : %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        do {
            sstNum = read(iFd, cBuffer, sizeof(cBuffer));
            if (sstNum > 0) {
                write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
            }
        } while (sstNum > 0);
        
        close(iFd);
        
        return  (ERROR_NONE);
    }
    
    if (sscanf(ppcArgV[1], "%d", &iId) != 1) {
        fprintf(stderr, "option error.\n");
        return  (PX_ERROR);
    }
    
    if (sscanf(ppcArgV[2], "%d", &iEn) != 1) {
        fprintf(stderr, "option error.\n");
        return  (PX_ERROR);
    }
    
    if (iId == 0) {
        fprintf(stderr, "CPU 0 can not set or clear strongly affinity schedule.\n");
        return  (PX_ERROR);
    }
    
    if (iId >= LW_NCPUS) {
        fprintf(stderr, "CPU ID invalid.\n");
        return  (PX_ERROR);
    }
    
    if (API_CpuGetSchedAffinity(sizeof(LW_CLASS_CPUSET), &cpuset)) {
        fprintf(stderr, "CPU strongly affinity schedule get fail: %s.\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    if (iEn && !LW_CPU_ISSET(iId, &cpuset)) {
        LW_CPU_SET(iId, &cpuset);
        
    } else if (!iEn && LW_CPU_ISSET(iId, &cpuset)) {
        LW_CPU_CLR(iId, &cpuset);
    
    } else {
        return  (ERROR_NONE);
    }
    
    if (API_CpuSetSchedAffinity(sizeof(LW_CLASS_CPUSET), &cpuset)) {
        fprintf(stderr, "CPU strongly affinity schedule set fail: %s.\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: __tshellSysCmdCdump
** ��������: ϵͳ���� "cdump"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)

static INT  __tshellSysCmdCdump (INT  iArgC, PCHAR  ppcArgV[])
{
    INT         iC;
    BOOL        bSave  = LW_FALSE;
    BOOL        bClear = LW_FALSE;
    CHAR        cFile[MAX_FILENAME_LENGTH] = "/var/log/cdump/";
    time_t      tm;
    
    while ((iC = getopt(iArgC, ppcArgV, "sc")) != EOF) {
        switch (iC) {
        
        case 's':
            bSave = LW_TRUE;
            break;
            
        case 'c':
            bClear = LW_TRUE;
            break;
        }
    }
    
    getopt_free();
    
    if (bSave) {
        lib_time(&tm);
        snprintf(cFile, MAX_FILENAME_LENGTH, "%s/%lld",
                 "/var/log/cdump", tm);
        
        if (API_CrashDumpSave(cFile, O_CREAT | O_WRONLY | O_TRUNC, 
                              DEFAULT_FILE_PERM, bClear) < ERROR_NONE) {
            if (errno == EMSGSIZE) {
                fprintf(stderr, "no message in crash dump buffer.\n");
            
            } else {
                fprintf(stderr, "crash dump save (%s) fail: %s.\n", cFile, lib_strerror(errno));
            }
            
            return  (PX_ERROR);
            
        } else {
            printf("crash dump save (%s) ok.\n", cFile);
        }
    
    } else {
        if (API_CrashDumpShow(STD_OUT, bClear) < ERROR_NONE) {
            if (errno == EMSGSIZE) {
                fprintf(stderr, "no message in crash dump buffer.\n");
            }
            
            return  (PX_ERROR);
        }
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CDUMP_EN > 0         */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: __tshellSysCmdCrashTrap
** ��������: ϵͳ���� "crashtrap"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0

static INT  __tshellSysCmdCrashTrap (INT  iArgC, PCHAR  ppcArgV[])
{
    pid_t  pid;
    INT    iEnable;
    INT    iFlags;

    if (iArgC == 2) {
        if (sscanf(ppcArgV[1], "%d", &pid) != 1) {
            fprintf(stderr, "pid error.\n");
            return  (PX_ERROR);
        }

        if (vprocDebugFlagsGet(pid, &iFlags)) {
            fprintf(stderr, "error: %s.\n", lib_strerror(errno));
            return  (PX_ERROR);
        }

        printf("Crash Trap is: %s.\n", (iFlags & LW_VPROC_DEBUG_TRAP) ? "Enable" : "Disable");

    } else if (iArgC == 3) {
        if (sscanf(ppcArgV[1], "%d", &pid) != 1) {
            fprintf(stderr, "pid error.\n");
            return  (PX_ERROR);
        }

        if (sscanf(ppcArgV[2], "%d", &iEnable) != 1) {
            fprintf(stderr, "option error.\n");
            return  (PX_ERROR);
        }

        iFlags = iEnable ? LW_VPROC_DEBUG_TRAP : LW_VPROC_DEBUG_NORMAL;
        if (vprocDebugFlagsSet(pid, iFlags)) {
            fprintf(stderr, "error: %s.\n", lib_strerror(errno));
            return  (PX_ERROR);
        }

    } else {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
** ��������: __tshellSysCmdInit
** ��������: ��ʼ��ϵͳ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __tshellSysCmdInit (VOID)
{
    API_TShellKeywordAdd("args", __tshellSysCmdArgs);
    API_TShellFormatAdd("args", " [any argument...]");
    API_TShellHelpAdd("args", "show all argument.\n");

    API_TShellKeywordAdd("echo", __tshellSysCmdEcho);
    API_TShellFormatAdd("echo", " [message]");
    API_TShellHelpAdd("echo", "echo the input command.\n"
                              "echo [message]\n");
                              
    API_TShellKeywordAdd("sleep", __tshellSysCmdSleep);
    API_TShellFormatAdd("sleep", " [<n>{s|m|h|d}]");
    API_TShellHelpAdd("sleep", "sleep specific time.\n");
                              
    API_TShellKeywordAdd("shell", __tshellSysCmdShell);
    API_TShellFormatAdd("shell", " [tty device[:hwopt]] [nologin]");
    API_TShellHelpAdd("shell", "create shell use [tty device] as standard file.\n"
                              "shell /dev/ttyS1\n"
                              "shell /dev/ttyS1 nologin\n"
                              "shell /dev/ttyS1:115200,n,8,1\n"
                              "shell /dev/ttyS1:9600,e,8,1 nologin\n");
                              
    API_TShellKeywordAdd("help", __tshellSysCmdHelp);
    API_TShellFormatAdd("help", " [-s keyword]");
    API_TShellHelpAdd("help", "display help information.\n"
                              "help [-s keyword]\n");
                              
    API_TShellKeywordAdd("vars", __tshellSysCmdVars);
    API_TShellHelpAdd("vars", "show current environment.\n");
    
    API_TShellKeywordAdd("env", __tshellSysCmdVars);
    API_TShellHelpAdd("env", "show current environment.\n");
    
    API_TShellKeywordAdd("varload", __tshellSysCmdVarload);
    API_TShellFormatAdd("varload", " [profile]");
    API_TShellHelpAdd("varload", "synchronize environment variables from profile.\n");
    
    API_TShellKeywordAdd("varsave", __tshellSysCmdVarsave);
    API_TShellFormatAdd("varsave", " [profile]");
    API_TShellHelpAdd("varsave", "synchronize environment variables to profile.\n");

    API_TShellKeywordAdd("free", __tshellSysCmdFree);
    API_TShellHelpAdd("free", "show system memory information.\n");

    API_TShellKeywordAdd("vardel", __tshellSysCmdVardel);
    API_TShellFormatAdd("vardel", " [variable]");
    API_TShellHelpAdd("vardel", "delete a variable.\n");
                              
    API_TShellKeywordAdd("msgq", __tshellSysCmdMsgq);
    API_TShellFormatAdd("msgq", " message queue handle");
    API_TShellHelpAdd("msgq", "show message queue information.\n");
    
    API_TShellKeywordAdd("eventset", __tshellSysCmdEventSet);
    API_TShellFormatAdd("eventset", " eventset handle");
    API_TShellHelpAdd("eventset", "show eventset information.\n");
    
    API_TShellKeywordAdd("part", __tshellSysCmdPart);
    API_TShellFormatAdd("part", " partition handle");
    API_TShellHelpAdd("part", "show partition information.\n");
    
    API_TShellKeywordAdd("sem", __tshellSysCmdSem);
    API_TShellFormatAdd("sem", " semaphore handle");
    API_TShellHelpAdd("sem", "show semaphore information.\n");
    
    API_TShellKeywordAdd("ts", __tshellSysCmdTs);
    API_TShellFormatAdd("ts", " [pid]");
    API_TShellHelpAdd("ts", "show thread information.\n"
                            "you can and pid argument to determine which process you want to see.\n");
                            
    API_TShellKeywordAdd("tp", __tshellSysCmdTp);
    API_TShellFormatAdd("tp", " [pid]");
    API_TShellHelpAdd("tp", "show thread pending information.\n"
                            "you can and pid argument to determine which process you want to see.\n");
    
    API_TShellKeywordAdd("wjs", __tshellSysCmdWjs);
    API_TShellFormatAdd("wjs", " [pid]");
    API_TShellHelpAdd("wjs", "show thread has been deleted but not recycling (need pthread_join()).\n"
                            "you can and pid argument to determine which process you want to see.\n");

    API_TShellKeywordAdd("ss", __tshellSysCmdSs);
    API_TShellHelpAdd("ss", "show all stack information.\n");
    
    API_TShellKeywordAdd("cpuus", __tshellSysCmdCpuus);
    API_TShellFormatAdd("cpuus", " [-n times] [-t wait_seconds]");
    API_TShellHelpAdd("cpuus", "show cpu usage information, wait_seconds max is 10s.\n");
    
#ifdef LW_CFG_CPU_ARCH_X86
    API_TShellKeywordAdd("x86", __tshellSysCmdX86);
    API_TShellFormatAdd("x86", " [cpuid | cputop | mps | bios | ioint | loint | acpi]");
    API_TShellHelpAdd("x86", "show x86 cpu information.\n");
#endif                                                                  /*  LW_CFG_CPU_ARCH_X86         */

    API_TShellKeywordAdd("top", __tshellSysCmdCpuus);
    API_TShellFormatAdd("top", " [-n times] [-t wait_seconds]");
    API_TShellHelpAdd("top", "show cpu usage information, wait_seconds max is 10s.\n");
    
    API_TShellKeywordAdd("ints", __tshellSysCmdInts);
    API_TShellFormatAdd("ints", " [cpuid start] [cpuid end]");
    API_TShellHelpAdd("ints", "show system interrupt vecter information.\n");
    
    API_TShellKeywordAdd("mems", __tshellSysCmdMems);
    API_TShellFormatAdd("mems", " [rid]");
    API_TShellHelpAdd("mems", "show region(heap) information.\n"
                              " if no [rid] is show system heap information.\n"
                              "notice : rid is HEX.\n");
    
#if LW_CFG_SIGNAL_EN > 0
    API_TShellKeywordAdd("kill", __tshellSysCmdKill);
    API_TShellFormatAdd("kill", " [-n signum tid/pid] | [tid/pid]");
    API_TShellHelpAdd("kill", "kill a thread or send a signal to a thread.\n"
                              "notice : tid is HEX, pid is DEC.\n");
    
    API_TShellKeywordAdd("sigqueue", __tshellSysCmdSigqueue);
    API_TShellFormatAdd("sigqueue", " [-n signum tid/pid] | [tid/pid]");
    API_TShellHelpAdd("sigqueue", "sigqueue a thread or send a signal to a thread.\n"
                                  "notice : tid is HEX, pid is DEC.\n");
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
    
    API_TShellKeywordAdd("times", __tshellSysCmdTimes);
    API_TShellFormatAdd("times", " [-utc]");
    API_TShellHelpAdd("times", "show system current time UTC/LOCAL.\n");
    
    API_TShellKeywordAdd("tzsync", __tshellSysCmdTzsync);
    API_TShellHelpAdd("tzsync", "synchronize time zone from environment variables : TZ.\n");
    
    API_TShellKeywordAdd("date", __tshellSysCmdDate);
    API_TShellFormatAdd("date", " [-s {time | date}]");
    API_TShellHelpAdd("date", "set system current time.\n"
                              "eg. date -s 23:28:25\n"
                              "    date -s 20110217\n");
    
    API_TShellKeywordAdd("drvs", __tshellSysCmdDrvs);
    API_TShellHelpAdd("drvs", "show all drivers.\n");
    
    API_TShellKeywordAdd("drvlics", __tshellSysCmdDrvlics);
    API_TShellHelpAdd("drvlics", "show all driver license.\n");
    
    API_TShellKeywordAdd("devs", __tshellSysCmdDevs);
    API_TShellFormatAdd("devs", " [-a]");
    API_TShellHelpAdd("devs", "show all device.\n"
                              "-a   show all information.\n");
    
    API_TShellKeywordAdd("buss", __tshellSysCmdBuss);
    API_TShellHelpAdd("buss", "show all bus info.\n");
    
    API_TShellKeywordAdd("files", __tshellSysCmdFiles);
    API_TShellHelpAdd("files", "show all file.\n");
    
    API_TShellKeywordAdd("fdentrys", __tshellSysCmdFdentrys);
    API_TShellHelpAdd("fdentrys", "show all file entrys.\n");
    
    API_TShellKeywordAdd("open", __tshellSysCmdOpen);
    API_TShellFormatAdd("open", " [filename flag mode]");
    API_TShellHelpAdd("open", "open a file.\n"
                              "filename : file name\n"
                              "flag : O_RDONLY   : 00000000\n"
                              "       O_WRONLY   : 00000001\n"
                              "       O_RDWR     : 00000002\n"
                              "       O_APPEND   : 00000008\n"
                              "       O_SHLOCK   : 00000010\n"
                              "       O_EXLOCK   : 00000080\n"
                              "       O_ASYNC    : 00000040\n"
                              "       O_CREAT    : 00000200\n"
                              "       O_TRUNC    : 00000400\n"
                              "       O_EXCL     : 00000800\n"
                              "       O_SYNC     : 00002000\n"
                              "       O_NONBLOCK : 00004000\n"
                              "       O_NOCTTY   : 00008000\n"
                              "       O_CLOEXEC  : 00080000\n"
                              "mode : create file mode, eg: 0666\n");
    
    API_TShellKeywordAdd("close", __tshellSysCmdClose);
    API_TShellFormatAdd("close", " [fd]");
    API_TShellHelpAdd("close", "close a file.\n");
    
    API_TShellKeywordAdd("sync", __tshellSysCmdSync);
    API_TShellHelpAdd("sync", "flush all system buffer, For example: file system buffer.\n");
    
    API_TShellKeywordAdd("color", __tshellSysCmdColor);
    API_TShellHelpAdd("color", "update LS_COLORS configure.\n");

    API_TShellKeywordAdd("tty", __tshellSysCmdTty);
    API_TShellHelpAdd("tty", "show tty name.\n");

    API_TShellKeywordAdd("clear", __tshellSysCmdClear);
    API_TShellHelpAdd("clear", "clear the current screen.\n");
    
    API_TShellKeywordAdd("exit", __tshellSysCmdExit);
    API_TShellHelpAdd("exit", "exit current terminal.\n");
    
#if LW_CFG_VMM_EN > 0
    API_TShellKeywordAdd("zones", __tshellSysCmdZones);
    API_TShellHelpAdd("zones", "show system vmm physical zone information.\n");
    
    API_TShellKeywordAdd("virtuals", __tshellSysCmdVirtuals);
    API_TShellHelpAdd("virtuals", "show system virtuals address space information.\n");
    
    API_TShellKeywordAdd("aborts", __tshellSysCmdAborts);
    API_TShellHelpAdd("aborts", "show system vmm abort statistics information.\n");

    API_TShellKeywordAdd("pagefaultlimit", __tshellSysCmdPageFaultLimit);
    API_TShellFormatAdd("pagefaultlimit", " [root warning] [root limit] [user warning] [user limit]");
    API_TShellHelpAdd("pagefaultlimit", "set (MByts) or show system vmm page fault limit.\n");
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

#if LW_CFG_THREAD_RESTART_EN > 0
    API_TShellKeywordAdd("restart", __tshellSysCmdRestart);
    API_TShellFormatAdd("restart", " tid argument");
    API_TShellHelpAdd("restart", "restart a thread with specify argument.\n"
                                 "notice : tid is HEX.\n");
#endif                                                                  /*  LW_CFG_THREAD_RESTART_EN    */

    API_TShellKeywordAdd("sprio", __tshellSysCmdSprio);
    API_TShellFormatAdd("sprio", " [priority thread_id]");
    API_TShellHelpAdd("sprio", "set thread priority.\n"
                               "notice : tid is HEX.\n");
                               
    API_TShellKeywordAdd("renice", __tshellSysCmdRenice);
    API_TShellFormatAdd("renice", " priority [[-p] pid ...] [[-g] pgrp ...] [[-u] user ...]");
    API_TShellHelpAdd("renice",  "set process / process group / process user priority.\n");
                               
#if LW_CFG_LOG_LIB_EN > 0
    API_TShellKeywordAdd("logfileadd", __tshellSysCmdLogFileAdd);
    API_TShellFormatAdd("logfileadd", " [file descriptor]");
    API_TShellHelpAdd("logfileadd", "add a file to log file set.\n");
    
    API_TShellKeywordAdd("logfileclear", __tshellSysCmdLogFileClear);
    API_TShellFormatAdd("logfileclear", " [file descriptor]");
    API_TShellHelpAdd("logfileclear", "clear a file from log file set.\n");
    
    API_TShellKeywordAdd("logfiles", __tshellSysCmdLogFiles);
    API_TShellHelpAdd("logfiles", "show all the file(s) in log file set.\n");
    
    API_TShellKeywordAdd("loglevel", __tshellSysCmdLogLevel);
    API_TShellFormatAdd("loglevel", " [level]");
    API_TShellHelpAdd("loglevel", "show or set printk() log level.\n");
#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */
    
#if LW_CFG_RTC_EN > 0
    API_TShellKeywordAdd("hwclock", __tshellSysCmdHwclock);
    API_TShellFormatAdd("hwclock", " [{--show | --hctosys | --systohc}]");
    API_TShellHelpAdd("hwclock", "set/get hardware realtime clock.\n"
                                 "hwclock --show    : show hardware RTC time.\n"
                                 "hwclock --hctosys : sync RTC to system clock.\n"
                                 "hwclock --systohc : sync system clock to RTC.\n");
#endif                                                                  /*  LW_CFG_RTC_EN > 0           */

    API_TShellKeywordAdd("shstack", __tshellSysCmdShStack);
    API_TShellFormatAdd("shstack", " [new stack size]");
    API_TShellHelpAdd("shstack", "show or set sh stack size.\n");
    
#if LW_CFG_SHELL_PASS_CRYPT_EN > 0
    API_TShellKeywordAdd("crypt", __tshellSysCmdCrypt);
    API_TShellFormatAdd("crypt", " [key] [salt]");
    API_TShellHelpAdd("crypt", "crypt() in libcrypt.\n");
#endif                                                                  /*  LW_CFG_SHELL_PASS_CRYPT_EN  */
    
    API_TShellKeywordAdd("hostname", __tshellSysCmdHostname);
    API_TShellFormatAdd("hostname", " [hostname]");
    API_TShellHelpAdd("hostname", "get or show current machine host name.\n");
    
    API_TShellKeywordAdd("login", __tshellSysCmdLogin);
    API_TShellHelpAdd("login", "change current user.\n");
    
    API_TShellKeywordAdd("logout", __tshellSysCmdLogout);
    API_TShellHelpAdd("logout", "logout.\n");
    
    API_TShellKeywordAdd("who", __tshellSysCmdWho);
    API_TShellHelpAdd("who", "get current user message.\n");
    
    API_TShellKeywordAdd("shutdown", __tshellSysCmdShutdown);
    API_TShellFormatAdd("shutdown", " [shutdown parameter]");
    API_TShellHelpAdd("shutdown", "shutdown this computer.\n"
                                  "-r    shutdown & reboot\n"
                                  "-f    force shutdown & reboot\n"
                                  "-h    shutdown this computer\n"
                                  "no parameter means shutdown only\n");
                                  
    API_TShellKeywordAdd("reboot", __tshellSysCmdReboot);
    API_TShellFormatAdd("reboot",  " [reboot parameter]");
    API_TShellHelpAdd("reboot",   "reboot this computer.\n"
                                  "-n or -f    force reboot\n"
                                  "no parameter means reboot only\n");
                                  
#if LW_CFG_MONITOR_EN > 0
    API_TShellKeywordAdd("monitor", __tshellSysCmdMonitor);
    API_TShellFormatAdd("monitor",  " {[start {file | ip:port}] | [stop] | [filter [event allow-mask]] | [pid [pid]]}");
    API_TShellHelpAdd("monitor",    "kernel moniter setting.\n"
                                    "monitor start 192.168.1.1:1234\n"
                                    "monitor start /mnt/nfs/monitor.data\n"
                                    "monitor stop\n"
                                    "monitor filter\n"
                                    "monitor filter 10 1b\n"
                                    "monitor pid\n"
                                    "monitor pid 20\n"
                                    "            <  0 : all.\n"
                                    "            == 0 : kernel.\n"
                                    "            >  0 : whose process ID is equal to the value of pid.\n");
#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
                                    
    API_TShellKeywordAdd("pcis", __tshellSysCmdPcis);
    API_TShellHelpAdd("pcis", "show PCI Bus message.\n");
    
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_POSIX_EN > 0)
    API_TShellKeywordAdd("affinity", __tshellSysCmdAffinity);
    API_TShellFormatAdd("affinity", " [pid | thread id] [cpu id | 'clear']");
    API_TShellHelpAdd("affinity", "set / clear process or thread cpu affinity.\n"
                                  "affinity 1 0         set process 1 affinity to cpu 0\n"
                                  "affinity 1 clear     clear process 1 affinity\n");
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
                                                                        /*  LW_CFG_POSIX_EN > 0         */
#if LW_CFG_SMP_EN > 0
    API_TShellKeywordAdd("cpuaffinity", __tshellSysCmdCpuAffinity);
    API_TShellFormatAdd("cpuaffinity", " [cpu id] [1 / 0]");
    API_TShellHelpAdd("cpuaffinity", "set / clear cpu affinity schedule.\n"
                                  "cpuaffinity 1 1     set cpu 1 strongly affinity schedule.\n"
                                  "cpuaffinity 1 0     set cpu 1 no strongly affinity schedule\n");
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
    API_TShellKeywordAdd("cdump", __tshellSysCmdCdump);
    API_TShellFormatAdd("cdump", " [-s] [-c]");
    API_TShellHelpAdd("cdump",   "show or save crash dump message.\n"
                                 "-s    save crash dump message in /var/log/cdump\n"
                                 "-c    after show or save clear the message\n");
#endif                                                                  /*  LW_CFG_CDUMP_EN > 0         */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
#if LW_CFG_GDB_EN > 0
    API_TShellKeywordAdd("crashtrap", __tshellSysCmdCrashTrap);
    API_TShellFormatAdd("crashtrap", " [pid] [1 | 0]");
    API_TShellHelpAdd("crashtrap",   "set or get process crash trap setting.\n"
                                     "if enable, the process crash will not be killed but waiting for debugger.\n");
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
