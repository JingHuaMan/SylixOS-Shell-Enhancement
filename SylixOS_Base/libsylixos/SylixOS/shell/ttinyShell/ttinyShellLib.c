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
** ��   ��   ��: ttinyShellLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 07 �� 27 ��
**
** ��        ��: һ����С�͵� shell ϵͳ, ʹ�� tty/pty ���ӿ�, ��Ҫ���ڵ�����򵥽���.

** BUG
2008.10.20  ������΢������������ս��Ҫ������, ����ʮ�ָ���... 
            shell ϵͳ֧�ֱ�����������.
2008.12.24  ������ʥ��ǰҹ, ף���Լ�ʥ������, �Ǻ�! �޸���������ʾ��.
2009.02.09  �޸� shell һ��С BUG.
2009.02.13  dup2 ������ fd ��Ҫ�ر�.
2009.02.19  ��������ʾ���޸�Ϊ�� linux bash ����.
2009.02.23  ���� join �������еķ���.
2009.03.10  �ն�����ʱ�޸��ն˴��ڵı���.
2009.03.24  ��ʹ���������ַ�, �·���ӡ > ��ʾ.
2009.03.25  �ڵ��� shell exec ǰ����� optind �� optreset �Ĳ���, ȷ�� getopt ����ȷ��.
            �����̲߳���Ҫ, �ڳ�ʼ�� getopt �߳�������ʱ, �Ѿ����˳�ֵ.
2009.04.13  �����̼߳������������ջ, ��ֹ�ڲ����������� exit ֱ���˳�, �����޷��ͷŻ�������ڴ�й¶.
2009.05.22  �����û���֤����.
2009.05.28  ������ shell �����Ĺ���.
2009.06.30  ���� shell ʱ����ӡ tty �ն���.
            ������ʾ����ʾȫ����ǰ·��.
2009.07.13  ֧�� LW_OPTION_TSHELL_NOECHO ���� shell �߳�.
2009.08.04  __tshellKeywordAdd() ����ѡ�����.
            __tshellExec() ֧�ֹؼ���ѡ���.
2009.10.28  __tshellExec() ֱ�Ӻ���ע����.
2009.11.21  shell �߳�(�����������ı����߳�)ʹ��˽�е� io ����. �������߳̽��̳и�ϵ�̵߳� io ����.
2010.05.06  ���� undef �ӿڵĲ���.
2010.12.27  ��� __tshellExec ִ�еı�����ֵ��䳤�ȴ������ؼ��ֳ���ʱ, ���ֵĸ�ֵ���� bug. 
2011.03.10  ����ϵͳ�ض����������.
            ʹ���µı����׼�ļ��������ķ���.
2011.04.23  �ض���ʱ, ��Ҫ���������б�.
2011.05.19  ���� __tshellExec() ���� pfuncHook ʱ�Ĳ���ǰ�з��Ŵ���.
2011.06.23  ʹ���µ�ɫ��ת��.
2011.06.24  ʹ���µı��������㷨, �� __tshellExec() �ж��������Ƿ���Ҫ����ִ��.
2011.07.27  shell �����߳�ʹ��������������Ϊ�߳���.
2011.08.06  shell ����ͬ��ִ�����ʱ, ��Ҫ���������ֵ, shell ������ҲҪ������Ӧ�ķ���ֵ.
2012.03.25  shell undef ����д����ļ�, ͬʱ shell ��ȡ����ʹ�� read ������һ��ר�ú���, ���������ʵ��һ
            Щ�����Բ���, ������ʷ����, ��������λ���޸ĵȵ�.
2012.03.25  __tshellReadline() �ڲ��Ѿ������� \n, shell ������Ҫ�ٴδ���.
2012.03.27  ÿһ�� shell ���붼Ҫ�ж�ǿ���˳�����.
2012.03.30  ��������ִ���߳�ʹ�� API_ThreadStartEx() ���� join ԭ�Ӳ���.
2012.08.25  ���� __tshellBgCreateEx() �ṩ��չ�ļ��������Ĺ���.
2012.11.09  __tshellRestart() �ڲ���Ҫ�ϸ��жϾ���Ƿ���Ч.
2012.12.07  ������� shell ִ�б�������ķ�ʽ, ��ֹ���α���ִ����ɵ��˷�.
2012.12.24  shell ֧���˳�����ļ�����.
2012.12.25  __tshellBgCreateEx() ��������ֵ�������ֵ���뿪.
2013.01.23  shell ��ϵͳ����Ϳ�ִ���ļ����ظ�ʱ, ���������ļ�. 
2014.07.23  shell �����׼�ļ��ض���֧��.
2014.08.27  __tshellRestart() ����ж����ڵȴ���������, ��ʹ�� kill ����.
2014.04.17  ���ض����������Ļ�����Ҫ���ں� IO �������н���.
2017.07.19  ִ����ÿ�������ʹ�� fpurge(stdin) ������뻺��.
2018.12.13  ��Щ�������ʹ��ǿ���ڽ�����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  Ӧ�ü� API
*********************************************************************************************************/
#include "../SylixOS/api/Lw_Api_Kernel.h"
#include "../SylixOS/api/Lw_Api_System.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "limits.h"
#include "termios.h"
#include "pwd.h"
#include "ttinyShell.h"
#include "ttinyShellLib.h"
#include "ttinyShellSysCmd.h"
#include "ttinyShellSysVar.h"
#include "ttinyShellReadline.h"
#include "ttinyString.h"
#include "../SylixOS/shell/hashLib/hashHorner.h"
#include "../SylixOS/shell/ttinyVar/ttinyVarLib.h"
/*********************************************************************************************************
  �ڲ�������غ�������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  �������� (t_shell �߳���ͨ�����յ������ַ���ĩβ�ж�)
*********************************************************************************************************/
#define __TTINY_SHELL_BG_ASYNC            '&'                           /*  �����첽ִ��                */
#define __TTINY_SHELL_BG_JOIN             '^'                           /*  ����ͬ��ִ��                */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
#if LW_CFG_SHELL_HOOK_EN > 0
static FUNCPTR          _G_pfuncShellHook   = LW_NULL;
#endif                                                                  /*  LW_CFG_SHELL_HOOK_EN > 0    */
static PLW_LIST_LINE    _G_plineTSKeyHeader = LW_NULL;                  /*  ��������ͷ                  */
static PLW_LIST_LINE    _G_plineTSKeyHeaderHashTbl[LW_CFG_SHELL_KEY_HASH_SIZE];
                                                                        /*  ��ϣ������                  */
/*********************************************************************************************************
  ǿ��ʹ���ڽ������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static const PCHAR      _G_pcResBuildinCmd[] = {
    "help", "ts", "tp", "wjs", "ps", "modules", "ints", "cpuus", "sem", "msgq"
};

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  shell ִ���̶߳�ջ��С (Ĭ���� shell ��ͬ)
*********************************************************************************************************/
extern size_t           _G_stShellStackSize;
/*********************************************************************************************************
  �����߳�ִ�в���
*********************************************************************************************************/
typedef struct {
    INT          TSBG_iFd[3];                                           /*  �ļ�������                  */
    BOOL         TSBG_bClosed[3];                                       /*  ��������Ƿ�رն�Ӧ�ļ�    */
    PCHAR        TSBG_pcDefPath;                                        /*  ��ϵ�̵߳�ǰĿ¼��          */
    BOOL         TSBG_bIsJoin;                                          /*  shell �Ƿ��ڵȴ�����ֵ      */
    CHAR         TSBG_cCommand[1];                                      /*  ִ������                    */
} __TSHELL_BACKGROUND;
typedef __TSHELL_BACKGROUND     *__PTSHELL_BACKGROUND;

typedef struct {
    uid_t        TPC_uid;
    gid_t        TPC_gid;
    CHAR         TPC_cUserName[MAX_FILENAME_LENGTH];
    CHAR         TPC_cPsColor[12];
    CHAR         TPC_cDirColor[12];
    CHAR         TPC_cNorColor[12];
} __TSHELL_PROMPT_CTX;
typedef __TSHELL_PROMPT_CTX     *__PTSHELL_PROMPT_CTX;
/*********************************************************************************************************
  �û���֤
*********************************************************************************************************/
       ULONG  __tshellUserAuthen(INT  iTtyFd, BOOL  bWaitInf);
static INT    __tshellBgCreate(INT      iFd, 
                               CPCHAR   pcCommand, 
                               size_t   stCommandLen, 
                               ULONG    ulKeywordOpt,
                               BOOL     bIsJoin, 
                               ULONG    ulMagic);
/*********************************************************************************************************
  װ�����ڲ���������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
extern BOOL   __ldPathIsFile(CPCHAR  pcName, struct stat *pstatFile);
/*********************************************************************************************************
** ��������: __tshellIsResCmd
** ��������: �Ƿ�Ϊǿ���ڽ�����
** �䡡��  : pcKeyword         ��ǰδ��ʶ�������
** �䡡��  : �Ƿ��ж�Ӧ���ļ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __tshellIsResCmd (CPCHAR  pcKeyword)
{
    INT  i;
    
    for (i = 0; i < (sizeof(_G_pcResBuildinCmd) / sizeof(PCHAR)); i++) {
        if (lib_strcmp(pcKeyword, _G_pcResBuildinCmd[i]) == 0) {
            return  (LW_TRUE);
        }
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __tshellCheckFile
** ��������: shell ����Ƿ���ڶ�Ӧ������ļ�
** �䡡��  : pcKeyword         ��ǰδ��ʶ�������
** �䡡��  : �Ƿ��ж�Ӧ���ļ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __tshellCheckFile (CPCHAR  pcKeyword)
{
    PCHAR   pcBuffer;
    PCHAR   pcPathBuffer;
    
    PCHAR   pcStart;
    PCHAR   pcDiv;
    
    if (lib_strchr(pcKeyword, PX_DIVIDER)) {                            /*  ��һ��·��                  */
        if (__ldPathIsFile(pcKeyword, LW_NULL)) {
            return  (LW_TRUE);
        
        } else {
            return  (LW_FALSE);
        }
    
    } else {
        pcBuffer = (PCHAR)__SHEAP_ALLOC(MAX_FILENAME_LENGTH * 2);
        if (pcBuffer == LW_NULL) {
            return  (LW_FALSE);
        }
        pcPathBuffer = pcBuffer + MAX_FILENAME_LENGTH;
    
        if (lib_getenv_r("PATH", pcBuffer, MAX_FILENAME_LENGTH)
            != ERROR_NONE) {                                            /*  PATH ��������ֵΪ��         */
            __SHEAP_FREE(pcBuffer);
            return  (LW_FALSE);                                         /*  �޷��ҵ��ļ�                */
        }
        
        pcPathBuffer[MAX_FILENAME_LENGTH - 1] = PX_EOS;
        
        pcDiv = pcBuffer;                                               /*  �ӵ�һ��������ʼ��          */
        do {
            pcStart = pcDiv;
            pcDiv   = lib_strchr(pcStart, ':');                         /*  ������һ�������ָ��        */
            if (pcDiv) {
                *pcDiv = PX_EOS;
                pcDiv++;
            }
            
            snprintf(pcPathBuffer, MAX_FILENAME_LENGTH, "%s/%s", 
                     pcStart, pcKeyword);                               /*  �ϲ�Ϊ������Ŀ¼            */
            if (__ldPathIsFile(pcPathBuffer, LW_NULL)) {                /*  ���ļ����Ա�����            */
                __SHEAP_FREE(pcBuffer);
                return  (LW_TRUE);
            }
        } while (pcDiv);
        
        __SHEAP_FREE(pcBuffer);
    }
    
    return  (LW_FALSE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: __tshellThreadHook
** ��������: �� ttiny shell ����ע��һ���ص�����.
** �䡡��  : pfuncShellHook     �� shell ע��Ļص�����
** �䡡��  : ֮ǰ�Ļص�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SHELL_HOOK_EN > 0

FUNCPTR  __tshellThreadHook (FUNCPTR  pfuncShellHook)
{
    FUNCPTR  pfuncOrg = _G_pfuncShellHook;

    _G_pfuncShellHook = pfuncShellHook;

    return  (pfuncOrg);
}

#endif                                                                  /*  LW_CFG_SHELL_HOOK_EN > 0    */
/*********************************************************************************************************
** ��������: __tshellUndef
** ��������: shell δ��������� (����ִ���ļ�ת���� exec ����ִ��)
** �䡡��  : pcCmd             ��������
**           pcKeyword         �Ƿ���Լ����ؼ���, ������� pcKeyword Ϊ���ִ�
**           ppcParam          ���¶�λ�Ĳ���ͷ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellUndef (PCHAR  pcCmd, PCHAR  pcKeyword, PCHAR  *ppcParam)
{
    *pcKeyword = PX_EOS;                                                /*  ����������ʽ              */
    return  (__tshellVarDefine(pcCmd));
}
/*********************************************************************************************************
** ��������: __tshellCloseRedir
** ��������: ttiny shell �ر� wrapper �������ض���������
** �䡡��  : iFd               �ļ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __tshellCloseRedir (INT  iFd)
{
    if (iFd >= 0) {
        __KERNEL_SPACE_ENTER();                                         /*  ��Щ�ļ�����ʱ��֤���ں����*/
        close(iFd);
        __KERNEL_SPACE_EXIT();
    }
}
/*********************************************************************************************************
** ��������: __tshellRedir
** ��������: �����ض����ִ�������ص��ļ�
** �䡡��  : pcString     �ض����ִ�
**           pcRedir      �ض����
**           ulMe         ��ǰ�߳�
**           piPopCnt     POP �����ĸ���
** �䡡��  : �Ƿ�����ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellRedir (PCHAR  pcString, PCHAR  pcRedir, LW_OBJECT_HANDLE ulMe, INT *piPopCnt)
{
    INT         iFd;
    INT         iFlag = O_CREAT | O_WRONLY;
    INT         iOutOpt;
    PCHAR       pcFile;
    
    if (*pcRedir == *(pcRedir + 1)) {                                   /*  ׷�Ӵ�                    */
        iFlag |= O_APPEND;
        pcFile = pcRedir + 2;                                           /*  ��λ�ļ�                    */
        
    } else {
        iFlag |= O_TRUNC;
        pcFile = pcRedir + 1;
    }
    
    if (*pcFile == '&') {
        pcFile++;
    }
    
    if (*pcFile == PX_EOS) {                                            /*  ��λ�ļ�����                */
        return  (PX_ERROR);
    }
    
    if (*pcRedir == '>') {                                              /*  ��׼������׼����          */
        *pcRedir =  PX_EOS;
        
        if (pcString == pcRedir) {
            iOutOpt  =  STD_OUT;
            
        } else if (*pcString == '&') {
            iOutOpt  =  STD_OUT | STD_ERR;
        
        } else if (*pcString == '1') {
            iOutOpt  =  STD_OUT;
        
        } else if (*pcString == '2') {
            iOutOpt  =  STD_ERR;
        
        } else {
            iOutOpt  =  STD_OUT;
        }
        
        if ((lib_strlen(pcFile) == 1) && (*pcFile >= '0') && (*pcFile <= '2')) {
            iFd = API_IoTaskStdGet(ulMe, (*pcFile - '0'));
            if (iOutOpt & STD_OUT) {
                API_IoTaskStdSet(ulMe, STD_OUT, iFd);
            }
            if (iOutOpt & STD_ERR) {
                API_IoTaskStdSet(ulMe, STD_ERR, iFd);
            }
        
        } else {
            iFd = open(pcFile, iFlag, DEFAULT_FILE_PERM);
            if (iFd < 0) {
                return  (PX_ERROR);
            }
            
            API_ThreadCleanupPush(__tshellCloseRedir, (PVOID)(LONG)iFd);
            (*piPopCnt)++;
            
            if (iOutOpt & STD_OUT) {
                API_IoTaskStdSet(ulMe, STD_OUT, iFd);
            }
            if (iOutOpt & STD_ERR) {
                API_IoTaskStdSet(ulMe, STD_ERR, iFd);
            }
        }
    } else {                                                            /*  ��׼���붨λ                */
        iFlag &= ~O_TRUNC;                                              /*  ����û�� O_TRUNC ����       */
        iFd = open(pcFile, iFlag, DEFAULT_FILE_PERM);
        if (iFd < 0) {
            return  (PX_ERROR);
        }
        
        API_ThreadCleanupPush(__tshellCloseRedir, (PVOID)(LONG)iFd);
        (*piPopCnt)++;
        
        API_IoTaskStdSet(ulMe, STD_IN, iFd);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellWrapper
** ��������: ���� shell ��������
** �䡡��  : pfuncCommand ϵͳ����ָ��
**           iArgC        ��������
**           ppcArgV ������
** �䡡��  : �����ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellWrapper (FUNCPTR  pfuncCommand, INT  iArgC, PCHAR  ppcArgV[])
{
    INT         i;
    INT         iPopCnt    = 0;
    INT         iOldStd[3] = {PX_ERROR, PX_ERROR, PX_ERROR};
    INT         iRealArgc  = iArgC;
    INT         iRet       = PX_ERROR;
    
    LW_OBJECT_HANDLE  ulMe = API_ThreadIdSelf();

    if (iArgC < 2) {                                                    /*  �������ض���                */
        return  pfuncCommand(iArgC, ppcArgV);
    }
    
    for (i = 0; i < 3; i++) {                                           /*  ����ɵ�                    */
        iOldStd[i] = API_IoTaskStdGet(ulMe, i);
    }
    
    for (i = 1; i < iArgC; i++) {                                       /*  ��������                    */
        PCHAR   pcRedir = lib_strchr(ppcArgV[i], '<');
        
        if (pcRedir == LW_NULL) {
            pcRedir = lib_strchr(ppcArgV[i], '>');
            if (pcRedir == LW_NULL) {
                continue;
            }
        }
        
        if (__tshellRedir(ppcArgV[i], pcRedir, ulMe, &iPopCnt)) {       /*  ������ִ���ض�λ����        */
            fprintf(stderr, "can not process redirect.\n");
            goto    __ret;
        
        } else if (iRealArgc > i) {
            iRealArgc = i;                                              /*  �ض�λ�����ɹ�              */
        }
    }
    
    ppcArgV[iRealArgc] = LW_NULL;
    iRet = pfuncCommand(iRealArgc, ppcArgV);                            /*  ִ������                    */
    
__ret:
    for (i = 0; i < 3; i++) {                                           /*  ��ԭ�ɵ�                    */
        API_IoTaskStdSet(ulMe, i, iOldStd[i]);
    }

    for (i = 0; i < iPopCnt; i++) {
        API_ThreadCleanupPop(LW_TRUE);
    }
    
    return  (iRet);
    
}
/*********************************************************************************************************
** ��������: __tshellChkBg
** ��������: Ԥ������ִ����ز��� shell �����ִ�в���
** �䡡��  : pcCommand    �����ַ���
**           pbNeedJoin   �Ƿ���Ҫ JOIN.
**           pbNeedAsyn   ʹ����Ҫ�첽ִ��.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __tshellPreTreatedBg (PCHAR  cCommand, BOOL  *pbNeedJoin, BOOL  *pbNeedAsyn)
{
    size_t     stStrLen;
    PCHAR      pcTail = cCommand;
    
    if (pbNeedJoin) {
        *pbNeedJoin = LW_FALSE;
    }
    if (pbNeedAsyn) {
        *pbNeedAsyn = LW_FALSE;
    }

    __tshellStrDelCRLF(cCommand);                                       /*  ɾ�� CR �� LF �ַ�          */
    __tshellStrFormat(cCommand, cCommand);                              /*  ���� shell ����             */
    
    stStrLen = lib_strlen(cCommand);
    if (stStrLen < 1) {
        return;
    }
    
    if (cCommand[stStrLen - 1] == __TTINY_SHELL_BG_JOIN) {              /*  ������ͬ������ִ��          */
        if (pbNeedJoin) {
            *pbNeedJoin = LW_TRUE;
        }
        pcTail = &cCommand[stStrLen - 1];
        
    } else if (cCommand[stStrLen - 1] == __TTINY_SHELL_BG_ASYNC) {      /*  �������첽����ִ��          */
        if (pbNeedAsyn) {
            *pbNeedAsyn = LW_TRUE;
        }
        pcTail = &cCommand[stStrLen - 1];
    }
    
    while (pcTail != cCommand) {                                        /*  ȥ���������з���            */
        if ((*pcTail == __TTINY_SHELL_BG_JOIN) ||
            (*pcTail == __TTINY_SHELL_BG_ASYNC)) {
            *pcTail = PX_EOS;
            pcTail--;
        
        } else if (*pcTail == PX_EOS) {
            pcTail--;
        
        } else {
            break;
        }
    }
}
/*********************************************************************************************************
** ��������: __tshellExec
** ��������: ttiny shell ϵͳ, ִ��һ�� shell ����
** �䡡��  : pcCommand    �����ַ���
**           pfuncHook    ��ȡ�ؼ��ֺ���õĻص�����.
** �䡡��  : �����ֵ(����������ʱ, ����ֵΪ��ֵ)
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �� shell ���������ַ�������ʱ, ���᷵�ظ�ֵ, ��ֵȡ�෴����Ϊ������.
*********************************************************************************************************/
INT  __tshellExec (CPCHAR  pcCommandExec, VOIDFUNCPTR  pfuncHook)
{
#define __TTINY_SHELL_CMD_ISWHITE(pcCmd)    \
        ((*(pcCmd) == ' ') || (*(pcCmd) == '\t') || (*(pcCmd) == '\r') || (*(pcCmd) == '\n'))

#define __TTINY_SHELL_CMD_ISEND(pcCmd)      (*(pcCmd) == PX_EOS)

    REGISTER INT        i;
    REGISTER PCHAR      pcCmd = (PCHAR)pcCommandExec;
    REGISTER size_t     stStrLen;
    REGISTER ULONG      ulError;
    
    PLW_CLASS_TCB       ptcbCur;
    __PTSHELL_KEYWORD   pskwNode = LW_NULL;
    
             CHAR       cCommandBuffer[LW_CFG_SHELL_MAX_COMMANDLEN + 1];/*  ��ʱ������                  */
             CHAR       cCommandBg[LW_CFG_SHELL_MAX_COMMANDLEN + 1];    /*  �������л�����              */
             
             CHAR       cKeyword[LW_CFG_SHELL_MAX_KEYWORDLEN + 1];      /*  �ؼ���                      */
             PCHAR      pcParamList[LW_CFG_SHELL_MAX_PARAMNUM + 1];     /*  �����б�                    */
             
             PCHAR      pcParam = LW_NULL;                              /*  ָ���һ��������ָ��        */
             
             INT        iStdFd;                                         /*  ����ִ�б�׼�ļ�            */
             BOOL       bIsJoin;                                        /*  ����ִ���Ƿ�ͬ��            */
             CPCHAR     pcBgCmd;                                        /*  ����ִ������                */
             size_t     stBgCmdLen;                                     /*  ����ִ�г���                */
             ULONG      ulMagic;                                        /*  ���� magic ��               */
             
             BOOL       bCmdLineNeedJoin;                               /*  �������Ƿ�Ҫ��ͬ������ִ��  */
             BOOL       bCmdLineNeedAsyn;                               /*  �������Ƿ�Ҫ���첽����ִ��  */
             
    
    if (!pcCmd || __TTINY_SHELL_CMD_ISEND(pcCmd)) {                     /*  �������                    */
        return  (ERROR_NONE);
    }
    
    while (__TTINY_SHELL_CMD_ISWHITE(pcCmd)) {                          /*  ����ǰ��Ĳ��ɼ��ַ�        */
        pcCmd++;
        if (__TTINY_SHELL_CMD_ISEND(pcCmd)) {
            return  (ERROR_NONE);                                       /*  ������Ч��������            */
        }
    }
    
    if (*pcCmd == '#') {                                                /*  ע����ֱ�Ӻ���              */
        return  (ERROR_NONE);
    }
    
    stStrLen = lib_strnlen(pcCmd, LW_CFG_SHELL_MAX_COMMANDLEN + 1);     /*  �����ַ�������              */
    if ((stStrLen > LW_CFG_SHELL_MAX_COMMANDLEN - 1) || (stStrLen < 1)) {
        return  (-ERROR_TSHELL_EPARAM);                                 /*  �ַ������ȴ���              */
    }
    
    ulError = __tshellStrConvertVar(pcCmd, cCommandBuffer);             /*  �����滻                    */
    if (ulError) {
        return  ((INT)(-ulError));                                      /*  �滻����                    */
    }
    
    __tshellPreTreatedBg(cCommandBuffer, 
                         &bCmdLineNeedJoin, &bCmdLineNeedAsyn);         /*  Ԥ������ִ����ز���      */
    
    lib_strcpy(cCommandBg, cCommandBuffer);                             /*  ����������Ҫ�ı���ִ������  */
    
    ulError = __tshellStrKeyword(cCommandBuffer, cKeyword, &pcParam);   /*  ��ȡ�ؼ���                  */
    if (ulError) {
        /*
         *  ����ؼ��ֹ���, �������Ϊ��������
         */
        ulError = __tshellVarDefine(cCommandBuffer);
        return  ((INT)(-ulError));                                      /*  ��ȡ����                    */
    }
    
    /*
     *  �����Ҫ, ���� pfuncHook ����.
     */
    if (pfuncHook) {
        PCHAR   pcStartAlpha = cKeyword;
        while ((lib_isalpha(*pcStartAlpha) == 0) && 
               (*pcStartAlpha != PX_EOS)) {                             /*  �ӵ�һ���ַ���ʼ            */
            pcStartAlpha++;
        }
        if (*pcStartAlpha != PX_EOS) {
            pfuncHook(API_ThreadIdSelf(), pcStartAlpha);                /*  ���ûص�����                */
        }
    }
    
    /*
     *  ������ڿ�ִ���ļ���, �����������ļ�.
     */
#if LW_CFG_MODULELOADER_EN > 0                                          /*  ��������ļ�, ����������    */
    if (!__tshellIsResCmd(cKeyword)) {
        if (__tshellCheckFile(cKeyword)) {                              /*  �ж�Ӧ�ļ�                  */
            lib_strcpy(cKeyword, "exec");                               /*  ʹ�� exec ת����������      */
            pcParam = (PCHAR)cCommandBuffer;                            /*  ������ͷ��ʼ�������      */
        }
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    /*
     *  ���ҹؼ���.
     */
    if (ERROR_NONE != __tshellKeywordFind(cKeyword, &pskwNode)) {       /*  ���ҹؼ���                  */
        ulError = __tshellUndef(cCommandBuffer, cKeyword, &pcParam);
        if (ulError != ERROR_NONE) {
            return  (-ERROR_TSHELL_CMDNOTFUND);                         /*  ����δ�ҵ�                  */
        }
        return  (ERROR_NONE);                                           /*  ����������߸�ֵ�ɹ�        */
    }
    
    /*
     *  ���ע�������Ƿ���Ҫ����ִ��
     */
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (((pskwNode->SK_ulOption & LW_OPTION_KEYWORD_SYNCBG) ||
         (pskwNode->SK_ulOption & LW_OPTION_KEYWORD_ASYNCBG)) &&
        __TTINY_SHELL_GET_MAGIC(ptcbCur) != (ULONG)pskwNode) {          /*  �ж��Ƿ���Ҫ����ִ��        */
        
        pcBgCmd    = cCommandBg;
        stBgCmdLen = lib_strlen(cCommandBg);
        ulMagic    = (ULONG)pskwNode;
        iStdFd     = API_IoTaskStdGet(API_ThreadIdSelf(), STD_OUT);     /*  ��ñ�׼�ļ�                */
        
        /*
         *  �������ִ�з�ʽ�� shell �����з�ʽ��ͻ, ������ʹ�������еķ�ʽ
         */
        if (bCmdLineNeedJoin) {
            bIsJoin = LW_TRUE;
        
        } else if (bCmdLineNeedAsyn) {
            bIsJoin = LW_FALSE;
        
        } else {                                                        /*  ������û�б���ִ�в���      */
            if (pskwNode->SK_ulOption & LW_OPTION_KEYWORD_SYNCBG) {
                bIsJoin = LW_TRUE;
            
            } else {
                bIsJoin = LW_FALSE;
            }
        }
        goto    __bg_run;
    
    } else {                                                            /*  ������Ҫ�󱳾�ִ��      */
        __TTINY_SHELL_SET_MAGIC(ptcbCur, 0ul);
        
        if (bCmdLineNeedJoin || bCmdLineNeedAsyn) {                     /*  �����д��б���ִ�в���      */
            
            pcBgCmd    = cCommandBg;
            stBgCmdLen = lib_strlen(cCommandBg);
            ulMagic    = 0ul;
            iStdFd     = API_IoTaskStdGet(API_ThreadIdSelf(), STD_OUT); /*  ��ñ�׼�ļ�                */
            
            if (bCmdLineNeedJoin) {
                bIsJoin = LW_TRUE;
            
            } else {
                bIsJoin = LW_FALSE;
            }
            
            goto    __bg_run;
        }
    }
    
    pcParamList[0] = cKeyword;
    if (!pcParam) {                                                     /*  ����Ƿ���ڲ���            */
        REGISTER INT iRet = pskwNode->SK_pfuncCommand(1, pcParamList);  /*  ִ������,û�в���           */
        return  (iRet);
    }
    
    pcParamList[1] = pcParam;                                           /*  ��һ��������ַ              */
    for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {                   /*  ��ʼ��ѯ����                */
        __tshellStrGetToken(pcParamList[i], 
                            &pcParamList[i + 1]);
        __tshellStrDelTransChar(pcParamList[i], pcParamList[i]);        /*  ɾ��ת���ַ����ת������    */
        if (pcParamList[i + 1] == LW_NULL) {                            /*  ��������                    */
            break;
        }
    }
                                                                        /*  ִ������                    */
    return  (__tshellWrapper(pskwNode->SK_pfuncCommand, i + 1, pcParamList));
    
__bg_run:                                                               /*  �첽ִ������                */
    return  (__tshellBgCreate(iStdFd, pcBgCmd, stBgCmdLen, 
                              pskwNode->SK_ulOption,
                              bIsJoin, ulMagic));                       /*  ���������߳�ִ������        */
}
/*********************************************************************************************************
** ��������: __tshellPromptFree
** ��������: ttiny shell ɾ��������ʾ������
** �䡡��  : ptpc      ������ʾ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __tshellPromptFree (__PTSHELL_PROMPT_CTX  ptpc)
{
    __SHEAP_FREE(ptpc);
}
/*********************************************************************************************************
** ��������: __tshellShowPrompt
** ��������: ttiny shell ��ʾ������ʾ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __tshellShowPrompt (VOID)
{
    CHAR                    cPrompt[MAX_FILENAME_LENGTH + 3];
    PCHAR                   pcSeparator;
    PLW_CLASS_TCB           ptcbCur;
    __PTSHELL_PROMPT_CTX    ptpc;
    CHAR                    cPrivilege;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    getwd(cPrompt);
    
    if (geteuid() == 0) {
        cPrivilege = '#';                                           /*  ��Ȩ�û�                        */
    } else {
        cPrivilege = '$';                                           /*  ����Ȩ�û�                      */
    }
    
    if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_PROMPT_FULL) {
        PCHAR           pcUserName;
        CHAR            cHostName[HOST_NAME_MAX + 1] = "(none)";
    
        ptpc = (__PTSHELL_PROMPT_CTX)API_ThreadGetNotePad(API_ThreadIdSelf(), 0);
        if (ptpc == LW_NULL) {
            ptpc =  (__PTSHELL_PROMPT_CTX)__SHEAP_ALLOC(sizeof(__TSHELL_PROMPT_CTX));
            if (ptpc == LW_NULL) {
                goto    __simple;
            }
            API_ThreadCleanupPush(__tshellPromptFree, ptpc);
            API_ThreadSetNotePad(API_ThreadIdSelf(), 0, (ULONG)ptpc);
            
            ptpc->TPC_uid = getuid();
            ptpc->TPC_gid = getgid();
            
            if (API_TShellGetUserName(ptpc->TPC_uid, 
                                      ptpc->TPC_cUserName, 
                                      sizeof(ptpc->TPC_cUserName))) {
                lib_strcpy(ptpc->TPC_cUserName, "unknown");
            }
            
            if ((__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_VT100) &&
                (API_TShellVarGetRt("TERM_PS_COLOR", &ptpc->TPC_cPsColor[2], 10) > 0)) {
                ptpc->TPC_cPsColor[0] = '\033';
                ptpc->TPC_cPsColor[1] = '[';
                lib_strlcat(ptpc->TPC_cPsColor, "m", 12);               /*  ������ʾ��ɫ��              */
                API_TShellColorGet(S_IFDIR, ptpc->TPC_cDirColor, 12);
                API_TShellColorGet(0, ptpc->TPC_cNorColor, 12);
                
            } else {
                ptpc->TPC_cPsColor[0]  = PX_EOS;
                ptpc->TPC_cDirColor[0] = PX_EOS;
                ptpc->TPC_cNorColor[0] = PX_EOS;
            }
        }
        
        if (ptpc->TPC_uid != getuid()) {                                /*  ���û��ı�                  */
            ptpc->TPC_uid  = getuid();
            
            if (API_TShellGetUserName(ptpc->TPC_uid, 
                                      ptpc->TPC_cUserName, 
                                      sizeof(ptpc->TPC_cUserName))) {
                lib_strcpy(ptpc->TPC_cUserName, "unknown");
            }
        }
        pcUserName = ptpc->TPC_cUserName;
        gethostname(cHostName, sizeof(cHostName));
        
        pcSeparator = lib_strrchr(cPrompt, PX_DIVIDER);
        if (pcSeparator) {
            printf("[%s%s@%s%s:%s%s%s]%c ", 
                   ptpc->TPC_cPsColor, pcUserName, cHostName, ptpc->TPC_cNorColor,
                   ptpc->TPC_cDirColor, cPrompt, ptpc->TPC_cNorColor,
                   cPrivilege);                                         /*  ��ʾ��ǰ·��                */
        
        } else {
            printf("[%s%s@%s%s:%s/%s]%c ",
                   ptpc->TPC_cPsColor, pcUserName, cHostName, ptpc->TPC_cNorColor,
                   ptpc->TPC_cDirColor, ptpc->TPC_cNorColor, cPrivilege);
        }
    
    } else {
__simple:
        pcSeparator = lib_strrchr(cPrompt, PX_DIVIDER);
        if (pcSeparator) {
            printf("[%s]%c ", cPrompt, cPrivilege);                     /*  ��ʾ��ǰ·��                */
        } else {
            printf("[/]%c ", cPrivilege);
        }
    }
    
    fflush(stdout);                                                     /*  ��֤ stdout �����          */
}
/*********************************************************************************************************
** ��������: __tshellRestartEx
** ��������: ttiny shell �߳�����
** �䡡��  : ulThread      �߳̾��
**           bNeedAuthen   �Ƿ���Ҫ��¼
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __tshellRestartEx (LW_OBJECT_HANDLE  ulThread, BOOL  bNeedAuthen)
{
    REGISTER ULONG            ulOption;
    REGISTER PLW_CLASS_TCB    ptcbShell;
    REGISTER PLW_CLASS_TCB    ptcbJoin;
             LW_OBJECT_HANDLE ulJoin = LW_OBJECT_HANDLE_INVALID;
             INT              iMsg;
             UINT16           usIndex;

#if LW_CFG_ISR_DEFER_EN > 0
    if (API_InterDeferContext())
#else
    if (LW_CPU_GET_CUR_NESTING())
#endif
    {
        return  (_excJobAdd((VOIDFUNCPTR)__tshellRestartEx, 
                            (PVOID)ulThread, (PVOID)(LONG)bNeedAuthen, 0, 0, 0, 0));
    }
    
    usIndex = _ObjectGetIndex(ulThread);
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    ptcbShell = __GET_TCB_FROM_INDEX(usIndex);
    ptcbJoin  = ptcbShell->TCB_ptcbJoin;
    if (ptcbJoin) {                                                     /*  �ȴ������߳̽���            */
        if (bNeedAuthen) {
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        ulJoin = ptcbJoin->TCB_ulId;
    
    } else {
        ulOption  = __TTINY_SHELL_GET_OPT(ptcbShell);
        if (bNeedAuthen) {
            ulOption &= ~LW_OPTION_TSHELL_NOLOGO;
            ulOption |= LW_OPTION_TSHELL_AUTHEN | LW_OPTION_TSHELL_LOOPLOGIN;
        
        } else {
            ulOption |= LW_OPTION_TSHELL_NOLOGO;                        /*  ����ʱ����Ҫ��ʾ logo       */
            ulOption &= ~LW_OPTION_TSHELL_AUTHEN;                       /*  ����ʱ����Ҫ�û���֤        */
        }
        __TTINY_SHELL_SET_OPT(ptcbShell, ulOption);
    }
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (ulJoin) {
#if LW_CFG_SIGNAL_EN > 0
#if LW_CFG_MODULELOADER_EN > 0
        pid_t pid = vprocGetPidByThread(ulJoin);
        if (pid > 0) {
            kill(pid, SIGKILL);                                         /*  ɱ���ȴ��Ľ���              */
        } else
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
        {
            kill(ulJoin, SIGKILL);                                      /*  ɱ���ȴ����߳�              */
        }
#else
        API_ThreadDelete(&ulJoin, LW_NULL);
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
        iMsg = API_IoTaskStdGet(ulThread, STD_OUT);
        if (iMsg >= 0) {
            fdprintf(iMsg, "[sh]Warning: Program is killed (SIGKILL) by shell.\n"
                           "    Restart SylixOS is recommended!\n");
        }
    } else {                                                            /*  �����߳�                    */
        __tshellHistoryBackup(ptcbShell);

        API_ThreadRestart(ulThread, (PVOID)(LONG)__TTINY_SHELL_GET_STDFILE(ptcbShell));
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellRestart
** ��������: ttiny shell �߳����� (control-C)
** �䡡��  : ulThread   �߳̾��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __tshellRestart (LW_OBJECT_HANDLE  ulThread)
{
    __tshellRestartEx(ulThread, LW_FALSE);
}
/*********************************************************************************************************
** ��������: __tshellRename
** ��������: ttiny shell �߳���������
** �䡡��  : ulThread   �߳̾��
**           pcName     ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __tshellRename (LW_OBJECT_HANDLE  ulThread, CPCHAR  pcName)
{
    CHAR    cNewName[LW_CFG_OBJECT_NAME_SIZE];
    PCHAR   pcNewStart = lib_rindex(pcName, PX_DIVIDER);
    
    if (pcNewStart) {
        pcNewStart++;
        lib_strlcpy(cNewName, pcNewStart, LW_CFG_OBJECT_NAME_SIZE);
    } else {
        lib_strlcpy(cNewName, pcName, LW_CFG_OBJECT_NAME_SIZE);
    }
    
    API_ThreadSetName(ulThread, cNewName);
}
/*********************************************************************************************************
** ��������: __tshellBackgroundCleanup
** ��������: ttiny shell �������з����߳����������Ϣ����
** �䡡��  : pvArg      ��Ҫ������ڴ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __tshellBackgroundCleanup (PVOID  pvArg)
{
    REGISTER __PTSHELL_BACKGROUND   tsbg = (__PTSHELL_BACKGROUND)pvArg;
             INT                    i;
             
    if (tsbg) {
        __KERNEL_SPACE_ENTER();                                         /*  ��Щ�ļ�����ʱ��֤���ں����*/
        for (i = 0; i < 3; i++) {
            if (tsbg->TSBG_bClosed[i]) {
                close(tsbg->TSBG_iFd[i]);                               /*  �رն�Ӧ�ı�׼�ļ�          */
            }
        }
        __KERNEL_SPACE_EXIT();
    
        if (tsbg->TSBG_pcDefPath) {
            __SHEAP_FREE(tsbg->TSBG_pcDefPath);                         /*  �ͷ�Ŀ¼����                */
        }
        __SHEAP_FREE(tsbg);                                             /*  �ͷ��ڴ�                    */
    }
}
/*********************************************************************************************************
** ��������: __tshellBackground
** ��������: ttiny shell �������з����߳�
** �䡡��  : pcArg      �����ն��ļ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  __tshellBackground (PVOID  pvArg)
{
    REGISTER __PTSHELL_BACKGROUND       tsbg = (__PTSHELL_BACKGROUND)pvArg;
             INT                        iRetValue;
             LW_OBJECT_HANDLE           ulMe = API_ThreadIdSelf();
    
    /*
     *  �����е��������֮ǰ, ���̲߳��ܱ�ɾ��, ���� pvArg ������ڴ潫�޷�����.
     */
    API_ThreadCleanupPush(__tshellBackgroundCleanup, pvArg);            /*  �����������                */
    
    if (ioPrivateEnv() < 0) {                                           /*  ʹ��˽���߳� io ����        */
        return  (LW_NULL);
    } else {
        chdir(tsbg->TSBG_pcDefPath);                                    /*  �̳и�ϵ io ��ǰĿ¼        */
    }
    
    API_IoTaskStdSet(ulMe, STD_IN,  tsbg->TSBG_iFd[0]);                 /*  I/O �ض���                  */
    API_IoTaskStdSet(ulMe, STD_OUT, tsbg->TSBG_iFd[1]);
    API_IoTaskStdSet(ulMe, STD_ERR, tsbg->TSBG_iFd[2]);
    
    iRetValue = __tshellExec(tsbg->TSBG_cCommand,                       /*  ִ�� shell ָ��             */
                             __tshellRename);                           /*  ͬʱ�����̵߳�����          */
    if ((iRetValue < 0) && !tsbg->TSBG_bIsJoin) {                       /*  ���ִ�����shû�еȴ����߳�  */
        
        switch (iRetValue) {                                            /*  ϵͳ��������ʾ              */
        
        case -ERROR_TSHELL_EPARAM:
            fprintf(stderr, "parameter(s) error.\n");
            break;
        
        case -ERROR_TSHELL_CMDNOTFUND:
            fprintf(stderr, "sh: command not found.\n");
            break;
            
        case -ERROR_TSHELL_EVAR:
            fprintf(stderr, "sh: variable error.\n");
            break;
            
        case -ERROR_SYSTEM_LOW_MEMORY:
            fprintf(stderr, "sh: no memory.\n");
            break;
        }
        
        API_TShellTermAlert(STD_OUT);                                   /*  ��������                    */
    }
    
    /*
     *  ��¼��ǰ shell ��������Ĵ���.
     */
    __TTINY_SHELL_SET_ERROR(API_ThreadTcbSelf(), iRetValue);
    
    API_ThreadCleanupPop(LW_TRUE);                                      /*  �����������                */
    
    return  ((PVOID)(LONG)iRetValue);                                   /*  ��������ִ�н��            */
}
/*********************************************************************************************************
** ��������: __tshellBgCreateEx
** ��������: ttiny shell �����������з����߳� (pid �̳е�����)
** �䡡��  : iFd[3]         ��׼�ļ�
**           pcCommand      ��Ҫִ�е�����
**           stCommandLen   �����
**           ulKeywordOpt   �ؼ���ѡ��
**           bIsJoin        �Ƿ�ȴ��������.
**           ulMagic        ʶ���
**           pulSh          �����߳̾�� (���� bIsJoin = LW_FALSE ʱ����)
**           piRet          �����ֵ
** �䡡��  : �����ͬ��ִ��, �򷵻�ִ��������.
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ��������ʱΪ�ں��߳�, ������е�������Ҫת��Ϊ����, ����û������.
*********************************************************************************************************/
INT    __tshellBgCreateEx (INT               iFd[3],
                           BOOL              bClosed[3],
                           CPCHAR            pcCommand, 
                           size_t            stCommandLen, 
                           ULONG             ulKeywordOpt,
                           BOOL              bIsJoin,
                           ULONG             ulMagic,
                           LW_OBJECT_HANDLE *pulSh,
                           INT              *piRet)
{
    REGISTER __PTSHELL_BACKGROUND       tsbg;
             LW_CLASS_THREADATTR        threadattrTShell;
             LW_OBJECT_HANDLE           hTShellHandle;
             LONG                       lRetValue = ERROR_NONE;         /*  LONG ��֤ 64 λ CPU ����    */
             
             PLW_CLASS_TCB              ptcbShellBg;
             PLW_CLASS_TCB              ptcbCur;
    REGISTER ULONG                      ulOption;
             ULONG                      ulTaskOpt = LW_CFG_SHELL_THREAD_OPTION | LW_OPTION_OBJECT_GLOBAL;
             size_t                     stSize;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ulOption = __TTINY_SHELL_GET_OPT(ptcbCur);

    __KERNEL_SPACE_ENTER();
    if (!isatty(iFd[1])) {                                              /*  �ں��ļ��������Ƿ�Ϊ tty    */
        ulOption &= ~LW_OPTION_TSHELL_VT100;
    }
    __KERNEL_SPACE_EXIT();
    
    tsbg = (__PTSHELL_BACKGROUND)__SHEAP_ALLOC(sizeof(__TSHELL_BACKGROUND) + stCommandLen);
    if (!tsbg) {
        fprintf(stderr, "system low memory, can not create background thread.\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    stSize = lib_strlen(_PathGetDef()) + 1;
    tsbg->TSBG_pcDefPath = (PCHAR)__SHEAP_ALLOC(stSize);
    if (!tsbg->TSBG_pcDefPath) {
        __SHEAP_FREE(tsbg);
        fprintf(stderr, "system low memory, can not create background thread.\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_strlcpy(tsbg->TSBG_pcDefPath, _PathGetDef(), stSize);           /*  ������ϵ��ǰĿ¼            */
    
    tsbg->TSBG_iFd[0] = iFd[0];
    tsbg->TSBG_iFd[1] = iFd[1];
    tsbg->TSBG_iFd[2] = iFd[2];
    
    tsbg->TSBG_bClosed[0] = bClosed[0];
    tsbg->TSBG_bClosed[1] = bClosed[1];
    tsbg->TSBG_bClosed[2] = bClosed[2];
    
    tsbg->TSBG_bIsJoin = bIsJoin;
    
    lib_strlcpy(tsbg->TSBG_cCommand, pcCommand, stCommandLen + 1);
    
#if LW_CFG_VMM_EN > 0
    if (ulKeywordOpt & LW_OPTION_KEYWORD_STK_MAIN) {
        ulTaskOpt |= LW_OPTION_THREAD_STK_MAIN;
    }
#endif
    
    API_ThreadAttrBuild(&threadattrTShell,
                        _G_stShellStackSize,                            /*  shell ��ջ��С              */
                        LW_PRIO_T_SHELL,
                        ulTaskOpt,
                        (PVOID)tsbg);                                   /*  �������Կ�                  */
                        
    hTShellHandle = API_ThreadInit("t_tshellbg", __tshellBackground,
                                   &threadattrTShell, LW_NULL);         /*  ���� tshell �߳�            */
    if (!hTShellHandle) {
        __SHEAP_FREE(tsbg->TSBG_pcDefPath);
        __SHEAP_FREE(tsbg);
        _DebugHandle(__ERRORMESSAGE_LEVEL, 
                     "tshellbg thread can not create.\r\n");
        _DebugHandle(__LOGMESSAGE_LEVEL, 
                     "ttiny shell system is not initialize.\r\n");
        return  (PX_ERROR);
    }
    
    ptcbShellBg = __GET_TCB_FROM_INDEX(_ObjectGetIndex(hTShellHandle));
    __TTINY_SHELL_SET_OPT(ptcbShellBg, ulOption);
    __TTINY_SHELL_SET_MAGIC(ptcbShellBg, ulMagic);                      /*  ��¼ʶ���                  */
    
    if (pulSh && (bIsJoin == LW_FALSE)) {
        *pulSh = hTShellHandle;
    
    } else if (pulSh) {
        *pulSh = LW_OBJECT_HANDLE_INVALID;
    }
    
    API_ThreadStartEx(hTShellHandle, bIsJoin, (PVOID *)&lRetValue);     /*  ���� shell �߳�             */
    __TTINY_SHELL_SET_ERROR(ptcbCur, (INT)lRetValue);                   /*  ��¼ shell ��������Ĵ���   */
    
    if (piRet) {
        *piRet = (INT)lRetValue;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellBgCreate
** ��������: ttiny shell �����������з����߳�
** �䡡��  : iFd            �ļ�������
**           pcCommand      ��Ҫִ�е�����
**           stCommandLen   �����
**           ulKeywordOpt   �ؼ���ѡ��
**           bIsJoin        �Ƿ�ȴ��������.
**           ulMagic        ʶ���
** �䡡��  : �����ͬ��ִ��, �򷵻�ִ��������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT    __tshellBgCreate (INT     iFd, 
                                CPCHAR  pcCommand, 
                                size_t  stCommandLen, 
                                ULONG   ulKeywordOpt,
                                BOOL    bIsJoin,
                                ULONG   ulMagic)
{
    INT  iError;
    INT  iRet;
    INT  iFdArry[3];
    BOOL bClosed[3] = {LW_FALSE, LW_FALSE, LW_FALSE};                   /*  ���رձ�׼���ļ�            */
    
    iFdArry[0] = iFd;
    iFdArry[1] = iFd;
    iFdArry[2] = iFd;
    
    iError = __tshellBgCreateEx(iFdArry, bClosed, pcCommand, stCommandLen, 
                                ulKeywordOpt, bIsJoin, ulMagic, LW_NULL, &iRet);
    if (iError < 0) {
        return  (iError);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __tshellCloseFd
** ��������: ttiny shell �ں��߳��˳�ʱ��Ҫ����Ƿ���Ҫ�ر��ļ�
** �䡡��  : ptcb          shell TCB
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __tshellCloseFd (PLW_CLASS_TCB  ptcb)
{
    INT     iFd;

    if (ptcb->TCB_iDeleteProcStatus == LW_TCB_DELETE_PROC_DEL) {        /*  ���ڱ�ɾ��                  */
        if (__TTINY_SHELL_GET_OPT(ptcb) & LW_OPTION_TSHELL_CLOSE_FD) {
            iFd = __TTINY_SHELL_GET_STDFILE(ptcb);
            if (iFd >= 0) {
                close(iFd);
            }
        }
    }
}
/*********************************************************************************************************
** ��������: __tshellThread
** ��������: ttiny shell �����߳�
** �䡡��  : pcArg      �����ն��ļ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID   __tshellThread (PVOID  pcArg)
{
             FUNCPTR        pfuncRunCallback = LW_NULL;
             PLW_CLASS_TCB  ptcbCur;
    REGISTER INT            iTtyFd = (INT)(LONG)pcArg;
             INT            iRetValue;
             CHAR           cRecvBuffer[LW_CFG_SHELL_MAX_COMMANDLEN + 1];
             CHAR           cCtrl[NCCS];
             INT            iReadNum;
             INT            iTotalNum = 0;
             BOOL           bIsCommandOver = LW_TRUE;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __TTINY_SHELL_SET_STDFILE(ptcbCur, iTtyFd);                         /*  �����������б����׼�ļ�    */

    API_ThreadCleanupPush(__tshellCloseFd, ptcbCur);                    /*  ��ʼ���������              */

    if (ioPrivateEnv() < 0) {                                           /*  ʹ��˽���߳� io ����        */
        exit(-1);
    }
    
    if (iTtyFd >= 0) {
        LW_OBJECT_HANDLE  ulMe = API_ThreadIdSelf();
        API_IoTaskStdSet(ulMe, STD_IN,  iTtyFd);                        /*  I/O �ض���                  */
        API_IoTaskStdSet(ulMe, STD_OUT, iTtyFd);
        API_IoTaskStdSet(ulMe, STD_ERR, iTtyFd);
    }
    
    if (!isatty(iTtyFd)) {                                              /*  ����Ƿ�Ϊ�ն��豸          */
        exit(-1);
    }
    
    ioctl(iTtyFd, FIOSYNC);                                             /*  �ȴ�֮ǰ�������ݷ������    */
    ioctl(iTtyFd, FIORTIMEOUT, LW_NULL);                                /*  ��ȡ�޳�ʱʱ��              */
    
    if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_AUTHEN) {     /*  �Ƿ���Ҫ�����û���֤        */
        if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_LOOPLOGIN) {
__reauthen:
            if (__tshellUserAuthen(iTtyFd, LW_TRUE)) {                  /*  ������֤                    */
                goto    __reauthen;
            }
        } else {
            if (__tshellUserAuthen(iTtyFd, LW_FALSE)) {                 /*  ������֤                    */
                exit(-ERROR_TSHELL_EUSER);
            }
        }
    } else {
        printf("\n");
    }
    
    if (API_TShellGetUserHome(getuid(),                                 /*  �л���ǰ����Ŀ¼            */
                              cRecvBuffer,
                              LW_CFG_SHELL_MAX_COMMANDLEN + 1) == ERROR_NONE) {
        if (chdir(cRecvBuffer) < ERROR_NONE) {
            chdir(PX_STR_ROOT);
        }
    } else {
        chdir(PX_STR_ROOT);                                             /*  ��ʼ��Ϊ��Ŀ¼              */
    }

    if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_NOECHO) {     /*  ����Ҫ����                  */
        iRetValue = ioctl(iTtyFd, FIOSETOPTIONS, 
                          (OPT_TERMINAL & (~OPT_ECHO)));                /*  ����Ϊ�ն�ģʽ              */
    } else {
        iRetValue = ioctl(iTtyFd, FIOSETOPTIONS, OPT_TERMINAL);         /*  ����Ϊ�ն�ģʽ              */
    }
    if (iRetValue < 0) {
        perror("shell can not change into a TERMINAL mode");
        exit(-1);
    }
    
    iRetValue = ioctl(iTtyFd, FIORBUFSET, 
                      LW_OSIOD_LARG(LW_CFG_SHELL_MAX_COMMANDLEN + 2));  /*  ���ý��ջ�������С          */
                                                                        /*  Ϊ ty ���������� 2 �ֽڿռ� */
    if (iRetValue < 0) {
        perror("shell can not change set read buffer size");
        exit(-1);
    }
    
    iRetValue = ioctl(iTtyFd, FIOWBUFSET, 
                      LW_OSIOD_LARG(LW_CFG_SHELL_MAX_COMMANDLEN));      /*  ���÷��ͻ���                */
    if (iRetValue < 0) {
        perror("shell can not change set write buffer size");
        exit(-1);
    }
    
    API_TShellSetTitle(STD_OUT, "SylixOS Terminal");                    /*  �޸ı�������                */
    API_TShellColorEnd(STD_OUT);

    pfuncRunCallback = __TTINY_SHELL_GET_CALLBACK(ptcbCur);
    if (pfuncRunCallback) {                                             /*  ���������ص�����            */
        iRetValue = pfuncRunCallback(iTtyFd, __TTINY_SHELL_GET_CBARG(ptcbCur));
        if (iRetValue < 0) {
            perror("shell run callback fail");
            exit(-1);
        }
    }
    
    if ((__TTINY_SHELL_GET_OPT(ptcbCur) & 
         LW_OPTION_TSHELL_NOLOGO) == 0) {                               /*  �Ƿ���Ҫ��ӡ logo           */
        API_SystemLogoPrint();                                          /*  ��ӡ LOGO ��Ϣ              */
        API_SystemHwInfoPrint();
    }
    
    __TTINY_SHELL_SET_ERROR(ptcbCur, 0);                                /*  ��մ�����                */
    
#if LW_CFG_THREAD_CANCEL_EN > 0
    API_ThreadSetCancelType(LW_THREAD_CANCEL_ASYNCHRONOUS, LW_NULL);
#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN     */
    
    __TTINY_SHELL_SET_MAIN(ptcbCur);
    
    ioctl(iTtyFd, FIOABORTARG,  API_ThreadIdSelf());                    /*  control-C ����              */
    ioctl(iTtyFd, FIOABORTFUNC, __tshellRestart);                       /*  control-C ��Ϊ              */
    ioctl(iTtyFd, FIOGETCC, cCtrl);                                     /*  ��ÿ����ַ�                */

    for (;;) {
        if (bIsCommandOver) {                                           /*  ���������������������ʾ��*/
            __tshellShowPrompt();                                       /*  ��ʾ������ʾ��              */
        
        } else {
            printf(">");                                                /*  ���������                  */
            fflush(stdout);                                             /*  ��֤ stdout �����          */
        }
        
        if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_NOECHO) {
            iReadNum = (INT)read(0, &cRecvBuffer[iTotalNum], 
                                 LW_CFG_SHELL_MAX_COMMANDLEN - 
                                 iTotalNum);                            /*  �����ַ���                  */
        } else {
            iReadNum = (INT)__tshellReadline(0, &cRecvBuffer[iTotalNum], 
                                             LW_CFG_SHELL_MAX_COMMANDLEN - 
                                             iTotalNum);                /*  �����ַ���                  */
        }
        
        if (iReadNum > 0) {
            iTotalNum += iReadNum;                                      /*  ��¼�ܸ���                  */
            if (cRecvBuffer[iTotalNum - 1] == '\n') {
                cRecvBuffer[iTotalNum - 1] =  PX_EOS;
                iTotalNum -= 1;
                if (cRecvBuffer[iTotalNum - 1] == '\r') {               /*  ���� \r\n                   */
                    cRecvBuffer[iTotalNum - 1] =  PX_EOS;
                    iTotalNum -= 1;
                }
            }
            if (cRecvBuffer[iTotalNum - 1] == '\\') {                   /*  �û���Ҫ��д����            */
                iTotalNum -= 1;
                bIsCommandOver = LW_FALSE;
                if ((LW_CFG_SHELL_MAX_COMMANDLEN - iTotalNum) <= 1) {   /*  û�п���, ���ܽ���          */
                    printf("command is too long.\n");
                    iTotalNum      = 0;                                 /*  ������ʧЧ                  */
                    bIsCommandOver = LW_TRUE;
                } else {
                    continue;                                           /*  �ȴ���������                */
                }
            }
        }
        
        if (iTotalNum > 0) {
            if (lib_strstr(cRecvBuffer, __TTINY_SHELL_FORCE_ABORT)) {
                break;                                                  /*  ǿ���˳� shell              */
            }
            
            optind   = 1;                                               /*  ȷ�� getopt() ִ����ȷ      */
            optreset = 1;
            
            __tshellBeforeExecution(STD_OUT);

            iRetValue = API_TShellExec(cRecvBuffer);                    /*  ִ�� shell ָ��             */

            __tshellAfterExecution(cRecvBuffer, iTotalNum, iRetValue);

#if LW_CFG_SHELL_HOOK_EN > 0
            if ((iRetValue == -ERROR_TSHELL_CMDNOTFUND) && _G_pfuncShellHook) {
                LW_SOFUNC_PREPARE(_G_pfuncShellHook);
                iRetValue = _G_pfuncShellHook(cRecvBuffer);
            }
#endif                                                                  /*  LW_CFG_SHELL_HOOK_EN > 0    */

            if (iRetValue < 0) {
                switch (iRetValue) {                                    /*  ϵͳ��������ʾ              */
                
                case -ERROR_TSHELL_EPARAM:
                    fprintf(stderr, "parameter(s) error.\n");
                    break;
                
                case -ERROR_TSHELL_CMDNOTFUND:
                    fprintf(stderr, "sh: command not found.\n");
                    break;
                    
                case -ERROR_TSHELL_EVAR:
                    fprintf(stderr, "sh: variable error.\n");
                    break;
                    
                case -ERROR_SYSTEM_LOW_MEMORY:
                    fprintf(stderr, "sh: no memory.\n");
                    break;
                }
                
                API_TShellTermAlert(STD_OUT);                           /*  ��������                    */
            }
            __TTINY_SHELL_SET_ERROR(ptcbCur, iRetValue);                /*  ��¼��ǰ��������Ĵ���.     */

            fpurge(stdin);                                              /*  ������뻺��                */

            if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_NOECHO) {
                ioctl(iTtyFd, FIOSETOPTIONS, (OPT_TERMINAL & (~OPT_ECHO)));
                
            } else {                                                    /*  ���»ָ�Ϊ��ģʽ            */
                ioctl(iTtyFd, FIOSETOPTIONS, OPT_TERMINAL);
            }
            
            ioctl(iTtyFd, FIOSETCC, cCtrl);                             /*  �ָ�֮ǰ�Ŀ����ַ�          */
        }
        
        iTotalNum      = 0;
        bIsCommandOver = LW_TRUE;
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __tshellKeywordAdd
** ��������: �� ttiny shell ϵͳ���һ���ؼ���.
** �䡡��  : pcKeyword     �ؼ���
**           stStrLen      �ؼ��ֵĳ���
**           pfuncCommand  ִ�е� shell ����
**           ulOption      ѡ��
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellKeywordAdd (CPCHAR  pcKeyword, size_t  stStrLen, 
                           PCOMMAND_START_ROUTINE  pfuncCommand, ULONG   ulOption)
{
    REGISTER __PTSHELL_KEYWORD    pskwNode         = LW_NULL;           /*  �ؼ��ֽڵ�                  */
    REGISTER PLW_LIST_LINE       *pplineHashHeader = LW_NULL;
    REGISTER INT                  iHashVal;
    
    /*
     *  �����ڴ�
     */
    pskwNode = (__PTSHELL_KEYWORD)__SHEAP_ALLOC(sizeof(__TSHELL_KEYWORD));
    if (!pskwNode) {                                                    /*  ����ʧ��                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);                              /*  ȱ���ڴ�                    */
    }
    
    pskwNode->SK_pcKeyword = (PCHAR)__SHEAP_ALLOC(stStrLen + 1);
    if (!pskwNode->SK_pcKeyword) {
        __SHEAP_FREE(pskwNode);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);                              /*  ȱ���ڴ�                    */
    }
    
    iHashVal = __hashHorner(pcKeyword, LW_CFG_SHELL_KEY_HASH_SIZE);     /*  ȷ��һ��ɢ�е�λ��          */
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    /*
     *  ��д��Ϣ
     */
    pskwNode->SK_pfuncCommand   = pfuncCommand;                         /*  ��¼�ص�����                */
    pskwNode->SK_ulOption       = ulOption;                             /*  ��¼����ѡ��                */
    pskwNode->SK_pcFormatString = LW_NULL;                              /*  ��ʽ���ִ�                  */
    pskwNode->SK_pcHelpString   = LW_NULL;                              /*  �����ֶ�                    */
    lib_strlcpy(pskwNode->SK_pcKeyword, pcKeyword, 
                (LW_CFG_SHELL_MAX_KEYWORDLEN + 1));                     /*  ���ƹؼ���                  */

    /*
     *  �����������
     */
    _List_Line_Add_Ahead(&pskwNode->SK_lineManage, 
                         &_G_plineTSKeyHeader);                         /*  ��������ͷ                */
     
    /*
     *  �����ϣ��
     */
    pplineHashHeader = &_G_plineTSKeyHeaderHashTbl[iHashVal];
    
    _List_Line_Add_Ahead(&pskwNode->SK_lineHash, 
                         pplineHashHeader);                             /*  ������Ӧ�ı�                */
    
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellKeywordFine
** ��������: �� ttiny shell ϵͳ����һ���ؼ���.
** �䡡��  : pcKeyword     �ؼ���
**           ppskwNode     �ؼ��ֽڵ�˫ָ��
** �䡡��  : 0: ��ʾִ�гɹ� -1: ��ʾִ��ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellKeywordFind (CPCHAR  pcKeyword, __PTSHELL_KEYWORD   *ppskwNode)
{
    REGISTER PLW_LIST_LINE        plineHash;
    REGISTER __PTSHELL_KEYWORD    pskwNode = LW_NULL;                   /*  �ؼ��ֽڵ�                  */
    REGISTER INT                  iHashVal;

    iHashVal = __hashHorner(pcKeyword, LW_CFG_SHELL_KEY_HASH_SIZE);     /*  ȷ��һ��ɢ�е�λ��          */
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    plineHash = _G_plineTSKeyHeaderHashTbl[iHashVal];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
        
        pskwNode = _LIST_ENTRY(plineHash, __TSHELL_KEYWORD, 
                               SK_lineHash);                            /*  ��ÿ��ƿ�                  */
        
        if (lib_strcmp(pcKeyword, pskwNode->SK_pcKeyword) == 0) {       /*  �ؼ�����ͬ                  */
            break;
        }
    }

    if (plineHash == LW_NULL) {
        __TTINY_SHELL_UNLOCK();                                         /*  �ͷ���Դ                    */
        _ErrorHandle(ERROR_TSHELL_EKEYWORD);
        return  (ERROR_TSHELL_EKEYWORD);                                /*  û���ҵ��ؼ���              */
    }

    *ppskwNode = pskwNode;

    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellKeywordList
** ��������: ��������е����йؼ��ֿ��ƿ��ַ
** �䡡��  : pskwNodeStart   ��ʼ�ڵ��ַ, NULL ��ʾ��ͷ��ʼ
**           ppskwNode[]     �ڵ��б�
**           iMaxCounter     �б��п��Դ�ŵ����ڵ�����
** �䡡��  : ��ʵ��ȡ�Ľڵ�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellKeywordList (__PTSHELL_KEYWORD   pskwNodeStart,
                            __PTSHELL_KEYWORD   ppskwNode[],
                            INT                 iMaxCounter)
{
    REGISTER INT                  i = 0;
    
    REGISTER PLW_LIST_LINE        plineNode;
    REGISTER __PTSHELL_KEYWORD    pskwNode = pskwNodeStart;             /*  �ؼ��ֽڵ�                  */
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    if (pskwNode == LW_NULL) {
        plineNode = _G_plineTSKeyHeader;
    } else {
        plineNode = _list_line_get_next(&pskwNode->SK_lineManage);
    }
    
    for (;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  ��ʼ����                    */
        
        pskwNode = _LIST_ENTRY(plineNode, __TSHELL_KEYWORD, 
                               SK_lineManage);                          /*  ��ÿ��ƿ�                  */
        
        ppskwNode[i++] = pskwNode;                                      /*  ����                        */
        
        if (i >= iMaxCounter) {                                         /*  �Ѿ���������                */
            break;
        }
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    return  (i);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
