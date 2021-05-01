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
** ��   ��   ��: ttinyShellLib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 07 �� 27 ��
**
** ��        ��: һ����С�͵� shell ϵͳ, ʹ�� tty/pty ���ӿ�, ��Ҫ���ڵ�����򵥽���.

** BUG:
2009.08.04  ������ keyword ѡ���.
2011.03.11  ���� TCB_ulReserve0 ��Ϊ shell ��׼�ļ������ַ.
2011.06.24  TCB_ulReserve0 ��Ϊ TCB_ulShellStdFile !
*********************************************************************************************************/

#ifndef __TTINYSHELLLIB_H
#define __TTINYSHELLLIB_H

/*********************************************************************************************************
  shell ǿ���˳�����
*********************************************************************************************************/

#define __TTINY_SHELL_FORCE_ABORT   "`${@_force_\x82\x93\xa4\xe0_abort_@}$`"

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

/*********************************************************************************************************
  �����
*********************************************************************************************************/

extern  LW_OBJECT_HANDLE            _G_hTShellLock;

/*********************************************************************************************************
  �����������
*********************************************************************************************************/

#define __TTINY_SHELL_LOCK_OPT      (LW_OPTION_WAIT_PRIORITY | \
                                     LW_OPTION_INHERIT_PRIORITY | \
                                     LW_OPTION_DELETE_SAFE | \
                                     LW_OPTION_OBJECT_GLOBAL)
#define __TTINY_SHELL_LOCK()        API_SemaphoreMPend(_G_hTShellLock, LW_OPTION_WAIT_INFINITE);
#define __TTINY_SHELL_UNLOCK()      API_SemaphoreMPost(_G_hTShellLock);

/*********************************************************************************************************
  shell std file ����
*********************************************************************************************************/

#define __TTINY_SHELL_SET_STDFILE(ptcb, iFd)    ptcb->TCB_shc.SHC_ulShellStdFile = iFd;
#define __TTINY_SHELL_GET_STDFILE(ptcb)         (INT)ptcb->TCB_shc.SHC_ulShellStdFile

/*********************************************************************************************************
  shell �����ݴ�
*********************************************************************************************************/

#define __TTINY_SHELL_SET_ERROR(ptcb, iRet)     ptcb->TCB_shc.SHC_ulShellError = (ULONG)iRet
#define __TTINY_SHELL_GET_ERROR(ptcb)           (INT)ptcb->TCB_shc.SHC_ulShellError

/*********************************************************************************************************
  shell ѡ�����
*********************************************************************************************************/

#define __TTINY_SHELL_SET_OPT(ptcb, ulOption)   ptcb->TCB_shc.SHC_ulShellOption = ulOption
#define __TTINY_SHELL_GET_OPT(ptcb)             ptcb->TCB_shc.SHC_ulShellOption

/*********************************************************************************************************
  shell �ص�
*********************************************************************************************************/

#define __TTINY_SHELL_SET_CALLBACK(ptcb, pfunc) ptcb->TCB_shc.SHC_pfuncShellCallback = pfunc
#define __TTINY_SHELL_GET_CALLBACK(ptcb)        ptcb->TCB_shc.SHC_pfuncShellCallback
#define __TTINY_SHELL_SET_CBARG(ptcb, pvArg)    ptcb->TCB_shc.SHC_pvCallbackArg = pvArg
#define __TTINY_SHELL_GET_CBARG(ptcb)           ptcb->TCB_shc.SHC_pvCallbackArg

/*********************************************************************************************************
  shell ��ʷ�������
*********************************************************************************************************/

#define __TTINY_SHELL_SET_HIS(ptcb, psihc)      ptcb->TCB_shc.SHC_ulShellHistoryCtx = (addr_t)psihc
#define __TTINY_SHELL_GET_HIS(ptcb)             (__PSHELL_HISTORY_CTX)ptcb->TCB_shc.SHC_ulShellHistoryCtx

/*********************************************************************************************************
  shell ħ��
*********************************************************************************************************/

#define __TTINY_SHELL_SET_MAGIC(ptcb, ulMagic)  ptcb->TCB_shc.SHC_ulShellMagic = ulMagic
#define __TTINY_SHELL_GET_MAGIC(ptcb)           ptcb->TCB_shc.SHC_ulShellMagic

/*********************************************************************************************************
  shell ���߳�
*********************************************************************************************************/

#define __TTINY_SHELL_SET_MAIN(ptcb)            ptcb->TCB_shc.SHC_ulShellMain = ptcb->TCB_ulId
#define __TTINY_SHELL_GET_MAIN(ptcb)            ptcb->TCB_shc.SHC_ulShellMain

/*********************************************************************************************************
  KEYWORD
*********************************************************************************************************/

typedef struct __shell_keyword {
    LW_LIST_LINE             SK_lineManage;                             /*  ������˫����                */
    LW_LIST_LINE             SK_lineHash;                               /*  ��ϣ��������                */

    PCOMMAND_START_ROUTINE   SK_pfuncCommand;                           /*  ִ�к���                    */
    ULONG                    SK_ulOption;                               /*  �������ѡ��                */
    
    PCHAR                    SK_pcKeyword;                              /*  �ؼ���                      */
    PCHAR                    SK_pcFormatString;                         /*  ��ʽ�ֶ�                    */
    PCHAR                    SK_pcHelpString;                           /*  ָ������ַ���������        */
} __TSHELL_KEYWORD;
typedef __TSHELL_KEYWORD    *__PTSHELL_KEYWORD;                         /*  ָ������                    */

/*********************************************************************************************************
  �ڲ���������
*********************************************************************************************************/

ULONG  __tshellKeywordAdd(CPCHAR  pcKeyword, size_t stStrLen, PCOMMAND_START_ROUTINE  pfuncCommand, 
                          ULONG   ulOption);
ULONG  __tshellKeywordFind(CPCHAR  pcKeyword, __PTSHELL_KEYWORD   *ppskwNode);
ULONG  __tshellKeywordList(__PTSHELL_KEYWORD   pskwNodeStart,
                           __PTSHELL_KEYWORD   ppskwNode[],
                           INT                 iMaxCounter);
PVOID  __tshellThread(PVOID  pcArg);
INT    __tshellRestartEx(LW_OBJECT_HANDLE  ulThread, BOOL  bNeedAuthen);
VOID   __tshellPreTreatedBg(PCHAR  cCommand, BOOL  *pbNeedJoin, BOOL  *pbNeedAsyn);
INT    __tshellExec(CPCHAR  pcCommandExec, VOIDFUNCPTR  pfuncHook);
INT    __tshellBgCreateEx(INT               iFd[3],
                          BOOL              bClosed[3],
                          CPCHAR            pcCommand, 
                          size_t            stCommandLen, 
                          ULONG             ulKeywordOpt,
                          BOOL              bIsJoin,
                          ULONG             ulMagic,
                          LW_OBJECT_HANDLE *pulSh,
                          INT              *piRet);

/*********************************************************************************************************
  �û�����
*********************************************************************************************************/

INT     __tshellGetUserName(uid_t  uid, PCHAR  pcName, size_t  stNSize, PCHAR  pcHome, size_t  stHSize);
INT     __tshellGetGrpName(gid_t  gid, PCHAR  pcName, size_t  stSize);
VOID    __tshellFlushCache(VOID);
ULONG   __tshellUserAuthen(INT  iTtyFd, BOOL  bWaitInf);
VOID    __tshellUserCmdInit(VOID);

/*********************************************************************************************************
  shell ���� hook
*********************************************************************************************************/

#if LW_CFG_SHELL_HOOK_EN > 0
FUNCPTR __tshellThreadHook(FUNCPTR  pfuncShellHook);
#endif                                                                  /*  LW_CFG_SHELL_HOOK_EN > 0    */

/*********************************************************************************************************
  ���������ļ�
*********************************************************************************************************/

#define __TTINY_SHELL_VAR_FILE                  "/etc/profile"

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYSHELLLIB_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
