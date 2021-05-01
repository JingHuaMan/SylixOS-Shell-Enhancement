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
** ��   ��   ��: loader_shell.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 04 �� 17 ��
**
** ��        ��: loader shell interface. 

** BUG:
2010.05.05  ֧��ʹ�� PATH ����������ѯ��ִ���ļ���·��.
2010.05.14  __ldGetFilePath() ������ linux ��ִ���ļ������ʽ����.
2010.10.24  �޸�ע��.
2011.02.20  ���� __tshellLd() ����, ת��ģ��ʱʹ�� LW_OPTION_LOADER_SYM_GLOBAL ��ʽ.
            �� exec ������Ҫ���ʻ�������, �����Ĳ���Ҫ.
2011.03.02  ��̭ ld ����, ���� modulereg �� moduleunreg, ������ϵͳ�ں�ע��ͽ��ע��ģ��.
            ������ʾ���� module �Ĺ���.
2011.03.26  ʹ�� LW_LD_DEFAULT_ENTRY �滻 "main".
2011.05.18  ���� libreg �� libunreg ����, ע���ж��ȫ�ֹ��������.
            ��ʼ�� symbol table ���һص�.
2011.06.09  ���� __ldPathIsFile() ���ж��Ƿ�Ϊ reg �ļ�.
2011.07.29  ȥ�� libreg �ӿ�, ���еĶ�̬���ӿ�, �� linux ��ͬ, ������������һ������.
2011.11.02  ����� vp patch �������ڴ�ʹ�����Ĵ�ӡ.
2011.11.09  �Խ����ڴ��ͳ����Ϣ, ʹ�� API_VmmPCountInArea() �ӿ�.
2012.04.12  ע���ں�ģ��, ��Ϊû��Ȩ�޴���ʱ, ��ӡ��ش���.
2012.04.26  ϵͳ reboot ʱ, ��Ҫ���� moduleRebootHook() ����.
2012.05.10  ��ʼ��ʱ������ں˷��ű����ص�.
2012.08.24  ���� ps ����.
2012.09.29  ���ģ���ļ���Ϣʱ, ��Ҫ���ж��ļ���Ч���������ļ�.
2012.10.18  __tshellExec() ֧�����нű�����.
2012.12.09  ps �������Խ��̸��׵Ĵ�ӡ.
2012.12.17  __ldGetFilePath �������·��.
2012.12.22  ps �� modules �����ڲ���Ҫ����, ������ģ����, ����װ�ػ�ж�ؽ���ʱ�鿴������Ϣ�����.
2013.01.18  ���� which �ڽ�����.
2013.06.05  ����ʹ�ù�������, ��ʾ�����ڴ�������ʱ, ʹ���µĻ���.
2013.06.07  ���� dlrefresh ����, ���¶�̬�����Ӧ��ǰ, ��Ҫ���д�����.
2013.06.13  shell �����Ľ���, ��ʼ���ȼ�Ϊ posix Ĭ�����ȼ�.
            ps �������Խ��������ʾ.
2013.12.04  ���� loader ��ʼ������.
2014.04.21  �ں�ģ������ж�ش�ӡ���·����.
2014.05.02  ���� lsmod ����.
2017.02.06  ps������ʾ����״̬. (������)
2017.12.26  ���� 64 λ��ӡ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "pwd.h"
#include "../include/loader_lib.h"
/*********************************************************************************************************
  �����б�
*********************************************************************************************************/
extern LW_LIST_LINE_HEADER      _G_plineVProcHeader;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_RMMOD_ATREBOOT_EN > 0
VOID          moduleRebootHook(INT  iRebootType);
#endif

PVOIDFUNCPTR  __symbolFindHookSet(PVOIDFUNCPTR  pfuncSymbolFindHook);
VOIDFUNCPTR   __symbolTraverseHookSet(VOIDFUNCPTR  pfuncSymbolTraverseHook);

PVOID         __moduleFindKernelSymHook(CPCHAR  pcSymName, INT  iFlag);
VOID          __moduleTraverseKernelSymHook(BOOL (*pfuncCb)(PVOID, PLW_SYMBOL), PVOID  pvArg);
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#if LW_CFG_CPU_WORD_LENGHT == 64
static const CHAR               _G_cModuleInfoHdr[] = "\n\
            NAME               HANDLE       TYPE  GLB       BASE         SIZE   SYMCNT\n\
------------------------- ---------------- ------ --- ---------------- -------- ------\n";
#else
static const CHAR               _G_cModuleInfoHdr[] = "\n\
            NAME           HANDLE   TYPE  GLB   BASE     SIZE   SYMCNT\n\
------------------------- -------- ------ --- -------- -------- ------\n";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
static const CHAR               _G_cVProcInfoHdr[] = "\n\
      NAME            FATHER      STAT  PID   GRP    MEMORY    UID   GID   USER\n\
---------------- ---------------- ---- ----- ----- ---------- ----- ----- ------\n";
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: __ldPathIsFile
** ��������: �ж��ļ���·���Ƿ�Ϊ�ļ�
** �䡡��  : pcParam       �û�·������
**           pstatFile     ��ȡ�ļ� stat
** �䡡��  : BOOL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __ldPathIsFile (CPCHAR  pcName, struct stat *pstatFile)
{
    struct stat     statFs;
    
    if (stat(pcName, &statFs) >= 0) {
        if (S_ISREG(statFs.st_mode)) {
            if (pstatFile) {
                *pstatFile = statFs;
            }
            return  (LW_TRUE);
        }
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __ldGetFilePath
** ��������: ��ÿ�ִ���ļ���·��
** �䡡��  : pcParam       �û�·������
**           pcPathBuffer  ���ҵ����ļ�·������
**           stMaxLen      ��������С
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __ldGetFilePath (CPCHAR  pcParam, PCHAR  pcPathBuffer, size_t  stMaxLen)
{
    CHAR    cBuffer[MAX_FILENAME_LENGTH];
    
    PCHAR   pcStart;
    PCHAR   pcDiv;
    
    if (stMaxLen < 2) {                                                 /*  ��������С����              */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (lib_strchr(pcParam, '/')) {                                     /*  ��һ��·��                  */
        if (__ldPathIsFile(pcParam, LW_NULL)) {                         /*  ֱ��ʹ�ò�������            */
            _PathGetFull(pcPathBuffer, stMaxLen, pcParam);              /*  �������·��                */
            return  (ERROR_NONE);
        }
    
    } else {                                                            /*  ����һ��·��                */
        if (lib_getenv_r("PATH", cBuffer, MAX_FILENAME_LENGTH)
            != ERROR_NONE) {                                            /*  PATH ��������ֵΪ��         */
            _ErrorHandle(ENOENT);                                       /*  �޷��ҵ��ļ�                */
            return  (PX_ERROR);
        }
        
        pcPathBuffer[stMaxLen - 1] = PX_EOS;
        
        pcDiv = cBuffer;                                                /*  �ӵ�һ��������ʼ��          */
        do {
            pcStart = pcDiv;
            pcDiv   = lib_strchr(pcStart, ':');                         /*  ������һ�������ָ��        */
            if (pcDiv) {
                *pcDiv = PX_EOS;
                pcDiv++;
            }
            
            snprintf(pcPathBuffer, stMaxLen, "%s/%s", pcStart, pcParam);/*  �ϲ�Ϊ������Ŀ¼            */
            if (__ldPathIsFile(pcPathBuffer, LW_NULL)) {                /*  ���ļ����Ա�����            */
                return  (ERROR_NONE);
            }
        } while (pcDiv);
    }
    
    _ErrorHandle(ENOENT);                                               /*  �޷��ҵ��ļ�                */
    return  (PX_ERROR);
}
/*********************************************************************************************************
  ���º�����Ҫ shell ֧��
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
/*********************************************************************************************************
** ��������: __tshellWhich
** ��������: ϵͳ���� "which"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellWhich (INT  iArgC, PCHAR  *ppcArgV)
{
    CHAR    cFilePath[MAX_FILENAME_LENGTH];

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (__ldGetFilePath(ppcArgV[1], cFilePath, MAX_FILENAME_LENGTH) != ERROR_NONE) {
        fprintf(stderr, "can not find file!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    printf("%s\n", cFilePath);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ldRunScript
** ��������: ��ÿ�ִ���ļ���·��
** �䡡��  : pcSheBangLine shebang line (�Ѿ�ȥ�� #! ����)
**           iArgC         ��������
**           ppcArgV       ������ 
                           �˲����� shell ϵͳ����, ��������鳤��Ϊ LW_CFG_SHELL_MAX_PARAMNUM + 1
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ldRunScript (PCHAR   pcSheBangLine, INT  iArgC, PCHAR  *ppcArgV)
{
#define __IS_WHITE(c)       (c == ' ' || c == '\t' || c == '\r' || c == '\n')
#define __IS_END(c)         (c == PX_EOS)
#define __SKIP_WHITE(str)   while (__IS_WHITE(*str)) {  \
                                str++;  \
                            }
#define __NEXT_WHITE(str)   while (!__IS_WHITE(*str) && !__IS_END(*str)) { \
                                str++;  \
                            }
#define __LAST_WHITE(str)   while (!__IS_END(*str)) {   \
                                str++;  \
                            }   \
                            str--;  \
                            while (__IS_WHITE(*str)) {  \
                                str--;  \
                            }   \
                            str++;
                            
    PCHAR      pcTemp = pcSheBangLine;
    PCHAR      pcCmd;
    PCHAR      pcArg;
    INT        i;
    
    /*
     *  ��ʱ�� argv �ṹ����:
     *
     *  +-------------+
     *  |    exec     |  0
     *  +-------------+
     *  | script file |  1
     *  +-------------+
     *  |    <arg>    |  2
     *  +-------------+
     *  |     ...     |  ...
     *  +-------------+
     *  |     NULL    |  ...
     *  +-------------+
     */
     
    pcTemp = pcSheBangLine;
    
    __SKIP_WHITE(pcTemp);                                               /*  ���Կո�                    */
    if (__IS_END(*pcTemp)) {
        return  (-ERROR_TSHELL_CMDNOTFUND);                             /*  �޷��ҵ�����                */
    }
    
    pcCmd = pcTemp;                                                     /*  ��������ʼ                  */
    ppcArgV[0] = pcCmd;                                                 /*  �滻���� 0 Ϊ cmd           */
    
    /*
     *  ��ʱ�� argv �ṹ����:
     *
     *  +-------------+
     *  |    pcCmd    |  0
     *  +-------------+
     *  | script file |  1
     *  +-------------+
     *  |    <arg>    |  2
     *  +-------------+
     *  |     ...     |  ...
     *  +-------------+
     *  |     NULL    |  ...
     *  +-------------+
     */
    
    __NEXT_WHITE(pcTemp);
    if (__IS_END(*pcTemp)) {                                            /*  û�а󶨲���                */
        return  (moduleRunEx(pcCmd, LW_LD_DEFAULT_ENTRY, 
                             iArgC, (CPCHAR *)ppcArgV, LW_NULL));
    }
    
    *pcTemp = PX_EOS;                                                   /*  �������                    */
    pcTemp++;
    __SKIP_WHITE(pcTemp);                                               /*  ���Կո�                    */
    if (__IS_END(*pcTemp)) {                                            /*  û�а󶨲���                */
        return  (moduleRunEx(pcCmd, LW_LD_DEFAULT_ENTRY, 
                             iArgC, (CPCHAR *)ppcArgV, LW_NULL));
    }
    
    pcArg = pcTemp;                                                     /*  �󶨲�����ʼ                */
    __LAST_WHITE(pcTemp);
    *pcTemp = PX_EOS;                                                   /*  �󶨲�������                */
    
    if ((iArgC + 1) > LW_CFG_SHELL_MAX_PARAMNUM) {
        return  (-ERROR_TSHELL_EPARAM);                                 /*  �ܲ���̫��                  */
    }
    
    for (i = iArgC - 1; i > 0; i--) {                                   /*  �ӵ�2��������ʼ, �������   */
        ppcArgV[i + 1] = ppcArgV[i];
    }
    iArgC++;
    ppcArgV[iArgC] = LW_NULL;
    
    /*
     *  ��ʱ�� argv �ṹ����:
     *
     *  +-------------+
     *  |    pcCmd    |  0
     *  +-------------+
     *  |             |  1
     *  +-------------+
     *  | script file |  2
     *  +-------------+
     *  |    <arg>    |  3
     *  +-------------+
     *  |     ...     |  ...
     *  +-------------+
     *  |     NULL    |  ...
     *  +-------------+
     */
     
    ppcArgV[1] = pcArg;                                                 /*  �滻���� 1 Ϊ�󶨲���       */
    
    /*
     *  ��ʱ�� argv �ṹ����:
     *
     *  +-------------+
     *  |    pcCmd    |  0
     *  +-------------+
     *  |  bang arg   |  1
     *  +-------------+
     *  | script file |  2
     *  +-------------+
     *  |    <arg>    |  3
     *  +-------------+
     *  |     ...     |  ...
     *  +-------------+
     *  |     NULL    |  ...
     *  +-------------+
     */
     
    return  (moduleRunEx(pcCmd, LW_LD_DEFAULT_ENTRY, iArgC, (CPCHAR *)ppcArgV, LW_NULL));
}
/*********************************************************************************************************
** ��������: __tshellExec
** ��������: ϵͳ���� "exec" �������һ���Ǳ������е�, ���߳̽���Ϊ���̵����߳�.
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellExec (INT  iArgC, PCHAR  *ppcArgV)
{
    CHAR          cFilePath[MAX_FILENAME_LENGTH];
    CHAR          cSheBangBuffer[MAX_FILENAME_LENGTH];
    PCHAR         pcSheBangLine;
    FILE         *pfile;
    struct stat   statBuf;
    
    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (__ldGetFilePath(ppcArgV[1], cFilePath, MAX_FILENAME_LENGTH) != ERROR_NONE) {
        return  (-ERROR_TSHELL_CMDNOTFUND);                             /*  �޷��ҵ�����                */
    }
    
    pfile = fopen(cFilePath, "r");
    if (!pfile) {
        return  (-ERROR_TSHELL_CMDNOTFUND);                             /*  �޷��ҵ�����                */
    }
    
    if (fstat(fileno(pfile), &statBuf) < ERROR_NONE) {                  /*  ����ļ���Ϣ                */
        return  (-ERROR_TSHELL_CMDNOTFUND);                             /*  �޷��ҵ�����                */
    }
    
    if (_IosCheckPermissions(O_RDONLY, LW_TRUE, statBuf.st_mode,        /*  ����ļ�ִ��Ȩ��            */
                             statBuf.st_uid, statBuf.st_gid) < ERROR_NONE) {
        fclose(pfile);
        fprintf(stderr, "insufficient permissions!\n");
        return  (PX_ERROR);
    }
    
#if LW_CFG_POSIX_EN > 0
    API_ThreadSetPriority(API_ThreadIdSelf(), LW_PRIO_NORMAL);
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
    
    pcSheBangLine = fgets(cSheBangBuffer, MAX_FILENAME_LENGTH, pfile);  /*  ����ļ��ĵ�һ��            */
    fclose(pfile);
    
    if (pcSheBangLine) {
        if (pcSheBangLine[0] == '#' && pcSheBangLine[1] == '!') {       /*  �ǽű������ļ�              */
            return  (__ldRunScript(&pcSheBangLine[2], iArgC, ppcArgV)); /*  ִ�нű��ļ�                */
        }
    }
    
    return  (moduleRunEx(cFilePath, LW_LD_DEFAULT_ENTRY, 
                         iArgC - 1, (CPCHAR *)&ppcArgV[1], LW_NULL));
}
/*********************************************************************************************************
** ��������: __tshellDlConfig
** ��������: ϵͳ���� "dlconfig"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellDlConfig (INT  iArgC, PCHAR  *ppcArgV)
{
    PCHAR   pcFileName = LW_NULL;
    BOOL    bShare;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (lib_strcmp(ppcArgV[1], "refresh") == 0) {
        if (iArgC >= 3) {
            pcFileName = ppcArgV[2];
        }
        
        return  (moduleShareRefresh(pcFileName));
    
    } else if (lib_strcmp(ppcArgV[1], "share") == 0) {
        if (iArgC < 3) {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        if ((ppcArgV[2][0] == 'e') ||
            (ppcArgV[2][0] == 'y') ||
            (ppcArgV[2][0] == '1')) {
            bShare = LW_TRUE;
        } else {
            bShare = LW_FALSE;
        }
        
        return  (moduleShareConfig(bShare));
    
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
}
/*********************************************************************************************************
** ��������: __tshellModuleReg
** ��������: ϵͳ���� "modulereg"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellModuleReg (INT  iArgC, PCHAR  *ppcArgV)
{
    PVOID   pvModule;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    pvModule = moduleRegister(ppcArgV[1]);                              /*  ���ں�ע��ģ��              */
    if (pvModule) {
        printf("module %s register ok, handle: 0x%lx\n", ppcArgV[1], (addr_t)pvModule);
        return  (ERROR_NONE);
    
    } else {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not register module, error: %s\n", lib_strerror(errno));
        }
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __tshellModuleUnReg
** ��������: ϵͳ���� "moduleunreg"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellModuleUnreg (INT  iArgC, PCHAR  *ppcArgV)
{
    ULONG               ulModule = 0ul;
    LW_LD_EXEC_MODULE  *pmod     = LW_NULL; 
    CHAR                cModule[MAX_FILENAME_LENGTH];
    INT                 iError;

    BOOL                bStart;

    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (sscanf(ppcArgV[1], "%lx", &ulModule) != 1) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    /*
     *  �жϲ����Ƿ�Ϊ��Ч���ں�ģ����
     */
    pvproc = _LIST_ENTRY(_G_plineVProcHeader, LW_LD_VPROC, VP_lineManage);
    LW_VP_LOCK(pvproc);
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodTemp == (LW_LD_EXEC_MODULE *)ulModule) {
            pmod = (LW_LD_EXEC_MODULE *)ulModule;
            break;
        }
    }
    LW_VP_UNLOCK(pvproc);

    if ((pmod == LW_NULL) ||
        (pmod->EMOD_ulMagic != __LW_LD_EXEC_MODULE_MAGIC)) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    lib_strlcpy(cModule, pmod->EMOD_pcModulePath, MAX_FILENAME_LENGTH);
    
    iError = moduleUnregister((PVOID)ulModule);                         /*  ���ں�ж��ģ��              */
    if (iError == ERROR_NONE) {
        printf("module %s unregister ok.\n", cModule);
        return  (ERROR_NONE);
    
    } else {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not unregister module, error: %s\n", lib_strerror(errno));
        }
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __tshellInsModule
** ��������: ϵͳ���� "insmod"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellInsModule (INT  iArgC, PCHAR  *ppcArgV)
{
    PVOID   pvModule;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (iArgC == 2) {
        return  (__tshellModuleReg(iArgC, ppcArgV));
    
    } else if (!lib_strcmp(ppcArgV[1], "-x")) {
        pvModule = API_ModuleLoadEx(ppcArgV[2],
                                    LW_OPTION_LOADER_SYM_GLOBAL,
                                    LW_MODULE_INIT,
                                    LW_MODULE_EXIT,
                                    LW_NULL,
                                    ".????????????????",
                                    LW_NULL);
        if (pvModule) {
            printf("module %s register ok, handle: 0x%lx\n", ppcArgV[1], (addr_t)pvModule);
            return  (ERROR_NONE);
        
        } else {
            if (errno == EACCES) {
                fprintf(stderr, "insufficient permissions.\n");
            } else {
                fprintf(stderr, "can not register module, error: %s\n", lib_strerror(errno));
            }
            return  (PX_ERROR);
        }
    
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
}
/*********************************************************************************************************
** ��������: __tshellRmModule
** ��������: ϵͳ���� "rmmod"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellRmModule (INT  iArgC, PCHAR  *ppcArgV)
{
    LW_LD_EXEC_MODULE  *pmod = LW_NULL; 
    CHAR                cModule[MAX_FILENAME_LENGTH];
    INT                 iError;

    BOOL                bStart;

    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    /*
     *  �жϲ����Ƿ�Ϊ��Ч���ں�ģ����
     */
    pvproc = _LIST_ENTRY(_G_plineVProcHeader, LW_LD_VPROC, VP_lineManage);
    LW_VP_LOCK(pvproc);
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        
        if (!lib_strcmp(_PathLastNamePtr(pmodTemp->EMOD_pcModulePath),
                        _PathLastNamePtr(ppcArgV[1]))) {
            pmod = pmodTemp;
            break;
        }
    }
    LW_VP_UNLOCK(pvproc);

    if ((pmod == LW_NULL) ||
        (pmod->EMOD_ulMagic != __LW_LD_EXEC_MODULE_MAGIC)) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    lib_strlcpy(cModule, pmod->EMOD_pcModulePath, MAX_FILENAME_LENGTH);
    
    iError = moduleUnregister((PVOID)pmod);                             /*  ���ں�ж��ģ��              */
    if (iError == ERROR_NONE) {
        printf("module %s unregister ok.\n", cModule);
        return  (ERROR_NONE);
    
    } else {
        if (errno == EACCES) {
            fprintf(stderr, "insufficient permissions.\n");
        } else {
            fprintf(stderr, "can not unregister module, error: %s\n", lib_strerror(errno));
        }
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __tshellModulestat
** ��������: ϵͳ���� "modulestat"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellModulestat (INT  iArgC, PCHAR  *ppcArgV)
{
    struct stat  statGet;

    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (stat(ppcArgV[1], &statGet) < 0) {
        fprintf(stderr, "can not open %s : %s\n", ppcArgV[1], lib_strerror(errno));
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!S_ISREG(statGet.st_mode)) {
        fprintf(stderr, "not a REG file.\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    return  (moduleStatus(ppcArgV[1], STD_OUT));
}
/*********************************************************************************************************
** ��������: __tshellModuleShow
** ��������: ϵͳ���� "modules"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellModuleShow (INT  iArgC, PCHAR  *ppcArgV)
{
    INT                 iCnt = 0;
    PCHAR               pcFileName;
    PCHAR               pcProcessName;
    PCHAR               pcModuleName;
    BOOL                bStart;
    
    LW_LIST_LINE       *plineTemp;
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    
    size_t              stStatic;
    size_t              stHeapMem;
    size_t              stMmapMem;

    CHAR                cVpVersion[128] = "";
    
    if (iArgC < 2) {
        pcFileName = LW_NULL;
    } else {
        pcFileName = ppcArgV[1];
    }
    
    printf(_G_cModuleInfoHdr);
    LW_LD_LOCK();
    for (plineTemp  = _G_plineVProcHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pvproc = _LIST_ENTRY(plineTemp, LW_LD_VPROC, VP_lineManage);
        
        _PathLastName((PCHAR)pvproc->VP_pcName, &pcProcessName);
        
        if (pcFileName) {
            if (lib_strcmp(pcFileName, pcProcessName) != 0) {
                continue;
            }
        }
        
        vprocMemInfoNoLock(pvproc, &stStatic, &stHeapMem, &stMmapMem);

        printf("VPROC: %-18s pid:%4d TOTAL MEM: %zu ",
               pcProcessName,
               pvproc->VP_pid,
               stStatic + stHeapMem + stMmapMem);
               
        if (cVpVersion[0] != PX_EOS) {
            printf("<vp ver:%s>\n", cVpVersion);
        
        } else {
            printf("\n");
        }
               
        /*
         *  ��ӡģ����Ϣ
         */
        LW_VP_LOCK(pvproc);
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

            _PathLastName(pmodTemp->EMOD_pcModulePath, &pcModuleName);

#if LW_CFG_CPU_WORD_LENGHT == 64
            printf("+ %-23s %16lx %-6s %-3s %16lx %8lx %6ld\n",
#else
            printf("+ %-23s %08lx %-6s %-3s %08lx %8lx %6ld\n",
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
                   pcModuleName,
                   (addr_t)pmodTemp,
                   (((pmodTemp->EMOD_bIsGlobal) && (pmodTemp->EMOD_pcSymSection)) ? "KERNEL" : "USER"),
                   ((pmodTemp->EMOD_bIsGlobal) ? "YES" : "NO"),
                   (addr_t)pmodTemp->EMOD_pvBaseAddr,
                   (ULONG)pmodTemp->EMOD_stLen,
                   (ULONG)pmodTemp->EMOD_ulSymCount);
            iCnt++;
        }
        LW_VP_UNLOCK(pvproc);
    }
    LW_LD_UNLOCK();

    printf("\n");
    printf("total modules: %d\n", iCnt);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellVProcShow
** ��������: ϵͳ���� "ps"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellVProcShow (INT  iArgC, PCHAR  *ppcArgV)
{
    INT                 iCnt = 0;
    PCHAR               pcProcessName;
    PCHAR               pcFatherName;
    CHAR                cVprocStat;                                     /*  ����״̬                    */

    LW_LIST_LINE       *plineTemp;
    LW_LD_VPROC        *pvproc;
    
    size_t              stStatic;
    size_t              stHeapMem;
    size_t              stMmapMem;
    
    struct passwd       passwd;
    struct passwd      *ppasswd = LW_NULL;
    
    CHAR                cUserName[MAX_FILENAME_LENGTH];
    PCHAR               pcUser;
    
    uid_t               uid;
    gid_t               gid;
    
    printf(_G_cVProcInfoHdr);
    LW_LD_LOCK();
    for (plineTemp  = _G_plineVProcHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pvproc = _LIST_ENTRY(plineTemp, LW_LD_VPROC, VP_lineManage);
        
        _PathLastName((PCHAR)pvproc->VP_pcName, &pcProcessName);
        
        if (pvproc->VP_pvprocFather) {
            _PathLastName((PCHAR)pvproc->VP_pvprocFather->VP_pcName, &pcFatherName);
        } else {
            pcFatherName = "<orphan>";
        }
        
        if (pvproc->VP_pid == 0) {
            /*
             *  kernel ������ʵ�������������ϵĽ���, ֻ�Ƕ��ں�������Դ�ı�ʶ, ���߱����̵�����.
             *  ��������ʾ����״̬ʱ��һֱ��ʾ���ڳ�ʼ��״̬, Ϊ�����û��������ps������״̬ʱ
             *  kernel ������ʾ����״̬ ('R')
             */
            cVprocStat = 'R';

        } else {
            /*
             *  �жϽ��̽���״̬:
             *  1. ��������״̬: R (TASK_RUNNING), ��ִ��״̬&����״̬ (�� run_queue �������״̬)
             *  2. ����ֹͣ״̬: T (TASK_STOPPED or TASK_TRACED), ֹ̬ͣ
             *  3. ���̽�ʬ״̬: Z (TASK_DEAD - EXIT_ZOMBIE), �˳�״̬, ���̳�Ϊ��ʬ����
             *  4. ���̳�ʼ״̬: I (TASK_INIT), ����״̬
             */
            if (pvproc->VP_iStatus == __LW_VP_EXIT) {
                cVprocStat = 'Z';
            } else if (pvproc->VP_iStatus == __LW_VP_STOP) {
                cVprocStat = 'T';
            } else if (pvproc->VP_iStatus == __LW_VP_INIT) {
                cVprocStat = 'I';
            } else {
                cVprocStat = 'R';
            }
        }
        
        vprocMemInfoNoLock(pvproc, &stStatic, &stHeapMem, &stMmapMem);
        
        uid = 0;
        gid = 0;
        if (pvproc->VP_ulMainThread) {
            _ThreadUserGet(pvproc->VP_ulMainThread, &uid, &gid);
        }
        
        getpwuid_r(uid, &passwd, cUserName, sizeof(cUserName), &ppasswd);
        if (ppasswd) {
            pcUser = cUserName;
        } else {
            pcUser = "<unknown>";
        }
        
        printf("%-16s %-16s %-4c %5d %5d %8zuKB %5d %5d %s\n",
                      pcProcessName, pcFatherName, cVprocStat, pvproc->VP_pid, pvproc->VP_pidGroup,
                      (stStatic + stHeapMem + stMmapMem) / LW_CFG_KB_SIZE, uid, gid, pcUser);
        
        iCnt++;
    }
    LW_LD_UNLOCK();

    printf("\n");
    printf("total vprocess: %d\n", iCnt);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellLsmod
** ��������: ϵͳ���� "ps"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellLsmod (INT  iArgC, PCHAR  *ppcArgV)
{
    PCHAR   pcArgV[3] = {"modules", "kernel", LW_NULL};

    return  (__tshellModuleShow(2, pcArgV));
}
/*********************************************************************************************************
** ��������: __tshellModuleGcov
** ��������: ϵͳ���� "modulegcov"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_GCOV_EN > 0

static INT  __tshellModuleGcov (INT  iArgC, PCHAR  *ppcArgV)
{
    ULONG   ulModule = (ULONG)LW_NULL;
    INT     iRet;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (sscanf(ppcArgV[1], "%lx", &ulModule) != 1) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iRet = moduleGcov((PVOID)ulModule);
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_GCOV_EN */
/*********************************************************************************************************
** ��������: __ldShellInit
** ��������: ��ʼ�� loader �ڲ� shell ����.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��: 
*********************************************************************************************************/
static VOID  __ldShellInit (VOID)
{
    API_TShellKeywordAdd("which", __tshellWhich);
    API_TShellFormatAdd("which", " [program file]");
    API_TShellHelpAdd("which",   "find a program file real path.\n");
     
    API_TShellKeywordAddEx("exec", __tshellExec, LW_OPTION_KEYWORD_SYNCBG | LW_OPTION_KEYWORD_STK_MAIN);
    API_TShellFormatAdd("exec", " [program file] [arguments...]");
    API_TShellHelpAdd("exec",   "execute a program.\n");
    
    API_TShellKeywordAdd("dlconfig", __tshellDlConfig);
    API_TShellFormatAdd("dlconfig",  " {[share {en | dis}] | [refresh [*]]}");
    API_TShellHelpAdd("dlconfig",    "config dynamic loader.\n"
                                     "dlconfig share en     enable share code.\n"
                                     "dlconfig share dis    disable share code.\n"
                                     "dlconfig refresh *    clean system share segment info (* mean a file).\n"
                                     "dlconfig refresh      clean all system share segment info.\n"
                                     "NOTICE: before update share library or application\n"
                                     "        you must first run 'dlconfig refresh' command.\n");
    
    API_TShellKeywordAdd("modulereg", __tshellModuleReg);
    API_TShellFormatAdd("modulereg",  " [kernel module file *.ko]");
    API_TShellHelpAdd("modulereg",    "register a kernel module into system.\n"
                                      "NOTICE: only import LW_SYMBOL_EXPORT attribute\n"
                                      "        symbol(s) to kernel symbol table.\n");

    API_TShellKeywordAdd("moduleunreg", __tshellModuleUnreg);
    API_TShellFormatAdd("moduleunreg", " [kernel module handle]");
    API_TShellHelpAdd("moduleunreg",   "unregister a kernel module from system [DANGER! Not Safety].\n");
    
    API_TShellKeywordAdd("insmod", __tshellInsModule);
    API_TShellFormatAdd("insmod",  " [-x] [kernel module file *.ko]");
    API_TShellHelpAdd("insmod",    "register a kernel module into system.\n"
                                   "-x do not export global symbol.\n");
    
    API_TShellKeywordAdd("rmmod", __tshellRmModule);
    API_TShellFormatAdd("rmmod",  " [kernel module file *.ko]");
    API_TShellHelpAdd("rmmod",    "unregister a kernel module from system [DANGER! Not Safety].\n");
    
    API_TShellKeywordAdd("modulestat", __tshellModulestat);
    API_TShellFormatAdd("modulestat", " [program file]");
    API_TShellHelpAdd("modulestat",   "show a module file information.\n");
    
    API_TShellKeywordAdd("modules", __tshellModuleShow);
    API_TShellFormatAdd("modules", " [module name]");
    API_TShellHelpAdd("modules",   "show module information.\n");
    
#if LW_CFG_MODULELOADER_GCOV_EN > 0
    API_TShellKeywordAdd("modulegcov", __tshellModuleGcov);
    API_TShellFormatAdd("modulegcov",  " [kernel module handle]");
    API_TShellHelpAdd("modulegcov",    "generate kernel module code coverage file(*.gcda).\n");
#endif                                                                  /*  LW_CFG_MODULELOADER_GCOV_EN */

    API_TShellKeywordAdd("lsmod", __tshellLsmod);
    API_TShellHelpAdd("lsmod",    "show all kernel module.\n");
    
    API_TShellKeywordAdd("ps", __tshellVProcShow);
    API_TShellHelpAdd("ps",   "show vprocess information.\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: API_LoaderInit
** ��������: ��ʼ�� loader ���.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_LoaderInit (VOID)
{
    static BOOL     bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return;
    }
    
    bIsInit = LW_TRUE;

    _G_ulVProcMutex    = API_SemaphoreMCreate("loader_lock", LW_PRIO_DEF_CEILING, LW_OPTION_WAIT_PRIORITY |
                                              LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                              LW_OPTION_OBJECT_GLOBAL, LW_NULL);
#if LW_CFG_VMM_EN > 0
    _G_ulExecShareLock = API_SemaphoreMCreate("execshare_lock", LW_PRIO_DEF_CEILING, LW_OPTION_WAIT_PRIORITY |
                                              LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                              LW_OPTION_OBJECT_GLOBAL, LW_NULL);
#endif                                                                  /*  LW_CFG_VMM_EN > 0          */

    lib_bzero(&_G_vprocKernel, sizeof(_G_vprocKernel));
    
    _G_vprocKernel.VP_pcName = "kernel";
    _G_vprocKernel.VP_ulModuleMutex = API_SemaphoreMCreate("kvproc_lock",
                                                           LW_PRIO_DEF_CEILING,
                                                           LW_OPTION_WAIT_PRIORITY |
                                                           LW_OPTION_INHERIT_PRIORITY | 
                                                           LW_OPTION_DELETE_SAFE |
                                                           LW_OPTION_OBJECT_GLOBAL,
                                                           LW_NULL);
                                                            
    _List_Line_Add_Ahead(&_G_vprocKernel.VP_lineManage, &_G_plineVProcHeader);

    __symbolFindHookSet(__moduleFindKernelSymHook);                     /*  ��װ���ű��ѯ�ص�          */
    __symbolTraverseHookSet(__moduleTraverseKernelSymHook);
    
    _S_pfuncGetCurPid   = getpid;                                       /*  ��װ I/O ϵͳ�ص���         */
    _S_pfuncFileGet     = vprocIoFileGet;
    _S_pfuncFileDescGet = vprocIoFileDescGet;
    _S_pfuncFileDup     = vprocIoFileDup;
    _S_pfuncFileDup2    = vprocIoFileDup2;
    _S_pfuncFileRefInc  = vprocIoFileRefInc;
    _S_pfuncFileRefDec  = vprocIoFileRefDec;
    _S_pfuncFileRefGet  = vprocIoFileRefGet;
    
#if LW_CFG_MODULELOADER_RMMOD_ATREBOOT_EN > 0
    API_SystemHookAdd(moduleRebootHook, LW_OPTION_KERNEL_REBOOT);       /*  ��װϵͳ�����ص�            */
#endif
    
#if LW_CFG_SHELL_EN > 0
    __ldShellInit();
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#if LW_CFG_TRUSTED_COMPUTING_EN > 0
    bspTrustedModuleInit();                                             /*  ��ʼ�����ż���ģ��          */
#endif
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
